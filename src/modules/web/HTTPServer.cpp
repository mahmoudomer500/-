// HTTPServer.cpp - تطبيق خادم HTTP بسيط
// الأسبوع الأول من الشهر الخامس: خادم ويب بسيط
#include "HTTPServer.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace ArabicLanguage {

// ==================== HTTPRequest Implementation ====================

std::string HTTPRequest::getHeader(const std::string& name) const {
    auto it = headers.find(name);
    return it != headers.end() ? it->second : "";
}

std::string HTTPRequest::getQueryParam(const std::string& name) const {
    auto it = queryParams.find(name);
    return it != queryParams.end() ? it->second : "";
}

std::string HTTPRequest::getPathParam(const std::string& name) const {
    auto it = pathParams.find(name);
    return it != pathParams.end() ? it->second : "";
}

bool HTTPRequest::hasHeader(const std::string& name) const {
    return headers.find(name) != headers.end();
}

// ==================== HTTPResponse Implementation ====================

void HTTPResponse::setHeader(const std::string& name, const std::string& value) {
    headers[name] = value;
}

void HTTPResponse::setContentType(const std::string& type) {
    setHeader("Content-Type", type);
}

void HTTPResponse::setJSON(const std::string& json) {
    setContentType("application/json");
    body = json;
}

void HTTPResponse::setText(const std::string& text) {
    setContentType("text/plain");
    body = text;
}

void HTTPResponse::setHTML(const std::string& html) {
    setContentType("text/html");
    body = html;
}

std::string HTTPResponse::toString() const {
    std::stringstream ss;
    
    // Status line
    ss << "HTTP/1.1 " << static_cast<int>(status) << " ";
    switch (status) {
        case HTTPStatus::OK: ss << "OK"; break;
        case HTTPStatus::CREATED: ss << "Created"; break;
        case HTTPStatus::NO_CONTENT: ss << "No Content"; break;
        case HTTPStatus::BAD_REQUEST: ss << "Bad Request"; break;
        case HTTPStatus::UNAUTHORIZED: ss << "Unauthorized"; break;
        case HTTPStatus::FORBIDDEN: ss << "Forbidden"; break;
        case HTTPStatus::NOT_FOUND: ss << "Not Found"; break;
        case HTTPStatus::METHOD_NOT_ALLOWED: ss << "Method Not Allowed"; break;
        case HTTPStatus::INTERNAL_SERVER_ERROR: ss << "Internal Server Error"; break;
        case HTTPStatus::NOT_IMPLEMENTED: ss << "Not Implemented"; break;
        case HTTPStatus::SERVICE_UNAVAILABLE: ss << "Service Unavailable"; break;
    }
    ss << "\r\n";
    
    // Headers
    if (headers.find("Content-Length") == headers.end()) {
        ss << "Content-Length: " << body.length() << "\r\n";
    }
    
    for (const auto& header : headers) {
        ss << header.first << ": " << header.second << "\r\n";
    }
    
    ss << "\r\n";
    ss << body;
    
    return ss.str();
}

// ==================== HTTPServer Implementation ====================

HTTPServer::HTTPServer(int port, const std::string& host)
    : port(port), host(host), running(false), maxConnections(100),
      connectionTimeout(30000), corsEnabled(false), sslEnabled(false),
      sslContext(nullptr), securityHeadersEnabled(true), xssProtectionEnabled(true),
      csrfProtectionEnabled(true), csrfTokenName("csrf_token") {
    
    // معالجات افتراضية
    notFoundHandler = [](const HTTPRequest& req) -> HTTPResponse {
        HTTPResponse res;
        res.setStatus(HTTPStatus::NOT_FOUND);
        res.setHTML("<h1>404 Not Found</h1><p>The requested resource was not found.</p>");
        return res;
    };
    
    errorHandler = [](const HTTPRequest& req) -> HTTPResponse {
        HTTPResponse res;
        res.setStatus(HTTPStatus::INTERNAL_SERVER_ERROR);
        res.setHTML("<h1>500 Internal Server Error</h1>");
        return res;
    };
}

HTTPServer::~HTTPServer() {
    stop();
    cleanupSSL();
}

bool HTTPServer::start() {
    if (running.load()) {
        std::cout << "[WARNING] Server is already running" << std::endl;
        return false;
    }
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[ERROR] Failed to initialize Winsock" << std::endl;
        return false;
    }
#endif
    
    running = true;
    serverThread = std::thread(&HTTPServer::serverLoop, this);
    
    std::cout << "[INFO] HTTP Server started on http://" << host << ":" << port << std::endl;
    return true;
}

void HTTPServer::stop() {
    if (!running.load()) return;
    
    running = false;
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    std::cout << "[INFO] HTTP Server stopped" << std::endl;
}

void HTTPServer::wait() {
    if (serverThread.joinable()) {
        serverThread.join();
    }
}

void HTTPServer::addRoute(HTTPMethod method, const std::string& path, RequestHandler handler) {
    std::lock_guard<std::mutex> lock(routesMutex);
    
    Route route;
    route.method = method;
    route.path = path;
    route.pattern = path;
    route.handler = handler;
    
    routes.push_back(route);
    
    std::cout << "[INFO] Added route: " << methodToString(method) << " " << path << std::endl;
}

void HTTPServer::get(const std::string& path, RequestHandler handler) {
    addRoute(HTTPMethod::GET, path, handler);
}

void HTTPServer::post(const std::string& path, RequestHandler handler) {
    addRoute(HTTPMethod::POST, path, handler);
}

void HTTPServer::put(const std::string& path, RequestHandler handler) {
    addRoute(HTTPMethod::PUT, path, handler);
}

void HTTPServer::del(const std::string& path, RequestHandler handler) {
    addRoute(HTTPMethod::DEL, path, handler);
}

void HTTPServer::patch(const std::string& path, RequestHandler handler) {
    addRoute(HTTPMethod::PATCH, path, handler);
}

void HTTPServer::setDefaultHandler(RequestHandler handler) {
    defaultHandler = handler;
}

void HTTPServer::setNotFoundHandler(RequestHandler handler) {
    notFoundHandler = handler;
}

void HTTPServer::setErrorHandler(RequestHandler handler) {
    errorHandler = handler;
}

void HTTPServer::enableCORS(bool enable, const std::string& origin) {
    corsEnabled = enable;
    corsOrigin = origin;
}

void HTTPServer::serverLoop() {
    int serverSocket = createSocket();
    if (serverSocket < 0) {
        std::cerr << "[ERROR] Failed to create server socket" << std::endl;
        running = false;
        return;
    }
    
    setupSocket(serverSocket);
    
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (host == "0.0.0.0") {
        serverAddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);
    }
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "[ERROR] Failed to bind socket" << std::endl;
        running = false;
        return;
    }
    
    if (listen(serverSocket, maxConnections) < 0) {
        std::cerr << "[ERROR] Failed to listen on socket" << std::endl;
        running = false;
        return;
    }
    
    std::cout << "[INFO] Server listening on port " << port << std::endl;
    
    while (running.load()) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket < 0) {
            if (running.load()) {
                std::cerr << "[ERROR] Failed to accept connection" << std::endl;
            }
            continue;
        }
        
        // معالجة الاتصال في thread منفصل (مبسط - في الإصدار الكامل سيستخدم thread pool)
        std::thread([this, clientSocket, clientAddr]() {
            handleConnection(clientSocket);
            
#ifdef _WIN32
            closesocket(clientSocket);
#else
            close(clientSocket);
#endif
        }).detach();
    }
    
#ifdef _WIN32
    closesocket(serverSocket);
#else
    close(serverSocket);
#endif
}

void HTTPServer::handleConnection(int clientSocket) {
    SSL* ssl = nullptr;
    std::string rawRequest;

    try {
        // Handle SSL connection if enabled
        if (sslEnabled) {
            ssl = static_cast<SSL*>(acceptSSLConnection(clientSocket));
            if (!ssl) {
                std::cout << "[WARNING] SSL handshake failed" << std::endl;
                return;
            }
            rawRequest = readSSLRequest(ssl);
        } else {
            // Handle regular HTTP connection
            char buffer[8192] = {0};
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived <= 0) {
                return;
            }

            buffer[bytesReceived] = '\0';
            rawRequest = buffer;
        }

        if (rawRequest.empty()) {
            return;
        }

        HTTPRequest request = parseRequest(rawRequest);

        // Security validations
        if (!validateOrigin(request)) {
            HTTPResponse response;
            response.setStatus(HTTPStatus::FORBIDDEN);
            response.setHTML("<h1>403 Forbidden</h1><p>Invalid origin</p>");
            applySecurityHeaders(response);
            if (ssl) {
                sendSSLResponse(ssl, response);
            } else {
                std::string responseStr = response.toString();
                send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
            }
            return;
        }

        if (!validateCSRFToken(request)) {
            HTTPResponse response;
            response.setStatus(HTTPStatus::FORBIDDEN);
            response.setHTML("<h1>403 Forbidden</h1><p>Invalid CSRF token</p>");
            applySecurityHeaders(response);
            if (ssl) {
                sendSSLResponse(ssl, response);
            } else {
                std::string responseStr = response.toString();
                send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
            }
            return;
        }

        HTTPResponse response = processRequest(request);

        // Apply security headers
        applySecurityHeaders(response);

        // إضافة CORS headers إذا كان مفعلاً
        if (corsEnabled) {
            response.setHeader("Access-Control-Allow-Origin", corsOrigin);
            response.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            response.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-CSRF-Token");
        }

        // Send response
        if (ssl) {
            sendSSLResponse(ssl, response);
        } else {
            std::string responseStr = response.toString();
            send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
        }

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Error processing request: " << e.what() << std::endl;
        if (errorHandler) {
            HTTPRequest dummyReq;
            HTTPResponse errorRes = errorHandler(dummyReq);
            applySecurityHeaders(errorRes);

            if (ssl) {
                sendSSLResponse(ssl, errorRes);
            } else {
                std::string errorStr = errorRes.toString();
                send(clientSocket, errorStr.c_str(), errorStr.length(), 0);
            }
        }
    }

    // Clean up SSL connection
    if (ssl) {
        closeSSLConnection(ssl);
    }

#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
}

HTTPRequest HTTPServer::parseRequest(const std::string& rawRequest) {
    HTTPRequest request;
    
    std::istringstream iss(rawRequest);
    std::string line;
    
    // Parse request line
    if (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        std::string methodStr, path, httpVersion;
        lineStream >> methodStr >> path >> httpVersion;
        
        request.method = stringToMethod(methodStr);
        
        // Parse path and query string
        size_t queryPos = path.find('?');
        if (queryPos != std::string::npos) {
            request.path = path.substr(0, queryPos);
            request.queryString = path.substr(queryPos + 1);
            
            // Parse query parameters
            std::istringstream queryStream(request.queryString);
            std::string param;
            while (std::getline(queryStream, param, '&')) {
                size_t eqPos = param.find('=');
                if (eqPos != std::string::npos) {
                    std::string key = param.substr(0, eqPos);
                    std::string value = param.substr(eqPos + 1);
                    request.queryParams[key] = value;
                }
            }
        } else {
            request.path = path;
        }
    }
    
    // Parse headers
    while (std::getline(iss, line) && line != "\r" && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            request.headers[key] = value;
        }
    }
    
    // Parse body
    std::string body;
    while (std::getline(iss, line)) {
        body += line + "\n";
    }
    if (!body.empty() && body.back() == '\n') {
        body.pop_back();
    }
    request.body = body;
    
    return request;
}

HTTPResponse HTTPServer::processRequest(const HTTPRequest& request) {
    Route* route = findRoute(request.method, request.path);
    
    if (route) {
        try {
            return route->handler(request);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Handler error: " << e.what() << std::endl;
            if (errorHandler) {
                return errorHandler(request);
            }
        }
    }
    
    if (notFoundHandler) {
        return notFoundHandler(request);
    }
    
    HTTPResponse response;
    response.setStatus(HTTPStatus::NOT_FOUND);
    response.setHTML("<h1>404 Not Found</h1>");
    return response;
}

Route* HTTPServer::findRoute(HTTPMethod method, const std::string& path) {
    std::lock_guard<std::mutex> lock(routesMutex);
    
    for (auto& route : routes) {
        if (route.method == method) {
            std::map<std::string, std::string> params;
            if (matchRoute(route.pattern, path, params)) {
                // في الإصدار الكامل سيتم إضافة params إلى request
                return &route;
            }
        }
    }
    
    return nullptr;
}

bool HTTPServer::matchRoute(const std::string& pattern, const std::string& path,
                           std::map<std::string, std::string>& params) {
    // مطابقة بسيطة (في الإصدار الكامل سيتم استخدام regex متقدم)
    if (pattern == path) {
        return true;
    }
    
    // دعم parameters بسيط مثل /users/:id
    std::regex patternRegex(pattern);
    std::smatch matches;
    if (std::regex_match(path, matches, patternRegex)) {
        return true;
    }
    
    return false;
}

void HTTPServer::sendResponse(int clientSocket, const HTTPResponse& response) {
    std::string responseStr = response.toString();
    send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
}

std::string HTTPServer::methodToString(HTTPMethod method) const {
    switch (method) {
        case HTTPMethod::GET: return "GET";
        case HTTPMethod::POST: return "POST";
        case HTTPMethod::PUT: return "PUT";
        case HTTPMethod::DEL: return "DELETE";
        case HTTPMethod::PATCH: return "PATCH";
        case HTTPMethod::HEAD: return "HEAD";
        case HTTPMethod::OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

HTTPMethod HTTPServer::stringToMethod(const std::string& str) const {
    if (str == "GET") return HTTPMethod::GET;
    if (str == "POST") return HTTPMethod::POST;
    if (str == "PUT") return HTTPMethod::PUT;
    if (str == "DELETE") return HTTPMethod::DEL;
    if (str == "PATCH") return HTTPMethod::PATCH;
    if (str == "HEAD") return HTTPMethod::HEAD;
    if (str == "OPTIONS") return HTTPMethod::OPTIONS;
    return HTTPMethod::UNKNOWN;
}

std::string HTTPServer::statusToString(HTTPStatus status) const {
    switch (status) {
        case HTTPStatus::OK: return "200 OK";
        case HTTPStatus::CREATED: return "201 Created";
        case HTTPStatus::NO_CONTENT: return "204 No Content";
        case HTTPStatus::BAD_REQUEST: return "400 Bad Request";
        case HTTPStatus::UNAUTHORIZED: return "401 Unauthorized";
        case HTTPStatus::FORBIDDEN: return "403 Forbidden";
        case HTTPStatus::NOT_FOUND: return "404 Not Found";
        case HTTPStatus::METHOD_NOT_ALLOWED: return "405 Method Not Allowed";
        case HTTPStatus::INTERNAL_SERVER_ERROR: return "500 Internal Server Error";
        case HTTPStatus::NOT_IMPLEMENTED: return "501 Not Implemented";
        case HTTPStatus::SERVICE_UNAVAILABLE: return "503 Service Unavailable";
        default: return "500 Internal Server Error";
    }
}

int HTTPServer::createSocket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

void HTTPServer::setupSocket(int socket) {
    int opt = 1;
#ifdef _WIN32
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
}

// ==================== SSL/TLS Implementation ====================

void HTTPServer::enableSSL(const std::string& certFile, const std::string& keyFile,
                          const std::string& caFile) {
    if (running.load()) {
        std::cout << "[ERROR] Cannot enable SSL while server is running" << std::endl;
        return;
    }

    sslCertFile = certFile;
    sslKeyFile = keyFile;
    sslCaFile = caFile;
    sslEnabled = true;

    if (!initializeSSL()) {
        std::cout << "[ERROR] Failed to initialize SSL" << std::endl;
        sslEnabled = false;
    }
}

void HTTPServer::disableSSL() {
    if (running.load()) {
        std::cout << "[ERROR] Cannot disable SSL while server is running" << std::endl;
        return;
    }

    cleanupSSL();
    sslEnabled = false;
}

bool HTTPServer::initializeSSL() {
    // Initialize OpenSSL
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    // Create SSL context
    const SSL_METHOD* method = TLS_server_method();
    sslContext = SSL_CTX_new(method);

    if (!sslContext) {
        std::cout << "[ERROR] Unable to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Configure SSL context
    SSL_CTX_set_ecdh_auto(sslContext, 1);

    // Load certificate and key
    if (!loadSSLCertificate()) {
        cleanupSSL();
        return false;
    }

    // Set SSL options for security
    SSL_CTX_set_options(sslContext,
        SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);

    std::cout << "[INFO] SSL/TLS initialized successfully" << std::endl;
    return true;
}

void HTTPServer::cleanupSSL() {
    if (sslContext) {
        SSL_CTX_free(sslContext);
        sslContext = nullptr;
    }
    EVP_cleanup();
}

bool HTTPServer::loadSSLCertificate() {
    if (!sslContext) return false;

    // Load certificate
    if (SSL_CTX_use_certificate_file(sslContext, sslCertFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cout << "[ERROR] Failed to load certificate file: " << sslCertFile << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Load private key
    if (SSL_CTX_use_PrivateKey_file(sslContext, sslKeyFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cout << "[ERROR] Failed to load private key file: " << sslKeyFile << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Verify private key
    if (!SSL_CTX_check_private_key(sslContext)) {
        std::cout << "[ERROR] Private key does not match the certificate" << std::endl;
        return false;
    }

    // Load CA certificate if provided
    if (!sslCaFile.empty()) {
        if (SSL_CTX_load_verify_locations(sslContext, sslCaFile.c_str(), nullptr) <= 0) {
            std::cout << "[WARNING] Failed to load CA certificate file: " << sslCaFile << std::endl;
        }
    }

    return true;
}

void* HTTPServer::acceptSSLConnection(int clientSocket) {
    if (!sslContext) return nullptr;

    SSL* ssl = SSL_new(sslContext);
    SSL_set_fd(ssl, clientSocket);

    if (SSL_accept(ssl) <= 0) {
        std::cout << "[ERROR] SSL handshake failed" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return nullptr;
    }

    return ssl;
}

void HTTPServer::closeSSLConnection(SSL* ssl) {
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
}

std::string HTTPServer::readSSLRequest(SSL* ssl) {
    std::string request;
    char buffer[4096];
    int bytesRead;

    while ((bytesRead = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        request += buffer;

        // Check if we have the complete headers
        if (request.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    return request;
}

void HTTPServer::sendSSLResponse(SSL* ssl, const HTTPResponse& response) {
    std::string responseStr = response.toString();
    SSL_write(ssl, responseStr.c_str(), responseStr.length());
}

// ==================== Security Implementation ====================

void HTTPServer::enableSecurityHeaders(bool enable) {
    securityHeadersEnabled = enable;
    if (enable) {
        initializeSecurityHeaders();
    }
}

void HTTPServer::enableXSSProtection(bool enable) {
    xssProtectionEnabled = enable;
}

void HTTPServer::enableCSRFProtection(bool enable, const std::string& tokenName) {
    csrfProtectionEnabled = enable;
    csrfTokenName = tokenName;
}

void HTTPServer::addSecurityHeader(const std::string& name, const std::string& value) {
    securityHeaders[name] = value;
}

void HTTPServer::setSecurityHeaders(const std::map<std::string, std::string>& headers) {
    securityHeaders = headers;
}

void HTTPServer::initializeSecurityHeaders() {
    // Default security headers
    securityHeaders = {
        {"X-Content-Type-Options", "nosniff"},
        {"X-Frame-Options", "DENY"},
        {"X-XSS-Protection", "1; mode=block"},
        {"Strict-Transport-Security", "max-age=31536000; includeSubDomains"},
        {"Content-Security-Policy", "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'"},
        {"Referrer-Policy", "strict-origin-when-cross-origin"}
    };
}

void HTTPServer::applySecurityHeaders(HTTPResponse& response) const {
    if (!securityHeadersEnabled) return;

    for (const auto& header : securityHeaders) {
        response.setHeader(header.first, header.second);
    }
}

bool HTTPServer::validateCSRFToken(const HTTPRequest& request) const {
    if (!csrfProtectionEnabled) return true;

    std::string token = request.getHeader("X-CSRF-Token");
    if (token.empty()) {
        token = request.getQueryParam(csrfTokenName);
    }
    if (token.empty() && request.method == HTTPMethod::POST) {
        // Check body for POST requests
        // Simple implementation - in production, parse form data or JSON
        if (request.body.find(csrfTokenName + "=") != std::string::npos) {
            return true; // Simplified validation
        }
    }

    return !token.empty();
}

std::string HTTPServer::generateCSRFToken() const {
    // Simple token generation - in production, use cryptographically secure random
    static int counter = 0;
    std::stringstream ss;
    ss << "csrf_" << std::time(nullptr) << "_" << ++counter;
    return ss.str();
}

bool HTTPServer::validateOrigin(const HTTPRequest& request) const {
    std::string origin = request.getHeader("Origin");
    if (origin.empty()) {
        origin = request.getHeader("Referer");
    }

    // Simple origin validation - in production, check against whitelist
    if (!origin.empty()) {
        // Allow localhost for development
        if (origin.find("localhost") != std::string::npos ||
            origin.find("127.0.0.1") != std::string::npos) {
            return true;
        }
    }

    return true; // Allow by default for now
}

std::string HTTPServer::sanitizeHTML(const std::string& input) const {
    std::string sanitized = input;

    // Basic HTML sanitization - replace dangerous tags
    std::map<std::string, std::string> replacements = {
        {"<script", "&lt;script"},
        {"</script>", "&lt;/script&gt;"},
        {"<iframe", "&lt;iframe"},
        {"</iframe>", "&lt;/iframe&gt;"},
        {"javascript:", ""},
        {"onload=", "data-onload="},
        {"onerror=", "data-onerror="},
        {"onclick=", "data-onclick="}
    };

    for (const auto& replacement : replacements) {
        size_t pos = 0;
        while ((pos = sanitized.find(replacement.first, pos)) != std::string::npos) {
            sanitized.replace(pos, replacement.first.length(), replacement.second);
            pos += replacement.second.length();
        }
    }

    return sanitized;
}

std::string HTTPServer::escapeHTML(const std::string& input) const {
    std::string escaped = input;

    std::map<std::string, std::string> htmlEntities = {
        {"&", "&amp;"},
        {"<", "&lt;"},
        {">", "&gt;"},
        {"\"", "&quot;"},
        {"'", "&#x27;"},
        {"/", "&#x2F;"}
    };

    for (const auto& entity : htmlEntities) {
        size_t pos = 0;
        while ((pos = escaped.find(entity.first, pos)) != std::string::npos) {
            escaped.replace(pos, entity.first.length(), entity.second);
            pos += entity.second.length();
        }
    }

    return escaped;
}

// Factory function
std::unique_ptr<HTTPServer> createHTTPServer(int port, const std::string& host) {
    return std::make_unique<HTTPServer>(port, host);
}

} // namespace ArabicLanguage

