#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <numeric>
#include <chrono>

#ifdef __AVX2__
#include <immintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#include "ArabicTypes.h"
#include "ArabicMemoryManager.h"

namespace ArabicLanguage {

/**
 * @brief مكتبة مصفوفات محسّنة مع دعم SIMD
 *
 * توفر عمليات متجهية محسّنة للمعالجات الحديثة
 * مع إدارة ذاكرة متقدمة وخوارزميات محسنة.
 */
class ArabicArraySIMD {
public:
    /**
     * @brief نتيجة العمليات المتجهية
     */
    struct SIMDResult {
        double scalar_result;
        std::vector<double> vector_result;
        bool success;
        std::string error_message;

        SIMDResult() : scalar_result(0.0), success(true) {}
        SIMDResult(double val) : scalar_result(val), success(true) {}
        SIMDResult(const std::string& error) : scalar_result(0.0), success(false), error_message(error) {}
    };

    /**
     * @brief إحصائيات الأداء
     */
    struct PerformanceStats {
        size_t operations_count;
        std::chrono::nanoseconds total_time;
        size_t memory_allocated;
        bool used_simd;

        PerformanceStats() : operations_count(0), total_time(0), memory_allocated(0), used_simd(false) {}

        double avg_operation_time_ns() const {
            return operations_count > 0 ?
                   static_cast<double>(total_time.count()) / operations_count : 0.0;
        }
    };

private:
    PerformanceStats stats;
    ArabicMemoryManager::ObjectPool<Value>* valuePool;

    // دوال SIMD الخاصة
#ifdef __AVX2__
    SIMDResult add_simd_avx2(const std::vector<double>& a, const std::vector<double>& b);
    SIMDResult multiply_simd_avx2(const std::vector<double>& a, const std::vector<double>& b);
    SIMDResult sum_simd_avx2(const std::vector<double>& data);
    double dot_product_simd_avx2(const std::vector<double>& a, const std::vector<double>& b);
#endif

#ifdef __ARM_NEON
    SIMDResult add_simd_neon(const std::vector<double>& a, const std::vector<double>& b);
    SIMDResult multiply_simd_neon(const std::vector<double>& a, const std::vector<double>& b);
#endif

    // دوال احتياطية (fallback) للمعالجات القديمة
    SIMDResult add_fallback(const std::vector<double>& a, const std::vector<double>& b);
    SIMDResult multiply_fallback(const std::vector<double>& a, const std::vector<double>& b);
    SIMDResult sum_fallback(const std::vector<double>& data);
    double dot_product_fallback(const std::vector<double>& a, const std::vector<double>& b);

    // دوال مساعدة للكشف عن إمكانيات SIMD
    bool has_avx2_support() const;
    bool has_neon_support() const;

    // دوال مساعدة للذاكرة
    void* allocate_aligned(size_t size, size_t alignment = 32);
    void deallocate_aligned(void* ptr);

    // دوال مساعدة للإحصائيات
    void record_operation(std::chrono::nanoseconds duration, size_t memory_used = 0, bool used_simd = false);

public:
    ArabicArraySIMD();
    ~ArabicArraySIMD();

    // منع النسخ والتعيين
    ArabicArraySIMD(const ArabicArraySIMD&) = delete;
    ArabicArraySIMD& operator=(const ArabicArraySIMD&) = delete;

    /**
     * @brief جمع مصفوفتين مع SIMD
     */
    SIMDResult add_arrays(const std::vector<double>& a, const std::vector<double>& b);

    /**
     * @brief ضرب مصفوفتين مع SIMD
     */
    SIMDResult multiply_arrays(const std::vector<double>& a, const std::vector<double>& b);

    /**
     * @brief حساب مجموع عناصر المصفوفة مع SIMD
     */
    SIMDResult sum_array(const std::vector<double>& data);

    /**
     * @brief حساب المتوسط مع SIMD
     */
    SIMDResult average_array(const std::vector<double>& data);

    /**
     * @brief البحث عن القيمة القصوى مع SIMD
     */
    SIMDResult max_array(const std::vector<double>& data);

    /**
     * @brief البحث عن القيمة الصغرى مع SIMD
     */
    SIMDResult min_array(const std::vector<double>& data);

    /**
     * @brief حساب الجداء النقطي (dot product) مع SIMD
     */
    SIMDResult dot_product(const std::vector<double>& a, const std::vector<double>& b);

    /**
     * @brief تطبيق دالة على جميع عناصر المصفوفة
     */
    SIMDResult apply_function(const std::vector<double>& data,
                             std::function<double(double)> func);

    /**
     * @brief ترتيب المصفوفة باستخدام خوارزميات محسنة
     */
    SIMDResult sort_array(std::vector<double>& data);

    /**
     * @brief بحث ثنائي محسن
     */
    SIMDResult binary_search(const std::vector<double>& data, double target);

    /**
     * @brief إنشاء مصفوفة باستخدام إدارة الذاكرة المحسنة
     */
    Value create_optimized_array(const std::vector<double>& data);

    /**
     * @brief نسخ مصفوفة مع تحسينات الذاكرة
     */
    Value copy_optimized_array(const Value& source);

    /**
     * @brief الحصول على إحصائيات الأداء
     */
    const PerformanceStats& get_performance_stats() const { return stats; }

    /**
     * @brief إعادة تعيين الإحصائيات
     */
    void reset_stats() { stats = PerformanceStats(); }

    /**
     * @brief معلومات عن إمكانيات SIMD المتاحة
     */
    std::vector<std::string> get_simd_capabilities() const;

    /**
     * @brief تشخيص مشاكل SIMD
     */
    std::vector<std::string> diagnose_simd_support() const;
};

/**
 * @brief دوال مساعدة للكشف عن SIMD
 */
namespace SIMDHelpers {
    bool detect_avx2();
    bool detect_avx512();
    bool detect_neon();
    bool detect_sse4_1();

    std::string get_cpu_vendor();
    std::vector<std::string> get_supported_simd_extensions();
}

/**
 * @brief دوال محسّنة للعمليات الشائعة
 */
namespace OptimizedArrayOps {
    /**
     * @brief جمع سريع مع SIMD
     */
    double fast_sum(const std::vector<double>& data);

    /**
     * @brief بحث خطي محسن
     */
    size_t fast_linear_search(const std::vector<double>& data, double target);

    /**
     * @brief فرز محسن (QuickSort مع SIMD)
     */
    void fast_sort(std::vector<double>& data);

    /**
     * @brief إزالة التكرارات بكفاءة
     */
    std::vector<double> remove_duplicates(const std::vector<double>& data);
}

} // namespace ArabicLanguage
