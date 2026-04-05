// ArabicIOS.h - مكتبة دعم تطبيقات iOS باللغة العربية
#ifndef ARABIC_IOS_H
#define ARABIC_IOS_H

#include <string>
#include <vector>
#include <functional>

namespace ArabicOS {

    // أنواع الإشعارات على iOS
    enum class IOSNotificationType {
        Banner,
        Alert,
        Badge,
        Sound
    };

    // معلومات البطارية لنظام iOS
    struct IOSBatteryInfo {
        float level; // 0.0 to 1.0
        bool isCharging;
        bool isLowPowerMode;
    };

    // جسر التواصل مع Objective-C / Swift
    class IOSBridge {
    public:
        static void callSwiftMethod(const std::string& methodName, const std::string& params);
        static void onReceiveFromSwift(const std::string& data);
        static void setSwiftCallback(std::function<void(std::string)> callback);
    private:
        static std::function<void(std::string)> swiftCallback;
    };

    class ArabicIOS {
    public:
        // إدارة النظام
        static void initialize();
        static std::string getDeviceModel();
        static std::string getOSVersion();

        // ميزات الجهاز
        static IOSBatteryInfo getBatteryInfo();
        static void vibrate(int durationMs = 100);
        static void showNativeAlert(const std::string& title, const std::string& message);
        
        // الإشعارات
        static void scheduleNotification(const std::string& title, const std::string& body, int delaySeconds);
        
        // واجهة المستخدم
        static float getSafeAreaTop();
        static float getSafeAreaBottom();
        static void setStatusBarHidden(bool hidden);
    };

} // namespace ArabicOS

#endif // ARABIC_IOS_H
