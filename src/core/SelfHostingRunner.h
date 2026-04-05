// SelfHostingRunner.h - مشغل عمليات الإقلاع الذاتي المتكررة
#ifndef SELF_HOSTING_RUNNER_H
#define SELF_HOSTING_RUNNER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <functional>

namespace ArabicLanguage {

struct SelfHostingRunResult {
    int runId;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    bool success;
    std::chrono::milliseconds duration;
    size_t memoryPeak;
    size_t outputSize;
    int errorCount;
    int warningCount;
    std::string sourceCode;
    std::string generatedOutput;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::map<std::string, std::string> metrics;
};

struct SelfHostingRunConfig {
    int maxRuns;
    std::chrono::milliseconds maxDurationPerRun;
    size_t maxMemoryPerRun;
    bool enableDetailedLogging;
    bool saveIntermediateResults;
    std::string outputDirectory;
    std::vector<std::string> testCases;
    bool enableParallelExecution;
    int maxParallelRuns;
};

enum class ConsistencyLevel {
    EXCELLENT,    // نسبة تطابق > 95%
    GOOD,         // نسبة تطابق 80-95%
    FAIR,         // نسبة تطابق 60-80%
    POOR,         // نسبة تطابق 40-60%
    INCONSISTENT  // نسبة تطابق < 40%
};

// مشغل عمليات الإقلاع الذاتي المتكررة
class SelfHostingRunner {
private:
    SelfHostingRunConfig config;
    std::vector<SelfHostingRunResult> runResults;
    std::map<std::string, std::vector<SelfHostingRunResult>> testCaseResults;
    bool running;

    // إحصائيات التكرار
    struct RunStatistics {
        int totalRuns;
        int successfulRuns;
        int failedRuns;
        double averageDuration;
        double averageMemoryUsage;
        ConsistencyLevel overallConsistency;
        std::map<std::string, double> consistencyByTestCase;
        std::vector<std::string> commonErrors;
        std::vector<std::string> performanceInsights;
    } statistics;

public:
    SelfHostingRunner();
    ~SelfHostingRunner() = default;

    // الواجهة العامة
    void configure(const SelfHostingRunConfig& cfg);
    bool runMultipleTimes(int count = 10);
    bool runTestCase(const std::string& testCase, int count = 5);
    std::vector<SelfHostingRunResult> getResults() const;
    RunStatistics getStatistics() const;

    // دوال التحليل والتقارير
    ConsistencyLevel analyzeConsistency() const;
    std::map<std::string, double> analyzePerformanceTrends() const;
    std::vector<std::string> identifyCommonIssues() const;
    bool generateDetailedReport(const std::string& filename);

    // دوال المراقبة والتحكم
    bool isRunning() const { return running; }
    void stopRuns();
    int getCompletedRuns() const;
    double getProgressPercentage() const;

private:
    SelfHostingRunResult executeSingleRun(int runId, const std::string& testCase = "");
    bool validateRunResult(const SelfHostingRunResult& result);
    double calculateOutputSimilarity(const std::string& output1, const std::string& output2) const;
    ConsistencyLevel evaluateConsistencyLevel(double similarityPercentage) const;
    void updateStatistics();
    void logRunResult(const SelfHostingRunResult& result);
    std::string generateTestCaseName(int runId, const std::string& baseName = "");
};

// Factory function لإنشاء مشغل الإقلاع الذاتي
std::unique_ptr<SelfHostingRunner> createSelfHostingRunner();

} // namespace ArabicLanguage

#endif // SELF_HOSTING_RUNNER_H