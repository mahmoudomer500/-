// HTTPServer.h - خادم HTTP/HTTPS آمن
// الأسبوع الرابع من الشهر الخامس: أمان الويب
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

namespace ArabicLanguage {

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

// طلب HTTP
struct HTTPRequest {
    HTTPMethod method;
    std::string path;
    std::string queryString;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> queryParams;
    std::map<std::string, std::string> pathParams;
    std::string body;
    std::string remoteAddress;
    int remotePort;
    
    HTTPRequest() : method(HTTPMethod::GET), remotePort(0) {}
    
    std::string getHeader(const std::string& name) const;
    std::string getQueryParam(const std::string& name) const;
    std::string getPathParam(const std::string& name) const;
    bool hasHeader(const std::string& name) const;
};

// استجابة HTTP
struct HTTPResponse {
    HTTPStatus status;
    std::map<std::string, std::string> headers;
    std::string body;
    
    HTTPResponse() : status(HTTPStatus::OK) {}
    
    void setHeader(const std::string& name, const std::string& value);
    void setContentType(const std::string& type);
    void setStatus(HTTPStatus s) { status = s; }
    void setBody(const std::string& b) { body = b; }
    void setJSON(const std::string& json);
    void setText(const std::string& text);
    void setHTML(const std::string& html);
    
    std::string toString() const;
};

// معالج الطلب
using RequestHandler = std::function<HTTPResponse(const HTTPRequest&)>;

// معلومات المسار
struct Route {
    HTTPMethod method;
    std::string path;
    std::string pattern;  // نمط مع parameters مثل /users/:id
    RequestHandler handler;
    
    Route() : method(HTTPMethod::GET) {}
};

// خادم HTTP بسيط
class HTTPServer {
private:
    int port;
    std::string host;
    std::atomic<bool> running;
    std::thread serverThread;
    std::vector<Route> routes;
    std::mutex routesMutex;
    
    // معالجات افتراضية
    RequestHandler defaultHandler;
    RequestHandler notFoundHandler;
    RequestHandler errorHandler;
    
    // إعدادات
    int maxConnections;
    int connectionTimeout;
    bool corsEnabled;
    std::string corsOrigin;

    // إعدادات SSL/TLS
    bool sslEnabled;
    std::string sslCertFile;
    std::string sslKeyFile;
    std::string sslCaFile;
    SSL_CTX* sslContext;

    // إعدادات الأمان
    bool securityHeadersEnabled;
    bool xssProtectionEnabled;
    bool csrfProtectionEnabled;
    std::string csrfTokenName;
    std::map<std::string, std::string> securityHeaders;
    
public:
    HTTPServer(int port = 8080, const std::string& host = "0.0.0.0");
    ~HTTPServer();

    // منع النسخ
    HTTPServer(const HTTPServer&) = delete;
    HTTPServer& operator=(const HTTPServer&) = delete;
    
    // إدارة الخادم
    bool start();
    void stop();
    bool isRunning() const { return running.load(); }
    void wait();
    
    // إضافة المسارات
    void addRoute(HTTPMethod method, const std::string& path, RequestHandler handler);
    void get(const std::string& path, RequestHandler handler);
    void post(const std::string& path, RequestHandler handler);
    void put(const std::string& path, RequestHandler handler);
    void del(const std::string& path, RequestHandler handler);
    void patch(const std::string& path, RequestHandler handler);
    
    // معالجات افتراضية
    void setDefaultHandler(RequestHandler handler);
    void setNotFoundHandler(RequestHandler handler);
    void setErrorHandler(RequestHandler handler);
    
    // إعدادات CORS
    void enableCORS(bool enable, const std::string& origin = "*");
    
    // إعدادات الاتصال
    void setMaxConnections(int max) { maxConnections = max; }
    void setConnectionTimeout(int timeout) { connectionTimeout = timeout; }

    // إعدادات SSL/TLS
    void enableSSL(const std::string& certFile, const std::string& keyFile,
                   const std::string& caFile = "");
    void disableSSL();
    bool isSSL() const { return sslEnabled; }

    // إعدادات الأمان
    void enableSecurityHeaders(bool enable = true);
    void enableXSSProtection(bool enable = true);
    void enableCSRFProtection(bool enable = true, const std::string& tokenName = "csrf_token");
    void addSecurityHeader(const std::string& name, const std::string& value);
    void setSecurityHeaders(const std::map<std::string, std::string>& headers);

    // مصادقة المستخدمين
    bool validateCSRFToken(const HTTPRequest& request) const;
    std::string generateCSRFToken() const;
    void applySecurityHeaders(HTTPResponse& response) const;

    // معلومات
    int getPort() const { return port; }
    std::string getHost() const { return host; }
    
private:
    void serverLoop();
    void handleConnection(int clientSocket);
    HTTPRequest parseRequest(const std::string& rawRequest);
    HTTPResponse processRequest(const HTTPRequest& request);
    Route* findRoute(HTTPMethod method, const std::string& path);
    bool matchRoute(const std::string& pattern, const std::string& path,
                   std::map<std::string, std::string>& params);
    void sendResponse(int clientSocket, const HTTPResponse& response);
    std::string methodToString(HTTPMethod method) const;
    HTTPMethod stringToMethod(const std::string& str) const;
    std::string statusToString(HTTPStatus status) const;
    int createSocket();
    void setupSocket(int socket);

    // SSL/TLS methods
    bool initializeSSL();
    void cleanupSSL();
    void* acceptSSLConnection(int clientSocket);
    void closeSSLConnection(SSL* ssl);
    bool loadSSLCertificate();
    std::string readSSLRequest(SSL* ssl);
    void sendSSLResponse(SSL* ssl, const HTTPResponse& response);

    // Security methods
    void initializeSecurityHeaders();
    bool validateOrigin(const HTTPRequest& request) const;
    std::string sanitizeHTML(const std::string& input) const;
    std::string escapeHTML(const std::string& input) const;

    // Public security utilities
    std::string getSanitizedHTML(const std::string& input) const { return sanitizeHTML(input); }
    std::string getEscapedHTML(const std::string& input) const { return escapeHTML(input); }
};

// Factory function
std::unique_ptr<HTTPServer> createHTTPServer(int port = 8080, const std::string& host = "0.0.0.0");

} // namespace ArabicLanguage

#endif // HTTP_SERVER_H

