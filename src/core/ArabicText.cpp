#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ArabicText.h"
#include "ArabicTextUtils.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cmath>
#include <codecvt>
#include <locale>

namespace ArabicLanguage {

ArabicText::ArabicText()
    : hdc(nullptr), enableBidi(true), enableShaping(true), enableLigatures(true) {
}

ArabicText::~ArabicText() {
    shutdown();
}

bool ArabicText::initialize(HDC deviceContext) {
    hdc = deviceContext ? deviceContext : GetDC(nullptr);

    if (!hdc) {
        std::cerr << "ArabicText: Failed to get device context" << std::endl;
        return false;
    }

    return true;
}

void ArabicText::shutdown() {
    clearFontCache();

    if (hdc && !GetDC(nullptr)) { // Only release if we created it
        ReleaseDC(nullptr, hdc);
        hdc = nullptr;
    }
}

bool ArabicText::loadFont(const std::string& fontName, const FontProperties& properties) {
    if (!hdc) return false;

    std::string fontKey = createFontKey(properties);

    // Check if already loaded
    if (fontCache.find(fontKey) != fontCache.end()) {
        return true;
    }

    // Create font
    int weight = properties.bold ? FW_BOLD : FW_NORMAL;
    DWORD italic = properties.italic ? TRUE : FALSE;
    DWORD underline = properties.underline ? TRUE : FALSE;

    // Convert font name to wide string for Win32 API
    int wLen = MultiByteToWideChar(CP_UTF8, 0, fontName.c_str(), -1, nullptr, 0);
    std::vector<WCHAR> wFontName(wLen);
    MultiByteToWideChar(CP_UTF8, 0, fontName.c_str(), -1, wFontName.data(), wLen);

    HFONT hFont = CreateFontW(
        -MulDiv(static_cast<int>(properties.size), GetDeviceCaps(hdc, LOGPIXELSY), 72), // Height
        0, 0, 0, weight, italic, underline, FALSE,
        ARABIC_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        wFontName.data()
    );

    if (!hFont) {
        return false;
    }

    // Select font and get metrics
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);

    // Store font and metrics
    fontCache[fontKey] = hFont;
    fontMetricsCache[fontKey] = tm;

    // Restore old font
    SelectObject(hdc, oldFont);

    return true;
}

ArabicText::TextMetrics ArabicText::measureText(const std::string& text,
                                              const FontProperties& font) {
    TextMetrics metrics;
    if (!hdc) return metrics;

    std::string fontKey = createFontKey(font);
    if (fontCache.find(fontKey) == fontCache.end()) {
        loadFont(font.fontName, font);
    }

    HFONT hFont = fontCache[fontKey];
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

    // Convert to wide string for measurement
    int wLen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::vector<WCHAR> wText(wLen);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wText.data(), wLen);

    SIZE size;
    GetTextExtentPoint32W(hdc, wText.data(), wLen - 1, &size);

    metrics.width = static_cast<float>(size.cx);
    metrics.height = static_cast<float>(size.cy);
    metrics.glyphCount = text.length();

    // Get ascent/descent from metrics
    auto& tm = fontMetricsCache[fontKey];
    metrics.ascent = static_cast<float>(tm.tmAscent);
    metrics.descent = static_cast<float>(tm.tmDescent);

    SelectObject(hdc, oldFont);

    return metrics;
}

ArabicText::TextParagraph ArabicText::layoutText(const std::string& text,
                                               const FontProperties& font,
                                               float maxWidth,
                                               TextDirection direction) {
    TextParagraph paragraph;

    if (direction == AUTO_DETECT) {
        paragraph.overallDirection = detectTextDirection(text);
    } else {
        paragraph.overallDirection = direction;
    }

    paragraph.maxWidth = maxWidth;

    // Simple line breaking - split by spaces
    std::vector<std::string> words = splitIntoWords(text);

    TextLine currentLine;
    float currentWidth = 0.0f;

    for (const auto& word : words) {
        TextMetrics wordMetrics = measureText(word + " ", font);
        float wordWidth = wordMetrics.width;

        if (currentWidth + wordWidth > maxWidth && !currentLine.text.empty()) {
            // Start new line
            paragraph.lines.push_back(currentLine);
            currentLine = TextLine();
            currentWidth = 0.0f;
        }

        currentLine.text += word + " ";
        currentWidth += wordWidth;
        currentLine.width = currentWidth;
        currentLine.height = wordMetrics.height;
        currentLine.direction = paragraph.overallDirection;
    }

    if (!currentLine.text.empty()) {
        paragraph.lines.push_back(currentLine);
    }

    // Calculate overall metrics
    for (const auto& line : paragraph.lines) {
        paragraph.metrics.width = std::max(paragraph.metrics.width, line.width);
        paragraph.metrics.height += line.height;
        paragraph.metrics.glyphCount += line.glyphs.size();
    }

    return paragraph;
}

std::string ArabicText::reverseForDisplay(const std::string& text) {
    if (text.empty()) return text;

    TextDirection dir = detectTextDirection(text);

    if (dir == LEFT_TO_RIGHT) {
        return text; // No reversal needed
    }

    // For RTL, reverse characters (not bytes)
    std::u32string utf32 = utf8ToUtf32(text);
    std::reverse(utf32.begin(), utf32.end());
    return utf32ToUtf8(utf32);
}

std::string ArabicText::shapeArabicText(const std::string& text) {
    // Basic Arabic text shaping - simplified implementation
    // In a full implementation, this would use a proper shaping engine

    std::u32string utf32 = utf8ToUtf32(text);
    std::u32string shaped;

    for (size_t i = 0; i < utf32.size(); ++i) {
        char32_t current = utf32[i];
        char32_t prev = (i > 0) ? utf32[i-1] : 0;
        char32_t next = (i < utf32.size() - 1) ? utf32[i+1] : 0;

        // Basic shaping logic
        if (isArabicChar(current)) {
            ArabicTextHelpers::ArabicCharType type =
                ArabicTextHelpers::getArabicCharType(current, prev, next);
            shaped.push_back(ArabicTextHelpers::shapeArabicChar(current, type));
        } else {
            shaped.push_back(current);
        }
    }

    return utf32ToUtf8(shaped);
}

ArabicText::TextDirection ArabicText::detectTextDirection(const std::string& text) {
    int arabicChars = 0;
    int totalChars = 0;

    std::u32string utf32 = utf8ToUtf32(text);

    for (char32_t c : utf32) {
        if (ArabicTextHelpers::isArabicCharacter(c)) {
            arabicChars++;
        }
        if (c >= 32 && c < 127) { // ASCII printable
            totalChars++;
        }
    }

    // If more than 30% Arabic characters, consider RTL
    if (arabicChars > totalChars * 0.3) {
        return RIGHT_TO_LEFT;
    }

    return LEFT_TO_RIGHT;
}

std::vector<std::string> ArabicText::splitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::stringstream ss(text);
    std::string word;

    while (ss >> word) {
        if (!word.empty()) {
            words.push_back(word);
        }
    }

    return words;
}

std::vector<std::string> ArabicText::splitIntoSentences(const std::string& text) {
    std::vector<std::string> sentences;
    std::string sentence;
    bool inSentence = false;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        sentence += c;

        if (c == '!' || c == '?' || c == '.' ||
            (i + 1 < text.length() && (unsigned char)text[i] == 0xD8 && (unsigned char)text[i+1] == 0x9F) || // ؟
            (i + 1 < text.length() && (unsigned char)text[i] == 0xDB && (unsigned char)text[i+1] == 0x94)) { // ۔
            if (!sentence.empty()) {
                // Trim whitespace
                size_t start = sentence.find_first_not_of(" \t\n\r");
                if (start != std::string::npos) {
                    sentence = sentence.substr(start);
                    sentences.push_back(sentence);
                    sentence.clear();
                }
            }
        }
    }

    if (!sentence.empty()) {
        sentences.push_back(sentence);
    }

    return sentences;
}

std::vector<size_t> ArabicText::findInArabicText(const std::string& text, const std::string& searchTerm) {
    std::vector<size_t> positions;

    // Normalize both texts for better matching
    std::string normalizedText = normalizeArabicText(text);
    std::string normalizedSearch = normalizeArabicText(searchTerm);

    size_t pos = 0;
    while ((pos = normalizedText.find(normalizedSearch, pos)) != std::string::npos) {
        positions.push_back(pos);
        pos += normalizedSearch.length();
    }

    return positions;
}

std::string ArabicText::replaceInArabicText(const std::string& text, const std::string& oldStr,
                                          const std::string& newStr) {
    std::string result = text;
    size_t pos = 0;

    while ((pos = result.find(oldStr, pos)) != std::string::npos) {
        result.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }

    return result;
}

std::string ArabicText::normalizeArabicText(const std::string& text) {
    return ArabicTextHelpers::normalizeArabicText(text);
}

std::string ArabicText::removeDiacritics(const std::string& text) {
    return ArabicTextHelpers::stripDiacritics(text);
}

std::string ArabicText::removeDiacriticsKeepHamza(const std::string& text) {
    std::u32string utf32 = ArabicTextHelpers::utf8ToUtf32(text);
    std::u32string result;

    for (char32_t c : utf32) {
        // Keep Hamza and basic letters, remove other diacritics
        if (ArabicTextHelpers::isDiacritic(c) &&
            c != 0x0654 && c != 0x0655) { // Keep Hamza diacritics
            continue; // Skip diacritic
        }
        result.push_back(c);
    }

    return ArabicTextHelpers::utf32ToUtf8(result);
}

size_t ArabicText::countWords(const std::string& text) {
    return ArabicTextHelpers::tokenizeArabicText(text).size();
}

size_t ArabicText::countCharacters(const std::string& text, bool includeDiacritics) {
    std::u32string utf32 = ArabicTextHelpers::utf8ToUtf32(text);
    size_t count = 0;

    for (char32_t c : utf32) {
        if (includeDiacritics || !ArabicTextHelpers::isDiacritic(c)) {
            count++;
        }
    }

    return count;
}

std::vector<std::string> ArabicText::extractKeywords(const std::string& text, size_t maxKeywords) {
    auto words = splitIntoWords(text);
    std::unordered_map<std::string, size_t> frequency;

    // Count word frequency
    for (const auto& word : words) {
        if (word.length() > 2) { // Skip very short words
            frequency[word]++;
        }
    }

    // Sort by frequency
    std::vector<std::pair<size_t, std::string>> sortedWords;
    for (const auto& pair : frequency) {
        sortedWords.emplace_back(pair.second, pair.first);
    }

    std::sort(sortedWords.rbegin(), sortedWords.rend());

    // Extract top keywords
    std::vector<std::string> keywords;
    for (size_t i = 0; i < std::min(maxKeywords, sortedWords.size()); ++i) {
        keywords.push_back(sortedWords[i].second);
    }

    return keywords;
}

ArabicText::Sentiment ArabicText::analyzeSentiment(const std::string& text) {
    // Simple sentiment analysis - placeholder implementation
    // In a real implementation, this would use NLP models

    std::unordered_set<std::string> positiveWords = {
        "جيد", "ممتاز", "رائع", "جميل", "سعيد", "مبهج"
    };

    std::unordered_set<std::string> negativeWords = {
        "سيء", "فظيع", "حزين", "مؤلم", "غاضب", "سيء"
    };

    auto words = splitIntoWords(text);
    int positiveScore = 0;
    int negativeScore = 0;

    for (const auto& word : words) {
        if (positiveWords.count(word)) positiveScore++;
        if (negativeWords.count(word)) negativeScore++;
    }

    if (positiveScore > negativeScore) return POSITIVE;
    if (negativeScore > positiveScore) return NEGATIVE;
    return NEUTRAL;
}

std::string ArabicText::spellCheck(const std::string& text) {
    // Simple spell checking - placeholder
    // In a real implementation, this would use a dictionary

    std::unordered_map<std::string, std::string> corrections = {
        {"الى", "إلى"},
        {"علي", "على"},
        {"في", "في"},
        {"من", "من"}
    };

    auto words = splitIntoWords(text);
    std::string result;

    for (size_t i = 0; i < words.size(); ++i) {
        auto it = corrections.find(words[i]);
        if (it != corrections.end()) {
            result += it->second;
        } else {
            result += words[i];
        }

        if (i < words.size() - 1) {
            result += " ";
        }
    }

    return result;
}

std::string ArabicText::numberToWords(long long number) {
    return ArabicTextHelpers::numberToWords(number);
}

std::string ArabicText::dateToWords(int day, int month, int year) {
    std::string monthName = ArabicTextHelpers::getArabicMonthName(month);
    std::string dayStr = numberToWords(day);
    std::string yearStr = numberToWords(year);

    return dayStr + " " + monthName + " " + yearStr;
}

bool ArabicText::isArabicChar(char32_t codepoint) {
    return ArabicTextHelpers::isArabicCharacter(codepoint);
}

bool ArabicText::isArabicDiacritic(char32_t codepoint) {
    return ArabicTextHelpers::isDiacritic(codepoint);
}

bool ArabicText::isArabicLetter(char32_t codepoint) {
    return ArabicTextHelpers::isArabicLetter(codepoint);
}

bool ArabicText::isArabicNumber(char32_t codepoint) {
    return ArabicTextHelpers::isArabicNumber(codepoint);
}

std::string ArabicText::utf8ToCp1256(const std::string& utf8) {
    // Convert UTF-8 to CP1256 (Arabic Windows codepage)
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::vector<WCHAR> wide(len);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), len);

    len = WideCharToMultiByte(1256, 0, wide.data(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(len, '\0');
    WideCharToMultiByte(1256, 0, wide.data(), -1, &result[0], len, nullptr, nullptr);

    return result;
}

std::string ArabicText::cp1256ToUtf8(const std::string& cp1256) {
    // Convert CP1256 to UTF-8
    int len = MultiByteToWideChar(1256, 0, cp1256.c_str(), -1, nullptr, 0);
    std::vector<WCHAR> wide(len);
    MultiByteToWideChar(1256, 0, cp1256.c_str(), -1, wide.data(), len);

    len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1, &result[0], len, nullptr, nullptr);

    return result;
}

std::u32string ArabicText::utf8ToUtf32(const std::string& utf8) {
    return ArabicTextHelpers::utf8ToUtf32(utf8);
}

std::string ArabicText::utf32ToUtf8(const std::u32string& utf32) {
    return ArabicTextHelpers::utf32ToUtf8(utf32);
}

std::vector<std::string> ArabicText::getAvailableArabicFonts() {
    std::vector<std::string> fonts;

    // Get system fonts that support Arabic
    LOGFONT lf = {0};
    lf.lfCharSet = ARABIC_CHARSET;

    HDC hdc = GetDC(nullptr);
    EnumFontFamiliesEx(hdc, &lf, [](const LOGFONT* lpelfe, const TEXTMETRIC* lpntme,
                                  DWORD FontType, LPARAM lParam) -> int {
        std::vector<std::string>* fonts = reinterpret_cast<std::vector<std::string>*>(lParam);
#ifdef UNICODE
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, lpelfe->lfFaceName, -1, NULL, 0, NULL, NULL);
        std::string faceName(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, lpelfe->lfFaceName, -1, &faceName[0], size_needed, NULL, NULL);
        if (!faceName.empty() && faceName.back() == '\0') faceName.pop_back();
        fonts->push_back(faceName);
#else
        fonts->push_back(lpelfe->lfFaceName);
#endif
        return 1; // Continue enumeration
    }, reinterpret_cast<LPARAM>(&fonts), 0);

    ReleaseDC(nullptr, hdc);

    return fonts;
}

bool ArabicText::isArabicFontAvailable(const std::string& fontName) {
    auto fonts = getAvailableArabicFonts();
    return std::find(fonts.begin(), fonts.end(), fontName) != fonts.end();
}

void ArabicText::clearFontCache() {
    for (auto& pair : fontCache) {
        DeleteObject(pair.second);
    }
    fontCache.clear();
    fontMetricsCache.clear();
}

TEXTMETRIC ArabicText::getFontMetrics(const std::string& fontKey) {
    auto it = fontMetricsCache.find(fontKey);
    if (it != fontMetricsCache.end()) {
        return it->second;
    }

    TEXTMETRIC tm = {0};
    return tm;
}

std::string ArabicText::createFontKey(const FontProperties& props) {
    std::stringstream ss;
    ss << props.fontName << "_" << props.size << "_" << props.bold << "_"
       << props.italic << "_" << props.underline;
    return ss.str();
}

// ────────────────────────────────────────────────────────
// تطبيق ArabicTextProcessor
// ────────────────────────────────────────────────────────

ArabicTextProcessor::ArabicTextProcessor(std::shared_ptr<ArabicText> engine)
    : textEngine(engine) {
    // Initialize basic Arabic roots (simplified)
    arabicRoots = {
        {"كتب", "كتب"}, {"فعل", "فعل"}, {"قال", "قول"}, {"أكل", "أكل"}
    };

    // Common corrections
    commonCorrections = {
        {"الى", "إلى"}, {"علي", "على"}, {"بال", "بال"}, {"فال", "فال"}
    };

    // Basic stop words
    arabicStopWords = {
        "في", "من", "إلى", "على", "مع", "هو", "هي", "هم", "هن", "أنا", "نحن"
    };
}

std::string ArabicTextProcessor::extractRoot(const std::string& word) {
    // Simple root extraction - placeholder
    auto it = arabicRoots.find(word);
    if (it != arabicRoots.end()) {
        return it->second;
    }

    // Basic stemming: remove common prefixes/suffixes
    std::string stem = word;

    // Remove common prefixes
    std::vector<std::string> prefixes = {"ال", "و", "ف", "ب", "ك", "ل"};
    for (const auto& prefix : prefixes) {
        if (stem.find(prefix) == 0) {
            stem = stem.substr(prefix.length());
            break;
        }
    }

    // Remove common suffixes
    std::vector<std::string> suffixes = {"ة", "ات", "ون", "ين", "ان", "وا"};
    for (const auto& suffix : suffixes) {
        size_t pos = stem.rfind(suffix);
        if (pos == stem.length() - suffix.length()) {
            stem = stem.substr(0, pos);
            break;
        }
    }

    return stem;
}

ArabicTextProcessor::MorphologicalAnalysis ArabicTextProcessor::analyzeMorphology(const std::string& word) {
    MorphologicalAnalysis analysis;

    // Simple morphological analysis - placeholder
    analysis.root = extractRoot(word);

    // Basic pattern recognition
    if (word.find("ال") == 0) {
        analysis.prefixes.push_back("ال");
    }

    if (word.length() >= 2 && 
        ((unsigned char)word[word.length()-2] == 0xD8 && (unsigned char)word[word.length()-1] == 0xA9) || // ة
        ((unsigned char)word[word.length()-2] == 0xD8 && (unsigned char)word[word.length()-1] == 0xAA)) { // ت
        analysis.suffixes.push_back("ة");
    }

    analysis.pos = "اسم"; // Default to noun

    return analysis;
}

std::string ArabicTextProcessor::generateArabicText(const std::string& pattern, size_t length) {
    // Simple text generation - placeholder
    std::string result;

    std::vector<std::string> words = {
        "مرحباً", "كيف", "حالك", "أنا", "جيد", "شكراً", "لك", "الله", "أكبر"
    };

    while (result.length() < length) {
        result += words[rand() % words.size()] + " ";
    }

    return result.substr(0, length);
}

std::string ArabicTextProcessor::compressArabicText(const std::string& text) {
    // Simple RLE compression for Arabic text
    std::string compressed;

    for (size_t i = 0; i < text.length(); ++i) {
        char current = text[i];
        size_t count = 1;

        while (i + 1 < text.length() && text[i + 1] == current && count < 255) {
            count++;
            i++;
        }

        if (count > 3) { // Only compress if repetition > 3
            compressed += '\0'; // Special marker
            compressed += current;
            compressed += static_cast<unsigned char>(count);
        } else {
            for (size_t j = 0; j < count; ++j) {
                compressed += current;
            }
        }
    }

    return compressed;
}

std::string ArabicTextProcessor::decompressArabicText(const std::string& compressed) {
    std::string decompressed;

    for (size_t i = 0; i < compressed.length(); ++i) {
        if (compressed[i] == '\0' && i + 2 < compressed.length()) {
            char ch = compressed[i + 1];
            size_t count = static_cast<unsigned char>(compressed[i + 2]);

            for (size_t j = 0; j < count; ++j) {
                decompressed += ch;
            }
            i += 2;
        } else {
            decompressed += compressed[i];
        }
    }

    return decompressed;
}

std::string ArabicTextProcessor::encryptArabicText(const std::string& text, const std::string& key) {
    // Simple XOR encryption - NOT secure for production use
    std::string encrypted = text;

    for (size_t i = 0; i < encrypted.length(); ++i) {
        encrypted[i] ^= key[i % key.length()];
    }

    return encrypted;
}

std::string ArabicTextProcessor::decryptArabicText(const std::string& encrypted, const std::string& key) {
    // XOR decryption (same as encryption)
    return encryptArabicText(encrypted, key);
}

// ────────────────────────────────────────────────────────
// تطبيق ArabicTextEditor
// ────────────────────────────────────────────────────────

ArabicTextEditor::ArabicTextEditor(std::shared_ptr<ArabicText> engine, size_t maxUndo)
    : textEngine(engine), cursorPosition(0), maxUndoSteps(maxUndo) {
}

void ArabicTextEditor::insertText(const std::string& text, size_t position) {
    saveState();

    if (position == std::string::npos) {
        position = cursorPosition;
    }

    position = clampPosition(position);
    content.insert(position, text);
    cursorPosition = position + text.length();

    // Clear redo stack
    redoStack.clear();
}

void ArabicTextEditor::deleteText(size_t start, size_t length) {
    if (start >= content.length()) return;

    saveState();

    length = std::min(length, content.length() - start);
    content.erase(start, length);

    if (cursorPosition > start) {
        cursorPosition = start;
    }

    redoStack.clear();
}

void ArabicTextEditor::replaceText(size_t start, size_t length, const std::string& replacement) {
    saveState();

    content.replace(start, length, replacement);
    cursorPosition = start + replacement.length();

    redoStack.clear();
}

std::string ArabicTextEditor::cut() {
    if (selection.isEmpty()) return "";

    std::string cutText = content.substr(selection.start, selection.length());
    deleteText(selection.start, selection.length());
    selection = Selection();

    return cutText;
}

std::string ArabicTextEditor::copy() const {
    if (selection.isEmpty()) return "";
    return content.substr(selection.start, selection.length());
}

void ArabicTextEditor::paste(const std::string& text) {
    if (!selection.isEmpty()) {
        replaceText(selection.start, selection.length(), text);
        selection = Selection();
    } else {
        insertText(text, cursorPosition);
    }
}

std::vector<size_t> ArabicTextEditor::find(const std::string& searchTerm) {
    return textEngine->findInArabicText(content, searchTerm);
}

void ArabicTextEditor::replaceAll(const std::string& oldStr, const std::string& newStr) {
    saveState();

    size_t pos = 0;
    while ((pos = content.find(oldStr, pos)) != std::string::npos) {
        content.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }

    redoStack.clear();
}

void ArabicTextEditor::moveCursor(int delta) {
    cursorPosition = clampPosition(cursorPosition + delta);
}

void ArabicTextEditor::setCursorPosition(size_t position) {
    cursorPosition = clampPosition(position);
}

void ArabicTextEditor::undo() {
    if (!canUndo()) return;

    redoStack.push_back(content);
    content = undoStack.back();
    undoStack.pop_back();

    cursorPosition = clampPosition(cursorPosition);
}

void ArabicTextEditor::redo() {
    if (!canRedo()) return;

    undoStack.push_back(content);
    content = redoStack.back();
    redoStack.pop_back();

    cursorPosition = clampPosition(cursorPosition);
}

ArabicTextEditor::TextStats ArabicTextEditor::getStats() const {
    TextStats stats;

    stats.characterCount = content.length();
    stats.wordCount = textEngine->countWords(content);

    // Count lines
    stats.lineCount = 1;
    for (char c : content) {
        if (c == '\n') stats.lineCount++;
    }

    // Count Arabic characters
    std::u32string utf32 = textEngine->utf8ToUtf32(content);
    for (char32_t c : utf32) {
        if (ArabicTextHelpers::isArabicCharacter(c)) {
            stats.arabicCharCount++;
        }
    }

    // Word frequency
    auto words = textEngine->splitIntoWords(content);
    for (const auto& word : words) {
        stats.wordFrequency[word]++;
    }

    return stats;
}

void ArabicTextEditor::saveState() {
    undoStack.push_back(content);

    if (undoStack.size() > maxUndoSteps) {
        undoStack.erase(undoStack.begin());
    }
}

size_t ArabicTextEditor::clampPosition(size_t position) const {
    return std::min(position, content.length());
}

// ────────────────────────────────────────────────────────
// تطبيق ArabicTextHelpers
// ────────────────────────────────────────────────────────

namespace ArabicTextHelpers {

bool isArabicCharacter(char32_t codepoint) {
    // Arabic Unicode ranges
    return (codepoint >= 0x0600 && codepoint <= 0x06FF) ||  // Arabic
           (codepoint >= 0x0750 && codepoint <= 0x077F) ||  // Arabic Supplement
           (codepoint >= 0x08A0 && codepoint <= 0x08FF) ||  // Arabic Extended-A
           (codepoint >= 0xFB50 && codepoint <= 0xFDFF) ||  // Arabic Presentation Forms-A
           (codepoint >= 0xFE70 && codepoint <= 0xFEFF);    // Arabic Presentation Forms-B
}

bool isDiacritic(char32_t codepoint) {
    return ARABIC_DIACRITICS_U32.find(codepoint) != std::u32string::npos;
}

bool isArabicLetter(char32_t codepoint) {
    return ARABIC_LETTERS_U32.find(codepoint) != std::u32string::npos;
}

bool isArabicNumber(char32_t codepoint) {
    return ARABIC_NUMBERS_U32.find(codepoint) != std::u32string::npos;
}

ArabicCharType getArabicCharType(char32_t current, char32_t prev, char32_t next) {
    // List of Arabic characters that don't connect to the next character
    auto doesNotConnectToNext = [](char32_t c) {
        return c == 0x0621 || // Hamza
               c == 0x0622 || // Alef with Madda
               c == 0x0623 || // Alef with Hamza Above
               c == 0x0624 || // Waw with Hamza Above
               c == 0x0625 || // Alef with Hamza Below
               c == 0x0627 || // Alef
               c == 0x062F || // Dal
               c == 0x0630 || // Thal
               c == 0x0631 || // Ra
               c == 0x0632 || // Zain
               c == 0x0648 || // Waw
               c == 0x0649 || // Alef Maksura
               c == 0x0629;   // Taa Marbouta
    };

    bool hasPrev = isArabicCharacter(prev) && !doesNotConnectToNext(prev);
    bool hasNext = isArabicCharacter(next);

    if (!hasPrev && !hasNext) return ISOLATED;
    if (!hasPrev && hasNext) return INITIAL;
    if (hasPrev && hasNext) return MEDIAL;
    if (hasPrev && !hasNext) return FINAL;

    return ISOLATED;
}

char32_t shapeArabicChar(char32_t codepoint, ArabicCharType type) {
    // Basic Arabic character shaping table (Simplified)
    // Map of (codepoint, type) -> Presentation Form B codepoint
    struct ShapingEntry {
        char32_t isolated, initial, medial, final;
    };

    static const std::unordered_map<char32_t, ShapingEntry> shapingTable = {
        {0x0621, {0xFE80, 0xFE80, 0xFE80, 0xFE80}}, // Hamza
        {0x0622, {0xFE81, 0xFE81, 0xFE82, 0xFE82}}, // Alef Madda
        {0x0623, {0xFE83, 0xFE83, 0xFE84, 0xFE84}}, // Alef Hamza Above
        {0x0624, {0xFE85, 0xFE85, 0xFE86, 0xFE86}}, // Waw Hamza Above
        {0x0625, {0xFE87, 0xFE87, 0xFE88, 0xFE88}}, // Alef Hamza Below
        {0x0626, {0xFE89, 0xFE8B, 0xFE8C, 0xFE8A}}, // Yeh Hamza Above
        {0x0627, {0xFE8D, 0xFE8D, 0xFE8E, 0xFE8E}}, // Alef
        {0x0628, {0xFE8F, 0xFE91, 0xFE92, 0xFE90}}, // Beh
        {0x0629, {0xFE93, 0xFE93, 0xFE94, 0xFE94}}, // Teh Marbuta
        {0x062A, {0xFE95, 0xFE97, 0xFE98, 0xFE96}}, // Teh
        {0x062B, {0xFE99, 0xFE9B, 0xFE9C, 0xFE9A}}, // Theh
        {0x062C, {0xFE9D, 0xFE9F, 0xFEA0, 0xFE9E}}, // Jeem
        {0x062D, {0xFEA1, 0xFEA3, 0xFEA4, 0xFEA2}}, // Hah
        {0x062E, {0xFEA5, 0xFEA7, 0xFEA8, 0xFEA6}}, // Khah
        {0x062F, {0xFEA9, 0xFEA9, 0xFEAA, 0xFEAA}}, // Dal
        {0x0630, {0xFEAB, 0xFEAB, 0xFEAC, 0xFEAC}}, // Thal
        {0x0631, {0xFEAD, 0xFEAD, 0xFEAE, 0xFEAE}}, // Ra
        {0x0632, {0xFEAF, 0xFEAF, 0xFEB0, 0xFEB0}}, // Zain
        {0x0633, {0xFEB1, 0xFEB3, 0xFEB4, 0xFEB2}}, // Seen
        {0x0634, {0xFEB5, 0xFEB7, 0xFEB8, 0xFEB6}}, // Sheen
        {0x0635, {0xFEB9, 0xFEBB, 0xFEBC, 0xFEBA}}, // Sad
        {0x0636, {0xFEBD, 0xFEBF, 0xFEC0, 0xFEBE}}, // Dad
        {0x0637, {0xFEC1, 0xFEC3, 0xFEC4, 0xFEC2}}, // Tah
        {0x0638, {0xFEC5, 0xFEC7, 0xFEC8, 0xFEC6}}, // Zah
        {0x0639, {0xFEC9, 0xFECB, 0xFECC, 0xFECA}}, // Ain
        {0x063A, {0xFECD, 0xFECF, 0xFED0, 0xFECE}}, // Ghain
        {0x0641, {0xFED1, 0xFED3, 0xFED4, 0xFED2}}, // Feh
        {0x0642, {0xFED5, 0xFED7, 0xFED8, 0xFED6}}, // Qaf
        {0x0643, {0xFED9, 0xFEDB, 0xFEDC, 0xFEDA}}, // Kaf
        {0x0644, {0xFEDD, 0xFEDF, 0xFEE0, 0xFEDE}}, // Lam
        {0x0645, {0xFEE1, 0xFEE3, 0xFEE4, 0xFEE2}}, // Meem
        {0x0646, {0xFEE5, 0xFEE7, 0xFEE8, 0xFEE6}}, // Noon
        {0x0647, {0xFEE9, 0xFEEB, 0xFEEC, 0xFEEA}}, // Heh
        {0x0648, {0xFEED, 0xFEED, 0xFEEE, 0xFEEE}}, // Waw
        {0x0649, {0xFEEF, 0xFEEF, 0xFEF0, 0xFEF0}}, // Alef Maksura
        {0x064A, {0xFEF1, 0xFEF3, 0xFEF4, 0xFEF2}}  // Yeh
    };

    auto it = shapingTable.find(codepoint);
    if (it != shapingTable.end()) {
        switch (type) {
            case ISOLATED: return it->second.isolated;
            case INITIAL: return it->second.initial;
            case MEDIAL: return it->second.medial;
            case FINAL: return it->second.final;
        }
    }

    return codepoint;
}

std::string reverseForRTLDisplay(const std::string& text) {
    std::u32string utf32 = utf8ToUtf32(text);
    std::reverse(utf32.begin(), utf32.end());
    return utf32ToUtf8(utf32);
}

std::vector<std::string> tokenizeArabicText(const std::string& text) {
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string token;

    while (ss >> token) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

std::string normalizeArabicText(const std::string& text) {
    std::u32string utf32 = utf8ToUtf32(text);
    std::u32string normalized;

    for (char32_t c : utf32) {
        // Basic normalization
        if (c == 0x0623 || c == 0x0625 || c == 0x0622) {
            normalized.push_back(0x0627); // Standard Alef
        } else if (c == 0x0629 || c == 0x0647) {
            normalized.push_back(0x0647); // Heh
        } else {
            normalized.push_back(c);
        }
    }

    return utf32ToUtf8(normalized);
}

std::string stripDiacritics(const std::string& text) {
    std::u32string utf32 = utf8ToUtf32(text);
    std::u32string result;

    for (char32_t c : utf32) {
        if (!isDiacritic(c)) {
            result.push_back(c);
        }
    }

    return utf32ToUtf8(result);
}

std::string westernToArabicNumerals(const std::string& text) {
    std::u32string utf32 = utf8ToUtf32(text);

    for (size_t i = 0; i < utf32.length(); ++i) {
        if (utf32[i] >= '0' && utf32[i] <= '9') {
            utf32[i] = ARABIC_NUMBERS_U32[utf32[i] - '0'];
        }
    }

    return utf32ToUtf8(utf32);
}

std::string arabicToWesternNumerals(const std::string& text) {
    std::u32string utf32 = utf8ToUtf32(text);

    for (size_t i = 0; i < utf32.length(); ++i) {
        size_t pos = ARABIC_NUMBERS_U32.find(utf32[i]);
        if (pos != std::u32string::npos) {
            utf32[i] = '0' + static_cast<char32_t>(pos);
        }
    }

    return utf32ToUtf8(utf32);
}

std::string getArabicMonthName(int month) {
    std::vector<std::string> months = {
        "", "يناير", "فبراير", "مارس", "أبريل", "مايو", "يونيو",
        "يوليو", "أغسطس", "سبتمبر", "أكتوبر", "نوفمبر", "ديسمبر"
    };

    if (month >= 1 && month <= 12) {
        return months[month];
    }

    return "";
}

std::string getArabicDayName(int dayOfWeek) {
    std::vector<std::string> days = {
        "الأحد", "الاثنين", "الثلاثاء", "الأربعاء", "الخميس", "الجمعة", "السبت"
    };

    if (dayOfWeek >= 0 && dayOfWeek <= 6) {
        return days[dayOfWeek];
    }

    return "";
}

std::string timeToArabicWords(int hour, int minute) {
    std::string hourStr = numberToWords(hour);
    std::string minuteStr = numberToWords(minute);

    return "الساعة " + hourStr + " و" + minuteStr + " دقيقة";
}

std::string numberToWords(long long number) {
    if (number == 0) return "صفر";

    static const std::vector<std::string> units = {
        "", "واحد", "اثنان", "ثلاثة", "أربعة", "خمسة", "ستة", "سبعة", "ثمانية", "تسعة"
    };

    static const std::vector<std::string> teens = {
        "عشرة", "أحد عشر", "اثنا عشر", "ثلاثة عشر", "أربعة عشر", "خمسة عشر",
        "ستة عشر", "سبعة عشر", "ثمانية عشر", "تسعة عشر"
    };

    static const std::vector<std::string> tens = {
        "", "", "عشرون", "ثلاثون", "أربعون", "خمسون", "ستون", "سبعون", "ثمانون", "تسعون"
    };

    if (number < 10) return units[static_cast<size_t>(number)];
    if (number < 20) return teens[static_cast<size_t>(number - 10)];
    if (number < 100) {
        int ten = static_cast<int>(number / 10);
        int unit = static_cast<int>(number % 10);
        if (unit == 0) return tens[static_cast<size_t>(ten)];
        return units[static_cast<size_t>(unit)] + " و" + tens[static_cast<size_t>(ten)];
    }

    if (number < 1000) {
        int hundred = static_cast<int>(number / 100);
        int remainder = static_cast<int>(number % 100);
        std::string result;
        if (hundred == 1) result = "مائة";
        else if (hundred == 2) result = "مائتان";
        else result = units[static_cast<size_t>(hundred)] + " مائة";

        if (remainder > 0) {
            result += " و" + numberToWords(remainder);
        }
        return result;
    }

    return std::to_string(number); // Fallback
}

std::u32string utf8ToUtf32(const std::string& utf8) {
    std::u32string utf32;
    size_t i = 0;
    while (i < utf8.length()) {
        unsigned char c = utf8[i];
        char32_t cp = 0;
        if ((c & 0x80) == 0) { cp = c; i += 1; }
        else if ((c & 0xE0) == 0xC0) { if (i+1 < utf8.length()) cp = ((c&0x1F)<<6)|(utf8[i+1]&0x3F); i += 2; }
        else if ((c & 0xF0) == 0xE0) { if (i+2 < utf8.length()) cp = ((c&0x0F)<<12)|((utf8[i+1]&0x3F)<<6)|(utf8[i+2]&0x3F); i += 3; }
        else if ((c & 0xF8) == 0xF0) { if (i+3 < utf8.length()) cp = ((c&0x07)<<18)|((utf8[i+1]&0x3F)<<12)|((utf8[i+2]&0x3F)<<6)|(utf8[i+3]&0x3F); i += 4; }
        else i++;
        utf32 += cp;
    }
    return utf32;
}

std::string utf32ToUtf8(const std::u32string& utf32) {
    std::string utf8;
    for (char32_t cp : utf32) {
        if (cp <= 0x7F) utf8 += static_cast<char>(cp);
        else if (cp <= 0x7FF) { utf8 += static_cast<char>(0xC0|(cp>>6)); utf8 += static_cast<char>(0x80|(cp&0x3F)); }
        else if (cp <= 0xFFFF) { utf8 += static_cast<char>(0xE0|(cp>>12)); utf8 += static_cast<char>(0x80|((cp>>6)&0x3F)); utf8 += static_cast<char>(0x80|(cp&0x3F)); }
        else if (cp <= 0x10FFFF) { utf8 += static_cast<char>(0xF0|(cp>>18)); utf8 += static_cast<char>(0x80|((cp>>12)&0x3F)); utf8 += static_cast<char>(0x80|((cp>>6)&0x3F)); utf8 += static_cast<char>(0x80|(cp&0x3F)); }
    }
    return utf8;
}

size_t utf8Length(const std::string& utf8) {
    size_t len = 0, i = 0;
    while (i < utf8.length()) {
        unsigned char c = utf8[i];
        if ((c & 0x80) == 0) i += 1;
        else if ((c & 0xE0) == 0xC0) i += 2;
        else if ((c & 0xF0) == 0xE0) i += 3;
        else if ((c & 0xF8) == 0xF0) i += 4;
        else i++;
        len++;
    }
    return len;
}

std::string utf8CharAt(const std::string& utf8, size_t position) {
    size_t cur = 0, i = 0;
    while (i < utf8.length() && cur < position) {
        unsigned char c = utf8[i];
        size_t len = 1;
        if ((c & 0x80) == 0) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        i += len; cur++;
    }
    if (cur == position && i < utf8.length()) {
        unsigned char c = utf8[i];
        size_t len = 1;
        if ((c & 0x80) == 0) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        return utf8.substr(i, len);
    }
    return "";
}

std::string utf8Substring(const std::string& utf8, size_t start, size_t length) {
    std::string res;
    size_t cur = 0, i = 0, added = 0;
    while (i < utf8.length() && cur < start) {
        unsigned char c = utf8[i];
        size_t len = 1;
        if ((c & 0x80) == 0) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        i += len; cur++;
    }
    while (i < utf8.length() && added < length) {
        unsigned char c = utf8[i];
        size_t len = 1;
        if ((c & 0x80) == 0) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        res += utf8.substr(i, len);
        i += len; added++;
    }
    return res;
}

bool isArabicChar(char32_t c) {
    return (c >= 0x0600 && c <= 0x06FF) || (c >= 0x0750 && c <= 0x077F) ||
           (c >= 0x08A0 && c <= 0x08FF) || (c >= 0xFB50 && c <= 0xFDFF) ||
           (c >= 0xFE70 && c <= 0xFEFF);
}

std::string removeDiacritics(const std::string& text) {
    std::u32string utf32 = utf8ToUtf32(text);
    std::u32string res;
    for (char32_t c : utf32) {
        if (!isDiacritic(c)) res += c;
    }
    return utf32ToUtf8(res);
}

std::string normalizeHamza(const std::string& text) {
    std::u32string utf32 = utf8ToUtf32(text);
    std::u32string res;
    for (char32_t c : utf32) {
        if (c == 0x0623 || c == 0x0625 || c == 0x0622 || c == 0x0627) res += 0x0627;
        else if (c == 0x0629 || c == 0x0647) res += 0x0647;
        else res += c;
    }
    return utf32ToUtf8(res);
}

std::string reversePreservingWordOrder(const std::string& text) {
    std::vector<std::string> words = tokenizeArabicText(text);
    std::reverse(words.begin(), words.end());
    std::string res;
    for (size_t i = 0; i < words.size(); ++i) {
        if (i > 0) res += " ";
        res += words[i];
    }
    return res;
}
} // namespace ArabicTextHelpers

} // namespace ArabicLanguage
