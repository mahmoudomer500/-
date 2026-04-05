// ArabicUtilities.h - الأدوات المساعدة الموحدة للمترجم العربي
// يجمع: ErrorHandler + CodeOptimizer + ArabicDebugger
// ✅ النسخة المدمجة الكاملة - بدون تحذيرات

#ifndef ARABIC_UTILITIES_H
#define ARABIC_UTILITIES_H

#include "ArabicTypes.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace ArabicLanguage {

    // ════════════════════════════════════════════════════════════════
    // 🚨 PART 1: ERROR HANDLER - معالج الأخطاء
    // ════════════════════════════════════════════════════════════════

    enum class ErrorLevel {
        INFO,       // معلومات
        WARNING,    // تحذير
        ERROR,      // خطأ
        FATAL       // خطأ فادح
    };

    enum class ErrorCode {
        UNDEFINED_VARIABLE,
        INVALID_SYNTAX,
        TYPE_MISMATCH,
        DIVISION_BY_ZERO,
        UNDEFINED_FUNCTION,
        WRONG_ARGUMENT_COUNT,
        FILE_NOT_FOUND,
        ASSEMBLY_FAILED,
        LINKING_FAILED,
        UNKNOWN_OPERATOR,
        INVALID_ARRAY_ACCESS,
        STACK_OVERFLOW
    };

    struct ErrorInfo {
        ErrorLevel level;
        ErrorCode code;
        std::string message;
        std::string context;
        int line_number;
        std::string filename;

        ErrorInfo(ErrorLevel lvl, ErrorCode c, const std::string& msg,
            int line = -1, const std::string& ctx = "",
            const std::string& file = "")
            : level(lvl), code(c), message(msg), context(ctx),
            line_number(line), filename(file) {
        }
    };

    class ErrorHandler {
    public:
        static ErrorHandler& getInstance() {
            static ErrorHandler instance;
            return instance;
        }

        void reportError(ErrorLevel level, ErrorCode code,
            const std::string& message,
            int line = -1,
            const std::string& context = "",
            const std::string& filename = "") {
            ErrorInfo error(level, code, message, line, context, filename);
            errors.push_back(error);

            if (level == ErrorLevel::ERROR || level == ErrorLevel::FATAL) {
                errorCount++;
            }
            else if (level == ErrorLevel::WARNING) {
                warningCount++;
            }

            printError(error);

            if (level == ErrorLevel::FATAL) {
                std::cerr << "\n💀 خطأ فادح: لا يمكن الاستمرار في الترجمة" << std::endl;
                hasErrors = true;
            }
        }

        void reportInfo(const std::string& message) {
            std::cout << "ℹ️  " << message << std::endl;
        }

        void reportWarning(ErrorCode code, const std::string& message, int line = -1) {
            reportError(ErrorLevel::WARNING, code, message, line);
        }

        void reportError(ErrorCode code, const std::string& message, int line = -1) {
            reportError(ErrorLevel::ERROR, code, message, line);
            hasErrors = true;
        }

        bool hasAnyErrors() const { return hasErrors; }
        int getErrorCount() const { return errorCount; }
        int getWarningCount() const { return warningCount; }

        void printSummary() const {
            std::cout << "\n📊 ملخص التحليل:" << std::endl;
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;

            if (errorCount == 0 && warningCount == 0) {
                std::cout << "✅ لا توجد أخطاء أو تحذيرات" << std::endl;
            }
            else {
                if (errorCount > 0) {
                    std::cout << "❌ الأخطاء: " << errorCount << std::endl;
                }
                if (warningCount > 0) {
                    std::cout << "⚠️  التحذيرات: " << warningCount << std::endl;
                }
            }
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        }

        void clear() {
            errors.clear();
            errorCount = 0;
            warningCount = 0;
            hasErrors = false;
        }

        const std::vector<ErrorInfo>& getErrors() const {
            return errors;
        }

    private:
        ErrorHandler() : errorCount(0), warningCount(0), hasErrors(false) {}
        ErrorHandler(const ErrorHandler&) = delete;
        ErrorHandler& operator=(const ErrorHandler&) = delete;

        void printError(const ErrorInfo& error) const {
            std::string icon;
            std::string levelStr;

            switch (error.level) {
            case ErrorLevel::INFO:
                icon = "ℹ️ ";
                levelStr = "معلومة";
                break;
            case ErrorLevel::WARNING:
                icon = "⚠️ ";
                levelStr = "تحذير";
                break;
            case ErrorLevel::ERROR:
                icon = "❌";
                levelStr = "خطأ";
                break;
            case ErrorLevel::FATAL:
                icon = "💀";
                levelStr = "خطأ فادح";
                break;
            }

            std::cerr << icon << " " << levelStr;

            if (error.line_number >= 0) {
                std::cerr << " [السطر " << error.line_number << "]";
            }

            if (!error.filename.empty()) {
                std::cerr << " [" << error.filename << "]";
            }

            std::cerr << ": " << error.message << std::endl;

            if (!error.context.empty()) {
                std::cerr << "    السياق: " << error.context << std::endl;
            }

            std::string suggestion = getSuggestion(error.code);
            if (!suggestion.empty()) {
                std::cerr << "    💡 اقتراح: " << suggestion << std::endl;
            }
        }

        std::string getSuggestion(ErrorCode code) const {
            switch (code) {
            case ErrorCode::UNDEFINED_VARIABLE:
                return "تأكد من تعريف المتغير قبل استخدامه";
            case ErrorCode::INVALID_SYNTAX:
                return "راجع صيغة الأمر والأقواس والفواصل";
            case ErrorCode::DIVISION_BY_ZERO:
                return "تأكد من أن المقسوم عليه ليس صفراً";
            case ErrorCode::UNDEFINED_FUNCTION:
                return "تأكد من تعريف الدالة قبل استدعائها";
            case ErrorCode::WRONG_ARGUMENT_COUNT:
                return "تحقق من عدد المعاملات المطلوبة للدالة";
            case ErrorCode::FILE_NOT_FOUND:
                return "تأكد من وجود الملف والمسار الصحيح";
            case ErrorCode::ASSEMBLY_FAILED:
                return "تأكد من تثبيت NASM وإضافته لـ PATH";
            case ErrorCode::LINKING_FAILED:
                return "تأكد من تثبيت GCC/MinGW وإضافته لـ PATH";
            default:
                return "";
            }
        }

        std::vector<ErrorInfo> errors;
        int errorCount;
        int warningCount;
        bool hasErrors;
    };

    // ════════════════════════════════════════════════════════════════
    // ⚡ PART 2: CODE OPTIMIZER - محسّن الكود
    // ════════════════════════════════════════════════════════════════

    class CodeOptimizer {
    public:
        CodeOptimizer();

        struct OptimizationStats {
            int constant_folding_count = 0;
            int dead_code_removed = 0;
            int common_subexpr_eliminated = 0;
            int strength_reduced = 0;
            int loops_optimized = 0;
            int variables_inlined = 0;
        };

        std::vector<std::shared_ptr<Command>> optimize(
            const std::vector<std::shared_ptr<Command>>& commands);

        OptimizationStats getStats() const { return stats; }
        void printStats() const;

    private:
        OptimizationStats stats;
        std::map<std::string, Value> constantValues;

        std::shared_ptr<Command> optimizeCommand(const std::shared_ptr<Command>& cmd);
        Value optimizeValue(const Value& value);
        Value foldConstants(const Value& value);
        bool isConstant(const Value& value);
        int evaluateConstant(const Value& value);
        std::vector<std::shared_ptr<Command>> removeDeadCode(
            const std::vector<std::shared_ptr<Command>>& commands);
        Value reduceStrength(const Value& value);
        std::shared_ptr<Command> optimizeLoop(const std::shared_ptr<Command>& cmd);
        bool isLoopInvariant(const Value& value, const std::string& loopVar);
        void buildExpressionCache();
        std::string valueToString(const Value& value);
        bool areValuesEqual(const Value& v1, const Value& v2);
        bool isDeadCode(const std::shared_ptr<Command>& cmd);

        std::map<std::string, std::string> exprCache;
    };

    inline CodeOptimizer::CodeOptimizer() {
        stats = OptimizationStats();
    }

    inline std::vector<std::shared_ptr<Command>> CodeOptimizer::optimize(
        const std::vector<std::shared_ptr<Command>>& commands) {

        std::vector<std::shared_ptr<Command>> optimized;

        for (const auto& cmd : commands) {
            if (!cmd) continue;
            auto opt = optimizeCommand(cmd);
            if (opt) {
                optimized.push_back(opt);
            }
        }

        optimized = removeDeadCode(optimized);
        return optimized;
    }

    inline std::shared_ptr<Command> CodeOptimizer::optimizeCommand(
        const std::shared_ptr<Command>& cmd) {

        if (!cmd) return nullptr;

        auto optimized = std::make_shared<Command>(*cmd);

        if (cmd->type == CommandType::ASSIGNMENT) {
            optimized->value = optimizeValue(cmd->value);

            if (isConstant(optimized->value)) {
                constantValues[cmd->variable] = optimized->value;
            }
        }
        else if (cmd->type == CommandType::CONDITION) {
            if (cmd->condition) {
                optimized->condition = std::make_shared<Value>(optimizeValue(*cmd->condition));
            }

            if (optimized->condition && isConstant(*optimized->condition)) {
                int condValue = evaluateConstant(*optimized->condition);
                if (condValue != 0) {
                    stats.dead_code_removed += static_cast<int>(optimized->else_body.size());
                    optimized->else_body.clear();
                }
                else {
                    stats.dead_code_removed += static_cast<int>(optimized->body.size());
                    optimized->body = optimized->else_body;
                    optimized->else_body.clear();
                }
            }

            for (auto& bodyCmd : optimized->body) {
                bodyCmd = optimizeCommand(bodyCmd);
            }
            for (auto& altCmd : optimized->else_body) {
                altCmd = optimizeCommand(altCmd);
            }
        }
        else if (cmd->type == CommandType::LOOP_FOR ||
            cmd->type == CommandType::LOOP_WHILE) {
            optimized = optimizeLoop(cmd);
        }
        else if (cmd->type == CommandType::FUNCTION_CALL ||
            cmd->type == CommandType::PRINT) {
            for (auto& arg : optimized->arguments) {
                if (arg) {
                    *arg = optimizeValue(*arg);
                }
            }
        }

        return optimized;
    }

    inline Value CodeOptimizer::optimizeValue(const Value& value) {
        Value folded = foldConstants(value);
        Value reduced = reduceStrength(folded);
        return reduced;
    }

    inline Value CodeOptimizer::foldConstants(const Value& value) {
        if (value.type != ValueType::OPERATION) {
            return value;
        }

        Value left = value.left ? foldConstants(*value.left) : Value();
        Value right = value.right ? foldConstants(*value.right) : Value();

        if (isConstant(left) && isConstant(right)) {
            int leftVal = evaluateConstant(left);
            int rightVal = evaluateConstant(right);
            int result = 0;

            if (value.operator_ == "+") {
                result = leftVal + rightVal;
            }
            else if (value.operator_ == "-") {
                result = leftVal - rightVal;
            }
            else if (value.operator_ == "*") {
                result = leftVal * rightVal;
            }
            else if (value.operator_ == "/" && rightVal != 0) {
                result = leftVal / rightVal;
            }
            else if (value.operator_ == "%" && rightVal != 0) {
                result = leftVal % rightVal;
            }
            else if (value.operator_ == "^") {
                result = 1;
                for (int i = 0; i < rightVal; i++) {
                    result *= leftVal;
                }
            }
            else {
                Value newValue = value;
                newValue.left.reset(new Value(left));
                newValue.right.reset(new Value(right));
                return newValue;
            }

            stats.constant_folding_count++;
            return Value(static_cast<double>(result));
        }

        Value newValue = value;
        newValue.left.reset(new Value(left));
        newValue.right.reset(new Value(right));
        return newValue;
    }

    inline Value CodeOptimizer::reduceStrength(const Value& value) {
        if (value.type != ValueType::OPERATION) {
            return value;
        }

        if (value.operator_ == "*") {
            if ((value.left && isConstant(*value.left) &&
                evaluateConstant(*value.left) == 0) ||
                (value.right && isConstant(*value.right) &&
                    evaluateConstant(*value.right) == 0)) {
                stats.strength_reduced++;
                return Value(0.0);
            }
        }

        if (value.operator_ == "+") {
            if (value.right && isConstant(*value.right) &&
                evaluateConstant(*value.right) == 0) {
                stats.strength_reduced++;
                return *value.left;
            }
            if (value.left && isConstant(*value.left) &&
                evaluateConstant(*value.left) == 0) {
                stats.strength_reduced++;
                return *value.right;
            }
        }

        return value;
    }

    inline bool CodeOptimizer::isConstant(const Value& value) {
        if (value.type == ValueType::NUMBER) {
            // Verify that the value can be converted to a valid integer
            try {
                std::stoi(value.value);
                return true;
            } catch (...) {
                return false;
            }
        }

        if (value.type == ValueType::VARIABLE) {
            return constantValues.find(value.value) != constantValues.end();
        }

        return false;
    }

    inline int CodeOptimizer::evaluateConstant(const Value& value) {
        if (value.type == ValueType::NUMBER) {
            try {
                return std::stoi(value.value);
            } catch (...) {
                return 0;
            }
        }

        if (value.type == ValueType::VARIABLE) {
            auto it = constantValues.find(value.value);
            if (it != constantValues.end()) {
                return evaluateConstant(it->second);
            }
        }

        return 0;
    }

    inline std::vector<std::shared_ptr<Command>> CodeOptimizer::removeDeadCode(
        const std::vector<std::shared_ptr<Command>>& commands) {

        std::vector<std::shared_ptr<Command>> alive;

        for (const auto& cmd : commands) {
            if (!cmd || isDeadCode(cmd)) {
                stats.dead_code_removed++;
                continue;
            }
            alive.push_back(cmd);
        }

        return alive;
    }

    inline bool CodeOptimizer::isDeadCode(const std::shared_ptr<Command>&) {
        return false;
    }

    inline std::shared_ptr<Command> CodeOptimizer::optimizeLoop(
        const std::shared_ptr<Command>& cmd) {

        auto optimized = std::make_shared<Command>(*cmd);

        for (auto& bodyCmd : optimized->body) {
            bodyCmd = optimizeCommand(bodyCmd);
        }

        stats.loops_optimized++;
        return optimized;
    }

    inline bool CodeOptimizer::isLoopInvariant(const Value& value, const std::string& loopVar) {
        if (value.type == ValueType::VARIABLE) {
            return value.value != loopVar;
        }

        if (value.type == ValueType::OPERATION) {
            bool leftInvariant = value.left ? isLoopInvariant(*value.left, loopVar) : true;
            bool rightInvariant = value.right ? isLoopInvariant(*value.right, loopVar) : true;
            return leftInvariant && rightInvariant;
        }

        return true;
    }

    inline void CodeOptimizer::buildExpressionCache() {
        exprCache.clear();
    }

    inline std::string CodeOptimizer::valueToString(const Value& value) {
        std::ostringstream oss;

        if (value.type == ValueType::NUMBER) {
            oss << value.value;
        }
        else if (value.type == ValueType::STRING) {
            oss << "\"" << value.value << "\"";
        }
        else if (value.type == ValueType::VARIABLE) {
            oss << value.value;
        }
        else if (value.type == ValueType::OPERATION) {
            oss << "(";
            if (value.left) {
                oss << valueToString(*value.left);
            }
            oss << " " << value.operator_ << " ";
            if (value.right) {
                oss << valueToString(*value.right);
            }
            oss << ")";
        }
        else if (value.type == ValueType::FUNCTION_CALL) {
            oss << value.function_name << "(";
            for (size_t i = 0; i < value.arguments.size(); i++) {
                if (i > 0) oss << ", ";
                if (value.arguments[i]) {
                    oss << valueToString(*value.arguments[i]);
                }
            }
            oss << ")";
        }
        else {
            oss << "unknown";
        }

        return oss.str();
    }

    inline bool CodeOptimizer::areValuesEqual(const Value& v1, const Value& v2) {
        if (v1.type != v2.type) {
            return false;
        }

        if (v1.type == ValueType::NUMBER || v1.type == ValueType::STRING ||
            v1.type == ValueType::VARIABLE) {
            return v1.value == v2.value;
        }

        if (v1.type == ValueType::OPERATION) {
            if (v1.operator_ != v2.operator_) {
                return false;
            }

            bool leftEqual = (v1.left && v2.left) ?
                areValuesEqual(*v1.left, *v2.left) : (!v1.left && !v2.left);

            bool rightEqual = (v1.right && v2.right) ?
                areValuesEqual(*v1.right, *v2.right) : (!v1.right && !v2.right);

            return leftEqual && rightEqual;
        }

        return false;
    }

    inline void CodeOptimizer::printStats() const {
        std::cout << "\n⚡ إحصائيات التحسين:" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "  📊 طي الثوابت: " << stats.constant_folding_count << std::endl;
        std::cout << "  🗑️  كود ميت محذوف: " << stats.dead_code_removed << std::endl;
        std::cout << "  💪 تقليل القوة: " << stats.strength_reduced << std::endl;
        std::cout << "  🔄 حلقات محسنة: " << stats.loops_optimized << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;

        int totalOpts = stats.constant_folding_count +
            stats.dead_code_removed +
            stats.strength_reduced +
            stats.loops_optimized;

        if (totalOpts > 0) {
            std::cout << "✅ مجموع التحسينات: " << totalOpts << std::endl;
        }
        else {
            std::cout << "ℹ️  لا توجد تحسينات متاحة" << std::endl;
        }
    }

    // ════════════════════════════════════════════════════════════════
    // 🐛 PART 3: ARABIC DEBUGGER - نظام التصحيح
    // ════════════════════════════════════════════════════════════════

    class ArabicDebugger {
    public:
        enum class DebugLevel {
            OFF,
            MINIMAL,
            VERBOSE,
            FULL
        };

        ArabicDebugger() : level(DebugLevel::OFF), stepMode(false), maxCallDepth(0) {}

        void setDebugLevel(DebugLevel lvl) { level = lvl; }
        void enableStepMode(bool enable) { stepMode = enable; }

        void trackVariable(const std::string& name, int value, int line) {
            if (level == DebugLevel::OFF) return;

            VariableState state{ value, line };
            variableHistory[name].push_back(state);

            if (level >= DebugLevel::VERBOSE) {
                std::cout << "🔍 [السطر " << line << "] "
                    << name << " = " << value << std::endl;
            }
        }

        void enterFunction(const std::string& name,
            const std::vector<std::string>& params,
            int line) {
            if (level == DebugLevel::OFF) return;

            FunctionCall call{ name, params, line };
            callStack.push_back(call);

            if (level >= DebugLevel::MINIMAL) {
                std::string indent(callStack.size() * 2, ' ');
                std::cout << indent << "➡️  دخول الدالة: " << name
                    << " [السطر " << line << "]" << std::endl;

                if (level >= DebugLevel::VERBOSE && !params.empty()) {
                    std::cout << indent << "   المعاملات: ";
                    for (size_t i = 0; i < params.size(); i++) {
                        if (i > 0) std::cout << ", ";
                        std::cout << params[i];
                    }
                    std::cout << std::endl;
                }
            }
        }

        void exitFunction(const std::string& name, int returnValue) {
            if (level == DebugLevel::OFF || callStack.empty()) return;

            std::string indent(callStack.size() * 2, ' ');
            if (level >= DebugLevel::MINIMAL) {
                std::cout << indent << "⬅️  خروج من: " << name
                    << " (القيمة المرجعة: " << returnValue << ")"
                    << std::endl;
            }

            callStack.pop_back();
        }

        void logCondition(const std::string& condition, bool result, int line) {
            if (level < DebugLevel::VERBOSE) return;

            std::cout << "🔀 [السطر " << line << "] شرط: "
                << condition << " = "
                << (result ? "صحيح ✅" : "خاطئ ❌") << std::endl;
        }

        void logLoop(const std::string& loopType,
            const std::string&,
            int iteration, int line) {
            if (level < DebugLevel::VERBOSE) return;

            std::cout << "🔄 [السطر " << line << "] " << loopType
                << " - التكرار #" << iteration << std::endl;
        }

        void logAssemblyLine(const std::string& asmCode, int sourceLine) {
            if (level != DebugLevel::FULL) return;

            std::cout << "⚙️  [" << std::setw(3) << sourceLine << "] "
                << asmCode << std::endl;
        }

        void printVariableSnapshot() {
            if (level == DebugLevel::OFF) return;

            std::cout << "\n📊 لقطة المتغيرات الحالية:" << std::endl;
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;

            for (const auto& [name, history] : variableHistory) {
                if (!history.empty()) {
                    const auto& latest = history.back();
                    std::cout << "  " << std::setw(15) << std::left << name
                        << " = " << std::setw(8) << latest.value
                        << " [السطر " << latest.line << "]" << std::endl;
                }
            }

            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        }

        void printCallStack() {
            if (level == DebugLevel::OFF || callStack.empty()) return;

            std::cout << "\n📚 مكدس الاستدعاءات:" << std::endl;
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;

            for (int i = static_cast<int>(callStack.size()) - 1; i >= 0; i--) {
                const auto& call = callStack[i];
                std::cout << "  #" << i << " " << call.functionName
                    << " [السطر " << call.line << "]" << std::endl;
            }

            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        }

        void waitForStep(int line) {
            if (!stepMode || level == DebugLevel::OFF) return;

            std::cout << "\n⏸️  [السطر " << line << "] ";
            std::cout << "اضغط Enter للمتابعة (أو 'v' لعرض المتغيرات، 'q' للخروج): ";

            std::string input;
            std::getline(std::cin, input);

            if (input == "v") {
                printVariableSnapshot();
                printCallStack();
                waitForStep(line);
            }
            else if (input == "q") {
                stepMode = false;
            }
        }

        void printVariableHistory(const std::string& varName) {
            auto it = variableHistory.find(varName);
            if (it == variableHistory.end()) {
                std::cout << "⚠️  المتغير '" << varName << "' غير موجود" << std::endl;
                return;
            }

            std::cout << "\n📜 تاريخ المتغير: " << varName << std::endl;
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;

            for (size_t i = 0; i < it->second.size(); i++) {
                const auto& state = it->second[i];
                std::cout << "  #" << i << " القيمة: " << std::setw(8) << state.value
                    << " [السطر " << state.line << "]" << std::endl;
            }

            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        }

        void printDebugStats() {
            if (level == DebugLevel::OFF) return;

            std::cout << "\n📈 إحصائيات التصحيح:" << std::endl;
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
            std::cout << "  المتغيرات المتتبعة: " << variableHistory.size() << std::endl;
            std::cout << "  أعمق استدعاء: " << maxCallDepth << std::endl;
            std::cout << "  مستوى التصحيح: " << debugLevelToString(level) << std::endl;
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        }

    private:
        DebugLevel level;
        bool stepMode;

        struct VariableState {
            int value;
            int line;
        };

        struct FunctionCall {
            std::string functionName;
            std::vector<std::string> params;
            int line;
        };

        std::map<std::string, std::vector<VariableState>> variableHistory;
        std::vector<FunctionCall> callStack;
        int maxCallDepth;

        std::string debugLevelToString(DebugLevel lvl) {
            switch (lvl) {
            case DebugLevel::OFF: return "متوقف";
            case DebugLevel::MINIMAL: return "أساسي";
            case DebugLevel::VERBOSE: return "تفصيلي";
            case DebugLevel::FULL: return "كامل";
            default: return "غير معروف";
            }
        }
    };

} // namespace ArabicLanguage

#endif // ARABIC_UTILITIES_H
