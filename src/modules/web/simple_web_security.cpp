// simple_web_security.cpp - تطبيق نظام أمان ويب مبسط
// الأسبوع الرابع من الشهر الخامس: أمان الويب
#include "simple_web_security.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <cstring>
#include <iomanip>

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

// ==================== SecurityHTTPRequest Implementation ====================

std::string SecurityHTTPRequest::getHeader(const std::string& name) const {
    auto it = headers.find(name);
    return it != headers.end() ? it->second : "";
}

std::string SecurityHTTPRequest::getQueryParam(const std::string& name) const {
    auto it = queryParams.find(name);
    return it != queryParams.end() ? it->second : "";
}

bool SecurityHTTPRequest::hasHeader(const std::string& name) const {
    return headers.find(name) != headers.end();
}

// ==================== SecurityHTTPResponse Implementation ====================

void SecurityHTTPResponse::setHeader(const std::string& name, const std::string& value) {
    headers[name] = value;
}

void SecurityHTTPResponse::setContentType(const std::string& type) {
    headers["Content-Type"] = type;
}

void SecurityHTTPResponse::setJSON(const std::string& json) {
    setContentType("application/json");
    body = json;
}

void SecurityHTTPResponse::setText(const std::string& text) {
    setContentType("text/plain");
    body = text;
}

void SecurityHTTPResponse::setHTML(const std::string& html) {
    setContentType("text/html");
    body = html;
}

static std::string statusToString(HTTPStatus status) {

    switch (status) {
        case HTTPStatus::OK: return "OK";
        case HTTPStatus::CREATED: return "Created";
        case HTTPStatus::NO_CONTENT: return "No Content";
        case HTTPStatus::BAD_REQUEST: return "Bad Request";
        case HTTPStatus::UNAUTHORIZED: return "Unauthorized";
        case HTTPStatus::FORBIDDEN: return "Forbidden";
        case HTTPStatus::NOT_FOUND: return "Not Found";
        case HTTPStatus::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HTTPStatus::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HTTPStatus::NOT_IMPLEMENTED: return "Not Implemented";
        case HTTPStatus::SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return "Unknown";
    }
}

// ==================== SecureHTTPServer Implementation ====================

SecureHTTPServer::SecureHTTPServer(int port, const std::string& host)
    : port(port), host(host), running(false),
      securityHeadersEnabled(true), xssProtectionEnabled(true),
      csrfProtectionEnabled(true), csrfTokenName("csrf_token") {

    initializeSecurityHeaders();
}

SecureHTTPServer::~SecureHTTPServer() {
    stop();
}

bool SecureHTTPServer::start() {
    if (running) {
        std::cout << "[WARNING] Server is already running" << std::endl;
        return false;
    }

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "[ERROR] WSAStartup failed" << std::endl;
        return false;
    }
#endif

    std::cout << "[INFO] Server starting on port " << port << std::endl;
    running = true;

    // Simple server loop (for demonstration only)
    // In a real implementation, this would be in a separate thread
    // and handle multiple connections properly

    return true;
}

void SecureHTTPServer::stop() {
    running = false;

#ifdef _WIN32
    WSACleanup();
#endif

    std::cout << "[INFO] Server stopped" << std::endl;
}

void SecureHTTPServer::get(const std::string& path, std::function<SecurityHTTPResponse(const SecurityHTTPRequest&)> handler) {
    routes.push_back({path, handler});
}

void SecureHTTPServer::post(const std::string& path, std::function<SecurityHTTPResponse(const SecurityHTTPRequest&)> handler) {
    routes.push_back({path, handler});
}

void SecureHTTPServer::put(const std::string& path, std::function<SecurityHTTPResponse(const SecurityHTTPRequest&)> handler) {
    routes.push_back({path, handler});
}

void SecureHTTPServer::del(const std::string& path, std::function<SecurityHTTPResponse(const SecurityHTTPRequest&)> handler) {
    routes.push_back({path, handler});
}

void SecureHTTPServer::enableSecurityHeaders(bool enable) {
    securityHeadersEnabled = enable;
}

void SecureHTTPServer::enableXSSProtection(bool enable) {
    xssProtectionEnabled = enable;
}

void SecureHTTPServer::enableCSRFProtection(bool enable, const std::string& tokenName) {
    csrfProtectionEnabled = enable;
    csrfTokenName = tokenName;
}

std::string SecureHTTPServer::generateCSRFToken() const {
    // Simple token generation
    static int counter = 0;
    std::stringstream ss;
    ss << "csrf_" << std::time(nullptr) << "_" << ++counter;
    return ss.str();
}

bool SecureHTTPServer::validateCSRFToken(const SecurityHTTPRequest& request) const {
    if (!csrfProtectionEnabled) return true;

    std::string token = request.getHeader("X-CSRF-Token");
    if (token.empty()) {
        token = request.getQueryParam(csrfTokenName);
    }

    return !token.empty();
}

void SecureHTTPServer::applySecurityHeaders(SecurityHTTPResponse& response) const {
    if (!securityHeadersEnabled) return;

    for (const auto& header : securityHeaders) {
        response.setHeader(header.first, header.second);
    }
}

std::string SecureHTTPServer::sanitizeHTML(const std::string& input) const {
    std::string sanitized = input;

    // Basic HTML sanitization
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

std::string SecureHTTPServer::escapeHTML(const std::string& input) const {
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

void SecureHTTPServer::initializeSecurityHeaders() {
    securityHeaders = {
        {"X-Content-Type-Options", "nosniff"},
        {"X-Frame-Options", "DENY"},
        {"X-XSS-Protection", "1; mode=block"},
        {"Content-Security-Policy", "default-src 'self'"}
    };
}

SecurityHTTPRequest SecureHTTPServer::parseRequest(const std::string& rawRequest) {
    SecurityHTTPRequest request;
    std::istringstream iss(rawRequest);
    std::string line;

    // Parse request line
    if (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        std::string methodStr, path, version;
        lineStream >> methodStr >> path >> version;

        request.method = stringToMethod(methodStr);
        request.path = path;
    }

    // Parse headers
    while (std::getline(iss, line) && !line.empty()) {
        if (line.back() == '\r') line.pop_back();
        if (line.empty()) break;

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 2); // Skip ": "
            request.headers[name] = value;
        }
    }

    // Parse body (simplified)
    std::string body;
    while (std::getline(iss, line)) {
        body += line + "\n";
    }
    if (!body.empty() && body.back() == '\n') body.pop_back();
    request.body = body;

    return request;
}

SecurityHTTPResponse SecureHTTPServer::processRequest(const SecurityHTTPRequest& request) {
    auto handler = findRoute(request.path);
    if (handler) {
        try {
            SecurityHTTPResponse response = (*handler)(request);
            applySecurityHeaders(response);
            return response;
        } catch (const std::exception& e) {
            SecurityHTTPResponse response;
            response.setStatus(HTTPStatus::INTERNAL_SERVER_ERROR);
            response.setHTML("<h1>500 Internal Server Error</h1>");
            applySecurityHeaders(response);
            return response;
        }
    }

    SecurityHTTPResponse response;
    response.setStatus(HTTPStatus::NOT_FOUND);
    response.setHTML("<h1>404 Not Found</h1><p>The requested resource was not found.</p>");
    applySecurityHeaders(response);
    return response;
}

std::function<SecurityHTTPResponse(const SecurityHTTPRequest&)>* SecureHTTPServer::findRoute(const std::string& path) {
    for (auto& route : routes) {
        if (matchRoute(route.first, path)) {
            return &route.second;
        }
    }
    return nullptr;
}

bool SecureHTTPServer::matchRoute(const std::string& pattern, const std::string& path) {
    return pattern == path; // Simple exact match for demonstration
}

std::string SecureHTTPServer::methodToString(HTTPMethod method) const {
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

HTTPMethod SecureHTTPServer::stringToMethod(const std::string& str) const {
    if (str == "GET") return HTTPMethod::GET;
    if (str == "POST") return HTTPMethod::POST;
    if (str == "PUT") return HTTPMethod::PUT;
    if (str == "DELETE") return HTTPMethod::DEL;
    if (str == "PATCH") return HTTPMethod::PATCH;
    if (str == "HEAD") return HTTPMethod::HEAD;
    if (str == "OPTIONS") return HTTPMethod::OPTIONS;
    return HTTPMethod::UNKNOWN;
}

// ==================== SimpleAuthManager Implementation ====================

SimpleAuthManager::SimpleAuthManager() : tokenExpirationHours(24) {
    loadDefaultUsers();
}

bool SimpleAuthManager::createUser(const std::string& username, const std::string& password, const std::string& role) {
    if (getUserByUsername(username)) {
        return false; // User already exists
    }

    User user;
    user.id = generateUserId();
    user.username = username;
    user.passwordHash = hashPassword(password);
    user.role = role;

    users[username] = user;
    return true;
}

User* SimpleAuthManager::getUserByUsername(const std::string& username) {
    auto it = users.find(username);
    return it != users.end() ? &it->second : nullptr;
}

AuthResult SimpleAuthManager::authenticate(const std::string& username, const std::string& password) {
    AuthResult result;

    User* user = getUserByUsername(username);
    if (!user) {
        result.message = "User not found";
        return result;
    }

    if (!user->isActive) {
        result.message = "Account is disabled";
        return result;
    }

    if (!verifyPassword(password, user->passwordHash)) {
        result.message = "Invalid password";
        return result;
    }

    // Create token
    std::string token = generateToken();
    AuthToken authToken;
    authToken.token = token;
    authToken.userId = user->id;
    authToken.expiresAt = std::chrono::system_clock::now() +
                         std::chrono::hours(tokenExpirationHours);

    activeTokens[token] = authToken;

    result.success = true;
    result.userId = user->id;
    result.role = user->role;
    result.token = token;
    result.message = "Authentication successful";

    return result;
}

AuthResult SimpleAuthManager::validateToken(const std::string& token) {
    AuthResult result;

    auto it = activeTokens.find(token);
    if (it == activeTokens.end()) {
        result.message = "Token not found";
        return result;
    }

    if (it->second.isExpired()) {
        activeTokens.erase(it);
        result.message = "Token expired";
        return result;
    }

    User* user = getUserByUsername(it->second.userId);
    if (!user || !user->isActive) {
        activeTokens.erase(it);
        result.message = "User not found or inactive";
        return result;
    }

    result.success = true;
    result.userId = user->id;
    result.role = user->role;
    result.token = token;
    result.message = "Token valid";

    return result;
}

bool SimpleAuthManager::revokeToken(const std::string& token) {
    auto it = activeTokens.find(token);
    if (it != activeTokens.end()) {
        activeTokens.erase(it);
        return true;
    }
    return false;
}

bool SimpleAuthManager::hasRole(const std::string& userId, const std::string& role) {
    for (const auto& pair : users) {
        if (pair.second.id == userId) {
            return pair.second.role == role;
        }
    }
    return false;
}

std::string SimpleAuthManager::hashPassword(const std::string& password) {
    return SimpleCrypto::hashPassword(password, SimpleCrypto::generateSalt());
}

bool SimpleAuthManager::verifyPassword(const std::string& password, const std::string& hash) {
    size_t colonPos = hash.find(':');
    if (colonPos == std::string::npos) return false;

    std::string salt = hash.substr(0, colonPos);
    std::string expectedHash = hash.substr(colonPos + 1);

    return SimpleCrypto::verifyPassword(password, expectedHash, salt);
}

std::string SimpleAuthManager::generateToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (int i = 0; i < 32; ++i) {
        ss << std::setw(1) << dis(gen);
    }

    return ss.str();
}

std::string SimpleAuthManager::generateUserId() {
    static int counter = 0;
    std::stringstream ss;
    ss << "user_" << std::time(nullptr) << "_" << ++counter;
    return ss.str();
}

void SimpleAuthManager::loadDefaultUsers() {
    createUser("admin", "admin123", "admin");
    createUser("user", "user123", "user");
    createUser("guest", "guest123", "guest");
}

// ==================== SimpleCrypto Implementation ====================

std::string SimpleCrypto::hashSHA256(const std::string& input) {
    // Simple hash function for demonstration (NOT cryptographically secure)
    std::hash<std::string> hasher;
    size_t hash_value = hasher(input);

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash_value;

    // Add some scrambling
    for (char c : input) {
        hash_value = (hash_value * 31) + c;
    }

    ss << std::setw(16) << (hash_value & 0xFFFFFFFF);
    return ss.str();
}

std::string SimpleCrypto::generateSalt(size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::string salt;
    for (size_t i = 0; i < length; ++i) {
        salt += static_cast<char>(dis(gen));
    }

    // Simple base64-like encoding
    std::string encoded;
    for (char c : salt) {
        encoded += "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(c & 0x3F)];
    }

    return encoded;
}

std::string SimpleCrypto::hashPassword(const std::string& password, const std::string& salt) {
    std::string combined = salt + password;
    std::string hash = hashSHA256(combined);
    return salt + ":" + hash;
}

bool SimpleCrypto::verifyPassword(const std::string& password, const std::string& hash, const std::string& salt) {
    std::string expectedHash = hashSHA256(salt + password);
    return expectedHash == hash;
}

} // namespace ArabicLanguage
