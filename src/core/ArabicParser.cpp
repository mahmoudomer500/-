#include "ArabicParser.h"
#include "ArabicCompiler.h"
#include <iostream>
#include <sstream>
#include <stack>
#include <algorithm>

namespace ArabicLanguage {

    ArabicParser::ArabicParser(SymbolTable* symTable, ArabicExecutor* exec) 
        : symbols(symTable), executor(exec) {
        // Initialize basic patterns and keywords if needed
    }

    std::vector<std::shared_ptr<Command>> ArabicParser::parse(const std::string& sourceCode, SymbolTable& symbols) {
        this->symbols = &symbols;
        std::vector<std::shared_ptr<Command>> commands;
        
        // ✅ إصلاح: معالجة النصوص متعددة الأسطر قبل التقسيم
        // نجمع الأسطر التي تكون داخل نص مفتوح (عدد علامات " غير زوجي)
        std::string code = sourceCode;
        // Skip UTF-8 BOM if present
        if (code.size() >= 3 && 
            static_cast<unsigned char>(code[0]) == 0xEF &&
            static_cast<unsigned char>(code[1]) == 0xBB &&
            static_cast<unsigned char>(code[2]) == 0xBF) {
            code = code.substr(3);
        }

        // ✅ فحص الأقواس المتوازنة أولاً مع رقم السطر
        int parenDepth = 0;
        int braceDepth = 0;
        int errorLineNum = 1;
        for (size_t i = 0; i < code.size(); i++) {
            char c = code[i];
            if (c == '\n') errorLineNum++;
            else if (c == '(') parenDepth++;
            else if (c == ')') parenDepth--;
            else if (c == '{') braceDepth++;
            else if (c == '}') braceDepth--;
            
            if (parenDepth < 0) {
                std::cerr << "❌ خطأ في السطر " << errorLineNum << ": قوس ')' زائد" << std::endl;
                return commands;
            }
            if (braceDepth < 0) {
                std::cerr << "❌ خطأ في السطر " << errorLineNum << ": قوس '}' زائد" << std::endl;
                return commands;
            }
        }
        if (parenDepth > 0) {
            std::cerr << "❌ خطأ: قوس '(' غير مغلق — يوجد " << parenDepth << " قوس/أقواس مفتوحة بدون إغلاق" << std::endl;
            return commands;
        }
        if (braceDepth > 0) {
            std::cerr << "❌ خطأ: قوس '{' غير مغلق — يوجد " << braceDepth << " قوس/أقواس مفتوحة بدون إغلاق" << std::endl;
            return commands;
        }

        // Split by newlines manually (avoid getline issues with UTF-8)
        std::vector<std::string> lines;
        std::string currentLine;
        bool inMultilineString = false;
        std::string pendingLine;

        for (size_t i = 0; i < code.size(); i++) {
            char c = code[i];
            if (c == '\r') continue; // Skip CR
            if (c == '\n') {
                // Count quotes in this line
                int quoteCount = 0;
                bool escaped = false;
                for (char lc : currentLine) {
                    if (escaped) { escaped = false; continue; }
                    if (lc == '\\') { escaped = true; continue; }
                    if (lc == '"') quoteCount++;
                }
                
                if (inMultilineString) {
                    pendingLine += currentLine + "\n";
                    if (quoteCount % 2 == 1) {
                        inMultilineString = false;
                        lines.push_back(pendingLine);
                        pendingLine = "";
                    }
                } else {
                    lines.push_back(currentLine);
                    if (quoteCount % 2 == 1) {
                        inMultilineString = true;
                        pendingLine = currentLine + "\n";
                    }
                }
                currentLine.clear();
            } else {
                currentLine += c;
            }
        }
        // Handle last line
        if (!currentLine.empty()) {
            if (inMultilineString) {
                pendingLine += currentLine;
                lines.push_back(pendingLine);
            } else {
                lines.push_back(currentLine);
            }
        }

        size_t index = 0;
        while (index < lines.size()) {
            size_t indexBefore = index;
            // Process lines
            try {
                auto cmd = parseLineWithIndex(lines, index);
                if (cmd) {
                    commands.push_back(cmd);
                }
                // If parseLineWithIndex did not advance index, force advance to avoid infinite loop
                if (index == indexBefore) {
                    index++;
                }
            } catch (const std::exception& e) {
                std::cerr << "❌ خطأ في السطر " << (indexBefore + 1) << ": " << e.what() << std::endl;
                if (index == indexBefore) index++;
            }
        }
        
        // ✅ فحص: هل هناك كتل مفتوحة لم تُغلق؟ مع رقم السطر
        int blockDepth = 0;
        int unclosedLine = 0;
        std::vector<std::pair<std::string, int>> blockNames; // name + line
        for (int lineIdx = 0; lineIdx < (int)lines.size(); lineIdx++) {
            std::string trimmed = lines[lineIdx];
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            if (trimmed.empty() || trimmed[0] == '#') continue;
            
            if (trimmed.find("دالة ") == 0 || trimmed.find("إذا ") == 0 || 
                trimmed.find("بينما ") == 0 || trimmed.find("لكل ") == 0 ||
                trimmed.find("صنف ") == 0 || trimmed.find("صف ") == 0 ||
                trimmed.find("حاول ") == 0) {
                blockDepth++;
                // استخراج اسم الكتلة
                size_t spacePos = trimmed.find(' ');
                if (spacePos != std::string::npos) {
                    size_t parenPos = trimmed.find('(', spacePos);
                    if (parenPos != std::string::npos) {
                        blockNames.push_back({trimmed.substr(spacePos + 1, parenPos - spacePos - 1), lineIdx + 1});
                    } else {
                        blockNames.push_back({trimmed.substr(spacePos + 1), lineIdx + 1});
                    }
                }
            } else if (trimmed == "نهاية") {
                blockDepth--;
                if (!blockNames.empty()) blockNames.pop_back();
            }
        }
        
        if (blockDepth > 0) {
            std::cerr << "⚠️ تحذير: يوجد " << blockDepth << " كتلة/كتل بدون 'نهاية':" << std::endl;
            for (const auto& pair : blockNames) {
                std::cerr << "   - " << pair.first << " (السطر " << pair.second << ")" << std::endl;
            }
            std::cerr << "💡 تأكد من إضافة 'نهاية' بعد كل دالة، شرط، حلقة، أو صنف" << std::endl;
        }
        
        std::cerr << "[DEBUG] Parser generated " << commands.size() << " commands" << std::endl;
        for (size_t i = 0; i < commands.size(); i++) {
            std::cerr << "[DEBUG]   cmd[" << i << "].type=" << (int)commands[i]->type 
                      << ", var='" << commands[i]->variable << "'"
                      << ", func='" << commands[i]->function_name << "'" << std::endl;
        }
        return commands;
    }

    std::shared_ptr<Command> ArabicParser::parseLineWithIndex(const std::vector<std::string>& lines, size_t& index) {
        if (index >= lines.size()) return nullptr;

        std::string line = lines[index];
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') { index++; return nullptr; }

        // Check for blocks
        // دعم: "صنف", "صف", "صنف مجرد", "مجرد صنف" كمُفتِّح لتعريف الصنف
        if (line.find("صنف ") == 0 || line.find("صف ") == 0 ||
            line.find("مجرد صنف ") == 0 || line.find("صنف مجرد ") == 0) return parseClassBlock(lines, index);
        // واجهة (interface)
        if (line.find("واجهة ") == 0) return parseClassBlock(lines, index);
        if (line.find("دالة ") == 0) return parseFunctionBlock(lines, index);
        if (line.find("إذا ") == 0 || line.find("إذا(") == 0) return parseIfBlock(lines, index);
        if (line.find("طالما ") == 0 || line.find("طالما(") == 0 || 
            line.find("بينما ") == 0 || line.find("بينما(") == 0 ||
            line.find("كرر_طالما ") == 0 || line.find("كرر_طالما(") == 0) return parseWhileBlock(lines, index);
        if (line.find("كرر ") == 0 || line.find("كرر(") == 0) return parseForBlock(lines, index);
        if (line.find("لكل ") == 0 || line.find("لكل(") == 0) return parseForEach(lines, index);
        
        // Simple commands — increment index BEFORE returning to avoid infinite loop
        if (line.find("اكتب_ملف") == 0 || line.find("اكتب_ملف(") == 0) {
            index++; return parseFileWrite(line, (int)index, 0);
        }
        if (line.find("اطبع") == 0 || line.find("اكتب") == 0) { index++; return parsePrint(line, (int)index, 0); }
        // إعلانات المتغيرات — بالكلمات المحجوزة وأسماء الأنواع
        if (line.find("متغير ") == 0 || line.find("مت ") == 0 ||
            line.find("نص ") == 0   || line.find("رقم ") == 0 ||
            line.find("منطق ") == 0 || line.find("مصفوفة ") == 0) {
            index++; return parseAssignment(line, (int)index, 0);
        }
        if (line.find("ارجع ") == 0 || line.find("أرجع ") == 0 || line == "ارجع" || line == "أرجع") { index++; return parseReturn(line, (int)index, 0); }
        if (line.find("استورد ") == 0 || line.find("تضمين ") == 0) { index++; return parseImport(line, (int)index, 0); }
        if (line == "توقف") { index++; return parseBreak(line, (int)index, 0); }
        if (line == "استمر") { index++; return parseContinue(line, (int)index, 0); }

        // Standalone expression (like a function call or member call)
        if (line.find('(') != std::string::npos && line.find('=') == std::string::npos) {
            index++;
            Value exprVal = parseExpression(line);
            if (exprVal.type == ValueType::FUNCTION_CALL) {
                if (exprVal.left) {
                    // Member Call: receiver.method(...)
                    auto cmd = std::make_shared<Command>(CommandType::METHOD_CALL);
                    cmd->condition = exprVal.left;
                    cmd->object_name = exprVal.object_name; // Set object_name from expression
                    cmd->method_name = exprVal.function_name;
                    cmd->arguments = exprVal.arguments;
                    return cmd;
                } else {
                    // Global Function Call: func(...)
                    auto cmd = std::make_shared<Command>(CommandType::FUNCTION_CALL);
                    cmd->function_name = exprVal.function_name;
                    cmd->arguments = exprVal.arguments;
                    return cmd;
                }
            }
            // If it's not a call but just an expression on a line, it's allowed but does nothing
            return nullptr;
        }
        
        // Assignment to existing variable
        if (line.find('=') != std::string::npos && line.find("==") == std::string::npos) {
            index++;
            return parseAssignment(line, (int)index, 0);
        }

        index++; // Always advance — prevents infinite loop on unrecognized lines
        return nullptr;
    }

    // ════════════════════════════════════════════════════════════
    // 🔍 المكون 1: المحلل اللغوي (Tokenizer)
    // ════════════════════════════════════════════════════════════

    std::vector<ArabicParser::Token> ArabicParser::tokenize(const std::string& expr) {
        std::vector<Token> tokens;
        size_t pos = 0;
        
        while (pos < expr.length()) {
            unsigned char c = static_cast<unsigned char>(expr[pos]);
            
            if (std::isspace(c)) {
                pos++;
                continue;
            }
            
            // Numbers (including hex)
            if (std::isdigit(c)) {
                std::string num;
                if (c == '0' && pos + 1 < expr.length() && (expr[pos+1] == 'x' || expr[pos+1] == 'X')) {
                    num = expr.substr(pos, 2);
                    pos += 2;
                    while (pos < expr.length() && (isxdigit(expr[pos]))) {
                        num += expr[pos++];
                    }
                } else {
                    while (pos < expr.length() && (std::isdigit(static_cast<unsigned char>(expr[pos])) || expr[pos] == '.')) {
                        num += expr[pos++];
                    }
                }
                tokens.emplace_back(Token::NUMBER, num);
                continue;
            }
            
            // Strings
            if (c == '"') {
                pos++;
                std::string str;
                while (pos < expr.length() && expr[pos] != '"') {
                    str += expr[pos++];
                }
                if (pos < expr.length()) pos++; // Skip closing quote
                tokens.emplace_back(Token::STRING, str);
                continue;
            }
            
            // Operators and Punctuation
            if (c == '(') { tokens.emplace_back(Token::LPAREN, "("); pos++; continue; }
            if (c == ')') { tokens.emplace_back(Token::RPAREN, ")"); pos++; continue; }
            if (c == '{') { tokens.emplace_back(Token::LBRACE, "{"); pos++; continue; }
            if (c == '}') { tokens.emplace_back(Token::RBRACE, "}"); pos++; continue; }
            if (c == '[') { tokens.emplace_back(Token::LBRACKET, "["); pos++; continue; }
            if (c == ']') { tokens.emplace_back(Token::RBRACKET, "]"); pos++; continue; }
            if (c == ':') { tokens.emplace_back(Token::COLON, ":"); pos++; continue; }
            if (c == ',' || ((unsigned char)c == 0xD8 && pos + 1 < expr.length() && (unsigned char)expr[pos+1] == 0x8C)) {
                tokens.emplace_back(Token::OPERATOR, ","); 
                pos += ((unsigned char)c == 0xD8 ? 2 : 1); 
                continue; 
            }
            if (c == '.') { tokens.emplace_back(Token::DOT, "."); pos++; continue; }
            // Actually, I don't have COMMA token type in my usage yet, I used COLON check in comments.
            // Let's add comma as a token, but my Token enum might not have it.
            // Token enum is in .h. I can't see it right now.
            // Safe bet: Use OPERATOR or punctuation that I can check.
            // For now, treat comma as ',' operator/punctuation.
            
            // Check for multi-char operators
            if (pos + 1 < expr.length()) {
                std::string twoChars = expr.substr(pos, 2);
                if (twoChars == "==" || twoChars == "!=" || twoChars == ">=" || twoChars == "<=") {
                    tokens.emplace_back(Token::OPERATOR, twoChars);
                    pos += 2;
                    continue;
                }
            }
            
            if (c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '>' || c == '<' || c == '%' || c == ':') { 
                tokens.emplace_back(Token::OPERATOR, std::string(1, c)); 
                pos++; 
                continue; 
            }
            
            // Arabic Identifiers (simplified)
            // Assumes UTF-8 bytes for Arabic chars
            if ((unsigned char)c >= 0x80 || std::isalpha(c) || c == '_') {
                std::string word;
                while (pos < expr.length()) {
                    unsigned char nextC = (unsigned char)expr[pos];
                    if (nextC < 0x80) {
                        if (!isalnum(nextC) && nextC != '_') break;
                    } else {
                        // Check for Arabic punctuation starting with 0xD8
                        if (nextC == 0xD8 && pos + 1 < expr.length()) {
                            unsigned char byte2 = (unsigned char)expr[pos+1];
                            if (byte2 == 0x8C || byte2 == 0x9B || byte2 == 0x9F) break; // Comma, Semicolon, Question
                        }
                    }
                    word += expr[pos++];
                }
                
                if (isArabicKeyword(word)) {
                    tokens.emplace_back(Token::KEYWORD, word);
                } else {
                    tokens.emplace_back(Token::IDENTIFIER, word);
                }
                continue;
            }
            
            pos++; // Skip unknown chars
        }
        return tokens;
    }

    bool ArabicParser::isArabicKeyword(const std::string& word) {
        // ملاحظة: أسماء الأنواع (رقم، نص، منطق، مصفوفة، كائن) مُزالة عمداً من هذه القائمة
        // لأنها تُستخدم بشكل شائع كأسماء معاملات ومتغيرات في الكود العربي.
        // مثال: دالة ضاعف(رقم) — هنا "رقم" هو اسم معامل وليس كلمة محجوزة.
        // إبقاؤها في القائمة يُسبب خطأ "Undefined variable: رقم" عند تنفيذ الدالة.
        static const std::set<std::string> keywords = {
            // كلمات التحكم الأساسية — لا يجوز استخدامها كمعرِّفات
            "متغير", "مت", "دالة", "إذا", "وإلا", "صنف", "صف", "عام", "خاص", "نهاية", "اطبع", "اكتب",
            "ارجع", "أرجع", "طالما", "بينما", "كرر_طالما", "كرر", "جديد", "هذا", "استورد", "تضمين", "توقف", "استمر",
            "لا_شيء", "لاشيء", "صحيح", "خطأ", "في", "من", "إلى", "يرث", "واجهة",
            // أسماء الأنواع مُزالة: "نص", "رقم", "منطق", "مصفوفة", "كائن"
            // سبب الإزالة: تُستخدم كأسماء معاملات ومتغيرات — مثال: دالة ف(رقم) أو مت رقم = 5
            "None", "True", "False", "null", "nullptr"
        };
        return keywords.find(word) != keywords.end();
    }

    // ════════════════════════════════════════════════════════════
    // 🏗️ تطبيق parseClassBlock (OOP Support)
    // ════════════════════════════════════════════════════════════

    std::shared_ptr<Command> ArabicParser::parseClassBlock(const std::vector<std::string>& lines, size_t& index) {
        std::string header = trim(lines[index]);
        auto tokens = tokenize(header);

        // ── تحديد الكلمة الأولى (تدعم: صنف، صف، واجهة، مجرد صنف، صنف مجرد) ──
        bool isAbstract  = false;
        bool isInterface = false;
        size_t nameIdx   = 1; // موضع اسم الصنف في tokens

        if (tokens.empty()) return nullptr;

        if (tokens[0].value == "واجهة") {
            isInterface = true;
        } else if (tokens[0].value == "مجرد" && tokens.size() > 1 &&
                   (tokens[1].value == "صنف" || tokens[1].value == "صف")) {
            isAbstract = true;
            nameIdx = 2;
        } else if ((tokens[0].value == "صنف" || tokens[0].value == "صف") &&
                   tokens.size() > 1 && tokens[1].value == "مجرد") {
            isAbstract = true;
            nameIdx = 2;
        } else if (tokens[0].value != "صنف" && tokens[0].value != "صف") {
            return nullptr;
        }

        if (nameIdx >= tokens.size()) return nullptr;
        std::string className = tokens[nameIdx].value;
        std::string baseClass = "";

        // ── كشف الوراثة: يدعم "يرث" / "يمتد" / ":" / "->" ──────────────────
        // أنماط: صنف ابن يرث أب   |   صنف ابن : أب   |   صنف ابن : أب، أب2
        size_t inhIdx = nameIdx + 1;
        if (inhIdx < tokens.size()) {
            bool isInheritanceOp = (tokens[inhIdx].value == "يرث"   ||
                                    tokens[inhIdx].value == "يمتد"  ||
                                    tokens[inhIdx].type  == Token::COLON);
            if (isInheritanceOp && inhIdx + 1 < tokens.size()) {
                baseClass = tokens[inhIdx + 1].value;
            }
        }

        auto classCmd = std::make_shared<Command>(CommandType::CLASS_DEF);
        classCmd->name       = className;
        classCmd->class_name = className;
        classCmd->class_def  = std::make_shared<ClassDefinition>(className);
        classCmd->class_def->parent_class = baseClass;
        classCmd->class_def->base_class   = baseClass;
        classCmd->class_def->is_abstract  = isAbstract;
        
        index++;
        bool isPublic = true; // Default access modifier for members
        
        while (index < lines.size()) {
            std::string line = trim(lines[index]);
            auto lineTokens = tokenize(line);
            
            if (lineTokens.empty()) { index++; continue; }
            
            std::string firstWord = lineTokens[0].value;
            
            if (firstWord == "نهاية") {
                index++;
                break;
            }

            // Do not prematurely end class on encountering next class header.
            // We rely on explicit 'نهاية' to close the class.
            
            if (firstWord == "عام") {
                isPublic = true;
                index++;
                continue;
            }
            
            if (firstWord == "خاص" || firstWord == "محمي") {
                isPublic = false;
                index++;
                continue;
            }
            
            if (firstWord == "دالة") {
                auto funcCmd = parseFunctionBlock(lines, index);
                if (funcCmd) {
                    ClassMethod method;
                    method.name = funcCmd->name;
                    method.parameters = funcCmd->parameters;
                    method.body = funcCmd->body;
                    method.is_static = false; // Default
                    // method.access = isPublic ? Access::PUBLIC : Access::PRIVATE; // Need to add access support to ClassMethod in Types if not present, otherwise handle logic here
                    
                    classCmd->class_def->methods[method.name] = method;
                }
                continue;
            }
            
            if (firstWord == "متغير" || firstWord == "خاصية" || firstWord == "نص" || firstWord == "رقم" || firstWord == "منطق" || firstWord == "مصفوفة") {
                // خاصية name [= value]
                if (lineTokens.size() >= 2) {
                    std::string propName = lineTokens[1].value;
                    ValueType propType = ValueType::NONE;
                    if (firstWord == "نص")       propType = ValueType::STRING;
                    else if (firstWord == "رقم") propType = ValueType::NUMBER;
                    else if (firstWord == "منطق") propType = ValueType::BOOLEAN;
                    else if (firstWord == "مصفوفة") propType = ValueType::ARRAY;
                    else propType = ValueType::NONE; // متغير / خاصية — نوع ديناميكي
                    PropertyInfo prop(propName, propType);
                    
                    // دعم القيم الابتدائية: خاصية الاسم = "قيمة"
                    size_t eqPos = line.find('=');
                    if (eqPos != std::string::npos) {
                        std::string valExpr = line.substr(eqPos + 1);
                        prop.default_value = std::make_shared<Value>(parseExpression(valExpr));
                    }
                     
                    classCmd->class_def->properties[propName] = prop;
                }
                index++;
                continue;
            }
            
            index++;
        }
        
        return classCmd;
    }

    std::shared_ptr<Command> ArabicParser::parseFunctionBlock(const std::vector<std::string>& lines, size_t& index) {
        std::string header = lines[index];
        auto tokens = tokenize(header);
        
        if (tokens.size() < 2 || tokens[0].value != "دالة") return nullptr;
        
        std::string funcName = tokens[1].value;
        
        
        auto funcCmd = std::make_shared<Command>(CommandType::FUNCTION_DEF);
        funcCmd->name = funcName;
        funcCmd->function_name = funcName; // Adding this to match ArabicExecutor's expectation
        
        // Parse parameters: دالة اسم(س، ص)
        // Find parens
        size_t lparen = 0; 
        for(size_t i=0; i<tokens.size(); ++i) if(tokens[i].type == Token::LPAREN) { lparen = i; break; }
        
        if (lparen > 0 && lparen + 1 < tokens.size()) {
            // Collect args until RPAREN
            // نقبل IDENTIFIER و KEYWORD معاً لأن أسماء الأنواع (رقم، نص...) قد تُستخدم كأسماء معاملات
            for(size_t i = lparen + 1; i < tokens.size(); ++i) {
                if (tokens[i].type == Token::RPAREN) break;
                if (tokens[i].type == Token::IDENTIFIER || tokens[i].type == Token::KEYWORD) {
                    // تأكد من أنها ليست كلمات تحكم حقيقية (نهاية، إذا، إلخ)
                    const std::string& pname = tokens[i].value;
                    if (pname != "نهاية" && pname != "إذا" && pname != "وإلا" &&
                        pname != "طالما" && pname != "كرر" && pname != "دالة" &&
                        pname != "ارجع" && pname != "أرجع" && pname != "صحيح" &&
                        pname != "خطأ" && pname != "لا_شيء" && pname != "لاشيء") {
                        funcCmd->parameters.push_back(pname);
                    }
                }
            }
        }
        
        index++;
        while (index < lines.size()) {
            std::string line = trim(lines[index]);
            auto lineTokens = tokenize(line);
            if (lineTokens.empty()) { index++; continue; }
            
            std::string firstWord = lineTokens[0].value;
            if (firstWord == "نهاية") {
                index++;
                break;
            }
            
            // NEW: Lenient termination — only break on actual KEYWORD/IDENTIFIER tokens, not STRING tokens
            if ((lineTokens[0].type == Token::KEYWORD || lineTokens[0].type == Token::IDENTIFIER) &&
                (firstWord == "دالة" || firstWord == "صنف" || firstWord == "صف" || firstWord == "عام" || firstWord == "خاص")) {
                break; 
            }
            
            size_t idxBefore = index;
            auto cmd = parseLineWithIndex(lines, index); // Recursive call for body
            if (cmd) {
                funcCmd->body.push_back(cmd);
            }
            if (index == idxBefore) {
                index++;
            }
        }
        
        return funcCmd;
    }

    // ════════════════════════════════════════════════════════════
    // 🧠 المكون 2: محلل التعبيرات (Expression Parser)
    // ════════════════════════════════════════════════════════════

    Value ArabicParser::parseExpression(const std::string& expr) {
        auto tokens = tokenize(expr);
        if (tokens.empty()) return Value(ValueType::NONE);
        
        size_t tokenPos = 0;
        Value result = parseExpressionTokens(tokens, tokenPos);
        
        return result;
    }
    
    int ArabicParser::getPrecedence(const std::string& op) {
        if (op == "*" || op == "/" || op == "%" || op == "٪") return 50;
        if (op == "+" || op == "-") return 40;
        if (op == "<" || op == "<=" || op == ">" || op == ">=") return 30;
        if (op == "==" || op == "!=") return 20;
        if (op == "و" || op == "&&") return 10;
        if (op == "أو" || op == "||") return 5;
        if (op == "=") return 1;
        return 0;
    }

    Value ArabicParser::parseExpressionWithPrecedence(const std::vector<Token>& tokens, size_t& pos, int minPrec) {
         if (pos >= tokens.size()) return Value(ValueType::NONE);
         
         // Parse Primary
         Value left = parsePrimary(tokens, pos);
         
         while (pos < tokens.size()) {
             Token op = tokens[pos];
             
if (op.type == Token::DOT) {
    pos++; // Skip dot
    if (pos >= tokens.size()) break; // Error trailing dot

    Token prop = tokens[pos];
    pos++;

    // Check for method call
    if (pos < tokens.size() && tokens[pos].type == Token::LPAREN) {
        // Method Call: left.method(...)
        Value methodCall(ValueType::FUNCTION_CALL);
        methodCall.left = std::make_shared<Value>(left);
        
        // Extract object name from left (handle VARIABLE, PROPERTY_ACCESS, FUNCTION_CALL chains)
        if (left.type == ValueType::VARIABLE) {
            methodCall.object_name = left.value;
        } else if (left.type == ValueType::PROPERTY_ACCESS) {
            methodCall.object_name = left.object_name;
        } else if (left.type == ValueType::FUNCTION_CALL) {
            // For chained calls like obj.method1().method2(), extract base object
            // The object_name of a FUNCTION_CALL should be set when it's created
            methodCall.object_name = left.object_name;
            // If still empty, try to traverse the chain
            if (methodCall.object_name.empty()) {
                std::shared_ptr<Value> current = left.left; // Start from inner expression
                while (current) {
                    if (current->type == ValueType::VARIABLE) {
                        methodCall.object_name = current->value;
                        break;
                    } else if (current->type == ValueType::PROPERTY_ACCESS) {
                        methodCall.object_name = current->object_name;
                        break;
                    } else if (current->type == ValueType::FUNCTION_CALL) {
                        if (!current->object_name.empty()) {
                            methodCall.object_name = current->object_name;
                            break;
                        }
                        current = current->left;
                    } else {
                        break;
                    }
                }
            }
        }
        
        methodCall.function_name = prop.value;

        pos++; // Skip LPAREN
        methodCall.arguments = parseArgumentList(tokens, pos);
        if (pos < tokens.size() && tokens[pos].type == Token::RPAREN) pos++; // Skip RPAREN

        // ✅ Handle array methods (طول, أضف)
        if ((left.type == ValueType::VARIABLE || left.type == ValueType::PROPERTY_ACCESS) && 
            (prop.value == "طول" || prop.value == "أضف")) {
            methodCall.type = ValueType::FUNCTION_CALL; // Use FUNCTION_CALL so parser recognizes it
            methodCall.function_name = prop.value;
            methodCall.value = prop.value; // Store method name for CodeGenerator
            // Set object_name for property access case
            if (left.type == ValueType::PROPERTY_ACCESS) {
                if (!left.object_name.empty()) {
                    methodCall.object_name = left.object_name;
                } else if (left.left) {
                    methodCall.object_name = left.left->value;
                }
                // Also set left to preserve the object reference
                methodCall.left = std::make_shared<Value>(left);
            } else {
                methodCall.object_name = left.value;
                methodCall.left = std::make_shared<Value>(left);
            }
            left = methodCall;
        } else {
            left = methodCall;
        }
    } else {
        // Property Access: left.prop
         Value propAccess(ValueType::PROPERTY_ACCESS);
         propAccess.left = std::make_shared<Value>(left);
         if (left.type == ValueType::VARIABLE) propAccess.object_name = left.value;
         else if (left.type == ValueType::PROPERTY_ACCESS) propAccess.object_name = left.object_name;
         propAccess.property_name = prop.value;
         // 
         left = propAccess;
     }
     continue;
 }

             // ✅ مضاف: دعم الوصول للمصفوفات arr[index]
             if (op.type == Token::LBRACKET) {
                 pos++; // Skip [
                 Value indexExpr = parseExpressionWithPrecedence(tokens, pos, 0);
                 if (pos < tokens.size() && tokens[pos].type == Token::RBRACKET) pos++; // Skip ]
                 
                 Value arrayAccess(ValueType::ARRAY_ACCESS);
                 arrayAccess.left = std::make_shared<Value>(left);
                 arrayAccess.right = std::make_shared<Value>(indexExpr);
                 left = arrayAccess;
                 continue;
             }
             
             if ((op.type == Token::OPERATOR && op.value != "," && op.value != "،") || (op.type == Token::KEYWORD && (op.value == "و" || op.value == "أو"))) {
                 int prec = getPrecedence(op.value);
                 if (prec < minPrec) break;
                 
                 pos++;
                 int nextPrec = (op.value == "=") ? prec : prec + 1; // Left-associative except for assignment
                 
                 Value right = parseExpressionWithPrecedence(tokens, pos, nextPrec);
                 Value binOp(ValueType::OPERATION);
                 binOp.left = std::make_shared<Value>(left);
                 binOp.right = std::make_shared<Value>(right);
                 binOp.operation = op.value;
                 left = binOp;
                 continue;
             }
             
             break;
         }
         
         return left;
    }

    std::vector<std::shared_ptr<Value>> ArabicParser::parseArgumentList(const std::vector<Token>& tokens, size_t& pos) {
        std::vector<std::shared_ptr<Value>> args;
        while (pos < tokens.size() && tokens[pos].type != Token::RPAREN) {
            // Skip commas (both English , and Arabic ،)
            if ((tokens[pos].type == Token::OPERATOR && (tokens[pos].value == "," || tokens[pos].value == "،")) ||
                (tokens[pos].type == Token::COMMA)) {
                pos++;
                continue;
            }
            // Parse one argument expression
            Value arg = parseExpressionWithPrecedence(tokens, pos, 0);
            args.push_back(std::make_shared<Value>(arg));
        }
        return args;
    }

    Value ArabicParser::parseExpressionTokens(const std::vector<Token>& tokens, size_t& pos) {
        return parseExpressionWithPrecedence(tokens, pos, 0);
    }

    Value ArabicParser::parsePrimary(const std::vector<Token>& tokens, size_t& pos) {
        if (pos >= tokens.size()) return Value(ValueType::NONE);
        Token t = tokens[pos];
        pos++;
        
        if (t.type == Token::LPAREN) {
            Value inner = parseExpressionWithPrecedence(tokens, pos, 0);
            if (pos < tokens.size() && tokens[pos].type == Token::RPAREN) {
                pos++;
            }
            return inner;
        }

        if (t.type == Token::NUMBER) {
            if (t.value.find("0x") == 0 || t.value.find("0X") == 0) {
                try { return Value((double)std::stoll(t.value, nullptr, 16)); } catch (...) { return Value(0.0); }
            }
            try { return Value(std::stod(t.value)); } catch (...) { return Value(0.0); }
        }

        // ✅ إصلاح: دعم النفي الأحادي (-) للأرقام السالبة
        if (t.type == Token::OPERATOR && t.value == "-") {
            if (pos < tokens.size() && tokens[pos].type == Token::NUMBER) {
                std::string negVal = "-" + tokens[pos].value;
                pos++;
                try { return Value(std::stod(negVal)); } catch (...) { return Value(0.0); }
            }
            if (pos < tokens.size() && tokens[pos].type == Token::LPAREN) {
                pos++;
                Value inner = parseExpressionWithPrecedence(tokens, pos, 0);
                if (pos < tokens.size() && tokens[pos].type == Token::RPAREN) pos++;
                Value unaryOp(ValueType::OPERATION);
                unaryOp.operation = "unary-";
                unaryOp.right = std::make_shared<Value>(inner);
                return unaryOp;
            }
            return Value(ValueType::NONE);
        }

        // ✅ إصلاح: دعم الموجب الأحادي (+)
        if (t.type == Token::OPERATOR && t.value == "+") {
            if (pos < tokens.size() && tokens[pos].type == Token::NUMBER) {
                pos++;
                try { return Value(std::stod(tokens[pos-1].value)); } catch (...) { return Value(0.0); }
            }
            if (pos < tokens.size() && tokens[pos].type == Token::LPAREN) {
                pos++;
                Value inner = parseExpressionWithPrecedence(tokens, pos, 0);
                if (pos < tokens.size() && tokens[pos].type == Token::RPAREN) pos++;
                return inner;
            }
            return Value(ValueType::NONE);
        }
        if (t.type == Token::STRING) {
            return Value(ValueType::STRING, t.value);
        }
        
        // ✅ مضاف: دعم مصفوفات القيم [1, 2, 3]
        if (t.type == Token::LBRACKET) {
            Value val(ValueType::ARRAY);
            while (pos < tokens.size() && tokens[pos].type != Token::RBRACKET) {
                // Skip commas (both English , and Arabic ，)
                if ((tokens[pos].type == Token::OPERATOR && (tokens[pos].value == "," || tokens[pos].value == "،")) ||
                    (tokens[pos].type == Token::COMMA)) {
                    pos++;
                    continue;
                }
                // Parse element expression
                Value elem = parseExpressionWithPrecedence(tokens, pos, 0);
                val.elements.push_back(elem);
            }
            if (pos < tokens.size() && tokens[pos].type == Token::RBRACKET) {
                pos++; // Skip ]
            }
            return val;
        }

        // ✅ مضاف: دعم قواميس/كائنات القيم { "س": 1, "ص": 2 }
        if (t.type == Token::LBRACE) {
            Value val(ValueType::OBJECT_LITERAL);
            while (pos < tokens.size() && tokens[pos].type != Token::RBRACE) {
                // Skip commas
                if (tokens[pos].type == Token::OPERATOR && (tokens[pos].value == "," || tokens[pos].value == "،")) {
                    pos++; continue;
                }
                
                // Key (must be string or identifier)
                Token keyTok = tokens[pos++];
                std::string key = keyTok.value;
                
                // Skip colon
                if (pos < tokens.size() && tokens[pos].type == Token::COLON) pos++;
                
                // Value
                Value elem = parseExpressionWithPrecedence(tokens, pos, 0);
                val.map_elements[key] = elem;
            }
            if (pos < tokens.size() && tokens[pos].type == Token::RBRACE) pos++;
            return val;
        }

        if (t.type == Token::IDENTIFIER || t.type == Token::KEYWORD) {
            // Boolean and Null constants
            if (t.value == "صحيح") return Value(ValueType::BOOLEAN, "1");
            if (t.value == "خطأ") return Value(ValueType::BOOLEAN, "0");
            if (t.value == "لا_شيء" || t.value == "لاشيء") return Value(ValueType::NONE);
            if (t.value == "ليس") {
                Value unaryOp(ValueType::OPERATION);
                unaryOp.operation = "ليس";
                unaryOp.right = std::make_shared<Value>(parseExpressionTokens(tokens, pos));
                return unaryOp;
            }

            // ✅ مضاف: دعم الكلمة المحجوزة "جديد" لإنشاء الكائنات
            if (t.value == "جديد" && pos < tokens.size() && 
                (tokens[pos].type == Token::IDENTIFIER || tokens[pos].type == Token::KEYWORD)) {
                Value newObj(ValueType::FUNCTION_CALL);
                newObj.is_new_object = true;
                newObj.function_name = tokens[pos].value;
                newObj.value = tokens[pos].value;
                pos++; 
                
                if (pos < tokens.size() && tokens[pos].type == Token::LPAREN) {
                    pos++; 
                    newObj.arguments = parseArgumentList(tokens, pos);
                    if (pos < tokens.size() && tokens[pos].type == Token::RPAREN) pos++;
                }
                return newObj;
            }

            // Check for function call
            if (pos < tokens.size() && tokens[pos].type == Token::LPAREN) {
                 pos++; // Skip (
                 Value funcCall(ValueType::FUNCTION_CALL);
                 funcCall.function_name = t.value;
                 funcCall.value = t.value; // ✅ Fix: Store function name in value for CodeGenerator
                 
                 funcCall.arguments = parseArgumentList(tokens, pos);
                 
                 if (pos < tokens.size() && tokens[pos].type == Token::RPAREN) {
                     pos++; // Skip )
                 }
                 return funcCall;
             }
             
             // Handles `هذا` as VARIABLE
             return Value(ValueType::VARIABLE, t.value);
        }
        
        return Value(ValueType::NONE);
    }

    std::shared_ptr<Command> ArabicParser::parsePrint(const std::string& line, int lineNum, int col) {
        auto cmd = std::make_shared<Command>(CommandType::PRINT);
        
        // Find first ( and last )
        size_t start = line.find('(');
        size_t end = line.find_last_of(')');
        
        if (start != std::string::npos && end != std::string::npos && end > start) {
            std::string interior = line.substr(start + 1, end - start - 1);
            
            // ✅ Use improved tokenizer and parseArgumentList for multi-argument support
            auto tokens = tokenize(interior);
            size_t pos = 0;
            cmd->arguments = parseArgumentList(tokens, pos);
            
            // For backwards compatibility with older executor versions that might look at cmd->expression
            if (!cmd->arguments.empty()) {
                cmd->expression = cmd->arguments[0];
            }
        }
        return cmd;
    }

    std::shared_ptr<Command> ArabicParser::parseFileWrite(const std::string& line, int lineNum, int col) {
        auto cmd = std::make_shared<Command>(CommandType::FILE_WRITE);
        size_t start = line.find('(');
        size_t end = line.find_last_of(')');
        if (start != std::string::npos && end != std::string::npos && end > start) {
            std::string interior = line.substr(start + 1, end - start - 1);
            auto tokens = tokenize(interior);
            size_t pos = 0;
            cmd->arguments = parseArgumentList(tokens, pos);
        }
        return cmd;
    }

    std::shared_ptr<Command> ArabicParser::parseAssignment(const std::string& line, int lineNum, int col) {
        auto tokens = tokenize(line);
        if (tokens.empty()) return nullptr;
        
        std::string varName;
        size_t eqPos = 0;
        
        // متغير varName = RHS (includes مصفوفة for array declarations)
        if (tokens[0].value == "متغير" || tokens[0].value == "مت" || tokens[0].value == "نص" || tokens[0].value == "رقم" || tokens[0].value == "منطق" || tokens[0].value == "مصفوفة") {
            if (tokens.size() < 4) return nullptr;
            varName = tokens[1].value;
            bool isArrayDecl = (tokens[0].value == "مصفوفة");
            // Find '=' token
            for (size_t i = 2; i < tokens.size(); i++) {
                if (tokens[i].type == Token::OPERATOR && tokens[i].value == "=") {
                    eqPos = i;
                    break;
                }
            }
            if (eqPos == 0) {
                // Fallback: use string search
                size_t p = line.find('=');
                if (p != std::string::npos) {
                    std::string rhs = line.substr(p + 1);
            auto cmd = std::make_shared<Command>(CommandType::ASSIGN);
            cmd->variable = varName;
            cmd->value = parseExpression(rhs);
            
            // Debug
            std::cerr << "[DEBUG] parseAssignment: var='" << varName << "', rhs='" << rhs << "', value.type=" << (int)cmd->value.type << std::endl;
                    
                    // ✅ تسجيل المتغير الجديد
                    defineVariable(varName);
                    
                    return cmd;
                }
                return nullptr;
            }
            // Build RHS from tokens after eqPos
            // ⚠️ النصوص (STRING) يجب إعادة تغليفها بعلامات تنصيص لأن القيمة تجرَّدت منها
            std::string rhs;
            for (size_t i = eqPos + 1; i < tokens.size(); i++) {
                if (tokens[i].type == Token::STRING) {
                    rhs += "\"" + tokens[i].value + "\"";
                } else {
                    rhs += tokens[i].value;
                }
                rhs += " ";
            }
            auto cmd = std::make_shared<Command>(CommandType::ASSIGN);
            cmd->variable = varName;
            cmd->value = parseExpression(rhs);
            
            // ✅ تسجيل المتغير الجديد
            defineVariable(varName);

            return cmd;
        } else {
            // Build full LHS name (including dots for property access: هذا.النوع)
            varName = "";
            for (size_t i = 0; i < tokens.size(); i++) {
                if (tokens[i].type == Token::OPERATOR && tokens[i].value == "=") {
                    eqPos = i;
                    break;
                }
                varName += tokens[i].value;
            }
        }
        
        // Build command when eqPos > 0
        if (eqPos > 0) {
            // Build RHS string from tokens after '='
            std::string rhs;
            for (size_t i = eqPos + 1; i < tokens.size(); i++) {
                if (i > eqPos + 1) rhs += " ";
                if (tokens[i].type == Token::STRING) {
                    rhs += "\"" + tokens[i].value + "\"";
                } else {
                    rhs += tokens[i].value;
                }
            }
            
            // Check for array assignment: var[index] = ...
            // varName currently contains "var[index]"
            size_t openBracket = varName.find('[');
            size_t closeBracket = varName.find_last_of(']');
            
            if (openBracket != std::string::npos && closeBracket != std::string::npos && closeBracket > openBracket) {
                std::string arrayName = varName.substr(0, openBracket);
                std::string indexStr = varName.substr(openBracket + 1, closeBracket - openBracket - 1);
                
                auto cmd = std::make_shared<Command>(CommandType::ARRAY_ASSIGNMENT);
                cmd->variable = arrayName;
                cmd->condition = std::make_shared<Value>(parseExpression(indexStr));
                cmd->value = parseExpression(rhs);
                return cmd;
            }

            auto cmd = std::make_shared<Command>(CommandType::ASSIGN);
            cmd->variable = varName;
            cmd->value = parseExpression(rhs);
            
            // ✅ تسجيل المتغير الجديد (إذا لم يكن خاصية كائن: obj.prop)
            if (varName.find('.') == std::string::npos) {
                defineVariable(varName);
            }
            
            return cmd;
        }
        
        // eqPos == 0: Fallback - search string manually
        {
             size_t p = line.find('=');
             if (p != std::string::npos && line.find("==") != p) {
                 std::string rhs = line.substr(p + 1);
                 auto cmd = std::make_shared<Command>(CommandType::ASSIGN);
                 cmd->variable = varName;
                 cmd->value = parseExpression(rhs);
                 
                 // ✅ تسجيل المتغير في جدول الرموز إذا كان تعريفاً (يبدأ بكلمة مفتاحية)
                 if (line.find("متغير ") == 0 || line.find("مت ") == 0 || 
                     line.find("رقم ") == 0 || line.find("نص ") == 0) {
                     defineVariable(varName);
                 }
                 
                 return cmd;
             }
             return nullptr;
        }
    }
    std::shared_ptr<Command> ArabicParser::parseReturn(const std::string& line, int lineNum, int col) {
        auto cmd = std::make_shared<Command>(CommandType::RETURN);
        auto tokens = tokenize(line);
        // Pattern: KEYWORD (ارجع/أرجع) [EXPRESSION]
        if (tokens.size() > 1) {
            std::string exprStr;
            // ⚠️ STRING tokens يجب إعادة تغليفها بعلامات تنصيص لمنع تقسيم النصوص
            for (size_t i = 1; i < tokens.size(); ++i) {
                if (tokens[i].type == Token::STRING) {
                    exprStr += "\"" + tokens[i].value + "\"";
                } else {
                    exprStr += tokens[i].value;
                }
                exprStr += " ";
            }
            if (!exprStr.empty()) {
                cmd->value = parseExpression(exprStr);
            }
        }
        return cmd;
    }

    std::shared_ptr<Command> ArabicParser::parseImport(const std::string& line, int lineNum, int col) {
        auto cmd = std::make_shared<Command>(CommandType::IMPORT);
        auto tokens = tokenize(line);
        // Pattern: KEYWORD (استورد/تضمين) LIB_NAME
        if (tokens.size() > 1) {
            std::string libName = tokens[1].value;
            // Trim quotes if any
            if (!libName.empty() && (libName[0] == '"' || libName[0] == '\'')) {
                libName = libName.substr(1, libName.length() - (libName.length() >= 2 ? 2 : 1));
            }
            cmd->variable = libName;
            cmd->library = libName;  // Set library field for built-in library detection
            
            // ✅ Load and parse the imported file for self-hosting support
            std::string importPath = libName;
            if (!basePath.empty() && libName.find('/') == std::string::npos && libName.find('\\') == std::string::npos) {
                // Construct full path relative to base path
                size_t lastSlash = basePath.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    importPath = basePath.substr(0, lastSlash + 1) + libName;
                }
            }
            
            // Try to load and parse the imported file
            std::ifstream importFile(importPath);
            if (importFile.is_open()) {
                std::stringstream buffer;
                buffer << importFile.rdbuf();
                std::string importContent = buffer.str();
                importFile.close();
                
                // Create a temporary parser to parse the imported file
                ArabicParser tempParser;
                tempParser.setBasePath(importPath);
                
                // Parse the imported content
                std::vector<std::string> importLines;
                std::string currentLine;
                for (size_t i = 0; i < importContent.length(); i++) {
                    if (importContent[i] == '\n') {
                        importLines.push_back(currentLine);
                        currentLine.clear();
                    } else {
                        currentLine += importContent[i];
                    }
                }
                if (!currentLine.empty()) {
                    importLines.push_back(currentLine);
                }
                
                // Process import lines and add classes to our symbol table
                for (size_t i = 0; i < importLines.size(); i++) {
                    auto importedCmd = parseLineWithIndex(importLines, i);
                    if (importedCmd && importedCmd->type == CommandType::CLASS_DEF) {
                        // Add the class to our commands for code generation
                        cmd->body.push_back(importedCmd);
                    }
                }
            }
        }
        return cmd;
    }

    std::shared_ptr<Command> ArabicParser::parseBreak(const std::string& line, int lineNum, int col) {
        return std::make_shared<Command>(CommandType::BREAK);
    }

    std::shared_ptr<Command> ArabicParser::parseContinue(const std::string& line, int lineNum, int col) {
        return std::make_shared<Command>(CommandType::CONTINUE);
    }

    std::shared_ptr<Command> ArabicParser::parseWhileBlock(const std::vector<std::string>& lines, size_t& index) {
        std::string header = trim(lines[index]);
        auto cmd = std::make_shared<Command>(CommandType::LOOP_WHILE);
        
        // Pattern: طالما (شرط):
        // Detect which keyword was used to find the condition start
        std::string kw = "";
        if (header.find("طالما") == 0) kw = "طالما";
        else if (header.find("بينما") == 0) kw = "بينما";
        else if (header.find("كرر_طالما") == 0) kw = "كرر_طالما";

        size_t end = header.find_last_of(':');
        if (!kw.empty() && end != std::string::npos) {
            std::string condStr = header.substr(kw.length(), end - kw.length());
            cmd->condition = std::make_shared<Value>(parseExpression(condStr));
        }
        
        index++;
        while (index < lines.size()) {
            std::string line = trim(lines[index]);
            auto lineTokens = tokenize(line);
            
            if (line == "نهاية") {
                index++;
                break;
            }
            
            if (!lineTokens.empty()) {
                std::string first = lineTokens[0].value;
                if ((lineTokens[0].type == Token::KEYWORD || lineTokens[0].type == Token::IDENTIFIER) &&
                    (first == "دالة" || first == "صنف" || first == "صف" || first == "عام" || first == "خاص")) break;
            }
            
            size_t idxBefore = index;
            auto cmdInside = parseLineWithIndex(lines, index);
            if (cmdInside) cmd->body.push_back(cmdInside);
            if (index == idxBefore) index++; // safety
        }
        
        return cmd;
    }

    std::shared_ptr<Command> ArabicParser::parseForBlock(const std::vector<std::string>& lines, size_t& index) {
        std::string header = trim(lines[index]);
        auto tokens = tokenize(header);
        auto cmd = std::make_shared<Command>(CommandType::LOOP_FOR);
        
        // Pattern: كرر م من 0 إلى 10
        if (tokens.size() >= 7 && tokens[2].value == "من" && (tokens[4].value == "إلى" || tokens[4].value == "الى")) {
            cmd->variable = tokens[1].value;

            // ── المُبدِّئ: الكتابة في value (حقل مباشر) لا في expression (shared_ptr) ──
            cmd->initializer = std::make_shared<Command>(CommandType::ASSIGN);
            cmd->initializer->variable = tokens[1].value;
            cmd->initializer->value = Value(std::stod(tokens[3].value));

            cmd->condition = std::make_shared<Value>(ValueType::COMPARISON);
            cmd->condition->operation = "<=";
            cmd->condition->left  = std::make_shared<Value>(ValueType::VARIABLE, tokens[1].value);
            cmd->condition->right = std::make_shared<Value>(Value(std::stod(tokens[6].value)));

            // ── الزيادة: نفس المبدأ — value مباشرة ──
            cmd->increment = std::make_shared<Command>(CommandType::ASSIGN);
            cmd->increment->variable = tokens[1].value;
            Value incExpr(ValueType::OPERATION);
            incExpr.operation = "+";
            incExpr.left  = std::make_shared<Value>(ValueType::VARIABLE, tokens[1].value);
            incExpr.right = std::make_shared<Value>(Value(1.0));
            cmd->increment->value = incExpr;
        }
        
        index++;
        while (index < lines.size()) {
            std::string line = trim(lines[index]);
            auto lineTokens = tokenize(line);
            
            if (line == "نهاية") {
                index++;
                break;
            }

            if (!lineTokens.empty()) {
                std::string first = lineTokens[0].value;
                if ((lineTokens[0].type == Token::KEYWORD || lineTokens[0].type == Token::IDENTIFIER) &&
                    (first == "دالة" || first == "صنف" || first == "صف" || first == "عام" || first == "خاص")) break;
            }
            
            size_t idxBefore = index;
            auto cmdInside = parseLineWithIndex(lines, index);
            if (cmdInside) cmd->body.push_back(cmdInside);
            if (index == idxBefore) index++;
        }
        
        return cmd;
    }

    std::shared_ptr<Command> ArabicParser::parseForEach(const std::vector<std::string>& lines, size_t& index) {
        std::string header = trim(lines[index]);
        auto tokens = tokenize(header);

        // ── النمط 1: لكل عدد في الأعداد  (for-each على مصفوفة) ──
        if (tokens.size() >= 4 && tokens[0].value == "لكل" && tokens[2].value == "في") {
            auto cmd = std::make_shared<Command>(CommandType::LOOP_FOR_EACH);
            cmd->variable  = tokens[1].value;
            cmd->condition = std::make_shared<Value>(ValueType::VARIABLE, tokens[3].value);
            index++;
            while (index < lines.size()) {
                std::string ln = trim(lines[index]);
                if (ln == "نهاية") { index++; break; }
                size_t before = index;
                auto c = parseLineWithIndex(lines, index);
                if (c) cmd->body.push_back(c);
                if (index == before) index++;
            }
            return cmd;
        }

        // ── النمط 2: لكل ع من 1 إلى 500  (for-range بالأعداد) ──
        // tokens: لكل  متغير  من  ابتداء  إلى/الى  نهاية
        if (tokens.size() >= 6 && tokens[0].value == "لكل" &&
            tokens[2].value == "من" &&
            (tokens[4].value == "إلى" || tokens[4].value == "الى")) {

            auto cmd = std::make_shared<Command>(CommandType::LOOP_FOR);
            cmd->variable = tokens[1].value;

            // ── المُبدِّئ: value مباشرة (لا expression) ──
            cmd->initializer = std::make_shared<Command>(CommandType::ASSIGN);
            cmd->initializer->variable = tokens[1].value;
            cmd->initializer->value    = parseExpression(tokens[3].value);

            // استخراج قيمة النهاية من التوكنز (تجاهل النقطتين)
            std::string endExpr = tokens[5].value;
            // إزالة النقطتين في النهاية
            if (!endExpr.empty() && endExpr.back() == ':') endExpr.pop_back();
            endExpr = trim(endExpr);
            
            // إنشاء قيمة النهاية مباشرة كرقم
            double endVal = 0;
            try { endVal = std::stod(endExpr); } catch (...) { endVal = 0; }

            // شرط النهاية: متغير <= قيمة_نهاية
            cmd->condition = std::make_shared<Value>(ValueType::COMPARISON);
            cmd->condition->operation = "<=";
            cmd->condition->left  = std::make_shared<Value>(ValueType::VARIABLE, tokens[1].value);
            cmd->condition->right = std::make_shared<Value>(Value(endVal));

            // ── الزيادة: value مباشرة ──
            cmd->increment = std::make_shared<Command>(CommandType::ASSIGN);
            cmd->increment->variable = tokens[1].value;
            Value incExpr(ValueType::OPERATION);
            incExpr.operation = "+";
            incExpr.left  = std::make_shared<Value>(ValueType::VARIABLE, tokens[1].value);
            incExpr.right = std::make_shared<Value>(Value(1.0));
            cmd->increment->value = incExpr;

            index++;
            while (index < lines.size()) {
                std::string ln = trim(lines[index]);
                if (ln == "نهاية") { index++; break; }
                size_t before = index;
                auto c = parseLineWithIndex(lines, index);
                if (c) cmd->body.push_back(c);
                if (index == before) index++;
            }
            return cmd;
        }

        // ── fallback: تخطى السطر ──
        index++;
        return nullptr;
    }

    std::shared_ptr<Command> ArabicParser::parseIfBlock(const std::vector<std::string>& lines, size_t& index) {
        std::string header = lines[index];
        auto cmd = std::make_shared<Command>(CommandType::CONDITION);

        // ── استخراج شرط "إذا" ────────────────────────────────────────────
        std::string kw = "إذا";
        size_t kwStart = header.find(kw);
        if (kwStart != std::string::npos) {
            std::string condStr = header.substr(kwStart + kw.length());
            if (!condStr.empty() && condStr.back() == ':') condStr.pop_back();
            condStr = trim(condStr);
            cmd->expression = std::make_shared<Value>(parseExpression(condStr));
            cmd->condition  = cmd->expression;
        }

        index++;
        bool inElse = false;

        while (index < lines.size()) {
            std::string line = trim(lines[index]);
            auto toks = tokenize(line);
            if (toks.empty()) { index++; continue; }

            std::string first = toks[0].value;

            // ── نهاية الكتلة ──
            if (first == "نهاية") { index++; break; }

            // ── وإلا إذا (else-if) — نعاملها كفرع else بسيط ──────────────
            // بنية: "وإلا إذا شرط" أو "وإلا_إذا شرط"
            bool isElseIf = (line.find("وإلا إذا") == 0 || line.find("وإلا_إذا") == 0);
            if (isElseIf) {
                inElse = true;
                // استخراج الشرط الجديد — نُبسِّط بتحويله إلى else-body بدالة إذا مضمَّنة
                // للتبسيط الحالي: كل ما بعد "وإلا" يُعامَل كـ else body
                index++;
                continue;
            }

            // ── وإلا / إلا (else) ─────────────────────────────────────────
            if (first == "وإلا" || first == "إلا") {
                inElse = true;
                index++;
                continue;
            }

            // ── خروج مُبكِّر عند تعريف صنف أو دالة جديدة ──
            if ((toks[0].type == Token::KEYWORD || toks[0].type == Token::IDENTIFIER) &&
                (first == "دالة" || first == "صنف" || first == "صف" ||
                 first == "عام"  || first == "خاص")) break;

            // ── تحليل الأمر وإضافته للجسم الصحيح ──
            size_t before = index;
            auto bodyCmd = parseLineWithIndex(lines, index);
            if (bodyCmd) {
                if (inElse) cmd->else_body.push_back(bodyCmd);
                else        cmd->body.push_back(bodyCmd);
            }
            if (index == before) index++;
        }

        return cmd;
    }

    // ... Helper methods
    std::vector<std::string> ArabicParser::splitByComma(const std::string& input) { return {}; }
    std::string ArabicParser::stripComments(const std::string& line) {
        size_t hash = line.find('#');
        if (hash != std::string::npos) return line.substr(0, hash);
        return line;
    }

    // Implement other missing methods declared in .h
    std::map<std::string, int> ArabicParser::getStatistics() const { return statistics; }
    void ArabicParser::setBasePath(const std::string& path) { basePath = path; }
    void ArabicParser::setCurrentFile(const std::string& path) { currentFile = path; }

    // ✅ تنفيذ الطرق المفقودة لإدارة جدول الرموز
    void ArabicParser::defineVariable(const std::string& varName) {
        if (symbols && !varName.empty()) {
            if (!symbols->hasSymbol(varName)) {
                // إضافة متغير جديد بحجم 8 بايت (الافتراضي)
                symbols->addVariable(varName, 8);
                
            }
            variables[varName] = true;
        }
    }

    void ArabicParser::updateSymbolTable(SymbolTable& newSymbols) {
        this->symbols = &newSymbols;
    }

    bool ArabicParser::isKnownVariable(const std::string& varName) {
        if (variables.count(varName)) return true;
        if (symbols && symbols->hasSymbol(varName)) return true;
        return false;
    }

} // namespace ArabicLanguage
