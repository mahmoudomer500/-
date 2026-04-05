#ifndef ARABIC_LEXER_H
#define ARABIC_LEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>

namespace ArabicLanguage {

    // ═══════════════════════════════════════════════════════════
    // أنواع الرموز (Token Types)
    // ═══════════════════════════════════════════════════════════
    
    enum class TokenType {
        // الرموز الأساسية
        TOK_EOF = 0,
        TOK_ERROR = 1,
        
        // الكلمات الرئيسية
        TOK_IF = 10,           // إذا
        TOK_ELSE = 11,         // وإلا
        TOK_WHILE = 12,        // بينما
        TOK_FOR = 13,          // لـ
        TOK_FUNCTION = 14,     // دالة
        TOK_RETURN = 15,       // إرجاع
        TOK_VAR = 16,          // متغير
        TOK_CONST = 17,        // ثابت
        TOK_TRUE = 18,         // صحيح
        TOK_FALSE = 19,        // خطأ
        TOK_NULL = 20,         // فارغ
        TOK_BREAK = 21,        // فاصل
        TOK_CONTINUE = 22,     // استمرار
        TOK_CLASS = 23,        // فئة
        TOK_STRUCT = 24,       // هيكل
        TOK_ENUM = 25,         // تعداد
        TOK_IMPORT = 26,       // استيراد
        TOK_EXPORT = 27,       // تصدير
        TOK_ASYNC = 28,        // متزامن
        TOK_AWAIT = 29,        // انتظر
        TOK_TRY = 30,          // حاول
        TOK_CATCH = 31,        // أمسك
        TOK_FINALLY = 32,      // أخيراً
        TOK_THROW = 33,        // رمي
        TOK_STATIC = 34,       // ثابت
        TOK_PUBLIC = 35,       // عام
        TOK_PRIVATE = 36,      // خاص
        TOK_PROTECTED = 37,    // محمي
        TOK_SWITCH = 38,       // اختر
        TOK_CASE = 39,         // حالة
        TOK_DEFAULT = 40,      // افتراضي
        
        // الكلمات الحسابية
        TOK_ADD = 50,          // أضف
        TOK_REMOVE = 51,       // أزل
        TOK_INSERT = 52,       // أدخل
        TOK_LENGTH = 53,       // طول
        TOK_CAPACITY = 54,     // سعة
        TOK_PRINT = 55,        // اطبع
        TOK_INPUT = 56,        // إدخال
        
        // المعرفات والثوابت
        TOK_IDENTIFIER = 100,  // معرّف
        TOK_NUMBER = 101,      // رقم
        TOK_STRING = 102,      // نص/سلسلة
        TOK_CHAR = 103,        // حرف
        
        // العوامل (Operators)
        TOK_PLUS = 200,        // +
        TOK_MINUS = 201,       // -
        TOK_MULTIPLY = 202,    // *
        TOK_DIVIDE = 203,      // /
        TOK_MODULO = 204,      // %
        TOK_POWER = 205,       // ^
        TOK_ASSIGN = 206,      // =
        TOK_PLUS_ASSIGN = 207, // +=
        TOK_MINUS_ASSIGN = 208,// -=
        TOK_MULT_ASSIGN = 209, // *=
        TOK_DIV_ASSIGN = 210,  // /=
        TOK_MOD_ASSIGN = 211,  // %=
        TOK_EQ = 212,          // ==
        TOK_NE = 213,          // !=
        TOK_LT = 214,          // <
        TOK_LE = 215,          // <=
        TOK_GT = 216,          // >
        TOK_GE = 217,          // >=
        TOK_AND = 218,         // و (&&)
        TOK_OR = 219,          // أو (||)
        TOK_NOT = 220,         // ليس (!)
        TOK_BITWISE_AND = 221, // &
        TOK_BITWISE_OR = 222,  // |
        TOK_BITWISE_XOR = 223, // ^
        TOK_BITWISE_NOT = 224, // ~
        TOK_LSHIFT = 225,      // <<
        TOK_RSHIFT = 226,      // >>
        TOK_INCREMENT = 227,   // ++
        TOK_DECREMENT = 228,   // --
        TOK_ARROW = 229,       // =>
        TOK_DOT = 230,         // .
        TOK_COMMA = 231,       // ,
        TOK_SEMICOLON = 232,   // ;
        TOK_COLON = 233,       // :
        TOK_QUESTION = 234,    // ?
        
        // الأقواس والفواصل
        TOK_LPAREN = 300,      // (
        TOK_RPAREN = 301,      // )
        TOK_LBRACE = 302,      // {
        TOK_RBRACE = 303,      // }
        TOK_LBRACKET = 304,    // [
        TOK_RBRACKET = 305,    // ]
        
        // التعليقات
        TOK_COMMENT = 400,     // تعليق
        TOK_BLOCK_COMMENT = 401,// تعليق كتلة
    };
    
    // ═══════════════════════════════════════════════════════════
    // بنية الرمز (Token Structure)
    // ═══════════════════════════════════════════════════════════
    
    struct Token {
        TokenType type;
        std::string value;      // قيمة الرمز
        std::string raw;        // القيمة الأصلية
        uint32_t line;          // رقم السطر
        uint32_t column;        // رقم العمود
        uint32_t offset;        // إزاحة في الملف
        
        Token() : type(TokenType::TOK_ERROR), line(0), column(0), offset(0) {}
        
        explicit Token(TokenType t, const std::string& v = "", uint32_t l = 0, uint32_t c = 0, uint32_t o = 0)
            : type(t), value(v), raw(v), line(l), column(c), offset(o) {}
        
        explicit Token(TokenType t, const char* v, uint32_t l, uint32_t c, uint32_t o)
            : type(t), value(v ? v : ""), raw(value), line(l), column(c), offset(o) {}
    };
    
    // ═══════════════════════════════════════════════════════════
    // المحلل اللغوي (Lexer)
    // ═══════════════════════════════════════════════════════════
    
    class ArabicLexer {
    private:
        std::string source;         // الكود المصدري
        size_t current;             // الموضع الحالي
        uint32_t line;              // السطر الحالي
        uint32_t column;            // العمود الحالي
        std::vector<Token> tokens;  // قائمة الرموز
        std::unordered_map<std::string, TokenType> keywords; // الكلمات الرئيسية
        
        // الدوال المساعدة
        char peek() const;
        char peekAhead(size_t offset = 1) const;
        char advance();
        void skipWhitespace();
        void skipComment();
        void skipBlockComment();
        
        Token scanString(char delimiter);
        Token scanNumber();
        Token scanIdentifier();
        Token scanOperator();
        
        void initializeKeywords();
        TokenType getKeywordType(const std::string& word);
        
    public:
        ArabicLexer(const std::string& sourceCode);
        
        // الدوال الرئيسية
        std::vector<Token> tokenize();
        Token nextToken();
        
        // معلومات التحليل
        size_t getTokenCount() const { return tokens.size(); }
        const Token& getToken(size_t index) const;
        const std::vector<Token>& getTokens() const { return tokens; }
        
        // الأخطاء
        void reportError(const std::string& message, uint32_t line, uint32_t column);
        bool hasErrors() const;
    };

} // namespace ArabicLanguage

#endif // ARABIC_LEXER_H
