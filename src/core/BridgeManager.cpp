// BridgeManager.cpp - تنفيذ مدير الجسور الموحد
#include "BridgeManager.h"
#include "ArabicCompiler.h"
#include "ArabicRuntime.h"
#include "ArabicExecutor.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace ArabicLanguage {

BridgeManager::BridgeManager() : isInitialized(false) {}

BridgeManager::~BridgeManager() = default;

BridgeManager& BridgeManager::getInstance() {
    static BridgeManager instance;
    return instance;
}

bool BridgeManager::initialize() {
    if (isInitialized) return true;

    // ✅ تعيين العلم مبكراً لمنع التكرار اللانهائي (Infinite Recursion)
    // حيث أن ArabicCompiler يستدعي initialize() في مشيده
    isInitialized = true;

    try {
        selfCompiler = std::make_unique<ArabicCompiler>();
        selfCompiler->setUseBridges(false); // ❌ تعطيل الجسور للمترجم الذاتي لتجنب الدوران اللانهائي
        
        selfRuntime = std::make_unique<ArabicRuntime>();
        selfRuntime->initialize();

        // ✅ محاولة تحميل كود المترجم الذاتي من مسارات متعددة
        std::vector<std::string> paths = {
            "src/core/SelfHostingCompiler.arabic",
            "../src/core/SelfHostingCompiler.arabic",
            "../../src/core/SelfHostingCompiler.arabic",
            "d:/Arabic_Programming_Language_v1.0.0/src/core/SelfHostingCompiler.arabic"
        };
        
        bool loaded = false;
        for (const auto& path : paths) {
            std::ifstream file(path);
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string arabicCode = buffer.str();
                
                // ✅ إزالة علامة BOM إذا وجدت
                if (arabicCode.length() >= 3 && 
                    (unsigned char)arabicCode[0] == 0xEF && 
                    (unsigned char)arabicCode[1] == 0xBB && 
                    (unsigned char)arabicCode[2] == 0xBF) {
                    arabicCode = arabicCode.substr(3);
                }
                
                // ✅ ربط الـ Runtime بالمترجم لضمان بقاء تعريفات الدوال
                selfCompiler->setPersistentRuntime(selfRuntime);
                
                // تنفيذ الكود لتعريف الدوال في البيئة
                selfCompiler->executeIntermediate(arabicCode);
                std::cout << "[INFO] Self-Hosting Arabic Compiler loaded and functions registered from: " << path << std::endl;
                loaded = true;
                break;
            }
        }
        
        if (!loaded) {
            std::cerr << "[WARNING] Could not find SelfHostingCompiler.arabic" << std::endl;
        }
        
        std::cout << "[INFO] BridgeManager initialized successfully." << std::endl;
        return true;
    } catch (const std::exception& e) {
        isInitialized = false; // إعادة التعيين في حالة الفشل
        std::cerr << "[ERROR] Failed to initialize BridgeManager: " << e.what() << std::endl;
        return false;
    }
}

std::vector<ArabicParser::Token> BridgeManager::tokenize(const std::string& code) {
    if (!initialize()) return {};
    std::cout << "[BRIDGE] Tokenizing using Arabic Lexer..." << std::endl;
    
    try {
        ArabicExecutor executor(selfRuntime);
        
        // تجهيز نداء الدالة "حلل_رموز"
        auto callCmd = std::make_shared<Command>(CommandType::FUNCTION_CALL);
        callCmd->function_name = "حلل_رموز";
        
        Value codeVal(ValueType::STRING);
        codeVal.string_value = code;
        callCmd->arguments.push_back(std::make_shared<Value>(codeVal));
        
        // تنفيذ النداء
        std::cout << "[BRIDGE] Scope count before calling 'حلل_رموز': " << selfRuntime->scopeCount() << std::endl;
        Value result = executor.executeCommand(callCmd);
        std::cout << "[BRIDGE] Scope count after calling 'حلل_رموز': " << selfRuntime->scopeCount() << std::endl;
        
        if (result.type == ValueType::ARRAY) {
            std::cout << "[BRIDGE] Arabic Lexer returned: " << result.elements.size() << " tokens." << std::endl;
            
            std::vector<ArabicParser::Token> tokens;
            for (const auto& val : result.elements) {
                if (val.type == ValueType::OBJECT || val.type == ValueType::ARRAY) { // Arabic map is often handled as OBJECT/ARRAY with map_elements
                    std::string typeStr = "";
                    std::string valStr = "";
                    
                    if (val.map_elements.count("نوع")) typeStr = val.map_elements.at("نوع").string_value;
                    if (val.map_elements.count("قيمة")) valStr = val.map_elements.at("قيمة").string_value;
                    
                    // تحويل النوع النصي إلى enum
                    ArabicParser::Token::Type type = ArabicParser::Token::IDENTIFIER;
                    if (typeStr == "عدد") type = ArabicParser::Token::NUMBER;
                    else if (typeStr == "نص") type = ArabicParser::Token::STRING;
                    else if (typeStr == "مؤثر") type = ArabicParser::Token::OPERATOR;
                    else if (typeStr == "كلمة_محجوزة") type = ArabicParser::Token::KEYWORD;
                    else if (typeStr == "فاصل") {
                        if (valStr == "(") type = ArabicParser::Token::LPAREN;
                        else if (valStr == ")") type = ArabicParser::Token::RPAREN;
                        else if (valStr == "[") type = ArabicParser::Token::LBRACKET;
                        else if (valStr == "]") type = ArabicParser::Token::RBRACKET;
                        else if (valStr == ":") type = ArabicParser::Token::COLON;
                        else if (valStr == "،") type = ArabicParser::Token::COMMA;
                    }
                    
                    tokens.emplace_back(type, valStr);
                }
            }
            return tokens;
        }
    } catch (const std::exception& e) {
        std::cerr << "[BRIDGE ERROR] Failed to call Arabic Lexer: " << e.what() << std::endl;
    }
    
    return {}; 
}

std::vector<std::shared_ptr<Command>> BridgeManager::callParser(const std::string& code, SymbolTable& symbols) {
    if (!initialize()) return {};
    std::cout << "[BRIDGE] Parsing using Arabic Parser..." << std::endl;
    
    try {
        ArabicExecutor executor(selfRuntime);
        
        // 1. أولاً نحصل على الرموز
        std::vector<ArabicParser::Token> tokens = tokenize(code);
        
        // تحويل الرموز إلى Value ARRAY لتمريرها للدالة العربية
        Value tokensArray(ValueType::ARRAY);
        for (const auto& token : tokens) {
            Value t(ValueType::OBJECT);
            std::string typeStr = "معرف";
            if (token.type == ArabicParser::Token::NUMBER) typeStr = "عدد";
            else if (token.type == ArabicParser::Token::STRING) typeStr = "نص";
            else if (token.type == ArabicParser::Token::OPERATOR) typeStr = "مؤثر";
            
            t.map_elements["نوع"] = Value(typeStr);
            t.map_elements["قيمة"] = Value(token.value);
            tokensArray.elements.push_back(t);
        }
        
        // 2. ثم نحلل نحوياً
        auto callParserCmd = std::make_shared<Command>(CommandType::FUNCTION_CALL);
        callParserCmd->function_name = "حلل_نحوياً";
        callParserCmd->arguments.push_back(std::make_shared<Value>(tokensArray));
        
        Value nodes = executor.executeCommand(callParserCmd);
        
        if (nodes.type == ValueType::ARRAY) {
            std::cout << "[BRIDGE] Arabic Parser returned: " << nodes.elements.size() << " nodes." << std::endl;
            
            std::vector<std::shared_ptr<Command>> commands;
            for (const auto& node : nodes.elements) {
                if (node.type == ValueType::OBJECT || node.type == ValueType::ARRAY) {
                    std::string typeStr = "";
                    if (node.map_elements.count("نوع")) typeStr = node.map_elements.at("نوع").string_value;
                    
                    if (typeStr == "إسناد") {
                        auto cmd = std::make_shared<Command>(CommandType::ASSIGNMENT);
                        if (node.map_elements.count("متغير")) cmd->variable = node.map_elements.at("متغير").string_value;
                        if (node.map_elements.count("قيمة")) {
                            cmd->value = node.map_elements.at("قيمة");
                        }
                        commands.push_back(cmd);
                    } else if (typeStr == "طباعة") {
                        auto cmd = std::make_shared<Command>(CommandType::PRINT);
                        if (node.map_elements.count("قيمة")) {
                            cmd->value = node.map_elements.at("قيمة");
                        }
                        commands.push_back(cmd);
                    } else if (typeStr == "دالة") {
                        auto cmd = std::make_shared<Command>(CommandType::FUNCTION_DEF);
                        if (node.map_elements.count("اسم")) cmd->function_name = node.map_elements.at("اسم").string_value;
                        if (node.map_elements.count("بارامترات")) {
                            Value params = node.map_elements.at("بارامترات");
                            for (const auto& p : params.elements) cmd->parameters.push_back(p.string_value);
                        }
                        // ملاحظة: معالجة الجسم تتطلب تحويلاً معقداً للرموز إلى أوامر
                        // سنكتفي حالياً بتسجيل وجود الدالة
                        commands.push_back(cmd);
                    } else if (typeStr == "شرط") {
                        auto cmd = std::make_shared<Command>(CommandType::CONDITION);
                        // يمكن توسيع هذا لاحقاً لتحليل الشرط والجسم
                        commands.push_back(cmd);
                    } else if (typeStr == "إرجاع") {
                        auto cmd = std::make_shared<Command>(CommandType::RETURN);
                        if (node.map_elements.count("قيمة")) cmd->value = node.map_elements.at("قيمة");
                        commands.push_back(cmd);
                    } else if (typeStr == "نداء_دالة") {
                        auto cmd = std::make_shared<Command>(CommandType::FUNCTION_CALL);
                        if (node.map_elements.count("اسم")) cmd->function_name = node.map_elements.at("اسم").string_value;
                        if (node.map_elements.count("بارامترات")) {
                            Value params = node.map_elements.at("بارامترات");
                            for (const auto& p : params.elements) cmd->arguments.push_back(std::make_shared<Value>(p));
                        }
                        commands.push_back(cmd);
                    } else if (typeStr == "حلقة_طالما") {
                        auto cmd = std::make_shared<Command>(CommandType::LOOP_WHILE);
                        // تحليل الشرط والجسم لاحقاً
                        commands.push_back(cmd);
                    } else if (typeStr == "حلقة_كرر") {
                        auto cmd = std::make_shared<Command>(CommandType::LOOP_FOR);
                        if (node.map_elements.count("متغير")) cmd->variable = node.map_elements.at("متغير").string_value;
                        commands.push_back(cmd);
                    } else if (typeStr == "تحكم_تدفق") {
                        std::string op = "";
                        if (node.map_elements.count("أمر")) op = node.map_elements.at("أمر").string_value;
                        
                        if (op == "توقف") commands.push_back(std::make_shared<Command>(CommandType::BREAK));
                        else if (op == "استمر") commands.push_back(std::make_shared<Command>(CommandType::CONTINUE));
                    }
                }
            }
            return commands;
        }
    } catch (const std::exception& e) {
        std::cerr << "[BRIDGE ERROR] Failed to call Arabic Parser: " << e.what() << std::endl;
        std::cerr << "[BRIDGE ERROR] Code that caused fallback: " << code.substr(0, 200) << "..." << std::endl;
    }

    return {}; // نعود بـ {} لتفعيل الـ Fallback في المترجم إذا فشل التحويل
}

Value BridgeManager::parseToValue(const std::vector<ArabicParser::Token>& tokens) {
    if (!initialize()) return Value(ValueType::NONE);
    std::cout << "[BRIDGE] Parsing tokens to Value AST..." << std::endl;
    return Value(ValueType::NONE);
}

std::string BridgeManager::callCodeGen(const std::vector<std::shared_ptr<Command>>& ast) {
    if (!initialize()) return "";
    std::cout << "[BRIDGE] Generating code using Arabic CodeGen..." << std::endl;
    
    try {
        ArabicExecutor executor(selfRuntime);
        
        // تجهيز نداء الدالة "ترجم"
        auto callCmd = std::make_shared<Command>(CommandType::FUNCTION_CALL);
        callCmd->function_name = "ترجم";
        
        // تحويل حجم الـ AST كقيمة تجريبية (لأن المترجم الحالي PoC)
        Value astSize(ValueType::NUMBER);
        astSize.number_value = static_cast<double>(ast.size());
        callCmd->arguments.push_back(std::make_shared<Value>(astSize));
        
        // تنفيذ النداء
        Value result = executor.executeCommand(callCmd);
        
        if (result.type != ValueType::NONE) {
            std::cout << "[BRIDGE] Arabic CodeGen returned: " << result.number_value << " instructions." << std::endl;
            return "/* Generated by Arabic CodeGen: " + std::to_string(static_cast<long long>(result.number_value)) + " instructions */";
        }
    } catch (const std::exception& e) {
        std::cerr << "[BRIDGE ERROR] Failed to call Arabic CodeGen: " << e.what() << std::endl;
    }
    
    return "";
}

std::string BridgeManager::generateFromValue(const Value& astNodes) {
    if (!initialize()) return "";
    std::cout << "[BRIDGE] Generating C++ from Arabic AST Value..." << std::endl;
    return "";
}

void BridgeManager::reportError(const std::string& message, int line, int column) {
    if (!initialize()) return;

    // منطق جسر معالج الأخطاء
    std::cout << "[BRIDGE] Reporting error: " << message << " @ " << line << ":" << column << std::endl;
}

void BridgeManager::optimizeAST(std::vector<std::shared_ptr<Command>>& ast) {
    if (!initialize()) return;

    // منطق جسر المحسن
    std::cout << "[BRIDGE] Optimizing AST..." << std::endl;
}

void BridgeManager::reset() {
    selfCompiler.reset();
    selfRuntime.reset();
    isInitialized = false;
}

} // namespace ArabicLanguage
