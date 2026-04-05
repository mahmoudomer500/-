#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef Value
#undef Value
#endif
#ifdef Command
#undef Command
#endif
#endif

#include "ArabicExecutor.h"
#include "ArabicParser.h"
#include "ArabicTextUtils.h"
#include "ArabicMemoryManager.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <fstream>
#include "../modules/ai/ArabicVisionBridge.h"

namespace ArabicLanguage {

ArabicExecutor::ArabicExecutor(std::shared_ptr<ArabicRuntime> runtimePtr)
    : runtime(runtimePtr ? runtimePtr : std::make_shared<ArabicRuntime>()),
      jitEnabled(true) {}

Value ArabicExecutor::execute(const std::vector<std::shared_ptr<Command>>& commands, bool resetRuntime) {
    if (resetRuntime) runtime->reset();
    runtime->pushScope();
    
    try {
        return executeBlock(commands);
    } catch (const std::exception& e) {
        executionErrors++;
        std::cerr << "❌ خطأ غير معالج: " << e.what() << std::endl;
        return Value(ValueType::NONE);
    } catch (...) {
        executionErrors++;
        std::cerr << "❌ خطأ غير معروف" << std::endl;
        return Value(ValueType::NONE);
    }
}

Value ArabicExecutor::executeBlock(const std::vector<std::shared_ptr<Command>>& commands) {
    if (shouldStop) return Value(ValueType::NONE); // ✅ تحقق من الإيقاف

    // First, execute all function, class, and interface definitions to ensure they are defined before use
    for (const auto& cmd : commands) {
        if (!cmd) continue;
        if (cmd->type == CommandType::FUNCTION_DEF || cmd->type == CommandType::CLASS_DEF || cmd->type == CommandType::INTERFACE_DEF) {
            executeCommand(cmd);
        }
    }

    // Then execute the rest of the commands
    Value last(ValueType::NONE);
    for (const auto& cmd : commands) {
        if (shouldStop) break; // ✅ تحقق من الإيقاف
        if (!cmd) continue;
        if (cmd->type == CommandType::FUNCTION_DEF || cmd->type == CommandType::CLASS_DEF || cmd->type == CommandType::INTERFACE_DEF) {
            continue; // Already executed
        }
        last = executeCommand(cmd);
        if (runtime->hasReturn()) {
            return runtime->getReturnValue();
        }
        if (runtime->hasBreak() || runtime->hasContinue()) {
            break;
        }
    }
    return last;
}

Value ArabicExecutor::executeCommand(const std::shared_ptr<Command>& cmd) {
    if (!cmd) return Value(ValueType::NONE);
    
    commandCount++;
    if (commandCount > 50000000) {
        throwError("تم تجاوز الحد الأقصى للأوامر (50,000,000) - احتمال وجود حلقة لانهائية أو كود ضخم جداً");
        return Value(ValueType::NONE);
    }
    switch (cmd->type) {
        case CommandType::ASSIGN:
        case CommandType::DECLARE:
        case CommandType::ASSIGNMENT: return executeAssignment(cmd);
        case CommandType::PRINT: return executePrint(cmd);
        case CommandType::PRINT_NO_NEWLINE: return executePrintNoNewline(cmd);
        case CommandType::CONDITION: return executeIf(cmd);
        case CommandType::ELSE_IF: return executeIf(cmd);
        case CommandType::LOOP_WHILE: return executeWhile(cmd);
        case CommandType::LOOP_FOR: return executeFor(cmd);
        case CommandType::LOOP_FOR_EACH: return executeForEach(cmd);
        case CommandType::FUNCTION_DEF: return executeFunctionDef(cmd);
        case CommandType::CLASS_METHOD_DEF: return executeClassMethodDef(cmd);
        case CommandType::FUNCTION_CALL: return executeFunctionCall(cmd);
        case CommandType::RETURN: return executeReturn(cmd);
        case CommandType::WAIT: return executeWait(cmd);
        case CommandType::ARRAY_ASSIGNMENT: return executeArrayAssignment(cmd);
        case CommandType::CLASS_DEF: return executeClassDef(cmd);
        case CommandType::CREATE_OBJECT: return executeObjectCreation(cmd);
        case CommandType::METHOD_CALL: return executeMethodCall(cmd);
        case CommandType::IMPORT: return executeImport(cmd);
        case CommandType::PROPERTY_DEF: return executePropertyDef(cmd);
        case CommandType::INTERFACE_DEF: return executeInterfaceDef(cmd);
        case CommandType::BREAK: return executeBreak(cmd);
        case CommandType::CONTINUE: return executeContinue(cmd);
        case CommandType::TRY: return executeTry(cmd);
        case CommandType::THROW: return executeThrow(cmd);
        case CommandType::PUSH_SCOPE: runtime->pushScope(); return Value(ValueType::NONE);
        case CommandType::POP_SCOPE: runtime->popScope(); return Value(ValueType::NONE);
        
        // 📁 File I/O Commands
        case CommandType::FILE_OPEN: {
            Value path = evaluateExpression(*cmd->arguments[0]);
            Value mode = evaluateExpression(*cmd->arguments[1]);
            // Use the native function implementation for consistency
            std::vector<Value> args = {path, mode};
            Value handle = runtime->getNativeFunction("افتح_ملف")(args);
            runtime->setVariable(cmd->variable, handle);
            return handle;
        }
        case CommandType::FILE_READ: {
            Value handle = evaluateExpression(*cmd->arguments[0]);
            Value content = runtime->getNativeFunction("اقرأ_سطر")({handle});
            runtime->setVariable(cmd->variable, content);
            return content;
        }
        case CommandType::FILE_READ_ALL: {
            Value handle = evaluateExpression(*cmd->arguments[0]);
            Value content;
            if (handle.string_value.find("file_") == 0) {
                // handle-based read
                content = runtime->getNativeFunction("اقرأ_كل")({handle});
            } else {
                // direct filename read
                content = runtime->getNativeFunction("اقرأ_ملف")({handle});
            }
            runtime->setVariable(cmd->variable, content);
            return content;
        }
        case CommandType::FILE_WRITE: {
            Value handle = evaluateExpression(*cmd->arguments[0]);
            Value content = evaluateExpression(*cmd->arguments[1]);
            if (handle.string_value.find("file_") == 0) {
                // handle-based write
                return runtime->getNativeFunction("اكتب")({handle, content});
            } else {
                // direct filename write
                return runtime->getNativeFunction("اكتب_ملف")({handle, content});
            }
        }
        case CommandType::FILE_CLOSE: {
            Value handle = evaluateExpression(*cmd->arguments[0]);
            return runtime->getNativeFunction("أغلق_ملف")({handle});
        }
        
        default: return Value(ValueType::NONE);
    }
}

Value ArabicExecutor::executeAssignment(const std::shared_ptr<Command>& cmd) {
    // ── اختيار مصدر القيمة: value (حقل مباشر) أو expression (مؤشر shared_ptr) ──
    // المحلل القديم يكتب في cmd->value، المحلل الجديد (parseForEach / parseForBlock)
    // يكتب في cmd->expression — نتحقق من كليهما لضمان التوافق.
    Value val;
    if (cmd->value.type != ValueType::NONE) {
        val = evaluateExpression(cmd->value);
    } else if (cmd->expression && cmd->expression->type != ValueType::NONE) {
        val = evaluateExpression(*cmd->expression);
    } else {
        val = Value(ValueType::NONE);
    }
    

    // التحقق مما إذا كان التعيين لخاصية كائن (مثال: أحمد.الاسم = "أحمد")
    size_t dotPos = cmd->variable.find('.');
    if (dotPos != std::string::npos) {
        std::string objName = cmd->variable.substr(0, dotPos);
        std::string propName = cmd->variable.substr(dotPos + 1);

        Value objVal;
    // التحقق من "الأب" (الأب أو سوبر) بشكل يدوي لتجنب مشاكل الترميز
    if (isSuperKeyword(objName)) {
        objVal = runtime->getVariable("هذا");
    } else if (isThisKeyword(objName)) {
        objVal = runtime->getVariable("هذا");
    } else {
        // تنظيف اسم المتغير من أي رموز غير مرغوبة قد تكون تسربت
        objVal = runtime->getVariable(trim(objName));
    }

        if (objVal.type == ValueType::OBJECT || objVal.type == ValueType::GENERIC_MAP) {
            objVal.map_elements[propName] = val;
            // تحديث المتغير في النطاق
            runtime->setVariable(trim(objName), objVal);
            return val;
        }

        if ((objVal.type == ValueType::OBJECT || objVal.type == ValueType::CLASS_INSTANCE) && !objVal.string_value.empty()) {
            
            runtime->setObjectProperty(objVal.string_value, propName, val);
            return val;
        }

        throwError("فقط الكائنات (instances) تملك حقولاً. (النوع الحالي: " + valueTypeToString(objVal.type) + ")");
        return Value(ValueType::NONE);
    }

    // تنظيف اسم المتغير
    std::string cleanName = trim(cmd->variable);
    
    if (cmd->type == CommandType::DECLARE) {
        
        runtime->defineVariable(cleanName, val);
    } else {
        // إذا كان تعييناً (ASSIGNMENT) وليس تعريفاً، نستخدم setVariable التي تبحث في النطاقات
        // ولكننا سنقوم بتعديلها لتكون أكثر صرامة إذا لزم الأمر في المستقبل.
        // حالياً، نضمن أن التعيين لا ينشئ متغيراً جديداً في النطاق الحالي إذا كان موجوداً في نطاق أعلى.
        
        runtime->setVariable(cleanName, val);
    }
    
    return val;
}

Value ArabicExecutor::executePrint(const std::shared_ptr<Command>& cmd) {
    std::stringstream ss;
    for (size_t i = 0; i < cmd->arguments.size(); ++i) {
        Value v = evaluateExpression(*cmd->arguments[i]);
        ss << valueToString(v);
        // إضافة مسافة إذا لم يكن هذا هو العنصر الأخير
        if (i < cmd->arguments.size() - 1) ss << " ";
    }
    
    std::string output = ss.str();
    if (outputCallback) {
        outputCallback(output + "\n");
    } else {
        std::cout << output << std::endl;
    }
    return Value(ValueType::NONE);
}

Value ArabicExecutor::executePrintNoNewline(const std::shared_ptr<Command>& cmd) {
    std::stringstream ss;
    for (size_t i = 0; i < cmd->arguments.size(); ++i) {
        Value v = evaluateExpression(*cmd->arguments[i]);
        ss << valueToString(v);
        // إضافة مسافة إذا لم يكن هذا هو العنصر الأخير
        if (i < cmd->arguments.size() - 1) ss << " ";
    }
    
    if (outputCallback) {
        outputCallback(ss.str());
    } else {
        std::cout << ss.str();
    }
    return Value(ValueType::NONE);
}

Value ArabicExecutor::executeIf(const std::shared_ptr<Command>& cmd) {
    if (!cmd->condition) return executeBlock(cmd->else_body);
    Value cond = evaluateCondition(*cmd->condition);
    bool truthy = isTruthy(cond);
    if (truthy) {
        return executeBlock(cmd->body);
    } else {
        return executeBlock(cmd->else_body);
    }
}

Value ArabicExecutor::executeWhile(const std::shared_ptr<Command>& cmd) {
    Value last(ValueType::NONE);
    if (!cmd->condition) return last;
    while (!shouldStop && isTruthy(evaluateCondition(*cmd->condition))) {
        runtime->pushScope();
        runtime->clearBreak();
        runtime->clearContinue();
        last = executeBlock(cmd->body);
        runtime->popScope();
        
        if (shouldStop) break;
        if (runtime->hasBreak()) { runtime->clearBreak(); break; }
        if (runtime->hasReturn()) break;
        if (runtime->hasContinue()) { runtime->clearContinue(); }

        // ضخ أحداث الواجهة الرسومية لمنع تجمد النوافذ
        WindowManager::pumpMessages();
    }
    return last;
}

Value ArabicExecutor::executeFor(const std::shared_ptr<Command>& cmd) {
    if (cmd->initializer) {
        runtime->pushScope();
        executeCommand(cmd->initializer);
        
        Value last(ValueType::NONE);
        int maxIter = 1000;
        while (!shouldStop && cmd->condition && maxIter-- > 0) {
            bool condResult = false;
            try {
                Value condVal = evaluateCondition(*cmd->condition);
                condResult = isTruthy(condVal);
            } catch (...) {
                break;
            }
            if (!condResult) break;
            runtime->clearBreak();
            runtime->clearContinue();
            
            last = executeBlock(cmd->body);
            if (shouldStop) break;
            
            if (runtime->hasBreak()) { runtime->clearBreak(); break; }
            if (runtime->hasReturn()) break;
            if (runtime->hasContinue()) { runtime->clearContinue(); }
            
            if (cmd->increment) {
                executeCommand(cmd->increment);
            }

            WindowManager::pumpMessages();
        }
        
        runtime->popScope();
        return last;
    }

    // Traditional range-based for loop
    Value startVal = evaluateExpression(cmd->value);
    Value endVal = evaluateExpression(*cmd->condition);
    double start = valueToNumber(startVal);
    double end = valueToNumber(endVal);
    double step = 1.0;
    if (!cmd->arguments.empty()) step = valueToNumber(evaluateExpression(*cmd->arguments[0]));

    // Use a helper to convert double to string without trailing zeros if it's an integer
    auto doubleToString = [](double d) {
        std::string s = std::to_string(d);
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    };

    runtime->pushScope();
    runtime->defineVariable(cmd->variable, Value(ValueType::NUMBER, doubleToString(start)));
    Value last(ValueType::NONE);
    if (step >= 0) {
        for (double i = start; i <= end && !shouldStop; i += step) {
            runtime->setVariable(cmd->variable, Value(ValueType::NUMBER, doubleToString(i)));
            runtime->clearBreak();
            runtime->clearContinue();
            last = executeBlock(cmd->body);
            if (shouldStop) break;
            if (runtime->hasBreak()) { runtime->clearBreak(); break; }
            if (runtime->hasReturn()) break;
            if (runtime->hasContinue()) { runtime->clearContinue(); }
            WindowManager::pumpMessages();
        }
    } else {
        for (double i = start; i >= end && !shouldStop; i += step) {
            runtime->setVariable(cmd->variable, Value(ValueType::NUMBER, doubleToString(i)));
            runtime->clearBreak();
            runtime->clearContinue();
            last = executeBlock(cmd->body);
            if (shouldStop) break;
            if (runtime->hasBreak()) { runtime->clearBreak(); break; }
            if (runtime->hasReturn()) break;
            if (runtime->hasContinue()) { runtime->clearContinue(); }
            WindowManager::pumpMessages();
        }
    }
    runtime->popScope();
    return last;
}

Value ArabicExecutor::executeForEach(const std::shared_ptr<Command>& cmd) {
    Value collection = evaluateExpression(*cmd->condition);
    
    // دعم التكرار على المصفوفات والقواميس (OBJECT)
    if (collection.type != ValueType::ARRAY && collection.type != ValueType::OBJECT) {
        std::cerr << "❌ [Executor] محاولة التكرار على نوع غير مدعوم (يجب أن يكون مصفوفة أو قاموس): " << valueTypeToString(collection.type) << " (القيمة: " << valueToString(collection) << ")" << std::endl;
        throwError("محاولة التكرار على نوع غير مدعوم");
        return Value(ValueType::NONE);
    }

    runtime->pushScope();
    
    // فحص إذا كان هناك متغيران (للقواميس: مفتاح، قيمة)
    std::string varName = cmd->variable;
    std::string keyVar = varName;
    std::string valVar = "";
    
    size_t commaPos = varName.find(',');
    if (commaPos != std::string::npos) {
        keyVar = ArabicLanguage::trim(varName.substr(0, commaPos));
        valVar = ArabicLanguage::trim(varName.substr(commaPos + 1));
        runtime->defineVariable(keyVar, Value(ValueType::NONE));
        runtime->defineVariable(valVar, Value(ValueType::NONE));
    } else {
        runtime->defineVariable(varName, Value(ValueType::NONE));
    }

    Value last(ValueType::NONE);
    
    if (collection.type == ValueType::ARRAY) {
        for (const auto& item : collection.elements) {
            if (shouldStop) break;
            runtime->setVariable(keyVar, item);
            
            runtime->clearBreak();
            runtime->clearContinue();
            last = executeBlock(cmd->body);
            if (shouldStop) break;
            if (runtime->hasBreak()) { runtime->clearBreak(); break; }
            if (runtime->hasContinue()) { runtime->clearContinue(); }
            if (runtime->hasReturn()) break;
            WindowManager::pumpMessages();
        }
    } else if (collection.type == ValueType::OBJECT) {
        for (const auto& pair : collection.map_elements) {
            if (shouldStop) break;
            
            // تعيين المفتاح (دائماً نص)
            runtime->setVariable(keyVar, Value(ValueType::STRING, pair.first));
            
            // تعيين القيمة إذا طلب المستخدم ذلك (لكل ك، ق في قاموس)
            if (!valVar.empty()) {
                runtime->setVariable(valVar, pair.second);
            }
            
            runtime->clearBreak();
            runtime->clearContinue();
            last = executeBlock(cmd->body);
            if (shouldStop) break;
            if (runtime->hasBreak()) { runtime->clearBreak(); break; }
            if (runtime->hasContinue()) { runtime->clearContinue(); }
            if (runtime->hasReturn()) break;
            WindowManager::pumpMessages();
        }
    }
    
    runtime->popScope();
    return last;
}

Value ArabicExecutor::executeClassMethodDef(const std::shared_ptr<Command>& cmd) {
    std::string funcName = cmd->function_name.empty() ? cmd->variable : cmd->function_name;
    
    
    for (const auto& p : cmd->parameters) std::cout << p << " ";
    std::cout << std::endl;
    
    // البحث عن اسم الصنف في الأطفال
    std::string className = "";
    for (const auto& child : cmd->class_def->methods) {
        if (child.first == funcName) {
            className = cmd->class_name;
            break;
        }
    }
    
    if (!className.empty()) {
        // إضافة الدالة كدالة عضو في الصنف
        auto classInfo = runtime->getClassInfo(className);
        if (classInfo) {
            ClassMethod method;
            method.name = funcName;
            method.parameters = cmd->parameters;
            method.body = cmd->body;
            method.is_abstract = false;
            method.return_type = ValueType::NONE;
            const_cast<ClassDefinition*>(classInfo)->methods[funcName] = method;
            
            
        }
    }
    
    return Value(ValueType::NONE);
}

Value ArabicExecutor::executeFunctionDef(const std::shared_ptr<Command>& cmd) {
    std::string funcName = cmd->function_name.empty() ? cmd->variable : cmd->function_name;
    
    
    for (const auto& p : cmd->parameters) std::cout << p << " ";
    std::cout << std::endl;
    
    // التحقق مما إذا كانت دالة عضو في صنف من خلال اسم الدالة
    if (!funcName.empty() && funcName.find("@") != std::string::npos) {
        std::size_t atPos = funcName.find("@");
        if (atPos != std::string::npos) {
            std::string className = funcName.substr(0, atPos);
            
            
            // تسجيل الدالة كدالة عضو في الصنف
            auto classInfo = runtime->getClassInfo(className);
            if (classInfo) {
                ClassMethod method;
                method.name = funcName;
                method.parameters = cmd->parameters;
                method.body = cmd->body;
                method.is_abstract = false;
                method.return_type = ValueType::NONE;
                const_cast<ClassDefinition*>(classInfo)->methods[funcName] = method;
                
                
            }
        }
    }
    
    // إضافة العد المرجعي لتعريف الدالة
    ArabicMemoryManager::getInstance().getReferenceCounter().add(
        cmd.get(), nullptr, "function_definition_" + funcName);
    
    runtime->defineFunction(funcName, cmd);
    return Value(ValueType::NONE);
}


Value ArabicExecutor::executeFunctionCall(const std::shared_ptr<Command>& cmd) {
    std::string name = !cmd->function_name.empty() ? cmd->function_name : cmd->variable;
    
    // Debug: track all function calls
    std::cerr << "[DEBUG] executeFunctionCall: name='" << name << "', args.size()=" << cmd->arguments.size() << std::endl;
    
    // ✅ دعم الدوال المدمجة الأساسية للمرحلة الثالثة
    if (name == "حصول_على_عنوان") {
        if (cmd->arguments.empty()) return Value(ValueType::NONE);
        Value val = evaluateExpression(*cmd->arguments[0]);
        if (!val.string_value.empty()) {
            return Value(ValueType::STRING, val.string_value);
        }
        if (val.type == ValueType::OBJECT || val.type == ValueType::ARRAY || val.type == ValueType::GENERIC_MAP) {
             // توليد معرف فريد مؤقت للقيم الحرفية (Literals)
             static int nextLitId = 1;
             return Value(ValueType::STRING, "LIT_" + std::to_string(nextLitId++));
        }
        return Value(ValueType::STRING, "ADDR_PTR_" + std::to_string((size_t)cmd.get())); 
    }
    if (name == "وقت_الحالي") {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return Value(ValueType::NUMBER, std::to_string(ms));
    }
    if (name == "مفاتيح") {
        if (cmd->arguments.empty()) return Value(ValueType::ARRAY);
        Value mapVal = evaluateExpression(*cmd->arguments[0]);
        Value keys(ValueType::ARRAY);
        if (mapVal.type == ValueType::GENERIC_MAP || mapVal.type == ValueType::OBJECT) {
            for (auto const& [key, val] : mapVal.map_elements) {
                keys.elements.push_back(Value(ValueType::STRING, key));
            }
        }
        return keys;
    }
    if (name == "حجم") {
        if (cmd->arguments.empty()) return Value(ValueType::NUMBER, "0");
        Value val = evaluateExpression(*cmd->arguments[0]);
        if (val.type == ValueType::STRING) return Value(ValueType::NUMBER, std::to_string(val.string_value.length()));
        if (val.type == ValueType::ARRAY) return Value(ValueType::NUMBER, std::to_string(val.elements.size()));
        if (val.type == ValueType::GENERIC_MAP || val.type == ValueType::OBJECT)        return Value(ValueType::NUMBER, std::to_string(val.map_elements.size()));
        return Value(ValueType::NUMBER, "0");
    }
    if (name == "نوع") {
        if (cmd->arguments.empty()) return Value(ValueType::STRING, "لا شيء");
        Value val = evaluateExpression(*cmd->arguments[0]);
        return Value(ValueType::STRING, valueTypeToString(val.type));
    }
    if (name == "نص_إلى_رقم") {
        if (cmd->arguments.empty()) return Value(ValueType::NUMBER, "0");
        Value val = evaluateExpression(*cmd->arguments[0]);
        return Value(ValueType::NUMBER, std::to_string(valueToNumber(val)));
    }
    if (name == "رقم_إلى_نص") {
        if (cmd->arguments.empty()) return Value(ValueType::STRING, "");
        Value val = evaluateExpression(*cmd->arguments[0]);
        return Value(ValueType::STRING, valueToString(val));
    }
    if (name == "أضف") {
        if (cmd->arguments.size() < 2) return Value(ValueType::NONE);
        Value arr = evaluateExpression(*cmd->arguments[0]);
        Value item = evaluateExpression(*cmd->arguments[1]);
        if (arr.type == ValueType::ARRAY) {
            arr.elements.push_back(item);
            // Updating the original variable if possible
            if (cmd->arguments[0]->type == ValueType::VARIABLE) {
                runtime->setVariable(cmd->arguments[0]->value, arr);
            }
        }
        return arr;
    }

    if (!runtime->hasFunction(name)) {
        throwError("دالة غير معرّفة: " + name);
        return Value(ValueType::NONE);
    }


    // 1. تحقق من وجود دالة معرّفة في الكود العربي أولاً (للسماح بتجاوز الدوال الأصلية)
    auto fn = runtime->getFunction(name);
    if (fn) {
        auto startTime = std::chrono::steady_clock::now();
        
        // ✅ 1. تقييم المعاملات في السياق الحالي (قبل دفع نطاق جديد)
        std::vector<Value> evaluatedArgs;
        for (const auto& argExpr : cmd->arguments) {
            evaluatedArgs.push_back(evaluateExpression(*argExpr));
        }

        runtime->pushScope();
          runtime->pushScopeBase(); // ✅ عزل النطاق الحالي للدالة
          runtime->pushReturnFrame(); // ✅ دفع إطار إرجاع جديد لهذه المكالمة
          
          // تسجيل الدخول للدالة للتتبع
          
          
          for (size_t i = 0; i < fn->parameters.size(); ++i) {
              Value argVal = i < evaluatedArgs.size() ? evaluatedArgs[i] : Value(ValueType::NONE);
              // std::cout << "  DEBUG: Defining param " << fn->parameters[i] << " in scope " << (runtime->scopeCount()-1) << std::endl;
              runtime->defineVariable(fn->parameters[i], argVal);
          }

          // ✅ تنفيذ جسم الدالة وجمع قيمة الإرجاع
          Value ret(ValueType::NONE);
          try {
              ret = executeBlock(fn->body);
          } catch (const std::exception& e) {
              std::cout << "❌ [Executor] Error in function '" << name << "': " << e.what() << std::endl;
              runtime->popReturnFrame();
              runtime->popScopeBase();
              runtime->popScope();
              throw;
          } catch (...) {
              runtime->popReturnFrame();
              runtime->popScopeBase();
              runtime->popScope();
              throw;
          }

          // ✅ التحقق من وجود قيمة إرجاع محددة (من ارجع)
          if (runtime->hasReturn()) {
              ret = runtime->getReturnValue();
              runtime->clearReturn();
          }

          
          runtime->popReturnFrame(); // ✅ سحب إطار الإرجاع
          runtime->popScopeBase();   // ✅ إلغاء عزل النطاق
          runtime->popScope();

        auto endTime = std::chrono::steady_clock::now();
        auto executionTime = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);

        // تسجيل استدعاء الدالة
        recordFunctionCall(name, executionTime);

        // ✅ تخزين النتيجة في cmd->variable إذا كان محدداً (للاستخدام في الجسور)
        if (!cmd->variable.empty()) {
            runtime->setVariable(cmd->variable, ret);
        }

        return ret;
    }

    // 2. إذا لم تكن معرّفة في الكود العربي، تحقق من الدوال الأصلية
    auto nativeFn = runtime->getNativeFunction(name);
    if (nativeFn) {
        std::vector<Value> args;
        for (const auto& argExpr : cmd->arguments) {
            args.push_back(evaluateExpression(*argExpr));
        }
        try {
            Value result = nativeFn(args);

            // ✅ دعم تعديل المصفوفات في الدوال المدمجة (مثل أضف)
             if ((name == "أضف" || name == "أضيف") && !cmd->arguments.empty() && cmd->arguments[0]->type == ValueType::VARIABLE) {
                 runtime->setVariable(cmd->arguments[0]->value, result);
             }

            return result;
        } catch (const std::exception& e) {
            throwError("خطأ في الدالة '" + name + "': " + e.what());
            return Value(ValueType::NONE);
        }
    }

    // 3. إذا لم توجد في أي مكان
    throwError("فشل في استرجاع تعريف الدالة: " + name);
    return Value(ValueType::NONE);
}

Value ArabicExecutor::executeReturn(const std::shared_ptr<Command>& cmd) {
    // fallback: إذا كان value فارغاً، نجرب expression
    Value v;
    if (cmd->value.type != ValueType::NONE) {
        v = evaluateExpression(cmd->value);
    } else if (cmd->expression && cmd->expression->type != ValueType::NONE) {
        v = evaluateExpression(*cmd->expression);
    } else {
        v = Value(ValueType::NONE);
    }
    runtime->setReturnValue(v);
    return v;
}

Value ArabicExecutor::executeWait(const std::shared_ptr<Command>&) {
    return Value(ValueType::NONE);
}

Value ArabicExecutor::executeArrayAssignment(const std::shared_ptr<Command>& cmd) {
    Value arr;
    std::string objName;
    std::string propName;
    size_t dotPos = cmd->variable.find('.');
    
    if (dotPos != std::string::npos) {
        objName = cmd->variable.substr(0, dotPos);
        propName = cmd->variable.substr(dotPos + 1);
        
        Value objVal;
        if (isThisKeyword(objName) || isSuperKeyword(objName)) {
            objVal = runtime->getVariable("هذا");
        } else {
            objVal = runtime->getVariable(trim(objName));
        }
        
        if (objVal.type == ValueType::OBJECT || objVal.type == ValueType::GENERIC_MAP) {
            auto it = objVal.map_elements.find(propName);
            if (it != objVal.map_elements.end()) {
                arr = it->second;
            } else {
                arr = Value(ValueType::GENERIC_MAP); // Default to map if missing
            }
        } else if (objVal.type == ValueType::CLASS_INSTANCE && !objVal.string_value.empty()) {
            arr = runtime->getObjectProperty(objVal.string_value, propName);
        } else {
            throwError("لا يمكن استخدام الوصول بالفهرس على هذا النوع: " + valueTypeToString(objVal.type));
            return Value(ValueType::NONE);
        }
    } else {
        arr = runtime->hasVariable(cmd->variable) ? runtime->getVariable(cmd->variable) : Value(ValueType::ARRAY);
    }
    
    Value result;
    // Check if it's a Dictionary or we should treat it as one
    if (arr.type == ValueType::GENERIC_MAP || arr.type == ValueType::OBJECT) {
        Value keyVal = evaluateExpression(*cmd->condition);
        std::string key;
        if (keyVal.type == ValueType::STRING) key = keyVal.string_value;
        else if (keyVal.type == ValueType::NUMBER) key = valueToString(keyVal);
        else { throwError("مفتاح القاموس يجب أن يكون نصاً أو رقماً"); return Value(ValueType::NONE); }

        Value newVal = evaluateExpression(cmd->value);
        arr.map_elements[key] = newVal;
        result = arr;
    } else {
        // Array logic
        if (arr.type != ValueType::ARRAY) arr.type = ValueType::ARRAY;
        Value idxVal = evaluateExpression(*cmd->condition);
        int idx = static_cast<int>(valueToNumber(idxVal));
        if (idx < 0) { throwError("فهرس مصفوفة سالب"); return Value(ValueType::NONE); }
        if (static_cast<size_t>(idx) >= arr.elements.size()) arr.elements.resize(idx + 1, Value(ValueType::NONE));

        Value newVal = evaluateExpression(cmd->value);
        arr.elements[idx] = newVal;
        result = arr;
    }

    // Save back
    if (dotPos != std::string::npos) {
        Value objVal;
        if (isThisKeyword(objName) || isSuperKeyword(objName)) {
            objVal = runtime->getVariable("هذا");
        } else {
            objVal = runtime->getVariable(trim(objName));
        }

        if (objVal.type == ValueType::OBJECT || objVal.type == ValueType::GENERIC_MAP) {
            objVal.map_elements[propName] = result;
            runtime->setVariable(trim(objName), objVal);
        } else if (objVal.type == ValueType::CLASS_INSTANCE && !objVal.string_value.empty()) {
            runtime->setObjectProperty(objVal.string_value, propName, result);
        }
    } else {
        runtime->setVariable(cmd->variable, result);
    }
    
    return result;
}

Value ArabicExecutor::executeClassDef(const std::shared_ptr<Command>& cmd) {
    if (cmd->class_def) {
        // ✅ Stage 6: التحقق من تنفيذ جميع الواجهات المطلوبة
        for (const auto& interfaceName : cmd->class_def->interfaces) {
            const InterfaceDefinition* interfaceDef = runtime->getInterfaceInfo(interfaceName);
            if (interfaceDef) {
                for (const auto& methodName : interfaceDef->methods) {
                    bool methodFound = false;
                    
                    // البحث في الصنف الحالي
                    if (cmd->class_def->methods.count(methodName)) {
                        methodFound = true;
                    } else {
                        // البحث في سلسلة الوراثة
                        std::string currentParent = cmd->class_def->parent_class;
                        while (!currentParent.empty()) {
                            const ClassDefinition* parentDef = runtime->getClassInfo(currentParent);
                            if (parentDef && parentDef->methods.count(methodName)) {
                                methodFound = true;
                                break;
                            }
                            currentParent = (parentDef) ? parentDef->parent_class : "";
                        }
                    }

                    if (!methodFound) {
                        throwError("الصنف '" + cmd->class_name + "' لا ينفذ الدالة المطلوبة '" + methodName + "' من الواجهة '" + interfaceName + "'");
                    }
                }
            } else {
                std::cout << "⚠️ تحذير: لم يتم العثور على الواجهة '" << interfaceName << "'" << std::endl;
            }
        }

        runtime->defineClass(cmd->class_name, *cmd->class_def);
        // ✅ تسجيل اسم الصنف كمتغير لتمكين الوصول للدوال الثابتة (مثل: صنف.جديد())
        runtime->defineVariable(cmd->class_name, Value(ValueType::CLASS_DEFINITION, cmd->class_name));
    }
    return Value(ValueType::NONE);
}

Value ArabicExecutor::executeInterfaceDef(const std::shared_ptr<Command>& cmd) {
    if (cmd->interface_def) {
        runtime->defineInterface(cmd->interface_name, *cmd->interface_def);
    }
    return Value(ValueType::NONE);
}

Value ArabicExecutor::executeObjectCreation(const std::shared_ptr<Command>& cmd) {
    // ✅ Stage 6: منع إنشاء كائنات من الأصناف المجردة
    const ClassDefinition* classInfo = runtime->getClassInfo(cmd->class_name);
    if (classInfo && classInfo->is_abstract) {
        throwError("لا يمكن إنشاء كائن من صنف مجرد: " + cmd->class_name);
        return Value(ValueType::NONE);
    }

    auto instance = runtime->createObjectInstance(cmd->class_name);
    if (!instance) {
        throwError("فشل في إنشاء كائن من النوع: " + cmd->class_name);
        return Value(ValueType::NONE);
    }
    
    runtime->registerObjectInstance(instance);
    
    // ✅ تهيئة الخصائص بالقيم الابتدائية (بتسلسل من الأب إلى الابن)
    std::vector<const ClassDefinition*> hierarchy;
    std::string classIter = cmd->class_name;
    while (!classIter.empty()) {
        const ClassDefinition* def = runtime->getClassInfo(classIter);
        if (!def) break;
        hierarchy.push_back(def);
        classIter = def->parent_class;
    }
    
    // التكرار من الأقدم (الأب) إلى الأحدث (الابن) لضمان أن قيم الابن تغطي الأب
    for (auto it = hierarchy.rbegin(); it != hierarchy.rend(); ++it) {
        const ClassDefinition* def = *it;
        for (const auto& pair : def->properties) {
            const auto& p = pair.second;
            if (p.default_value && p.default_value->type != ValueType::NONE) {
                // تقييم التعبير المبدئي
                Value val = evaluateExpression(*p.default_value);
                runtime->setObjectProperty(instance->unique_id, p.name, val);
            }
        }
    }

    Value objVal(ValueType::CLASS_INSTANCE);
    objVal.class_type = cmd->class_name;
    objVal.string_value = instance->unique_id;
    
    // ✅ تخزين الكائن في cmd->object_name أو cmd->variable (للاستخدام في الجسور)
    std::string objVarName = cmd->object_name.empty() ? cmd->variable : cmd->object_name;
    if (!objVarName.empty()) {
        runtime->setVariable(objVarName, objVal);
    }
    
    // ✅ تنفيذ المُنشئ إذا وجد (دالة بنفس اسم الصنف أو دالة تسمى "جديد")
    std::string constructorName = "";
    if (runtime->classHasMethod(cmd->class_name, cmd->class_name)) {
        constructorName = cmd->class_name;
    } else if (runtime->classHasMethod(cmd->class_name, "جديد")) {
        constructorName = "جديد";
    }

    if (!constructorName.empty()) {
        std::shared_ptr<Command> constructorCall = std::make_shared<Command>(CommandType::METHOD_CALL);
        
        // ✅ استخدام اسم المتغير الصحيح (object_name أو variable)
        std::string tempName = objVarName.empty() ? "__temp_obj_" + instance->unique_id : objVarName;
        constructorCall->object_name = tempName;
        
        // ✅ التأكد من أن الكائن مسجل في جدول الرموز قبل استدعاء المنشئ
        // (تم تسجيله أعلاه، لكن نتحقق مرة أخرى)
        runtime->setVariable(tempName, objVal);
        
        // ✅ التحقق من أن الكائن مسجل بشكل صحيح
        if (!runtime->hasVariable(tempName)) {
            throwError("فشل في تسجيل الكائن '" + tempName + "' قبل استدعاء المنشئ (عدد النطاقات: " + std::to_string(runtime->scopeCount()) + ")");
            return objVal; // إرجاع الكائن بدون تنفيذ المنشئ
        }
        
        // ✅ التحقق من أن objVal صحيح
        Value verifyObj = runtime->getVariable(tempName);
        if (verifyObj.type != ValueType::OBJECT && verifyObj.type != ValueType::CLASS_INSTANCE) {
            throwError("الكائن '" + tempName + "' غير صالح (النوع: " + valueTypeToString(verifyObj.type) + ")");
            return objVal;
        }

        constructorCall->method_name = constructorName;
        constructorCall->arguments = cmd->arguments;
        
        // ✅ تنفيذ المنشئ - سيتم تعريف "هذا" داخل executeMethodCall
        // ✅ التأكد من أن objVal يحتوي على الكائن الصحيح قبل الاستدعاء
        try {
            // ✅ رسائل تصحيح
            // std::cout << "🚀 تنفيذ المنشئ (" << constructorName << ") لـ: " << tempName << std::endl;
            // std::cout << "   الكائن موجود: " << (runtime->hasVariable(tempName) ? "نعم" : "لا") << std::endl;
            // std::cout << "   عدد النطاقات قبل الاستدعاء: " << runtime->scopeCount() << std::endl;
            
            Value constructorResult = executeMethodCall(constructorCall);
            
            // ✅ تحديث قيمة الكائن بعد تنفيذ المنشئ (في حالة تعديله)
            if (!objVarName.empty() && runtime->hasVariable(tempName)) {
                Value updatedObj = runtime->getVariable(tempName);
                if (updatedObj.type == ValueType::OBJECT || updatedObj.type == ValueType::CLASS_INSTANCE) {
                    objVal = updatedObj;
                }
            }
        } catch (const std::exception& e) {
            std::string errorMsg = e.what();
            // ✅ معالجة خاصة لخطأ "هذا" غير معرّف
            std::cerr << "❌ خطأ أثناء تنفيذ المنشئ للصنف '" << cmd->class_name << "': " << errorMsg << std::endl;
            if (errorMsg.find("هذا") != std::string::npos) {
                throwError("خطأ في المنشئ: " + errorMsg + " (الكائن: " + tempName + ", الصنف: " + cmd->class_name + ")");
            } else {
                throw;
            }
        }
    } else {
        // std::cout << "ℹ️ تم إنشاء كائن '" << cmd->class_name << "' بدون منشئ." << std::endl;
    }
    
    // ✅ تخزين النتيجة النهائية في cmd->variable إذا كان محدداً
    if (!cmd->variable.empty() && cmd->variable != objVarName) {
        runtime->setVariable(cmd->variable, objVal);
    }
    
    return objVal;
}

Value ArabicExecutor::executeMethodCall(const std::shared_ptr<Command>& cmd) {
    
    Value objVal;
    const ClassDefinition* startClass = nullptr;

    // التحقق من "الأب" (الأب أو سوبر) بشكل يدوي لتجنب مشاكل الترميز
    bool isSuper = isSuperKeyword(cmd->object_name);
    if (!isSuper && cmd->condition && cmd->condition->type == ValueType::VARIABLE) {
        isSuper = isSuperKeyword(cmd->condition->value);
    }

    if (isSuper) {
        // 1. استدعاء من الصنف الأب (super / الأب)
        // ── استراتيجية متدرجة لإيجاد السياق ──────────────────
        // أ) هذا + __صنف_السياق__ (المسار الطبيعي لدوال الأعضاء)
        // ب) هذا + class_type  (متاح دائماً في CLASS_INSTANCE)
        // ج) fallback: بحث مباشر في تعريف الصنف المنشأ حالياً
        std::string parentClassName;
        try {
            objVal = runtime->getVariable("هذا");

            // محاولة أ: __صنف_السياق__
            if (runtime->hasVariable("__صنف_السياق__")) {
                Value contextVal = runtime->getVariable("__صنف_السياق__");
                const ClassDefinition* ctx = runtime->getClassInfo(contextVal.string_value);
                if (ctx && !ctx->parent_class.empty()) {
                    parentClassName = ctx->parent_class;
                }
            }

            // محاولة ب: class_type مباشرة على الكائن
            if (parentClassName.empty() && !objVal.class_type.empty()) {
                const ClassDefinition* ctx = runtime->getClassInfo(objVal.class_type);
                if (ctx && !ctx->parent_class.empty()) {
                    parentClassName = ctx->parent_class;
                }
            }

            if (parentClassName.empty()) {
                throwError("لا يمكن استخدام 'الأب' — الصنف الحالي ليس لديه أب معروف");
                return Value(ValueType::NONE);
            }

            startClass = runtime->getClassInfo(parentClassName);
            if (!startClass) {
                throwError("الصنف الأب '" + parentClassName + "' غير معرَّف");
                return Value(ValueType::NONE);
            }

        } catch (const std::exception& e) {
            throwError("لا يمكن استخدام 'الأب' خارج نطاق دوال الأصناف: " + std::string(e.what()));
            return Value(ValueType::NONE);
        } catch (...) {
            throwError("لا يمكن استخدام 'الأب' خارج نطاق دوال الأصناف");
            return Value(ValueType::NONE);
        }
    } else {
        // 2. استدعاء عادي
        try {
            if (cmd->condition) {
                objVal = evaluateExpression(*cmd->condition);
            } else if (!cmd->object_name.empty()) {
                if (!runtime->hasVariable(cmd->object_name)) {
                    if (isThisKeyword(cmd->object_name)) {
                        throwError("'هذا' غير متاح في هذا السياق.");
                    } else {
                        throwError("كائن غير معرّف: " + cmd->object_name);
                    }
                    return Value(ValueType::NONE);
                }
                objVal = runtime->getVariable(cmd->object_name);
            } else {
                throwError("لم يتم تحديد كائن لاستدعاء الدالة: " + cmd->method_name);
                return Value(ValueType::NONE);
            }
        } catch (const std::exception& e) {
            throwError("خطأ في تحديد الكائن: " + std::string(e.what()));
            return Value(ValueType::NONE);
        }

        // ✅ Handle native array methods (طول، أضف) before class method dispatch
        if (objVal.type == ValueType::ARRAY) {
            if (cmd->method_name == "طول") {
                return Value(ValueType::NUMBER, std::to_string(objVal.elements.size()));
            } else if (cmd->method_name == "أضف") {
                if (cmd->arguments.empty()) {
                    throwError("أضف يتطلب معاملاً واحداً على الأقل");
                    return Value(ValueType::NONE);
                }
                Value newElem = evaluateExpression(*cmd->arguments[0]);
                objVal.elements.push_back(newElem);
                // Update the original variable
                if (!cmd->object_name.empty()) {
                    runtime->setVariable(cmd->object_name, objVal);
                }
                return Value(ValueType::NONE);
            } else {
                throwError("طريقة غير معروفة للمصفوفة: " + cmd->method_name);
                return Value(ValueType::NONE);
            }
        }

        if (objVal.type != ValueType::OBJECT && objVal.type != ValueType::CLASS_INSTANCE) {
            throwError("المتغير ليس كائناً (النوع: " + valueTypeToString(objVal.type) + ")");
            return Value(ValueType::NONE);
        }
        
        if (objVal.string_value.empty()) {
            if (objVal.type == ValueType::OBJECT || objVal.type == ValueType::GENERIC_MAP) {
                throwError("لا يمكن استدعاء دالة عضو على كائن حرفي (Object Literal) بدون صنف.");
            } else {
                throwError("معرّف الكائن مفقود لاستدعاء الدالة: " + cmd->method_name);
            }
            return Value(ValueType::NONE);
        }
        
        auto tempInstance = runtime->getObjectInstance(objVal.string_value);
        if (!tempInstance) {
            throwError("كائن غير موجود في الذاكرة (ID: " + objVal.string_value + ") لاستدعاء: " + cmd->method_name);
            return Value(ValueType::NONE);
        }
        
        startClass = runtime->getClassInfo(tempInstance->class_type);
    }

    if (!startClass) {
        throwError("فشل العثور على صنف البداية للبحث عن الدالة");
        return Value(ValueType::NONE);
    }
    
    // DEBUG: Method lookup info
    
    
    for (const auto& m : startClass->methods) {
        
    }
    
    // الحصول على معلومات الكائن للرسائل الخطأ
    auto instance = runtime->getObjectInstance(objVal.string_value);
    if (!instance) {
        throwError("كائن غير موجود في الذاكرة: " + objVal.string_value);
        return Value(ValueType::NONE);
    }
    
    const ClassDefinition* currentClassInfo = startClass;
    const ClassMethod* methodPtr = nullptr;
    
    // ✅ استخدام جدول الدوال الافتراضية (Virtual Methods Table) لتعدد الأشكال
    if (!isSuper) {
        auto itV = currentClassInfo->virtual_methods.find(cmd->method_name);
        if (itV != currentClassInfo->virtual_methods.end()) {
            methodPtr = itV->second;
            // تحديث currentClassInfo ليكون الصنف الذي يحتوي فعلياً على الدالة (لأغراض السياق)
            // ملاحظة: في النسخة الحالية VMT يخزن مؤشرات للدوال، سنفترض أنها دوال صالحة.
        }
    }

    // إذا لم نجدها في VMT (أو كان استدعاء "الأب")، نستخدم البحث التقليدي
    if (!methodPtr) {
        while (currentClassInfo) {
            auto it = currentClassInfo->methods.find(cmd->method_name);
            
            // تحسين: إذا كان المطلوب "جديد" عبر "الأب"، نبحث أيضاً عن دالة باسم الصنف نفسه (منشئ الصنف الأب)
            if (it == currentClassInfo->methods.end() && isSuper && cmd->method_name == "جديد") {
                it = currentClassInfo->methods.find(currentClassInfo->class_name);
            }

            if (it != currentClassInfo->methods.end()) {
                methodPtr = &(it->second);
                break;
            }
            
            if (currentClassInfo->parent_class.empty()) {
                currentClassInfo = nullptr;
            } else {
                currentClassInfo = runtime->getClassInfo(currentClassInfo->parent_class);
            }
        }
    }
    
    // Debug Trace
    // if (!methodPtr) {
    //    std::cout << "❌ فشل العثور على الدالة '" << cmd->method_name << "' في الصنف '" << startClass->class_name << "' أو آبائه." << std::endl;
    //}

    if (!methodPtr || !currentClassInfo) {
        throwError("دالة عضو غير معرّفة: " + cmd->method_name + " في الصنف " + instance->class_type);
        return Value(ValueType::NONE);
    }
    
    const auto& method = *methodPtr;

    // ✅ منع استدعاء الدوال المجردة مباشرة
    if (method.is_abstract) {
        throwError("لا يمكن استدعاء دالة مجردة: " + cmd->method_name + " في الصنف " + currentClassInfo->class_name);
        return Value(ValueType::NONE);
    }

    // تعريف المعاملات
    for (size_t i = 0; i < method.parameters.size(); ++i) {
        // ✅ تقييم المعاملات في النطاق القديم (قبل دفع نطاق جديد)
        // هذا يتم لأننا قيمنا المعاملات ومررناها في الدالة executeMethodCall
        // ولكن executeMethodCall في الكود الحالي تقيم المعاملات داخل النطاق الجديد خطأ!
        // يجب أن نكون حذرين. في executeMethodCall الحالية (سطر 778):
        // Value argVal = i < cmd->arguments.size() ? evaluateExpression(cmd->arguments[i]) : Value(ValueType::NONE);
        // evaluateExpression تستخدم runtime->getVariable التي تبحث في النطاقات الحالية.
        // بما أننا دفعنا نطاقاً جديداً (سطر 769)، فإن البحث سيبدأ منه ثم يصعد.
        // المشكلة: "هذا.رقم_السطر_الحالي" موجود في الكائن (هذا)، و "هذا" تم تعريفه في النطاق الجديد (سطر 772).
        // لذا "هذا" الجديد يشير للكائن المستدعى عليه الدالة.
        // في حالة: مت ر = جديد رمز("...", "...", هذا.رقم_السطر_الحالي) داخل "المحلل_اللغوي"
        // الكائن المستدعى عليه هو "رمز" (الدالة هي المنشئ "جديد").
        // لذا "هذا" داخل المنشئ هو كائن الرمز الجديد.
        // بينما "هذا.رقم_السطر_الحالي" يجب أن يشير لـ "المحلل_اللغوي" (المستدعي).
        
        // الحل: تقييم المعاملات *قبل* دفع النطاق الجديد.
    }
    
    // ✅ 1. تقييم جميع المعاملات في السياق الحالي (المستدعي) قبل دفع نطاق جديد
    std::vector<Value> evaluatedArgs;
    for (const auto& argExpr : cmd->arguments) {
        evaluatedArgs.push_back(evaluateExpression(*argExpr));
    }

    // Capture context before pushing scope
    // std::string callerContext = "Global"; // TODO: Get actual context if possible
    
    runtime->pushScope();
    
    // ✅ التأكد من أن objVal صحيح قبل التعريف (CRITICAL FIX)
    if (objVal.type != ValueType::OBJECT && objVal.type != ValueType::CLASS_INSTANCE) {
         std::cerr << "❌ CRITICAL: Object became invalid before 'this' definition!" << std::endl;
         runtime->popScope();
         throwError("خطأ داخلي: فساد الكائن قبل استدعاء الدالة");
         return Value(ValueType::NONE);
    }

    // ✅ تعريف 'هذا' (this) - يجب أن يكون أول شيء في النطاق الجديد
    // استخدام defineVariable مباشرة للتأكد من أنه في النطاق الحالي الجديد
    runtime->defineVariable("هذا", objVal);

    // ✅ التحقق الفوري
    if (!runtime->hasVariable("هذا")) {
         std::cerr << "❌ CRITICAL: Failed to define 'this' in new scope!" << std::endl;
    }
    if (objVal.type != ValueType::OBJECT && objVal.type != ValueType::CLASS_INSTANCE) {
        throwError("خطأ: الكائن غير صالح لاستدعاء الدالة '" + cmd->method_name + "' (النوع: " + valueTypeToString(objVal.type) + ")");
        runtime->popScope();
        return Value(ValueType::NONE);
    }
    
    
    // تم التعريف أعلاه
    // runtime->defineVariable("هذا", objVal);
    
    // ✅ التحقق من أن "هذا" تم تعريفه بشكل صحيح (للتأكد من عدم وجود مشاكل في النطاق)
    if (!runtime->hasVariable("هذا")) {
        throwError("فشل في تعريف 'هذا' في النطاق الجديد (عدد النطاقات: " + std::to_string(runtime->scopeCount()) + ", الكائن: " + cmd->object_name + ", النوع: " + valueTypeToString(objVal.type) + ")");
        runtime->popScope();
        return Value(ValueType::NONE);
    }
    
    // ✅ التحقق من أن "هذا" يحتوي على القيمة الصحيحة
    Value verifyThis = runtime->getVariable("هذا");
    if (verifyThis.type != ValueType::OBJECT && verifyThis.type != ValueType::CLASS_INSTANCE) {
        throwError("'هذا' تم تعريفه لكن بقيمة غير صحيحة (النوع: " + valueTypeToString(verifyThis.type) + ")");
        runtime->popScope();
        return Value(ValueType::NONE);
    }
    
    // ✅ تعريف سياق الصنف الحالي لتسهيل عمل 'الأب'
    runtime->defineVariable("__صنف_السياق__", Value(ValueType::STRING, currentClassInfo->class_name));
    
    // تعريف المعاملات (استخدام القيم المقيمة مسبقاً)
    for (size_t i = 0; i < method.parameters.size(); ++i) {
        Value argVal = i < evaluatedArgs.size() ? evaluatedArgs[i] : Value(ValueType::NONE);
        runtime->defineVariable(method.parameters[i], argVal);
    }
    
    // Debug: Trace Inside Method
    // std::cout << "➡️ دخول الدالة: " << cmd->method_name << " (مع '" << method.parameters.size() << "' معاملات)" << std::endl;
    
    Value result = executeBlock(method.body);
    
    // ✅ التحقق من وجود قيمة إرجاع محددة (من ارجع)
    if (runtime->hasReturn()) {
        result = runtime->getReturnValue();
        runtime->clearReturn();
    }

    // ✅ إصلاح: تحديث متغير الكائن الأصلي بعد تنفيذ الدالة
    // (يضمن أن التغييرات في هذا.property تنعكس على المتغير الأصلي)
    if (!cmd->object_name.empty() && runtime->hasVariable(cmd->object_name)) {
        Value updatedThis = runtime->getVariable("هذا");
        if (updatedThis.type == ValueType::OBJECT || updatedThis.type == ValueType::CLASS_INSTANCE) {
            // تحديث الكائن في السجل إذا تغير
            if (!updatedThis.string_value.empty()) {
                auto inst = runtime->getObjectInstance(updatedThis.string_value);
                if (inst) {
                    // الكائن محدَّث في السجل تلقائياً عبر setObjectProperty
                    // نُحدِّث فقط المتغير المحلي للمستدعي
                    runtime->setVariable(cmd->object_name, updatedThis);
                }
            }
        }
    }

    runtime->popScope();

    // ✅ تخزين النتيجة في cmd->variable إذا كان محدداً (للاستخدام في الجسور)
    if (!cmd->variable.empty()) {
        runtime->setVariable(cmd->variable, result);
    }
    
    return result;
}

Value ArabicExecutor::executeImport(const std::shared_ptr<Command>& cmd) {
    // ─── المكتبات الداخلية المدمجة ───────────────────────────
    if (!cmd->library.empty()) {
        if (outputCallback) outputCallback("[DEBUG] library field: " + cmd->library + "\n");
        if (cmd->library == "الرؤية_الحاسوبية") {
            std::cerr << "[DEBUG] Loading vision library (library field)..." << std::endl;
            ArabicVisionBridge::registerFunctions(runtime, outputCallback);
            if (outputCallback) outputCallback("✅ تم تحميل مكتبة الرؤية الحاسوبية\n");
            return Value(ValueType::NONE);
        }
    }
    
    // Also check variable field as fallback
    if (!cmd->variable.empty()) {
        if (outputCallback) outputCallback("[DEBUG] variable field: " + cmd->variable + "\n");
        if (cmd->variable == "الرؤية_الحاسوبية") {
            std::cerr << "[DEBUG] Loading vision library (variable field)..." << std::endl;
            ArabicVisionBridge::registerFunctions(runtime, outputCallback);
            if (outputCallback) outputCallback("✅ تم تحميل مكتبة الرؤية الحاسوبية\n");
            return Value(ValueType::NONE);
        }
    }
    
    return Value(ValueType::NONE);
}

Value ArabicExecutor::executePropertyDef(const std::shared_ptr<Command>& cmd) {
    // الخصائص عادةً ما يتم التعامل معها في مرحلة التحليل (Metadata)
    // ولكن إذا تم تنفيذ هذا الأمر، فهذا يعني أنه قد يكون تعريفاً ديناميكياً
    
    try {
        Value thisVal = runtime->getVariable("هذا");
        if (thisVal.type == ValueType::OBJECT || thisVal.type == ValueType::CLASS_INSTANCE) {
            std::string propName = cmd->variable;
            Value val = evaluateExpression(cmd->value);
            
            if (thisVal.type == ValueType::OBJECT) {
                thisVal.map_elements[propName] = val;
                runtime->setVariable("هذا", thisVal);
            } else if (!thisVal.string_value.empty()) {
                
                runtime->setObjectProperty(thisVal.string_value, propName, val);
            }
            
            // std::cout << "📝 تم تعريف خاصية ديناميكية: " << propName << " = " << valueToString(val) << std::endl;
        } else {
            throwError("لا يمكن تعريف خاصية خارج سياق صنف (هذا).");
        }
    } catch (const std::exception& e) {
        throwError("خطأ في تعريف الخاصية: " + std::string(e.what()));
    } catch (...) {
        throwError("خطأ غير معروف أثناء تعريف الخاصية.");
    }
    
    return Value(ValueType::NONE);
}

Value ArabicExecutor::executeBreak(const std::shared_ptr<Command>&) {
    runtime->setBreak();
    return Value(ValueType::NONE);
}

Value ArabicExecutor::executeContinue(const std::shared_ptr<Command>&) {
    runtime->setContinue();
    return Value(ValueType::NONE);
}

Value ArabicExecutor::executeTry(const std::shared_ptr<Command>& cmd) {
    try {
        return executeBlock(cmd->body);
    } catch (const std::exception& e) {
        if (!cmd->else_body.empty()) {
            runtime->pushScope();
            // Store error message
            if (!cmd->variable.empty()) {
                runtime->defineVariable(cmd->variable, Value(ValueType::STRING, e.what()));
            }
            Value result = executeBlock(cmd->else_body);
            runtime->popScope();
            return result;
        }
        throw; // Re-throw
    } catch (...) {
        if (!cmd->else_body.empty()) {
            runtime->pushScope();
            if (!cmd->variable.empty()) {
                runtime->defineVariable(cmd->variable, Value(ValueType::STRING, "خطأ غير معروف"));
            }
            Value result = executeBlock(cmd->else_body);
            runtime->popScope();
            return result;
        }
        throw;
    }
}

Value ArabicExecutor::executeThrow(const std::shared_ptr<Command>& cmd) {
    Value val = evaluateExpression(cmd->value);
    throwError(valueToString(val));
    return Value(ValueType::NONE);
}

Value ArabicExecutor::evaluateExpression(const Value& expr) {
    // Catch-all for boolean keywords regardless of parsed type
    std::string rawVal = expr.string_value.empty() ? expr.value : expr.string_value;
    if (rawVal == "صحيح" || expr.value == "صحيح") return Value(ValueType::NUMBER, "1");
    if (rawVal == "خطأ" || expr.value == "خطأ") return Value(ValueType::NUMBER, "0");
    if (rawVal == "نعم" || expr.value == "نعم") return Value(ValueType::NUMBER, "1");
    if (rawVal == "لا" || expr.value == "لا") return Value(ValueType::NUMBER, "0");
    if (rawVal == "لاشيء" || rawVal == "لا_شيء" || expr.value == "لاشيء" || expr.value == "لا_شيء") return Value(ValueType::NONE);
    
    switch (expr.type) {
        case ValueType::NUMBER: return Value(ValueType::NUMBER, expr.string_value.empty() ? expr.value : expr.string_value);
        case ValueType::BOOLEAN: return Value(ValueType::BOOLEAN, expr.string_value.empty() ? expr.value : expr.string_value);
        case ValueType::STRING: {
            return Value(ValueType::STRING, expr.string_value.empty() ? expr.value : expr.string_value);
        }
        case ValueType::VARIABLE: {
            // تنظيف اسم المتغير
            std::string cleanName = trim(expr.value);
            
            // Check for boolean keywords first
            if (cleanName == "صحيح") {
                return Value(ValueType::NUMBER, "1");
            }
            if (cleanName == "خطأ") {
                return Value(ValueType::NUMBER, "0");
            }
            if (cleanName == "نعم") {
                return Value(ValueType::NUMBER, "1");
            }
            if (cleanName == "لا") {
                return Value(ValueType::NUMBER, "0");
            }
            if (cleanName == "لا_شيء" || cleanName == "لاشيء" || cleanName == "لا شىء" || cleanName == "لاشىء") {
                return Value(ValueType::NONE);
            }
            
            if (isThisKeyword(cleanName)) {
                 // ✅ إصلاح: إذا لم يكن "هذا" متاحاً (خارج سياق صنف)، نُرجع NONE بدلاً من رمي خطأ مميت
                 if (runtime->hasVariable("هذا")) {
                     return runtime->getVariable("هذا");
                 } else {
                     std::cerr << "⚠️ تحذير: 'هذا' غير متاح في هذا السياق (خارج دالة عضو). تم إرجاع قيمة فارغة." << std::endl;
                     return Value(ValueType::NONE);
                 }
            }

            // ✅ Check if name refers to a class definition (for ClassName.جديد())
            if (runtime->hasClass(cleanName)) {
                Value classVal(ValueType::CLASS_DEFINITION);
                classVal.string_value = cleanName;
                classVal.class_type = cleanName;
                return classVal;
            }

            return runtime->getVariable(cleanName);
        }
        case ValueType::ARRAY: return expr;
        case ValueType::ARRAY_LITERAL: {
            Value result(ValueType::ARRAY);
            for (const auto& el : expr.elements) {
                result.elements.push_back(evaluateExpression(el));
            }
            return result;
        }
        case ValueType::OBJECT: return expr;
        case ValueType::OBJECT_LITERAL: {
            Value result(ValueType::OBJECT);
            for (const auto& pair : expr.map_elements) {
                result.map_elements[pair.first] = evaluateExpression(pair.second);
            }
            return result;
        }
        case ValueType::CLASS_INSTANCE: return expr;
        case ValueType::GENERIC_LIST: return expr;
        case ValueType::GENERIC_MAP: return expr;
        case ValueType::SLICE: return expr;
        case ValueType::ARRAY_ACCESS: return evaluateArrayAccess(expr);
        case ValueType::PROPERTY_ACCESS: return evaluatePropertyAccess(expr);
        case ValueType::FUNCTION_CALL: return evaluateFunctionCall(expr);
        case ValueType::OPERATION: return evaluateOperation(expr);
        case ValueType::COMPARISON: return evaluateCondition(expr);
        default: return Value(ValueType::NONE);
    }
}

Value ArabicExecutor::evaluateCondition(const Value& cond) {
    char dbg[512];
    if (cond.type == ValueType::NONE) return Value(ValueType::NUMBER, "0");
    if (cond.type == ValueType::BOOLEAN) {
        std::string s = cond.string_value.empty() ? cond.value : cond.string_value;
        return Value(ValueType::NUMBER, (s == "1" || s == "true") ? "1" : "0");
    }
    if (cond.type == ValueType::NUMBER) {
        return Value(ValueType::NUMBER, cond.string_value.empty() ? cond.value : cond.string_value);
    }
    if (cond.type == ValueType::STRING) {
        std::string s = cond.string_value.empty() ? cond.value : cond.string_value;
        return Value(ValueType::NUMBER, (!s.empty() && s != "0") ? "1" : "0");
    }
    if (cond.type == ValueType::VARIABLE) {
        std::string v = cond.value;
        if (v == "صحيح" || v == "نعم") return Value(ValueType::NUMBER, "1");
        if (v == "خطأ" || v == "لا") return Value(ValueType::NUMBER, "0");
        try {
            Value resolved = runtime->getVariable(v);
            return Value(ValueType::NUMBER, isTruthy(resolved) ? "1" : "0");
        } catch (...) {
            return Value(ValueType::NUMBER, "0");
        }
    }
    if (cond.type == ValueType::OPERATION) {
        if (cond.operation == "و") {
            if (!cond.left || !cond.right) return Value(ValueType::NUMBER, "0");
            bool l = isTruthy(evaluateCondition(*cond.left));
            if (!l) return Value(ValueType::NUMBER, "0");
            bool r = isTruthy(evaluateCondition(*cond.right));
            return Value(ValueType::NUMBER, r ? "1" : "0");
        }
        if (cond.operation == "أو") {
            if (!cond.left || !cond.right) return Value(ValueType::NUMBER, "0");
            bool l = isTruthy(evaluateCondition(*cond.left));
            if (l) return Value(ValueType::NUMBER, "1");
            bool r = isTruthy(evaluateCondition(*cond.right));
            return Value(ValueType::NUMBER, r ? "1" : "0");
        }
        if (cond.operation == "ليس") {
            if (!cond.left) return Value(ValueType::NUMBER, "0");
            bool v = isTruthy(evaluateCondition(*cond.left));
            return Value(ValueType::NUMBER, (!v) ? "1" : "0");
        }
        // Handle comparison operators that were parsed as OPERATION
        if (cond.operation == ">" || cond.operation == "<" || cond.operation == ">=" || cond.operation == "<=" || cond.operation == "==" || cond.operation == "!=") {
            if (!cond.left || !cond.right) return Value(ValueType::NUMBER, "0");
            Value L = evaluateExpression(*cond.left);
            Value R = evaluateExpression(*cond.right);
            double a = valueToNumber(L);
            double b = valueToNumber(R);
            bool res = false;
            if (cond.operation == ">") res = (a > b);
            else if (cond.operation == "<") res = (a < b);
            else if (cond.operation == ">=") res = (a >= b);
            else if (cond.operation == "<=") res = (a <= b);
            else if (cond.operation == "==") res = (a == b);
            else if (cond.operation == "!=") res = (a != b);
            return Value(ValueType::NUMBER, res ? "1" : "0");
        }
        Value res = evaluateExpression(cond);
        return Value(ValueType::NUMBER, isTruthy(res) ? "1" : "0");
    }
    if (cond.type == ValueType::COMPARISON) {
        if (!cond.left || !cond.right) return Value(ValueType::NUMBER, "0");
        Value L, R;
        try { L = evaluateExpression(*cond.left); } catch (...) { L = Value(ValueType::NUMBER, "0"); }
        try { R = evaluateExpression(*cond.right); } catch (...) { R = Value(ValueType::NUMBER, "0"); }
        
        std::string op = cond.operator_.empty() ? cond.operation : cond.operator_;
        
        // ✅ Special handling for equality/inequality (supports NONE and other types)
        if (op == "==" || op == "!=") {
            bool isEqual = false;
            if (L.type == R.type) {
                if (L.type == ValueType::NUMBER) isEqual = (valueToNumber(L) == valueToNumber(R));
                else if (L.type == ValueType::STRING) isEqual = (L.string_value == R.string_value);
                else if (L.type == ValueType::NONE) isEqual = true;
                else if (L.type == ValueType::ARRAY) isEqual = (L.elements.size() == R.elements.size());
                else isEqual = (L.value == R.value);
            } else {
                // Different types - generally not equal
                isEqual = false;
            }
            bool res = (op == "==") ? isEqual : !isEqual;
            return Value(ValueType::NUMBER, res ? "1" : "0");
        }

        double a = valueToNumber(L);
        double b = valueToNumber(R);
        bool res = false;
        if (op == ">") res = (a > b);
        else if (op == ">=") res = (a >= b);
        else if (op == "<") res = (a < b);
        else if (op == "<=") res = (a <= b);
        else res = isTruthy(L);
        return Value(ValueType::NUMBER, res ? "1" : "0");
    }
    Value v = evaluateExpression(cond);
    return Value(ValueType::NUMBER, isTruthy(v) ? "1" : "0");
}

Value ArabicExecutor::evaluateOperation(const Value& expr) {
    std::string op = expr.operation.empty() ? expr.operator_ : expr.operation;

    // ✅ Handle unary operations
    if (op.find("unary") == 0) {
        std::string actualOp = op.substr(5); // Get "-", "+", or "!"
        if (!expr.right) return Value(ValueType::NONE);
        Value R = evaluateExpression(*expr.right);
        return performUnaryOperation(actualOp, R);
    }

    if (!expr.left || !expr.right) return Value(ValueType::NONE);

    Value L = evaluateExpression(*expr.left);
    Value R = evaluateExpression(*expr.right);
    return performOperation(op, L, R);
}

Value ArabicExecutor::evaluateFunctionCall(const Value& expr) {
    if (expr.is_new_object) {
        auto cmd = std::make_shared<Command>(CommandType::CREATE_OBJECT);
        cmd->class_name = expr.function_name;
        cmd->arguments = expr.arguments;
        cmd->object_name = ""; // No name for anonymous creation in expression
        return executeObjectCreation(cmd);
    }

    // إذا كان استدعاء دالة عضو (method call)
    if (!expr.object_name.empty() || expr.left) {
        std::shared_ptr<Command> call = std::make_shared<Command>(CommandType::METHOD_CALL);
        call->object_name = expr.object_name;
        call->method_name = expr.function_name.empty() ? expr.value : expr.function_name;
        call->arguments = expr.arguments;

        // إذا كان هناك تعبير جهة اليسار، نحتاج لتقييمه أولاً
        if (expr.left) {
            Value objVal = evaluateExpression(*expr.left);
            // ✅ إصلاح: إذا كان الكائن NONE (مثل هذا خارج سياق الصنف)، نتجاهل الاستدعاء بأمان
            if (objVal.type == ValueType::NONE) {
                std::cerr << "⚠️ تحذير: استدعاء دالة على قيمة فارغة، تم تجاهله: " << call->method_name << std::endl;
                return Value(ValueType::NONE);
            }
            if (objVal.type == ValueType::OBJECT || objVal.type == ValueType::CLASS_INSTANCE) {
                // نستخدم القيمة المقيمة للكائن لتجنب مشاكل البحث عن المتغيرات (خاصة المعرفات obj_X)
                call->object_name = objVal.string_value;
                call->condition = std::make_shared<Value>(objVal);
            } else if (objVal.type == ValueType::ARRAY) {
                // ✅ دعم طرق المصفوفات (طول، أضف)
                std::string methodName = expr.function_name.empty() ? expr.value : expr.function_name;
                if (methodName == "طول") {
                    return Value(ValueType::NUMBER, std::to_string(objVal.elements.size()));
                } else if (methodName == "أضف") {
                    // إضافة عنصر إلى المصفوفة
                    if (expr.arguments.empty()) {
                        throwError("أضف يتطلب معاملاً واحداً على الأقل");
                        return Value(ValueType::NONE);
                    }
                    Value newElem = evaluateExpression(*expr.arguments[0]);
                    objVal.elements.push_back(newElem);
                    // تحديث المتغير الأصلي
                    if (!expr.object_name.empty()) {
                        runtime->setVariable(expr.object_name, objVal);
                    }
                    return Value(ValueType::NONE);
                } else {
                    throwError("طريقة غير معروفة للمصفوفة: " + methodName);
                    return Value(ValueType::NONE);
                }
            } else if (objVal.type == ValueType::CLASS_DEFINITION) {
                // ✅ دعم إنشاء كائن باستخدام صنف.جديد()
                std::string methodName = expr.function_name.empty() ? expr.value : expr.function_name;
                if (methodName == "جديد") {
                    auto createCmd = std::make_shared<Command>(CommandType::CREATE_OBJECT);
                    createCmd->class_name = objVal.string_value;
                    createCmd->arguments = expr.arguments;
                    return executeObjectCreation(createCmd);
                }
                throwError("لا يمكن استدعاء دالة '" + methodName + "' على تعريف صنف (فقط 'جديد مسموح)");
                return Value(ValueType::NONE);
            } else {
                throwError("الطرف الأيسر للاستدعاء ليس كائناً (نوعه: " + valueTypeToString(objVal.type) + ")");
                return Value(ValueType::NONE);
            }
        }

        return executeMethodCall(call);
    }

    std::shared_ptr<Command> call = std::make_shared<Command>(CommandType::FUNCTION_CALL);
    call->function_name = expr.function_name.empty() ? expr.value : expr.function_name;
    call->arguments = expr.arguments;
    return executeFunctionCall(call);
}

Value ArabicExecutor::evaluateArrayAccess(const Value& expr) {
    if (!expr.left || !expr.right) return Value(ValueType::NONE);
    Value arr = evaluateExpression(*expr.left);
    Value idxVal = evaluateExpression(*expr.right);
    
    // Handle Slicing
    if (idxVal.type == ValueType::SLICE) {
        int start = 0;
        int end = -1;

        if (idxVal.left && idxVal.left->type != ValueType::NONE) {
            start = static_cast<int>(valueToNumber(evaluateExpression(*idxVal.left)));
        }
        if (idxVal.right && idxVal.right->type != ValueType::NONE) {
            end = static_cast<int>(valueToNumber(evaluateExpression(*idxVal.right)));
        }

        if (arr.type == ValueType::ARRAY) {
            if (end == -1) end = static_cast<int>(arr.elements.size());
            if (start < 0) start = 0;
            if (end > static_cast<int>(arr.elements.size())) end = static_cast<int>(arr.elements.size());
            if (start > end) return Value(ValueType::ARRAY);

            Value result(ValueType::ARRAY);
            for (int i = start; i < end; ++i) {
                result.elements.push_back(arr.elements[i]);
            }
            return result;
        } else if (arr.type == ValueType::STRING) {
            ArabicTextUtils utils;
            size_t totalLen = utils.utf8Length(arr.string_value);
            
            if (end == -1) end = static_cast<int>(totalLen);
            if (start < 0) start = 0;
            if (end > static_cast<int>(totalLen)) end = static_cast<int>(totalLen);
            if (start > end) return Value(ValueType::STRING, "");

            return Value(ValueType::STRING, utils.utf8Substring(arr.string_value, start, end - start));
        }
        return Value(ValueType::NONE);
    }

    // Handle normal Array access
    if (arr.type == ValueType::ARRAY) {
        int idx = static_cast<int>(valueToNumber(idxVal));
        if (idx < 0 || static_cast<size_t>(idx) >= arr.elements.size()) return Value(ValueType::NONE);
        return arr.elements[idx];
    }
    
    // Handle String access
    if (arr.type == ValueType::STRING) {
        ArabicTextUtils utils;
        int idx = static_cast<int>(valueToNumber(idxVal));
        size_t totalLen = utils.utf8Length(arr.string_value);
        
        if (idx < 0 || static_cast<size_t>(idx) >= totalLen) return Value(ValueType::NONE);
        return Value(ValueType::STRING, utils.utf8CharAt(arr.string_value, idx));
    }
    
    // Handle Dictionary access
    if (arr.type == ValueType::GENERIC_MAP || arr.type == ValueType::OBJECT) {
        std::string key;
        if (idxVal.type == ValueType::STRING) key = idxVal.string_value;
        else if (idxVal.type == ValueType::NUMBER) key = valueToString(idxVal);
        else return Value(ValueType::NONE); // Invalid key type

        if (arr.map_elements.find(key) != arr.map_elements.end()) {
            return arr.map_elements.at(key);
        }
        return Value(ValueType::NONE); // Key not found
    }

    return Value(ValueType::NONE);
}

Value ArabicExecutor::evaluatePropertyAccess(const Value& expr) {
    Value objVal;
    if (expr.left) {
        objVal = evaluateExpression(*expr.left);
    } else {
        try {
            if (isSuperKeyword(expr.object_name)) {
                objVal = runtime->getVariable("هذا");
            } else if (isThisKeyword(expr.object_name)) {
                objVal = runtime->getVariable("هذا");
            } else {
                objVal = runtime->getVariable(expr.object_name);
            }
        } catch (...) {
            throwError("كائن غير معرّف: " + expr.object_name);
            return Value(ValueType::NONE);
        }
    }
    
    // 

    // ✅ دعم الوصول لخصائص القواميس والكائنات الحرفية (Literals)
    if (objVal.type == ValueType::OBJECT || objVal.type == ValueType::GENERIC_MAP) {
        auto it = objVal.map_elements.find(expr.property_name);
        if (it != objVal.map_elements.end()) {
            return it->second;
        }
    }
    
    // ✅ دعم الكائنات المسجلة (Class Instances)
    if ((objVal.type == ValueType::OBJECT || objVal.type == ValueType::CLASS_INSTANCE) && !objVal.string_value.empty()) {
        try {
            // 
            return runtime->getObjectProperty(objVal.string_value, expr.property_name);
        } catch (...) {
            // تجاهل الخطأ والمحاولة في مكان آخر أو الإرجاع لا شيء
            // 
        }
    }
    
    if (objVal.type != ValueType::OBJECT && objVal.type != ValueType::CLASS_INSTANCE && objVal.type != ValueType::GENERIC_MAP) {
        // بدلاً من رمي خطأ، نعيد لا_شيء للسماح بالتحقق الديناميكي من الأنواع
        // throwError("فقط الكائنات تملك خصائص. (النوع الحالي: " + valueTypeToString(objVal.type) + ")");
        return Value(ValueType::NONE);
    }
    
    return Value(ValueType::NONE);
}

bool ArabicExecutor::isTruthy(const Value& value) {
    if (value.type == ValueType::NUMBER) {
        return valueToNumber(value) != 0.0;
    }
    if (value.type == ValueType::STRING) {
        return !value.string_value.empty();
    }
    if (value.type == ValueType::ARRAY) {
        return !value.elements.empty();
    }
    return value.type != ValueType::NONE;
}

std::string ArabicExecutor::valueToString(const Value& value) {
    if (value.type == ValueType::STRING) return value.string_value;
    if (value.type == ValueType::NUMBER) {
        std::string s = value.string_value.empty() ? value.value : value.string_value;
        // إزالة الأصفار الزائدة إذا كان رقماً عشرياً
        if (s.find('.') != std::string::npos) {
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            if (s.back() == '.') s.pop_back();
        }
        return s;
    }
    if (value.type == ValueType::ARRAY) {
        std::string result = "[";
        for (size_t i = 0; i < value.elements.size(); ++i) {
            result += valueToString(value.elements[i]);
            if (i < value.elements.size() - 1) result += ", ";
        }
        result += "]";
        return result;
    }
    if (value.type == ValueType::GENERIC_MAP) {
        std::string result = "{";
        size_t count = 0;
        for (const auto& pair : value.map_elements) {
            result += pair.first + ": " + valueToString(pair.second);
            if (count < value.map_elements.size() - 1) result += ", ";
            count++;
        }
        result += "}";
        return result;
    }
    if (value.type == ValueType::OBJECT || value.type == ValueType::CLASS_INSTANCE) {
        return value.class_type + "(" + value.string_value + ")";
    }
    if (value.type == ValueType::BOOLEAN) {
        std::string s = value.string_value.empty() ? value.value : value.string_value;
        return (s == "1" || s == "true") ? "صحيح" : "خطأ";
    }
    if (value.type == ValueType::NONE) return "لا شيء";
    return "";
}

Value ArabicExecutor::stringToValue(const std::string& str) {
    return Value(ValueType::STRING, str);
}

double ArabicExecutor::valueToNumber(const Value& value) {
    std::string s;
    if (value.type == ValueType::NUMBER) {
        s = value.string_value.empty() ? value.value : value.string_value;
    }
    else if (value.type == ValueType::STRING) {
        s = value.string_value;
    }
    else if (value.type == ValueType::VARIABLE) {
        try {
            Value v = runtime->getVariable(value.value);
            return valueToNumber(v);
        } catch (...) { return 0.0; }
    }
    else {
        return 0.0;
    }
    
    // التحقق من صحة السطر قبل التحويل
    if (s.empty()) return 0.0;
    
    try {
        return std::stod(s);
    } catch (...) {
        return 0.0;
    }
}

Value ArabicExecutor::multiply(const Value& left, const Value& right) {
    if (left.type == ValueType::STRING && right.type == ValueType::NUMBER) {
        std::string res = "";
        int count = static_cast<int>(valueToNumber(right));
        for (int i = 0; i < count; ++i) res += left.string_value;
        return Value(ValueType::STRING, res);
    }
    if (left.type == ValueType::NUMBER && right.type == ValueType::STRING) {
        std::string res = "";
        int count = static_cast<int>(valueToNumber(left));
        for (int i = 0; i < count; ++i) res += right.string_value;
        return Value(ValueType::STRING, res);
    }
    double a = valueToNumber(left);
    double b = valueToNumber(right);
    return Value(ValueType::NUMBER, std::to_string(a * b));
}

Value ArabicExecutor::performOperation(const std::string& op, const Value& left, const Value& right) {
    // دعم دمج النصوص إذا كان أحد الطرفين نصاً
    if (op == "+" && (left.type == ValueType::STRING || right.type == ValueType::STRING)) {
        return Value(ValueType::STRING, valueToString(left) + valueToString(right));
    }

    double a = valueToNumber(left);
    double b = valueToNumber(right);
    
    auto doubleToCleanString = [](double d) {
        std::string s = std::to_string(d);
        if (s.find('.') != std::string::npos) {
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            if (s.back() == '.') s.pop_back();
        }
        return s;
    };
    
    if (op == "+") {
        return Value(ValueType::NUMBER, doubleToCleanString(a + b));
    }
    if (op == "-") {
        return Value(ValueType::NUMBER, doubleToCleanString(a - b));
    }
    if (op == "*") {
        return multiply(left, right);
    }
    if (op == "/") {
        if (b == 0) return Value(ValueType::NUMBER, "0");
        return Value(ValueType::NUMBER, doubleToCleanString(a / b));
    }
    if (op == "%" || op == "٪") {
        long long ai = (long long)a; long long bi = (long long)b;
        return Value(ValueType::NUMBER, std::to_string(bi == 0 ? 0 : (ai % bi)));
    }
    if (op == "&") return Value(ValueType::NUMBER, std::to_string(((long long)a) & ((long long)b)));
    if (op == "|") return Value(ValueType::NUMBER, std::to_string(((long long)a) | ((long long)b)));
    if (op == "^") return Value(ValueType::NUMBER, std::to_string(((long long)a) ^ ((long long)b)));
    return Value(ValueType::NONE);
}

Value ArabicExecutor::performUnaryOperation(const std::string& op, const Value& value) {
    if (op == "!") {
        return Value(ValueType::NUMBER, isTruthy(value) ? "0" : "1");
    }
    double a = valueToNumber(value);
    if (op == "+") return Value(ValueType::NUMBER, std::to_string(a));
    if (op == "-") return Value(ValueType::NUMBER, std::to_string(-a));
    return Value(ValueType::NONE);
}

void ArabicExecutor::throwError(const std::string& message) const {
    executionErrors++;
    std::string fullMsg = message;
    
    // We throw a C++ exception so it can be caught by 'executeTry'
    throw std::runtime_error(fullMsg);
}

size_t ArabicExecutor::getGenericListSize(const Value& array) const {
    if (array.type == ValueType::ARRAY || array.type == ValueType::GENERIC_LIST) {
        return array.elements.size();
    }
    return 0;
}

void ArabicExecutor::printError(const std::string& message) {
    std::cerr << message << std::endl;
}

void ArabicExecutor::printWarning(const std::string& message) {
    std::cerr << message << std::endl;
}

void ArabicExecutor::setVerbose(bool) {}
void ArabicExecutor::logDebug(const std::string&) {}
void ArabicExecutor::importLibrary(const std::string&) {}

// ────────────────────────────────────────────────────────
// تطبيق دوال JIT
// ────────────────────────────────────────────────────────

void ArabicExecutor::recordFunctionCall(const std::string& functionName, std::chrono::nanoseconds executionTime) {
    std::lock_guard<std::mutex> lock(functionStatsMutex);
    auto& info = functionCallStats[functionName];
    info.callCount++;
    info.totalTime += executionTime;
    info.lastCall = std::chrono::steady_clock::now();
}

std::vector<std::string> ArabicExecutor::getHotFunctions(size_t minCalls) const {
    std::lock_guard<std::mutex> lock(functionStatsMutex);
    std::vector<std::pair<std::string, size_t>> functions;

    for (const auto& pair : functionCallStats) {
        if (pair.second.callCount >= minCalls) {
            functions.emplace_back(pair.first, pair.second.callCount);
        }
    }

    // ترتيب حسب عدد الاستدعاءات (الأكثر أولاً)
    std::sort(functions.begin(), functions.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<std::string> result;
    for (const auto& func : functions) {
        result.push_back(func.first);
    }

    return result;
}

bool ArabicExecutor::tryCompileFunction(const std::string& functionName) {
    return false;
}

std::vector<std::string> ArabicExecutor::getJITStats() const {
    return {"JIT disabled (SimpleJIT missing)"};
}

void ArabicExecutor::optimizeHotFunctions() {
    return;
}

    // ────────────────────────────────────────────────────────
    // تطبيق دوال فحص الأنواع في وقت التشغيل
    // ────────────────────────────────────────────────────────

    bool ArabicExecutor::checkTypeCompatibility(const Value& value, ValueType expectedType) const {
        if (value.type == expectedType) return true;

        // قواعد التوافق المحسنة
        switch (expectedType) {
            case ValueType::NUMBER:
                return value.type == ValueType::STRING; // يمكن تحويل النص إلى رقم

            case ValueType::STRING:
                return value.type == ValueType::NUMBER; // يمكن تحويل الرقم إلى نص

            case ValueType::ARRAY:
                return value.type == ValueType::GENERIC_LIST; // القائمة العامة متوافقة

            case ValueType::GENERIC_LIST:
                return value.type == ValueType::ARRAY; // المصفوفة متوافقة

            default:
                return false;
        }
    }

    Value ArabicExecutor::performAutomaticConversion(const Value& value, ValueType targetType) {
        if (value.type == targetType) return value;

        Value result = value;
        result.type = targetType;

        switch (targetType) {
            case ValueType::NUMBER:
                if (value.type == ValueType::STRING) {
                    // محاولة تحويل النص إلى رقم
                    try {
                        double num = std::stod(value.string_value);
                        result.value = std::to_string(num);
                        result.string_value = result.value;
                    } catch (...) {
                        throwError("لا يمكن تحويل النص إلى رقم: " + value.string_value);
                        return Value(ValueType::NONE);
                    }
                }
                break;

            case ValueType::STRING:
                if (value.type == ValueType::NUMBER) {
                    result.string_value = value.value;
                    result.value = result.string_value;
                }
                break;

            case ValueType::ARRAY:
                if (value.type == ValueType::GENERIC_LIST) {
                    result.elements = value.elements;
                }
                break;

            case ValueType::GENERIC_LIST:
                if (value.type == ValueType::ARRAY) {
                    result.elements = value.elements;
                    result.generic_type = ValueType::VARIABLE; // نوع عام
                }
                break;

            default:
                throwError("تحويل غير مدعوم من " + valueTypeToString(value.type) +
                          " إلى " + valueTypeToString(targetType));
                return Value(ValueType::NONE);
        }

        return result;
    }

    bool ArabicExecutor::validateOperation(const Value& left, const std::string& op, const Value& right) const {
        // فحص صحة العملية بناءً على الأنواع

        // العمليات الحسابية
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            // يمكن للأرقام والنصوص (للجمع)
            if (op == "+" && (left.type == ValueType::STRING || right.type == ValueType::STRING)) {
                return true; // جمع النصوص مسموح
            }
            return (left.type == ValueType::NUMBER || checkTypeCompatibility(left, ValueType::NUMBER)) &&
                   (right.type == ValueType::NUMBER || checkTypeCompatibility(right, ValueType::NUMBER));
        }

        // المقارنات
        if (op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=") {
            return true; // المقارنات مسموحة بين أي أنواع
        }

        // العمليات المنطقية
        if (op == "&&" || op == "||") {
            return true; // العمليات المنطقية مسموحة
        }

        return true; // العملية صحيحة افتراضياً
    }

    bool ArabicExecutor::validateArrayAccess(const Value& array, int index) const {
        if (array.type != ValueType::ARRAY && array.type != ValueType::GENERIC_LIST) {
            throwError("المتغير ليس مصفوفة أو قائمة");
            return false;
        }

        if (index < 0) {
            throwError("فهرس المصفوفة لا يمكن أن يكون سالباً: " + std::to_string(index));
            return false;
        }

        size_t size = (array.type == ValueType::ARRAY) ? array.elements.size() :
                     getGenericListSize(array);

        if (static_cast<size_t>(index) >= size) {
            throwError("فهرس المصفوفة خارج النطاق: " + std::to_string(index) +
                      " (الحجم: " + std::to_string(size) + ")");
            return false;
        }

        return true;
    }

    bool ArabicExecutor::isThisKeyword(const std::string& name) {
        // "هذا" bytes in UTF-8: D9 87 D8 B0 D8 A7 (6 bytes)
        return (name.length() == 6 && 
               ((unsigned char)name[0] == 0xd9 && (unsigned char)name[1] == 0x87 &&
                (unsigned char)name[2] == 0xd8 && (unsigned char)name[3] == 0xb0 &&
                (unsigned char)name[4] == 0xd8 && (unsigned char)name[5] == 0xa7)) ||
               (name == "هذا"); // Fallback for standard comparison
    }

    bool ArabicExecutor::isSuperKeyword(const std::string& name) {
        // "الأب" bytes in UTF-8: D8 A7 D9 84 D8 A3 D8 A8 (8 bytes)
        // "سوبر" bytes in UTF-8: D8 B3 D9 88 D8 A8 D8 B1 (8 bytes)
        return (name.length() == 8 && 
               (((unsigned char)name[0] == 0xd8 && (unsigned char)name[1] == 0xa7 &&
                 (unsigned char)name[2] == 0xd9 && (unsigned char)name[3] == 0x84 &&
                 (unsigned char)name[4] == 0xd8 && (unsigned char)name[5] == 0xa3 &&
                 (unsigned char)name[6] == 0xd8 && (unsigned char)name[7] == 0xa8) ||
                ((unsigned char)name[0] == 0xd8 && (unsigned char)name[1] == 0xb3 &&
                 (unsigned char)name[2] == 0xd9 && (unsigned char)name[3] == 0x88 &&
                 (unsigned char)name[4] == 0xd8 && (unsigned char)name[5] == 0xa8 &&
                 (unsigned char)name[6] == 0xd8 && (unsigned char)name[7] == 0xb1))) ||
               (name == "الأب" || name == "سوبر"); // Fallback
    }

}
