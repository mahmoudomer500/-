// ComprehensiveTestSuite.h - مجموعة الاختبارات الشاملة النهائية
#ifndef COMPREHENSIVE_TEST_SUITE_H
#define COMPREHENSIVE_TEST_SUITE_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <functional>

namespace ArabicLanguage {

enum class TestCategory {
    FUNCTIONAL,     // اختبارات وظيفية
    PERFORMANCE,    // اختبارات أداء
    COMPATIBILITY,  // اختبارات توافق
    SECURITY,       // اختبارات أمان
    USABILITY,      // اختبارات سهولة الاستخدام
    RELIABILITY     // اختبارات موثوقية
};

enum class TestPriority {
    CRITICAL,   // حرج
    HIGH,       // عالي
    MEDIUM,     // متوسط
    LOW         // منخفض
};

struct TestResult {
    std::string testId;
    std::string testName;
    TestCategory category;
    TestPriority priority;
    bool passed;
    std::chrono::milliseconds duration;
    std::string errorMessage;
    std::map<std::string, std::string> metrics;
    std::vector<std::string> details;
};

struct TestSuite {
    std::string suiteId;
    std::string suiteName;
    std::string description;
    TestCategory category;
    std::vector<std::function<TestResult()>> tests;
    std::map<std::string, TestResult> results;
    std::chrono::milliseconds totalDuration;
    int passedTests;
    int failedTests;
};

struct ComprehensiveTestReport {
    std::string reportId;
    std::chrono::system_clock::time_point timestamp;
    std::vector<TestSuite> testSuites;
    std::map<TestCategory, int> categoryStats;
    std::map<TestPriority, int> priorityStats;
    int totalTests;
    int passedTests;
    int failedTests;
    double overallSuccessRate;
    std::chrono::milliseconds totalDuration;
    std::vector<std::string> criticalFailures;
    std::vector<std::string> recommendations;
    bool systemReady;
};

// مجموعة الاختبارات الشاملة النهائية
class ComprehensiveTestSuite {
private:
    std::vector<TestSuite> testSuites;
    ComprehensiveTestReport finalReport;
    bool testingInProgress;

public:
    ComprehensiveTestSuite();
    ~ComprehensiveTestSuite() = default;

    // الواجهة العامة
    void initialize();
    ComprehensiveTestReport runAllTests();
    TestResult runSingleTest(const std::string& suiteId, const std::string& testId);
    void addTestSuite(const TestSuite& suite);

    // دوال التقارير والتحليل
    void generateDetailedReport(const std::string& filename);
    std::vector<TestResult> getFailedTests() const;
    std::vector<std::string> getTestRecommendations() const;
    bool isSystemReady() const;

    // دوال المراقبة
    bool isTestingActive() const { return testingInProgress; }
    int getCompletedTests() const;
    double getProgressPercentage() const;

private:
    void initializeStandardTestSuites();
    TestResult executeTestWithTimeout(std::function<TestResult()> testFunc,
                                    std::chrono::milliseconds timeout);
    void updateReportStatistics();
    std::vector<std::string> analyzeTestFailures();
    std::vector<std::string> generateSystemRecommendations();
    bool validateTestPrerequisites(const TestSuite& suite);

    // دوال إنشاء مجموعات الاختبارات
    TestSuite createFunctionalTestSuite();
    TestSuite createPerformanceTestSuite();
    TestSuite createCompatibilityTestSuite();
    TestSuite createSecurityTestSuite();
    TestSuite createUsabilityTestSuite();
    TestSuite createReliabilityTestSuite();
};

// Factory function لإنشاء مجموعة الاختبارات الشاملة
std::unique_ptr<ComprehensiveTestSuite> createComprehensiveTestSuite();

} // namespace ArabicLanguage

#endif // COMPREHENSIVE_TEST_SUITE_H