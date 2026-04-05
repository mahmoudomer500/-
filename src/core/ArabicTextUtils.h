#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <functional>
#include <codecvt>
#include <locale>
#include <algorithm>    // ✅ إضافة جديد لـ std::max

namespace ArabicLanguage {

/**
 * @brief مكتبة معالجة النصوص العربية المحسنة
 *
 * توفر خوارزميات بحث واستبدال متقدمة مع دعم كامل لـ Unicode
 * وتحسينات أداء للنصوص الكبيرة.
 */
class ArabicTextUtils {
public:
    /**
     * @brief نتيجة عملية البحث
     */
    struct SearchResult {
        size_t position;
        bool found;
        std::chrono::nanoseconds timeTaken;

        SearchResult() : position(0), found(false), timeTaken(0) {}
        SearchResult(size_t pos, bool f, std::chrono::nanoseconds t)
            : position(pos), found(f), timeTaken(t) {}
    };

    /**
     * @brief إحصائيات الأداء
     */
    struct PerformanceStats {
        size_t operationsCount;
        std::chrono::nanoseconds totalTime;
        size_t memoryUsed;
        double avgTimePerOperation;

        PerformanceStats() : operationsCount(0), totalTime(0), memoryUsed(0), avgTimePerOperation(0.0) {}
    };

private:
    mutable PerformanceStats stats;

    void recordOperation(std::chrono::nanoseconds time, size_t memory = 0, bool isSearch = false) const;

    /**
     * @brief String Builder محسّن للعمليات الكبيرة
     */
    class EfficientStringBuilder {
    private:
        std::vector<char> buffer;
        size_t capacity;
        size_t currentSize;

        void ensureCapacity(size_t additionalSize) {
            if (currentSize + additionalSize > capacity) {
                size_t needed = currentSize + additionalSize;
                capacity = (capacity * 2 > needed) ? (capacity * 2) : needed;
                buffer.resize(capacity);
            }
        }

    public:
        EfficientStringBuilder(size_t initialCapacity = 1024)
            : capacity(initialCapacity), currentSize(0) {
            buffer.resize(capacity);
        }

        void append(const std::string& str) {
            ensureCapacity(str.length());
            memcpy(&buffer[currentSize], str.c_str(), str.length());
            currentSize += str.length();
        }

        void append(char c) {
            ensureCapacity(1);
            buffer[currentSize++] = c;
        }

        void replace(size_t pos, size_t len, const std::string& replacement) {
            if (pos + len > currentSize) return;

            size_t newLen = replacement.length();
            size_t sizeDiff = newLen - len;

            if (sizeDiff > 0) {
                ensureCapacity(sizeDiff);
            }

            // نقل الجزء المتبقي
            memmove(&buffer[pos + newLen], &buffer[pos + len],
                   currentSize - (pos + len));

            // نسخ النص الجديد
            memcpy(&buffer[pos], replacement.c_str(), newLen);

            currentSize += sizeDiff;
        }

        std::string toString() const {
            return std::string(buffer.data(), currentSize);
        }

        void clear() {
            currentSize = 0;
        }

        size_t size() const { return currentSize; }
        size_t getCapacity() const { return capacity; }
    };

public:
    ArabicTextUtils();
    ~ArabicTextUtils() = default;

    // منع النسخ والتعيين
    ArabicTextUtils(const ArabicTextUtils&) = delete;
    ArabicTextUtils& operator=(const ArabicTextUtils&) = delete;

    /**
     * @brief بحث باستخدام خوارزمية KMP
     * O(n + m) preprocessing, O(n) search
     */
    SearchResult KMPSearch(const std::string& text, const std::string& pattern);

    /**
     * @brief بحث باستخدام خوارزمية Boyer-Moore
     * O(m + σ) preprocessing, O(n) average case
     */
    SearchResult BoyerMooreSearch(const std::string& text, const std::string& pattern);

    /**
     * @brief بحث ذكي - يختار البيرونية الأنسب تلقائياً
     */
    SearchResult SmartSearch(const std::string& text, const std::string& pattern);

    /**
     * @brief استبدال محسّن باستخدام String Builder
     */
    std::string EfficientReplace(const std::string& text,
                               const std::string& oldStr,
                               const std::string& newStr);

    /**
     * @brief استبدال متعدد المراحل للأداء الأمثل
     */
    std::string MultiPassReplace(const std::string& text,
                               const std::vector<std::pair<std::string, std::string>>& replacements);

    /**
     * @brief عد مرات الظهور باستخدام البحث المتقدم
     */
    size_t CountOccurrences(const std::string& text, const std::string& pattern);

    /**
     * @brief عكس النص مع دعم Unicode
     */
    std::string UnicodeReverse(const std::string& text);

    /**
     * @brief تطبيع النص العربي (إزالة التشكيل الاختياري)
     */
    std::string NormalizeArabic(const std::string& text, bool removeDiacritics = false);

    /**
     * @brief التحقق من احتواء النص على أحرف عربية
     */
    bool ContainsArabic(const std::string& text) const;

    /**
     * @brief استخراج الكلمات من النص
     */
    static std::vector<std::string> ExtractWords(const std::string& text);

    /**
     * @brief حساب مسافة Levenshtein بين نصين
     */
    size_t LevenshteinDistance(const std::string& s1, const std::string& s2);

    /**
     * @brief البحث الضبابي (Fuzzy Search)
     */
    std::vector<SearchResult> FuzzySearch(const std::string& text,
                                        const std::string& pattern,
                                        size_t maxDistance = 2);

    /**
     * @brief ضغط النص البسيط (RLE)
     */
    std::string CompressText(const std::string& text);

    /**
     * @brief فك ضغط النص
     */
    std::string DecompressText(const std::string& compressed);

    /**
     * @brief الحصول على إحصائيات الأداء
     */
    const PerformanceStats& getPerformanceStats() const { return stats; }

    /**
     * @brief إعادة تعيين الإحصائيات
     */
    void resetStats() { stats = PerformanceStats(); }

    /**
     * @brief معلومات عن البيرونيات المدعومة
     */
    std::vector<std::string> getSupportedAlgorithms() const;

    /**
     * @brief تشخيص مشاكل Unicode
     */
    std::vector<std::string> diagnoseUnicodeSupport(const std::string& text) const;

    /**
     * @brief تحويل من UTF-8 إلى UTF-32
     */
    std::u32string utf8ToUtf32(const std::string& utf8) const;

    /**
     * @brief تحويل من UTF-32 إلى UTF-8
     */
    std::string utf32ToUtf8(const std::u32string& utf32) const;

    /**
     * @brief عد الأحرف الحقيقية (ليس البايتات)
     */
    size_t utf8Length(const std::string& utf8) const;

    /**
     * @brief استخراج حرف UTF-8 في موضع معين
     */
    std::string utf8CharAt(const std::string& utf8, size_t position) const;

    /**
     * @brief استخراج جزء من نص UTF-8
     */
    std::string utf8Substring(const std::string& utf8, size_t start, size_t length) const;

    /**
     * @brief عكس النص مع الحفاظ على اتجاه الكلمات
     */
    std::string reversePreservingWordOrder(const std::string& text) const;

    /**
     * @brief تحويل الأرقام إلى كلمات عربية
     */
    std::string numberToWords(long long number) const;
};

/**
 * @brief دوال مساعدة لخوارزميات البحث
 */
namespace SearchAlgorithms {

    /**
     * @brief حساب مصفوفة LPS لخوارزمية KMP
     */
    void computeLPSArray(const std::string& pattern, std::vector<int>& lps);

    /**
     * @brief حساب جدول الأحرف السيئة لخوارزمية Boyer-Moore
     */
    void computeBadCharTable(const std::string& pattern, std::vector<int>& badChar);

    /**
     * @brief حساب جدول السوابق الجيدة لخوارزمية Boyer-Moore
     */
    void computeGoodSuffixTable(const std::string& pattern, std::vector<int>& goodSuffix);

    /**
     * @brief البحث الخطي البسيط (fallback)
     */
    size_t naiveSearch(const std::string& text, const std::string& pattern);

}

/**
 * @brief دوال مساعدة للنصوص العربية
 */
namespace ArabicTextHelpers {

    /**
     * @brief قائمة حروف التشكيل العربي
     */
    extern const std::string ARABIC_DIACRITICS;

    /**
     * @brief تحويل من UTF-8 إلى UTF-32
     */
    std::u32string utf8ToUtf32(const std::string& utf8);

    /**
     * @brief تحويل من UTF-32 إلى UTF-8
     */
    std::string utf32ToUtf8(const std::u32string& utf32);

    /**
     * @brief عد الأحرف الحقيقية (ليس البايتات)
     */
    size_t utf8Length(const std::string& utf8);

    /**
     * @brief استخراج حرف UTF-8 في موضع معين
     */
    std::string utf8CharAt(const std::string& utf8, size_t position);

    /**
     * @brief استخراج جزء من نص UTF-8
     */
    std::string utf8Substring(const std::string& utf8, size_t start, size_t length);

    /**
     * @brief التحقق من أن الحرف عربي
     */
    bool isArabicChar(char32_t c);

    /**
     * @brief إزالة التشكيل من النص العربي
     */
    std::string removeDiacritics(const std::string& text);

    /**
     * @brief إزالة التشكيل (مرادف)
     */
    std::string stripDiacritics(const std::string& text);

    /**
     * @brief تطبيع الألف والتاء المربوطة
     */
    std::string normalizeHamza(const std::string& text);

    /**
     * @brief تحويل الأرقام إلى كلمات عربية
     */
    std::string numberToWords(long long number);

    /**
     * @brief عكس النص مع الحفاظ على اتجاه الكلمات
     */
    std::string reversePreservingWordOrder(const std::string& text);

} // namespace ArabicTextHelpers

} // namespace ArabicLanguage
