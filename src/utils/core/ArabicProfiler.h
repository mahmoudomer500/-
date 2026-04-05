#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <filesystem>
#include "ArabicTypes.h"
#include "ArabicMemoryManager.h"

namespace fs = std::filesystem;

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // 📊 أدوات تحليل الأداء المتقدمة
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief نظام تحليل أداء شامل للمترجم العربي
     */
    class ArabicProfiler {
    public:
        // Singleton pattern
        static ArabicProfiler& getInstance();

        // منع النسخ والتعيين
        ArabicProfiler(const ArabicProfiler&) = delete;
        ArabicProfiler& operator=(const ArabicProfiler&) = delete;

        // ══════════════════════════════════════════════════════════════
        // 📈 مقياس الأداء
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مقياس أداء تلقائي مع تتبع زمني
         */
        class PerformanceTimer {
        private:
            std::string operationName;
            std::chrono::steady_clock::time_point startTime;
            std::function<void(const std::string&, std::chrono::milliseconds)> callback;
            bool active = true;

        public:
            explicit PerformanceTimer(const std::string& name,
                                    std::function<void(const std::string&, std::chrono::milliseconds)> cb = nullptr);
            ~PerformanceTimer();

            void stop();
            std::chrono::milliseconds getElapsed() const;
            const std::string& getOperationName() const { return operationName; }
        };

        /**
         * @brief نقطة قياس أداء
         */
        struct PerformancePoint {
            std::string name;
            std::chrono::steady_clock::time_point timestamp;
            size_t memoryUsage = 0;
            size_t cpuUsage = 0;
            std::string context;

            PerformancePoint(const std::string& n, const std::string& ctx = "")
                : name(n), timestamp(std::chrono::steady_clock::now()), context(ctx) {}
        };

        // ══════════════════════════════════════════════════════════════
        // 📊 جامع البيانات
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief جامع بيانات الأداء
         */
        class DataCollector {
        private:
            std::vector<PerformancePoint> performancePoints;
            std::unordered_map<std::string, std::vector<std::chrono::milliseconds>> operationTimes;
            mutable std::mutex dataMutex;

        public:
            void recordPerformancePoint(const PerformancePoint& point);
            void recordOperationTime(const std::string& operation, std::chrono::milliseconds time);

            const std::vector<PerformancePoint>& getPerformancePoints() const;
            std::vector<std::chrono::milliseconds> getOperationTimes(const std::string& operation) const;
            std::vector<std::string> getAllOperations() const;

            void clear();
            size_t getTotalPoints() const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🔍 محلل الأداء
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief محلل بيانات الأداء مع تحليلات متقدمة
         */
        class PerformanceAnalyzer {
        private:
            DataCollector& collector;

        public:
            explicit PerformanceAnalyzer(DataCollector& coll) : collector(coll) {}

            /**
             * @brief تحليل الأداء العام
             */
            struct PerformanceAnalysis {
                double averageParseTime = 0.0;
                double averageExecutionTime = 0.0;
                double memoryEfficiency = 0.0;
                std::vector<std::string> bottlenecks;
                std::vector<std::string> recommendations;
                double overallScore = 0.0; // 0.0 - 100.0
            };

            PerformanceAnalysis analyze() const;

            /**
             * @brief العثور على الاختناقات
             */
            std::vector<std::string> findBottlenecks() const;

            /**
             * @brief توليد توصيات للتحسين
             */
            std::vector<std::string> generateRecommendations() const;

            /**
             * @brief مقارنة مع معايير الأداء
             */
            std::vector<std::string> compareWithBenchmarks() const;

            /**
             * @brief حساب الدرجة الكلية للأداء
             */
            double calculateOverallScore(const PerformanceAnalysis& analysis) const;
        };

        // ══════════════════════════════════════════════════════════════
        // 📈 مراقب الموارد
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مراقب استخدام الموارد (CPU، ذاكرة، I/O)
         */
        class ResourceMonitor {
        private:
            std::thread monitorThread;
            std::atomic<bool> monitoring{false};
            std::chrono::milliseconds samplingInterval{100};
            std::function<void(size_t, size_t, size_t)> callback; // memory, cpu, io

            // إحصائيات الاستخدام
            std::atomic<size_t> peakMemoryUsage{0};
            std::atomic<size_t> averageCpuUsage{0};
            std::atomic<size_t> totalIoOperations{0};

        public:
            ResourceMonitor();
            ~ResourceMonitor();

            void startMonitoring(std::function<void(size_t, size_t, size_t)> cb = nullptr);
            void stopMonitoring();

            size_t getCurrentMemoryUsage() const;
            size_t getPeakMemoryUsage() const;
            size_t getAverageCpuUsage() const;
            size_t getTotalIoOperations() const;

            void resetStats();

        private:
            void monitorLoop();
            size_t getMemoryUsage() const;
            size_t getCpuUsage() const;
            size_t getIoOperations() const;
        };

        // ══════════════════════════════════════════════════════════════
        // 📋 منشئ التقارير
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief منشئ تقارير الأداء المفصلة
         */
        class ReportGenerator {
        private:
            DataCollector& collector;
            PerformanceAnalyzer& analyzer;
            ResourceMonitor& monitor;

        public:
            ReportGenerator(DataCollector& coll, PerformanceAnalyzer& anal, ResourceMonitor& mon)
                : collector(coll), analyzer(anal), monitor(mon) {}

            /**
             * @brief تقرير شامل
             */
            struct PerformanceReport {
                std::string summary;
                std::vector<std::string> detailedAnalysis;
                std::vector<std::string> recommendations;
                std::vector<std::string> charts; // ASCII charts
                double performanceScore;
                std::chrono::system_clock::time_point generatedAt;
            };

            PerformanceReport generateFullReport() const;

            /**
             * @brief تقرير سريع
             */
            std::string generateQuickReport() const;

            /**
             * @brief تصدير التقرير
             */
            bool exportReport(const PerformanceReport& report, const fs::path& outputPath) const;

            /**
             * @brief تقرير HTML تفاعلي
             */
            std::string generateHtmlReport() const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🎯 محسن الأداء التلقائي
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief محسن أداء تلقائي مع اقتراحات للتحسين
         */
        class PerformanceOptimizer {
        private:
            PerformanceAnalyzer& analyzer;
            std::vector<std::function<void()>> optimizationStrategies;

        public:
            explicit PerformanceOptimizer(PerformanceAnalyzer& anal) : analyzer(anal) {}

            /**
             * @brief تحسين تلقائي
             */
            struct OptimizationResult {
                bool success = false;
                std::vector<std::string> appliedOptimizations;
                std::vector<std::string> failedOptimizations;
                double performanceImprovement = 0.0; // نسبة مئوية
            };

            OptimizationResult optimize();

            /**
             * @brief تطبيق استراتيجية تحسين محددة
             */
            bool applyOptimization(const std::string& strategyName);

            /**
             * @brief قائمة الاستراتيجيات المتاحة
             */
            std::vector<std::string> getAvailableStrategies() const;

            /**
             * @brief تقييم تأثير التحسين
             */
            double evaluateOptimizationImpact(const std::string& strategyName) const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🎮 واجهة التحكم التفاعلية
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief واجهة تحكم تفاعلية للتحليل
         */
        class InteractiveProfiler {
        private:
            bool active = false;
            std::thread interfaceThread;

        public:
            void startInteractiveMode();
            void stopInteractiveMode();

            /**
             * @brief أوامر تفاعلية
             */
            void showMenu() const;
            void handleCommand(const std::string& command);
            void displayRealTimeStats() const;

        private:
            void interfaceLoop();
            void processUserInput();
        };

        // ══════════════════════════════════════════════════════════════
        // 🎯 واجهة الاستخدام العامة
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief بدء جلسة تحليل
         */
        void startProfilingSession(const std::string& sessionName = "");

        /**
         * @brief إنهاء جلسة التحليل
         */
        void endProfilingSession();

        /**
         * @brief إنشاء نقطة قياس
         */
        void createProfilePoint(const std::string& name, const std::string& context = "");

        /**
         * @brief قياس عملية محددة
         */
        std::unique_ptr<PerformanceTimer> measureOperation(const std::string& operationName);

        /**
         * @brief الحصول على تحليل الأداء
         */
        PerformanceAnalyzer::PerformanceAnalysis getPerformanceAnalysis();

        /**
         * @brief توليد تقرير الأداء
         */
        ReportGenerator::PerformanceReport generatePerformanceReport();

        // إدارة المراقبة
        ResourceMonitor& getResourceMonitor() { return resourceMonitor; }
        DataCollector& getDataCollector() { return dataCollector; }
        PerformanceAnalyzer& getAnalyzer() { return analyzer; }
        ReportGenerator& getReportGenerator() { return reportGenerator; }
        PerformanceOptimizer& getOptimizer() { return optimizer; }

        // إعدادات
        void setSamplingInterval(std::chrono::milliseconds interval);
        void enableDetailedLogging(bool enable);
        void setOutputDirectory(const fs::path& dir);

        // مراقبة
        bool isProfilingActive() const { return profilingActive; }
        std::string getCurrentSessionName() const { return currentSession; }

    private:
        DataCollector dataCollector;
        ResourceMonitor resourceMonitor;
        PerformanceAnalyzer analyzer{dataCollector};
        ReportGenerator reportGenerator{dataCollector, analyzer, resourceMonitor};
        PerformanceOptimizer optimizer{analyzer};
        InteractiveProfiler interactiveProfiler;

        std::atomic<bool> profilingActive{false};
        std::string currentSession;
        fs::path outputDirectory{"./profiler_output"};
        std::chrono::milliseconds samplingInterval{100};
        bool detailedLogging = false;

        ArabicProfiler();
        ~ArabicProfiler();

        // دوال مساعدة
        void initializeStrategies();
        void cleanupSession();
        std::string generateSessionId() const;
    };

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة للتحليل
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief قياس سريع لعملية
     */
    std::chrono::milliseconds quickProfile(const std::function<void()>& operation);

    /**
     * @brief مقارنة أداء عمليتين
     */
    std::vector<std::string> comparePerformance(const std::function<void()>& op1,
                                              const std::function<void()>& op2,
                                              const std::string& name1 = "Operation1",
                                              const std::string& name2 = "Operation2");

    /**
     * @brief تحليل استخدام الذاكرة لعملية
     */
    std::vector<std::string> analyzeMemoryUsage(const std::function<void()>& operation);

    /**
     * @brief إنشاء رسم بياني ASCII للأداء
     */
    std::string createPerformanceChart(const std::vector<double>& data,
                                     const std::string& title = "Performance Chart");

    /**
     * @brief تصدير بيانات التحليل إلى CSV
     */
    bool exportProfilingDataToCsv(const std::vector<ArabicProfiler::PerformancePoint>& points,
                                const fs::path& outputPath);

    /**
     * @brief تحميل بيانات التحليل من CSV
     */
    std::vector<ArabicProfiler::PerformancePoint> importProfilingDataFromCsv(const fs::path& inputPath);

} // namespace ArabicLanguage
