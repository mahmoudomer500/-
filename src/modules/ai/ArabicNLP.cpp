// ArabicNLP.cpp - تطبيق مكتبة معالجة اللغة الطبيعية العربية
// الأسبوع الثاني من الشهر السادس: معالجة اللغة الطبيعية
#include "ArabicNLP.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <set>

namespace ArabicLanguage {

// ==================== ArabicTokenizer Implementation ====================

ArabicTokenizer::ArabicTokenizer() {
    initializeArabicPatterns();
}

void ArabicTokenizer::initializeArabicPatterns() {
    // علامات الترقيم العربية
    arabicPunctuation = {
        "،", ".", "؛", ":", "؟", "!", "(", ")", "[", "]", "{", "}",
        "\"", "'", "«", "»", "…", "–", "—"
    };
    
    // التشكيل العربي
    arabicDiacritics = {
        "\u064B", "\u064C", "\u064D", "\u064E", "\u064F", "\u0650",
        "\u0651", "\u0652", "\u0653", "\u0654", "\u0655", "\u0656"
    };
}

std::vector<std::string> ArabicTokenizer::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string cleaned = cleanText(text);
    
    // تقسيم بسيط على المسافات
    std::istringstream iss(cleaned);
    std::string token;
    
    while (iss >> token) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::vector<std::string> ArabicTokenizer::sentenceTokenize(const std::string& text) {
    std::vector<std::string> sentences;
    std::string cleaned = cleanText(text);
    
    // تقسيم على علامات نهاية الجمل
    std::string currentSentence;
    for (size_t i = 0; i < cleaned.length(); ++i) {
        char c = cleaned[i];
        currentSentence += c;
        
        // التحقق من علامات نهاية الجمل (بما في ذلك علامة الاستفهام العربية)
        bool isSentenceEnd = (c == '.' || c == '!' || c == ';' || c == '\n');
        
        // التحقق من علامة الاستفهام العربية (UTF-8: D9 8F)
        if (i + 1 < cleaned.length() && 
            static_cast<unsigned char>(cleaned[i]) == 0xD9 && 
            static_cast<unsigned char>(cleaned[i + 1]) == 0x8F) {
            isSentenceEnd = true;
            if (i + 1 < cleaned.length()) {
                currentSentence += cleaned[++i]; // إضافة البايت الثاني
            }
        }
        
        if (isSentenceEnd) {
            if (!currentSentence.empty()) {
                std::string trimmed = currentSentence;
                trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
                trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
                if (!trimmed.empty()) {
                    sentences.push_back(trimmed);
                }
                currentSentence.clear();
            }
        }
    }
    
    if (!currentSentence.empty()) {
        std::string trimmed = currentSentence;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
        trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
        if (!trimmed.empty()) {
            sentences.push_back(trimmed);
        }
    }
    
    return sentences;
}

std::vector<std::string> ArabicTokenizer::characterTokenize(const std::string& text) {
    std::vector<std::string> characters;
    for (char c : text) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            characters.push_back(std::string(1, c));
        }
    }
    return characters;
}

std::string ArabicTokenizer::removePunctuation(const std::string& text) {
    std::string result = text;
    
    for (const std::string& punct : arabicPunctuation) {
        size_t pos = 0;
        while ((pos = result.find(punct, pos)) != std::string::npos) {
            result.erase(pos, punct.length());
        }
    }
    
    return result;
}

std::string ArabicTokenizer::cleanText(const std::string& text) {
    std::string cleaned = text;
    
    // إزالة المسافات الزائدة
    std::regex multipleSpaces("\\s+");
    cleaned = std::regex_replace(cleaned, multipleSpaces, " ");
    
    // إزالة المسافات في البداية والنهاية
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);
    
    return cleaned;
}

// ==================== ArabicNormalizer Implementation ====================

ArabicNormalizer::ArabicNormalizer() {
    initializeNormalizationMaps();
}

void ArabicNormalizer::initializeNormalizationMaps() {
    // توحيد الألفات
    alefVariations = {
        {"أ", "ا"}, {"إ", "ا"}, {"آ", "ا"}
    };
    
    // توحيد التاء المربوطة
    taaVariations = {
        {"ة", "ه"}
    };
    
    // التشكيل العربي
    arabicDiacritics = {
        "\u064B", "\u064C", "\u064D", "\u064E", "\u064F", "\u0650",
        "\u0651", "\u0652", "\u0653", "\u0654", "\u0655", "\u0656"
    };
}

std::string ArabicNormalizer::removeDiacritics(const std::string& text) {
    std::string result = text;
    
    for (const std::string& diacritic : arabicDiacritics) {
        size_t pos = 0;
        while ((pos = result.find(diacritic, pos)) != std::string::npos) {
            result.erase(pos, diacritic.length());
        }
    }
    
    return result;
}

std::string ArabicNormalizer::normalizeAlef(const std::string& text) {
    std::string result = text;
    
    for (const auto& pair : alefVariations) {
        size_t pos = 0;
        while ((pos = result.find(pair.first, pos)) != std::string::npos) {
            result.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length();
        }
    }
    
    return result;
}

std::string ArabicNormalizer::normalizeTaa(const std::string& text) {
    std::string result = text;
    
    for (const auto& pair : taaVariations) {
        size_t pos = 0;
        while ((pos = result.find(pair.first, pos)) != std::string::npos) {
            result.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length();
        }
    }
    
    return result;
}

std::string ArabicNormalizer::normalize(const std::string& text) {
    std::string normalized = text;
    normalized = removeDiacritics(normalized);
    normalized = normalizeAlef(normalized);
    normalized = normalizeTaa(normalized);
    return normalized;
}

std::string ArabicNormalizer::normalizeNumbers(const std::string& text) {
    std::string result = text;
    
    // تحويل الأرقام العربية إلى إنجليزية (مبسط)
    std::map<std::string, std::string> arabicToEnglish = {
        {"٠", "0"}, {"١", "1"}, {"٢", "2"}, {"٣", "3"}, {"٤", "4"},
        {"٥", "5"}, {"٦", "6"}, {"٧", "7"}, {"٨", "8"}, {"٩", "9"}
    };
    
    for (const auto& pair : arabicToEnglish) {
        size_t pos = 0;
        while ((pos = result.find(pair.first, pos)) != std::string::npos) {
            result.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length();
        }
    }
    
    return result;
}

// ==================== SentimentAnalyzer Implementation ====================

SentimentAnalyzer::SentimentAnalyzer() {
    initializeSentimentLexicon();
}

void SentimentAnalyzer::initializeSentimentLexicon() {
    // كلمات إيجابية
    positiveWords = {
        "جميل", "رائع", "ممتاز", "جيد", "حسن", "مشرف", "مبهر",
        "سعيد", "فرح", "مبتهج", "متفائل", "متحمس", "مشجع",
        "نجح", "فاز", "انتصار", "فوز", "نجاح", "إنجاز"
    };
    
    // كلمات سلبية
    negativeWords = {
        "سيء", "رديء", "مؤسف", "حزين", "محبط", "مخيب",
        "فشل", "خسر", "هزيمة", "خسارة", "فشل", "إخفاق",
        "غاضب", "مستاء", "محبط", "مكتئب", "قلق", "خائف"
    };
    
    // كلمات محايدة
    neutralWords = {
        "هذا", "ذلك", "هذه", "تلك", "كان", "يكون", "كانت"
    };
    
    // درجات المشاعر للكلمات
    wordSentimentScores = {
        {"جميل", 0.8}, {"رائع", 0.9}, {"ممتاز", 1.0}, {"جيد", 0.6},
        {"سيء", -0.6}, {"رديء", -0.8}, {"مؤسف", -0.7}, {"حزين", -0.5},
        {"فشل", -0.9}, {"نجح", 0.9}, {"سعيد", 0.7}, {"غاضب", -0.6}
    };
}

Sentiment SentimentAnalyzer::analyzeSentiment(const std::string& text) {
    double score = calculateSentimentScore(text);
    
    if (score > 0.2) {
        return Sentiment::POSITIVE;
    } else if (score < -0.2) {
        return Sentiment::NEGATIVE;
    } else {
        return Sentiment::NEUTRAL;
    }
}

double SentimentAnalyzer::calculateSentimentScore(const std::string& text) {
    ArabicTokenizer tokenizer;
    std::vector<std::string> tokens = tokenizer.tokenize(text);
    
    double totalScore = 0.0;
    int wordCount = 0;
    
    for (const std::string& token : tokens) {
        double wordScore = calculateWordSentiment(token);
        totalScore += wordScore;
        if (wordScore != 0.0) {
            wordCount++;
        }
    }
    
    if (wordCount > 0) {
        return totalScore / wordCount;
    }
    
    return 0.0;
}

SentimentAnalyzer::SentimentDetails SentimentAnalyzer::analyzeDetailed(const std::string& text) {
    SentimentDetails details;
    
    ArabicTokenizer tokenizer;
    std::vector<std::string> tokens = tokenizer.tokenize(text);
    
    double positiveSum = 0.0;
    double negativeSum = 0.0;
    int positiveCount = 0;
    int negativeCount = 0;
    
    for (const std::string& token : tokens) {
        double score = calculateWordSentiment(token);
        
        if (score > 0) {
            positiveSum += score;
            positiveCount++;
            details.positiveWords.push_back(token);
        } else if (score < 0) {
            negativeSum += score;
            negativeCount++;
            details.negativeWords.push_back(token);
        }
    }
    
    details.positiveScore = positiveCount > 0 ? positiveSum / positiveCount : 0.0;
    details.negativeScore = negativeCount > 0 ? std::abs(negativeSum / negativeCount) : 0.0;
    details.neutralScore = 1.0 - details.positiveScore - details.negativeScore;
    
    double overallScore = calculateSentimentScore(text);
    if (overallScore > 0.2) {
        details.overall = Sentiment::POSITIVE;
    } else if (overallScore < -0.2) {
        details.overall = Sentiment::NEGATIVE;
    } else {
        details.overall = Sentiment::NEUTRAL;
    }
    
    return details;
}

double SentimentAnalyzer::calculateWordSentiment(const std::string& word) {
    // البحث في القاموس
    auto it = wordSentimentScores.find(word);
    if (it != wordSentimentScores.end()) {
        return it->second;
    }
    
    // البحث في القوائم
    if (std::find(positiveWords.begin(), positiveWords.end(), word) != positiveWords.end()) {
        return 0.5;
    }
    
    if (std::find(negativeWords.begin(), negativeWords.end(), word) != negativeWords.end()) {
        return -0.5;
    }
    
    return 0.0;
}

// ==================== TextClassifier Implementation ====================

TextClassifier::TextClassifier() {}

void TextClassifier::addCategory(const TextCategory& category) {
    categories.push_back(category);
}

std::string TextClassifier::classify(const std::string& text) {
    auto scores = classifyWithScores(text);
    
    if (scores.empty()) {
        return "unknown";
    }
    
    // العثور على الفئة بأعلى درجة
    std::string bestCategory = scores.begin()->first;
    double bestScore = scores.begin()->second;
    
    for (const auto& pair : scores) {
        if (pair.second > bestScore) {
            bestScore = pair.second;
            bestCategory = pair.first;
        }
    }
    
    return bestCategory;
}

std::map<std::string, double> TextClassifier::classifyWithScores(const std::string& text) {
    std::map<std::string, double> scores;
    
    ArabicTokenizer tokenizer;
    std::vector<std::string> tokens = tokenizer.tokenize(text);
    
    for (const TextCategory& category : categories) {
        double score = calculateCategoryScore(text, category);
        scores[category.name] = score;
    }
    
    return scores;
}

void TextClassifier::train(const std::vector<std::pair<std::string, std::string>>& trainingData) {
    // تدريب بسيط: حساب تكرار الكلمات لكل فئة
    categoryWordCounts.clear();
    categoryDocumentCounts.clear();
    
    ArabicTokenizer tokenizer;
    
    for (const auto& pair : trainingData) {
        const std::string& text = pair.first;
        const std::string& category = pair.second;
        
        categoryDocumentCounts[category]++;
        
        std::vector<std::string> tokens = tokenizer.tokenize(text);
        for (const std::string& token : tokens) {
            categoryWordCounts[category][token]++;
        }
    }
}

double TextClassifier::calculateCategoryScore(const std::string& text, const TextCategory& category) {
    ArabicTokenizer tokenizer;
    std::vector<std::string> tokens = tokenizer.tokenize(text);
    
    double score = 0.0;
    int matches = 0;
    
    // حساب عدد التطابقات مع الكلمات المفتاحية
    for (const std::string& keyword : category.keywords) {
        for (const std::string& token : tokens) {
            if (token.find(keyword) != std::string::npos || keyword.find(token) != std::string::npos) {
                matches++;
                score += category.weight;
            }
        }
    }
    
    // تطبيع الدرجة
    if (tokens.size() > 0) {
        score = score / tokens.size();
    }
    
    return score;
}

bool TextClassifier::saveModel(const std::string& filename) {
    // تنفيذ بسيط لحفظ النموذج
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // حفظ الفئات
    file << categories.size() << "\n";
    for (const TextCategory& cat : categories) {
        file << cat.name << "\n";
        file << cat.keywords.size() << "\n";
        for (const std::string& keyword : cat.keywords) {
            file << keyword << "\n";
        }
        file << cat.weight << "\n";
    }
    
    file.close();
    return true;
}

bool TextClassifier::loadModel(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    categories.clear();
    int categoryCount;
    file >> categoryCount;
    
    for (int i = 0; i < categoryCount; ++i) {
        TextCategory cat;
        file >> cat.name;
        
        int keywordCount;
        file >> keywordCount;
        for (int j = 0; j < keywordCount; ++j) {
            std::string keyword;
            file >> keyword;
            cat.keywords.push_back(keyword);
        }
        
        file >> cat.weight;
        categories.push_back(cat);
    }
    
    file.close();
    return true;
}

// ==================== ArabicTextAnalyzer Implementation ====================

ArabicTextAnalyzer::ArabicTextAnalyzer() {
    tokenizer = std::make_unique<ArabicTokenizer>();
    normalizer = std::make_unique<ArabicNormalizer>();
    sentimentAnalyzer = std::make_unique<SentimentAnalyzer>();
    classifier = std::make_unique<TextClassifier>();
    initializeStopWords();
}

void ArabicTextAnalyzer::initializeStopWords() {
    stopWords = {
        "في", "من", "إلى", "على", "هذا", "هذه", "ذلك", "تلك",
        "كان", "كانت", "يكون", "تكون", "التي", "الذي", "التي",
        "و", "أو", "لكن", "إذا", "إن", "أن", "لا", "لم", "لن"
    };
}

bool ArabicTextAnalyzer::isStopWord(const std::string& word) {
    return std::find(stopWords.begin(), stopWords.end(), word) != stopWords.end();
}

TextAnalysisResult ArabicTextAnalyzer::analyze(const std::string& text) {
    TextAnalysisResult result;
    result.originalText = text;
    
    // Tokenization
    result.tokens = tokenizer->tokenize(text);
    
    // Normalization
    for (const std::string& token : result.tokens) {
        result.normalizedTokens.push_back(normalizer->normalize(token));
    }
    
    // Word frequency
    for (const std::string& token : result.tokens) {
        result.wordFrequency[token]++;
    }
    
    // Statistics
    result.wordCount = result.tokens.size();
    result.sentenceCount = tokenizer->sentenceTokenize(text).size();
    
    double totalLength = 0.0;
    for (const std::string& token : result.tokens) {
        totalLength += token.length();
    }
    result.averageWordLength = result.wordCount > 0 ? totalLength / result.wordCount : 0.0;
    
    // Sentiment analysis
    result.sentiment = sentimentAnalyzer->analyzeSentiment(text);
    result.sentimentScore = sentimentAnalyzer->calculateSentimentScore(text);
    
    // Classification
    result.categoryScores = classifier->classifyWithScores(text);
    
    return result;
}

ArabicTextAnalyzer::TextStatistics ArabicTextAnalyzer::getStatistics(const std::string& text) {
    TextStatistics stats;
    
    std::vector<std::string> tokens = tokenizer->tokenize(text);
    std::vector<std::string> sentences = tokenizer->sentenceTokenize(text);
    
    stats.totalWords = tokens.size();
    stats.totalSentences = sentences.size();
    stats.totalCharacters = text.length();
    
    // Unique words
    std::set<std::string> uniqueWords(tokens.begin(), tokens.end());
    stats.uniqueWords = uniqueWords.size();
    
    // Average word length
    double totalLength = 0.0;
    for (const std::string& token : tokens) {
        totalLength += token.length();
    }
    stats.averageWordLength = stats.totalWords > 0 ? totalLength / stats.totalWords : 0.0;
    
    // Average sentence length
    stats.averageSentenceLength = stats.totalSentences > 0 ? 
        static_cast<double>(stats.totalWords) / stats.totalSentences : 0.0;
    
    // Word frequency
    for (const std::string& token : tokens) {
        stats.wordFrequency[token]++;
    }
    
    // Character frequency
    for (char c : text) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            stats.characterFrequency[std::string(1, c)]++;
        }
    }
    
    return stats;
}

std::vector<std::string> ArabicTextAnalyzer::extractKeywords(const std::string& text, int count) {
    std::vector<std::string> keywords;
    
    std::vector<std::string> tokens = tokenizer->tokenize(text);
    std::map<std::string, int> wordFreq;
    
    // حساب التكرار (بدون stop words)
    for (const std::string& token : tokens) {
        if (!isStopWord(token)) {
            wordFreq[token]++;
        }
    }
    
    // ترتيب حسب التكرار
    std::vector<std::pair<std::string, int>> sortedWords(wordFreq.begin(), wordFreq.end());
    std::sort(sortedWords.begin(), sortedWords.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // أخذ أعلى الكلمات
    for (size_t i = 0; i < std::min(static_cast<size_t>(count), sortedWords.size()); ++i) {
        keywords.push_back(sortedWords[i].first);
    }
    
    return keywords;
}

double ArabicTextAnalyzer::calculateSimilarity(const std::string& text1, const std::string& text2) {
    std::vector<std::string> tokens1 = tokenizer->tokenize(text1);
    std::vector<std::string> tokens2 = tokenizer->tokenize(text2);
    
    // حساب Jaccard similarity
    std::set<std::string> set1(tokens1.begin(), tokens1.end());
    std::set<std::string> set2(tokens2.begin(), tokens2.end());
    
    std::set<std::string> intersection;
    std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                         std::inserter(intersection, intersection.begin()));
    
    std::set<std::string> union_set;
    std::set_union(set1.begin(), set1.end(), set2.begin(), set2.end(),
                  std::inserter(union_set, union_set.begin()));
    
    if (union_set.empty()) {
        return 0.0;
    }
    
    return static_cast<double>(intersection.size()) / union_set.size();
}

std::string ArabicTextAnalyzer::summarize(const std::string& text, int maxSentences) {
    std::vector<std::string> sentences = tokenizer->sentenceTokenize(text);
    
    if (sentences.size() <= maxSentences) {
        std::string summary;
        for (const std::string& sentence : sentences) {
            summary += sentence + " ";
        }
        return summary;
    }
    
    // اختيار الجمل الأطول (تلخيص بسيط)
    std::vector<std::pair<int, std::string>> sentencePairs;
    for (size_t i = 0; i < sentences.size(); ++i) {
        sentencePairs.push_back({static_cast<int>(i), sentences[i]});
    }
    
    std::sort(sentencePairs.begin(), sentencePairs.end(),
              [](const auto& a, const auto& b) {
                  return a.second.length() > b.second.length();
              });
    
    std::string summary;
    for (int i = 0; i < maxSentences && i < static_cast<int>(sentencePairs.size()); ++i) {
        summary += sentences[sentencePairs[i].first] + " ";
    }
    
    return summary;
}

// ==================== NLPUtils Implementation ====================

namespace NLPUtils {

double calculateTF(const std::string& term, const std::vector<std::string>& document) {
    int count = 0;
    for (const std::string& word : document) {
        if (word == term) {
            count++;
        }
    }
    return document.empty() ? 0.0 : static_cast<double>(count) / document.size();
}

double calculateIDF(const std::string& term, const std::vector<std::vector<std::string>>& documents) {
    int docCount = 0;
    for (const auto& doc : documents) {
        if (std::find(doc.begin(), doc.end(), term) != doc.end()) {
            docCount++;
        }
    }
    
    if (docCount == 0) {
        return 0.0;
    }
    
    return std::log(static_cast<double>(documents.size()) / docCount);
}

double calculateTFIDF(const std::string& term, const std::vector<std::string>& document,
                      const std::vector<std::vector<std::string>>& documents) {
    double tf = calculateTF(term, document);
    double idf = calculateIDF(term, documents);
    return tf * idf;
}

double cosineSimilarity(const std::map<std::string, double>& vector1,
                       const std::map<std::string, double>& vector2) {
    double dotProduct = 0.0;
    double norm1 = 0.0;
    double norm2 = 0.0;
    
    // حساب dot product
    for (const auto& pair : vector1) {
        auto it = vector2.find(pair.first);
        if (it != vector2.end()) {
            dotProduct += pair.second * it->second;
        }
        norm1 += pair.second * pair.second;
    }
    
    for (const auto& pair : vector2) {
        norm2 += pair.second * pair.second;
    }
    
    norm1 = std::sqrt(norm1);
    norm2 = std::sqrt(norm2);
    
    if (norm1 == 0.0 || norm2 == 0.0) {
        return 0.0;
    }
    
    return dotProduct / (norm1 * norm2);
}

int levenshteinDistance(const std::string& s1, const std::string& s2) {
    int m = s1.length();
    int n = s2.length();
    
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    
    for (int i = 0; i <= m; ++i) {
        dp[i][0] = i;
    }
    for (int j = 0; j <= n; ++j) {
        dp[0][j] = j;
    }
    
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }
    
    return dp[m][n];
}

std::map<std::string, double> textToVector(const std::string& text,
                                           const std::vector<std::string>& vocabulary) {
    std::map<std::string, double> vector;
    
    ArabicTokenizer tokenizer;
    std::vector<std::string> tokens = tokenizer.tokenize(text);
    
    for (const std::string& word : vocabulary) {
        int count = 0;
        for (const std::string& token : tokens) {
            if (token == word) {
                count++;
            }
        }
        vector[word] = static_cast<double>(count);
    }
    
    return vector;
}

std::vector<std::string> extractNGrams(const std::vector<std::string>& tokens, int n) {
    std::vector<std::string> ngrams;
    
    if (tokens.size() < n) {
        return ngrams;
    }
    
    for (size_t i = 0; i <= tokens.size() - n; ++i) {
        std::string ngram;
        for (int j = 0; j < n; ++j) {
            if (j > 0) ngram += " ";
            ngram += tokens[i + j];
        }
        ngrams.push_back(ngram);
    }
    
    return ngrams;
}

} // namespace NLPUtils

} // namespace ArabicLanguage

