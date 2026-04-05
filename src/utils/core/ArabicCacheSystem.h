#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "ArabicTypes.h"
#include "ArabicMemoryManager.h"
#include "ArabicTypeInference.h"
#include "SymbolTable.h"

namespace fs = std::filesystem;

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // 💾 نظام التخزين المؤقت الشامل
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief نظام تخزين مؤقت متقدم للAST والرموز والكود المترجم
     */
    class ArabicCacheSystem {
    public:
        // Singleton pattern
        static ArabicCacheSystem& getInstance();

        // منع النسخ والتعيين
        ArabicCacheSystem(const ArabicCacheSystem&) = delete;
        ArabicCacheSystem& operator=(const ArabicCacheSystem&) = delete;

        // ══════════════════════════════════════════════════════════════
        // 📦 بنية عنصر التخزين المؤقت
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief عنصر تخزين مؤقت عام
         */
        struct CacheEntry {
            std::string key;
            std::shared_ptr<void> data;
            std::string dataType;                    // "ast", "symbols", "types", "bytecode"
            size_t dataSize = 0;
            std::chrono::steady_clock::time_point created;
            std::chrono::steady_clock::time_point lastAccessed;
            std::atomic<size_t> accessCount{0};
            std::atomic<size_t> hitCount{0};
            int priority = 0;                       // أولوية التخزين

            CacheEntry(const std::string& k, std::shared_ptr<void> d,
                      const std::string& type, size_t size = 0)
                : key(k), data(std::move(d)), dataType(type), dataSize(size),
                  created(std::chrono::steady_clock::now()),
                  lastAccessed(std::chrono::steady_clock::now()) {}
        };

        /**
         * @brief إحصائيات التخزين المؤقت
         */
        struct CacheStats {
            size_t totalEntries = 0;
            size_t totalSize = 0;
            size_t maxSize = 100 * 1024 * 1024; // 100MB
            double hitRate = 0.0;
            size_t totalHits = 0;
            size_t totalMisses = 0;
            std::chrono::milliseconds avgAccessTime{0};
        };

        // ══════════════════════════════════════════════════════════════
        // 🌳 مخزن AST
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مخزن متخصص لشجرة التحليل التركيبي (AST)
         */
        class ASTCache {
        private:
            std::unordered_map<std::string, std::shared_ptr<std::vector<std::shared_ptr<Command>>>> astMap;
            mutable std::shared_mutex astMutex;
            size_t maxEntries = 1000;

        public:
            bool store(const std::string& key, const std::vector<std::shared_ptr<Command>>& ast);
            std::shared_ptr<std::vector<std::shared_ptr<Command>>> retrieve(const std::string& key);
            bool contains(const std::string& key) const;
            void clear();
            size_t size() const { return astMap.size(); }

            // تحسينات
            void optimize();
            std::vector<std::string> getFrequentKeys() const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🔤 مخزن الرموز
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مخزن متخصص لجدول الرموز
         */
        class SymbolCache {
        private:
            std::unordered_map<std::string, std::shared_ptr<SymbolTable>> symbolMap;
            mutable std::shared_mutex symbolMutex;
            size_t maxEntries = 500;

        public:
            bool store(const std::string& key, const SymbolTable& symbols);
            std::shared_ptr<SymbolTable> retrieve(const std::string& key);
            bool contains(const std::string& key) const;
            void clear();
            size_t size() const { return symbolMap.size(); }

            // تحسينات
            void mergeSymbols(const std::string& key1, const std::string& key2);
            std::vector<std::string> getUndefinedSymbols(const std::string& key) const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🎯 مخزن الأنواع
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مخزن متخصص لمعلومات الأنواع المستنتجة
         */
        class TypeCache {
        private:
            std::unordered_map<std::string, std::shared_ptr<std::unordered_map<std::string, ArabicTypeInference::TypeInfo>>> typeMap;
            mutable std::shared_mutex typeMutex;
            size_t maxEntries = 800;

        public:
            bool store(const std::string& key, const std::unordered_map<std::string, ArabicTypeInference::TypeInfo>& types);
            std::shared_ptr<std::unordered_map<std::string, ArabicTypeInference::TypeInfo>> retrieve(const std::string& key);
            bool contains(const std::string& key) const;
            void clear();
            size_t size() const { return typeMap.size(); }

            // تحسينات
            void updateType(const std::string& key, const std::string& varName, const ArabicTypeInference::TypeInfo& type);
            std::vector<std::string> getTypeDependencies(const std::string& key) const;
        };

        // ══════════════════════════════════════════════════════════════
        // 💾 مخزن البايت كود
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مخزن متخصص للبايت كود المترجم
         */
        class BytecodeCache {
        private:
            std::unordered_map<std::string, std::shared_ptr<std::vector<uint8_t>>> bytecodeMap;
            mutable std::shared_mutex bytecodeMutex;
            size_t maxEntries = 300;

        public:
            bool store(const std::string& key, const std::vector<uint8_t>& bytecode);
            std::shared_ptr<std::vector<uint8_t>> retrieve(const std::string& key);
            bool contains(const std::string& key) const;
            void clear();
            size_t size() const { return bytecodeMap.size(); }

            // تحسينات
            void optimizeBytecode(const std::string& key);
            size_t getBytecodeSize(const std::string& key) const;
        };

        // ══════════════════════════════════════════════════════════════
        // 💾 تخزين مستمر (Persistent Storage)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief نظام تخزين مستمر على القرص
         */
        class PersistentStorage {
        private:
            fs::path cacheDir;
            std::mutex fileMutex;
            std::atomic<size_t> totalStoredSize{0};

        public:
            explicit PersistentStorage(const fs::path& dir = "./cache");
            void setDirectory(const fs::path& dir);

            bool saveToDisk(const std::string& key, const std::string& dataType, const std::string& data);
            std::string loadFromDisk(const std::string& key, const std::string& dataType);
            bool existsOnDisk(const std::string& key, const std::string& dataType) const;
            void cleanupOldEntries(std::chrono::hours maxAge = std::chrono::hours(24));
            void clearDiskCache();

            size_t getTotalStoredSize() const { return totalStoredSize.load(); }
            std::vector<std::string> getStoredKeys(const std::string& dataType = "") const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🧹 مدير التنظيف التلقائي
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مدير تنظيف تلقائي للتخزين المؤقت
         */
        class CacheCleaner {
        private:
            std::thread cleanerThread;
            std::atomic<bool> running{false};
            std::chrono::minutes cleanupInterval{30};
            std::function<void()> cleanupCallback;

        public:
            explicit CacheCleaner(std::function<void()> callback);
            ~CacheCleaner();

            void start();
            void stop();
            void setCleanupInterval(std::chrono::minutes interval);
            bool isRunning() const { return running.load(); }
        };

        // ══════════════════════════════════════════════════════════════
        // 🎯 واجهة الاستخدام العامة
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تخزين AST
         */
        bool storeAST(const std::string& key, const std::vector<std::shared_ptr<Command>>& ast);

        /**
         * @brief استرجاع AST
         */
        std::shared_ptr<std::vector<std::shared_ptr<Command>>> getAST(const std::string& key);

        /**
         * @brief تخزين جدول رموز
         */
        bool storeSymbols(const std::string& key, const SymbolTable& symbols);

        /**
         * @brief استرجاع جدول رموز
         */
        std::shared_ptr<SymbolTable> getSymbols(const std::string& key);

        /**
         * @brief تخزين معلومات أنواع
         */
        bool storeTypes(const std::string& key, const std::unordered_map<std::string, ArabicTypeInference::TypeInfo>& types);

        /**
         * @brief استرجاع معلومات أنواع
         */
        std::shared_ptr<std::unordered_map<std::string, ArabicTypeInference::TypeInfo>> getTypes(const std::string& key);

        /**
         * @brief تخزين بايت كود
         */
        bool storeBytecode(const std::string& key, const std::vector<uint8_t>& bytecode);

        /**
         * @brief استرجاع بايت كود
         */
        std::shared_ptr<std::vector<uint8_t>> getBytecode(const std::string& key);

        // إدارة عامة
        void clearAll();
        void optimizeAll();
        CacheStats getStats() const;

        // إعدادات
        void setMaxMemorySize(size_t bytes);
        void setCacheDirectory(const fs::path& dir);
        void enablePersistentStorage(bool enable);

        // مراقبة
        void startAutoCleanup();
        void stopAutoCleanup();

    private:
        ASTCache astCache;
        SymbolCache symbolCache;
        TypeCache typeCache;
        BytecodeCache bytecodeCache;
        PersistentStorage persistentStorage;
        CacheCleaner cacheCleaner;

        std::atomic<size_t> maxMemorySize{100 * 1024 * 1024}; // 100MB
        std::atomic<bool> persistentEnabled{true};

        CacheStats stats;
        mutable std::shared_mutex statsMutex;

        ArabicCacheSystem();
        ~ArabicCacheSystem();

        // دوال مساعدة
        void updateStats(const std::string& operation, bool hit, size_t dataSize = 0);
        void enforceMemoryLimits();
        std::string generateCacheKey(const std::string& baseKey, const std::string& dataType) const;
        void performCleanup();

        // دوال ثابتة للتنظيف التلقائي
        static void cleanupTask();
    };

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة للتخزين المؤقت
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief إنشاء مفتاح تخزين مؤقت ذكي
     */
    std::string createCacheKey(const std::string& source, const std::string& context = "");

    /**
     * @brief فحص ما إذا كان المفتاح صالح للتخزين
     */
    bool isValidCacheKey(const std::string& key);

    /**
     * @brief حساب حجم البيانات التقريبي
     */
    size_t estimateDataSize(const std::vector<std::shared_ptr<Command>>& ast);

    /**
     * @brief ضغط البيانات للتخزين
     */
    std::string compressData(const std::string& data);

    /**
     * @brief فك ضغط البيانات
     */
    std::string decompressData(const std::string& compressedData);

    /**
     * @brief تنظيف التخزين المؤقت تلقائياً
     */
    void cleanupCacheDirectory(const fs::path& cacheDir, std::chrono::hours maxAge = std::chrono::hours(24));

    /**
     * @brief تصدير إحصائيات التخزين المؤقت
     */
    void exportCacheStats(const ArabicCacheSystem::CacheStats& stats, const fs::path& outputFile);

} // namespace ArabicLanguage
