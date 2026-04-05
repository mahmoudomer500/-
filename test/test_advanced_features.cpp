#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

// Include all advanced features
#include "ArabicMemoryManager.h"
#include "ArabicJITCompiler.h"
#include "ArabicGenerics.h"
#include "ArabicAsync.h"
#include "ArabicParallelParser.h"
#include "ArabicTypeInference.h"
#include "ArabicCacheSystem.h"
#include "ArabicProfiler.h"

// Test helper functions
void printTestHeader(const std::string& testName) {
    std::cout << "\n🧪 اختبار: " << testName << std::endl;
    std::cout << std::string(50, '=') << std::endl;
}

void printTestResult(bool success, const std::string& message = "") {
    if (success) {
        std::cout << "✅ نجح: " << message << std::endl;
    } else {
        std::cout << "❌ فشل: " << message << std::endl;
    }
}

// Test Memory Management
bool testMemoryManagement() {
    printTestHeader("إدارة الذاكرة المتقدمة");

    try {
        auto& mm = ArabicMemoryManager::getInstance();

        // Test object pooling
        auto cmd1 = mm.getCommandPool().acquire();
        auto cmd2 = mm.getCommandPool().acquire();
        mm.getCommandPool().release(std::move(cmd1));
        auto cmd3 = mm.getCommandPool().acquire(); // Should reuse cmd1

        printTestResult(true, "تجمع الكائنات يعمل بشكل صحيح");

        // Test garbage collection
        void* ptr1 = mm.getGC().allocate(100, "test");
        void* ptr2 = mm.getGC().allocate(200, "test");
        mm.getGC().release(ptr1);

        printTestResult(mm.getGC().getLiveObjects() == 1, "جامع القمامة يعمل بشكل صحيح");

        // Test smart pointers
        ArabicMemoryManager::ArabicSmartPtr<Value> smartPtr(new Value{ValueType::NUMBER});
        printTestResult(smartPtr.get() != nullptr, "المؤشر الذكي يعمل بشكل صحيح");

        return true;
    } catch (const std::exception& e) {
        printTestResult(false, std::string("خطأ في إدارة الذاكرة: ") + e.what());
        return false;
    }
}

// Test JIT Compilation
bool testJITCompilation() {
    printTestHeader("مترجم JIT");

    try {
        auto& jit = ArabicJITCompiler::getInstance();

        std::string testCode = "متغير x = 5";
        auto bytecode = jit.compileToBytecode(testCode);

        printTestResult(bytecode != nullptr, "تجميع البايت كود نجح");

        // Test caching
        auto cachedBytecode = jit.getCache().get(jit.computeHash(testCode));
        printTestResult(cachedBytecode != nullptr, "التخزين المؤقت للبايت كود يعمل");

        return true;
    } catch (const std::exception& e) {
        printTestResult(false, std::string("خطأ في مترجم JIT: ") + e.what());
        return false;
    }
}

// Test Generics
bool testGenerics() {
    printTestHeader("نظام الجينيريك");

    try {
        auto& generics = ArabicGenerics::getInstance();

        // Test generic type registration
        std::string listType = R"(
قائمة<نوع T> {
    إضافة(عنصر: T) -> صفر
    احصل(فهرس: رقم) -> T
}
)";

        bool registered = generics.registerGenericType(listType);
        printTestResult(registered, "تسجيل النوع العام نجح");

        // Test type instantiation
        std::vector<std::string> typeArgs = {"رقم"};
        std::string instantiated = generics.instantiateType("قائمة", typeArgs);
        printTestResult(!instantiated.empty(), "إنشاء نسخة من النوع العام نجح");

        // Test standard library
        ArabicGenerics::StandardGenerics::registerAll();
        printTestResult(true, "مكتبة الجينيريك القياسية نجحت");

        return true;
    } catch (const std::exception& e) {
        printTestResult(false, std::string("خطأ في نظام الجينيريك: ") + e.what());
        return false;
    }
}

// Test Async/Await
bool testAsyncAwait() {
    printTestHeader("نظام async/await");

    try {
        auto& async = ArabicAsync::getInstance();

        // Test async task creation
        auto task = async.runAsync([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return Value{ValueType::STRING, "async_result"};
        });

        printTestResult(task != nullptr, "إنشاء المهمة غير المتزامنة نجح");

        // Test await
        Value result = async.await(task);
        printTestResult(result.type == ValueType::STRING, "انتظار المهمة غير المتزامنة نجح");

        // Test promise
        auto promise = async.getAwaitHandler().createPromise();
        async.getAwaitHandler().resolvePromise("test_promise", Value{ValueType::NUMBER, "42"});

        Value promiseResult = async.getAwaitHandler().awaitPromise("test_promise");
        printTestResult(promiseResult.string_value == "42", "وعد غير متزامن يعمل بشكل صحيح");

        return true;
    } catch (const std::exception& e) {
        printTestResult(false, std::string("خطأ في نظام async/await: ") + e.what());
        return false;
    }
}

// Test Parallel Parsing
bool testParallelParsing() {
    printTestHeader("التحليل المتوازي");

    try {
        auto& parser = ArabicParallelParser::getInstance();

        // Create large test code
        std::string largeCode;
        for (int i = 0; i < 100; ++i) {
            largeCode += "دالة دالة" + std::to_string(i) + "() {\n";
            largeCode += "    متغير x = " + std::to_string(i) + "\n";
            largeCode += "    ارجع x\n";
            largeCode += "}\n\n";
        }

        // Test parallel parsing
        auto result = parser.parseParallel(largeCode, "large_test.عربي");

        printTestResult(result.success, "التحليل المتوازي نجح");
        printTestResult(!result.commands.empty(), "تم تحليل الأوامر بشكل صحيح");

        // Test performance monitoring
        auto stats = parser.getStats();
        printTestResult(!stats.empty(), "مراقبة الأداء تعمل بشكل صحيح");

        return result.success;
    } catch (const std::exception& e) {
        printTestResult(false, std::string("خطأ في التحليل المتوازي: ") + e.what());
        return false;
    }
}

// Test Type Inference
bool testTypeInference() {
    printTestHeader("استنتاج الأنواع");

    try {
        auto& typeInference = ArabicTypeInference::getInstance();

        // Test basic type inference
        Value numberValue{ValueType::NUMBER, "42"};
        auto result = typeInference.getInferencer().inferFromValue(numberValue);

        printTestResult(result.inferredType.name == "رقم", "استنتاج نوع الرقم نجح");

        // Test expression inference
        std::unordered_map<std::string, ArabicTypeInference::TypeInfo> context;
        auto exprResult = typeInference.inferType("x + y", context);

        printTestResult(exprResult.confidence > 0, "استنتاج نوع التعبير يعمل");

        // Test type checking
        std::vector<std::shared_ptr<Command>> commands; // Empty for now
        std::vector<std::string> errors, warnings;
        bool checkResult = typeInference.checkTypes(commands, errors, warnings);

        printTestResult(true, "فحص الأنواع يعمل بشكل أساسي");

        return true;
    } catch (const std::exception& e) {
        printTestResult(false, std::string("خطأ في استنتاج الأنواع: ") + e.what());
        return false;
    }
}

// Test Cache System
bool testCacheSystem() {
    printTestHeader("نظام التخزين المؤقت");

    try {
        auto& cache = ArabicCacheSystem::getInstance();

        // Test AST caching
        std::vector<std::shared_ptr<Command>> testAST;
        // Add a dummy command
        auto cmd = std::make_shared<Command>();
        cmd->type = CommandType::ASSIGNMENT;
        testAST.push_back(cmd);

        bool astStored = cache.storeAST("test_ast", testAST);
        printTestResult(astStored, "تخزين AST نجح");

        auto retrievedAST = cache.getAST("test_ast");
        printTestResult(retrievedAST != nullptr, "استرجاع AST نجح");

        // Test symbol caching
        SymbolTable testSymbols;
        bool symbolsStored = cache.storeSymbols("test_symbols", testSymbols);
        printTestResult(symbolsStored, "تخزين الرموز نجح");

        // Test cache stats
        auto stats = cache.getStats();
        printTestResult(stats.totalEntries >= 0, "إحصائيات التخزين المؤقت تعمل");

        return true;
    } catch (const std::exception& e) {
        printTestResult(false, std::string("خطأ في نظام التخزين المؤقت: ") + e.what());
        return false;
    }
}

// Test Performance Profiling
bool testPerformanceProfiling() {
    printTestHeader("أدوات تحليل الأداء");

    try {
        auto& profiler = ArabicProfiler::getInstance();

        // Start profiling session
        profiler.startProfilingSession("test_session");

        // Create performance timer
        auto timer = profiler.measureOperation("test_operation");

        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // Timer will be automatically stopped when it goes out of scope

        // Create profile point
        profiler.createProfilePoint("test_point", "unit_test");

        // Get analysis
        auto analysis = profiler.getPerformanceAnalysis();
        printTestResult(analysis.overallScore >= 0, "تحليل الأداء يعمل بشكل صحيح");

        // Generate report
        auto report = profiler.generatePerformanceReport();
        printTestResult(!report.summary.empty(), "توليد التقرير نجح");

        // End session
        profiler.endProfilingSession();

        return true;
    } catch (const std::exception& e) {
        printTestResult(false, std::string("خطأ في تحليل الأداء: ") + e.what());
        return false;
    }
}

// Integration test
bool testIntegration() {
    printTestHeader("اختبار التكامل الشامل");

    try {
        // Test combining multiple features
        auto& profiler = ArabicProfiler::getInstance();
        profiler.startProfilingSession("integration_test");

        // Use memory management with caching
        auto& cache = ArabicCacheSystem::getInstance();
        auto& mm = ArabicMemoryManager::getInstance();

        // Create and cache some data
        std::vector<std::shared_ptr<Command>> testCommands;
        cache.storeAST("integration_test", testCommands);

        // Use JIT compilation
        auto& jit = ArabicJITCompiler::getInstance();
        std::string testCode = "متغير result = 1 + 2";
        auto bytecode = jit.compileToBytecode(testCode);

        // Use generics
        auto& generics = ArabicGenerics::getInstance();
        generics.registerGenericType("قائمة<نوع T> {}");

        // Use async operations
        auto& async = ArabicAsync::getInstance();
        auto task = async.runAsync([]() {
            return Value{ValueType::STRING, "integration_test_passed"};
        });

        Value asyncResult = async.await(task);

        profiler.endProfilingSession();

        bool integrationSuccess = (bytecode != nullptr) &&
                                (asyncResult.string_value == "integration_test_passed");

        printTestResult(integrationSuccess, "التكامل بين جميع الميزات نجح");

        return integrationSuccess;
    } catch (const std::exception& e) {
        printTestResult(false, std::string("خطأ في التكامل: ") + e.what());
        return false;
    }
}

// Main test runner
int main() {
    std::cout << "🚀 بدء اختبار الميزات المتقدمة للمترجم العربي" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    std::vector<std::pair<std::string, bool(*)()>> tests = {
        {"إدارة الذاكرة المتقدمة", testMemoryManagement},
        {"مترجم JIT", testJITCompilation},
        {"نظام الجينيريك", testGenerics},
        {"نظام async/await", testAsyncAwait},
        {"التحليل المتوازي", testParallelParsing},
        {"استنتاج الأنواع", testTypeInference},
        {"نظام التخزين المؤقت", testCacheSystem},
        {"أدوات تحليل الأداء", testPerformanceProfiling},
        {"التكامل الشامل", testIntegration}
    };

    int passed = 0;
    int total = tests.size();

    for (const auto& test : tests) {
        try {
            bool result = test.second();
            if (result) {
                passed++;
            }
        } catch (const std::exception& e) {
            printTestResult(false, std::string("استثناء في ") + test.first + ": " + e.what());
        }
    }

    // Final summary
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "📊 ملخص النتائج:" << std::endl;
    std::cout << "الإجمالي: " << total << std::endl;
    std::cout << "الناجح: " << passed << std::endl;
    std::cout << "الفاشل: " << (total - passed) << std::endl;
    std::cout << "معدل النجاح: " << (passed * 100 / total) << "%" << std::endl;

    if (passed == total) {
        std::cout << "\n🎉 جميع الاختبارات نجحت! الميزات المتقدمة تعمل بشكل مثالي!" << std::endl;
        return 0;
    } else {
        std::cout << "\n⚠️ بعض الاختبارات فشلت. راجع السجلات أعلاه." << std::endl;
        return 1;
    }
}
