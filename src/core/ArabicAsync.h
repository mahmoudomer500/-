#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <regex>
#include "ArabicTypes.h"
#include "ArabicMemoryManager.h"

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // ⚡ نظام async/await للغة العربية
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief نظام شامل للبرمجة غير المتزامنة مع async/await
     */
    class ArabicAsync {
    public:
        // Singleton pattern
        static ArabicAsync& getInstance();

        // منع النسخ والتعيين
        ArabicAsync(const ArabicAsync&) = delete;
        ArabicAsync& operator=(const ArabicAsync&) = delete;

        // ══════════════════════════════════════════════════════════════
        // 🎯 المهمة غير المتزامنة (Async Task)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تمثيل مهمة غير متزامنة
         */
        class AsyncTask {
        private:
            std::future<Value> future;
            std::string taskId;
            std::chrono::steady_clock::time_point startTime;
            bool completed = false;
            Value result;

        public:
            AsyncTask(std::future<Value> f, const std::string& id)
                : future(std::move(f)), taskId(id), startTime(std::chrono::steady_clock::now()) {}

            bool isReady() const {
                return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }

            Value getResult() {
                if (!completed) {
                    result = future.get();
                    completed = true;
                }
                return result;
            }

            const std::string& getId() const { return taskId; }

            double getExecutionTime() const {
                auto endTime = std::chrono::steady_clock::now();
                return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            }

            bool isCompleted() const { return completed; }
        };

        // ══════════════════════════════════════════════════════════════
        // 🏭 مُجمع المهام (Task Scheduler)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مجمع مهام متقدم مع دعم الأولويات
         */
        class TaskScheduler {
        private:
            struct ScheduledTask {
                std::function<Value()> taskFunction;
                std::string taskId;
                int priority = 0; // الأولوية (أعلى = أكثر أهمية)
                std::chrono::steady_clock::time_point scheduledTime;

                bool operator<(const ScheduledTask& other) const {
                    return priority < other.priority ||
                           (priority == other.priority && scheduledTime > other.scheduledTime);
                }
            };

            std::priority_queue<ScheduledTask> taskQueue;
            std::vector<std::thread> workerThreads;
            mutable std::mutex queueMutex;
            std::condition_variable queueCondition;
            std::atomic<bool> running{false};
            std::atomic<size_t> activeTasks{0};
            size_t maxThreads;

            void workerThread();

        public:
            explicit TaskScheduler(size_t numThreads = std::thread::hardware_concurrency());
            ~TaskScheduler();

            void start();
            void stop();
            void waitForAll();

            std::shared_ptr<AsyncTask> scheduleTask(std::function<Value()> task,
                                                  const std::string& taskId = "",
                                                  int priority = 0);
            std::shared_ptr<AsyncTask> scheduleDelayedTask(std::function<Value()> task,
                                                         std::chrono::milliseconds delay,
                                                         const std::string& taskId = "",
                                                         int priority = 0);

            size_t getActiveTaskCount() const { return activeTasks.load(); }
            size_t getQueuedTaskCount() const {
                std::lock_guard<std::mutex> lock(queueMutex);
                return taskQueue.size();
            }
            size_t getThreadCount() const { return workerThreads.size(); }
        };

        // ══════════════════════════════════════════════════════════════
        // 🔄 دالة غير متزامنة (Async Function)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief دالة غير متزامنة مع دعم await
         */
        class AsyncFunction {
        private:
            std::string name;
            std::vector<std::pair<std::string, std::string>> parameters;
            std::string body;
            std::string returnType;
            std::function<Value(std::vector<Value>)> compiledFunction;

        public:
            AsyncFunction() : name(""), returnType("وعد") {}
            AsyncFunction(const std::string& n, const std::string& retType = "وعد")
                : name(n), returnType(retType) {}

            void addParameter(const std::string& name, const std::string& type) {
                parameters.push_back({name, type});
            }

            void setBody(const std::string& b) { body = b; }
            void setCompiledFunction(std::function<Value(std::vector<Value>)> func) { compiledFunction = func; }

            std::shared_ptr<AsyncTask> invoke(const std::vector<Value>& args);
            std::string getSignature() const;
            const std::string& getName() const { return name; }
        };

        // ══════════════════════════════════════════════════════════════
        // 🎭 محلل async/await
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief محلل يحول الكود العربي غير المتزامن إلى كود C++
         */
        class AsyncParser {
        private:
            std::regex asyncFunctionRegex;
            std::regex awaitRegex;
            std::regex promiseRegex;

        public:
            AsyncParser();
            bool isAsyncFunction(const std::string& code);
            bool containsAwait(const std::string& code);
            std::vector<std::string> extractAwaitExpressions(const std::string& code);
            AsyncFunction parseAsyncFunction(const std::string& functionCode);
            std::string transformToCppAsync(const std::string& arabicAsyncCode);
        };

        // ══════════════════════════════════════════════════════════════
        // 📦 وعد (Promise)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تنفيذ الوعود للعمليات غير المتزامنة
         */
        class ArabicPromise {
        private:
            std::promise<Value> promise;
            std::shared_future<Value> future;
            std::atomic<bool> resolved{false};
            Value result;

        public:
            ArabicPromise() : future(promise.get_future()) {}

            void resolve(const Value& value) {
                if (!resolved.exchange(true)) {
                    result = value;
                    promise.set_value(value);
                }
            }

            void reject(const std::string& error) {
                if (!resolved.exchange(true)) {
                    // إنشاء قيمة خطأ
                    Value errorValue;
                    errorValue.type = ValueType::STRING;
                    errorValue.string_value = error;
                    result = errorValue;
                    promise.set_value(errorValue);
                }
            }

            Value await() {
                return future.get();
            }

            bool isResolved() const { return resolved.load(); }
            const Value& getResult() const { return result; }
        };

        // ══════════════════════════════════════════════════════════════
        // 🔄 مُنتظر (Await Handler)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief معالج عمليات await
         */
        class AwaitHandler {
        private:
            std::unordered_map<std::string, std::shared_ptr<ArabicPromise>> activePromises;

        public:
            std::shared_ptr<ArabicPromise> createPromise(const std::string& promiseId = "");
            std::shared_ptr<ArabicPromise> getPromise(const std::string& promiseId);
            Value awaitPromise(const std::string& promiseId);
            void resolvePromise(const std::string& promiseId, const Value& value);
            void rejectPromise(const std::string& promiseId, const std::string& error);

            std::vector<std::string> getActivePromises() const;
            void cleanupResolvedPromises();
        };

        // ══════════════════════════════════════════════════════════════
        // 🎯 واجهة الاستخدام العامة
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تنفيذ دالة غير متزامنة
         */
        std::shared_ptr<AsyncTask> runAsync(std::function<Value()> function,
                                          const std::string& taskId = "");

        /**
         * @brief انتظار مهمة غير متزامنة
         */
        Value await(const std::shared_ptr<AsyncTask>& task);

        /**
         * @brief انتظار وعود متعددة
         */
        std::vector<Value> awaitAll(const std::vector<std::shared_ptr<AsyncTask>>& tasks);

        /**
         * @brief انتظار أول مهمة تكتمل
         */
        Value awaitAny(const std::vector<std::shared_ptr<AsyncTask>>& tasks);

        /**
         * @brief تسجيل دالة غير متزامنة
         */
        bool registerAsyncFunction(const AsyncFunction& func);

        /**
         * @brief استدعاء دالة غير متزامنة مسجلة
         */
        std::shared_ptr<AsyncTask> callAsyncFunction(const std::string& functionName,
                                                   const std::vector<Value>& args);

        // إدارة المهام
        TaskScheduler& getScheduler() { return scheduler; }
        AwaitHandler& getAwaitHandler() { return awaitHandler; }

        // إحصائيات ومراقبة
        std::vector<std::string> getStats() const;
        void cleanup();

        // محلل async/await
        AsyncParser& getParser() { return parser; }

    private:
        TaskScheduler scheduler;
        AwaitHandler awaitHandler;
        AsyncParser parser;
        std::unordered_map<std::string, AsyncFunction> asyncFunctions;
        std::mutex functionsMutex;

        ArabicAsync();
        ~ArabicAsync();
    };

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة للبرمجة غير المتزامنة
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief دالة مساعدة لإنشاء مهمة async
     */
    template<typename Func, typename... Args>
    std::shared_ptr<ArabicAsync::AsyncTask> async(Func&& func, Args&&... args) {
        return ArabicAsync::getInstance().runAsync(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
    }

    /**
     * @brief دالة مساعدة للانتظار
     */
    Value await(const std::shared_ptr<ArabicAsync::AsyncTask>& task) {
        return ArabicAsync::getInstance().await(task);
    }

    /**
     * @brief تحويل كود عربي async إلى C++
     */
    std::string transformArabicAsyncToCpp(const std::string& arabicCode);

    /**
     * @brief إنشاء وعد جديد
     */
    std::shared_ptr<ArabicAsync::ArabicPromise> createPromise(const std::string& id = "");

    /**
     * @brief بدء حلقة الأحداث (Event Loop)
     */
    void startAsyncEventLoop();

    /**
     * @brief إيقاف حلقة الأحداث
     */
    void stopAsyncEventLoop();

    /**
     * @brief انتظار فترة زمنية
     */
    std::shared_ptr<ArabicAsync::AsyncTask> delayAsync(std::chrono::milliseconds duration,
                                                      const std::string& taskId = "");

    // ══════════════════════════════════════════════════════════════
    // 📚 مكتبات async قياسية
    // ══════════════════════════════════════════════════════════════

    namespace AsyncLibraries {

        /**
         * @brief دوال async قياسية
         */
        const std::string ASYNC_IO_OPERATIONS = R"(
دالة غير_متزامنة اقرأ_ملف_نصي(اسم_الملف: نص) -> وعد<نص> {
    وعد = إنشاء_وعد()
    شغل_في_خلفية {
        جرب {
            محتوى = اقرأ_ملف(اسم_الملف)
            وعد.حل(محتوى)
        } catch(خطأ) {
            وعد.رفض(خطأ)
        }
    }
    ارجع وعد
}

دالة غير_متزامنة اكتب_ملف_نصي(اسم_الملف: نص، محتوى: نص) -> وعد<منطق> {
    وعد = إنشاء_وعد()
    شغل_في_خلفية {
        جرب {
            اكتب_ملف(اسم_الملف، محتوى)
            وعد.حل(صحيح)
        } catch(خطأ) {
            وعد.رفض(خطأ)
        }
    }
    ارجع وعد
}
)";

        const std::string ASYNC_NETWORK_OPERATIONS = R"(
دالة غير_متزامنة طلب_شبكي(url: نص) -> وعد<نص> {
    وعد = إنشاء_وعد()
    شغل_في_خلفية {
        جرب {
            رد = أرسل_طلب_شبكي(url)
            وعد.حل(رد)
        } catch(خطأ) {
            وعد.رفض(خطأ)
        }
    }
    ارجع وعد
}

دالة غير_متزامنة انتظار_عدة_مهام(مهام: قائمة<وعد>) -> وعد<قائمة> {
    وعد = إنشاء_وعد()
    عداد = 0
    نتائج = قائمة_جديدة()

    لكل مهمة في مهام {
        انتظر مهمة بعد_انتهاء {
            عداد = عداد + 1
            أضف(نتائج، القيمة)

            إذا عداد == طول(مهام) {
                وعد.حل(نتائج)
            }
        }
    }

    ارجع وعد
}
)";

        /**
         * @brief تسجيل جميع المكتبات غير المتزامنة القياسية
         */
        void registerAll();

    } // namespace AsyncLibraries

} // namespace ArabicLanguage
