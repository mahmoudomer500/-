#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <string>
#include <vector>
#include <memory>

#include "ArabicCodeEditor.h"
#include "../graphics/ArabicGraphics.h"

namespace ArabicLanguage {

// ────────────────────────────────────────────────────────
// تنفيذ محرر الكود العربي
// ────────────────────────────────────────────────────────

ArabicCodeEditor::ArabicCodeEditor()
    : currentState(EDITING), hasUnsavedChanges(false),
      cursorLine(0), cursorColumn(0), scrollLine(0), scrollColumn(0),
      showSuggestions(false), selectedSuggestion(-1),
      currentSearchResult(0), fontSize(14.0f), lineHeight(20.0f),
      backgroundColor(30, 30, 30), textColor(255, 255, 255),
      selectionColor(60, 120, 200), cursorColor(255, 255, 255) {

    engine = std::make_shared<ArabicLanguage::ArabicGameEngine>();
    ui = std::make_shared<ArabicLanguage::ArabicUI>();
    textRenderer = std::make_shared<ArabicLanguage::ArabicText>();

    // إضافة سطر فارغ
    lines.push_back("");

    initializeKeywords();
}

bool ArabicCodeEditor::initialize(HINSTANCE hInstance) {
    ArabicLanguage::ArabicGameEngine::EngineConfig config;
    config.windowTitle = "محرر الكود العربي";
    config.windowWidth = 1200;
    config.windowHeight = 800;

    if (!engine->initialize(hInstance, config)) {
        return false;
    }

    ui = engine->getUI();
    textRenderer->initialize();

    // إنشاء UI
    createUI();

    return true;
}

void ArabicCodeEditor::createUI() {
    auto self = this;
    
    // منطقة الكود الرئيسية
    codeArea = ui->createTextArea("CodeArea", ArabicUI::UIRect(200, 50, 980, 650));
    codeArea->setFontSize(fontSize);
    codeArea->setOnValueChange([self](const ArabicUI::UIEvent& e) {
        self->lines = self->codeArea->getLines();
        self->tokenizeText();
        self->hasUnsavedChanges = true;
    });

    // أزرار التحكم
    newButton = ui->createButton("NewButton", "جديد", ArabicUI::UIRect(10, 10, 60, 30));
    newButton->setOnClick([self](const ArabicUI::UIEvent& e) { self->newFile(); });

    openButton = ui->createButton("OpenButton", "فتح", ArabicUI::UIRect(80, 10, 60, 30));
    openButton->setOnClick([self](const ArabicUI::UIEvent& e) { self->openFile("sample_code.عربي"); });

    saveButton = ui->createButton("SaveButton", "حفظ", ArabicUI::UIRect(150, 10, 60, 30));
    saveButton->setOnClick([self](const ArabicUI::UIEvent& e) { self->saveFile(); });

    runButton = ui->createButton("RunButton", "تشغيل", ArabicUI::UIRect(220, 10, 60, 30));
    runButton->setOnClick([self](const ArabicUI::UIEvent& e) { self->runCode(); });

    searchButton = ui->createButton("SearchButton", "بحث", ArabicUI::UIRect(290, 10, 60, 30));
    searchButton->setOnClick([self](const ArabicUI::UIEvent& e) { self->setState(SEARCHING); });

    replaceButton = ui->createButton("ReplaceButton", "استبدال", ArabicUI::UIRect(360, 10, 60, 30));
    replaceButton->setOnClick([self](const ArabicUI::UIEvent& e) { self->setState(REPLACING); });

    // حقول البحث والاستبدال
    searchInput = ui->createTextBox("SearchInput", "ابحث عن...", ArabicUI::UIRect(10, 720, 300, 25));
    searchInput->setOnValueChange([self](const ArabicUI::UIEvent& e) {
        self->searchText = self->searchInput->getText();
        self->performSearch();
    });

    replaceInput = ui->createTextBox("ReplaceInput", "استبدل بـ...", ArabicUI::UIRect(320, 720, 300, 25));
    replaceInput->setOnValueChange([self](const ArabicUI::UIEvent& e) {
        self->replaceText = self->replaceInput->getText();
    });

    // تسميات الحالة
    statusLabel = ui->createLabel("StatusLabel", "جاهز", ArabicUI::UIRect(10, 760, 200, 20));
    statusLabel->setFont(ArabicUI::UIFont("Arial", 12.0f, ArabicUI::UIColor(150, 150, 150), false, false, ArabicUI::TextDirection::RIGHT_TO_LEFT));

    lineColumnLabel = ui->createLabel("LineColLabel", "السطر: 1, العمود: 1", ArabicUI::UIRect(950, 760, 200, 20));
    lineColumnLabel->setFont(ArabicUI::UIFont("Arial", 12.0f, ArabicUI::UIColor(150, 150, 150), false, false, ArabicUI::TextDirection::RIGHT_TO_LEFT));
}

void ArabicCodeEditor::initializeKeywords() {
    // كلمات مفتاحية عربية
    this->keywords = {
        "دالة", "اذا", "وإلا", "ل", "كرر", "طالما", "كسر", "استمر",
        "أعد", "استورد", "من", "كـ", "في", "صحيح", "خطأ", "لاشيء",
        "رقم", "نص", "منطقي", "قائمة", "خريطة", "كائن", "ثابت"
    };

    // دوال عربية
    this->functions.clear();
    this->functions.insert({"اكتب", ArabicCodeEditor::TOKEN_FUNCTION});
    this->functions.insert({"اقرأ", ArabicCodeEditor::TOKEN_FUNCTION});
    this->functions.insert({"تحويل", ArabicCodeEditor::TOKEN_FUNCTION});
    this->functions.insert({"طول", ArabicCodeEditor::TOKEN_FUNCTION});
    this->functions.insert({"قطع", ArabicCodeEditor::TOKEN_FUNCTION});
    this->functions.insert({"استبدال", ArabicCodeEditor::TOKEN_FUNCTION});
    this->functions.insert({"ابحث", ArabicCodeEditor::TOKEN_FUNCTION});
    this->functions.insert({"انضم", ArabicCodeEditor::TOKEN_FUNCTION});

    // عمليات
    this->operators.clear();
    this->operators.insert({"+", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({"-", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({"*", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({"/", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({"=", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({"==", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({"!=", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({"<", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({">", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({"<=", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({">=", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({"&&", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({"||", ArabicCodeEditor::TOKEN_OPERATOR});
    this->operators.insert({"!", ArabicCodeEditor::TOKEN_OPERATOR});
}

void ArabicCodeEditor::tokenizeText() {
    tokenizedLines.clear();
    ArabicSyntaxHighlighter highlighter;

    for (size_t i = 0; i < lines.size(); ++i) {
        tokenizedLines.push_back(highlighter.highlightLine(lines[i], i));
    }
}

std::vector<ArabicCodeEditor::Token> ArabicCodeEditor::tokenizeLine(const std::string& line, size_t lineIndex) {
    ArabicSyntaxHighlighter highlighter;
    return highlighter.highlightLine(line, lineIndex);
}

ArabicUI::UIColor ArabicCodeEditor::getTokenColor(TokenType type) const {
    switch (type) {
        case ArabicCodeEditor::TOKEN_KEYWORD: return ArabicUI::UIColor(0, 120, 215);    // أزرق
        case ArabicCodeEditor::TOKEN_IDENTIFIER: return ArabicUI::UIColor(220, 220, 220); // رمادي فاتح
        case ArabicCodeEditor::TOKEN_STRING: return ArabicUI::UIColor(206, 145, 120);   // برتقالي
        case ArabicCodeEditor::TOKEN_NUMBER: return ArabicUI::UIColor(181, 206, 168);   // أخضر
        case ArabicCodeEditor::TOKEN_OPERATOR: return ArabicUI::UIColor(180, 180, 180); // رمادي
        case ArabicCodeEditor::TOKEN_COMMENT: return ArabicUI::UIColor(106, 153, 85);   // أخضر داكن
        case ArabicCodeEditor::TOKEN_FUNCTION: return ArabicUI::UIColor(220, 140, 220); // بنفسجي
        case ArabicCodeEditor::TOKEN_VARIABLE: return ArabicUI::UIColor(156, 220, 254); // أزرق فاتح
        case ArabicCodeEditor::TOKEN_ERROR: return ArabicUI::UIColor(255, 100, 100);    // أحمر
        default: return ArabicUI::UIColor(255, 255, 255);      // أبيض
    }
}

void ArabicCodeEditor::updateAutoComplete() {
    if (cursorLine >= lines.size()) return;

    currentWord = getCurrentWord();

    if (currentWord.empty()) {
        showSuggestions = false;
        return;
    }

    ArabicAutoComplete completer;
    suggestions = completer.getSuggestions(currentWord);
    showSuggestions = !suggestions.empty();
    selectedSuggestion = 0;
}

std::string ArabicCodeEditor::getCurrentWord() const {
    if (cursorLine >= lines.size() || cursorColumn == 0) return "";

    const std::string& line = lines[cursorLine];
    size_t start = cursorColumn - 1;

    // العثور على بداية الكلمة
    while (start > 0 && (std::isalnum(line[start-1]) || line[start-1] == '_' ||
                         (unsigned char)line[start-1] >= 128)) {
        start--;
    }

    return line.substr(start, cursorColumn - start);
}

void ArabicCodeEditor::insertText(const std::string& text) {
    if (cursorLine >= lines.size()) return;

    std::string& line = lines[cursorLine];
    line.insert(cursorColumn, text);
    cursorColumn += text.length();
    hasUnsavedChanges = true;

    tokenizeText();
    analyzeErrors();
    updateAutoComplete();
    updateStatusBar();
}

void ArabicCodeEditor::deleteText(bool deleteForward) {
    if (cursorLine >= lines.size()) return;

    std::string& line = lines[cursorLine];

    if (deleteForward) {
        // حذف الحرف التالي
        if (cursorColumn < line.length()) {
            line.erase(cursorColumn, 1);
        } else if (cursorLine + 1 < lines.size()) {
            // دمج السطر التالي
            line += lines[cursorLine + 1];
            lines.erase(lines.begin() + cursorLine + 1);
        }
    } else {
        // حذف الحرف السابق
        if (cursorColumn > 0) {
            line.erase(cursorColumn - 1, 1);
            cursorColumn--;
        } else if (cursorLine > 0) {
            // دمج مع السطر السابق
            cursorColumn = lines[cursorLine - 1].length();
            lines[cursorLine - 1] += line;
            lines.erase(lines.begin() + cursorLine);
            cursorLine--;
        }
    }

    hasUnsavedChanges = true;
    tokenizeText();
    analyzeErrors();
    updateAutoComplete();
    updateStatusBar();
}

void ArabicCodeEditor::handleKeyInput(const ArabicInput& input) {
    if (input.isKeyPressed(VK_LEFT)) {
        if (cursorColumn > 0) {
            cursorColumn--;
        } else if (cursorLine > 0) {
            cursorLine--;
            cursorColumn = lines[cursorLine].length();
        }
    } else if (input.isKeyPressed(VK_RIGHT)) {
        if (cursorColumn < lines[cursorLine].length()) {
            cursorColumn++;
        } else if (cursorLine + 1 < lines.size()) {
            cursorLine++;
            cursorColumn = 0;
        }
    } else if (input.isKeyPressed(VK_UP)) {
        if (cursorLine > 0) {
            cursorLine--;
            cursorColumn = std::min(cursorColumn, lines[cursorLine].length());
        }
    } else if (input.isKeyPressed(VK_DOWN)) {
        if (cursorLine + 1 < lines.size()) {
            cursorLine++;
            cursorColumn = std::min(cursorColumn, lines[cursorLine].length());
        }
    } else if (input.isKeyPressed(VK_HOME)) {
        cursorColumn = 0;
    } else if (input.isKeyPressed(VK_END)) {
        cursorColumn = lines[cursorLine].length();
    } else if (input.isKeyPressed(VK_BACK)) {
        deleteText(false);
    } else if (input.isKeyPressed(VK_DELETE)) {
        deleteText(true);
    } else if (input.isKeyPressed(VK_RETURN)) {
        // إدراج سطر جديد
        std::string& line = lines[cursorLine];
        std::string newLine = line.substr(cursorColumn);
        line = line.substr(0, cursorColumn);
        lines.insert(lines.begin() + cursorLine + 1, newLine);
        cursorLine++;
        cursorColumn = 0;
        hasUnsavedChanges = true;
        tokenizeText();
        analyzeErrors();
    } else if (input.isKeyPressed(VK_TAB)) {
        insertText("    "); // 4 مسافات
    } else {
        // إدخال أحرف عادية
        char typedChar = 0;
        for (char c = 32; c < 127; ++c) {
            if (input.isKeyPressed(c)) {
                typedChar = c;
                break;
            }
        }

        if (typedChar) {
            insertText(std::string(1, typedChar));
        }
    }

    // تحديث الواجهة
    updateStatusBar();
}

void ArabicCodeEditor::updateStatusBar() {
    // مزامنة مع UITextArea
    if (codeArea) {
        codeArea->setLines(lines);
    }

    // تحديث تسمية السطر والعمود
    std::string posText = "السطر: " + std::to_string(cursorLine + 1) +
                         ", العمود: " + std::to_string(cursorColumn + 1);
    lineColumnLabel->setText(posText);

    // تحديث تسمية الحالة
    std::string status = "السطر " + std::to_string(cursorLine + 1);
    if (!currentFilePath.empty()) {
        status += " - " + currentFilePath;
    }
    if (hasUnsavedChanges) {
        status += " (غير محفوظ)";
    }
    if (!errors.empty()) {
        status += " - أخطاء: " + std::to_string(errors.size());
    }

    statusLabel->setText(status);
}

void ArabicCodeEditor::analyzeErrors() {
    errors.clear();
    ArabicErrorAnalyzer analyzer;

    errors = analyzer.analyzeCode(lines);
}

void ArabicCodeEditor::runCode() {
    if (currentFilePath.empty()) {
        // حفظ مؤقت
        if (!saveFile("temp_run.عربي")) return;
    }

    // تشغيل الكود باستخدام المترجم
    std::string command = "arabic_compiler_v5.exe \"" + currentFilePath + "\"";
    system(command.c_str());
}

bool ArabicCodeEditor::saveFile(const std::string& filePath) {
    std::string path = filePath.empty() ? currentFilePath : filePath;
    if (path.empty()) return false;

    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        statusLabel->setText("خطأ في الحفظ: " + path);
        return false;
    }

    for (const auto& line : lines) {
        file.write(line.c_str(), line.length());
        file.write("\n", 1);
    }

    file.close();
    currentFilePath = path;
    hasUnsavedChanges = false;
    updateStatusBar();

    return true;
}

bool ArabicCodeEditor::loadFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        statusLabel->setText("خطأ في التحميل: " + filePath);
        return false;
    }

    lines.clear();
    std::string line;
    while (std::getline(file, line)) {
        // إزالة \r إذا كان موجوداً (للتوافق مع Windows)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }

    if (lines.empty()) {
        lines.push_back("");
    }

    file.close();
    currentFilePath = filePath;
    hasUnsavedChanges = false;
    cursorLine = 0;
    cursorColumn = 0;

    tokenizeText();
    analyzeErrors();
    updateStatusBar();

    return true;
}

void ArabicCodeEditor::newFile() {
    lines = {""};
    currentFilePath.clear();
    hasUnsavedChanges = false;
    cursorLine = 0;
    cursorColumn = 0;

    tokenizedLines.clear();
    errors.clear();
    updateStatusBar();
}

void ArabicCodeEditor::run() {
    while (engine->isRunning()) {
        engine->processEvents();

        // تحديث UI
        ui->update(static_cast<float>(engine->getDeltaTime()));

        // معالجة الإدخال
        auto input = engine->getInput();

        // معالجة إدخال المحرر
        if (currentState == EDITING) {
            handleKeyInput(*input);
        }

        // رسم
        engine->beginFrame();
        
        // مسح الخلفية
        engine->getGraphics()->clear(backgroundColor);

        // رسم الشريط العلوي
        engine->getGraphics()->drawRectangle(
            glm::vec2(0, 0), glm::vec2(1200, 45), ArabicUI::UIColor(45, 45, 45), true
        );

        // رسم العنوان
        engine->getGraphics()->drawText(
            "محرر الكود العربي", glm::vec2(500, 10),
            ArabicUI::UIColor(255, 255, 255), 20.0f
        );

        // رسم منطقة الكود
        drawCodeArea(*engine->getGraphics());

        // رسم الإكمال التلقائي
        if (showSuggestions) {
            drawAutoComplete(*engine->getGraphics());
        }

        // رسم واجهة المستخدم
        ui->render(engine->getGraphics());

        engine->endFrame();
    }
}

void ArabicCodeEditor::drawCodeArea(ArabicGraphics& graphics) {
    float x = 200.0f;
    float y = 50.0f;
    float width = 980.0f;
    float height = 650.0f;

    // رسم خلفية منطقة الكود
    graphics.drawRectangle(glm::vec2(x, y), glm::vec2(width, height), backgroundColor, true);

    // رسم أرقام الأسطر
    float lineNumberWidth = 50.0f;
    graphics.drawRectangle(glm::vec2(x, y), glm::vec2(lineNumberWidth, height),
                          ArabicUI::UIColor(40, 40, 40), true);

    // رسم الأسطر
    float currentY = y + lineHeight;
    for (size_t i = scrollLine; i < lines.size() && currentY < y + height; ++i) {
        // رسم تمييز الخطأ إذا وجد
        for (const auto& error : errors) {
            if (error.line == i) {
                drawErrorLine(graphics, i, error.message);
                break;
            }
        }

        // رسم رقم السطر
        std::string lineNum = std::to_string(i + 1);
        graphics.drawText(lineNum, glm::vec2(x + 5, currentY - 2),
                         ArabicUI::UIColor(100, 100, 100), fontSize * 0.8f);

        // رسم محتوى السطر مع تمييز الصيغة
        drawLineWithSyntaxHighlighting(graphics, i, glm::vec2(x + lineNumberWidth + 5, currentY), width - lineNumberWidth - 10);

        // رسم المؤشر إذا كان في هذا السطر
        if (i == cursorLine) {
            float cursorX = x + lineNumberWidth + 5 + cursorColumn * 8.0f; // تقريب لعرض الحرف
            graphics.drawLine(glm::vec3(cursorX, currentY, 0.0f), glm::vec3(cursorX, currentY + lineHeight, 0.0f),
                           cursorColor, 2.0f);
        }

        currentY += lineHeight;
    }

    // رسم خطوط الفاصل
    graphics.drawLine(glm::vec3(x + lineNumberWidth, y, 0.0f), glm::vec3(x + lineNumberWidth, y + height, 0.0f),
                    ArabicUI::UIColor(60, 60, 60), 1.0f);
}

void ArabicCodeEditor::drawErrorLine(ArabicGraphics& graphics, size_t lineIndex, const std::string& message) {
    float x = 200.0f;
    float y = 50.0f + (lineIndex - scrollLine) * lineHeight;
    float width = 980.0f;

    // رسم خلفية حمراء شفافة للسطر الذي به خطأ
    graphics.drawRectangle(glm::vec2(x, y), glm::vec2(width, lineHeight),
                          ArabicUI::UIColor(1.0f, 0.0f, 0.0f, 0.2f), true);

    // رسم رسالة الخطأ
    graphics.drawText(message, glm::vec2(x + width - 200, y + 2),
                     ArabicUI::UIColor(255, 100, 100), fontSize * 0.8f);
}

void ArabicCodeEditor::drawLineWithSyntaxHighlighting(ArabicGraphics& graphics, size_t lineIndex,
                                                   glm::vec2 position, float maxWidth) {
    if (lineIndex >= tokenizedLines.size()) return;

    float currentX = position.x;
    const auto& tokens = tokenizedLines[lineIndex];

    for (const auto& token : tokens) {
        if (currentX >= position.x + maxWidth) break;

        graphics.drawText(token.text, glm::vec2(currentX, position.y),
                         token.color, fontSize);

        // تقريب لعرض النص
        currentX += token.text.length() * 8.0f;
    }
}

void ArabicCodeEditor::drawAutoComplete(ArabicGraphics& graphics) {
    if (suggestions.empty()) return;

    float x = 200.0f + cursorColumn * 8.0f;
    float y = 50.0f + (cursorLine - scrollLine + 1) * lineHeight;
    float width = 250.0f;
    float height = suggestions.size() * 25.0f;

    // رسم خلفية القائمة
    graphics.drawRectangle(glm::vec2(x, y), glm::vec2(width, height),
                          ArabicUI::UIColor(50, 50, 50), true);
    graphics.drawRectangle(glm::vec2(x, y), glm::vec2(width, height),
                          ArabicUI::UIColor(100, 100, 100), false);

    // رسم العناصر
    for (size_t i = 0; i < suggestions.size(); ++i) {
        ArabicUI::UIColor bgColor = (i == selectedSuggestion) ? ArabicUI::UIColor(70, 70, 70) : ArabicUI::UIColor(50, 50, 50);

        graphics.drawRectangle(glm::vec2(x, y + i * 25), glm::vec2(width, 25), bgColor, true);
        graphics.drawText(suggestions[i].text, glm::vec2(x + 5, y + i * 25 + 5),
                         ArabicUI::UIColor(255, 255, 255), fontSize);
    }
}

void ArabicCodeEditor::stop() {
    if (engine) {
        engine->shutdown();
    }
}

bool ArabicCodeEditor::openFile(const std::string& filePath) {
    return loadFile(filePath);
}

std::string ArabicCodeEditor::getText() const {
    std::string result;
    for (size_t i = 0; i < lines.size(); ++i) {
        result += lines[i];
        if (i < lines.size() - 1) result += "\n";
    }
    return result;
}

// ────────────────────────────────────────────────────────
// تنفيذ وظائف إضافية لمحرر الكود
// ────────────────────────────────────────────────────────

void ArabicCodeEditor::setText(const std::string& text) {
    lines.clear();

    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }

    if (lines.empty()) {
        lines.push_back("");
    }

    cursorLine = 0;
    cursorColumn = 0;
    hasUnsavedChanges = true;

    tokenizeText();
    analyzeErrors();
    updateStatusBar();
}

void ArabicCodeEditor::setCursorPosition(size_t line, size_t column) {
    cursorLine = std::min(line, lines.size() - 1);
    cursorColumn = std::min(column, lines[cursorLine].length());
    updateStatusBar();
}

void ArabicCodeEditor::generateSuggestions(const std::string& prefix) {
    ArabicAutoComplete completer;
    suggestions = completer.getSuggestions(prefix);
    showSuggestions = !suggestions.empty();
    selectedSuggestion = 0;
}

void ArabicCodeEditor::performSearch() {
    if (searchText.empty()) {
        searchResults.clear();
        currentSearchResult = 0;
        return;
    }

    ArabicSearchReplace searcher;
    searcher.setSearchCriteria(searchText, caseSensitive, wholeWord, useRegex);
    searchResults = searcher.searchInLines(lines);
    currentSearchResult = 0;

    if (!searchResults.empty()) {
        cursorLine = searchResults[0].first;
        cursorColumn = searchResults[0].second;
        updateStatusBar();
    }
}

void ArabicCodeEditor::performReplace() {
    if (searchResults.empty()) return;

    // استبدال الحالي
    std::string& line = lines[cursorLine];
    line.replace(cursorColumn, searchText.length(), replaceText);
    hasUnsavedChanges = true;

    // تحديث نتائج البحث بعد الاستبدال
    performSearch();
    tokenizeText();
    analyzeErrors();
}

// ────────────────────────────────────────────────────────
// تنفيذ مساعد تمييز الصيغة (ArabicSyntaxHighlighter)
// ────────────────────────────────────────────────────────

ArabicSyntaxHighlighter::ArabicSyntaxHighlighter() {
    initializeArabicKeywords();
    stringRegex  = std::regex("\"[^\"]*\"|'[^']*'");
    numberRegex  = std::regex("\\b\\d+(\\.\\d+)?\\b");
    commentRegex = std::regex("//.*|#.*");          // ← يدعم // و #
    functionRegex = std::regex("\\b[a-zA-Z_\\x80-\\xFF][a-zA-Z0-9_\\x80-\\xFF]*(?=\\s*\\()");
}

void ArabicSyntaxHighlighter::initializeArabicKeywords() {
    arabicKeywords = {
        "اطبع", "مت", "دالة", "أرجع", "إذا", "وإلا", "بينما", "لكل", "صف",
        "جديد", "خاص", "عام", "محمي", "استورد", "صحيح", "خطأ", "و", "أو",
        "ليس", "في", "توقف", "استمر", "نهاية", "هذا", "حاول", "امسك",
        "لا_شيء", "رقم", "نص", "منطقي", "مصفوفة", "قاموس", "يرث"
    };

    arabicFunctions = {
        "جذر", "قوة", "مطلق", "طول", "نوع", "هل_رقم", "اكتب_ملف", "اقرأ_ملف",
        "هل_ملف_موجود", "افتح_ملف", "أغلق_ملف", "اقرأ_سطر", "نص_استبدل",
        "نص_فرعي", "نص_إلى_رقم", "رقم_إلى_نص", "لون_النص", "لون_الخلفية",
        "إعادة_لون", "لون_عشوائي", "انتظر", "أضف", "قاموس_جديد", "قاموس_ضع",
        "قاموس_اجلب", "قاموس_حجم", "قاموس_يحتوي", "قاموس_احذف"
    };

    arabicOperators = {
        "+", "-", "*", "/", "%", "=", "==", "!=", "<", ">", "<=", ">=", "&&", "||", "!"
    };
}

std::vector<ArabicCodeEditor::Token> ArabicSyntaxHighlighter::highlightLine(const std::string& line, size_t lineIndex) {
    std::vector<ArabicCodeEditor::Token> tokens;
    if (line.empty()) return tokens;

    size_t currentPos = 0;
    while (currentPos < line.length()) {
        if (std::isspace(static_cast<unsigned char>(line[currentPos]))) {
            currentPos++;
            continue;
        }

        std::string suffix = line.substr(currentPos);
        std::smatch match;

        // تعليقات
        if (std::regex_search(suffix, match, commentRegex) && match.position() == 0) {
            tokens.emplace_back(ArabicCodeEditor::TOKEN_COMMENT, match.str(), lineIndex, currentPos, ArabicUI::UIColor(106, 153, 85));
            currentPos += match.length();
            continue;
        }

        // سلاسل نصية
        if (std::regex_search(suffix, match, stringRegex) && match.position() == 0) {
            tokens.emplace_back(ArabicCodeEditor::TOKEN_STRING, match.str(), lineIndex, currentPos, ArabicUI::UIColor(206, 145, 120));
            currentPos += match.length();
            continue;
        }

        // أرقام
        if (std::regex_search(suffix, match, numberRegex) && match.position() == 0) {
            tokens.emplace_back(ArabicCodeEditor::TOKEN_NUMBER, match.str(), lineIndex, currentPos, ArabicUI::UIColor(181, 206, 168));
            currentPos += match.length();
            continue;
        }

        // استدعاء دوال
        if (std::regex_search(suffix, match, functionRegex) && match.position() == 0) {
            tokens.emplace_back(ArabicCodeEditor::TOKEN_FUNCTION, match.str(), lineIndex, currentPos, ArabicUI::UIColor(220, 140, 220));
            currentPos += match.length();
            continue;
        }

        // كلمات مفتاحية ومعرفات
        size_t start = currentPos;
        while (currentPos < line.length() && (std::isalnum((unsigned char)line[currentPos]) || 
               (unsigned char)line[currentPos] >= 128 || line[currentPos] == '_')) {
            currentPos++;
        }

        if (start != currentPos) {
            std::string word = line.substr(start, currentPos - start);
            ArabicCodeEditor::TokenType type = getTokenType(word);
            
            ArabicUI::UIColor color;
            switch (type) {
                case ArabicCodeEditor::TOKEN_KEYWORD: color = ArabicUI::UIColor(0, 120, 215); break;
                case ArabicCodeEditor::TOKEN_FUNCTION: color = ArabicUI::UIColor(220, 140, 220); break;
                default: color = ArabicUI::UIColor(220, 220, 220); break;
            }
            
            tokens.emplace_back(type, word, lineIndex, start, color);
            continue;
        }

        // عمليات ورموز أخرى
        std::string op(1, line[currentPos]);
        tokens.emplace_back(ArabicCodeEditor::TOKEN_OPERATOR, op, lineIndex, currentPos, ArabicUI::UIColor(180, 180, 180));
        currentPos++;
    }

    return tokens;
}

std::vector<std::vector<ArabicCodeEditor::Token>> ArabicSyntaxHighlighter::highlightFile(const std::vector<std::string>& lines) {
    std::vector<std::vector<ArabicCodeEditor::Token>> result;
    for (size_t i = 0; i < lines.size(); ++i) {
        result.push_back(highlightLine(lines[i], i));
    }
    return result;
}

void ArabicSyntaxHighlighter::addKeyword(const std::string& keyword) {
    arabicKeywords.insert(keyword);
}

void ArabicSyntaxHighlighter::addFunction(const std::string& function) {
    arabicFunctions.insert(function);
}

ArabicCodeEditor::TokenType ArabicSyntaxHighlighter::getTokenType(const std::string& token) const {
    if (arabicKeywords.find(token) != arabicKeywords.end()) {
        return ArabicCodeEditor::TOKEN_KEYWORD;
    }
    if (arabicFunctions.find(token) != arabicFunctions.end()) {
        return ArabicCodeEditor::TOKEN_FUNCTION;
    }
    return ArabicCodeEditor::TOKEN_IDENTIFIER;
}

// ────────────────────────────────────────────────────────
// تنفيذ نظام الإكمال التلقائي
// ────────────────────────────────────────────────────────

ArabicAutoComplete::ArabicAutoComplete() {
    initializeBasicSuggestions();
}

void ArabicAutoComplete::initializeBasicSuggestions() {
    // كلمات مفتاحية
    addSuggestion("دالة", "تعريف دالة جديدة", ArabicCodeEditor::TOKEN_KEYWORD);
    addSuggestion("اذا", "عبارة شرطية", ArabicCodeEditor::TOKEN_KEYWORD);
    addSuggestion("وإلا", "جزء آخر من العبارة الشرطية", ArabicCodeEditor::TOKEN_KEYWORD);
    addSuggestion("ل", "حلقة تكرار", ArabicCodeEditor::TOKEN_KEYWORD);
    addSuggestion("كرر", "حلقة تكرار بسيطة", ArabicCodeEditor::TOKEN_KEYWORD);
    addSuggestion("أعد", "إرجاع قيمة من دالة", ArabicCodeEditor::TOKEN_KEYWORD);

    // دوال
    addSuggestion("اكتب", "طباعة نص أو قيمة", ArabicCodeEditor::TOKEN_FUNCTION);
    addSuggestion("اقرأ", "قراءة إدخال من المستخدم", ArabicCodeEditor::TOKEN_FUNCTION);
    addSuggestion("طول", "الحصول على طول النص أو القائمة", ArabicCodeEditor::TOKEN_FUNCTION);
    addSuggestion("قطع", "قطع جزء من النص", ArabicCodeEditor::TOKEN_FUNCTION);
    addSuggestion("استبدال", "استبدال نص بآخر", ArabicCodeEditor::TOKEN_FUNCTION);

    // متغيرات شائعة
    addSuggestion("نص", "متغير نصي", ArabicCodeEditor::TOKEN_VARIABLE);
    addSuggestion("رقم", "متغير رقمي", ArabicCodeEditor::TOKEN_VARIABLE);
    addSuggestion("منطقي", "متغير منطقي", ArabicCodeEditor::TOKEN_VARIABLE);
    addSuggestion("قائمة", "قائمة من العناصر", ArabicCodeEditor::TOKEN_VARIABLE);
}

std::vector<ArabicCodeEditor::AutoCompleteSuggestion> ArabicAutoComplete::getSuggestions(const std::string& prefix) {
    std::vector<ArabicCodeEditor::AutoCompleteSuggestion> result;

    for (const auto& suggestion : suggestions) {
        if (suggestion.second.text.find(prefix) == 0) {
            result.push_back(suggestion.second);
        }
    }

    // ترتيب حسب الاستخدام الأخير
    std::sort(result.begin(), result.end(),
              [this](const auto& a, const auto& b) {
                  auto itA = std::find(recentCompletions.begin(), recentCompletions.end(), a.text);
                  auto itB = std::find(recentCompletions.begin(), recentCompletions.end(), b.text);
                  return itA < itB;
              });

    return result;
}

void ArabicAutoComplete::addSuggestion(const std::string& text, const std::string& description,
                                     ArabicCodeEditor::TokenType type) {
    suggestions[text] = {text, description, type};
}

void ArabicAutoComplete::updateContextSuggestions(const std::vector<std::string>& currentLines,
                                                size_t cursorLine, size_t cursorColumn) {
    // تحليل السياق لإضافة اقتراحات ذكية
    // يمكن تحسين هذا لتحليل المتغيرات المعرفة والدوال المتاحة
}

ArabicCodeEditor::AutoCompleteSuggestion ArabicAutoComplete::getBestSuggestion(const std::string& prefix) {
    auto suggs = getSuggestions(prefix);
    return suggs.empty() ? ArabicCodeEditor::AutoCompleteSuggestion() : suggs[0];
}

void ArabicAutoComplete::recordCompletion(const std::string& completion) {
    // إزالة إذا كان موجوداً
    auto it = std::find(recentCompletions.begin(), recentCompletions.end(), completion);
    if (it != recentCompletions.end()) {
        recentCompletions.erase(it);
    }

    // إضافة في البداية
    recentCompletions.insert(recentCompletions.begin(), completion);

    // الحفاظ على آخر 10 استخدامات
    if (recentCompletions.size() > 10) {
        recentCompletions.pop_back();
    }
}

// ────────────────────────────────────────────────────────
// تنفيذ نظام البحث والاستبدال
// ────────────────────────────────────────────────────────

ArabicSearchReplace::ArabicSearchReplace()
    : caseSensitive(false), wholeWord(false), useRegex(false) {}

void ArabicSearchReplace::setSearchCriteria(const std::string& pattern, bool caseSens,
                                          bool wholeWrd, bool useReg) {
    searchPattern = pattern;
    caseSensitive = caseSens;
    wholeWord = wholeWrd;
    useRegex = useReg;

    compileRegex();
}

void ArabicSearchReplace::compileRegex() {
    try {
        std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
        if (!caseSensitive) {
            flags |= std::regex_constants::icase;
        }

        if (wholeWord) {
            compiledRegex = std::regex("\\b" + std::regex_replace(searchPattern,
                                        std::regex("([.^$|()\\[\\]{}*+?\\\\])"), "\\$1") + "\\b", flags);
        } else if (useRegex) {
            compiledRegex = std::regex(searchPattern, flags);
        } else {
            std::string escaped = std::regex_replace(searchPattern,
                                std::regex("([.^$|()\\[\\]{}*+?\\\\])"), "\\$1");
            compiledRegex = std::regex(escaped, flags);
        }
    } catch (const std::regex_error&) {
        // خطأ في التعبير النمطي
        compiledRegex = std::regex("");
    }
}

std::vector<std::pair<size_t, size_t>> ArabicSearchReplace::searchInText(const std::string& text) {
    std::vector<std::pair<size_t, size_t>> results;

    try {
        std::sregex_iterator begin(text.begin(), text.end(), compiledRegex);
        std::sregex_iterator end;

        for (std::sregex_iterator i = begin; i != end; ++i) {
            std::smatch match = *i;
            results.emplace_back(match.position(), match.length());
        }
    } catch (const std::regex_error&) {
        // خطأ في البحث
    }

    return results;
}

std::vector<std::pair<size_t, size_t>> ArabicSearchReplace::searchInLines(const std::vector<std::string>& lines) {
    std::vector<std::pair<size_t, size_t>> results;
    size_t offset = 0;

    for (const auto& line : lines) {
        auto lineResults = searchInText(line);
        for (const auto& result : lineResults) {
            results.emplace_back(offset + result.first, result.second);
        }
        offset += line.length() + 1; // +1 للخط الجديد
    }

    return results;
}

std::string ArabicSearchReplace::replaceFirst(const std::string& text, size_t start, size_t end) {
    std::string result = text;
    result.replace(start, end - start, replaceText);
    return result;
}

std::string ArabicSearchReplace::replaceAll(const std::string& text) {
    return std::regex_replace(text, compiledRegex, replaceText);
}

size_t ArabicSearchReplace::getMatchCount() const {
    // يمكن تحسين هذا بحفظ عدد التطابقات من آخر بحث
    return 0;
}

bool ArabicSearchReplace::isValidRegex() const {
    try {
        std::regex test(compiledRegex);
        return true;
    } catch (const std::regex_error&) {
        return false;
    }
}

// ────────────────────────────────────────────────────────
// تنفيذ نظام تحليل الأخطاء
// ────────────────────────────────────────────────────────

ArabicErrorAnalyzer::ArabicErrorAnalyzer() {
    initializeErrorPatterns();
}

void ArabicErrorAnalyzer::initializeErrorPatterns() {
    // أنماط الأخطاء الشائعة
    errorPatterns = {
        "خطأ في البناء",
        "متغير غير معرف",
        "دالة غير موجودة",
        "خطأ في النوع",
        "علامة ترقيم مفقودة",
        "قوس غير مغلق"
    };

    // تحذيرات
    warningPatterns = {
        "متغير غير مستخدم",
        "كود ميت",
        "تحويل نوع ضمني"
    };
}

std::vector<ArabicCodeEditor::CodeError> ArabicErrorAnalyzer::analyzeCode(const std::vector<std::string>& lines) {
    std::vector<ArabicCodeEditor::CodeError> errors;

    for (size_t i = 0; i < lines.size(); ++i) {
        auto lineError = analyzeLine(lines[i], i);
        if (!lineError.message.empty()) {
            errors.push_back(lineError);
        }
    }

    return errors;
}

ArabicCodeEditor::CodeError ArabicErrorAnalyzer::analyzeLine(const std::string& line, size_t lineIndex) {
    // تحليل أساسي للأخطاء الشائعة

    // التحقق من الأقواس
    int bracketCount = 0;
    for (char c : line) {
        if (c == '(') bracketCount++;
        else if (c == ')') bracketCount--;
    }

    if (bracketCount > 0) {
        return {"قوس مفتوح غير مغلق", lineIndex, line.length() - 1, false};
    } else if (bracketCount < 0) {
        return {"قوس إغلاق بدون فتح", lineIndex, 0, false};
    }

    // التحقق من علامات الاقتباس
    int quoteCount = 0;
    for (char c : line) {
        if (c == '"') quoteCount++;
    }

    if (quoteCount % 2 != 0) {
        return {"علامة اقتباس غير مغلقة", lineIndex, line.length() - 1, false};
    }

    // التحقق من نقطة الفاصلة في نهاية العبارات
    if (!line.empty() && line.back() != '{' && line.back() != '}' &&
        line.find("//") == std::string::npos && !line.empty()) {
        // فحص أبسط - قد نحتاج تحسين
        bool needsSemicolon = false;
        if (line.find('=') != std::string::npos ||
            line.find("اكتب") != std::string::npos ||
            line.find("اقرأ") != std::string::npos) {
            needsSemicolon = true;
        }

        if (needsSemicolon && line.back() != ';') {
            return {"نقطة فاصلة مفقودة", lineIndex, line.length() - 1, true};
        }
    }

    return {"", 0, 0, false}; // لا يوجد خطأ
}

void ArabicErrorAnalyzer::addErrorPattern(const std::string& pattern, bool isWarning) {
    if (isWarning) {
        warningPatterns.push_back(pattern);
    } else {
        errorPatterns.push_back(pattern);
    }
}

bool ArabicErrorAnalyzer::validateSyntax(const std::vector<std::string>& lines) {
    // تحقق أساسي من بناء الكود
    int braceCount = 0;

    for (const auto& line : lines) {
        for (char c : line) {
            if (c == '{') braceCount++;
            else if (c == '}') braceCount--;
            if (braceCount < 0) return false;
        }
    }

    return braceCount == 0;
}

std::vector<std::string> ArabicErrorAnalyzer::getFixSuggestions(const ArabicCodeEditor::CodeError& error) {
    std::vector<std::string> suggestions;

    if (error.message.find("قوس") != std::string::npos) {
        suggestions.push_back("أضف قوس إغلاق ')' في نهاية العبارة");
        suggestions.push_back("تحقق من توازن الأقواس في السطر");
    } else if (error.message.find("اقتباس") != std::string::npos) {
        suggestions.push_back("أضف علامة اقتباس '\"' في نهاية النص");
        suggestions.push_back("تحقق من علامات الاقتباس في السطر");
    } else if (error.message.find("نقطة فاصلة") != std::string::npos) {
        suggestions.push_back("أضف ';' في نهاية العبارة");
        suggestions.push_back("تحقق من وجود نقطة الفاصلة في جميع العبارات");
    }

    return suggestions;
}

} // namespace ArabicLanguage
