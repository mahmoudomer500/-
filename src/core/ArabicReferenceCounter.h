#pragma once

#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>
#include <string>

namespace ArabicLanguage {

    /**
     * @brief نظام عد مرجعي بسيط وفعال لتحسين إدارة الذاكرة
     *
     * هذا النظام يوفر عد مرجعي سريع وآمن للخيوط للكائنات المختلفة
     * في المترجم العربي.
     */
    class ArabicReferenceCounter {
    public:
        // نوع الدالة لتحرير الموارد
        using CleanupFunction = std::function<void(void*)>;

        /**
         * @brief بنية تمثل كائن مرجعي
         */
        struct RefObject {
            void* ptr;                    // المؤشر للكائن
            std::atomic<int> refCount;   // عدد المراجع
            CleanupFunction cleanup;     // دالة التنظيف
            std::string typeName;        // اسم النوع للتصحيح
            bool pooled;                 // هل الكائن من تجمع؟

            RefObject(void* p, CleanupFunction c, const std::string& type, bool isPooled = false)
                : ptr(p), refCount(1), cleanup(c), typeName(type), pooled(isPooled) {}

            // منع النسخ
            RefObject(const RefObject&) = delete;
            RefObject& operator=(const RefObject&) = delete;
        };

    private:
        // خريطة الكائنات المرجعية
        std::unordered_map<void*, std::unique_ptr<RefObject>> objects;

        // إحصائيات الأداء
        std::atomic<size_t> totalCreated{0};
        std::atomic<size_t> totalDestroyed{0};
        std::atomic<size_t> currentObjects{0};
        std::atomic<size_t> peakObjects{0};

        // قفل للحماية من الوصول المتزامن
        mutable std::mutex counterMutex;

        /**
         * @brief تحديث ذروة عدد الكائنات
         */
        void updatePeak() {
            size_t current = currentObjects.load();
            size_t peak = peakObjects.load();
            while (current > peak && !peakObjects.compare_exchange_weak(peak, current)) {}
        }

    public:
        ArabicReferenceCounter() = default;
        ~ArabicReferenceCounter() {
            cleanup();
        }

        // منع النسخ والتعيين
        ArabicReferenceCounter(const ArabicReferenceCounter&) = delete;
        ArabicReferenceCounter& operator=(const ArabicReferenceCounter&) = delete;

        /**
         * @brief إضافة كائن جديد للعد المرجعي
         * @param ptr المؤشر للكائن
         * @param cleanup دالة التنظيف (اختيارية)
         * @param typeName اسم النوع للتصحيح
         * @param pooled هل الكائن من تجمع؟
         * @return true إذا تم إضافة الكائن بنجاح
         */
        bool add(void* ptr, CleanupFunction cleanup = nullptr,
                const std::string& typeName = "unknown", bool pooled = false) {
            if (!ptr) return false;

            std::lock_guard<std::mutex> lock(counterMutex);

            // تحقق من عدم وجود الكائن مسبقاً
            if (objects.find(ptr) != objects.end()) {
                return false; // الكائن موجود بالفعل
            }

            // إضافة الكائن الجديد
            objects[ptr] = std::make_unique<RefObject>(ptr, cleanup, typeName, pooled);

            // تحديث الإحصائيات
            totalCreated++;
            currentObjects++;
            updatePeak();

            return true;
        }

        /**
         * @brief زيادة عدد المراجع لكائن
         * @param ptr المؤشر للكائن
         * @return true إذا تم العثور على الكائن
         */
        bool increment(void* ptr) {
            if (!ptr) return false;

            std::lock_guard<std::mutex> lock(counterMutex);
            auto it = objects.find(ptr);
            if (it != objects.end()) {
                it->second->refCount++;
                return true;
            }
            return false;
        }

        /**
         * @brief تقليل عدد المراجع لكائن
         * @param ptr المؤشر للكائن
         * @return true إذا تم تدمير الكائن (عدد المراجع = 0)
         */
        bool decrement(void* ptr) {
            if (!ptr) return false;

            std::lock_guard<std::mutex> lock(counterMutex);
            auto it = objects.find(ptr);
            if (it != objects.end()) {
                if (--it->second->refCount <= 0) {
                    // تنظيف الكائن
                    if (it->second->cleanup) {
                        it->second->cleanup(ptr);
                    }

                    // إزالة من الخريطة
                    objects.erase(it);

                    // تحديث الإحصائيات
                    totalDestroyed++;
                    currentObjects--;

                    return true; // تم تدمير الكائن
                }
                return false; // لا يزال هناك مراجع
            }
            return false; // لم يتم العثور على الكائن
        }

        /**
         * @brief الحصول على عدد المراجع الحالي لكائن
         * @param ptr المؤشر للكائن
         * @return عدد المراجع أو -1 إذا لم يتم العثور على الكائن
         */
        int getRefCount(void* ptr) const {
            if (!ptr) return -1;

            std::lock_guard<std::mutex> lock(counterMutex);
            auto it = objects.find(ptr);
            return (it != objects.end()) ? it->second->refCount.load() : -1;
        }

        /**
         * @brief التحقق من وجود كائن في النظام
         * @param ptr المؤشر للكائن
         * @return true إذا كان الكائن موجوداً
         */
        bool hasObject(void* ptr) const {
            if (!ptr) return false;

            std::lock_guard<std::mutex> lock(counterMutex);
            return objects.find(ptr) != objects.end();
        }

        /**
         * @brief تنظيف جميع الكائنات المتبقية
         */
        void cleanup() {
            std::lock_guard<std::mutex> lock(counterMutex);

            for (auto& pair : objects) {
                if (pair.second->cleanup) {
                    pair.second->cleanup(pair.first);
                }
                totalDestroyed++;
            }

            objects.clear();
            currentObjects = 0;
        }

        /**
         * @brief جمع القمامة - إزالة الكائنات ذات المراجع الصفرية
         * @return عدد الكائنات التي تم تدميرها
         */
        size_t collectGarbage() {
            std::lock_guard<std::mutex> lock(counterMutex);
            size_t collected = 0;

            for (auto it = objects.begin(); it != objects.end(); ) {
                if (it->second->refCount <= 0) {
                    if (it->second->cleanup) {
                        it->second->cleanup(it->first);
                    }
                    it = objects.erase(it);
                    collected++;
                    totalDestroyed++;
                    currentObjects--;
                } else {
                    ++it;
                }
            }

            return collected;
        }

        /**
         * @brief الحصول على إحصائيات النظام
         * @return خريطة بالإحصائيات
         */
        std::unordered_map<std::string, size_t> getStats() const {
            std::lock_guard<std::mutex> lock(counterMutex);

            return {
                {"total_created", totalCreated.load()},
                {"total_destroyed", totalDestroyed.load()},
                {"current_objects", currentObjects.load()},
                {"peak_objects", peakObjects.load()},
                {"active_objects", objects.size()}
            };
        }

        /**
         * @brief الحصول على تقرير مفصل عن الكائنات
         * @return قائمة بالمعلومات
         */
        std::vector<std::string> getDetailedReport() const {
            std::lock_guard<std::mutex> lock(counterMutex);
            std::vector<std::string> report;

            report.push_back("=== تقرير نظام العد المرجعي ===");
            report.push_back("الكائنات النشطة: " + std::to_string(objects.size()));
            report.push_back("إجمالي المنشأ: " + std::to_string(totalCreated.load()));
            report.push_back("إجمالي المدمر: " + std::to_string(totalDestroyed.load()));
            report.push_back("الذروة: " + std::to_string(peakObjects.load()));

            if (!objects.empty()) {
                report.push_back("");
                report.push_back("تفاصيل الكائنات:");
                for (const auto& pair : objects) {
                    const auto& obj = pair.second;
                    std::string status = obj->pooled ? "[تجمع]" : "[عادي]";
                    report.push_back("  " + obj->typeName + " " + status +
                                   " - مراجع: " + std::to_string(obj->refCount.load()));
                }
            }

            return report;
        }

        /**
         * @brief الحصول على عدد الكائنات الحالية
         */
        size_t getObjectCount() const {
            std::lock_guard<std::mutex> lock(counterMutex);
            return objects.size();
        }

        /**
         * @brief الحصول على الذروة التاريخية
         */
        size_t getPeakCount() const {
            return peakObjects.load();
        }
    };

} // namespace ArabicLanguage
