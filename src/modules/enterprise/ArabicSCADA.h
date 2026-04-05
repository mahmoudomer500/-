// ArabicSCADA.h - نظام المراقبة والتحكم الصناعي باللغة العربية
#ifndef ARABIC_SCADA_H
#define ARABIC_SCADA_H

#include <string>
#include <vector>
#include <map>
#include <deque>

namespace ArabicSCADA {

    // أنواع الحساسات الصناعية
    enum class SensorType {
        TEMPERATURE, // حرارة
        PRESSURE,    // ضغط
        FLOW,        // تدفق
        VOLTAGE      // جهد كهربائي
    };

    struct SensorData {
        double value;
        std::string timestamp;
        bool isAlert;
    };

    struct Machine {
        int id;
        std::string name;
        bool isRunning;
        std::map<SensorType, std::deque<SensorData>> history;
        double thresholdMax;
        double thresholdMin;
    };

    class SCADAManager {
    public:
        // إدارة الآلات
        static void registerMachine(int id, const std::string& name);
        static void setMachineState(int id, bool start);
        static bool isMachineRunning(int id);

        // مراقبة البيانات
        static void updateSensorValue(int machineId, SensorType type, double value);
        static std::vector<SensorData> getSensorHistory(int machineId, SensorType type, int lastN);
        
        // التنبيهات والتحليل
        static std::vector<std::string> getActiveAlerts();
        static double calculateAverageValue(int machineId, SensorType type, int lastN);

    private:
        static std::map<int, Machine> factoryFloor;
        static std::vector<std::string> alerts;
        static const int MAX_HISTORY_SIZE = 100;
    };

} // namespace ArabicSCADA

#endif // ARABIC_SCADA_H
