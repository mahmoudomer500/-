// SelfHostingMonitor.cpp - تطبيق مراقب عملية الإقلاع الذاتي
#include "SelfHostingMonitor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace ArabicLanguage {

SelfHostingMonitor::SelfHostingMonitor() : monitoringActive(false) {
    std::cout << "[INFO] Self-Hosting Monitor initialized" << std::endl;
    setupDefaultAlertHandlers();
}

void SelfHostingMonitor::initialize(const MonitoringConfig& cfg) {
    config = cfg;
    currentMetrics = SystemMetrics{0.0, 0, 0, std::chrono::milliseconds(0), 0, 0, 0.0, 0, 0, 0};

    if (config.enableLogging && !config.logFile.empty()) {
        // إنشاء ملف السجل إذا لم يكن موجوداً
        std::ofstream logFile(config.logFile, std::ios::app);
        if (logFile.is_open()) {
            logFile << "\n=== Self-Hosting Monitor Started ===\n";
            logFile << "Timestamp: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << "\n";
            logFile << "Monitoring Level: " << static_cast<int>(config.level) << "\n\n";
            logFile.close();
        }
    }

    std::cout << "[INFO] Self-Hosting Monitor configured with level: "
              << static_cast<int>(config.level) << std::endl;
}

void SelfHostingMonitor::startMonitoring() {
    if (monitoringActive) {
        std::cout << "[WARNING] Monitoring already active" << std::endl;
        return;
    }

    monitoringActive = true;
    startTime = std::chrono::system_clock::now();

    logEvent("MONITORING_STARTED", "Self-hosting monitoring started",
             AlertSeverity::INFO, {{"level", std::to_string(static_cast<int>(config.level))}});

    std::cout << "[SUCCESS] Self-Hosting monitoring started" << std::endl;
}

void SelfHostingMonitor::stopMonitoring() {
    if (!monitoringActive) {
        std::cout << "[WARNING] Monitoring not active" << std::endl;
        return;
    }

    auto uptime = getUptime();
    monitoringActive = false;

    logEvent("MONITORING_STOPPED", "Self-hosting monitoring stopped",
             AlertSeverity::INFO, {{"uptime_seconds", std::to_string(uptime.count())}});

    std::cout << "[SUCCESS] Self-Hosting monitoring stopped (uptime: "
              << uptime.count() << "s)" << std::endl;
}

void SelfHostingMonitor::updateMetrics(const SystemMetrics& metrics) {
    if (!monitoringActive) return;

    currentMetrics = metrics;

    // فحص الحدود والتنبيهات
    checkThresholds(metrics);

    // تسجيل المقاييس حسب مستوى المراقبة
    if (config.level >= MonitoringLevel::DETAILED) {
        std::map<std::string, std::string> metadata = {
            {"cpu_usage", std::to_string(metrics.cpuUsage)},
            {"memory_usage", std::to_string(metrics.memoryUsage)},
            {"progress", std::to_string(metrics.progressPercentage)},
            {"errors", std::to_string(metrics.errorCount)}
        };

        logEvent("METRICS_UPDATE", "System metrics updated", AlertSeverity::INFO, metadata);
    }
}

void SelfHostingMonitor::logEvent(const std::string& eventType, const std::string& description,
                                AlertSeverity severity, const std::map<std::string, std::string>& metadata) {
    MonitoringEvent event;
    event.eventId = "event_" + std::to_string(events.size() + 1);
    event.eventType = eventType;
    event.description = description;
    event.severity = severity;
    event.timestamp = std::chrono::system_clock::now();
    event.metadata = metadata;

    events.push_back(event);

    // كتابة في ملف السجل
    if (config.enableLogging) {
        writeToLogFile(event);
    }

    // عرض في الكونسول حسب مستوى الخطورة
    if (severity >= AlertSeverity::WARNING) {
        std::string severityStr;
        switch (severity) {
            case AlertSeverity::INFO: severityStr = "[INFO]"; break;
            case AlertSeverity::WARNING: severityStr = "[WARNING]"; break;
            case AlertSeverity::CRITICAL: severityStr = "[CRITICAL]"; break;
            case AlertSeverity::FATAL: severityStr = "[FATAL]"; break;
        }

        std::cout << severityStr << " " << eventType << ": " << description << std::endl;
    }

    // تشغيل معالجات التنبيهات
    if (config.enableAlerts) {
        triggerAlert(event);
    }

    // تنظيف الأحداث القديمة إذا لزم الأمر
    if (events.size() > 1000) { // الحد الأقصى للأحداث
        cleanupOldEvents();
    }
}

SystemMetrics SelfHostingMonitor::getCurrentMetrics() const {
    return currentMetrics;
}

std::vector<MonitoringEvent> SelfHostingMonitor::getEvents(std::chrono::seconds lastN) const {
    if (lastN.count() == 0) {
        return events;
    }

    auto cutoff = std::chrono::system_clock::now() - lastN;
    std::vector<MonitoringEvent> recentEvents;

    for (const auto& event : events) {
        if (event.timestamp >= cutoff) {
            recentEvents.push_back(event);
        }
    }

    return recentEvents;
}

std::map<std::string, int> SelfHostingMonitor::getEventStatistics() const {
    std::map<std::string, int> stats;

    for (const auto& event : events) {
        stats[event.eventType]++;
        stats["total_events"]++;

        switch (event.severity) {
            case AlertSeverity::INFO: stats["info_events"]++; break;
            case AlertSeverity::WARNING: stats["warning_events"]++; break;
            case AlertSeverity::CRITICAL: stats["critical_events"]++; break;
            case AlertSeverity::FATAL: stats["fatal_events"]++; break;
        }
    }

    return stats;
}

std::string SelfHostingMonitor::generateReport() const {
    std::stringstream report;

    report << "Self-Hosting Monitoring Report\n";
    report << "==============================\n\n";

    report << "Monitoring Status: " << (monitoringActive ? "Active" : "Inactive") << "\n";

    if (monitoringActive) {
        auto uptime = getUptime();
        report << "Uptime: " << uptime.count() << " seconds\n";
    }

    report << "Total Events: " << events.size() << "\n\n";

    // إحصائيات الأحداث
    auto eventStats = getEventStatistics();
    report << "Event Statistics:\n";
    for (const auto& stat : eventStats) {
        report << "  " << stat.first << ": " << stat.second << "\n";
    }

    report << "\nCurrent Metrics:\n";
    report << "  CPU Usage: " << currentMetrics.cpuUsage << "%\n";
    report << "  Memory Usage: " << currentMetrics.memoryUsage << " bytes\n";
    report << "  Progress: " << currentMetrics.progressPercentage << "%\n";
    report << "  Errors: " << currentMetrics.errorCount << "\n";
    report << "  Warnings: " << currentMetrics.warningCount << "\n";

    report << "\nRecent Events (last 10):\n";
    auto recentEvents = getEvents(std::chrono::minutes(5)); // آخر 5 دقائق
    int count = 0;
    for (auto it = recentEvents.rbegin(); it != recentEvents.rend() && count < 10; ++it, ++count) {
        auto time = std::chrono::system_clock::to_time_t(it->timestamp);
        report << "  [" << std::put_time(std::localtime(&time), "%H:%M:%S") << "] "
               << it->eventType << ": " << it->description << "\n";
    }

    return report.str();
}

void SelfHostingMonitor::registerAlertHandler(const std::string& alertType,
                                            std::function<void(const MonitoringEvent&)> handler) {
    alertHandlers[alertType] = handler;
}

void SelfHostingMonitor::triggerAlert(const MonitoringEvent& event) {
    // تشغيل معالج التنبيه العام إذا كان موجوداً
    auto generalHandler = alertHandlers.find("GENERAL");
    if (generalHandler != alertHandlers.end()) {
        generalHandler->second(event);
    }

    // تشغيل معالج نوع التنبيه المحدد
    auto specificHandler = alertHandlers.find(event.eventType);
    if (specificHandler != alertHandlers.end()) {
        specificHandler->second(event);
    }
}

std::chrono::seconds SelfHostingMonitor::getUptime() const {
    if (!monitoringActive) return std::chrono::seconds(0);

    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
}

std::vector<std::string> SelfHostingMonitor::getActiveAlerts() const {
    std::vector<std::string> activeAlerts;

    for (const auto& event : events) {
        if (event.severity >= AlertSeverity::WARNING) {
            auto timeSince = std::chrono::system_clock::now() - event.timestamp;
            if (timeSince < std::chrono::minutes(10)) { // تنبيهات نشطة في آخر 10 دقائق
                activeAlerts.push_back(event.eventType + ": " + event.description);
            }
        }
    }

    return activeAlerts;
}

void SelfHostingMonitor::setupDefaultAlertHandlers() {
    // معالج تنبيهات الأخطاء الحرجة
    registerAlertHandler("GENERAL", [](const MonitoringEvent& event) {
        if (event.severity >= AlertSeverity::CRITICAL) {
            std::cerr << "[ALERT] Critical event: " << event.description << std::endl;

            // يمكن إضافة إجراءات تصحيحية هنا
            // مثل إيقاف العملية أو إرسال إشعارات
        }
    });

    // معالج تنبيهات فشل الترجمة
    registerAlertHandler("COMPILATION_FAILED", [](const MonitoringEvent& event) {
        std::cerr << "[COMPILATION ALERT] " << event.description << std::endl;
        // يمكن إضافة منطق استرداد هنا
    });

    // معالج تنبيهات استهلاك الموارد العالي
    registerAlertHandler("HIGH_RESOURCE_USAGE", [](const MonitoringEvent& event) {
        std::cout << "[RESOURCE ALERT] " << event.description << std::endl;
        // يمكن إضافة تحسينات للموارد هنا
    });
}

void SelfHostingMonitor::checkThresholds(const SystemMetrics& metrics) {
    // فحص حدود الذاكرة
    auto memoryThreshold = config.thresholds.find("max_memory_mb");
    if (memoryThreshold != config.thresholds.end()) {
        double maxMemoryMB = memoryThreshold->second;
        double currentMemoryMB = metrics.memoryUsage / (1024.0 * 1024.0);

        if (currentMemoryMB > maxMemoryMB) {
            logEvent("HIGH_MEMORY_USAGE",
                    "Memory usage exceeded threshold: " + std::to_string(currentMemoryMB) + " MB",
                    AlertSeverity::WARNING,
                    {{"current_mb", std::to_string(currentMemoryMB)},
                     {"threshold_mb", std::to_string(maxMemoryMB)}});
        }
    }

    // فحص حدود الأخطاء
    auto errorThreshold = config.thresholds.find("max_errors");
    if (errorThreshold != config.thresholds.end()) {
        if (metrics.errorCount > errorThreshold->second) {
            logEvent("HIGH_ERROR_COUNT",
                    "Error count exceeded threshold: " + std::to_string(metrics.errorCount),
                    AlertSeverity::CRITICAL,
                    {{"error_count", std::to_string(metrics.errorCount)},
                     {"threshold", std::to_string(errorThreshold->second)}});
        }
    }

    // فحص حدود الوقت
    auto timeThreshold = config.thresholds.find("max_time_minutes");
    if (timeThreshold != config.thresholds.end()) {
        double maxTimeMin = timeThreshold->second;
        double currentTimeMin = metrics.processTime.count() / (1000.0 * 60.0);

        if (currentTimeMin > maxTimeMin) {
            logEvent("PROCESS_TIMEOUT",
                    "Process time exceeded threshold: " + std::to_string(currentTimeMin) + " minutes",
                    AlertSeverity::CRITICAL,
                    {{"current_min", std::to_string(currentTimeMin)},
                     {"threshold_min", std::to_string(maxTimeMin)}});
        }
    }
}

void SelfHostingMonitor::writeToLogFile(const MonitoringEvent& event) {
    if (config.logFile.empty()) return;

    std::ofstream logFile(config.logFile, std::ios::app);
    if (logFile.is_open()) {
        logFile << formatEventForLogging(event) << "\n";
        logFile.close();
    }
}

std::string SelfHostingMonitor::formatEventForLogging(const MonitoringEvent& event) const {
    auto time = std::chrono::system_clock::to_time_t(event.timestamp);

    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] ";

    switch (event.severity) {
        case AlertSeverity::INFO: ss << "[INFO] "; break;
        case AlertSeverity::WARNING: ss << "[WARNING] "; break;
        case AlertSeverity::CRITICAL: ss << "[CRITICAL] "; break;
        case AlertSeverity::FATAL: ss << "[FATAL] "; break;
    }

    ss << event.eventType << ": " << event.description;

    if (!event.metadata.empty()) {
        ss << " {";
        bool first = true;
        for (const auto& meta : event.metadata) {
            if (!first) ss << ", ";
            ss << meta.first << "=" << meta.second;
            first = false;
        }
        ss << "}";
    }

    return ss.str();
}

AlertSeverity SelfHostingMonitor::evaluateEventSeverity(const std::string& eventType) const {
    // تقييم خطورة الحدث بناءً على نوعه
    if (eventType.find("ERROR") != std::string::npos ||
        eventType.find("FAIL") != std::string::npos) {
        return AlertSeverity::CRITICAL;
    } else if (eventType.find("WARNING") != std::string::npos ||
               eventType.find("HIGH") != std::string::npos) {
        return AlertSeverity::WARNING;
    }

    return AlertSeverity::INFO;
}

void SelfHostingMonitor::cleanupOldEvents() {
    // الاحتفاظ بآخر 500 حدث فقط
    if (events.size() > 500) {
        events.erase(events.begin(), events.end() - 500);
    }
}

// Factory function
std::unique_ptr<SelfHostingMonitor> createSelfHostingMonitor() {
    return std::make_unique<SelfHostingMonitor>();
}

} // namespace ArabicLanguage