// test_comprehensive_test_suite.cpp - اختبار مجموعة الاختبارات الشاملة النهائية
#include <iostream>
#include <memory>
#include "ComprehensiveTestSuite.h"

using namespace ArabicLanguage;

int main() {
    std::cout << "=== اختبار مجموعة الاختبارات الشاملة النهائية ===\n" << std::endl;

    try {
        // إنشاء مجموعة الاختبارات الشاملة
        auto testSuite = createComprehensiveTestSuite();

        std::cout << "[1/5] تهيئة مجموعة الاختبارات الشاملة..." << std::endl;
        testSuite->initialize();

        std::cout << "\n[2/5] عرض معلومات مجموعات الاختبارات..." << std::endl;
        std::cout << "Number of test suites: " << testSuite->getCompletedTests() << std::endl;
        std::cout << "Progress: " << testSuite->getProgressPercentage() << "%" << std::endl;

        std::cout << "\n[3/5] تشغيل جميع الاختبارات الشاملة..." << std::endl;

        // تشغيل جميع الاختبارات
        ComprehensiveTestReport report = testSuite->runAllTests();

        std::cout << "Comprehensive Test Report:" << std::endl;
        std::cout << "  Report ID: " << report.reportId << std::endl;
        std::cout << "  Total Tests: " << report.totalTests << std::endl;
        std::cout << "  Passed: " << report.passedTests << std::endl;
        std::cout << "  Failed: " << report.failedTests << std::endl;
        std::cout << "  Success Rate: " << (report.overallSuccessRate * 100) << "%" << std::endl;
        std::cout << "  Total Duration: " << report.totalDuration.count() << "ms" << std::endl;
        std::cout << "  System Ready: " << (report.systemReady ? "YES" : "NO") << std::endl;

        std::cout << "\n[4/5] عرض إحصائيات التصنيفات..." << std::endl;

        // عرض إحصائيات التصنيفات
        std::cout << "Category Statistics:" << std::endl;
        for (const auto& stat : report.categoryStats) {
            std::string categoryName;
            switch (stat.first) {
                case TestCategory::FUNCTIONAL: categoryName = "Functional"; break;
                case TestCategory::PERFORMANCE: categoryName = "Performance"; break;
                case TestCategory::COMPATIBILITY: categoryName = "Compatibility"; break;
                case TestCategory::SECURITY: categoryName = "Security"; break;
                case TestCategory::USABILITY: categoryName = "Usability"; break;
                case TestCategory::RELIABILITY: categoryName = "Reliability"; break;
            }
            std::cout << "  " << categoryName << ": " << stat.second << " tests" << std::endl;
        }

        std::cout << "\n[5/5] عرض التقرير النهائي..." << std::endl;

        // عرض التقرير النهائي
        std::vector<std::string> summary = testSuite->getTestSummary();
        std::cout << "Final Test Summary:" << std::endl;
        for (const auto& line : summary) {
            std::cout << line << std::endl;
        }

        // عرض الاختبارات الفاشلة
        auto failedTests = testSuite->getFailedTests();
        if (!failedTests.empty()) {
            std::cout << "\nFailed Tests Details:" << std::endl;
            for (const auto& test : failedTests) {
                std::cout << "  - " << test.testName << " (" << test.category << "): " << test.errorMessage << std::endl;
            }
        }

        // عرض التوصيات
        auto recommendations = testSuite->getTestRecommendations();
        if (!recommendations.empty()) {
            std::cout << "\nTest Recommendations:" << std::endl;
            for (size_t i = 0; i < recommendations.size(); ++i) {
                std::cout << "  " << (i + 1) << ". " << recommendations[i] << std::endl;
            }
        }

        // توليد التقرير التفصيلي
        testSuite->generateDetailedReport("comprehensive_test_report.txt");

        std::cout << "\n=== تم الاختبار الشامل ===\n" << std::endl;

        if (report.systemReady) {
            std::cout << "🎉 جميع الاختبارات نجحت! النظام جاهز للإعلان عن النجاح!" << std::endl;
            std::cout << "الإنجاز التاريخي للترجمة الذاتية مكتمل بنسبة 100%!" << std::endl;
        } else {
            std::cout << "⚠️ بعض الاختبارات تحتاج إلى إصلاح قبل الإعلان عن النجاح." << std::endl;
            std::cout << "يرجى مراجعة التقرير التفصيلي وإصلاح المشاكل المتبقية." << std::endl;
        }

        std::cout << "\nالتقرير التفصيلي محفوظ في: comprehensive_test_report.txt" << std::endl;

        return report.systemReady ? 0 : 1;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] فشل اختبار مجموعة الاختبارات الشاملة: " << e.what() << std::endl;
        return 1;
    }
}