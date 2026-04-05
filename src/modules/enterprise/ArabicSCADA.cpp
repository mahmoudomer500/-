// ArabicSCADA.cpp - تطبيق نظام المراقبة والتحكم الصناعي
#include "ArabicSCADA.h"
#include <iostream>
#include <numeric>

namespace ArabicSCADA {

    std::map<int, Machine> SCADAManager::factoryFloor;
    std::vector<std::string> SCADAManager::alerts;

    void SCADAManager::registerMachine(int id, const std::string& name) {
        Machine m;
        m.id = id;
        m.name = name;
        m.isRunning = false;
        m.thresholdMax = 100.0; // افتراضي
        m.thresholdMin = 0.0;
        factoryFloor[id] = m;
        std::cout << "🏭 تم تسجيل آلة جديدة في النظام: " << name << std::endl;
    }

    void SCADAManager::setMachineState(int id, bool start) {
        if (factoryFloor.count(id)) {
            factoryFloor[id].isRunning = start;
            std::cout << "⚙️ حالة الآلة " << factoryFloor[id].name << ": " << (start ? "تعمل" : "متوقفة") << std::endl;
        }
    }

    bool SCADAManager::isMachineRunning(int id) {
        return factoryFloor.count(id) && factoryFloor[id].isRunning;
    }

    void SCADAManager::updateSensorValue(int machineId, SensorType type, double value) {
        if (!factoryFloor.count(machineId)) return;

        bool alert = (value > factoryFloor[machineId].thresholdMax || value < factoryFloor[machineId].thresholdMin);
        SensorData data = { value, "2026-08-24 10:00:00", alert };

        auto& history = factoryFloor[machineId].history[type];
        history.push_back(data);
        if (history.size() > MAX_HISTORY_SIZE) history.pop_front();

        if (alert) {
            std::string alertMsg = "⚠️ تنبيه من " + factoryFloor[machineId].name + ": قيمة الحساس خارج الحدود (" + std::to_string(value) + ")";
            alerts.push_back(alertMsg);
            std::cout << alertMsg << std::endl;
        }
    }

    std::vector<SensorData> SCADAManager::getSensorHistory(int machineId, SensorType type, int lastN) {
        std::vector<SensorData> result;
        if (factoryFloor.count(machineId) && factoryFloor[machineId].history.count(type)) {
            auto const& history = factoryFloor[machineId].history[type];
            int count = 0;
            for (auto it = history.rbegin(); it != history.rend() && count < lastN; ++it, ++count) {
                result.push_back(*it);
            }
        }
        return result;
    }

    std::vector<std::string> SCADAManager::getActiveAlerts() {
        return alerts;
    }

    double SCADAManager::calculateAverageValue(int machineId, SensorType type, int lastN) {
        auto history = getSensorHistory(machineId, type, lastN);
        if (history.empty()) return 0.0;
        
        double sum = 0;
        for (auto const& h : history) sum += h.value;
        return sum / history.size();
    }

} // namespace ArabicSCADA
