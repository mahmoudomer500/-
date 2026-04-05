// ArabicEduAnalytics.cpp - تطبيق أدوات التقييم والتحليل التعليمي
#include "ArabicEduAnalytics.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace ArabicEdu {

    std::map<int, StudentStats> AnalyticsManager::allStats;

    StudentStats AnalyticsManager::getStudentReport(int studentId) {
        if (allStats.count(studentId)) return allStats[studentId];
        return { studentId, 0.0f, 0, 0, {} };
    }

    void AnalyticsManager::recordActivity(int studentId, int hours) {
        allStats[studentId].studentId = studentId;
        allStats[studentId].activeHours += hours;
        std::cout << "⏱️ تم تسجيل " << hours << " ساعات نشاط للطالب رقم: " << studentId << std::endl;
    }

    float AnalyticsManager::getClassAverage(int courseId) {
        if (allStats.empty()) return 0.0f;
        float sum = 0;
        for (auto const& [id, stats] : allStats) sum += stats.averageQuizScore;
        return sum / allStats.size();
    }

    std::vector<int> AnalyticsManager::getTopStudents(int count) {
        std::vector<std::pair<int, float>> scores;
        for (auto const& [id, stats] : allStats) scores.push_back({id, stats.averageQuizScore});
        
        std::sort(scores.begin(), scores.end(), [](auto& a, auto& b){ return a.second > b.second; });
        
        std::vector<int> top;
        for (int i = 0; i < std::min((int)scores.size(), count); ++i) top.push_back(scores[i].first);
        return top;
    }

    std::vector<std::string> AnalyticsManager::recommendTopics(int studentId) {
        std::vector<std::string> recommendations;
        auto stats = getStudentReport(studentId);
        if (stats.averageQuizScore < 50.0f) {
            recommendations.push_back("أساسيات البرمجة");
            recommendations.push_back("مراجعة الدروس السابقة");
        } else {
            recommendations.push_back("مفاهيم متقدمة");
            recommendations.push_back("مشاريع تطبيقية");
        }
        return recommendations;
    }

    std::string AnalyticsManager::generateSummaryReport(int studentId) {
        auto stats = getStudentReport(studentId);
        std::stringstream ss;
        ss << "--- تقرير أداء الطالب العربي ---" << std::endl;
        ss << "رقم الطالب: " << stats.studentId << std::endl;
        ss << "متوسط الدرجات: " << stats.averageQuizScore << "%" << std::endl;
        ss << "الدورات المكتملة: " << stats.completedCourses << std::endl;
        ss << "ساعات النشاط: " << stats.activeHours << std::endl;
        ss << "-------------------------------" << std::endl;
        return ss.str();
    }

} // namespace ArabicEdu
