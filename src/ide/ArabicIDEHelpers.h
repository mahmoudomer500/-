#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <regex>

namespace ArabicLanguage {

struct IDEToken {
    enum Type { TK_KEYWORD, TK_IDENTIFIER, TK_STRING, TK_NUMBER, TK_OPERATOR, TK_COMMENT, TK_FUNCTION, TK_VARIABLE, TK_ERROR, TK_NORMAL };
    Type type;
    std::string text;
    size_t line;
    size_t column;
    unsigned char r, g, b;

    IDEToken(Type t = TK_NORMAL, const std::string& txt = "", size_t ln = 0, size_t col = 0,
             unsigned char rr = 200, unsigned char gg = 200, unsigned char bb = 200)
        : type(t), text(txt), line(ln), column(col), r(rr), g(gg), b(bb) {}
};

struct IDECodeError {
    std::string message;
    size_t line;
    size_t column;
    bool isWarning;

    IDECodeError(const std::string& msg = "", size_t ln = 0, size_t col = 0, bool warning = false)
        : message(msg), line(ln), column(col), isWarning(warning) {}
};

class ArabicSyntaxHighlighter {
public:
    ArabicSyntaxHighlighter();
    std::vector<IDEToken> highlightLine(const std::string& line, size_t lineIndex);
    void addKeyword(const std::string& keyword);
    void addFunction(const std::string& function);

private:
    void initializeArabicKeywords();
    IDEToken::Type getTokenType(const std::string& token) const;

    std::unordered_set<std::string> arabicKeywords;
    std::unordered_set<std::string> arabicFunctions;
    std::unordered_set<std::string> arabicOperators;
};

class ArabicErrorAnalyzer {
public:
    ArabicErrorAnalyzer();
    std::vector<IDECodeError> analyzeCode(const std::vector<std::string>& lines);
    IDECodeError analyzeLine(const std::string& line, size_t lineIndex);
    void addErrorPattern(const std::string& pattern, bool isWarning);
    bool validateSyntax(const std::vector<std::string>& lines);
    std::vector<std::string> getFixSuggestions(const IDECodeError& error);

private:
    void initializeErrorPatterns();
    std::vector<std::string> errorPatterns;
    std::vector<std::string> warningPatterns;
};

} // namespace ArabicLanguage
