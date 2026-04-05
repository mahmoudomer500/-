// ArabicInteractiveEdu.cpp - تطبيق أدوات التعليم التفاعلي
#include "ArabicInteractiveEdu.h"
#include <iostream>

namespace ArabicEdu {

    std::vector<InteractiveActivity> InteractiveManager::activities;
    std::function<void(float)> InteractiveManager::simCallback = nullptr;

    int InteractiveManager::registerActivity(const std::string& title, ActivityType type, const std::string& desc) {
        InteractiveActivity act = { (int)activities.size() + 1, title, type, desc };
        activities.push_back(act);
        std::cout << "🎮 تم تسجيل نشاط تفاعلي جديد: " << title << std::endl;
        return act.id;
    }

    void InteractiveManager::startSimulator(int activityId) {
        std::cout << "🚀 بدء تشغيل المحاكي التعليمي رقم: " << activityId << std::endl;
        if (simCallback) simCallback(1.0f); // محاكاة تحديث الإطار الأول
    }

    bool InteractiveManager::runCodingExercise(int activityId, const std::string& studentCode, const std::string& expectedOutput) {
        std::cout << "💻 جاري تشغيل تمرين برمجي..." << std::endl;
        // محاكاة تنفيذ الكود والتحقق من النتيجة
        bool success = (studentCode.find("print") != std::string::npos); // محاكاة بسيطة
        if (success) {
            std::cout << "✅ أحسنت! الكود يعمل بشكل صحيح." << std::endl;
        } else {
            std::cout << "❌ حاول مرة أخرى، هناك خطأ في الكود." << std::endl;
        }
        return success;
    }

    void InteractiveManager::playEduGame(int activityId) {
        std::cout << "🕹️ بدء اللعبة التعليمية... استمتع بالتعلم!" << std::endl;
    }

    void InteractiveManager::onGameScoreUpdate(int activityId, int score) {
        std::cout << "🏆 تحديث النتيجة في اللعبة: " << score << " نقطة." << std::endl;
    }

    void InteractiveManager::setSimulatorUpdateCallback(std::function<void(float)> callback) {
        simCallback = callback;
    }

} // namespace ArabicEdu
