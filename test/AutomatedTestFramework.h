// AutomatedTestFramework.h - إطار عمل الاختبار الآلي الشامل
// الأسبوع الثاني من الشهر الرابع: نظام الاختبار الآلي
#ifndef AUTOMATED_TEST_FRAMEWORK_H
#define AUTOMATED_TEST_FRAMEWORK_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <exception>
#include <sstream>

namespace ArabicLanguage {

// أنواع الاختبارات
enum class TestType {
    UNIT,           // اختبارات الوحدة
    INTEGRATION,    // اختبارات التكامل
    PERFORMANCE,    // اختبارات الأداء
    REGRESSION,     // اختبارات الانحدار
    SMOKE           // اختبارات الدخان
};

// أولوية الاختبار
enum class TestPriority {
    CRITICAL,   // حرج
    HIGH,       // عالي
    MEDIUM,     // متوسط
    LOW         // منخفض
};

// نتيجة الاختبار
enum class TestStatus {
    PASSED,     // نجح
    FAILED,     // فشل
    SKIPPED,    // تم تجاهله
    ERROR,      // خطأ
    TIMEOUT     // انتهت المهلة
};

// معلومات الاختبار
struct TestInfo {
    std::string testId;
    std::string testName;
    std::string description;
    std::string suiteName;
    TestType type;
    TestPriority priority;
    std::vector<std::string> tags;
    std::chrono::milliseconds timeout;
    std::function<void()> setupFunction;
    std::function<void()> teardownFunction;
    std::function<void()> testFunction;
};

// نتيجة تنفيذ الاختبار
struct TestExecutionResult {
    std::string testId;
    std::string testName;
    TestStatus status;
    std::chrono::milliseconds duration;
    std::string errorMessage;
    std::string stackTrace;
    std::map<std::string, std::string> metrics;
    std::vector<std::string> assertions;
    bool hasAssertions;
};

// مجموعة الاختبارات
struct TestSuite {
    std::string suiteId;
    std::string suiteName;
    std::string description;
    std::vector<TestInfo> tests;
    std::map<std::string, TestExecutionResult> results;
    std::chrono::milliseconds totalDuration;
    int passedCount;
    int failedCount;
    int skippedCount;
    int errorCount;
};

// تقرير الاختبار الشامل
struct TestReport {
    std::string reportId;
    std::chrono::system_clock::time_point timestamp;
    std::vector<TestSuite> suites;
    std::map<TestType, int> typeStats;
    std::map<TestPriority, int> priorityStats;
    int totalTests;
    int totalPassed;
    int totalFailed;
    int totalSkipped;
    int totalErrors;
    double successRate;
    std::chrono::milliseconds totalDuration;
    std::vector<std::string> criticalFailures;
    std::vector<std::string> recommendations;
};

// مساعدات الادعاءات (Assertions)
class Assert {
public:
    static void True(bool condition, const std::string& message = "");
    static void False(bool condition, const std::string& message = "");
    static void Equal(const std::string& expected, const std::string& actual, const std::string& message = "");
    static void NotEqual(const std::string& expected, const std::string& actual, const std::string& message = "");
    static void Equal(int expected, int actual, const std::string& message = "");
    static void NotEqual(int expected, int actual, const std::string& message = "");
    static void Equal(double expected, double actual, double epsilon = 0.0001, const std::string& message = "");
    static void NotNull(void* ptr, const std::string& message = "");
    static void Null(void* ptr, const std::string& message = "");
    static void Throws(std::function<void()> func, const std::string& message = "");
    static void NotThrows(std::function<void()> func, const std::string& message = "");

private:
    static void fail(const std::string& message);
    static std::string getCurrentTestId();
};

// إطار الاختبار الآلي الشامل
class AutomatedTestFramework {
private:
    std::vector<TestSuite> testSuites;
    TestReport currentReport;
    bool testingInProgress;
    std::string currentTestId;
    std::vector<std::string> currentAssertions;
    std::chrono::milliseconds defaultTimeout;

public:
    AutomatedTestFramework();
    ~AutomatedTestFramework() = default;

    // الواجهة الرئيسية
    void initialize();
    void registerTestSuite(const TestSuite& suite);
    void registerTest(const std::string& suiteName, const TestInfo& test);
    TestReport runAllTests();
    TestReport runTestSuite(const std::string& suiteName);
    TestExecutionResult runSingleTest(const std::string& suiteName, const std::string& testName);
    TestExecutionResult runTestById(const std::string& testId);

    // تصفية الاختبارات
    std::vector<TestInfo> filterTestsByType(TestType type) const;
    std::vector<TestInfo> filterTestsByPriority(TestPriority priority) const;
    std::vector<TestInfo> filterTestsByTag(const std::string& tag) const;
    TestReport runFilteredTests(const std::vector<TestInfo>& tests);

    // إعدادات
    void setDefaultTimeout(std::chrono::milliseconds timeout);
    void setCurrentTestId(const std::string& testId);
    std::string getCurrentTestId() const { return currentTestId; }
    void addAssertion(const std::string& assertion);

    // التقارير
    TestReport getCurrentReport() const { return currentReport; }
    std::vector<TestExecutionResult> getFailedTests() const;
    std::vector<TestExecutionResult> getPassedTests() const;
    double getSuccessRate() const;

private:
    TestExecutionResult executeTest(const TestInfo& test);
    TestExecutionResult executeTestWithTimeout(const TestInfo& test);
    void updateReportStatistics();
    void analyzeFailures();
    std::vector<std::string> generateRecommendations();
    TestSuite* findSuite(const std::string& suiteName);
    TestInfo* findTest(const std::string& suiteName, const std::string& testName);
    std::string generateTestId(const std::string& suiteName, const std::string& testName);
};

// Factory function
std::unique_ptr<AutomatedTestFramework> createAutomatedTestFramework();

// Helper macros للاختبارات
#define TEST_SUITE(name) \
    ArabicLanguage::TestSuite name##_suite; \
    name##_suite.suiteId = #name; \
    name##_suite.suiteName = #name;

#define TEST(suite, name) \
    ArabicLanguage::TestInfo suite##_##name##_test; \
    suite##_##name##_test.testName = #name; \
    suite##_##name##_test.suiteName = #suite; \
    suite##_##name##_test.testFunction = [&]() {

#define END_TEST \
    }; \
    suite##_##name##_test.testId = suite##_##name##_test.suiteName + "." + suite##_##name##_test.testName; \
    suite##_##name##_test.type = ArabicLanguage::TestType::UNIT; \
    suite##_##name##_test.priority = ArabicLanguage::TestPriority::MEDIUM; \
    suite##_##name##_test.timeout = std::chrono::seconds(30);

} // namespace ArabicLanguage

#endif // AUTOMATED_TEST_FRAMEWORK_H

