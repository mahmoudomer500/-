// ArabicLMS.h - نظام إدارة التعلم الإلكتروني باللغة العربية
#ifndef ARABIC_LMS_H
#define ARABIC_LMS_H

#include <string>
#include <vector>
#include <map>

namespace ArabicLMS {

    // هيكل الدورة التدريبية
    struct Course {
        int id;
        std::string title;
        std::string instructor;
        std::vector<std::string> lessons;
        int durationHours;
    };

    // هيكل الاختبار
    struct Quiz {
        int id;
        int courseId;
        std::string title;
        std::map<std::string, std::vector<std::string>> questions; // سؤال -> خيارات
        std::map<std::string, int> correctAnswers; // سؤال -> مؤشر الإجابة الصحيحة
    };

    // سجل تقدم الطالب
    struct StudentProgress {
        int studentId;
        int courseId;
        int completedLessons;
        float quizScore;
        bool isCertified;
    };

    class LMSManager {
    public:
        // إدارة الدورات
        static int createCourse(const std::string& title, const std::string& instructor);
        static void addLesson(int courseId, const std::string& lessonTitle);
        static Course* getCourse(int id);

        // إدارة الطلاب والتقدم
        static void enrollStudent(int studentId, int courseId);
        static void updateProgress(int studentId, int courseId, int lessonIndex);
        static StudentProgress* getStudentProgress(int studentId, int courseId);

        // نظام الاختبارات
        static int createQuiz(int courseId, const std::string& title);
        static void addQuestion(int quizId, const std::string& question, const std::vector<std::string>& options, int correctIdx);
        static float gradeQuiz(int studentId, int quizId, const std::vector<int>& studentAnswers);

    private:
        static std::map<int, Course> courses;
        static std::map<int, Quiz> quizzes;
        static std::map<std::pair<int, int>, StudentProgress> progressMap; // {studentId, courseId} -> Progress
        static int nextCourseId;
        static int nextQuizId;
    };

} // namespace ArabicLMS

#endif // ARABIC_LMS_H
