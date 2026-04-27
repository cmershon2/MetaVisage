#include "utils/HttpClient.h"
#include "utils/Logger.h"

#include <curl/curl.h>

#include <atomic>
#include <cstdlib>
#include <format>
#include <mutex>

namespace MetaVisage {

namespace {

std::once_flag g_curlInitOnce;

void EnsureGlobalInit() {
    std::call_once(g_curlInitOnce, [] {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        std::atexit([] { curl_global_cleanup(); });
    });
}

size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    const size_t total = size * nmemb;
    auto* body = static_cast<std::string*>(userdata);
    body->append(ptr, total);
    return total;
}

size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    const size_t total = size * nitems;
    auto* response = static_cast<HttpResponse*>(userdata);
    constexpr const char* kPrefix = "Retry-After:";
    constexpr size_t kPrefixLen = 12;
    // Need at least one char after the colon. Some curl builds strip the
    // trailing CRLF and others don't, so we parse from the prefix end and let
    // strtol stop at the first non-digit (space, '\r', '\n', or end-of-buffer).
    if (total > kPrefixLen && _strnicmp(buffer, kPrefix, kPrefixLen) == 0) {
        const std::string value(buffer + kPrefixLen, total - kPrefixLen);
        response->retryAfterSeconds = std::strtol(value.c_str(), nullptr, 10);
    }
    return total;
}

HttpResponse Execute(const std::string& url,
                     const std::string& body,
                     const std::map<std::string, std::string>& headers,
                     bool isPost) {
    EnsureGlobalInit();
    HttpResponse response;

    CURL* curl = curl_easy_init();
    if (!curl) {
        response.errorMessage = "curl_easy_init failed";
        return response;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    if (isPost) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    }

    curl_slist* headerList = nullptr;
    for (const auto& [k, v] : headers) {
        const std::string line = k + ": " + v;
        headerList = curl_slist_append(headerList, line.c_str());
    }
    if (headerList) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

    const CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        response.errorMessage = curl_easy_strerror(rc);
        MV_LOG_WARNING(std::format("HttpClient: {} -> {}", url, response.errorMessage));
    } else {
        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        response.status = code;
    }

    if (headerList) curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);
    return response;
}

} // namespace

HttpClient::HttpClient() { EnsureGlobalInit(); }
HttpClient::~HttpClient() = default;

HttpResponse HttpClient::PostJson(const std::string& url,
                                  const std::string& jsonBody,
                                  const std::map<std::string, std::string>& headers) const {
    auto full = headers;
    full.emplace("Content-Type", "application/json");
    full.emplace("Accept", "application/json");
    return Execute(url, jsonBody, full, /*isPost=*/true);
}

HttpResponse HttpClient::PostJsonBearer(const std::string& url,
                                        const std::string& jsonBody,
                                        const std::string& bearerToken) const {
    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer " + bearerToken;
    return PostJson(url, jsonBody, headers);
}

HttpResponse HttpClient::GetJsonBearer(const std::string& url,
                                       const std::string& bearerToken) const {
    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer " + bearerToken;
    headers["Accept"] = "application/json";
    return Execute(url, /*body=*/{}, headers, /*isPost=*/false);
}

} // namespace MetaVisage
