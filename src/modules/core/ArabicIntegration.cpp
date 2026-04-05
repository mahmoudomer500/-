// ArabicIntegration.cpp - تطبيق نظام التكامل مع الأنظمة الخارجية
#include "ArabicIntegration.h"
#include <iostream>
#include <sstream>

namespace ArabicIntegration {

    std::map<std::string, APIConfig> IntegrationManager::connections;

    void IntegrationManager::setupConnection(const std::string& systemName, const APIConfig& config) {
        connections[systemName] = config;
        std::cout << "🔗 تم إعداد الاتصال بنظام: " << systemName << " عبر بروتوكول " << (int)config.protocol << std::endl;
    }

    std::string IntegrationManager::sendRequest(const std::string& systemName, const std::string& method, const std::string& payload) {
        if (!connections.count(systemName)) {
            return "Error: System not found";
        }
        
        std::cout << "📤 إرسال طلب [" << method << "] إلى " << systemName << "..." << std::endl;
        // محاكاة استجابة ناجحة
        return "{\"status\": \"success\", \"message\": \"تمت المعالجة بنجاح\"}";
    }

    void IntegrationManager::onReceiveData(const std::string& systemName, const std::string& data) {
        std::cout << "📥 استقبال بيانات من " << systemName << ": " << data << std::endl;
    }

    bool IntegrationManager::connectToBigData(const std::string& connectionString) {
        std::cout << "🐘 جاري الاتصال بقاعدة بيانات ضخمة: " << connectionString << std::endl;
        return true;
    }

    void IntegrationManager::syncData(const std::string& tableName) {
        std::cout << "🔄 جاري مزامنة الجدول: " << tableName << "..." << std::endl;
    }

    std::string IntegrationManager::toJSON(const std::map<std::string, std::string>& data) {
        std::stringstream ss;
        ss << "{";
        for (auto it = data.begin(); it != data.end(); ++it) {
            ss << "\"" << it->first << "\": \"" << it->second << "\"";
            if (std::next(it) != data.end()) ss << ", ";
        }
        ss << "}";
        return ss.str();
    }

    std::map<std::string, std::string> IntegrationManager::fromJSON(const std::string& json) {
        // محاكاة بسيطة للتحويل من JSON
        std::map<std::string, std::string> result;
        result["status"] = "success";
        return result;
    }

} // namespace ArabicIntegration
