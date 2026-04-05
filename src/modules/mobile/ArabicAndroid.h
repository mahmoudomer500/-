// ArabicAndroid.h - مكتبة دعم تطبيقات Android باللغة العربية
// الأسبوع الأول من الشهر السابع: مكتبة Android

#ifndef ARABIC_ANDROID_H
#define ARABIC_ANDROID_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <cstdint>

namespace ArabicLanguage {

/**
 * @brief أنواع ميزات الجهاز المحمول
 */
enum class MobileFeature {
    CAMERA,
    GPS,
    BATTERY,
    ACCELEROMETER,
    VIBRATOR,
    NOTIFICATIONS
};

/**
 * @brief معلومات البطارية
 */
struct BatteryInfo {
    int level;          // 0-100
    bool isCharging;
    float temperature;
};

/**
 * @brief معلومات الموقع الجغرافي
 */
struct LocationInfo {
    double latitude;
    double longitude;
    double altitude;
    float accuracy;
};

/**
 * @brief واجهة الوصول إلى ميزات Android
 */
class ArabicAndroid {
public:
    static ArabicAndroid& getInstance();

    // تهيئة البيئة (JNI context)
    void initialize(void* jniEnv, void* context);

    // ميزات الجهاز
    BatteryInfo getBatteryInfo();
    LocationInfo getCurrentLocation();
    void vibrate(int durationMs);
    void showToast(const std::string& message, bool isLong = false);
    void sendNotification(const std::string& title, const std::string& message);

    // إدارة الكاميرا
    bool openCamera();
    void closeCamera();
    // سيتم إضافة دوال التقاط الصور والفيديو لاحقاً

    // التحقق من الصلاحيات
    bool hasPermission(const std::string& permission);
    void requestPermission(const std::string& permission, std::function<void(bool)> callback);

    // واجهة المستخدم المحمولة
    void setStatusBarColor(uint32_t color);
    void setKeepScreenOn(bool on);

private:
    ArabicAndroid() = default;
    ~ArabicAndroid() = default;
    ArabicAndroid(const ArabicAndroid&) = delete;
    ArabicAndroid& operator=(const ArabicAndroid&) = delete;

    void* _jniEnv = nullptr;
    void* _context = nullptr;
};

/**
 * @brief جسر لربط الكود العربي مع واجهات Android UI
 */
class AndroidBridge {
public:
    static void callJavaVoidMethod(const std::string& className, const std::string& methodName, const std::string& signature, ...);
    static std::string callJavaStringMethod(const std::string& className, const std::string& methodName, const std::string& signature, ...);
    static int callJavaIntMethod(const std::string& className, const std::string& methodName, const std::string& signature, ...);
};

} // namespace ArabicLanguage

#endif // ARABIC_ANDROID_H
