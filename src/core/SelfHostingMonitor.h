// SelfHostingMonitor.h - مراقب عملية الإقلاع الذاتي
#ifndef SELF_HOSTING_MONITOR_H
#define SELF_HOSTING_MONITOR_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <functional>

namespace ArabicLanguage {

enum class MonitoringLevel {
    BASIC,      // مراقبة أساسية
    DETAILED,   // مراقبة مفصلة
    COMPREHENSIVE // مراقبة شاملة
};

enum class AlertSeverity {
    INFO,
    WARNING,
    CRITICAL,
    FATAL
};

struct MonitoringEvent {
    std::string eventId;
    std::string eventType;
    std::string description;
    AlertSeverity severity;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;
};

struct SystemMetrics {
    // مقاييس الأداء
    double cpuUsage;
    size_t memoryUsage;
    size_t diskUsage;
    std::chrono::milliseconds processTime;

    // مقاييس التقدم
    int currentStep;
    int totalSteps;
    double progressPercentage;

    // مقاييس الجودة
    int errorCount;
    int warningCount;
    int successCount;
};

struct MonitoringConfig {
    MonitoringLevel level;
    std::chrono::seconds updateInterval;
    bool enableAlerts;
    bool enableLogging;
    bool enableMetrics;
    std::string logFile;
    std::vector<std::string> alertRecipients;
    std::map<std::string, double> thresholds;
};

// مراقب عملية الإقلاع الذاتي
class SelfHostingMonitor {
private:
    MonitoringConfig config;
    std::vector<MonitoringEvent> events;
    SystemMetrics currentMetrics;
    bool monitoringActive;
    std::chrono::system_clock::time_point startTime;
    std::map<std::string, std::function<void(const MonitoringEvent&)>> alertHandlers;

public:
    SelfHostingMonitor();
    ~SelfHostingMonitor() = default;

    // الواجهة العامة
    void initialize(const MonitoringConfig& config);
    void startMonitoring();
    void stopMonitoring();
    void updateMetrics(const SystemMetrics& metrics);
    void logEvent(const std::string& eventType, const std::string& description,
                 AlertSeverity severity = AlertSeverity::INFO,
                 const std::map<std::string, std::string>& metadata = {});

    // دوال التقارير
    SystemMetrics getCurrentMetrics() const;
    std::vector<MonitoringEvent> getEvents(std::chrono::seconds lastN = std::chrono::seconds(0)) const;
    std::map<std::string, int> getEventStatistics() const;
    std::string generateReport() const;

    // دوال التنبيهات
    void registerAlertHandler(const std::string& alertType,
                            std::function<void(const MonitoringEvent&)> handler);
    void triggerAlert(const MonitoringEvent& event);

    // دوال التشخيص
    bool isMonitoringActive() const { return monitoringActive; }
    std::chrono::seconds getUptime() const;
    std::vector<std::string> getActiveAlerts() const;

private:
    void setupDefaultAlertHandlers();
    void checkThresholds(const SystemMetrics& metrics);
    void writeToLogFile(const MonitoringEvent& event);
    std::string formatEventForLogging(const MonitoringEvent& event) const;
    AlertSeverity evaluateEventSeverity(const std::string& eventType) const;
    void cleanupOldEvents();
};

// Factory function لإنشاء مراقب الإقلاع الذاتي
std::unique_ptr<SelfHostingMonitor> createSelfHostingMonitor();

} // namespace ArabicLanguage

#endif // SELF_HOSTING_MONITOR_H