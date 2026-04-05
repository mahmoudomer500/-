// ArabicIntegration.h - نظام التكامل مع الأنظمة الخارجية باللغة العربية
#ifndef ARABIC_INTEGRATION_H
#define ARABIC_INTEGRATION_H

#include <string>
#include <vector>
#include <map>

namespace ArabicIntegration {

    enum class Protocol {
        REST,
        SOAP,
        GRAPHQL,
        GRPC
    };

    struct APIConfig {
        std::string endpoint;
        std::string apiKey;
        Protocol protocol;
        int timeoutMs;
    };

    class IntegrationManager {
    public:
        // إدارة الاتصالات
        static void setupConnection(const std::string& systemName, const APIConfig& config);
        
        // تبادل البيانات (REST/SOAP)
        static std::string sendRequest(const std::string& systemName, const std::string& method, const std::string& payload);
        static void onReceiveData(const std::string& systemName, const std::string& data);

        // التكامل مع قواعد البيانات الكبيرة
        static bool connectToBigData(const std::string& connectionString);
        static void syncData(const std::string& tableName);

        // أدوات التحويل (Serialization)
        static std::string toJSON(const std::map<std::string, std::string>& data);
        static std::map<std::string, std::string> fromJSON(const std::string& json);

    private:
        static std::map<std::string, APIConfig> connections;
    };

} // namespace ArabicIntegration

#endif // ARABIC_INTEGRATION_H
