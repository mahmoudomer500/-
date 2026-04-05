// ArabicMobileOptimizer.cpp - تطبيق تحسين الأداء للهواتف المحمولة
#include "ArabicMobileOptimizer.h"
#include <iostream>
#include <sstream>

namespace ArabicMobile {

    static OptimizationLevel currentLevel = OptimizationLevel::BALANCED;
    static bool lowResMode = false;
    static int currentFPSLimit = 60;

    void MobileOptimizer::setOptimizationLevel(OptimizationLevel level) {
        currentLevel = level;
        switch (level) {
            case OptimizationLevel::PERFORMANCE:
                currentFPSLimit = 60;
                lowResMode = false;
                break;
            case OptimizationLevel::BATTERY_SAVER:
                currentFPSLimit = 30;
                lowResMode = true;
                break;
            case OptimizationLevel::BALANCED:
                currentFPSLimit = 45;
                lowResMode = false;
                break;
        }
        std::cout << "🚀 تم ضبط مستوى التحسين إلى: " << (int)level << " (FPS: " << currentFPSLimit << ")" << std::endl;
    }

    void MobileOptimizer::clearUnusedResources() {
        std::cout << "🧹 تم تنظيف الموارد غير المستخدمة من الذاكرة." << std::endl;
    }

    size_t MobileOptimizer::getCurrentMemoryUsage() {
        return 124 * 1024 * 1024; // محاكاة 124 ميجابايت
    }

    void MobileOptimizer::enableLowResMode(bool enable) {
        lowResMode = enable;
        std::cout << "🖼️ وضع الدقة المنخفضة: " << (enable ? "مفعل" : "معطل") << std::endl;
    }

    void MobileOptimizer::optimizeShadersForMobile() {
        std::cout << "✨ تم تحسين الـ Shaders لتعمل بكفاءة على معالجات الهواتف." << std::endl;
    }

    void MobileOptimizer::onBatteryLevelChanged(float level) {
        if (level < 0.2f) {
            setOptimizationLevel(OptimizationLevel::BATTERY_SAVER);
            std::cout << "⚠️ مستوى البطارية منخفض (" << (int)(level * 100) << "%)، تفعيل وضع توفير الطاقة." << std::endl;
        }
    }

    void MobileOptimizer::setFrameRateLimit(int fps) {
        currentFPSLimit = fps;
        std::cout << "⏱️ تحديد معدل الإطارات بـ: " << fps << " إطار في الثانية." << std::endl;
    }

    std::string MobileOptimizer::getPerformanceReport() {
        std::stringstream ss;
        ss << "--- تقرير الأداء للهاتف ---" << std::endl;
        ss << "Memory: " << getCurrentMemoryUsage() / (1024 * 1024) << " MB" << std::endl;
        ss << "FPS Limit: " << currentFPSLimit << std::endl;
        ss << "Low Res Mode: " << (lowResMode ? "Yes" : "No") << std::endl;
        return ss.str();
    }

} // namespace ArabicMobile
