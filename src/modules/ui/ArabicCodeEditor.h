#pragma once

#include "ArabicUI.h"
#include "../../core/ArabicText.h"
#include "../graphics/ArabicGameEngine.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <memory>

namespace ArabicLanguage {

/**
 * @brief محرر الكود العربي - Arabic Code Editor
 *
 * بيئة تطوير متكاملة لكتابة وتحرير كود اللغة العربية
 * مع تمييز الصيغة وإكمال تلقائي وتصحيح أخطاء
 */
class ArabicCodeEditor {
public:
    ArabicCodeEditor();
    ~ArabicCodeEditor() = default;

    /**
     * @brief حالة المحرر
     */
    enum EditorState {
        EDITING,
        SEARCHING,
        REPLACING,
        ERROR_DISPLAY
    };

    /**
     * @brief نوع الرمز (Token Type)
     */
    enum TokenType {
        TOKEN_KEYWORD,        // كلمات مفتاحية
        TOKEN_IDENTIFIER,     // معرفات
        TOKEN_STRING,         // سلاسل نصية
        TOKEN_NUMBER,         // أرقام
        TOKEN_OPERATOR,       // عمليات
        TOKEN_COMMENT,        // تعليقات
        TOKEN_FUNCTION,       // دوال
        TOKEN_VARIABLE,       // متغيرات
        TOKEN_ERROR,          // أخطاء
        TOKEN_NORMAL          // نص عادي
    };

    /**
     * @brief رمز مميز (Token)
     */
    struct Token {
        TokenType type;
        std::string text;
        size_t line;
        size_t column;
        ArabicUI::UIColor color;

        Token(TokenType t = TOKEN_NORMAL, const std::string& txt = "",
              size_t ln = 0, size_t col = 0, const ArabicUI::UIColor& c = ArabicUI::UIColor(0, 0, 0))
            : type(t), text(txt), line(ln), column(col), color(c) {}
    };

    /**
     * @brief اقتراح إكمال تلقائي
     */
    struct AutoCompleteSuggestion {
        std::string text;
        std::string description;
        TokenType type;

        AutoCompleteSuggestion(const std::string& t = "", const std::string& d = "",
                              TokenType typ = TOKEN_IDENTIFIER)
            : text(t), description(d), type(typ) {}
    };

    /**
     * @brief خطأ في الكود
     */
    struct CodeError {
        std::string message;
        size_t line;
        size_t column;
        bool isWarning;

        CodeError(const std::string& msg = "", size_t ln = 0, size_t col = 0,
                 bool warning = false)
            : message(msg), line(ln), column(col), isWarning(warning) {}
    };

    /**
     * @brief تشغيل الكود
     */
    void runCode();

    /**
     * @brief تشغيل المحرر
     */
    void run();

    /**
     * @brief تهيئة المحرر
     */
    bool initialize(HINSTANCE hInstance);

    /**
     * @brief إيقاف المحرر
     */
    void stop();

    /**
     * @brief فتح ملف
     */
    bool openFile(const std::string& filePath);

    /**
     * @brief حفظ ملف
     */
    bool saveFile(const std::string& filePath = "");

    /**
     * @brief إنشاء ملف جديد
     */
    void newFile();

    /**
     * @brief الحصول على النص الكامل
     */
    std::string getText() const;

    /**
     * @brief تعيين النص الكامل
     */
    void setText(const std::string& text);

    /**
     * @brief تعيين موقع المؤشر
     */
    void setCursorPosition(size_t line, size_t column);

    /**
     * @brief تعيين حالة المحرر
     */
    void setState(EditorState state) { currentState = state; }

private:
    std::shared_ptr<ArabicGameEngine> engine;
    std::shared_ptr<ArabicUI> ui;
    std::shared_ptr<ArabicText> textRenderer;

    // حالة المحرر
    EditorState currentState;
    std::string currentFilePath;
    bool hasUnsavedChanges;

    // محتوى الملف
    std::vector<std::string> lines;
    size_t cursorLine;
    size_t cursorColumn;
    size_t scrollLine;
    size_t scrollColumn;

    // تمييز الصيغة
    std::vector<std::vector<Token>> tokenizedLines;
    std::unordered_set<std::string> keywords;
    std::unordered_map<std::string, TokenType> functions;
    std::unordered_map<std::string, TokenType> operators;

    // إكمال تلقائي
    std::vector<AutoCompleteSuggestion> suggestions;
    std::string currentWord;
    bool showSuggestions;
    int selectedSuggestion;

    // البحث والاستبدال
    std::string searchText;
    std::string replaceText;
    bool caseSensitive;
    bool wholeWord;
    bool useRegex;
    std::vector<std::pair<size_t, size_t>> searchResults;
    size_t currentSearchResult;

    // أخطاء الكود
    std::vector<CodeError> errors;

    // إعدادات العرض
    float fontSize;
    float lineHeight;
    ArabicUI::UIColor backgroundColor;
    ArabicUI::UIColor textColor;
    ArabicUI::UIColor selectionColor;
    ArabicUI::UIColor cursorColor;

    // UI Elements
    std::shared_ptr<ArabicUI::UITextArea> codeArea;
    std::shared_ptr<ArabicUI::UIButton> newButton;
    std::shared_ptr<ArabicUI::UIButton> openButton;
    std::shared_ptr<ArabicUI::UIButton> saveButton;
    std::shared_ptr<ArabicUI::UIButton> runButton;
    std::shared_ptr<ArabicUI::UIButton> searchButton;
    std::shared_ptr<ArabicUI::UIButton> replaceButton;
    std::shared_ptr<ArabicUI::UITextBox> searchInput;
    std::shared_ptr<ArabicUI::UITextBox> replaceInput;
    std::shared_ptr<ArabicUI::UILabel> statusLabel;
    std::shared_ptr<ArabicUI::UILabel> lineColumnLabel;

    // Auto-complete popup
    std::vector<std::shared_ptr<ArabicUI::UIButton>> suggestionButtons;

    /**
     * @brief إنشاء واجهة المستخدم
     */
    void createUI();

    /**
     * @brief تهيئة الكلمات المفتاحية والدوال
     */
    void initializeKeywords();

    /**
     * @brief تحليل النص وتقسيمه إلى رموز
     */
    void tokenizeText();

    /**
     * @brief تحليل سطر واحد
     */
    std::vector<Token> tokenizeLine(const std::string& line, size_t lineIndex);

    /**
     * @brief الحصول على لون الرمز
     */
    ArabicUI::UIColor getTokenColor(TokenType type) const;

    /**
     * @brief تحديث الإكمال التلقائي
     */
    void updateAutoComplete();

    /**
     * @brief الحصول على الكلمة الحالية تحت المؤشر
     */
    std::string getCurrentWord() const;

    /**
     * @brief إدراج نص في موقع المؤشر
     */
    void insertText(const std::string& text);

    /**
     * @brief حذف نص في موقع المؤشر
     */
    void deleteText(bool deleteForward);

    /**
     * @brief معالجة إدخال لوحة المفاتيح
     */
    void handleKeyInput(const class ArabicInput& input);

    /**
     * @brief إنشاء قائمة الاقتراحات
     */
    void generateSuggestions(const std::string& prefix);

    /**
     * @brief البحث في النص
     */
    void performSearch();

    /**
     * @brief استبدال النص
     */
    void performReplace();

    /**
     * @brief تحليل الأخطاء في الكود
     */
    void analyzeErrors();

    /**
     * @brief تحديث تسمية الحالة
     */
    void updateStatusBar();

    /**
     * @brief تحميل ملف
     */
    bool loadFile(const std::string& filePath);

    /**
     * @brief رسم منطقة الكود
     */
    void drawCodeArea(class ArabicGraphics& graphics);

    /**
     * @brief رسم سطر الخطأ
     */
    void drawErrorLine(class ArabicGraphics& graphics, size_t lineIndex, const std::string& message);

    /**
     * @brief رسم سطر مع تمييز الصيغة
     */
    void drawLineWithSyntaxHighlighting(class ArabicGraphics& graphics, size_t lineIndex,
                                      glm::vec2 position, float maxWidth);

    /**
     * @brief رسم قائمة الإكمال التلقائي
     */
    void drawAutoComplete(class ArabicGraphics& graphics);
};

// ────────────────────────────────────────────────────────
// مساعدات محرر الكود العربي
// ────────────────────────────────────────────────────────

/**
 * @brief مساعد تمييز الصيغة
 */
class ArabicSyntaxHighlighter {
public:
    ArabicSyntaxHighlighter();
    std::vector<ArabicCodeEditor::Token> highlightLine(const std::string& line, size_t lineIndex);
    std::vector<std::vector<ArabicCodeEditor::Token>> highlightFile(const std::vector<std::string>& lines);
    void addKeyword(const std::string& keyword);
    void addFunction(const std::string& function);

private:
    void initializeArabicKeywords();
    ArabicCodeEditor::TokenType getTokenType(const std::string& token) const;

    std::unordered_set<std::string> arabicKeywords;
    std::unordered_set<std::string> arabicFunctions;
    std::unordered_set<std::string> arabicOperators;
    std::regex stringRegex;
    std::regex numberRegex;
    std::regex commentRegex;
    std::regex functionRegex;
};

/**
 * @brief مساعد الإكمال التلقائي
 */
class ArabicAutoComplete {
public:
    ArabicAutoComplete();
    std::vector<ArabicCodeEditor::AutoCompleteSuggestion> getSuggestions(const std::string& prefix);
    void addSuggestion(const std::string& text, const std::string& description,
                      ArabicCodeEditor::TokenType type);
    void updateContextSuggestions(const std::vector<std::string>& currentLines,
                                 size_t cursorLine, size_t cursorColumn);
    ArabicCodeEditor::AutoCompleteSuggestion getBestSuggestion(const std::string& prefix);
    void recordCompletion(const std::string& completion);

private:
    void initializeBasicSuggestions();
    std::unordered_map<std::string, ArabicCodeEditor::AutoCompleteSuggestion> suggestions;
    std::vector<std::string> recentCompletions;
};

/**
 * @brief مساعد البحث والاستبدال
 */
class ArabicSearchReplace {
public:
    ArabicSearchReplace();
    void setSearchCriteria(const std::string& pattern, bool caseSens,
                          bool wholeWrd, bool useReg);
    std::vector<std::pair<size_t, size_t>> searchInText(const std::string& text);
    std::vector<std::pair<size_t, size_t>> searchInLines(const std::vector<std::string>& lines);
    std::string replaceFirst(const std::string& text, size_t start, size_t end);
    std::string replaceAll(const std::string& text);
    size_t getMatchCount() const;
    bool isValidRegex() const;

private:
    void compileRegex();
    std::string searchPattern;
    std::string replaceText;
    bool caseSensitive;
    bool wholeWord;
    bool useRegex;
    std::regex compiledRegex;
};

/**
     * @brief مساعد تحليل الأخطاء
     */
    class ArabicErrorAnalyzer {
    public:
        ArabicErrorAnalyzer();
        std::vector<ArabicCodeEditor::CodeError> analyzeCode(const std::vector<std::string>& lines);
        ArabicCodeEditor::CodeError analyzeLine(const std::string& line, size_t lineIndex);
        void addErrorPattern(const std::string& pattern, bool isWarning);
        bool validateSyntax(const std::vector<std::string>& lines);
        std::vector<std::string> getFixSuggestions(const ArabicCodeEditor::CodeError& error);

    private:
        void initializeErrorPatterns();
        std::vector<std::string> errorPatterns;
        std::vector<std::string> warningPatterns;
    };

} // namespace ArabicLanguage

