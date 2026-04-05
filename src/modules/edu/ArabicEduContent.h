// ArabicEduContent.h - مكتبة المحتوى التعليمي العربي
#ifndef ARABIC_EDU_CONTENT_H
#define ARABIC_EDU_CONTENT_H

#include <string>
#include <vector>
#include <map>

namespace ArabicEdu {

    // أنواع المحتوى
    enum class ContentType {
        TEXTBOOK,   // كتاب نصي
        VIDEO_URL,  // رابط فيديو
        CODE_SAMPLE,// مثال برمجيم
        EXERCISE    // تمرين
    };

    struct Resource {
        std::string title;
        ContentType type;
        std::string data; // نص أو رابط أو كود
        std::string language; // دائماً "Arabic"
    };

    class ContentLibrary {
    public:
        // إدارة المحتوى
        static void addResource(const std::string& subject, const Resource& res);
        static std::vector<Resource> getResourcesBySubject(const std::string& subject);
        
        // أمثلة برمجية جاهزة
        static std::string getCodeSample(const std::string& topic);
        
        // البحث في المحتوى
        static std::vector<Resource> searchContent(const std::string& query);

        // تصنيفات المواد
        static std::vector<std::string> getAvailableSubjects();

    private:
        static std::map<std::string, std::vector<Resource>> repository;
    };

} // namespace ArabicEdu

#endif // ARABIC_EDU_CONTENT_H
