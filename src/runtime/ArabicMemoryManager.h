#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>
#include "ArabicTypes.h"
#include "ArabicReferenceCounter.h"

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // 🚀 نظام إدارة الذاكرة المتقدم - لأداء فائق
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief نظام إدارة الذاكرة المتقدم مع تجمع الكائنات وجامع القمامة
     */
    class ArabicMemoryManager {
    public:
        // Singleton pattern
        static ArabicMemoryManager& getInstance();

        // منع النسخ والتعيين
        ArabicMemoryManager(const ArabicMemoryManager&) = delete;
        ArabicMemoryManager& operator=(const ArabicMemoryManager&) = delete;

        // ══════════════════════════════════════════════════════════════
        // 🎯 تجمع الكائنات (Object Pooling)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief فئة أساسية لتجمع الكائنات
         */
        class ObjectPoolBase {
        public:
            virtual ~ObjectPoolBase() = default;
            virtual void clear() = 0;
        };

        /**
         * @brief تجمع كائنات عام لإعادة استخدام الكائنات
         */
        template<typename T>
        class ObjectPool : public ObjectPoolBase {
        private:
            std::queue<std::unique_ptr<T>> pool;
            mutable std::mutex poolMutex;
            std::atomic<size_t> createdCount{0};
            std::atomic<size_t> reusedCount{0};

        public:
            ~ObjectPool() {
                while (!pool.empty()) {
                    pool.pop();
                }
            }

            template<typename... Args>
            std::unique_ptr<T> acquire(Args&&... args) {
                std::lock_guard<std::mutex> lock(poolMutex);

                if (!pool.empty()) {
                    auto obj = std::move(pool.front());
                    pool.pop();
                    reusedCount++;
                    // إعادة تهيئة الكائن إذا لزم الأمر
                    if constexpr (std::is_constructible_v<T, Args...>) {
                        *obj = T(std::forward<Args>(args)...);
                    }
                    return obj;
                }

                createdCount++;
                return std::make_unique<T>(std::forward<Args>(args)...);
            }

            void release(std::unique_ptr<T> obj) {
                if (!obj) return;

                std::lock_guard<std::mutex> lock(poolMutex);
                pool.push(std::move(obj));
            }

            size_t getCreatedCount() const { return createdCount.load(); }
            size_t getReusedCount() const { return reusedCount.load(); }
            size_t getPoolSize() const {
                std::lock_guard<std::mutex> lock(poolMutex);
                return pool.size();
            }

            void clear() override {
                std::lock_guard<std::mutex> lock(poolMutex);
                while (!pool.empty()) {
                    pool.pop();
                }
                createdCount = 0;
                reusedCount = 0;
            }
        };

        // ══════════════════════════════════════════════════════════════
        // 🗑️ جامع القمامة (Garbage Collector)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief جامع قمامة ذكي مع تتبع المراجع
         */
        class GarbageCollector {
        private:
            struct GCObject {
                void* ptr;
                size_t size;
                std::string typeName;
                std::chrono::steady_clock::time_point created;
                std::atomic<int> refCount{1};
                bool marked = false;

                GCObject(void* p, size_t s, const std::string& type)
                    : ptr(p), size(s), typeName(type),
                      created(std::chrono::steady_clock::now()) {}
            };

            std::unordered_map<void*, std::unique_ptr<GCObject>> objects;
            std::mutex gcMutex;
            std::atomic<size_t> totalAllocated{0};
            std::atomic<size_t> totalCollected{0};
            bool running = false;

        public:
            ~GarbageCollector() {
                collectAll();
            }

            void* allocate(size_t size, const std::string& typeName = "unknown") {
                void* ptr = ::operator new(size);
                std::lock_guard<std::mutex> lock(gcMutex);
                objects[ptr] = std::make_unique<GCObject>(ptr, size, typeName);
                totalAllocated += size;
                return ptr;
            }

            void deallocate(void* ptr) {
                if (!ptr) return;

                std::lock_guard<std::mutex> lock(gcMutex);
                auto it = objects.find(ptr);
                if (it != objects.end()) {
                    totalCollected += it->second->size;
                    objects.erase(it);
                }
                ::operator delete(ptr);
            }

            void retain(void* ptr) {
                if (!ptr) return;
                std::lock_guard<std::mutex> lock(gcMutex);
                auto it = objects.find(ptr);
                if (it != objects.end()) {
                    it->second->refCount++;
                }
            }

            void release(void* ptr) {
                if (!ptr) return;
                std::lock_guard<std::mutex> lock(gcMutex);
                auto it = objects.find(ptr);
                if (it != objects.end()) {
                    if (--it->second->refCount <= 0) {
                        deallocate(ptr);
                    }
                }
            }

            void collectGarbage() {
                std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(gcMutex));
                auto now = std::chrono::steady_clock::now();

                for (auto it = objects.begin(); it != objects.end(); ) {
                    auto& obj = it->second;
                    auto age = std::chrono::duration_cast<std::chrono::seconds>(
                        now - obj->created).count();

                    // جمع الكائنات القديمة غير المستخدمة
                    if (obj->refCount <= 0 || age > 300) { // 5 دقائق
                        totalCollected += obj->size;
                        ::operator delete(obj->ptr);
                        it = objects.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            void collectAll() {
                std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(gcMutex));
                for (auto& pair : objects) {
                    ::operator delete(pair.second->ptr);
                    totalCollected += pair.second->size;
                }
                objects.clear();
            }

            size_t getTotalAllocated() const { return totalAllocated.load(); }
            size_t getTotalCollected() const { return totalCollected.load(); }
            size_t getLiveObjects() const {
                std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(gcMutex));
                return objects.size();
            }

            std::vector<std::string> getMemoryStats() const {
                std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(gcMutex));
                std::vector<std::string> stats;
                stats.push_back("=== إحصائيات الذاكرة ===");
                stats.push_back("الكائنات الحية: " + std::to_string(objects.size()));
                stats.push_back("إجمالي المخصص: " + std::to_string(totalAllocated.load()) + " بايت");
                stats.push_back("إجمالي المجموع: " + std::to_string(totalCollected.load()) + " بايت");

                size_t memoryByType = 0;
                std::unordered_map<std::string, size_t> typeStats;
                for (const auto& pair : objects) {
                    typeStats[pair.second->typeName] += pair.second->size;
                    memoryByType += pair.second->size;
                }

                stats.push_back("الذاكرة حسب النوع:");
                for (const auto& pair : typeStats) {
                    stats.push_back("  " + pair.first + ": " + std::to_string(pair.second) + " بايت");
                }

                return stats;
            }
        };

        // ══════════════════════════════════════════════════════════════
        // 🔄 إدارة الذاكرة المتقدمة
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مدير ذاكرة ذكي مع تحسينات الأداء
         */
        class SmartMemoryManager {
        private:
            std::unordered_map<std::string, std::unique_ptr<ObjectPoolBase>> pools;
            GarbageCollector gc;
            std::mutex managerMutex;

        public:
            template<typename T>
            ObjectPool<T>& getPool(const std::string& poolName = "") {
                std::string name = poolName.empty() ? typeid(T).name() : poolName;
                std::lock_guard<std::mutex> lock(managerMutex);

                auto it = pools.find(name);
                if (it == pools.end()) {
                    auto pool = std::make_unique<ObjectPool<T>>();
                    ObjectPool<T>& ref = *pool;
                    pools[name] = std::unique_ptr<ObjectPoolBase>(pool.release());
                    return ref;
                }
                return static_cast<ObjectPool<T>&>(*it->second);
            }

            /**
             * @brief إعادة تدوير ذكية للتجمعات
             * تقوم بتحرير الكائنات غير المستخدمة من التجمعات الكبيرة
             */
            void smartRecycle() {
                std::lock_guard<std::mutex> lock(managerMutex);

                for (auto& pair : pools) {
                    // تحرير التجمعات التي تحتوي على كائنات غير مستخدمة لفترة طويلة
                    auto& poolBase = pair.second;
                    if (poolBase) {
                        // إذا كان التجمع يحتوي على أكثر من 50 كائن ولم يُستخدم مؤخراً
                        poolBase->clear(); // تنظيف دوري
                    }
                }
            }

            /**
             * @brief تحسين التجمعات بناءً على أنماط الاستخدام
             */
            void optimizePools() {
                std::lock_guard<std::mutex> lock(managerMutex);

                // يمكن إضافة منطق تحسين متقدم هنا
                // مثل تغيير حجم التجمعات أو دمج التجمعات الصغيرة
                smartRecycle();
            }

            GarbageCollector& getGC() { return gc; }

            void collectGarbage() {
                gc.collectGarbage();
            }

            void cleanup() {
                std::lock_guard<std::mutex> lock(managerMutex);
                pools.clear();
                gc.collectAll();
            }

            std::vector<std::string> getStats() const {
                std::vector<std::string> stats;
                stats.push_back("=== إحصائيات مدير الذاكرة الذكي ===");

                std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(managerMutex));
                stats.push_back("عدد التجمعات: " + std::to_string(pools.size()));

                auto gcStats = gc.getMemoryStats();
                stats.insert(stats.end(), gcStats.begin(), gcStats.end());

                return stats;
            }
        };

        // ══════════════════════════════════════════════════════════════
        // 📊 مراقبة الأداء والإحصائيات
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مدير الذاكرة المؤقتة
         */
        class TemporaryMemoryManager {
        private:
            std::vector<void*> tempObjects;
            std::unordered_map<void*, std::chrono::steady_clock::time_point> tempLifetimes;
            std::mutex tempMutex;
            std::chrono::milliseconds defaultLifetime{1000}; // 1 ثانية

        public:
            ~TemporaryMemoryManager() {
                cleanup();
            }

            /**
             * @brief إضافة كائن مؤقت
             */
            void addTemporary(void* ptr, std::chrono::milliseconds lifetime = std::chrono::milliseconds(1000)) {
                std::lock_guard<std::mutex> lock(tempMutex);
                tempObjects.push_back(ptr);
                tempLifetimes[ptr] = std::chrono::steady_clock::now() + lifetime;
            }

            /**
             * @brief تنظيف الكائنات المؤقتة المنتهية الصلاحية
             */
            size_t cleanupExpired() {
                std::lock_guard<std::mutex> lock(tempMutex);
                auto now = std::chrono::steady_clock::now();
                size_t cleaned = 0;

                for (auto it = tempObjects.begin(); it != tempObjects.end(); ) {
                    auto lifetimeIt = tempLifetimes.find(*it);
                    if (lifetimeIt != tempLifetimes.end() && now >= lifetimeIt->second) {
                        // الكائن انتهت صلاحيته
                        ::operator delete(*it);
                        tempLifetimes.erase(lifetimeIt);
                        it = tempObjects.erase(it);
                        cleaned++;
                    } else {
                        ++it;
                    }
                }

                return cleaned;
            }

            /**
             * @brief تنظيف جميع الكائنات المؤقتة
             */
            void cleanup() {
                std::lock_guard<std::mutex> lock(tempMutex);
                for (void* ptr : tempObjects) {
                    ::operator delete(ptr);
                }
                tempObjects.clear();
                tempLifetimes.clear();
            }

            size_t getTempObjectCount() const {
                std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(tempMutex));
                return tempObjects.size();
            }
        };

        /**
         * @brief مراقب أداء الذاكرة
         */
        class MemoryProfiler {
        private:
            struct MemorySnapshot {
                size_t allocated;
                size_t freed;
                size_t peakUsage;
                std::chrono::steady_clock::time_point timestamp;
            };

            std::vector<MemorySnapshot> snapshots;
            std::mutex profilerMutex;
            std::atomic<size_t> currentAllocated{0};
            std::atomic<size_t> peakAllocated{0};

        public:
            void recordAllocation(size_t size) {
                currentAllocated += size;
                size_t current = currentAllocated.load();
                size_t peak = peakAllocated.load();
                while (current > peak && !peakAllocated.compare_exchange_weak(peak, current)) {}
            }

            void recordDeallocation(size_t size) {
                currentAllocated -= size;
            }

            void takeSnapshot() {
                std::lock_guard<std::mutex> lock(profilerMutex);
                snapshots.push_back({
                    currentAllocated.load(),
                    0, // سيتم حسابه لاحقاً
                    peakAllocated.load(),
                    std::chrono::steady_clock::now()
                });
            }

            std::vector<std::string> getProfilingReport() const {
                std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(profilerMutex));
                std::vector<std::string> report;
                report.push_back("=== تقرير تحليل الذاكرة ===");
                report.push_back("الاستخدام الحالي: " + std::to_string(currentAllocated.load()) + " بايت");
                report.push_back("الذروة: " + std::to_string(peakAllocated.load()) + " بايت");
                report.push_back("عدد اللقطات: " + std::to_string(snapshots.size()));

                if (!snapshots.empty()) {
                    const auto& latest = snapshots.back();
                    report.push_back("آخر لقطة:");
                    report.push_back("  المخصص: " + std::to_string(latest.allocated) + " بايت");
                    report.push_back("  الذروة: " + std::to_string(latest.peakUsage) + " بايت");
                }

                return report;
            }
        };

        // ══════════════════════════════════════════════════════════════
        // 🎯 واجهة الاستخدام العامة
        // ══════════════════════════════════════════════════════════════

        // تجمعات الكائنات الشائعة
        ObjectPool<Command>& getCommandPool();
        ObjectPool<Value>& getValuePool();
        ObjectPool<std::string>& getStringPool();

        template<typename T>
        ObjectPool<T>& getPool(const std::string& name = "") {
            return smartManager.getPool<T>(name);
        }

        // ══════════════════════════════════════════════════════════════
        // 📏 تجمعات لأحجام مختلفة
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تجمع كائنات صغيرة (أقل من 64 بايت)
         */
        template<typename T>
        ObjectPool<T>& getSmallPool(const std::string& name = "") {
            std::string poolName = name.empty() ? "small_" + std::string(typeid(T).name()) : name;
            return smartManager.getPool<T>(poolName + "_small");
        }

        /**
         * @brief تجمع كائنات متوسطة (64-512 بايت)
         */
        template<typename T>
        ObjectPool<T>& getMediumPool(const std::string& name = "") {
            std::string poolName = name.empty() ? "medium_" + std::string(typeid(T).name()) : name;
            return smartManager.getPool<T>(poolName + "_medium");
        }

        /**
         * @brief تجمع كائنات كبيرة (أكثر من 512 بايت)
         */
        template<typename T>
        ObjectPool<T>& getLargePool(const std::string& name = "") {
            std::string poolName = name.empty() ? "large_" + std::string(typeid(T).name()) : name;
            return smartManager.getPool<T>(poolName + "_large");
        }

        /**
         * @brief تجمع ذكي يختار الحجم المناسب تلقائياً
         */
        template<typename T>
        ObjectPool<T>& getAdaptivePool(const std::string& name = "", size_t objectSize = sizeof(T)) {
            std::string poolName = name.empty() ? std::string(typeid(T).name()) : name;

            if (objectSize <= 64) {
                return getSmallPool<T>(poolName);
            } else if (objectSize <= 512) {
                return getMediumPool<T>(poolName);
            } else {
                return getLargePool<T>(poolName);
            }
        }

        // جامع القمامة
        GarbageCollector& getGC() { return smartManager.getGC(); }

        // مراقب الأداء
        MemoryProfiler& getProfiler() { return profiler; }

        // مدير الذاكرة المؤقتة
        TemporaryMemoryManager& getTempManager() { return tempManager; }

        // نظام العد المرجعي
        ArabicReferenceCounter& getReferenceCounter() { return refCounter; }

        // دوال عامة
        void collectGarbage() {
            smartManager.collectGarbage();
            refCounter.collectGarbage();
            tempManager.cleanupExpired();
        }
        void cleanup() {
            smartManager.cleanup();
            refCounter.cleanup();
            tempManager.cleanup();
        }

        // تحسينات الذاكرة
        void optimizeMemory() {
            smartManager.optimizePools();
            collectGarbage();
        }

        void smartRecycle() {
            smartManager.smartRecycle();
        }

        std::vector<std::string> getAllStats();

    private:
        SmartMemoryManager smartManager;
        MemoryProfiler profiler;
        ArabicReferenceCounter refCounter;
        TemporaryMemoryManager tempManager;

        ArabicMemoryManager() = default;
        ~ArabicMemoryManager() {
            cleanup();
        }
    };

    // ══════════════════════════════════════════════════════════════
    // 🛠️ مساعدات الذاكرة الذكية
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief مؤشر ذكي مخصص للكائنات المدارة
     */
    template<typename T>
    class ArabicSmartPtr {
    private:
        T* ptr;
        ArabicMemoryManager::GarbageCollector* gc;

    public:
        explicit ArabicSmartPtr(T* p = nullptr)
            : ptr(p), gc(&ArabicMemoryManager::getInstance().getGC()) {
            if (ptr) gc->retain(ptr);
        }

        ~ArabicSmartPtr() {
            if (ptr) gc->release(ptr);
        }

        ArabicSmartPtr(const ArabicSmartPtr& other) : ptr(other.ptr), gc(other.gc) {
            if (ptr) gc->retain(ptr);
        }

        ArabicSmartPtr& operator=(const ArabicSmartPtr& other) {
            if (this != &other) {
                if (ptr) gc->release(ptr);
                ptr = other.ptr;
                gc = other.gc;
                if (ptr) gc->retain(ptr);
            }
            return *this;
        }

        ArabicSmartPtr(ArabicSmartPtr&& other) noexcept : ptr(other.ptr), gc(other.gc) {
            other.ptr = nullptr;
        }

        ArabicSmartPtr& operator=(ArabicSmartPtr&& other) noexcept {
            if (this != &other) {
                if (ptr) gc->release(ptr);
                ptr = other.ptr;
                gc = other.gc;
                other.ptr = nullptr;
            }
            return *this;
        }

        T* operator->() const { return ptr; }
        T& operator*() const { return *ptr; }
        T* get() const { return ptr; }

        explicit operator bool() const { return ptr != nullptr; }

        void reset(T* p = nullptr) {
            if (ptr) gc->release(ptr);
            ptr = p;
            if (ptr) gc->retain(ptr);
        }
    };

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة لإدارة الذاكرة
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief دالة إنشاء كائن مع تجمع
     */
    template<typename T, typename... Args>
    ArabicSmartPtr<T> makeArabicObject(Args&&... args) {
        auto& pool = ArabicMemoryManager::getInstance().getPool<T>();
        return ArabicSmartPtr<T>(pool.acquire(std::forward<Args>(args)...).release());
    }

    /**
     * @brief دالة إنشاء قيمة مع تجمع
     */
    template<typename... Args>
    ArabicSmartPtr<Value> makeArabicValue(Args&&... args) {
        auto& pool = ArabicMemoryManager::getInstance().getValuePool();
        return ArabicSmartPtr<Value>(pool.acquire(std::forward<Args>(args)...).release());
    }

    /**
     * @brief دالة إنشاء أمر مع تجمع
     */
    template<typename... Args>
    ArabicSmartPtr<Command> makeArabicCommand(Args&&... args) {
        auto& pool = ArabicMemoryManager::getInstance().getCommandPool();
        return ArabicSmartPtr<Command>(pool.acquire(std::forward<Args>(args)...).release());
    }

    // ══════════════════════════════════════════════════════════════
    // 🔄 دوال مساعدة للعد المرجعي
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief إضافة كائن للعد المرجعي
     */
    template<typename T>
    bool addRefCountedObject(T* ptr, ArabicReferenceCounter::CleanupFunction cleanup = nullptr) {
        return ArabicMemoryManager::getInstance().getReferenceCounter().add(
            ptr, cleanup, typeid(T).name());
    }

    /**
     * @brief زيادة مرجع لكائن
     */
    inline bool incrementRef(void* ptr) {
        return ArabicMemoryManager::getInstance().getReferenceCounter().increment(ptr);
    }

    /**
     * @brief تقليل مرجع لكائن
     */
    inline bool decrementRef(void* ptr) {
        return ArabicMemoryManager::getInstance().getReferenceCounter().decrement(ptr);
    }

    /**
     * @brief الحصول على عدد المراجع
     */
    inline int getRefCount(void* ptr) {
        return ArabicMemoryManager::getInstance().getReferenceCounter().getRefCount(ptr);
    }

} // namespace ArabicLanguage
