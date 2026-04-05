// ArabicNLP.h - مكتبة معالجة اللغة الطبيعية العربية
// الأسبوع الثاني من الشهر السادس: معالجة اللغة الطبيعية
#ifndef ARABIC_NLP_H
#define ARABIC_NLP_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <algorithm>
#include <sstream>
#include <regex>
#include <cmath>

namespace ArabicLanguage {

// ==================== أنواع البيانات الأساسية ====================

/**
 * @brief نتيجة تحليل المشاعر
 */
enum class Sentiment {
    POSITIVE,   // إيجابي
    NEGATIVE,   // سلبي
    NEUTRAL     // محايد
};

/**
 * @brief نتيجة تحليل النص
 */
struct TextAnalysisResult {
    std::string originalText;
    std::vector<std::string> tokens;
    std::vector<std::string> normalizedTokens;
    std::map<std::string, int> wordFrequency;
    int wordCount;
    int sentenceCount;
    double averageWordLength;
    Sentiment sentiment;
    double sentimentScore; // -1.0 إلى 1.0
    std::map<std::string, double> categoryScores; // تصنيفات مختلفة
};

/**
 * @brief فئة نصية
 */
struct TextCategory {
    std::string name;
    std::vector<std::string> keywords;
    double weight;
};

// ==================== Tokenization ====================

/**
 * @brief محلل النصوص (Tokenizer)
 */
class ArabicTokenizer {
public:
    ArabicTokenizer();
    ~ArabicTokenizer() = default;

    // تقسيم النص إلى كلمات
    std::vector<std::string> tokenize(const std::string& text);
    
    // تقسيم النص إلى جمل
    std::vector<std::string> sentenceTokenize(const std::string& text);
    
    // تقسيم النص إلى أحرف
    std::vector<std::string> characterTokenize(const std::string& text);
    
    // إزالة علامات الترقيم
    std::string removePunctuation(const std::string& text);
    
    // تنظيف النص
    std::string cleanText(const std::string& text);

private:
    std::vector<std::string> arabicPunctuation;
    std::vector<std::string> arabicDiacritics;
    void initializeArabicPatterns();
};

// ==================== Normalization ====================

/**
 * @brief تطبيع النصوص العربية
 */
class ArabicNormalizer {
public:
    ArabicNormalizer();
    ~ArabicNormalizer() = default;

    // إزالة التشكيل
    std::string removeDiacritics(const std::string& text);
    
    // توحيد الألفات
    std::string normalizeAlef(const std::string& text);
    
    // توحيد التاء المربوطة
    std::string normalizeTaa(const std::string& text);
    
    // تطبيع كامل
    std::string normalize(const std::string& text);
    
    // تحويل الأرقام إلى نص
    std::string normalizeNumbers(const std::string& text);

private:
    std::map<std::string, std::string> alefVariations;
    std::map<std::string, std::string> taaVariations;
    std::vector<std::string> arabicDiacritics;
    void initializeNormalizationMaps();
};

// ==================== تحليل المشاعر ====================

/**
 * @brief محلل المشاعر
 */
class SentimentAnalyzer {
public:
    SentimentAnalyzer();
    ~SentimentAnalyzer() = default;

    // تحليل المشاعر
    Sentiment analyzeSentiment(const std::string& text);
    
    // حساب درجة المشاعر (-1.0 إلى 1.0)
    double calculateSentimentScore(const std::string& text);
    
    // تحليل تفصيلي
    struct SentimentDetails {
        Sentiment overall;
        double positiveScore;
        double negativeScore;
        double neutralScore;
        std::vector<std::string> positiveWords;
        std::vector<std::string> negativeWords;
    };
    
    SentimentDetails analyzeDetailed(const std::string& text);

private:
    std::vector<std::string> positiveWords;
    std::vector<std::string> negativeWords;
    std::vector<std::string> neutralWords;
    std::map<std::string, double> wordSentimentScores;
    void initializeSentimentLexicon();
    double calculateWordSentiment(const std::string& word);
};

// ==================== تصنيف النصوص ====================

/**
 * @brief مصنف النصوص
 */
class TextClassifier {
public:
    TextClassifier();
    ~TextClassifier() = default;

    // إضافة فئة جديدة
    void addCategory(const TextCategory& category);
    
    // تصنيف النص
    std::string classify(const std::string& text);
    
    // تصنيف مع درجات
    std::map<std::string, double> classifyWithScores(const std::string& text);
    
    // تدريب المصنف (بسيط)
    void train(const std::vector<std::pair<std::string, std::string>>& trainingData);
    
    // حفظ/تحميل النموذج
    bool saveModel(const std::string& filename);
    bool loadModel(const std::string& filename);

private:
    std::vector<TextCategory> categories;
    std::map<std::string, std::map<std::string, int>> categoryWordCounts;
    std::map<std::string, int> categoryDocumentCounts;
    double calculateCategoryScore(const std::string& text, const TextCategory& category);
};

// ==================== تحليل النصوص الشامل ====================

/**
 * @brief محلل النصوص الشامل
 */
class ArabicTextAnalyzer {
public:
    ArabicTextAnalyzer();
    ~ArabicTextAnalyzer() = default;

    // تحليل شامل للنص
    TextAnalysisResult analyze(const std::string& text);
    
    // إحصائيات النص
    struct TextStatistics {
        int totalWords;
        int totalSentences;
        int totalCharacters;
        int uniqueWords;
        double averageWordLength;
        double averageSentenceLength;
        std::map<std::string, int> wordFrequency;
        std::map<std::string, int> characterFrequency;
    };
    
    TextStatistics getStatistics(const std::string& text);
    
    // استخراج الكلمات المفتاحية
    std::vector<std::string> extractKeywords(const std::string& text, int count = 10);
    
    // حساب التشابه بين نصين
    double calculateSimilarity(const std::string& text1, const std::string& text2);
    
    // تلخيص النص (بسيط)
    std::string summarize(const std::string& text, int maxSentences = 3);

private:
    std::unique_ptr<ArabicTokenizer> tokenizer;
    std::unique_ptr<ArabicNormalizer> normalizer;
    std::unique_ptr<SentimentAnalyzer> sentimentAnalyzer;
    std::unique_ptr<TextClassifier> classifier;
    
    std::vector<std::string> stopWords;
    void initializeStopWords();
    bool isStopWord(const std::string& word);
};

// ==================== أدوات مساعدة ====================

namespace NLPUtils {
    // حساب TF-IDF
    double calculateTF(const std::string& term, const std::vector<std::string>& document);
    double calculateIDF(const std::string& term, const std::vector<std::vector<std::string>>& documents);
    double calculateTFIDF(const std::string& term, const std::vector<std::string>& document,
                        const std::vector<std::vector<std::string>>& documents);
    
    // حساب تشابه جيب التمام
    double cosineSimilarity(const std::map<std::string, double>& vector1,
                           const std::map<std::string, double>& vector2);
    
    // حساب مسافة Levenshtein
    int levenshteinDistance(const std::string& s1, const std::string& s2);
    
    // تحويل النص إلى متجه (vectorization)
    std::map<std::string, double> textToVector(const std::string& text,
                                               const std::vector<std::string>& vocabulary);
    
    // استخراج n-grams
    std::vector<std::string> extractNGrams(const std::vector<std::string>& tokens, int n);
}

} // namespace ArabicLanguage

#endif // ARABIC_NLP_H

