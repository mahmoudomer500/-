// WebAuth.cpp - تطبيق نظام مصادقة الويب
// الأسبوع الرابع من الشهر الخامس: أمان الويب
#include "WebAuth.h"
#include "HTTPServer.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <iomanip>
// OpenSSL headers for cryptographic functions
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

// Base64 encoding/decoding functions
static std::string base64Encode(const std::string& input) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input.data(), input.size());
    BIO_flush(bio);

    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    return result;
}

static std::string base64Decode(const std::string& input) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input.data(), input.size());

    BIO* bio_out = BIO_new(BIO_s_mem());
    BIO_push(bio_out, bio);
    BIO_flush(bio_out);

    char* buffer = new char[input.size()];
    int decoded_size = BIO_read(bio_out, buffer, input.size());

    std::string result(buffer, decoded_size);
    delete[] buffer;

    BIO_free_all(bio_out);

    return result;
}

namespace ArabicLanguage {

// ==================== AuthManager Implementation ====================

AuthManager::AuthManager()
    : tokenExpirationHours(24), enableSessionTimeout(true) {

    jwtSecret = "arabic_language_web_secret_key_2025";
    loadDefaultUsers();
}

AuthManager::~AuthManager() {
    // تنظيف الرموز المنتهية الصلاحية
    cleanupExpiredTokens();
}

bool AuthManager::createUser(const std::string& username, const std::string& email,
                           const std::string& password, const std::string& role) {
    if (getUserByUsername(username)) {
        return false; // المستخدم موجود بالفعل
    }

    User user;
    user.id = generateUserId();
    user.username = username;
    user.email = email;
    user.passwordHash = hashPassword(password);
    user.role = role;
    user.createdAt = std::chrono::system_clock::now();

    users[username] = user;
    return true;
}

bool AuthManager::updateUser(const std::string& userId, const User& updates) {
    for (auto& pair : users) {
        if (pair.second.id == userId) {
            if (!updates.username.empty()) pair.second.username = updates.username;
            if (!updates.email.empty()) pair.second.email = updates.email;
            if (!updates.role.empty()) pair.second.role = updates.role;
            pair.second.isActive = updates.isActive;
            return true;
        }
    }
    return false;
}

bool AuthManager::deleteUser(const std::string& userId) {
    for (auto it = users.begin(); it != users.end(); ++it) {
        if (it->second.id == userId) {
            // إلغاء جميع رموز المستخدم
            revokeAllUserTokens(userId);
            users.erase(it);
            return true;
        }
    }
    return false;
}

User* AuthManager::getUser(const std::string& userId) {
    for (auto& pair : users) {
        if (pair.second.id == userId) {
            return &pair.second;
        }
    }
    return nullptr;
}

User* AuthManager::getUserByUsername(const std::string& username) {
    auto it = users.find(username);
    return it != users.end() ? &it->second : nullptr;
}

std::vector<User> AuthManager::getAllUsers() {
    std::vector<User> result;
    for (const auto& pair : users) {
        result.push_back(pair.second);
    }
    return result;
}

AuthResult AuthManager::authenticate(const std::string& username, const std::string& password,
                                   const std::string& ipAddress, const std::string& userAgent) {
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

    // إنشاء رمز جديد
    std::string token = generateToken();
    AuthToken authToken;
    authToken.token = token;
    authToken.userId = user->id;
    authToken.expiresAt = std::chrono::system_clock::now() +
                         std::chrono::hours(tokenExpirationHours);
    authToken.ipAddress = ipAddress;
    authToken.userAgent = userAgent;

    activeTokens[token] = authToken;
    userTokens[user->id].push_back(token);

    // تحديث وقت آخر تسجيل دخول
    user->lastLogin = std::chrono::system_clock::now();

    result.success = true;
    result.userId = user->id;
    result.role = user->role;
    result.token = token;
    result.message = "Authentication successful";

    return result;
}

AuthResult AuthManager::validateToken(const std::string& token) {
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

    User* user = getUser(it->second.userId);
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

bool AuthManager::revokeToken(const std::string& token) {
    auto it = activeTokens.find(token);
    if (it != activeTokens.end()) {
        std::string userId = it->second.userId;
        activeTokens.erase(it);

        // إزالة من قائمة رموز المستخدم
        auto& tokens = userTokens[userId];
        tokens.erase(std::remove(tokens.begin(), tokens.end(), token), tokens.end());

        return true;
    }
    return false;
}

void AuthManager::revokeAllUserTokens(const std::string& userId) {
    auto it = userTokens.find(userId);
    if (it != userTokens.end()) {
        for (const std::string& token : it->second) {
            activeTokens.erase(token);
        }
        it->second.clear();
    }
}

bool AuthManager::hasRole(const std::string& userId, const std::string& role) {
    User* user = getUser(userId);
    return user && user->role == role;
}

bool AuthManager::hasPermission(const std::string& userId, const std::string& permission) {
    User* user = getUser(userId);
    if (!user) return false;

    // منطق بسيط للصلاحيات - يمكن توسيعه
    if (user->role == "admin") return true;
    if (user->role == "user" && permission != "admin_only") return true;
    if (user->role == "guest" && permission == "read") return true;

    return false;
}

std::vector<AuthToken> AuthManager::getUserSessions(const std::string& userId) {
    std::vector<AuthToken> result;
    auto it = userTokens.find(userId);
    if (it != userTokens.end()) {
        for (const std::string& token : it->second) {
            auto tokenIt = activeTokens.find(token);
            if (tokenIt != activeTokens.end()) {
                result.push_back(tokenIt->second);
            }
        }
    }
    return result;
}

void AuthManager::cleanupExpiredTokens() {
    auto now = std::chrono::system_clock::now();

    for (auto it = activeTokens.begin(); it != activeTokens.end();) {
        if (it->second.isExpired()) {
            std::string userId = it->second.userId;
            auto& tokens = userTokens[userId];
            tokens.erase(std::remove(tokens.begin(), tokens.end(), it->first), tokens.end());
            it = activeTokens.erase(it);
        } else {
            ++it;
        }
    }
}

std::string AuthManager::hashPassword(const std::string& password) {
    return Crypto::hashPassword(password, Crypto::generateSalt());
}

bool AuthManager::verifyPassword(const std::string& password, const std::string& hash) {
    // استخراج الsalt من hash (format: salt:hash)
    size_t colonPos = hash.find(':');
    if (colonPos == std::string::npos) return false;

    std::string salt = hash.substr(0, colonPos);
    std::string expectedHash = hash.substr(colonPos + 1);

    return Crypto::verifyPassword(password, expectedHash, salt);
}

std::string AuthManager::generateToken() {
    // إنشاء رمز عشوائي آمن
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (int i = 0; i < 32; ++i) { // 128-bit token
        ss << std::setw(1) << dis(gen);
    }

    return ss.str();
}

std::string AuthManager::generateUserId() {
    static int counter = 0;
    std::stringstream ss;
    ss << "user_" << std::time(nullptr) << "_" << ++counter;
    return ss.str();
}

void AuthManager::loadDefaultUsers() {
    // إنشاء مستخدم افتراضي للاختبار
    createUser("admin", "admin@arabic-lang.dev", "admin123", "admin");
    createUser("user", "user@arabic-lang.dev", "user123", "user");
    createUser("guest", "guest@arabic-lang.dev", "guest123", "guest");
}

// ==================== AuthMiddleware Implementation ====================

AuthMiddleware::AuthMiddleware(std::shared_ptr<AuthManager> manager)
    : authManager(manager) {

    // مسارات عامة افتراضية
    publicPaths = {
        "/login",
        "/register",
        "/health",
        "/favicon.ico",
        "/static/"
    };
}

AuthMiddleware::~AuthMiddleware() {}

void AuthMiddleware::addPublicPath(const std::string& path) {
    publicPaths.push_back(path);
}

void AuthMiddleware::addAdminPath(const std::string& path) {
    adminPaths.push_back(path);
}

void AuthMiddleware::setRoleRequirement(const std::string& path, const std::string& role) {
    roleRequirements[path] = role;
}

AuthResult AuthMiddleware::checkAuthentication(const HTTPRequest& request) {
    // تحقق إذا كان المسار عام
    if (isPublicPath(request.path)) {
        AuthResult result(true);
        result.message = "Public path - no authentication required";
        return result;
    }

    // استخراج الرمز المميز
    std::string token = extractToken(request);
    if (token.empty()) {
        AuthResult result(false);
        result.message = "No authentication token provided";
        return result;
    }

    // التحقق من صحة الرمز
    AuthResult authResult = authManager->validateToken(token);
    if (!authResult.success) {
        return authResult;
    }

    // التحقق من الصلاحيات المطلوبة
    auto it = roleRequirements.find(request.path);
    if (it != roleRequirements.end()) {
        if (!authManager->hasRole(authResult.userId, it->second)) {
            AuthResult result(false);
            result.message = "Insufficient permissions";
            return result;
        }
    }

    return authResult;
}

bool AuthMiddleware::isPublicPath(const std::string& path) const {
    for (const std::string& publicPath : publicPaths) {
        if (path.find(publicPath) == 0) {
            return true;
        }
    }
    return false;
}

std::string AuthMiddleware::extractToken(const HTTPRequest& request) const {
    // البحث عن الرمز في رؤوس مختلفة
    std::string token = request.getHeader("Authorization");

    // إزالة "Bearer " إذا كان موجوداً
    if (token.find("Bearer ") == 0) {
        token = token.substr(7);
    }

    // البحث في رؤوس أخرى
    if (token.empty()) {
        token = request.getHeader("X-Auth-Token");
    }

    // البحث في query parameters
    if (token.empty()) {
        token = request.getQueryParam("token");
    }

    return token;
}

// ==================== JWT Implementation ====================

std::string JWT::encode(const std::map<std::string, std::string>& payload,
                       const std::string& secret) {
    // رأس JWT
    std::map<std::string, std::string> header = {
        {"alg", "HS256"},
        {"typ", "JWT"}
    };

    // ترميز الرأس والحمولة
    std::string headerB64 = base64Encode("{\"alg\":\"HS256\",\"typ\":\"JWT\"}");

    std::string payloadJson = "{";
    for (auto it = payload.begin(); it != payload.end(); ++it) {
        if (it != payload.begin()) payloadJson += ",";
        payloadJson += "\"" + it->first + "\":\"" + it->second + "\"";
    }
    payloadJson += "}";
    std::string payloadB64 = base64Encode(payloadJson);

    // إنشاء التوقيع
    std::string data = headerB64 + "." + payloadB64;
    std::string signature = base64Encode(Crypto::hashSHA256(data + secret));

    return data + "." + signature;
}

std::map<std::string, std::string> JWT::decode(const std::string& token,
                                              const std::string& secret) {
    std::map<std::string, std::string> result;

    // تقسيم الرمز إلى أجزائه
    size_t dot1 = token.find('.');
    size_t dot2 = token.find('.', dot1 + 1);

    if (dot1 == std::string::npos || dot2 == std::string::npos) {
        return result;
    }

    std::string headerB64 = token.substr(0, dot1);
    std::string payloadB64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
    std::string signatureB64 = token.substr(dot2 + 1);

    // التحقق من التوقيع
    std::string data = headerB64 + "." + payloadB64;
    std::string expectedSignature = base64Encode(Crypto::hashSHA256(data + secret));

    if (signatureB64 != expectedSignature) {
        return result; // توقيع غير صحيح
    }

    // فك ترميز الحمولة
    try {
        std::string payloadJson = base64Decode(payloadB64);
        // تحليل JSON البسيط (يمكن تحسينه)
        size_t pos = 0;
        while ((pos = payloadJson.find("\"", pos)) != std::string::npos) {
            size_t keyStart = pos + 1;
            size_t keyEnd = payloadJson.find("\"", keyStart);
            if (keyEnd == std::string::npos) break;

            std::string key = payloadJson.substr(keyStart, keyEnd - keyStart);

            pos = payloadJson.find(":", keyEnd);
            if (pos == std::string::npos) break;

            pos = payloadJson.find("\"", pos);
            if (pos == std::string::npos) break;

            size_t valueStart = pos + 1;
            size_t valueEnd = payloadJson.find("\"", valueStart);
            if (valueEnd == std::string::npos) break;

            std::string value = payloadJson.substr(valueStart, valueEnd - valueStart);
            result[key] = value;

            pos = valueEnd + 1;
        }
    } catch (...) {
        // خطأ في فك الترميز
    }

    return result;
}

bool JWT::verify(const std::string& token, const std::string& secret) {
    return !decode(token, secret).empty();
}

// ==================== Crypto Implementation ====================

std::string Crypto::hashSHA256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::setw(2) << (int)hash[i];
    }

    return ss.str();
}

std::string Crypto::generateSalt(size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::string salt;
    for (size_t i = 0; i < length; ++i) {
        salt += static_cast<char>(dis(gen));
    }

    return base64Encode(salt);
}

std::string Crypto::hashPassword(const std::string& password, const std::string& salt) {
    std::string combined = salt + password;
    std::string hash = hashSHA256(combined);
    return salt + ":" + hash;
}

bool Crypto::verifyPassword(const std::string& password, const std::string& hash, const std::string& salt) {
    std::string expectedHash = hashSHA256(salt + password);
    return expectedHash == hash;
}

} // namespace ArabicLanguage
