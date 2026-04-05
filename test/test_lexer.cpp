#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "../core/ArabicLexer.h"

using namespace ArabicLanguage;

// ═══════════════════════════════════════════════════════════
// برنامج اختبار المحلل اللغوي
// ═══════════════════════════════════════════════════════════

void printTokens(const std::vector<Token>& tokens) {
    std::cout << "\n════════════════════════════════════════\n";
    std::cout << "🔍 الرموز المُكتشفة:\n";
    std::cout << "════════════════════════════════════════\n";
    
    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& token = tokens[i];
        std::string typeName;
        
        // تحويل نوع الرمز إلى نص
        switch (token.type) {
            case TokenType::TOK_EOF: typeName = "EOF"; break;
            case TokenType::TOK_IF: typeName = "IF (إذا)"; break;
            case TokenType::TOK_ELSE: typeName = "ELSE (وإلا)"; break;
            case TokenType::TOK_WHILE: typeName = "WHILE (بينما)"; break;
            case TokenType::TOK_FOR: typeName = "FOR (لـ)"; break;
            case TokenType::TOK_FUNCTION: typeName = "FUNCTION (دالة)"; break;
            case TokenType::TOK_RETURN: typeName = "RETURN (إرجاع)"; break;
            case TokenType::TOK_VAR: typeName = "VAR (متغير)"; break;
            case TokenType::TOK_IDENTIFIER: typeName = "IDENTIFIER"; break;
            case TokenType::TOK_NUMBER: typeName = "NUMBER"; break;
            case TokenType::TOK_STRING: typeName = "STRING"; break;
            case TokenType::TOK_PLUS: typeName = "PLUS (+)"; break;
            case TokenType::TOK_MINUS: typeName = "MINUS (-)"; break;
            case TokenType::TOK_MULTIPLY: typeName = "MULTIPLY (*)"; break;
            case TokenType::TOK_DIVIDE: typeName = "DIVIDE (/)"; break;
            case TokenType::TOK_ASSIGN: typeName = "ASSIGN (=)"; break;
            case TokenType::TOK_EQ: typeName = "EQ (==)"; break;
            case TokenType::TOK_LT: typeName = "LT (<)"; break;
            case TokenType::TOK_GT: typeName = "GT (>)"; break;
            case TokenType::TOK_LPAREN: typeName = "LPAREN (()"; break;
            case TokenType::TOK_RPAREN: typeName = "RPAREN ())"; break;
            case TokenType::TOK_LBRACE: typeName = "LBRACE ({)"; break;
            case TokenType::TOK_RBRACE: typeName = "RBRACE (})"; break;
            case TokenType::TOK_LBRACKET: typeName = "LBRACKET ([)"; break;
            case TokenType::TOK_RBRACKET: typeName = "RBRACKET (])"; break;
            case TokenType::TOK_SEMICOLON: typeName = "SEMICOLON (;)"; break;
            case TokenType::TOK_COMMA: typeName = "COMMA (,)"; break;
            case TokenType::TOK_DOT: typeName = "DOT (.)"; break;
            case TokenType::TOK_ADD: typeName = "ADD (أضف)"; break;
            case TokenType::TOK_REMOVE: typeName = "REMOVE (أزل)"; break;
            case TokenType::TOK_LENGTH: typeName = "LENGTH (طول)"; break;
            case TokenType::TOK_PRINT: typeName = "PRINT (اطبع)"; break;
            default: typeName = "UNKNOWN"; break;
        }
        
        std::cout << "  [" << i << "] " << typeName;
        if (!token.value.empty() && token.type != TokenType::TOK_EOF) {
            std::cout << " -> \"" << token.value << "\"";
        }
        std::cout << " (Line: " << token.line << ", Col: " << token.column << ")\n";
    }
    std::cout << "════════════════════════════════════════\n";
}

// ─────────────────────────────────────────────────────────
// اختبار 1: رموز بسيطة
// ─────────────────────────────────────────────────────────

void test1_SimpleTokens() {
    std::cout << "\n✅ Test 1: رموز بسيطة\n";
    std::string code = "متغير x = 5;";
    
    ArabicLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    assert(tokens.size() >= 5);
    assert(tokens[0].type == TokenType::TOK_VAR);
    assert(tokens[1].type == TokenType::TOK_IDENTIFIER);
    assert(tokens[1].value == "x");
    assert(tokens[2].type == TokenType::TOK_ASSIGN);
    assert(tokens[3].type == TokenType::TOK_NUMBER);
    assert(tokens[3].value == "5");
    
    printTokens(tokens);
    std::cout << "✅ اجتياز\n";
}

// ─────────────────────────────────────────────────────────
// اختبار 2: العمليات على المصفوفات
// ─────────────────────────────────────────────────────────

void test2_ArrayOperations() {
    std::cout << "\n✅ Test 2: عمليات على المصفوفات\n";
    std::string code = "arr.أضف(10); arr.أزل(0); x = arr.طول();";
    
    ArabicLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    // التحقق من وجود العمليات
    bool hasAdd = false, hasRemove = false, hasLength = false;
    for (const auto& token : tokens) {
        if (token.type == TokenType::TOK_ADD) hasAdd = true;
        if (token.type == TokenType::TOK_REMOVE) hasRemove = true;
        if (token.type == TokenType::TOK_LENGTH) hasLength = true;
    }
    
    assert(hasAdd && "يجب أن يجد عملية أضف");
    assert(hasRemove && "يجب أن يجد عملية أزل");
    assert(hasLength && "يجب أن يجد عملية طول");
    
    printTokens(tokens);
    std::cout << "✅ اجتياز\n";
}

// ─────────────────────────────────────────────────────────
// اختبار 3: البنى التحكمية
// ─────────────────────────────────────────────────────────

void test3_ControlStructures() {
    std::cout << "\n✅ Test 3: البنى التحكمية\n";
    std::string code = R"(
        إذا (x > 5) {
            اطبع("أكبر من 5");
        } وإلا {
            اطبع("أقل من أو يساوي 5");
        }
    )";
    
    ArabicLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    bool hasIf = false, hasElse = false, hasString = false;
    for (const auto& token : tokens) {
        if (token.type == TokenType::TOK_IF) hasIf = true;
        if (token.type == TokenType::TOK_ELSE) hasElse = true;
        if (token.type == TokenType::TOK_STRING) hasString = true;
    }
    
    assert(hasIf && "يجب أن يجد إذا");
    assert(hasElse && "يجب أن يجد وإلا");
    assert(hasString && "يجب أن يجد نص");
    
    printTokens(tokens);
    std::cout << "✅ اجتياز\n";
}

// ─────────────────────────────────────────────────────────
// اختبار 4: الحلقات والدوال
// ─────────────────────────────────────────────────────────

void test4_LoopsAndFunctions() {
    std::cout << "\n✅ Test 4: الحلقات والدوال\n";
    std::string code = R"(
        دالة جمع(a, b) {
            إرجاع a + b;
        }
        
        لـ (i = 0; i < 10; i++) {
            بينما (x > 0) {
                x = x - 1;
            }
        }
    )";
    
    ArabicLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    bool hasFunction = false, hasReturn = false, hasFor = false, hasWhile = false;
    for (const auto& token : tokens) {
        if (token.type == TokenType::TOK_FUNCTION) hasFunction = true;
        if (token.type == TokenType::TOK_RETURN) hasReturn = true;
        if (token.type == TokenType::TOK_FOR) hasFor = true;
        if (token.type == TokenType::TOK_WHILE) hasWhile = true;
    }
    
    assert(hasFunction && "يجب أن يجد دالة");
    assert(hasReturn && "يجب أن يجد إرجاع");
    assert(hasFor && "يجب أن يجد لـ");
    assert(hasWhile && "يجب أن يجد بينما");
    
    printTokens(tokens);
    std::cout << "✅ اجتياز\n";
}

// ─────────────────────────────────────────────────────────
// اختبار 5: الأرقام والعوامل
// ─────────────────────────────────────────────────────────

void test5_NumbersAndOperators() {
    std::cout << "\n✅ Test 5: الأرقام والعوامل\n";
    std::string code = "x = 123 + 45.67; y = 3.14 * 2; z = 10 / 2.5;";
    
    ArabicLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    int numberCount = 0;
    for (const auto& token : tokens) {
        if (token.type == TokenType::TOK_NUMBER) numberCount++;
    }
    
    assert(numberCount >= 4 && "يجب أن يجد أرقام");
    
    printTokens(tokens);
    std::cout << "✅ اجتياز\n";
}

// ─────────────────────────────────────────────────────────
// اختبار 6: التعليقات
// ─────────────────────────────────────────────────────────

void test6_Comments() {
    std::cout << "\n✅ Test 6: التعليقات\n";
    std::string code = R"(
        // هذا تعليق بسطر واحد
        متغير x = 5;
        /* هذا تعليق
           على عدة أسطر */
        x = x + 1;
    )";
    
    ArabicLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    // التحقق من عدم وجود تعليقات في الرموز
    for (const auto& token : tokens) {
        assert(token.type != TokenType::TOK_COMMENT);
        assert(token.type != TokenType::TOK_BLOCK_COMMENT);
    }
    
    std::cout << "عدد الرموز بدون تعليقات: " << tokens.size() << "\n";
    std::cout << "✅ اجتياز\n";
}

// ─────────────────────────────────────────────────────────
// اختبار 7: السلاسل النصية
// ─────────────────────────────────────────────────────────

void test7_Strings() {
    std::cout << "\n✅ Test 7: السلاسل النصية\n";
    std::string code = R"(
        x = "نص عربي";
        y = 'حرف واحد';
        z = "نص مع علامات \"داخلية\"";
    )";
    
    ArabicLexer lexer(code);
    auto tokens = lexer.tokenize();
    
    int stringCount = 0;
    for (const auto& token : tokens) {
        if (token.type == TokenType::TOK_STRING) stringCount++;
    }
    
    assert(stringCount >= 3 && "يجب أن يجد 3 نصوص");
    
    printTokens(tokens);
    std::cout << "✅ اجتياز\n";
}

// ═══════════════════════════════════════════════════════════
// البرنامج الرئيسي
// ═══════════════════════════════════════════════════════════

int main() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║ اختبارات المحلل اللغوي العربي (Lexer)║\n";
    std::cout << "╚════════════════════════════════════════╝\n";
    
    try {
        test1_SimpleTokens();
        test2_ArrayOperations();
        test3_ControlStructures();
        test4_LoopsAndFunctions();
        test5_NumbersAndOperators();
        test6_Comments();
        test7_Strings();
        
        std::cout << "\n╔════════════════════════════════════════╗\n";
        std::cout << "║ ✅ جميع الاختبارات نجحت!             ║\n";
        std::cout << "║ اجتياز: 7/7 اختبارات                 ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ خطأ: " << e.what() << std::endl;
        return 1;
    }
}
