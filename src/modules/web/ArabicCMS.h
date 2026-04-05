// ArabicCMS.h - نظام إدارة المحتوى باللغة العربية
#ifndef ARABIC_CMS_H
#define ARABIC_CMS_H

#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace ArabicCMS {

    // أنواع المستخدمين وصلاحياتهم
    enum class UserRole {
        ADMIN,      // مدير النظام
        EDITOR,     // محرر
        AUTHOR,     // كاتب
        SUBSCRIBER  // مشترك
    };

    struct User {
        int id;
        std::string username;
        std::string email;
        UserRole role;
        bool isActive;
    };

    struct Article {
        int id;
        std::string title;
        std::string content;
        int authorId;
        std::string category;
        std::vector<std::string> tags;
        std::chrono::system_clock::time_point createdAt;
        bool isPublished;
    };

    class CMSManager {
    public:
        // إدارة المستخدمين
        static bool registerUser(const std::string& username, const std::string& email, UserRole role);
        static User* getUser(int id);
        static bool checkPermission(int userId, const std::string& action);

        // إدارة المقالات
        static int createArticle(const std::string& title, const std::string& content, int authorId);
        static bool updateArticle(int id, const std::string& title, const std::string& content);
        static bool publishArticle(int id);
        static std::vector<Article> getLatestArticles(int count);
        static Article* getArticle(int id);

        // تصنيفات ووسوم
        static void addCategory(const std::string& category);
        static std::vector<std::string> getCategories();

    private:
        static std::map<int, User> users;
        static std::map<int, Article> articles;
        static std::vector<std::string> categories;
        static int nextUserId;
        static int nextArticleId;
    };

} // namespace ArabicCMS

#endif // ARABIC_CMS_H
