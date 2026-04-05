#include "ArabicTextUtils.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cmath>

namespace ArabicLanguage {

ArabicTextUtils::ArabicTextUtils() {
    // تهيئة الإحصائيات
}

ArabicTextUtils::SearchResult ArabicTextUtils::KMPSearch(const std::string& text, const std::string& pattern) {
    auto start_time = std::chrono::steady_clock::now();

    if (pattern.empty()) {
        return SearchResult(0, true, std::chrono::nanoseconds(0));
    }

    if (text.length() < pattern.length()) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        recordOperation(duration);
        return SearchResult(0, false, duration);
    }

    std::vector<int> lps(pattern.length(), 0);
    SearchAlgorithms::computeLPSArray(pattern, lps);

    size_t i = 0; // index for text
    size_t j = 0; // index for pattern

    while (i < text.length()) {
        if (pattern[j] == text[i]) {
            i++;
            j++;
        }

        if (j == pattern.length()) {
            // Found pattern at index i - j
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
            recordOperation(duration);
            return SearchResult(i - j, true, duration);
        } else if (i < text.length() && pattern[j] != text[i]) {
            if (j != 0) {
                j = lps[j - 1];
            } else {
                i++;
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration);
    return SearchResult(0, false, duration);
}

ArabicTextUtils::SearchResult ArabicTextUtils::BoyerMooreSearch(const std::string& text, const std::string& pattern) {
    auto start_time = std::chrono::steady_clock::now();

    if (pattern.empty()) {
        return SearchResult(0, true, std::chrono::nanoseconds(0));
    }

    if (text.length() < pattern.length()) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        recordOperation(duration);
        return SearchResult(0, false, duration);
    }

    std::vector<int> badChar(256, -1);
    SearchAlgorithms::computeBadCharTable(pattern, badChar);

    size_t shift = 0;
    while (shift <= (text.length() - pattern.length())) {
        int j = static_cast<int>(pattern.length()) - 1;

        while (j >= 0 && pattern[j] == text[shift + j]) {
            j--;
        }

        if (j < 0) {
            // Found pattern
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
            recordOperation(duration);
            return SearchResult(shift, true, duration);
        } else {
            shift += std::max(static_cast<size_t>(1), static_cast<size_t>(std::max(0, j - badChar[static_cast<unsigned char>(text[shift + j])])));
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration);
    return SearchResult(0, false, duration);
}

ArabicTextUtils::SearchResult ArabicTextUtils::SmartSearch(const std::string& text, const std::string& pattern) {
    if (pattern.empty()) {
        return SearchResult(0, true, std::chrono::nanoseconds(0));
    }

    if (pattern.length() <= 4) {
        auto start_time = std::chrono::steady_clock::now();
        size_t pos = SearchAlgorithms::naiveSearch(text, pattern);
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        recordOperation(duration);
        return SearchResult(pos, pos != std::string::npos, duration);
    } else if (pattern.length() <= 32) {
        return KMPSearch(text, pattern);
    } else {
        return BoyerMooreSearch(text, pattern);
    }
}

std::string ArabicTextUtils::EfficientReplace(const std::string& text,
                                            const std::string& oldStr,
                                            const std::string& newStr) {
    auto start_time = std::chrono::steady_clock::now();

    if (oldStr.empty()) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        recordOperation(duration, text.length() * sizeof(char));
        return text;
    }

    EfficientStringBuilder builder(text.length());

    size_t pos = 0;
    size_t lastPos = 0;

    while ((pos = text.find(oldStr, pos)) != std::string::npos) {
        if (pos > lastPos) {
            builder.append(text.substr(lastPos, pos - lastPos));
        }
        builder.append(newStr);
        pos += oldStr.length();
        lastPos = pos;
    }

    if (lastPos < text.length()) {
        builder.append(text.substr(lastPos));
    }

    std::string result = builder.toString();

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration, result.length() * sizeof(char));

    return result;
}

std::string ArabicTextUtils::MultiPassReplace(const std::string& text,
                                            const std::vector<std::pair<std::string, std::string>>& replacements) {
    auto start_time = std::chrono::steady_clock::now();
    std::string result = text;
    for (const auto& replacement : replacements) {
        result = EfficientReplace(result, replacement.first, replacement.second);
    }
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration, result.length() * sizeof(char));
    return result;
}

size_t ArabicTextUtils::CountOccurrences(const std::string& text, const std::string& pattern) {
    auto start_time = std::chrono::steady_clock::now();
    if (pattern.empty()) return 0;
    size_t count = 0;
    size_t pos = 0;
    while ((pos = text.find(pattern, pos)) != std::string::npos) {
        count++;
        pos += pattern.length();
    }
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration);
    return count;
}

std::string ArabicTextUtils::UnicodeReverse(const std::string& text) {
    auto start_time = std::chrono::steady_clock::now();
    std::u32string utf32 = utf8ToUtf32(text);
    std::reverse(utf32.begin(), utf32.end());
    std::string result = utf32ToUtf8(utf32);
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration, result.length() * sizeof(char));
    return result;
}

std::string ArabicTextUtils::NormalizeArabic(const std::string& text, bool removeDiacritics) {
    auto start_time = std::chrono::steady_clock::now();
    std::string result = text;
    if (removeDiacritics) {
        result = ArabicTextHelpers::removeDiacritics(result);
    }
    result = ArabicTextHelpers::normalizeHamza(result);
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration, result.length() * sizeof(char));
    return result;
}

bool ArabicTextUtils::ContainsArabic(const std::string& text) const {
    auto start_time = std::chrono::steady_clock::now();
    std::u32string utf32 = ArabicTextHelpers::utf8ToUtf32(text);
    bool hasArabic = false;
    for (char32_t c : utf32) {
        if (ArabicTextHelpers::isArabicChar(c)) {
            hasArabic = true;
            break;
        }
    }
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration);
    return hasArabic;
}

std::vector<std::string> ArabicTextUtils::ExtractWords(const std::string& text) {
    std::vector<std::string> words;
    std::stringstream ss(text);
    std::string word;
    while (ss >> word) {
        if (!word.empty()) words.push_back(word);
    }
    return words;
}

size_t ArabicTextUtils::LevenshteinDistance(const std::string& s1, const std::string& s2) {
    auto start_time = std::chrono::steady_clock::now();
    size_t len1 = s1.length();
    size_t len2 = s2.length();
    if (len1 == 0) return len2;
    if (len2 == 0) return len1;
    std::vector<std::vector<size_t>> matrix(len1 + 1, std::vector<size_t>(len2 + 1));
    for (size_t i = 0; i <= len1; ++i) matrix[i][0] = i;
    for (size_t j = 0; j <= len2; ++j) matrix[0][j] = j;
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            matrix[i][j] = std::min({
                matrix[i - 1][j] + 1,
                matrix[i][j - 1] + 1,
                matrix[i - 1][j - 1] + cost
            });
        }
    }
    size_t result = matrix[len1][len2];
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration);
    return result;
}

std::vector<ArabicTextUtils::SearchResult> ArabicTextUtils::FuzzySearch(const std::string& text,
                                                                      const std::string& pattern,
                                                                      size_t maxDistance) {
    auto start_time = std::chrono::steady_clock::now();
    std::vector<SearchResult> results;
    size_t textLen = text.length();
    size_t patternLen = pattern.length();
    for (size_t i = 0; i <= textLen - patternLen; ++i) {
        std::string substring = text.substr(i, patternLen);
        size_t distance = LevenshteinDistance(substring, pattern);
        if (distance <= maxDistance) {
            results.emplace_back(i, true, std::chrono::nanoseconds(0));
        }
    }
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration);
    return results;
}

std::string ArabicTextUtils::CompressText(const std::string& text) {
    auto start_time = std::chrono::steady_clock::now();
    std::string compressed;
    for (size_t i = 0; i < text.length(); ++i) {
        char current = text[i];
        size_t count = 1;
        while (i + 1 < text.length() && text[i + 1] == current && count < 255) {
            count++;
            i++;
        }
        if (count > 3) {
            compressed += '\0';
            compressed += current;
            compressed += static_cast<char>(count);
        } else {
            for (size_t j = 0; j < count; ++j) compressed += current;
        }
    }
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration, compressed.length() * sizeof(char));
    return compressed;
}

std::string ArabicTextUtils::DecompressText(const std::string& compressed) {
    auto start_time = std::chrono::steady_clock::now();
    std::string decompressed;
    for (size_t i = 0; i < compressed.length(); ++i) {
        if (compressed[i] == '\0' && i + 2 < compressed.length()) {
            char ch = compressed[i + 1];
            size_t count = static_cast<unsigned char>(compressed[i + 2]);
            for (size_t j = 0; j < count; ++j) decompressed += ch;
            i += 2;
        } else {
            decompressed += compressed[i];
        }
    }
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    recordOperation(duration, decompressed.length() * sizeof(char));
    return decompressed;
}

void ArabicTextUtils::recordOperation(std::chrono::nanoseconds time, size_t memory, bool isSearch) const {
    stats.operationsCount++;
    stats.totalTime += time;
    stats.memoryUsed += memory;
    stats.avgTimePerOperation = static_cast<double>(stats.totalTime.count()) / stats.operationsCount;
}

std::vector<std::string> ArabicTextUtils::getSupportedAlgorithms() const {
    return {
        "KMP Search", "Boyer-Moore Search", "Smart Search", "Efficient Replace",
        "Multi-pass Replace", "Unicode Reverse", "Arabic Normalization",
        "Levenshtein Distance", "Fuzzy Search", "RLE Compression"
    };
}

std::vector<std::string> ArabicTextUtils::diagnoseUnicodeSupport(const std::string& text) const {
    std::vector<std::string> diagnosis;
    diagnosis.push_back("=== تشخيص دعم Unicode ===");
    size_t utf8Len = utf8Length(text);
    diagnosis.push_back("عدد الأحرف الحقيقية: " + std::to_string(utf8Len));
    diagnosis.push_back("طول البايتات: " + std::to_string(text.length()));
    diagnosis.push_back("يحتوي على أحرف عربية: " + std::string(ContainsArabic(text) ? "نعم" : "لا"));
    return diagnosis;
}

std::u32string ArabicTextUtils::utf8ToUtf32(const std::string& utf8) const {
    return ArabicTextHelpers::utf8ToUtf32(utf8);
}

std::string ArabicTextUtils::utf32ToUtf8(const std::u32string& utf32) const {
    return ArabicTextHelpers::utf32ToUtf8(utf32);
}

size_t ArabicTextUtils::utf8Length(const std::string& utf8) const {
    return ArabicTextHelpers::utf8Length(utf8);
}

std::string ArabicTextUtils::utf8CharAt(const std::string& utf8, size_t position) const {
    return ArabicTextHelpers::utf8CharAt(utf8, position);
}

std::string ArabicTextUtils::utf8Substring(const std::string& utf8, size_t start, size_t length) const {
    return ArabicTextHelpers::utf8Substring(utf8, start, length);
}

std::string ArabicTextUtils::numberToWords(long long number) const {
    return ArabicTextHelpers::numberToWords(number);
}

std::string ArabicTextUtils::reversePreservingWordOrder(const std::string& text) const {
    return ArabicTextHelpers::reversePreservingWordOrder(text);
}

// ────────────────────────────────────────────────────────
// تطبيق دوال SearchAlgorithms
// ────────────────────────────────────────────────────────

namespace SearchAlgorithms {

void computeLPSArray(const std::string& pattern, std::vector<int>& lps) {
    size_t m = pattern.length();
    lps.assign(m, 0);
    size_t len = 0;
    size_t i = 1;
    while (i < m) {
        if (pattern[i] == pattern[len]) {
            len++;
            lps[i] = len;
            i++;
        } else {
            if (len != 0) len = lps[len - 1];
            else { lps[i] = 0; i++; }
        }
    }
}

void computeBadCharTable(const std::string& pattern, std::vector<int>& badChar) {
    badChar.assign(256, -1);
    for (size_t i = 0; i < pattern.length(); ++i) {
        badChar[static_cast<unsigned char>(pattern[i])] = i;
    }
}

size_t naiveSearch(const std::string& text, const std::string& pattern) {
    return text.find(pattern);
}

}

// ────────────────────────────────────────────────────────
// تطبيق دوال ArabicTextHelpers
// ────────────────────────────────────────────────────────

} // namespace ArabicLanguage
