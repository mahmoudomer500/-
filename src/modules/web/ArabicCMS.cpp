// ArabicCMS.cpp - تطبيق نظام إدارة المحتوى
#include "ArabicCMS.h"
#include <iostream>
#include <algorithm>

namespace ArabicCMS {

    std::map<int, User> CMSManager::users;
    std::map<int, Article> CMSManager::articles;
    std::vector<std::string> CMSManager::categories;
    int CMSManager::nextUserId = 1;
    int CMSManager::nextArticleId = 1;

    bool CMSManager::registerUser(const std::string& username, const std::string& email, UserRole role) {
        User newUser = { nextUserId++, username, email, role, true };
        users[newUser.id] = newUser;
        std::cout << "👤 تم تسجيل مستخدم جديد: " << username << " بصلاحية " << (int)role << std::endl;
        return true;
    }

    User* CMSManager::getUser(int id) {
        if (users.count(id)) return &users[id];
        return nullptr;
    }

    bool CMSManager::checkPermission(int userId, const std::string& action) {
        User* user = getUser(userId);
        if (!user || !user->isActive) return false;

        if (user->role == UserRole::ADMIN) return true; // المدير لديه كل الصلاحيات

        if (action == "publish") {
            return user->role == UserRole::EDITOR;
        } else if (action == "write") {
            return user->role == UserRole::AUTHOR || user->role == UserRole::EDITOR;
        }
        
        return false;
    }

    int CMSManager::createArticle(const std::string& title, const std::string& content, int authorId) {
        if (!checkPermission(authorId, "write")) {
            std::cout << "❌ فشل إنشاء المقال: المستخدم ليس لديه صلاحية الكتابة." << std::endl;
            return -1;
        }

        Article newArticle;
        newArticle.id = nextArticleId++;
        newArticle.title = title;
        newArticle.content = content;
        newArticle.authorId = authorId;
        newArticle.createdAt = std::chrono::system_clock::now();
        newArticle.isPublished = false;

        articles[newArticle.id] = newArticle;
        std::cout << "📝 تم إنشاء مسودة مقال: " << title << std::endl;
        return newArticle.id;
    }

    bool CMSManager::updateArticle(int id, const std::string& title, const std::string& content) {
        if (articles.count(id)) {
            articles[id].title = title;
            articles[id].content = content;
            std::cout << "💾 تم تحديث المقال رقم: " << id << std::endl;
            return true;
        }
        return false;
    }

    bool CMSManager::publishArticle(int id) {
        if (articles.count(id)) {
            articles[id].isPublished = true;
            std::cout << "📢 تم نشر المقال: " << articles[id].title << std::endl;
            return true;
        }
        return false;
    }

    std::vector<Article> CMSManager::getLatestArticles(int count) {
        std::vector<Article> latest;
        for (auto it = articles.rbegin(); it != articles.rend() && latest.size() < count; ++it) {
            if (it->second.isPublished) {
                latest.push_back(it->second);
            }
        }
        return latest;
    }

    Article* CMSManager::getArticle(int id) {
        if (articles.count(id)) return &articles[id];
        return nullptr;
    }

    void CMSManager::addCategory(const std::string& category) {
        categories.push_back(category);
    }

    std::vector<std::string> CMSManager::getCategories() {
        return categories;
    }

} // namespace ArabicCMS
