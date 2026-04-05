// ArabicEduAnalytics.h - أدوات التقييم والتحليل التعليمي
#ifndef ARABIC_EDU_ANALYTICS_H
#define ARABIC_EDU_ANALYTICS_H

#include <string>
#include <vector>
#include <map>

namespace ArabicEdu {

    struct StudentStats {
        int studentId;
        float averageQuizScore;
        int completedCourses;
        int activeHours;
        std::map<std::string, float> subjectPerformance; // مادة -> درجة
    };

    class AnalyticsManager {
    public:
        // تحليل أداء الطالب
        static StudentStats getStudentReport(int studentId);
        static void recordActivity(int studentId, int hours);

        // تقارير عامة
        static float getClassAverage(int courseId);
        static std::vector<int> getTopStudents(int count);

        // تخصيص المحتوى (Personalization)
        static std::vector<std::string> recommendTopics(int studentId);

        // توليد تقارير بصيغة نصية عربية
        static std::string generateSummaryReport(int studentId);

    private:
        static std::map<int, StudentStats> allStats;
    };

} // namespace ArabicEdu

#endif // ARABIC_EDU_ANALYTICS_H
