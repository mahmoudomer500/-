// WebAuth.h - نظام مصادقة الويب
// الأسبوع الرابع من الشهر الخامس: أمان الويب
#ifndef WEB_AUTH_H
#define WEB_AUTH_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include "HTTPServer.h"

namespace ArabicLanguage {

// بيانات المستخدم
struct User {
    std::string id;
    std::string username;
    std::string email;
    std::string passwordHash;
    std::string role; // admin, user, guest
    bool isActive;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastLogin;

    User() : isActive(true) {}
};

// رمز المصادقة (Token)
struct AuthToken {
    std::string token;
    std::string userId;
    std::chrono::system_clock::time_point expiresAt;
    std::string ipAddress;
    std::string userAgent;

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

// مدير المصادقة
class AuthManager {
private:
    std::map<std::string, User> users; // username -> User
    std::map<std::string, AuthToken> activeTokens; // token -> AuthToken
    std::map<std::string, std::vector<std::string>> userTokens; // userId -> tokens

    // إعدادات الأمان
    int tokenExpirationHours;
    std::string jwtSecret;
    bool enableSessionTimeout;

public:
    AuthManager();
    ~AuthManager();

    // إدارة المستخدمين
    bool createUser(const std::string& username, const std::string& email,
                   const std::string& password, const std::string& role = "user");
    bool updateUser(const std::string& userId, const User& updates);
    bool deleteUser(const std::string& userId);
    User* getUser(const std::string& userId);
    User* getUserByUsername(const std::string& username);
    std::vector<User> getAllUsers();

    // المصادقة
    AuthResult authenticate(const std::string& username, const std::string& password,
                          const std::string& ipAddress = "", const std::string& userAgent = "");
    AuthResult validateToken(const std::string& token);
    bool revokeToken(const std::string& token);
    void revokeAllUserTokens(const std::string& userId);

    // التحقق من الصلاحيات
    bool hasRole(const std::string& userId, const std::string& role);
    bool hasPermission(const std::string& userId, const std::string& permission);

    // إدارة الجلسات
    std::vector<AuthToken> getUserSessions(const std::string& userId);
    void cleanupExpiredTokens();

    // إعدادات
    void setTokenExpiration(int hours) { tokenExpirationHours = hours; }
    void setJWTSecret(const std::string& secret) { jwtSecret = secret; }
    void setSessionTimeoutEnabled(bool enable) { enableSessionTimeout = enable; }

private:
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    std::string generateToken();
    std::string generateUserId();
    void loadDefaultUsers();
};

// مكون وسطي للمصادقة
class AuthMiddleware {
private:
    std::shared_ptr<AuthManager> authManager;
    std::vector<std::string> publicPaths; // مسارات لا تحتاج مصادقة
    std::vector<std::string> adminPaths;  // مسارات تحتاج صلاحية admin
    std::map<std::string, std::string> roleRequirements; // path -> required_role

public:
    AuthMiddleware(std::shared_ptr<AuthManager> manager);
    ~AuthMiddleware();

    // إضافة مسارات
    void addPublicPath(const std::string& path);
    void addAdminPath(const std::string& path);
    void setRoleRequirement(const std::string& path, const std::string& role);

    // التحقق من المصادقة
    AuthResult checkAuthentication(const HTTPRequest& request);
    bool isPublicPath(const std::string& path) const;

    // استخراج الرمز من الطلب
    std::string extractToken(const HTTPRequest& request) const;
};

// دوال مساعدة للـ JWT (مبسطة)
namespace JWT {
    std::string encode(const std::map<std::string, std::string>& payload,
                      const std::string& secret);
    std::map<std::string, std::string> decode(const std::string& token,
                                             const std::string& secret);
    bool verify(const std::string& token, const std::string& secret);
}

// دوال تشفير مساعدة
namespace Crypto {
    std::string hashSHA256(const std::string& input);
    std::string generateSalt(size_t length = 16);
    std::string hashPassword(const std::string& password, const std::string& salt);
    bool verifyPassword(const std::string& password, const std::string& hash, const std::string& salt);
}

} // namespace ArabicLanguage

#endif // WEB_AUTH_H
