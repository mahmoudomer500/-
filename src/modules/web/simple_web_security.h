// simple_web_security.h - نظام أمان ويب مبسط
// الأسبوع الرابع من الشهر الخامس: أمان الويب
#ifndef SIMPLE_WEB_SECURITY_H
#define SIMPLE_WEB_SECURITY_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>

namespace ArabicLanguage {

// ==================== HTTP Security Server ====================

// طريقة HTTP
enum class HTTPMethod {
    GET,
    POST,
    PUT,
    DEL,  // Changed from DELETE to avoid Windows API conflict
    PATCH,
    HEAD,
    OPTIONS,
    UNKNOWN
};

// حالة HTTP
enum class HTTPStatus {
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503
};

// طلب HTTP آمن
struct SecurityHTTPRequest {
    HTTPMethod method;
    std::string path;
    std::string queryString;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> queryParams;
    std::string body;
    std::string remoteAddress;
    int remotePort;

    SecurityHTTPRequest() : method(HTTPMethod::GET), remotePort(0) {}

    std::string getHeader(const std::string& name) const;
    std::string getQueryParam(const std::string& name) const;
    bool hasHeader(const std::string& name) const;
};

// استجابة HTTP آمنة
struct SecurityHTTPResponse {
    HTTPStatus status;
    std::map<std::string, std::string> headers;
    std::string body;

    SecurityHTTPResponse() : status(HTTPStatus::OK) {}

    void setHeader(const std::string& name, const std::string& value);
    void setContentType(const std::string& type);
    void setStatus(HTTPStatus s) { status = s; }
    void setBody(const std::string& b) { body = b; }
    void setJSON(const std::string& json);
    void setText(const std::string& text);
    void setHTML(const std::string& html);

    std::string toString() const;
};

// خادم HTTP آمن بسيط
class SecureHTTPServer {
private:
    int port;
    std::string host;
    bool running;
    std::vector<std::pair<std::string, std::function<SecurityHTTPResponse(const SecurityHTTPRequest&)>>> routes;

    // إعدادات الأمان
    bool securityHeadersEnabled;
    bool xssProtectionEnabled;
    bool csrfProtectionEnabled;
    std::string csrfTokenName;
    std::map<std::string, std::string> securityHeaders;

public:
    SecureHTTPServer(int port = 8080, const std::string& host = "0.0.0.0");
    ~SecureHTTPServer();

    // إدارة الخادم
    bool start();
    void stop();
    bool isRunning() const { return running; }

    // إضافة المسارات
    void get(const std::string& path, std::function<SecurityHTTPResponse(const SecurityHTTPRequest&)> handler);
    void post(const std::string& path, std::function<SecurityHTTPResponse(const SecurityHTTPRequest&)> handler);
    void put(const std::string& path, std::function<SecurityHTTPResponse(const SecurityHTTPRequest&)> handler);
    void del(const std::string& path, std::function<SecurityHTTPResponse(const SecurityHTTPRequest&)> handler);

    // إعدادات الأمان
    void enableSecurityHeaders(bool enable = true);
    void enableXSSProtection(bool enable = true);
    void enableCSRFProtection(bool enable = true, const std::string& tokenName = "csrf_token");

    // أدوات الأمان
    std::string generateCSRFToken() const;
    bool validateCSRFToken(const SecurityHTTPRequest& request) const;
    void applySecurityHeaders(SecurityHTTPResponse& response) const;
    std::string sanitizeHTML(const std::string& input) const;
    std::string escapeHTML(const std::string& input) const;

private:
    void initializeSecurityHeaders();
    SecurityHTTPRequest parseRequest(const std::string& rawRequest);
    SecurityHTTPResponse processRequest(const SecurityHTTPRequest& request);
    std::function<SecurityHTTPResponse(const SecurityHTTPRequest&)>* findRoute(const std::string& path);
    bool matchRoute(const std::string& pattern, const std::string& path);
    std::string methodToString(HTTPMethod method) const;
    HTTPMethod stringToMethod(const std::string& str) const;
    std::string statusToString(HTTPStatus status) const;
};

// ==================== Authentication System ====================

// بيانات المستخدم
struct User {
    std::string id;
    std::string username;
    std::string passwordHash;
    std::string role; // admin, user, guest
    bool isActive;

    User() : isActive(true) {}
};

// رمز المصادقة
struct AuthToken {
    std::string token;
    std::string userId;
    std::chrono::system_clock::time_point expiresAt;

    bool isExpired() const {
        return std::chrono::system_clock::now() > expiresAt;
    }
};

// نتيجة المصادقة
struct AuthResult {
    bool success;
    std::string userId;
    std::string role;
    std::string message;
    std::string token;

    AuthResult(bool s = false) : success(s) {}
};

// مدير المصادقة المبسط
class SimpleAuthManager {
private:
    std::map<std::string, User> users; // username -> User
    std::map<std::string, AuthToken> activeTokens; // token -> AuthToken
    int tokenExpirationHours;

public:
    SimpleAuthManager();

    // إدارة المستخدمين
    bool createUser(const std::string& username, const std::string& password, const std::string& role = "user");
    User* getUserByUsername(const std::string& username);

    // المصادقة
    AuthResult authenticate(const std::string& username, const std::string& password);
    AuthResult validateToken(const std::string& token);
    bool revokeToken(const std::string& token);

    // التحقق من الصلاحيات
    bool hasRole(const std::string& userId, const std::string& role);

private:
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    std::string generateToken();
    std::string generateUserId();
    void loadDefaultUsers();
};

// دوال تشفير مساعدة
namespace SimpleCrypto {
    std::string hashSHA256(const std::string& input);
    std::string generateSalt(size_t length = 16);
    std::string hashPassword(const std::string& password, const std::string& salt);
    bool verifyPassword(const std::string& password, const std::string& hash, const std::string& salt);
}

} // namespace ArabicLanguage

#endif // SIMPLE_WEB_SECURITY_H
