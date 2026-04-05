// ArabicExecutor.h - نظام التنفيذ
#ifndef ARABIC_EXECUTOR_H
#define ARABIC_EXECUTOR_H

#include "../runtime/ArabicTypes.h"
#include "ArabicRuntime.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <functional>
#include <sstream>

namespace ArabicLanguage {

    using OutputCallback = std::function<void(const std::string&)>;

    // ═══════════════════════════════════════════════════════════
    // نظام التنفيذ (Executor)
    // ═══════════════════════════════════════════════════════════
    
    class ArabicExecutor {
    public:
        explicit ArabicExecutor(std::shared_ptr<ArabicRuntime> runtime = nullptr);
        ~ArabicExecutor() = default;
        
        // ────────────────────────────────────────────────────────
        // تنفيذ الأوامر
        // ────────────────────────────────────────────────────────
        
        // تنفيذ قائمة الأوامر
        Value execute(const std::vector<std::shared_ptr<Command>>& commands, bool resetRuntime = true);
        
        // تنفيذ أمر واحد
        Value executeCommand(const std::shared_ptr<Command>& cmd);
        
        // تنفيذ كتلة من الأوامر
        Value executeBlock(const std::vector<std::shared_ptr<Command>>& commands);

        // ────────────────────────────────────────────────────────
        // نظام JIT
        // ────────────────────────────────────────────────────────

        // تفعيل/تعطيل JIT
        void enableJIT(bool enable = true) { jitEnabled = enable; }
        bool isJITEnabled() const { return jitEnabled; }

        // الحصول على إحصائيات JIT
        std::vector<std::string> getJITStats() const;

        // تحسين الدوال الشائعة
        void optimizeHotFunctions();

        // تعيين دالة استرجاع المخرجات
        void setOutputCallback(OutputCallback cb) { outputCallback = cb; }

        // ✅ نظام التحكم في التنفيذ (Phase 8+)
        void stop() { shouldStop = true; }
        bool isStopped() const { return shouldStop; }
        void resetStop() { shouldStop = false; }
        
    private:
        std::shared_ptr<ArabicRuntime> runtime;
        OutputCallback outputCallback;
        volatile bool shouldStop = false; // ✅ علم لإيقاف التنفيذ فوراً

        // تتبع استدعاءات الدوال لتحديد الدوال الشائعة
        struct FunctionCallInfo {
            size_t callCount;
            std::chrono::nanoseconds totalTime;
            std::chrono::steady_clock::time_point lastCall;
            bool isCompiled;

            FunctionCallInfo() : callCount(0), totalTime(0), isCompiled(false),
                               lastCall(std::chrono::steady_clock::now()) {}
        };

        std::unordered_map<std::string, FunctionCallInfo> functionCallStats;
        mutable std::mutex functionStatsMutex;
        bool jitEnabled;
        
        // ────────────────────────────────────────────────────────
        // تنفيذ الأوامر المختلفة
        // ────────────────────────────────────────────────────────
        
        Value executeAssignment(const std::shared_ptr<Command>& cmd);
        Value executePrint(const std::shared_ptr<Command>& cmd);
        Value executePrintNoNewline(const std::shared_ptr<Command>& cmd);
        Value executeIf(const std::shared_ptr<Command>& cmd);
        Value executeWhile(const std::shared_ptr<Command>& cmd);
        Value executeFor(const std::shared_ptr<Command>& cmd);
        Value executeForEach(const std::shared_ptr<Command>& cmd);
        Value executeFunctionDef(const std::shared_ptr<Command>& cmd);
        Value executeFunctionCall(const std::shared_ptr<Command>& cmd);
        Value executeReturn(const std::shared_ptr<Command>& cmd);
        Value executeWait(const std::shared_ptr<Command>& cmd);
        Value executeArrayAssignment(const std::shared_ptr<Command>& cmd);
        Value executeClassDef(const std::shared_ptr<Command>& cmd);
        Value executeObjectCreation(const std::shared_ptr<Command>& cmd);
        Value executeMethodCall(const std::shared_ptr<Command>& cmd);
        Value executeImport(const std::shared_ptr<Command>& cmd);
        Value executePropertyDef(const std::shared_ptr<Command>& cmd);
        Value executeInterfaceDef(const std::shared_ptr<Command>& cmd); // ✅ Stage 6
        Value executeBreak(const std::shared_ptr<Command>& cmd);
        Value executeContinue(const std::shared_ptr<Command>& cmd);
        Value executeTry(const std::shared_ptr<Command>& cmd);   // ✅ Stage 6
        Value executeThrow(const std::shared_ptr<Command>& cmd); // ✅ Stage 6
        Value executeClassMethodDef(const std::shared_ptr<Command>& cmd); // ✅ Added for Class Methods
        
        // ────────────────────────────────────────────────────────
        // تقييم التعبيرات
        // ────────────────────────────────────────────────────────
        
        Value evaluateExpression(const Value& expr);
        Value evaluateCondition(const Value& cond);
        Value evaluateOperation(const Value& expr);
        Value evaluateFunctionCall(const Value& expr);
        Value evaluateArrayAccess(const Value& expr);
        Value evaluatePropertyAccess(const Value& expr);
        
        // ────────────────────────────────────────────────────────
        // دوال مساعدة
        // ────────────────────────────────────────────────────────
        
        bool isTruthy(const Value& value);
        std::string valueToString(const Value& value);
        Value stringToValue(const std::string& str);
        Value performOperation(const std::string& op, const Value& left, const Value& right);
        Value performUnaryOperation(const std::string& op, const Value& value);
        double valueToNumber(const Value& value);
        
        // التحقق من الكلمات المحجوزة للبرمجة الكائنية بالبايت لتجنب مشاكل الترميز
        bool isThisKeyword(const std::string& name);
        bool isSuperKeyword(const std::string& name);
        
        // ────────────────────────────────────────────────────────
        // دوال العمليات الحسابية
        // ────────────────────────────────────────────────────────
        
        Value add(const Value& left, const Value& right);
        Value subtract(const Value& left, const Value& right);
        Value multiply(const Value& left, const Value& right);
        Value divide(const Value& left, const Value& right);
        Value modulo(const Value& left, const Value& right);
        
        // ────────────────────────────────────────────────────────
        // دوال المقارنة
        // ────────────────────────────────────────────────────────
        
        Value equals(const Value& left, const Value& right);
        Value notEquals(const Value& left, const Value& right);
        Value lessThan(const Value& left, const Value& right);
        Value lessThanOrEqual(const Value& left, const Value& right);
        Value greaterThan(const Value& left, const Value& right);
        Value greaterThanOrEqual(const Value& left, const Value& right);
        
        // ────────────────────────────────────────────────────────
        // دوال العمليات المنطقية
        // ────────────────────────────────────────────────────────
        
        Value logicalAnd(const Value& left, const Value& right);
        Value logicalOr(const Value& left, const Value& right);
        Value logicalNot(const Value& value);
        
        // ────────────────────────────────────────────────────────
        // دوال المصفوفات
        // ────────────────────────────────────────────────────────
        
        Value arrayLength(const Value& array);
        Value arrayAccess(const Value& array, size_t index);
        void arraySet(Value& array, size_t index, const Value& value);
        size_t getGenericListSize(const Value& array) const;
        
        // ────────────────────────────────────────────────────────
        // معالجة الأخطاء
        // ────────────────────────────────────────────────────────
        
        void throwError(const std::string& message) const;
        void printError(const std::string& message);
        void printWarning(const std::string& message);
        
        // ────────────────────────────────────────────────────────
        // إحصائيات
        // ────────────────────────────────────────────────────────
        
        size_t commandCount = 0;
        mutable size_t executionErrors = 0;

        // Flags and utilities
        void setVerbose(bool v);
        void logDebug(const std::string& msg);
        void importLibrary(const std::string& libName);

        // ────────────────────────────────────────────────────────
        // دوال مساعدة JIT
        // ────────────────────────────────────────────────────────

        // تسجيل استدعاء دالة
        void recordFunctionCall(const std::string& functionName, std::chrono::nanoseconds executionTime);

        // الحصول على الدوال الشائعة
        std::vector<std::string> getHotFunctions(size_t minCalls = 5) const;

        // محاولة ترجمة دالة
        bool tryCompileFunction(const std::string& functionName);

        // ────────────────────────────────────────────────────────
        // فحص الأنواع في وقت التشغيل
        // ────────────────────────────────────────────────────────

        // فحص توافق الأنواع
        bool checkTypeCompatibility(const Value& value, ValueType expectedType) const;

        // تحويل نوع تلقائي
        Value performAutomaticConversion(const Value& value, ValueType targetType);

        // التحقق من صحة العملية
        bool validateOperation(const Value& left, const std::string& op, const Value& right) const;

        // فحص حدود المصفوفة
        bool validateArrayAccess(const Value& array, int index) const;
    };

} // namespace ArabicLanguage

#endif // ARABIC_EXECUTOR_H
