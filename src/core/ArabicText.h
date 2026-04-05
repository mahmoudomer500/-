#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <cstdint>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include "SafeWindows.h"

namespace ArabicLanguage {

/**
 * @brief مكتبة النصوص العربية المحسّنة - Arabic Text Library Enhanced
 *
 * مكتبة شاملة لمعالجة النصوص العربية مع دعم كامل لـ RTL
 * وتخطيط من اليمين لليسار والخطوط العربية
 */
class ArabicText {
public:
    /**
     * @brief اتجاه النص
     */
    enum TextDirection {
        LEFT_TO_RIGHT,
        RIGHT_TO_LEFT,
        AUTO_DETECT
    };

    /**
     * @brief محاذاة النص
     */
    enum TextAlignment {
        ALIGN_LEFT,
        ALIGN_CENTER,
        ALIGN_RIGHT,
        ALIGN_JUSTIFY
    };

    /**
     * @brief خصائص الخط
     */
    struct FontProperties {
        std::string fontName;
        float size;
        bool bold;
        bool italic;
        bool underline;
        uint32_t color; // ARGB format

        FontProperties() : size(12.0f), bold(false), italic(false),
                          underline(false), color(0xFF000000) {}
    };

    /**
     * @brief مقاييس النص
     */
    struct TextMetrics {
        float width;
        float height;
        float ascent;
        float descent;
        size_t glyphCount;

        TextMetrics() : width(0.0f), height(0.0f), ascent(0.0f),
                       descent(0.0f), glyphCount(0) {}
    };

    /**
     * @brief موقع حرف في النص
     */
    struct GlyphPosition {
        size_t index;      // Position in original text
        float x, y;        // Screen coordinates
        float width;       // Glyph width
        uint32_t codepoint; // Unicode codepoint

        GlyphPosition() : index(0), x(0.0f), y(0.0f), width(0.0f), codepoint(0) {}
    };

    /**
     * @brief سطر نص منسق
     */
    struct TextLine {
        std::string text;
        std::vector<GlyphPosition> glyphs;
        float width;
        float height;
        TextDirection direction;

        TextLine() : width(0.0f), height(0.0f), direction(LEFT_TO_RIGHT) {}
    };

    /**
     * @brief فقرة نص منسقة
     */
    struct TextParagraph {
        std::vector<TextLine> lines;
        TextMetrics metrics;
        TextDirection overallDirection;
        float maxWidth;

        TextParagraph() : overallDirection(LEFT_TO_RIGHT), maxWidth(0.0f) {}
    };

private:
    // Font management
    std::unordered_map<std::string, HFONT> fontCache;
    std::unordered_map<std::string, TEXTMETRIC> fontMetricsCache;

    // Current device context for text measurement
    HDC hdc;

    // Text processing settings
    bool enableBidi;        // Bidirectional text support
    bool enableShaping;     // Arabic text shaping
    bool enableLigatures;   // Ligature support

public:
    ArabicText();
    ~ArabicText();

    // منع النسخ والتعيين
    ArabicText(const ArabicText&) = delete;
    ArabicText& operator=(const ArabicText&) = delete;

    /**
     * @brief تهيئة مكتبة النصوص
     */
    bool initialize(HDC deviceContext = nullptr);

    /**
     * @brief إنهاء مكتبة النصوص
     */
    void shutdown();

    /**
     * @brief تحميل خط
     */
    bool loadFont(const std::string& fontName, const FontProperties& properties);

    /**
     * @brief الحصول على مقاييس النص
     */
    TextMetrics measureText(const std::string& text, const FontProperties& font);

    /**
     * @brief تقسيم النص إلى أسطر
     */
    TextParagraph layoutText(const std::string& text, const FontProperties& font,
                           float maxWidth, TextDirection direction = AUTO_DETECT);

    /**
     * @brief عكس النص للعرض RTL
     */
    std::string reverseForDisplay(const std::string& text);

    /**
     * @brief تشكيل النص العربي
     */
    std::string shapeArabicText(const std::string& text);

    /**
     * @brief كشف اتجاه النص
     */
    TextDirection detectTextDirection(const std::string& text);

    /**
     * @brief فصل النص إلى كلمات
     */
    std::vector<std::string> splitIntoWords(const std::string& text);

    /**
     * @brief فصل النص إلى جمل
     */
    std::vector<std::string> splitIntoSentences(const std::string& text);

    /**
     * @brief البحث في النص العربي
     */
    std::vector<size_t> findInArabicText(const std::string& text, const std::string& searchTerm);

    /**
     * @brief استبدال في النص العربي
     */
    std::string replaceInArabicText(const std::string& text, const std::string& oldStr,
                                  const std::string& newStr);

    /**
     * @brief تطبيع النص العربي
     */
    std::string normalizeArabicText(const std::string& text);

    /**
     * @brief تحويل من UTF-8 إلى UTF-32
     */
    std::u32string utf8ToUtf32(const std::string& utf8);

    /**
     * @brief تحويل من UTF-32 إلى UTF-8
     */
    std::string utf32ToUtf8(const std::u32string& utf32);

    /**
     * @brief إزالة التشكيل
     */
    std::string removeDiacritics(const std::string& text);

    /**
     * @brief إزالة التشكيل مع الحفاظ على الهمزة
     */
    std::string removeDiacriticsKeepHamza(const std::string& text);

    /**
     * @brief عد الكلمات في النص العربي
     */
    size_t countWords(const std::string& text);

    /**
     * @brief عد الأحرف في النص العربي (مع/بدون التشكيل)
     */
    size_t countCharacters(const std::string& text, bool includeDiacritics = false);

    /**
     * @brief استخراج كلمات مفتاحية
     */
    std::vector<std::string> extractKeywords(const std::string& text, size_t maxKeywords = 10);

    /**
     * @brief تحليل المشاعر الأساسي
     */
    enum Sentiment { POSITIVE, NEGATIVE, NEUTRAL };
    Sentiment analyzeSentiment(const std::string& text);

    /**
     * @brief تصحيح إملائي بسيط
     */
    std::string spellCheck(const std::string& text);

    /**
     * @brief تحويل الأرقام إلى كلمات عربية
     */
    std::string numberToWords(long long number);

    /**
     * @brief تحويل التاريخ إلى كلمات عربية
     */
    std::string dateToWords(int day, int month, int year);

    /**
     * @brief دعم Unicode العربي
     */
    bool isArabicChar(char32_t codepoint);
    bool isArabicDiacritic(char32_t codepoint);
    bool isArabicLetter(char32_t codepoint);
    bool isArabicNumber(char32_t codepoint);

    /**
     * @brief تحويل بين أنظمة التشفير
     */
    std::string utf8ToCp1256(const std::string& utf8);
    std::string cp1256ToUtf8(const std::string& cp1256);

    /**
     * @brief دعم الخطوط العربية
     */
    std::vector<std::string> getAvailableArabicFonts();
    bool isArabicFontAvailable(const std::string& fontName);

    /**
     * @brief إعدادات التخطيط
     */
    void setBidiEnabled(bool enabled) { enableBidi = enabled; }
    void setShapingEnabled(bool enabled) { enableShaping = enabled; }
    void setLigaturesEnabled(bool enabled) { enableLigatures = enabled; }

    /**
     * @brief الحصول على إعدادات النظام
     */
    bool isBidiEnabled() const { return enableBidi; }
    bool isShapingEnabled() const { return enableShaping; }
    bool isLigaturesEnabled() const { return enableLigatures; }

    /**
     * @brief تنظيف الخطوط المحملة
     */
    void clearFontCache();

    /**
     * @brief الحصول على معلومات الخط
     */
    TEXTMETRIC getFontMetrics(const std::string& fontKey);

    /**
     * @brief إنشاء مفتاح خط فريد
     */
    static std::string createFontKey(const FontProperties& props);
};

// ────────────────────────────────────────────────────────
// معالج النصوص العربية
// ────────────────────────────────────────────────────────

class ArabicTextProcessor {
private:
    std::shared_ptr<ArabicText> textEngine;

    // قواميس للمعالجة
    std::unordered_map<std::string, std::string> arabicRoots;
    std::unordered_map<std::string, std::string> commonCorrections;
    std::unordered_set<std::string> arabicStopWords;

public:
    ArabicTextProcessor(std::shared_ptr<ArabicText> engine);

    /**
     * @brief استخراج الجذور العربية
     */
    std::string extractRoot(const std::string& word);

    /**
     * @brief تحليل الصرف
     */
    struct MorphologicalAnalysis {
        std::string root;
        std::string pattern;
        std::string pos; // Part of speech
        std::vector<std::string> prefixes;
        std::vector<std::string> suffixes;

        MorphologicalAnalysis() = default;
    };

    MorphologicalAnalysis analyzeMorphology(const std::string& word);

    /**
     * @brief توليد النصوص العربية
     */
    std::string generateArabicText(const std::string& pattern, size_t length = 100);

    /**
     * @brief ضغط النصوص العربية
     */
    std::string compressArabicText(const std::string& text);

    /**
     * @brief فك ضغط النصوص العربية
     */
    std::string decompressArabicText(const std::string& compressed);

    /**
     * @brief تشفير النصوص العربية
     */
    std::string encryptArabicText(const std::string& text, const std::string& key);

    /**
     * @brief فك تشفير النصوص العربية
     */
    std::string decryptArabicText(const std::string& encrypted, const std::string& key);
};

// ────────────────────────────────────────────────────────
// محرر النصوص العربي
// ────────────────────────────────────────────────────────

class ArabicTextEditor {
public:
    /**
     * @brief تحديد نص
     */
    struct Selection {
        size_t start;
        size_t end;

        Selection() : start(0), end(0) {}
        bool isEmpty() const { return start == end; }
        size_t length() const { return end - start; }
    };

    Selection selection;

private:
    std::shared_ptr<ArabicText> textEngine;
    std::string content;
    size_t cursorPosition;
    ArabicText::FontProperties currentFont;
    ArabicText::TextDirection textDirection;

    // Undo/Redo system
    std::vector<std::string> undoStack;
    std::vector<std::string> redoStack;
    size_t maxUndoSteps;

public:
    ArabicTextEditor(std::shared_ptr<ArabicText> engine, size_t maxUndo = 100);

    /**
     * @brief إدراج نص
     */
    void insertText(const std::string& text, size_t position = std::string::npos);

    /**
     * @brief حذف نص
     */
    void deleteText(size_t start, size_t length);

    /**
     * @brief استبدال نص
     */
    void replaceText(size_t start, size_t length, const std::string& replacement);

    /**
     * @brief قص النص المحدد
     */
    std::string cut();

    /**
     * @brief نسخ النص المحدد
     */
    std::string copy() const;

    /**
     * @brief لصق نص
     */
    void paste(const std::string& text);

    /**
     * @brief البحث والاستبدال
     */
    std::vector<size_t> find(const std::string& searchTerm);
    void replaceAll(const std::string& oldStr, const std::string& newStr);

    /**
     * @brief التنقل في النص
     */
    void moveCursor(int delta);
    void setCursorPosition(size_t position);
    size_t getCursorPosition() const { return cursorPosition; }

    /**
     * @brief Undo/Redo
     */
    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }
    void undo();
    void redo();

    /**
     * @brief إدارة الخط
     */
    void setFont(const ArabicText::FontProperties& font) { currentFont = font; }
    const ArabicText::FontProperties& getFont() const { return currentFont; }

    /**
     * @brief إدارة اتجاه النص
     */
    void setTextDirection(ArabicText::TextDirection direction) { textDirection = direction; }
    ArabicText::TextDirection getTextDirection() const { return textDirection; }

    /**
     * @brief الحصول على النص
     */
    const std::string& getContent() const { return content; }
    void setContent(const std::string& text) { content = text; }

    /**
     * @brief إحصائيات النص
     */
    struct TextStats {
        size_t characterCount;
        size_t wordCount;
        size_t lineCount;
        size_t arabicCharCount;
        std::unordered_map<std::string, size_t> wordFrequency;

        TextStats() : characterCount(0), wordCount(0), lineCount(0), arabicCharCount(0) {}
    };

    TextStats getStats() const;

private:
    /**
     * @brief حفظ حالة للـ undo
     */
    void saveState();

    /**
     * @brief التحقق من صحة الموضع
     */
    size_t clampPosition(size_t position) const;
};

// ────────────────────────────────────────────────────────
// دوال مساعدة للنصوص العربية
// ────────────────────────────────────────────────────────

namespace ArabicTextHelpers {

    /**
     * @brief قائمة حروف التشكيل العربي
     */
    const std::u32string ARABIC_DIACRITICS_U32 =
        U"\u064B\u064C\u064D\u064E\u064F\u0650\u0651\u0652\u0653\u0654\u0655";

    /**
     * @brief قائمة الحروف العربية الأساسية
     */
    const std::u32string ARABIC_LETTERS_U32 =
        U"\u0621\u0622\u0623\u0624\u0625\u0626\u0627\u0628\u0629\u062A\u062B\u062C"
        "\u062D\u062E\u062F\u0630\u0631\u0632\u0633\u0634\u0635\u0636\u0637\u0638"
        "\u0639\u063A\u0640\u0641\u0642\u0643\u0644\u0645\u0646\u0647\u0648\u0649"
        "\u064A";

    /**
     * @brief الأرقام العربية
     */
    const std::u32string ARABIC_NUMBERS_U32 = U"\u0660\u0661\u0662\u0663\u0664\u0665\u0666\u0667\u0668\u0669";

    /**
     * @brief كلمات القاموس العربي الأساسية
     */
    const std::unordered_set<std::string> BASIC_ARABIC_WORDS = {
        "من", "إلى", "على", "في", "مع", "هو", "هي", "هم", "هن", "أنا", "نحن",
        "أنت", "أنتِ", "أنتم", "أنتن", "هذا", "هذه", "هؤلاء", "ذلك", "تلك",
        "كل", "بعض", "كثير", "قليل", "جديد", "قديم", "كبير", "صغير", "طويل", "قصير"
    };

    /**
     * @brief كشف الحرف العربي
     */
    bool isArabicCharacter(char32_t codepoint);

    /**
     * @brief كشف حرف التشكيل
     */
    bool isDiacritic(char32_t codepoint);

    /**
     * @brief كشف الحرف الأساسي
     */
    bool isArabicLetter(char32_t codepoint);

    /**
     * @brief كشف الرقم العربي
     */
    bool isArabicNumber(char32_t codepoint);

    /**
     * @brief الحصول على نوع الحرف العربي
     */
    enum ArabicCharType {
        ISOLATED,
        INITIAL,
        MEDIAL,
        FINAL
    };

    ArabicCharType getArabicCharType(char32_t current, char32_t prev = 0, char32_t next = 0);

    /**
     * @brief تشكيل الحرف العربي
     */
    char32_t shapeArabicChar(char32_t codepoint, ArabicCharType type);

    /**
     * @brief عكس النص للعرض
     */
    std::string reverseForRTLDisplay(const std::string& text);

    /**
     * @brief فصل النص العربي إلى كلمات
     */
    std::vector<std::string> tokenizeArabicText(const std::string& text);

    /**
     * @brief تطبيع النص العربي
     */
    std::string normalizeArabicText(const std::string& text);

    /**
     * @brief تحويل من UTF-8 إلى UTF-32
     */
    std::u32string utf8ToUtf32(const std::string& utf8);

    /**
     * @brief تحويل من UTF-32 إلى UTF-8
     */
    std::string utf32ToUtf8(const std::u32string& utf32);

    /**
     * @brief إزالة التشكيل
     */
    std::string removeDiacritics(const std::string& text);

    /**
     * @brief تحويل الأرقام الغربية إلى عربية
     */
    std::string westernToArabicNumerals(const std::string& text);

    /**
     * @brief تحويل الأرقام العربية إلى غربية
     */
    std::string arabicToWesternNumerals(const std::string& text);

    /**
     * @brief الحصول على اسم الشهر العربي
     */
    std::string getArabicMonthName(int month);

    /**
     * @brief الحصول على اسم اليوم العربي
     */
    std::string getArabicDayName(int dayOfWeek);

    /**
     * @brief تحويل الوقت إلى كلمات عربية
     */
    std::string timeToArabicWords(int hour, int minute);

};

} // namespace ArabicLanguage
