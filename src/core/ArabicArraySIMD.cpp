#include "ArabicArraySIMD.h"
#include <iostream>
#include <cstring>
#include <cmath>
#ifdef _WIN32
#include <intrin.h>
#ifdef __GNUC__
#include <cpuid.h>
static inline void native_cpuid(int cpu_info[4], int leaf) {
    __get_cpuid(leaf, (unsigned int*)&cpu_info[0], (unsigned int*)&cpu_info[1], (unsigned int*)&cpu_info[2], (unsigned int*)&cpu_info[3]);
}
#else
static inline void native_cpuid(int cpu_info[4], int leaf) {
    __cpuid(cpu_info, leaf);
}
#endif
#else
#include <cpuid.h>
static inline void native_cpuid(int cpu_info[4], int leaf) {
    __get_cpuid(leaf, (unsigned int*)&cpu_info[0], (unsigned int*)&cpu_info[1], (unsigned int*)&cpu_info[2], (unsigned int*)&cpu_info[3]);
}
#endif

namespace ArabicLanguage {

ArabicArraySIMD::ArabicArraySIMD() {
    // تهيئة تجمع القيم
    valuePool = &ArabicMemoryManager::getInstance().getValuePool();
}

ArabicArraySIMD::~ArabicArraySIMD() {
    // تنظيف تلقائي
}

bool ArabicArraySIMD::has_avx2_support() const {
    return SIMDHelpers::detect_avx2();
}

bool ArabicArraySIMD::has_neon_support() const {
    return SIMDHelpers::detect_neon();
}

void* ArabicArraySIMD::allocate_aligned(size_t size, size_t alignment) {
    void* ptr = nullptr;
#ifdef _WIN32
    ptr = _aligned_malloc(size, alignment);
#else
    if (posix_memalign(&ptr, alignment, size) != 0) {
        ptr = nullptr;
    }
#endif
    return ptr;
}

void ArabicArraySIMD::deallocate_aligned(void* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

void ArabicArraySIMD::record_operation(std::chrono::nanoseconds duration,
                                      size_t memory_used,
                                      bool used_simd) {
    stats.operations_count++;
    stats.total_time += duration;
    stats.memory_allocated += memory_used;
    if (used_simd) stats.used_simd = true;
}

#ifdef __AVX2__

ArabicArraySIMD::SIMDResult ArabicArraySIMD::add_simd_avx2(const std::vector<double>& a,
                                                          const std::vector<double>& b) {
    auto start_time = std::chrono::steady_clock::now();

    if (a.size() != b.size()) {
        return SIMDResult("Array sizes don't match");
    }

    size_t size = a.size();
    std::vector<double> result(size);

    // معالجة العناصر في مجموعات من 4 (AVX2 يدعم 256-bit = 4 doubles)
    size_t i = 0;
    for (; i + 3 < size; i += 4) {
        __m256d va = _mm256_loadu_pd(&a[i]);
        __m256d vb = _mm256_loadu_pd(&b[i]);
        __m256d vresult = _mm256_add_pd(va, vb);
        _mm256_storeu_pd(&result[i], vresult);
    }

    // معالجة العناصر المتبقية
    for (; i < size; ++i) {
        result[i] = a[i] + b[i];
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, size * sizeof(double), true);

    SIMDResult res;
    res.vector_result = std::move(result);
    res.success = true;
    return res;
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::multiply_simd_avx2(const std::vector<double>& a,
                                                               const std::vector<double>& b) {
    auto start_time = std::chrono::steady_clock::now();

    if (a.size() != b.size()) {
        return SIMDResult("Array sizes don't match");
    }

    size_t size = a.size();
    std::vector<double> result(size);

    size_t i = 0;
    for (; i + 3 < size; i += 4) {
        __m256d va = _mm256_loadu_pd(&a[i]);
        __m256d vb = _mm256_loadu_pd(&b[i]);
        __m256d vresult = _mm256_mul_pd(va, vb);
        _mm256_storeu_pd(&result[i], vresult);
    }

    for (; i < size; ++i) {
        result[i] = a[i] * b[i];
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, size * sizeof(double), true);

    SIMDResult res;
    res.vector_result = std::move(result);
    res.success = true;
    return res;
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::sum_simd_avx2(const std::vector<double>& data) {
    auto start_time = std::chrono::steady_clock::now();

    size_t size = data.size();
    __m256d sum_vec = _mm256_setzero_pd();
    double scalar_sum = 0.0;

    size_t i = 0;
    for (; i + 3 < size; i += 4) {
        __m256d vec = _mm256_loadu_pd(&data[i]);
        sum_vec = _mm256_add_pd(sum_vec, vec);
    }

    // تجميع النتيجة
    double temp[4];
    _mm256_storeu_pd(temp, sum_vec);
    scalar_sum = temp[0] + temp[1] + temp[2] + temp[3];

    // إضافة العناصر المتبقية
    for (; i < size; ++i) {
        scalar_sum += data[i];
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, 0, true);

    return SIMDResult(scalar_sum);
}

double ArabicArraySIMD::dot_product_simd_avx2(const std::vector<double>& a,
                                             const std::vector<double>& b) {
    if (a.size() != b.size()) return 0.0;

    size_t size = a.size();
    __m256d dot_vec = _mm256_setzero_pd();

    size_t i = 0;
    for (; i + 3 < size; i += 4) {
        __m256d va = _mm256_loadu_pd(&a[i]);
        __m256d vb = _mm256_loadu_pd(&b[i]);
        __m256d prod = _mm256_mul_pd(va, vb);
        dot_vec = _mm256_add_pd(dot_vec, prod);
    }

    double temp[4];
    _mm256_storeu_pd(temp, dot_vec);
    double result = temp[0] + temp[1] + temp[2] + temp[3];

    for (; i < size; ++i) {
        result += a[i] * b[i];
    }

    return result;
}

#endif // __AVX2__

#ifdef __ARM_NEON

ArabicArraySIMD::SIMDResult ArabicArraySIMD::add_simd_neon(const std::vector<double>& a,
                                                          const std::vector<double>& b) {
    // تطبيق NEON للمعالجات ARM
    // (مبسط للتوضيح)
    return add_fallback(a, b);
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::multiply_simd_neon(const std::vector<double>& a,
                                                               const std::vector<double>& b) {
    // تطبيق NEON للمعالجات ARM
    return multiply_fallback(a, b);
}

#endif // __ARM_NEON

// دوال احتياطية للمعالجات غير المدعومة
ArabicArraySIMD::SIMDResult ArabicArraySIMD::add_fallback(const std::vector<double>& a,
                                                         const std::vector<double>& b) {
    auto start_time = std::chrono::steady_clock::now();

    if (a.size() != b.size()) {
        return SIMDResult("Array sizes don't match");
    }

    size_t size = a.size();
    std::vector<double> result(size);

    for (size_t i = 0; i < size; ++i) {
        result[i] = a[i] + b[i];
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, size * sizeof(double), false);

    SIMDResult res;
    res.vector_result = std::move(result);
    res.success = true;
    return res;
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::multiply_fallback(const std::vector<double>& a,
                                                              const std::vector<double>& b) {
    auto start_time = std::chrono::steady_clock::now();

    if (a.size() != b.size()) {
        return SIMDResult("Array sizes don't match");
    }

    size_t size = a.size();
    std::vector<double> result(size);

    for (size_t i = 0; i < size; ++i) {
        result[i] = a[i] * b[i];
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, size * sizeof(double), false);

    SIMDResult res;
    res.vector_result = std::move(result);
    res.success = true;
    return res;
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::sum_fallback(const std::vector<double>& data) {
    auto start_time = std::chrono::steady_clock::now();

    double result = 0.0;
    for (double val : data) {
        result += val;
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, 0, false);

    return SIMDResult(result);
}

double ArabicArraySIMD::dot_product_fallback(const std::vector<double>& a,
                                            const std::vector<double>& b) {
    if (a.size() != b.size()) return 0.0;

    double result = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        result += a[i] * b[i];
    }
    return result;
}

// تطبيق الواجهة العامة
ArabicArraySIMD::SIMDResult ArabicArraySIMD::add_arrays(const std::vector<double>& a,
                                                       const std::vector<double>& b) {
#ifdef __AVX2__
    if (has_avx2_support()) {
        return add_simd_avx2(a, b);
    }
#endif

#ifdef __ARM_NEON
    if (has_neon_support()) {
        return add_simd_neon(a, b);
    }
#endif

    return add_fallback(a, b);
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::multiply_arrays(const std::vector<double>& a,
                                                            const std::vector<double>& b) {
#ifdef __AVX2__
    if (has_avx2_support()) {
        return multiply_simd_avx2(a, b);
    }
#endif

#ifdef __ARM_NEON
    if (has_neon_support()) {
        return multiply_simd_neon(a, b);
    }
#endif

    return multiply_fallback(a, b);
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::sum_array(const std::vector<double>& data) {
    if (data.empty()) return SIMDResult(0.0);

#ifdef __AVX2__
    if (has_avx2_support()) {
        return sum_simd_avx2(data);
    }
#endif

    return sum_fallback(data);
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::average_array(const std::vector<double>& data) {
    if (data.empty()) return SIMDResult(0.0);

    SIMDResult sum_result = sum_array(data);
    if (!sum_result.success) return sum_result;

    double average = sum_result.scalar_result / data.size();
    return SIMDResult(average);
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::max_array(const std::vector<double>& data) {
    auto start_time = std::chrono::steady_clock::now();

    if (data.empty()) return SIMDResult("Empty array");

    double max_val = *std::max_element(data.begin(), data.end());

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, 0, false);

    return SIMDResult(max_val);
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::min_array(const std::vector<double>& data) {
    auto start_time = std::chrono::steady_clock::now();

    if (data.empty()) return SIMDResult("Empty array");

    double min_val = *std::min_element(data.begin(), data.end());

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, 0, false);

    return SIMDResult(min_val);
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::dot_product(const std::vector<double>& a,
                                                        const std::vector<double>& b) {
    auto start_time = std::chrono::steady_clock::now();

    if (a.size() != b.size()) {
        return SIMDResult("Array sizes don't match");
    }

    double result = 0.0;

#ifdef __AVX2__
    if (has_avx2_support()) {
        result = dot_product_simd_avx2(a, b);
    } else
#endif
    {
        result = dot_product_fallback(a, b);
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, 0, true);

    return SIMDResult(result);
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::apply_function(const std::vector<double>& data,
                                                           std::function<double(double)> func) {
    auto start_time = std::chrono::steady_clock::now();

    std::vector<double> result;
    result.reserve(data.size());

    for (double val : data) {
        result.push_back(func(val));
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, data.size() * sizeof(double), false);

    SIMDResult res;
    res.vector_result = std::move(result);
    res.success = true;
    return res;
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::sort_array(std::vector<double>& data) {
    auto start_time = std::chrono::steady_clock::now();

    OptimizedArrayOps::fast_sort(data);

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, 0, false);

    SIMDResult res;
    res.vector_result = data;
    res.success = true;
    return res;
}

ArabicArraySIMD::SIMDResult ArabicArraySIMD::binary_search(const std::vector<double>& data,
                                                          double target) {
    auto start_time = std::chrono::steady_clock::now();

    // افترض أن المصفوفة مرتبة
    auto it = std::lower_bound(data.begin(), data.end(), target);
    size_t index = (it != data.end() && *it == target) ?
                   std::distance(data.begin(), it) : static_cast<size_t>(-1);

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    record_operation(duration, 0, false);

    return SIMDResult(static_cast<double>(index));
}

Value ArabicArraySIMD::create_optimized_array(const std::vector<double>& data) {
    if (!valuePool) {
        return Value(ValueType::NONE);
    }

    auto array_value = valuePool->acquire();
    array_value->type = ValueType::ARRAY;
    array_value->elements.reserve(data.size());

    for (double val : data) {
        array_value->elements.push_back(Value(ValueType::NUMBER, std::to_string(val)));
    }

    return *array_value;
}

Value ArabicArraySIMD::copy_optimized_array(const Value& source) {
    if (source.type != ValueType::ARRAY || !valuePool) {
        return Value(ValueType::NONE);
    }

    auto copy_value = valuePool->acquire();
    copy_value->type = ValueType::ARRAY;
    copy_value->elements = source.elements;

    return *copy_value;
}

std::vector<std::string> ArabicArraySIMD::get_simd_capabilities() const {
    std::vector<std::string> caps;

    caps.push_back("=== إمكانيات SIMD المتاحة ===");

#ifdef __AVX2__
    caps.push_back("AVX2: " + std::string(has_avx2_support() ? "مدعوم" : "غير مدعوم"));
#endif

#ifdef __ARM_NEON
    caps.push_back("NEON: " + std::string(has_neon_support() ? "مدعوم" : "غير مدعوم"));
#endif

    caps.push_back("معالج: " + SIMDHelpers::get_cpu_vendor());

    auto extensions = SIMDHelpers::get_supported_simd_extensions();
    caps.push_back("امتدادات SIMD:");
    for (const auto& ext : extensions) {
        caps.push_back("  - " + ext);
    }

    return caps;
}

std::vector<std::string> ArabicArraySIMD::diagnose_simd_support() const {
    std::vector<std::string> diagnosis;

    diagnosis.push_back("=== تشخيص دعم SIMD ===");

    bool has_simd = false;

#ifdef __AVX2__
    bool avx2 = has_avx2_support();
    diagnosis.push_back("AVX2: " + std::string(avx2 ? "✅ مدعوم" : "❌ غير مدعوم"));
    if (avx2) has_simd = true;
#endif

#ifdef __ARM_NEON
    bool neon = has_neon_support();
    diagnosis.push_back("NEON: " + std::string(neon ? "✅ مدعوم" : "❌ غير مدعوم"));
    if (neon) has_simd = true;
#endif

    if (!has_simd) {
        diagnosis.push_back("⚠️  لا يوجد دعم SIMD - سيتم استخدام الكود العادي");
        diagnosis.push_back("💡 للحصول على أداء أفضل، استخدم معالج يدعم AVX2 أو NEON");
    } else {
        diagnosis.push_back("✅ SIMD مدعوم - الأداء محسّن");
    }

    return diagnosis;
}

// تطبيق دوال SIMDHelpers
namespace SIMDHelpers {

bool detect_avx2() {
#ifdef __AVX2__
    // فحص فعلي للدعم في runtime
    int cpu_info[4];
    native_cpuid(cpu_info, 7);
    return (cpu_info[1] & (1 << 5)) != 0; // AVX2 bit
#else
    return false;
#endif
}

bool detect_avx512() {
#ifdef __AVX512F__
    int cpu_info[4];
    native_cpuid(cpu_info, 7);
    return (cpu_info[1] & (1 << 16)) != 0; // AVX512F bit
#else
    return false;
#endif
}

bool detect_neon() {
#ifdef __ARM_NEON
    return true;
#else
    return false;
#endif
}

bool detect_sse4_1() {
#ifdef __SSE4_1__
    int cpu_info[4];
    native_cpuid(cpu_info, 1);
    return (cpu_info[2] & (1 << 19)) != 0; // SSE4.1 bit
#else
    return false;
#endif
}

std::string get_cpu_vendor() {
    int cpu_info[4];
    native_cpuid(cpu_info, 0);

    char vendor[13];
    memcpy(vendor, &cpu_info[1], 4);
    memcpy(vendor + 4, &cpu_info[3], 4);
    memcpy(vendor + 8, &cpu_info[2], 4);
    vendor[12] = '\0';

    return std::string(vendor);
}

std::vector<std::string> get_supported_simd_extensions() {
    std::vector<std::string> extensions;

    if (detect_avx2()) extensions.push_back("AVX2");
    if (detect_avx512()) extensions.push_back("AVX512");
    if (detect_neon()) extensions.push_back("NEON");
    if (detect_sse4_1()) extensions.push_back("SSE4.1");

    if (extensions.empty()) {
        extensions.push_back("لا توجد امتدادات SIMD مدعومة");
    }

    return extensions;
}

}

// تطبيق دوال OptimizedArrayOps
namespace OptimizedArrayOps {

double fast_sum(const std::vector<double>& data) {
    // استخدام SIMD إذا أمكن
    ArabicArraySIMD simd;
    auto result = simd.sum_array(data);
    return result.success ? result.scalar_result : 0.0;
}

size_t fast_linear_search(const std::vector<double>& data, double target) {
    // بحث خطي محسن مع SIMD hints
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i] == target) {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

void fast_sort(std::vector<double>& data) {
    // QuickSort محسّن
    if (data.size() <= 1) return;

    using Iterator = std::vector<double>::iterator;

    auto partition = [&](Iterator begin, Iterator end) {
        auto pivot = *begin;
        auto i = begin + 1;
        auto j = end - 1;

        while (i <= j) {
            while (i <= j && *i <= pivot) ++i;
            while (i <= j && *j > pivot) --j;
            if (i < j) std::swap(*i, *j);
        }
        std::swap(*begin, *j);
        return j;
    };

    std::function<void(Iterator, Iterator)> quicksort = [&](Iterator begin, Iterator end) {
        if (begin < end) {
            auto pivot = partition(begin, end);
            quicksort(begin, pivot);
            quicksort(pivot + 1, end);
        }
    };

    quicksort(data.begin(), data.end());
}

std::vector<double> remove_duplicates(const std::vector<double>& data) {
    std::vector<double> result = data;
    auto last = std::unique(result.begin(), result.end());
    result.erase(last, result.end());
    return result;
}

}

} // namespace ArabicLanguage
