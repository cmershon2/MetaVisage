#ifndef METAVISAGE_UTILS_HTTPCLIENT_H
#define METAVISAGE_UTILS_HTTPCLIENT_H

#include <QString>
#include <map>
#include <string>

namespace MetaVisage {

struct HttpResponse {
    long status = 0;              // HTTP status, or 0 on transport failure
    std::string body;             // Raw response body (UTF-8 JSON expected)
    std::string errorMessage;     // Non-empty iff transport failed
    long retryAfterSeconds = 0;   // Parsed from Retry-After header on 429

    bool Ok() const { return errorMessage.empty() && status >= 200 && status < 300; }
};

// Thin libcurl wrapper for the licensing API. Not a general-purpose HTTP
// client — scoped to what DESKTOP_APP_INTEGRATION.md and DESKTOP_AUTH_FLOW.md
// actually need (bearer-authenticated JSON POST + unauthenticated exchange).
class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpResponse PostJson(const QString& url,
                          const std::string& jsonBody,
                          const std::map<std::string, std::string>& headers) const;

    HttpResponse PostJsonBearer(const QString& url,
                                const std::string& jsonBody,
                                const QString& bearerToken) const;

    HttpResponse GetJsonBearer(const QString& url,
                               const QString& bearerToken) const;
};

} // namespace MetaVisage

#endif
