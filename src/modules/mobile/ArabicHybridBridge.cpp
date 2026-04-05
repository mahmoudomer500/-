// ArabicHybridBridge.cpp - تطبيق جسور المنصات الهجينة
#include "ArabicHybridBridge.h"
#include <iostream>
#include <sstream>

namespace ArabicHybrid {

    HybridPlatform HybridBridge::currentPlatform = HybridPlatform::REACT_NATIVE;
    std::map<std::string, std::function<std::string(std::string)>> HybridBridge::registeredMethods;

    void HybridBridge::initialize(HybridPlatform platform) {
        currentPlatform = platform;
        std::string platformName;
        switch (platform) {
            case HybridPlatform::REACT_NATIVE: platformName = "React Native"; break;
            case HybridPlatform::FLUTTER: platformName = "Flutter"; break;
            case HybridPlatform::UNITY: platformName = "Unity"; break;
            case HybridPlatform::WEB_ASSEMBLY: platformName = "WebAssembly"; break;
        }
        std::cout << "🌐 تم تهيئة الجسر الهجين للمنصة: " << platformName << std::endl;
    }

    void HybridBridge::sendMessageToHost(const BridgeData& data) {
        std::string json = serialize(data);
        std::cout << "📤 إرسال إلى المضيف: " << json << std::endl;
        // هنا يتم استدعاء Native Module API الخاص بالمنصة
    }

    void HybridBridge::onMessageFromHost(const std::string& jsonString) {
        BridgeData data = deserialize(jsonString);
        std::cout << "📥 استقبال من المضيف: " << data.action << std::endl;

        if (registeredMethods.count(data.action)) {
            std::string result = registeredMethods[data.action](data.payload);
            sendMessageToHost({ "RESPONSE_" + data.action, result, {} });
        }
    }

    void HybridBridge::registerMethod(const std::string& name, std::function<std::string(std::string)> func) {
        registeredMethods[name] = func;
        std::cout << "📝 تم تسجيل الدالة: " << name << std::endl;
    }

    std::string HybridBridge::serialize(const BridgeData& data) {
        // محاكاة بسيطة لتحويل البيانات إلى JSON
        std::stringstream ss;
        ss << "{\"action\":\"" << data.action << "\", \"payload\":\"" << data.payload << "\"}";
        return ss.str();
    }

    BridgeData HybridBridge::deserialize(const std::string& json) {
        // محاكاة بسيطة جداً لاستخراج البيانات من JSON
        BridgeData data;
        if (json.find("action") != std::string::npos) {
            data.action = "SAMPLE_ACTION";
            data.payload = "SAMPLE_PAYLOAD";
        }
        return data;
    }

} // namespace ArabicHybrid
