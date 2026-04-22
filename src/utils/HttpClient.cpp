#include "utils/HttpClient.h"
#include "utils/Logger.h"

#include <curl/curl.h>

#include <atomic>
#include <cstdlib>
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
    const std::string line(buffer, total);
    const char* prefix = "Retry-After:";
    if (line.size() > 13 && _strnicmp(line.c_str(), prefix, 12) == 0) {
        response->retryAfterSeconds = std::strtol(line.c_str() + 12, nullptr, 10);
    }
    return total;
}

HttpResponse Execute(const QString& url,
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

    const std::string urlUtf8 = url.toUtf8().toStdString();
    curl_easy_setopt(curl, CURLOPT_URL, urlUtf8.c_str());
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
        MV_LOG_WARNING(QString("HttpClient: %1 -> %2").arg(url, response.errorMessage.c_str()));
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

HttpResponse HttpClient::PostJson(const QString& url,
                                  const std::string& jsonBody,
                                  const std::map<std::string, std::string>& headers) const {
    auto full = headers;
    full.emplace("Content-Type", "application/json");
    full.emplace("Accept", "application/json");
    return Execute(url, jsonBody, full, /*isPost=*/true);
}

HttpResponse HttpClient::PostJsonBearer(const QString& url,
                                        const std::string& jsonBody,
                                        const QString& bearerToken) const {
    std::map<std::string, std::string> headers;
    headers["Authorization"] = std::string("Bearer ") + bearerToken.toUtf8().toStdString();
    return PostJson(url, jsonBody, headers);
}

HttpResponse HttpClient::GetJsonBearer(const QString& url,
                                       const QString& bearerToken) const {
    std::map<std::string, std::string> headers;
    headers["Authorization"] = std::string("Bearer ") + bearerToken.toUtf8().toStdString();
    headers["Accept"] = "application/json";
    return Execute(url, /*body=*/{}, headers, /*isPost=*/false);
}

} // namespace MetaVisage
