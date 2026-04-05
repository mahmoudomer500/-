// ArabicLMS.cpp - تطبيق نظام إدارة التعلم الإلكتروني
#include "ArabicLMS.h"
#include <iostream>

namespace ArabicLMS {

    std::map<int, Course> LMSManager::courses;
    std::map<int, Quiz> LMSManager::quizzes;
    std::map<std::pair<int, int>, StudentProgress> LMSManager::progressMap;
    int LMSManager::nextCourseId = 101;
    int LMSManager::nextQuizId = 201;

    int LMSManager::createCourse(const std::string& title, const std::string& instructor) {
        Course c = { nextCourseId++, title, instructor, {}, 0 };
        courses[c.id] = c;
        std::cout << "🎓 تم إنشاء دورة تدريبية جديدة: " << title << " بإشراف: " << instructor << std::endl;
        return c.id;
    }

    void LMSManager::addLesson(int courseId, const std::string& lessonTitle) {
        if (courses.count(courseId)) {
            courses[courseId].lessons.push_back(lessonTitle);
            std::cout << "📖 إضافة درس جديد لدورة " << courses[courseId].title << ": " << lessonTitle << std::endl;
        }
    }

    Course* LMSManager::getCourse(int id) {
        if (courses.count(id)) return &courses[id];
        return nullptr;
    }

    void LMSManager::enrollStudent(int studentId, int courseId) {
        if (courses.count(courseId)) {
            progressMap[{studentId, courseId}] = { studentId, courseId, 0, 0.0f, false };
            std::cout << "✅ تم تسجيل الطالب رقم " << studentId << " في دورة " << courses[courseId].title << std::endl;
        }
    }

    void LMSManager::updateProgress(int studentId, int courseId, int lessonIndex) {
        auto key = std::make_pair(studentId, courseId);
        if (progressMap.count(key)) {
            progressMap[key].completedLessons = lessonIndex + 1;
            std::cout << "📈 تقدم الطالب: أتم الدرس رقم " << (lessonIndex + 1) << " في دورة " << courses[courseId].title << std::endl;
        }
    }

    StudentProgress* LMSManager::getStudentProgress(int studentId, int courseId) {
        auto key = std::make_pair(studentId, courseId);
        if (progressMap.count(key)) return &progressMap[key];
        return nullptr;
    }

    int LMSManager::createQuiz(int courseId, const std::string& title) {
        Quiz q;
        q.id = nextQuizId++;
        q.courseId = courseId;
        q.title = title;
        quizzes[q.id] = q;
        std::cout << "📝 تم إنشاء اختبار جديد: " << title << std::endl;
        return q.id;
    }

    void LMSManager::addQuestion(int quizId, const std::string& question, const std::vector<std::string>& options, int correctIdx) {
        if (quizzes.count(quizId)) {
            quizzes[quizId].questions[question] = options;
            quizzes[quizId].correctAnswers[question] = correctIdx;
        }
    }

    float LMSManager::gradeQuiz(int studentId, int quizId, const std::vector<int>& studentAnswers) {
        if (!quizzes.count(quizId)) return 0.0f;
        
        Quiz& q = quizzes[quizId];
        int correctCount = 0;
        int i = 0;
        for (auto const& [question, correctIdx] : q.correctAnswers) {
            if (i < studentAnswers.size() && studentAnswers[i] == correctIdx) {
                correctCount++;
            }
            i++;
        }

        float score = (float)correctCount / q.correctAnswers.size() * 100.0f;
        auto key = std::make_pair(studentId, q.courseId);
        if (progressMap.count(key)) {
            progressMap[key].quizScore = score;
            if (score >= 60.0f) progressMap[key].isCertified = true;
        }

        std::cout << "🎯 نتيجة الاختبار للطالب " << studentId << ": " << score << "%" << std::endl;
        return score;
    }

} // namespace ArabicLMS
