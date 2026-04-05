// ArabicEduContent.cpp - تطبيق مكتبة المحتوى التعليمي العربي
#include "ArabicEduContent.h"
#include <iostream>
#include <algorithm>

namespace ArabicEdu {

    std::map<std::string, std::vector<Resource>> ContentLibrary::repository;

    void ContentLibrary::addResource(const std::string& subject, const Resource& res) {
        repository[subject].push_back(res);
        std::cout << "📚 تم إضافة مورد جديد لمادة [" << subject << "]: " << res.title << std::endl;
    }

    std::vector<Resource> ContentLibrary::getResourcesBySubject(const std::string& subject) {
        if (repository.count(subject)) return repository[subject];
        return {};
    }

    std::string ContentLibrary::getCodeSample(const std::string& topic) {
        if (topic == "مرحبا_بالعالم") {
            return "اطبع(\"مرحباً بك في عالم البرمجة بالعربية!\")";
        }
        return "// لا يوجد مثال متاح لهذا الموضوع حالياً";
    }

    std::vector<Resource> ContentLibrary::searchContent(const std::string& query) {
        std::vector<Resource> results;
        for (auto const& [subject, resources] : repository) {
            for (auto const& res : resources) {
                if (res.title.find(query) != std::string::npos || res.data.find(query) != std::string::npos) {
                    results.push_back(res);
                }
            }
        }
        return results;
    }

    std::vector<std::string> ContentLibrary::getAvailableSubjects() {
        std::vector<std::string> subjects;
        for (auto const& [subject, _] : repository) {
            subjects.push_back(subject);
        }
        return subjects;
    }

} // namespace ArabicEdu
