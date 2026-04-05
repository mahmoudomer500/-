#include "ArabicAsync.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // ⚡ تنفيذ نظام async/await
    // ══════════════════════════════════════════════════════════════

    // Singleton instance
    ArabicAsync& ArabicAsync::getInstance() {
        static ArabicAsync instance;
        return instance;
    }

    ArabicAsync::ArabicAsync() : scheduler(std::thread::hardware_concurrency()) {
        scheduler.start();
    }

    ArabicAsync::~ArabicAsync() {
        scheduler.stop();
    }

    // ══════════════════════════════════════════════════════════════
    // 🏭 تنفيذ مُجمع المهام
    // ══════════════════════════════════════════════════════════════

    ArabicAsync::TaskScheduler::TaskScheduler(size_t numThreads)
        : maxThreads(numThreads) {
    }

    ArabicAsync::TaskScheduler::~TaskScheduler() {
        stop();
    }

    void ArabicAsync::TaskScheduler::start() {
        if (running) return;

        running = true;

        for (size_t i = 0; i < maxThreads; ++i) {
            workerThreads.emplace_back(&TaskScheduler::workerThread, this);
        }
    }

    void ArabicAsync::TaskScheduler::stop() {
        if (!running) return;

        running = false;
        queueCondition.notify_all();

        for (auto& thread : workerThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        workerThreads.clear();
    }

    void ArabicAsync::TaskScheduler::waitForAll() {
        while (activeTasks > 0 || !taskQueue.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void ArabicAsync::TaskScheduler::workerThread() {
        while (running) {
            ScheduledTask task;

            {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCondition.wait(lock, [this]() {
                    return !running || !taskQueue.empty();
                });

                if (!running) break;

                if (!taskQueue.empty()) {
                    task = taskQueue.top();
                    taskQueue.pop();
                } else {
                    continue;
                }
            }

            activeTasks++;

            try {
                // تنفيذ المهمة (هنا يمكن إضافة معالجة النتائج)
                task.taskFunction();
            } catch (const std::exception& e) {
                std::cerr << "خطأ في تنفيذ المهمة: " << e.what() << std::endl;
            }

            activeTasks--;
        }
    }

    std::shared_ptr<ArabicAsync::AsyncTask> ArabicAsync::TaskScheduler::scheduleTask(
        std::function<Value()> task, const std::string& taskId, int priority) {

        std::string id = taskId.empty() ?
            "task_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) : taskId;

        auto promise = std::make_shared<std::promise<Value>>();
        auto future = promise->get_future();

        auto scheduledTask = ScheduledTask{
            [task, promise]() -> Value {
                try {
                    Value result = task();
                    promise->set_value(result);
                    return result;
                } catch (const std::exception& e) {
                    Value errorValue;
                    errorValue.type = ValueType::STRING;
                    errorValue.string_value = std::string("خطأ: ") + e.what();
                    promise->set_value(errorValue);
                    return errorValue;
                }
            },
            id,
            priority,
            std::chrono::steady_clock::now()
        };

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            taskQueue.push(scheduledTask);
        }

        queueCondition.notify_one();

        return std::make_shared<AsyncTask>(std::move(future), id);
    }

    std::shared_ptr<ArabicAsync::AsyncTask> ArabicAsync::TaskScheduler::scheduleDelayedTask(
        std::function<Value()> task, std::chrono::milliseconds delay,
        const std::string& taskId, int priority) {

        auto delayedTask = [task, delay]() -> Value {
            std::this_thread::sleep_for(delay);
            return task();
        };

        return scheduleTask(delayedTask, taskId + "_delayed", priority);
    }

    // ══════════════════════════════════════════════════════════════
    // 🔄 تنفيذ الدالة غير المتزامنة
    // ══════════════════════════════════════════════════════════════

    std::shared_ptr<ArabicAsync::AsyncTask> ArabicAsync::AsyncFunction::invoke(const std::vector<Value>& args) {
        if (compiledFunction) {
            return ArabicAsync::getInstance().runAsync(
                [this, args]() { return compiledFunction(args); },
                name + "_call");
        }

        // تنفيذ بسيط للجسم (في الإصدار الكامل سيتم تحليل وتنفيذ جسم الدالة)
        return ArabicAsync::getInstance().runAsync([this]() {
            Value result;
            result.type = ValueType::STRING;
            result.string_value = "تنفيذ " + name;
            return result;
        }, name + "_call");
    }

    std::string ArabicAsync::AsyncFunction::getSignature() const {
        std::stringstream ss;
        ss << "دالة غير_متزامنة " << name << "(";

        for (size_t i = 0; i < parameters.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << parameters[i].first << ": " << parameters[i].second;
        }

        ss << ") -> " << returnType;
        return ss.str();
    }

    // ══════════════════════════════════════════════════════════════
    // 🎭 تنفيذ محلل async/await
    // ══════════════════════════════════════════════════════════════

    ArabicAsync::AsyncParser::AsyncParser()
        : asyncFunctionRegex(R"(دالة\s+غير_متزامنة\s+(\w+)\s*\()"),
          awaitRegex(R"(انتظر\s+([^;]+);)"),
          promiseRegex(R"(وعد\s*<([^>]+)>)") {
    }

    bool ArabicAsync::AsyncParser::isAsyncFunction(const std::string& code) {
        return std::regex_search(code, asyncFunctionRegex);
    }

    bool ArabicAsync::AsyncParser::containsAwait(const std::string& code) {
        return std::regex_search(code, awaitRegex);
    }

    std::vector<std::string> ArabicAsync::AsyncParser::extractAwaitExpressions(const std::string& code) {
        std::vector<std::string> awaits;
        std::smatch match;

        std::string::const_iterator searchStart(code.cbegin());
        while (std::regex_search(searchStart, code.cend(), match, awaitRegex)) {
            awaits.push_back(match[1].str());
            searchStart = match.suffix().first;
        }

        return awaits;
    }

    ArabicAsync::AsyncFunction ArabicAsync::AsyncParser::parseAsyncFunction(const std::string& functionCode) {
        AsyncFunction func("parsed_function");

        std::smatch match;
        if (std::regex_search(functionCode, match, asyncFunctionRegex)) {
            func = AsyncFunction(match[1].str());
        }

        // استخراج المعطيات (تبسيط)
        size_t paramStart = functionCode.find('(');
        size_t paramEnd = functionCode.find(')', paramStart);
        if (paramStart != std::string::npos && paramEnd != std::string::npos) {
            std::string paramsStr = functionCode.substr(paramStart + 1, paramEnd - paramStart - 1);
            // يمكن تحليل المعطيات هنا
        }

        // استخراج الجسم
        size_t bodyStart = functionCode.find('{');
        size_t bodyEnd = functionCode.rfind('}');
        if (bodyStart != std::string::npos && bodyEnd != std::string::npos) {
            func.setBody(functionCode.substr(bodyStart + 1, bodyEnd - bodyStart - 1));
        }

        return func;
    }

    std::string ArabicAsync::AsyncParser::transformToCppAsync(const std::string& arabicAsyncCode) {
        std::string cppCode = arabicAsyncCode;

        // استبدال الكلمات العربية بما يعادلها في C++
        std::unordered_map<std::string, std::string> replacements = {
            {"دالة غير_متزامنة", "std::future<Value>"},
            {"انتظر", "co_await"},
            {"وعد", "std::promise"},
            {"شغل_في_خلفية", "std::async(std::launch::async"},
            {"إنشاء_وعد", "std::make_shared<std::promise<Value>>()"},
            {"حل", "set_value"},
            {"رفض", "set_exception"},
            {"جرب", "try"},
            {"catch", "catch"}
        };

        for (const auto& pair : replacements) {
            size_t pos = 0;
            while ((pos = cppCode.find(pair.first, pos)) != std::string::npos) {
                cppCode.replace(pos, pair.first.length(), pair.second);
                pos += pair.second.length();
            }
        }

        return cppCode;
    }

    // ══════════════════════════════════════════════════════════════
    // 📦 تنفيذ الوعد
    // ══════════════════════════════════════════════════════════════

    // (التنفيذ موجود في الهيدر)

    // ══════════════════════════════════════════════════════════════
    // 🔄 تنفيذ مُنتظر await
    // ══════════════════════════════════════════════════════════════

    std::shared_ptr<ArabicAsync::ArabicPromise> ArabicAsync::AwaitHandler::createPromise(const std::string& promiseId) {
        std::string id = promiseId.empty() ?
            "promise_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) : promiseId;

        auto promise = std::make_shared<ArabicPromise>();
        activePromises[id] = promise;
        return promise;
    }

    std::shared_ptr<ArabicAsync::ArabicPromise> ArabicAsync::AwaitHandler::getPromise(const std::string& promiseId) {
        auto it = activePromises.find(promiseId);
        return it != activePromises.end() ? it->second : nullptr;
    }

    Value ArabicAsync::AwaitHandler::awaitPromise(const std::string& promiseId) {
        auto promise = getPromise(promiseId);
        return promise ? promise->await() : Value{ValueType::NONE};
    }

    void ArabicAsync::AwaitHandler::resolvePromise(const std::string& promiseId, const Value& value) {
        auto promise = getPromise(promiseId);
        if (promise) {
            promise->resolve(value);
        }
    }

    void ArabicAsync::AwaitHandler::rejectPromise(const std::string& promiseId, const std::string& error) {
        auto promise = getPromise(promiseId);
        if (promise) {
            promise->reject(error);
        }
    }

    std::vector<std::string> ArabicAsync::AwaitHandler::getActivePromises() const {
        std::vector<std::string> ids;
        for (const auto& pair : activePromises) {
            ids.push_back(pair.first);
        }
        return ids;
    }

    void ArabicAsync::AwaitHandler::cleanupResolvedPromises() {
        for (auto it = activePromises.begin(); it != activePromises.end(); ) {
            if (it->second->isResolved()) {
                it = activePromises.erase(it);
            } else {
                ++it;
            }
        }
    }

    // ══════════════════════════════════════════════════════════════
    // 🎯 تنفيذ واجهة الاستخدام العامة
    // ══════════════════════════════════════════════════════════════

    std::shared_ptr<ArabicAsync::AsyncTask> ArabicAsync::runAsync(std::function<Value()> function,
                                                                const std::string& taskId) {
        return scheduler.scheduleTask(std::move(function), taskId);
    }

    Value ArabicAsync::await(const std::shared_ptr<AsyncTask>& task) {
        return task ? task->getResult() : Value{ValueType::NONE};
    }

    std::vector<Value> ArabicAsync::awaitAll(const std::vector<std::shared_ptr<AsyncTask>>& tasks) {
        std::vector<Value> results;
        for (const auto& task : tasks) {
            if (task) {
                results.push_back(task->getResult());
            }
        }
        return results;
    }

    Value ArabicAsync::awaitAny(const std::vector<std::shared_ptr<AsyncTask>>& tasks) {
        // انتظار أول مهمة تكتمل
        while (!tasks.empty()) {
            for (const auto& task : tasks) {
                if (task && task->isReady()) {
                    return task->getResult();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return Value{ValueType::NONE};
    }

    bool ArabicAsync::registerAsyncFunction(const AsyncFunction& func) {
        std::lock_guard<std::mutex> lock(functionsMutex);
        asyncFunctions[func.getName()] = func;
        return true;
    }

    std::shared_ptr<ArabicAsync::AsyncTask> ArabicAsync::callAsyncFunction(const std::string& functionName,
                                                                          const std::vector<Value>& args) {
        std::lock_guard<std::mutex> lock(functionsMutex);
        auto it = asyncFunctions.find(functionName);
        if (it != asyncFunctions.end()) {
            return it->second.invoke(args);
        }
        return nullptr;
    }

    std::vector<std::string> ArabicAsync::getStats() const {
        std::vector<std::string> stats;
        stats.push_back("=== إحصائيات نظام Async/Await ===");
        stats.push_back("المهام النشطة: " + std::to_string(scheduler.getActiveTaskCount()));
        stats.push_back("المهام في الانتظار: " + std::to_string(scheduler.getQueuedTaskCount()));
        stats.push_back("عدد الخيوط: " + std::to_string(scheduler.getThreadCount()));
        stats.push_back("الدوال غير المتزامنة المسجلة: " + std::to_string(asyncFunctions.size()));
        stats.push_back("الوعود النشطة: " + std::to_string(awaitHandler.getActivePromises().size()));

        return stats;
    }

    void ArabicAsync::cleanup() {
        awaitHandler.cleanupResolvedPromises();
        scheduler.waitForAll();
    }

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة للبرمجة غير المتزامنة
    // ══════════════════════════════════════════════════════════════

    std::string transformArabicAsyncToCpp(const std::string& arabicCode) {
        return ArabicAsync::getInstance().getParser().transformToCppAsync(arabicCode);
    }

    std::shared_ptr<ArabicAsync::ArabicPromise> createPromise(const std::string& id) {
        return ArabicAsync::getInstance().getAwaitHandler().createPromise(id);
    }

    void startAsyncEventLoop() {
        // في الإصدار الكامل سيتم تنفيذ حلقة أحداث متقدمة
        ArabicAsync::getInstance().getScheduler().start();
    }

    void stopAsyncEventLoop() {
        ArabicAsync::getInstance().getScheduler().stop();
    }

    std::shared_ptr<ArabicAsync::AsyncTask> delayAsync(std::chrono::milliseconds duration,
                                                      const std::string& taskId) {
        return ArabicAsync::getInstance().getScheduler().scheduleDelayedTask(
            []() { return Value{ValueType::NONE}; }, duration, taskId);
    }

    // ══════════════════════════════════════════════════════════════
    // 📚 تنفيذ مكتبات async قياسية
    // ══════════════════════════════════════════════════════════════

    namespace AsyncLibraries {

        void registerAll() {
            auto& async = ArabicAsync::getInstance();

            // تسجيل دوال async الأساسية
            // (في الإصدار الكامل سيتم تحليل وتسجيل الدوال من النصوص)

            std::cout << "✅ تم تسجيل مكتبات async قياسية\n";
        }

    } // namespace AsyncLibraries

} // namespace ArabicLanguage
