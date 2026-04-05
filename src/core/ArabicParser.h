// ArabicParser.h - النسخة المحسنة مع دمج التحسينات
#ifndef ARABIC_PARSER_H
#define ARABIC_PARSER_H

#include "../runtime/ArabicTypes.h"
#include "ArabicTypeInference.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <regex>
#include <set>  // ✅ For import tracking
#include <sstream>
#include "SymbolTable.h"

namespace ArabicLanguage {

    class ArabicExecutor;  // Forward declaration

    class ArabicParser {
    public:
        ArabicParser(SymbolTable* symTable = nullptr, ArabicExecutor* exec = nullptr);
        virtual ~ArabicParser() = default;

        // الدوال العامة
        virtual std::vector<std::shared_ptr<Command>> parse(const std::string& sourceCode, SymbolTable& symbols);
        virtual std::map<std::string, int> getStatistics() const;

        // ✅ دوال نظام الاستيراد
        void setBasePath(const std::string& path);
        void setCurrentFile(const std::string& path);

        // ✅ دوال التحليل العامة
        Value parseExpression(const std::string& expr);
        
        // دوال التحليل الهيكلية
        std::shared_ptr<Command> parseLineWithIndex(const std::vector<std::string>& lines, size_t& index);
        std::shared_ptr<Command> parseWhileBlock(const std::vector<std::string>& lines, size_t& index);
        std::shared_ptr<Command> parseForBlock(const std::vector<std::string>& lines, size_t& index);
        std::shared_ptr<Command> parseIfBlock(const std::vector<std::string>& lines, size_t& index);
        std::shared_ptr<Command> parseFunctionBlock(const std::vector<std::string>& lines, size_t& index);
        std::shared_ptr<Command> parsePrint(const std::string& line, int lineNum, int col);
        std::shared_ptr<Command> parseAssignment(const std::string& line, int lineNum, int col);
        std::shared_ptr<Command> parseReturn(const std::string& line, int lineNum, int col);
        std::shared_ptr<Command> parseImport(const std::string& line, int lineNum, int col);
        std::shared_ptr<Command> parseBreak(const std::string& line, int lineNum, int col);
        std::shared_ptr<Command> parseContinue(const std::string& line, int lineNum, int col);
        std::shared_ptr<Command> parseForEach(const std::vector<std::string>& lines, size_t& index);

        // Stage 1.1: Parser المحسّن (Tokenizer + Pratt Parser)
        // ========================================
        
        // Token struct for improved expression parsing
        struct Token {
            enum Type {
                NUMBER,      // 123
                STRING,      // "text"
                IDENTIFIER,  // variable_name
                KEYWORD,     // متغير، دالة
                OPERATOR,    // +, -, *, /
                LPAREN,      // (
                RPAREN,      // )
                LBRACKET,    // [
                RBRACKET,    // ]
                LBRACE,      // {
                RBRACE,      // }
                COLON,       // :
                DOT,         // .
                COMMA        // ،
            };
            
            Type type;
            std::string value;
            
            Token(Type t, const std::string& v) : type(t), value(v) {}
        };
        
        // Tokenizer - converts expression string to tokens
        std::vector<Token> tokenize(const std::string& expr);
        
        // Get operator precedence
        int getPrecedence(const std::string& op);
        
        // Parse atomic expression (number, variable, array access, etc.)
        Value parseAtom(const std::vector<Token>& tokens, size_t& pos);
        
        // Parse expression with operator precedence climbing
        Value parseExpressionWithPrecedence(const std::vector<Token>& tokens, size_t& pos, int minPrec);
        
        // مساعد لتقسيم النصوص بناءً على الفواصل مع احترام السلاسل النصية
        std::vector<std::string> splitByComma(const std::string& input);
        
        // مساعد لإزالة التعليقات من السطر
        std::string stripComments(const std::string& line);
        
        // ========================================
        
        // Tokenizer helpers
        Value parseExpressionTokens(const std::vector<Token>& tokens, size_t& pos);
        Value parsePrimary(const std::vector<Token>& tokens, size_t& pos);

    // ────────────────────────────────────────────────────────
    // إعداد المنفذ
    // ────────────────────────────────────────────────────────

    void setExecutor(ArabicExecutor* exec) { executor = exec; }

    // ────────────────────────────────────────────────────────
    // نظام استنتاج الأنواع
    // ────────────────────────────────────────────────────────

        // الحصول على نظام استنتاج الأنواع
        ArabicTypeInference* getTypeInferencer() const { return typeInferencer.get(); }

        // استنتاج نوع تعبير
        ArabicTypeInference::InferenceResult inferExpressionType(const std::string& expression);

        // تحديث سياق المتغيرات للاستنتاج
        void updateVariableContext(const std::string& varName, ValueType type);

    private:
        SymbolTable* symbols;  // Pointer to symbol table for pre-registration
        ArabicExecutor* executor;  // Pointer to executor for immediate execution of definitions
        std::unique_ptr<ArabicTypeInference> typeInferencer;  // نظام استنتاج الأنواع
        
        void printError(int lineNumber, const std::string& message, const std::string& line, int column = 1);
        void preRegisterFunctions(const std::vector<std::string>& lines);
        // دوال التحليل الأساسية
        std::shared_ptr<Command> parseLine(const std::string& line, int level, int lineNumber);
        std::shared_ptr<Command> parseIf(const std::string& line, int level, int lineNumber);
        std::shared_ptr<Command> parseElseIf(const std::string& line, int level, int lineNumber); // ✅ Added
        std::shared_ptr<Command> parseTryBlock(const std::vector<std::string>& lines, size_t& index);
        std::shared_ptr<Command> parseThrow(const std::string& line, int lineNum, int col);
        std::shared_ptr<Command> parseWait(const std::string& line, int lineNum, int col);
        std::shared_ptr<Command> parseLambda(const std::string& line, int lineNum, int col);
        std::shared_ptr<Command> parseThis(const std::string& line, int level, int lineNumber);

        // دوال File I/O
        std::shared_ptr<Command> parseFileOpen(const std::string& line, int level, int lineNumber);
        std::shared_ptr<Command> parseFileRead(const std::string& line, int level, int lineNumber);
        std::shared_ptr<Command> parseFileWrite(const std::string& line, int level, int lineNumber);
        std::shared_ptr<Command> parseFileWriteFile(const std::string& line, int level, int lineNumber);
        std::shared_ptr<Command> parseFileReadFile(const std::string& line, int level, int lineNumber);
        std::shared_ptr<Command> parseFileClose(const std::string& line, int level, int lineNumber);

        // دوال البرمجة الكائنية
        std::shared_ptr<Command> parseClassBlock(const std::vector<std::string>& lines, size_t& index);
        std::shared_ptr<Command> parseInterfaceBlock(const std::vector<std::string>& lines, size_t& index);
        std::shared_ptr<Command> parseAbstractClass(const std::string& line, int level, int lineNumber);
        std::vector<std::string> splitInterfaces(const std::string& interfacesStr);
        std::shared_ptr<Command> parseObjectCreation(const std::vector<std::string>& lines, size_t& index);
        std::shared_ptr<Command> parsePropertyAssignment(const std::string& line, int level, int lineNumber);
        std::shared_ptr<Command> parseMethodCall(const std::string& line, int level, int lineNumber);
        std::shared_ptr<Command> parseArrayAssignment(const std::string& line, int level, int lineNumber); // ✅ arr[i] = value

        // ✅ دوال مساعدة للاستيراد المتقدمة
        std::shared_ptr<Command> importLibrary(const std::string& libraryName, 
                                               const std::vector<std::string>& specificFunctions);
        std::string resolveLibraryPath(const std::string& libraryName);
        bool detectCycles(const std::string& libraryName);
        std::vector<std::string> extractFunctionNames(const std::string& statement);
        bool isValidImportStatement(const std::string& statement);
        std::string readFile(const std::string& filename); 

        // دوال مساعدة عامة
        bool isNewObjectCreation(const std::string& line);

        // دوال مساعدة للخصائص
        bool isPropertyAccess(const std::string& expr);
        Value parsePropertyAccess(const std::string& expr);

        // دوال تحليل القيم والشروط
        Value parseValue(const std::string& valueStr);
        Value parseCondition(const std::string& conditionStr);

        // دوال التحويل والمعالجة
        std::string convertArabicNumbers(const std::string& text);
        int arabicToDecimal(const std::string& arabicNumber);
        bool startsWith(const std::string& str, const std::string& prefix);

        // دوال مساعدة عامة
        bool isArabicKeyword(const std::string& word);
        bool isValidVariableName(const std::string& name);
        bool isArabicChar(unsigned char c);
        std::vector<std::shared_ptr<Value>> parseArgumentList(const std::vector<Token>& tokens, size_t& pos);
        std::vector<std::string> splitArguments(const std::string& args);

        // دعم المصفوفات المتقدم
        bool isArrayAccess(const std::string& expr);
        Value parseArrayAccess(const std::string& arrayExpr);

        Value parseArrayLiteral(const std::string& arrayStr);
        Value parseDictionaryLiteral(const std::string& dictStr); // ✅ Parse {k:v, ...}

        // ✅ دوال جديدة للتحسينات
        void updateSymbolTable(SymbolTable& newSymbols);
        void defineVariable(const std::string& varName);
        bool isKnownVariable(const std::string& varName);

        // المتغيرات الخاصة
        int spaceUnit;
        std::map<std::string, std::regex> basicPatterns;
        std::map<std::string, int> statistics;

        // ✅ متغير جديد لتتبع المتغيرات المعرّفة
        std::map<std::string, bool> variables;
        
        // ✅ نظام الاستيراد المتقدم
        std::string basePath;                    // مسار المجلد الأساسي
        std::set<std::string> importedFiles;     // منع الاستيراد الدائري
        std::string currentFile;
        std::map<std::string, std::string> libraryCache;  // ✅ تخزين مؤقت للمكتبات
        std::string currentClassName;            // ✅ تتبع اسم الصنف الحالي أثناء المسح الأولي
        
        void suggestFix(const std::string& errorMessage, const std::string& line); // ✅ اقتراح إصلاح للأخطاء
    };

} // namespace ArabicLanguage

#endif // ARABIC_PARSER_H
