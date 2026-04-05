// AutomatedTestFramework.cpp - تطبيق إطار الاختبار الآلي الشامل
// الأسبوع الثاني من الشهر الرابع: نظام الاختبار الآلي
#include "AutomatedTestFramework.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <future>
#include <thread>
#include <algorithm>
#include <iomanip>

namespace ArabicLanguage {

// ==================== Assert Implementation ====================

static std::string g_currentTestId;
static std::vector<std::string> g_currentAssertions;
static bool g_assertionFailed = false;
static std::string g_assertionMessage;

void Assert::fail(const std::string& message) {
    g_assertionFailed = true;
    g_assertionMessage = message;
    throw std::runtime_error(message);
}

std::string Assert::getCurrentTestId() {
    return g_currentTestId;
}

void Assert::True(bool condition, const std::string& message) {
    if (!condition) {
        std::string errorMsg = message.empty() ? "Assertion failed: Expected true" : message;
        fail(errorMsg);
    }
    g_currentAssertions.push_back("Assert::True passed");
}

void Assert::False(bool condition, const std::string& message) {
    if (condition) {
        std::string errorMsg = message.empty() ? "Assertion failed: Expected false" : message;
        fail(errorMsg);
    }
    g_currentAssertions.push_back("Assert::False passed");
}

void Assert::Equal(const std::string& expected, const std::string& actual, const std::string& message) {
    if (expected != actual) {
        std::stringstream ss;
        ss << (message.empty() ? "Assertion failed" : message) 
           << ": Expected '" << expected << "', but got '" << actual << "'";
        fail(ss.str());
    }
    g_currentAssertions.push_back("Assert::Equal(string) passed");
}

void Assert::NotEqual(const std::string& expected, const std::string& actual, const std::string& message) {
    if (expected == actual) {
        std::stringstream ss;
        ss << (message.empty() ? "Assertion failed" : message) 
           << ": Expected different values, but both are '" << expected << "'";
        fail(ss.str());
    }
    g_currentAssertions.push_back("Assert::NotEqual(string) passed");
}

void Assert::Equal(int expected, int actual, const std::string& message) {
    if (expected != actual) {
        std::stringstream ss;
        ss << (message.empty() ? "Assertion failed" : message) 
           << ": Expected " << expected << ", but got " << actual;
        fail(ss.str());
    }
    g_currentAssertions.push_back("Assert::Equal(int) passed");
}

void Assert::NotEqual(int expected, int actual, const std::string& message) {
    if (expected == actual) {
        std::stringstream ss;
        ss << (message.empty() ? "Assertion failed" : message) 
           << ": Expected different values, but both are " << expected;
        fail(ss.str());
    }
    g_currentAssertions.push_back("Assert::NotEqual(int) passed");
}

void Assert::Equal(double expected, double actual, double epsilon, const std::string& message) {
    if (std::abs(expected - actual) > epsilon) {
        std::stringstream ss;
        ss << (message.empty() ? "Assertion failed" : message) 
           << ": Expected " << expected << ", but got " << actual 
           << " (epsilon: " << epsilon << ")";
        fail(ss.str());
    }
    g_currentAssertions.push_back("Assert::Equal(double) passed");
}

void Assert::NotNull(void* ptr, const std::string& message) {
    if (ptr == nullptr) {
        std::string errorMsg = message.empty() ? "Assertion failed: Expected non-null pointer" : message;
        fail(errorMsg);
    }
    g_currentAssertions.push_back("Assert::NotNull passed");
}

void Assert::Null(void* ptr, const std::string& message) {
    if (ptr != nullptr) {
        std::string errorMsg = message.empty() ? "Assertion failed: Expected null pointer" : message;
        fail(errorMsg);
    }
    g_currentAssertions.push_back("Assert::Null passed");
}

void Assert::Throws(std::function<void()> func, const std::string& message) {
    bool threw = false;
    try {
        func();
    } catch (...) {
        threw = true;
    }
    if (!threw) {
        std::string errorMsg = message.empty() ? "Assertion failed: Expected exception to be thrown" : message;
        fail(errorMsg);
    }
    g_currentAssertions.push_back("Assert::Throws passed");
}

void Assert::NotThrows(std::function<void()> func, const std::string& message) {
    try {
        func();
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << (message.empty() ? "Assertion failed: Unexpected exception" : message) 
           << ": " << e.what();
        fail(ss.str());
    } catch (...) {
        std::string errorMsg = message.empty() ? "Assertion failed: Unexpected exception" : message;
        fail(errorMsg);
    }
    g_currentAssertions.push_back("Assert::NotThrows passed");
}

// ==================== AutomatedTestFramework Implementation ====================

AutomatedTestFramework::AutomatedTestFramework() 
    : testingInProgress(false), defaultTimeout(std::chrono::seconds(30)) {
    std::cout << "[INFO] Automated Test Framework initialized" << std::endl;
}

void AutomatedTestFramework::initialize() {
    std::cout << "[INFO] Initializing automated test framework..." << std::endl;
    testSuites.clear();
    currentReport = TestReport();
    testingInProgress = false;
}

void AutomatedTestFramework::registerTestSuite(const TestSuite& suite) {
    testSuites.push_back(suite);
    std::cout << "[INFO] Registered test suite: " << suite.suiteName << " (" << suite.tests.size() << " tests)" << std::endl;
}

void AutomatedTestFramework::registerTest(const std::string& suiteName, const TestInfo& test) {
    TestSuite* suite = findSuite(suiteName);
    if (!suite) {
        // إنشاء مجموعة جديدة
        TestSuite newSuite;
        newSuite.suiteId = suiteName;
        newSuite.suiteName = suiteName;
        newSuite.tests.push_back(test);
        testSuites.push_back(newSuite);
    } else {
        suite->tests.push_back(test);
    }
}

TestReport AutomatedTestFramework::runAllTests() {
    if (testingInProgress) {
        throw std::runtime_error("Testing is already in progress");
    }

    testingInProgress = true;
    currentReport = TestReport();
    currentReport.reportId = "test_report_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    currentReport.timestamp = std::chrono::system_clock::now();

    std::cout << "\n=== بدء تشغيل جميع الاختبارات ===" << std::endl;
    std::cout << "عدد مجموعات الاختبارات: " << testSuites.size() << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        for (auto& suite : testSuites) {
            std::cout << "\n[SUITE] تشغيل مجموعة: " << suite.suiteName << std::endl;
            
            suite.passedCount = 0;
            suite.failedCount = 0;
            suite.skippedCount = 0;
            suite.errorCount = 0;
            
            auto suiteStart = std::chrono::high_resolution_clock::now();

            for (auto& test : suite.tests) {
                TestExecutionResult result = executeTestWithTimeout(test);
                suite.results[test.testId] = result;

                // تحديث الإحصائيات
                switch (result.status) {
                    case TestStatus::PASSED: suite.passedCount++; break;
                    case TestStatus::FAILED: suite.failedCount++; break;
                    case TestStatus::SKIPPED: suite.skippedCount++; break;
                    case TestStatus::ERROR:
                    case TestStatus::TIMEOUT: suite.errorCount++; break;
                }

                // طباعة النتيجة
                std::string statusStr;
                switch (result.status) {
                    case TestStatus::PASSED: statusStr = "[PASS]"; break;
                    case TestStatus::FAILED: statusStr = "[FAIL]"; break;
                    case TestStatus::SKIPPED: statusStr = "[SKIP]"; break;
                    case TestStatus::ERROR: statusStr = "[ERROR]"; break;
                    case TestStatus::TIMEOUT: statusStr = "[TIMEOUT]"; break;
                }
                std::cout << "  " << statusStr << " " << test.testName 
                         << " (" << result.duration.count() << "ms)" << std::endl;
                
                if (result.status != TestStatus::PASSED && !result.errorMessage.empty()) {
                    std::cout << "      " << result.errorMessage << std::endl;
                }
            }

            auto suiteEnd = std::chrono::high_resolution_clock::now();
            suite.totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(suiteEnd - suiteStart);

            currentReport.suites.push_back(suite);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        currentReport.totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // تحديث الإحصائيات
        updateReportStatistics();
        analyzeFailures();
        currentReport.recommendations = generateRecommendations();

        std::cout << "\n=== انتهاء تشغيل الاختبارات ===" << std::endl;
        std::cout << "الوقت الإجمالي: " << currentReport.totalDuration.count() << "ms" << std::endl;
        std::cout << "إجمالي الاختبارات: " << currentReport.totalTests << std::endl;
        std::cout << "نجحت: " << currentReport.totalPassed << std::endl;
        std::cout << "فشلت: " << currentReport.totalFailed << std::endl;
        std::cout << "معدل النجاح: " << std::fixed << std::setprecision(2) 
                 << (currentReport.successRate * 100) << "%" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Test execution failed: " << e.what() << std::endl;
        currentReport.totalErrors++;
    }

    testingInProgress = false;
    return currentReport;
}

TestReport AutomatedTestFramework::runTestSuite(const std::string& suiteName) {
    TestSuite* suite = findSuite(suiteName);
    if (!suite) {
        throw std::runtime_error("Test suite not found: " + suiteName);
    }

    std::vector<TestInfo> tests = suite->tests;
    return runFilteredTests(tests);
}

TestExecutionResult AutomatedTestFramework::runSingleTest(const std::string& suiteName, const std::string& testName) {
    TestInfo* test = findTest(suiteName, testName);
    if (!test) {
        TestExecutionResult errorResult;
        errorResult.testId = suiteName + "." + testName;
        errorResult.testName = testName;
        errorResult.status = TestStatus::ERROR;
        errorResult.errorMessage = "Test not found";
        return errorResult;
    }

    return executeTestWithTimeout(*test);
}

TestExecutionResult AutomatedTestFramework::runTestById(const std::string& testId) {
    for (auto& suite : testSuites) {
        for (auto& test : suite.tests) {
            if (test.testId == testId) {
                return executeTestWithTimeout(test);
            }
        }
    }

    TestExecutionResult errorResult;
    errorResult.testId = testId;
    errorResult.status = TestStatus::ERROR;
    errorResult.errorMessage = "Test ID not found";
    return errorResult;
}

std::vector<TestInfo> AutomatedTestFramework::filterTestsByType(TestType type) const {
    std::vector<TestInfo> filtered;
    for (const auto& suite : testSuites) {
        for (const auto& test : suite.tests) {
            if (test.type == type) {
                filtered.push_back(test);
            }
        }
    }
    return filtered;
}

std::vector<TestInfo> AutomatedTestFramework::filterTestsByPriority(TestPriority priority) const {
    std::vector<TestInfo> filtered;
    for (const auto& suite : testSuites) {
        for (const auto& test : suite.tests) {
            if (test.priority == priority) {
                filtered.push_back(test);
            }
        }
    }
    return filtered;
}

std::vector<TestInfo> AutomatedTestFramework::filterTestsByTag(const std::string& tag) const {
    std::vector<TestInfo> filtered;
    for (const auto& suite : testSuites) {
        for (const auto& test : suite.tests) {
            if (std::find(test.tags.begin(), test.tags.end(), tag) != test.tags.end()) {
                filtered.push_back(test);
            }
        }
    }
    return filtered;
}

TestReport AutomatedTestFramework::runFilteredTests(const std::vector<TestInfo>& tests) {
    TestReport report;
    report.reportId = "filtered_test_report_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    report.timestamp = std::chrono::system_clock::now();

    auto startTime = std::chrono::high_resolution_clock::now();

    std::map<std::string, TestSuite> suiteMap;

    for (const auto& test : tests) {
        if (suiteMap.find(test.suiteName) == suiteMap.end()) {
            TestSuite suite;
            suite.suiteId = test.suiteName;
            suite.suiteName = test.suiteName;
            suiteMap[test.suiteName] = suite;
        }

        TestExecutionResult result = executeTestWithTimeout(test);
        suiteMap[test.suiteName].results[test.testId] = result;
        suiteMap[test.suiteName].tests.push_back(test);

        switch (result.status) {
            case TestStatus::PASSED: suiteMap[test.suiteName].passedCount++; break;
            case TestStatus::FAILED: suiteMap[test.suiteName].failedCount++; break;
            case TestStatus::SKIPPED: suiteMap[test.suiteName].skippedCount++; break;
            case TestStatus::ERROR:
            case TestStatus::TIMEOUT: suiteMap[test.suiteName].errorCount++; break;
        }
    }

    for (auto& pair : suiteMap) {
        report.suites.push_back(pair.second);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    report.totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // تحديث الإحصائيات
    report.totalTests = tests.size();
    for (const auto& suite : report.suites) {
        report.totalPassed += suite.passedCount;
        report.totalFailed += suite.failedCount;
        report.totalSkipped += suite.skippedCount;
        report.totalErrors += suite.errorCount;
    }
    report.successRate = report.totalTests > 0 ? 
        static_cast<double>(report.totalPassed) / report.totalTests : 0.0;

    return report;
}

void AutomatedTestFramework::setDefaultTimeout(std::chrono::milliseconds timeout) {
    defaultTimeout = timeout;
}

void AutomatedTestFramework::setCurrentTestId(const std::string& testId) {
    currentTestId = testId;
    g_currentTestId = testId;
}

void AutomatedTestFramework::addAssertion(const std::string& assertion) {
    currentAssertions.push_back(assertion);
    g_currentAssertions.push_back(assertion);
}

TestExecutionResult AutomatedTestFramework::executeTest(const TestInfo& test) {
    TestExecutionResult result;
    result.testId = test.testId;
    result.testName = test.testName;
    result.hasAssertions = false;

    setCurrentTestId(test.testId);
    g_currentAssertions.clear();
    g_assertionFailed = false;

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // تنفيذ setup
        if (test.setupFunction) {
            test.setupFunction();
        }

        // تنفيذ الاختبار
        test.testFunction();

        // تنفيذ teardown
        if (test.teardownFunction) {
            test.teardownFunction();
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        if (g_assertionFailed) {
            result.status = TestStatus::FAILED;
            result.errorMessage = g_assertionMessage;
        } else {
            result.status = TestStatus::PASSED;
        }

        result.assertions = g_currentAssertions;
        result.hasAssertions = !g_currentAssertions.empty();

    } catch (const std::exception& e) {
        auto endTime = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        result.status = TestStatus::FAILED;
        result.errorMessage = e.what();
        result.assertions = g_currentAssertions;
    } catch (...) {
        auto endTime = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        result.status = TestStatus::ERROR;
        result.errorMessage = "Unknown exception occurred";
    }

    return result;
}

TestExecutionResult AutomatedTestFramework::executeTestWithTimeout(const TestInfo& test) {
    std::chrono::milliseconds timeout = test.timeout.count() > 0 ? test.timeout : defaultTimeout;

    auto future = std::async(std::launch::async, [this, &test]() {
        return executeTest(test);
    });

    auto status = future.wait_for(timeout);

    if (status == std::future_status::timeout) {
        TestExecutionResult timeoutResult;
        timeoutResult.testId = test.testId;
        timeoutResult.testName = test.testName;
        timeoutResult.status = TestStatus::TIMEOUT;
        timeoutResult.errorMessage = "Test timed out after " + std::to_string(timeout.count()) + "ms";
        timeoutResult.duration = timeout;
        return timeoutResult;
    }

    return future.get();
}

void AutomatedTestFramework::updateReportStatistics() {
    currentReport.totalTests = 0;
    currentReport.totalPassed = 0;
    currentReport.totalFailed = 0;
    currentReport.totalSkipped = 0;
    currentReport.totalErrors = 0;

    currentReport.typeStats.clear();
    currentReport.priorityStats.clear();

    for (const auto& suite : currentReport.suites) {
        for (const auto& test : suite.tests) {
            currentReport.totalTests++;
            currentReport.typeStats[test.type]++;
            currentReport.priorityStats[test.priority]++;

            auto it = suite.results.find(test.testId);
            if (it != suite.results.end()) {
                switch (it->second.status) {
                    case TestStatus::PASSED: currentReport.totalPassed++; break;
                    case TestStatus::FAILED: currentReport.totalFailed++; break;
                    case TestStatus::SKIPPED: currentReport.totalSkipped++; break;
                    case TestStatus::ERROR:
                    case TestStatus::TIMEOUT: currentReport.totalErrors++; break;
                }
            }
        }
    }

    currentReport.successRate = currentReport.totalTests > 0 ?
        static_cast<double>(currentReport.totalPassed) / currentReport.totalTests : 0.0;
}

void AutomatedTestFramework::analyzeFailures() {
    currentReport.criticalFailures.clear();

    for (const auto& suite : currentReport.suites) {
        for (const auto& test : suite.tests) {
            auto it = suite.results.find(test.testId);
            if (it != suite.results.end() && 
                it->second.status != TestStatus::PASSED && 
                test.priority == TestPriority::CRITICAL) {
                currentReport.criticalFailures.push_back(
                    suite.suiteName + "." + test.testName + ": " + it->second.errorMessage
                );
            }
        }
    }
}

std::vector<std::string> AutomatedTestFramework::generateRecommendations() {
    std::vector<std::string> recommendations;

    if (currentReport.successRate < 0.90) {
        recommendations.push_back("تحسين معدل نجاح الاختبارات لتحقيق 90% على الأقل");
    }

    if (!currentReport.criticalFailures.empty()) {
        recommendations.push_back("معالجة جميع الاختبارات الحرجة الفاشلة قبل النشر");
    }

    if (currentReport.totalErrors > 0) {
        recommendations.push_back("مراجعة معالجة الأخطاء في الاختبارات");
    }

    if (currentReport.totalFailed > currentReport.totalPassed) {
        recommendations.push_back("مراجعة شاملة للكود بسبب ارتفاع معدل الفشل");
    }

    if (recommendations.empty()) {
        recommendations.push_back("النظام جاهز للنشر - جميع الاختبارات نجحت");
    }

    return recommendations;
}

TestSuite* AutomatedTestFramework::findSuite(const std::string& suiteName) {
    for (auto& suite : testSuites) {
        if (suite.suiteName == suiteName) {
            return &suite;
        }
    }
    return nullptr;
}

TestInfo* AutomatedTestFramework::findTest(const std::string& suiteName, const std::string& testName) {
    TestSuite* suite = findSuite(suiteName);
    if (!suite) return nullptr;

    for (auto& test : suite->tests) {
        if (test.testName == testName) {
            return &test;
        }
    }
    return nullptr;
}

std::string AutomatedTestFramework::generateTestId(const std::string& suiteName, const std::string& testName) {
    return suiteName + "." + testName;
}

std::vector<TestExecutionResult> AutomatedTestFramework::getFailedTests() const {
    std::vector<TestExecutionResult> failed;
    for (const auto& suite : currentReport.suites) {
        for (const auto& result : suite.results) {
            if (result.second.status != TestStatus::PASSED) {
                failed.push_back(result.second);
            }
        }
    }
    return failed;
}

std::vector<TestExecutionResult> AutomatedTestFramework::getPassedTests() const {
    std::vector<TestExecutionResult> passed;
    for (const auto& suite : currentReport.suites) {
        for (const auto& result : suite.results) {
            if (result.second.status == TestStatus::PASSED) {
                passed.push_back(result.second);
            }
        }
    }
    return passed;
}

double AutomatedTestFramework::getSuccessRate() const {
    return currentReport.successRate;
}

// Factory function
std::unique_ptr<AutomatedTestFramework> createAutomatedTestFramework() {
    return std::make_unique<AutomatedTestFramework>();
}

} // namespace ArabicLanguage

