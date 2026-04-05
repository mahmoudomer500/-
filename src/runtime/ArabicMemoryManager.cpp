#include "ArabicMemoryManager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // 🚀 تنفيذ نظام إدارة الذاكرة المتقدم
    // ══════════════════════════════════════════════════════════════

    // Singleton instance
    ArabicMemoryManager& ArabicMemoryManager::getInstance() {
        static ArabicMemoryManager instance;
        return instance;
    }

    // ══════════════════════════════════════════════════════════════
    // 🧵 خيط جامع القمامة التلقائي
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief خيط تلقائي لجمع القمامة كل فترة
     */
    class AutoGCThread {
    private:
        std::thread gcThread;
        std::atomic<bool> running{false};
        std::chrono::milliseconds interval{5000}; // 5 ثوانٍ

        void gcLoop() {
            while (running) {
                std::this_thread::sleep_for(interval);
                if (running) {
                    ArabicMemoryManager::getInstance().collectGarbage();
                }
            }
        }

    public:
        void start() {
            if (!running) {
                running = true;
                gcThread = std::thread(&AutoGCThread::gcLoop, this);
            }
        }

        void stop() {
            if (running) {
                running = false;
                if (gcThread.joinable()) {
                    gcThread.join();
                }
            }
        }

        void setInterval(std::chrono::milliseconds ms) {
            interval = ms;
        }

        ~AutoGCThread() {
            stop();
        }
    };

    // متغير عام للخيط التلقائي
    static AutoGCThread autoGC;

    // ══════════════════════════════════════════════════════════════
    // 📊 دوال إحصائيات متقدمة
    // ══════════════════════════════════════════════════════════════

    ArabicMemoryManager::ObjectPool<Command>& ArabicMemoryManager::getCommandPool() {
        return smartManager.getPool<Command>("commands");
    }

    ArabicMemoryManager::ObjectPool<Value>& ArabicMemoryManager::getValuePool() {
        return smartManager.getPool<Value>("values");
    }

    ArabicMemoryManager::ObjectPool<std::string>& ArabicMemoryManager::getStringPool() {
        return smartManager.getPool<std::string>("strings");
    }

    std::vector<std::string> ArabicMemoryManager::getAllStats() {
        std::vector<std::string> stats;
        stats.push_back("=== إحصائيات شاملة للذاكرة ===");
        stats.push_back("الوقت: " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()));

        auto smartStats = smartManager.getStats();
        stats.insert(stats.end(), smartStats.begin(), smartStats.end());

        auto profilerStats = profiler.getProfilingReport();
        stats.insert(stats.end(), profilerStats.begin(), profilerStats.end());

        // إحصائيات التجمعات
        stats.push_back("");
        stats.push_back("=== إحصائيات التجمعات ===");
        auto& cmdPool = getCommandPool();
        stats.push_back("تجمع الأوامر:");
        stats.push_back("  المُنشأ: " + std::to_string(cmdPool.getCreatedCount()));
        stats.push_back("  المُعاد استخدامه: " + std::to_string(cmdPool.getReusedCount()));
        stats.push_back("  حجم التجمع: " + std::to_string(cmdPool.getPoolSize()));

        auto& valPool = getValuePool();
        stats.push_back("تجمع القيم:");
        stats.push_back("  المُنشأ: " + std::to_string(valPool.getCreatedCount()));
        stats.push_back("  المُعاد استخدامه: " + std::to_string(valPool.getReusedCount()));
        stats.push_back("  حجم التجمع: " + std::to_string(valPool.getPoolSize()));

        return stats;
    }

    // ══════════════════════════════════════════════════════════════
    // 🎯 دوال تحسين الأداء
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief تحسين استخدام الذاكرة للكائنات المؤقتة
     */
    void optimizeMemoryUsage() {
        auto& mm = ArabicMemoryManager::getInstance();

        // جمع القمامة
        mm.collectGarbage();

        // تنظيف التجمعات الكبيرة جداً
        // (سيتم تنفيذ هذا لاحقاً في ObjectPool)

        // تسجيل لقطة للمراقبة
        mm.getProfiler().takeSnapshot();
    }

    /**
     * @brief بدء المراقبة التلقائية للذاكرة
     */
    void startMemoryMonitoring() {
        autoGC.start();
    }

    /**
     * @brief إيقاف المراقبة التلقائية
     */
    void stopMemoryMonitoring() {
        autoGC.stop();
    }

    /**
     * @brief تعيين فترة جمع القمامة التلقائي
     */
    void setGarbageCollectionInterval(std::chrono::milliseconds interval) {
        autoGC.setInterval(interval);
    }

    /**
     * @brief طباعة تقرير شامل عن استخدام الذاكرة
     */
    void printMemoryReport() {
        auto& mm = ArabicMemoryManager::getInstance();
        auto stats = mm.getAllStats();

        std::cout << "\n";
        for (const auto& stat : stats) {
            std::cout << stat << "\n";
        }
        std::cout << std::endl;
    }

    /**
     * @brief حفظ تقرير الذاكرة في ملف
     */
    bool saveMemoryReport(const std::string& filename) {
        try {
            auto& mm = ArabicMemoryManager::getInstance();
            auto stats = mm.getAllStats();

            std::ofstream file(filename, std::ios::out | std::ios::app);
            if (file.is_open()) {
                file << "\n=== تقرير الذاكرة - " <<
                    std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << " ===\n";

                for (const auto& stat : stats) {
                    file << stat << "\n";
                }
                file << "\n";
                file.close();
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "خطأ في حفظ تقرير الذاكرة: " << e.what() << std::endl;
        }
        return false;
    }

    // ══════════════════════════════════════════════════════════════
    // 🧪 دوال الاختبار والتشخيص
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief اختبار أداء نظام إدارة الذاكرة
     */
    void benchmarkMemoryManager(size_t iterations = 10000) {
        auto& mm = ArabicMemoryManager::getInstance();
        auto start = std::chrono::high_resolution_clock::now();

        std::cout << "🔬 اختبار أداء نظام إدارة الذاكرة (" << iterations << " تكرار)...\n";

        // اختبار تجمع الكائنات
        for (size_t i = 0; i < iterations; ++i) {
            auto cmd = mm.getCommandPool().acquire();
            // استخدام سريع للكائن
            mm.getCommandPool().release(std::move(cmd));
        }

        // اختبار جامع القمامة
        for (size_t i = 0; i < iterations; ++i) {
            void* ptr = mm.getGC().allocate(sizeof(int), "int");
            mm.getGC().deallocate(ptr);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "✅ اكتمل الاختبار في " << duration.count() << " مللي ثانية\n";
        std::cout << "متوسط الوقت لكل عملية: " << (duration.count() * 1000.0 / iterations) << " ميكرو ثانية\n";
    }

    /**
     * @brief فحص سلامة الذاكرة
     */
    bool performMemoryIntegrityCheck() {
        auto& mm = ArabicMemoryManager::getInstance();
        bool isHealthy = true;

        std::cout << "🔍 فحص سلامة الذاكرة...\n";

        // فحص جامع القمامة
        auto gcStats = mm.getGC().getMemoryStats();
        if (mm.getGC().getLiveObjects() > 10000) { // حد تعسفي
            std::cout << "⚠️  عدد كبير من الكائنات الحية: " << mm.getGC().getLiveObjects() << "\n";
            isHealthy = false;
        }

        // فحص التجمعات
        if (mm.getCommandPool().getPoolSize() > 1000) {
            std::cout << "⚠️  تجمع أوامر كبير جداً: " << mm.getCommandPool().getPoolSize() << "\n";
        }

        if (isHealthy) {
            std::cout << "✅ الذاكرة سليمة\n";
        } else {
            std::cout << "❌ تم العثور على مشاكل في الذاكرة\n";
        }

        return isHealthy;
    }

} // namespace ArabicLanguage
