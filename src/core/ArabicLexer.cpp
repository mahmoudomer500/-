#include "ArabicLexer.h"
#include <cctype>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstring>

namespace ArabicLanguage {

    // ═══════════════════════════════════════════════════════════
    // البنّاء والدوال المساعدة
    // ═══════════════════════════════════════════════════════════
    
    ArabicLexer::ArabicLexer(const std::string& sourceCode)
        : source(sourceCode), current(0), line(1), column(1) {
        initializeKeywords();
    }
    
    void ArabicLexer::initializeKeywords() {
        // تهيئة الكلمات الرئيسية
        keywords["إذا"] = TokenType::TOK_IF;
        keywords["if"] = TokenType::TOK_IF;
        
        keywords["وإلا"] = TokenType::TOK_ELSE;
        keywords["else"] = TokenType::TOK_ELSE;
        
        keywords["بينما"] = TokenType::TOK_WHILE;
        keywords["while"] = TokenType::TOK_WHILE;
        
        keywords["لـ"] = TokenType::TOK_FOR;
        keywords["for"] = TokenType::TOK_FOR;
        
        keywords["دالة"] = TokenType::TOK_FUNCTION;
        keywords["function"] = TokenType::TOK_FUNCTION;
        keywords["def"] = TokenType::TOK_FUNCTION;
        
        keywords["إرجاع"] = TokenType::TOK_RETURN;
        keywords["return"] = TokenType::TOK_RETURN;
        
        keywords["متغير"] = TokenType::TOK_VAR;
        keywords["var"] = TokenType::TOK_VAR;
        keywords["let"] = TokenType::TOK_VAR;
        
        keywords["ثابت"] = TokenType::TOK_CONST;
        keywords["const"] = TokenType::TOK_CONST;
        
        keywords["صحيح"] = TokenType::TOK_TRUE;
        keywords["true"] = TokenType::TOK_TRUE;
        
        keywords["خطأ"] = TokenType::TOK_FALSE;
        keywords["false"] = TokenType::TOK_FALSE;
        
        keywords["فارغ"] = TokenType::TOK_NULL;
        keywords["null"] = TokenType::TOK_NULL;
        keywords["nil"] = TokenType::TOK_NULL;
        
        keywords["فاصل"] = TokenType::TOK_BREAK;
        keywords["break"] = TokenType::TOK_BREAK;
        
        keywords["استمرار"] = TokenType::TOK_CONTINUE;
        keywords["continue"] = TokenType::TOK_CONTINUE;
        
        keywords["فئة"] = TokenType::TOK_CLASS;
        keywords["class"] = TokenType::TOK_CLASS;
        
        keywords["هيكل"] = TokenType::TOK_STRUCT;
        keywords["struct"] = TokenType::TOK_STRUCT;
        
        keywords["اختر"] = TokenType::TOK_SWITCH;
        keywords["switch"] = TokenType::TOK_SWITCH;
        
        keywords["حالة"] = TokenType::TOK_CASE;
        keywords["case"] = TokenType::TOK_CASE;
        
        keywords["افتراضي"] = TokenType::TOK_DEFAULT;
        keywords["default"] = TokenType::TOK_DEFAULT;
        
        // كلمات العمليات
        keywords["أضف"] = TokenType::TOK_ADD;
        keywords["add"] = TokenType::TOK_ADD;
        keywords["push"] = TokenType::TOK_ADD;
        
        keywords["أزل"] = TokenType::TOK_REMOVE;
        keywords["remove"] = TokenType::TOK_REMOVE;
        keywords["pop"] = TokenType::TOK_REMOVE;
        
        keywords["أدخل"] = TokenType::TOK_INSERT;
        keywords["insert"] = TokenType::TOK_INSERT;
        
        keywords["طول"] = TokenType::TOK_LENGTH;
        keywords["length"] = TokenType::TOK_LENGTH;
        keywords["size"] = TokenType::TOK_LENGTH;
        
        keywords["سعة"] = TokenType::TOK_CAPACITY;
        keywords["capacity"] = TokenType::TOK_CAPACITY;
        
        keywords["اطبع"] = TokenType::TOK_PRINT;
        keywords["print"] = TokenType::TOK_PRINT;
        keywords["puts"] = TokenType::TOK_PRINT;
        
        keywords["إدخال"] = TokenType::TOK_INPUT;
        keywords["input"] = TokenType::TOK_INPUT;
        keywords["read"] = TokenType::TOK_INPUT;
    }
    
    // ═══════════════════════════════════════════════════════════
    // دوال المساعدة الأساسية
    // ═══════════════════════════════════════════════════════════
    
    char ArabicLexer::peek() const {
        if (current >= source.length()) return '\0';
        return source[current];
    }
    
    char ArabicLexer::peekAhead(size_t offset) const {
        if (current + offset >= source.length()) return '\0';
        return source[current + offset];
    }
    
    char ArabicLexer::advance() {
        if (current >= source.length()) return '\0';
        char ch = source[current++];
        if (ch == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        return ch;
    }
    
    void ArabicLexer::skipWhitespace() {
        while (current < source.length()) {
            char ch = peek();
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                advance();
            } else {
                break;
            }
        }
    }
    
    void ArabicLexer::skipComment() {
        // تخطي تعليق بسطر واحد (// أو #)
        if (peek() == '/' && peekAhead() == '/') {
            advance(); advance();
            while (current < source.length() && peek() != '\n') {
                advance();
            }
        } else if (peek() == '#') {
            advance();
            while (current < source.length() && peek() != '\n') {
                advance();
            }
        }
    }
    
    void ArabicLexer::skipBlockComment() {
        // تخطي تعليق كتلة (/* ... */)
        if (peek() == '/' && peekAhead() == '*') {
            advance(); advance();
            while (current < source.length() - 1) {
                if (peek() == '*' && peekAhead() == '/') {
                    advance(); advance();
                    return;
                }
                advance();
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // فحص السلاسل النصية
    // ═══════════════════════════════════════════════════════════
    
    Token ArabicLexer::scanString(char delimiter) {
        uint32_t startLine = line;
        uint32_t startCol = column;
        uint32_t startOffset = current;
        
        advance(); // تخطي الفاتحة
        std::string value;
        
        while (current < source.length() && peek() != delimiter) {
            if (peek() == '\\') {
                advance();
                if (current < source.length()) {
                    char escaped = advance();
                    switch (escaped) {
                        case 'n': value += '\n'; break;
                        case 't': value += '\t'; break;
                        case 'r': value += '\r'; break;
                        case '\\': value += '\\'; break;
                        case '"': value += '"'; break;
                        case '\'': value += '\''; break;
                        default: value += escaped; break;
                    }
                }
            } else {
                value += advance();
            }
        }
        
        if (current < source.length()) {
            advance(); // تخطي الإغلاق
        }
        
        return Token(TokenType::TOK_STRING, value, startLine, startCol, startOffset);
    }
    
    // ═══════════════════════════════════════════════════════════
    // فحص الأرقام
    // ═══════════════════════════════════════════════════════════
    
    Token ArabicLexer::scanNumber() {
        uint32_t startLine = line;
        uint32_t startCol = column;
        uint32_t startOffset = current;
        
        std::string value;
        
        // فحص الجزء الصحيح
        while (current < source.length() && std::isdigit(static_cast<unsigned char>(peek()))) {
            value += advance();
        }
        
        // فحص العلامة العشرية
        if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekAhead()))) {
            value += advance(); // .
            while (current < source.length() && std::isdigit(static_cast<unsigned char>(peek()))) {
                value += advance();
            }
        }
        
        // فحص الترميز العلمي
        if (peek() == 'e' || peek() == 'E') {
            value += advance(); // e
            if (peek() == '+' || peek() == '-') {
                value += advance();
            }
            while (current < source.length() && std::isdigit(static_cast<unsigned char>(peek()))) {
                value += advance();
            }
        }
        
        return Token(TokenType::TOK_NUMBER, value, startLine, startCol, startOffset);
    }
    
    // ═══════════════════════════════════════════════════════════
    // فحص المعرّفات والكلمات الرئيسية
    // ═══════════════════════════════════════════════════════════
    
    Token ArabicLexer::scanIdentifier() {
        uint32_t startLine = line;
        uint32_t startCol = column;
        uint32_t startOffset = current;
        
        std::string value;
        
        // دعم النصوص العربية والإنجليزية
        while (current < source.length()) {
            char ch = peek();
            if (std::isalnum(ch) || ch == '_' || (unsigned char)ch >= 128) { // دعم الأحرف العربية
                value += advance();
            } else {
                break;
            }
        }
        
        // التحقق من الكلمات الرئيسية
        TokenType type = getKeywordType(value);
        if (type != TokenType::TOK_IDENTIFIER) {
            return Token(type, value, startLine, startCol, startOffset);
        }
        
        return Token(TokenType::TOK_IDENTIFIER, value, startLine, startCol, startOffset);
    }
    
    // ═══════════════════════════════════════════════════════════
    // فحص العوامل
    // ═══════════════════════════════════════════════════════════
    
    Token ArabicLexer::scanOperator() {
        uint32_t startLine = line;
        uint32_t startCol = column;
        uint32_t startOffset = current;
        
        char ch = advance();
        std::string value;
        value += ch;
        
        // العوامل المركبة
        if (ch == '+') {
            if (peek() == '+') {
                value += advance();
                return Token(TokenType::TOK_INCREMENT, value, startLine, startCol, startOffset);
            } else if (peek() == '=') {
                value += advance();
                return Token(TokenType::TOK_PLUS_ASSIGN, value, startLine, startCol, startOffset);
            }
            return Token(TokenType::TOK_PLUS, value, startLine, startCol, startOffset);
        }
        
        if (ch == '-') {
            if (peek() == '-') {
                value += advance();
                return Token(TokenType::TOK_DECREMENT, value, startLine, startCol, startOffset);
            } else if (peek() == '=') {
                value += advance();
                return Token(TokenType::TOK_MINUS_ASSIGN, value, startLine, startCol, startOffset);
            } else if (peek() == '>') {
                value += advance();
                return Token(TokenType::TOK_ARROW, value, startLine, startCol, startOffset);
            }
            return Token(TokenType::TOK_MINUS, value, startLine, startCol, startOffset);
        }
        
        if (ch == '*') {
            if (peek() == '=') {
                value += advance();
                return Token(TokenType::TOK_MULT_ASSIGN, value, startLine, startCol, startOffset);
            }
            return Token(TokenType::TOK_MULTIPLY, value, startLine, startCol, startOffset);
        }
        
        if (ch == '/') {
            if (peek() == '=') {
                value += advance();
                return Token(TokenType::TOK_DIV_ASSIGN, value, startLine, startCol, startOffset);
            }
            return Token(TokenType::TOK_DIVIDE, value, startLine, startCol, startOffset);
        }
        
        if (ch == '%') {
            if (peek() == '=') {
                value += advance();
                return Token(TokenType::TOK_MOD_ASSIGN, value, startLine, startCol, startOffset);
            }
            return Token(TokenType::TOK_MODULO, value, startLine, startCol, startOffset);
        }
        
        if (ch == '=') {
            if (peek() == '=') {
                value += advance();
                return Token(TokenType::TOK_EQ, value, startLine, startCol, startOffset);
            }
            return Token(TokenType::TOK_ASSIGN, value, startLine, startCol, startOffset);
        }
        
        if (ch == '!') {
            if (peek() == '=') {
                value += advance();
                return Token(TokenType::TOK_NE, value, startLine, startCol, startOffset);
            }
            return Token(TokenType::TOK_NOT, value, startLine, startCol, startOffset);
        }
        
        if (ch == '<') {
            if (peek() == '<') {
                value += advance();
                return Token(TokenType::TOK_LSHIFT, value, startLine, startCol, startOffset);
            } else if (peek() == '=') {
                value += advance();
                return Token(TokenType::TOK_LE, value, startLine, startCol, startOffset);
            }
            return Token(TokenType::TOK_LT, value, startLine, startCol, startOffset);
        }
        
        if (ch == '>') {
            if (peek() == '>') {
                value += advance();
                return Token(TokenType::TOK_RSHIFT, value, startLine, startCol, startOffset);
            } else if (peek() == '=') {
                value += advance();
                return Token(TokenType::TOK_GE, value, startLine, startCol, startOffset);
            }
            return Token(TokenType::TOK_GT, value, startLine, startCol, startOffset);
        }
        
        if (ch == '&') {
            if (peek() == '&') {
                value += advance();
                return Token(TokenType::TOK_AND, value, startLine, startCol, startOffset);
            }
            return Token(TokenType::TOK_BITWISE_AND, value, startLine, startCol, startOffset);
        }
        
        if (ch == '|') {
            if (peek() == '|') {
                value += advance();
                return Token(TokenType::TOK_OR, value, startLine, startCol, startOffset);
            }
            return Token(TokenType::TOK_BITWISE_OR, value, startLine, startCol, startOffset);
        }
        
        if (ch == '^') {
            return Token(TokenType::TOK_POWER, value, startLine, startCol, startOffset);
        }
        
        if (ch == '~') {
            return Token(TokenType::TOK_BITWISE_NOT, value, startLine, startCol, startOffset);
        }
        
        if (ch == '.') {
            return Token(TokenType::TOK_DOT, value, startLine, startCol, startOffset);
        }
        
        if (ch == ',') {
            return Token(TokenType::TOK_COMMA, value, startLine, startCol, startOffset);
        }
        
        if (ch == ';') {
            return Token(TokenType::TOK_SEMICOLON, value, startLine, startCol, startOffset);
        }
        
        if (ch == ':') {
            return Token(TokenType::TOK_COLON, value, startLine, startCol, startOffset);
        }
        
        if (ch == '?') {
            return Token(TokenType::TOK_QUESTION, value, startLine, startCol, startOffset);
        }
        
        return Token(TokenType::TOK_ERROR, value, startLine, startCol, startOffset);
    }
    
    // ═══════════════════════════════════════════════════════════
    // الدوال الرئيسية
    // ═══════════════════════════════════════════════════════════
    
    TokenType ArabicLexer::getKeywordType(const std::string& word) {
        auto it = keywords.find(word);
        if (it != keywords.end()) {
            return it->second;
        }
        return TokenType::TOK_IDENTIFIER;
    }
    
    Token ArabicLexer::nextToken() {
        skipWhitespace();
        
        // معالجة التعليقات
        while (peek() == '/' && peekAhead() == '/') {
            skipComment();
            skipWhitespace();
        }
        
        while (peek() == '/' && peekAhead() == '*') {
            skipBlockComment();
            skipWhitespace();
        }
        
        if (peek() == '#') {
            skipComment();
            skipWhitespace();
        }
        
        // نهاية الملف
        if (current >= source.length()) {
            return Token(TokenType::TOK_EOF, "", line, column, current);
        }
        
        uint32_t startLine = line;
        uint32_t startCol = column;
        uint32_t startOffset = current;
        char ch = peek();
        
        // السلاسل النصية
        if (ch == '"' || ch == '\'') {
            return scanString(ch);
        }
        
        // الأرقام
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            return scanNumber();
        }
        
        // المعرّفات والكلمات الرئيسية
        if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_' || (unsigned char)ch >= 128) {
            return scanIdentifier();
        }
        
        // الأقواس
        if (ch == '(') {
            advance();
            return Token(TokenType::TOK_LPAREN, "(", startLine, startCol, startOffset);
        }
        if (ch == ')') {
            advance();
            return Token(TokenType::TOK_RPAREN, ")", startLine, startCol, startOffset);
        }
        if (ch == '{') {
            advance();
            return Token(TokenType::TOK_LBRACE, "{", startLine, startCol, startOffset);
        }
        if (ch == '}') {
            advance();
            return Token(TokenType::TOK_RBRACE, "}", startLine, startCol, startOffset);
        }
        if (ch == '[') {
            advance();
            return Token(TokenType::TOK_LBRACKET, "[", startLine, startCol, startOffset);
        }
        if (ch == ']') {
            advance();
            return Token(TokenType::TOK_RBRACKET, "]", startLine, startCol, startOffset);
        }
        
        // العوامل والرموز الأخرى
        return scanOperator();
    }
    
    std::vector<Token> ArabicLexer::tokenize() {
        tokens.clear();
        
        while (true) {
            Token token = nextToken();
            tokens.push_back(token);
            
            if (token.type == TokenType::TOK_EOF) {
                break;
            }
        }
        
        return tokens;
    }
    
    const Token& ArabicLexer::getToken(size_t index) const {
        if (index >= tokens.size()) {
            throw std::out_of_range("Token index out of range");
        }
        return tokens[index];
    }
    
    bool ArabicLexer::hasErrors() const {
        for (const auto& token : tokens) {
            if (token.type == TokenType::TOK_ERROR) {
                return true;
            }
        }
        return false;
    }
    
    void ArabicLexer::reportError(const std::string& message, uint32_t l, uint32_t c) {
        std::cerr << "خطأ في السطر " << l << " العمود " << c << ": " << message << std::endl;
    }

} // namespace ArabicLanguage
