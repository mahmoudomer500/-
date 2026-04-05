// ArabicMobileOptimizer.h - تحسين الأداء للهواتف المحمولة
#ifndef ARABIC_MOBILE_OPTIMIZER_H
#define ARABIC_MOBILE_OPTIMIZER_H

#include <string>
#include <vector>

namespace ArabicMobile {

    enum class OptimizationLevel {
        BALANCED,
        PERFORMANCE,
        BATTERY_SAVER
    };

    class MobileOptimizer {
    public:
        // ضبط مستوى التحسين
        static void setOptimizationLevel(OptimizationLevel level);

        // إدارة الذاكرة
        static void clearUnusedResources();
        static size_t getCurrentMemoryUsage();

        // تحسين الرسوميات
        static void enableLowResMode(bool enable);
        static void optimizeShadersForMobile();

        // إدارة الطاقة
        static void onBatteryLevelChanged(float level);
        static void setFrameRateLimit(int fps);

        // أدوات التشخيص
        static std::string getPerformanceReport();
    };

} // namespace ArabicMobile

#endif // ARABIC_MOBILE_OPTIMIZER_H
