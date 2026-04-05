// ArabicHybridBridge.h - جسور المنصات الهجينة (React Native & Flutter)
#ifndef ARABIC_HYBRID_BRIDGE_H
#define ARABIC_HYBRID_BRIDGE_H

#include <string>
#include <map>
#include <functional>
#include <vector>

namespace ArabicHybrid {

    // أنواع المنصات المدعومة
    enum class HybridPlatform {
        REACT_NATIVE,
        FLUTTER,
        UNITY,
        WEB_ASSEMBLY
    };

    // هيكل البيانات المتبادلة (JSON-like simple structure)
    struct BridgeData {
        std::string action;
        std::string payload;
        std::map<std::string, std::string> metadata;
    };

    class HybridBridge {
    public:
        // تهيئة الجسر لمنصة معينة
        static void initialize(HybridPlatform platform);

        // إرسال البيانات من C++ إلى المنصة الهجينة
        static void sendMessageToHost(const BridgeData& data);

        // استقبال البيانات من المنصة الهجينة
        static void onMessageFromHost(const std::string& jsonString);

        // تسجيل الدوال المتاحة للمنصة الهجينة
        static void registerMethod(const std::string& name, std::function<std::string(std::string)> func);

        // تحويل البيانات إلى JSON (تبسيط)
        static std::string serialize(const BridgeData& data);
        static BridgeData deserialize(const std::string& json);

    private:
        static HybridPlatform currentPlatform;
        static std::map<std::string, std::function<std::string(std::string)>> registeredMethods;
    };

} // namespace ArabicHybrid

#endif // ARABIC_HYBRID_BRIDGE_H
