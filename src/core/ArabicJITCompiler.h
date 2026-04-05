#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <chrono>
#include "ArabicTypes.h"
#include "ArabicMemoryManager.h"

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // ⚡ مترجم JIT متقدم مع تخزين مؤقت للبايت كود
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief نظام JIT مع تحسينات الأداء المتقدمة
     */
    class ArabicJITCompiler {
    public:
        // Singleton pattern
        static ArabicJITCompiler& getInstance();

        // منع النسخ والتعيين
        ArabicJITCompiler(const ArabicJITCompiler&) = delete;
        ArabicJITCompiler& operator=(const ArabicJITCompiler&) = delete;

        // ══════════════════════════════════════════════════════════════
        // 📝 رموز البايت كود (Bytecode Opcodes)
        // ══════════════════════════════════════════════════════════════

        enum class Opcode {
            // عمليات أساسية
            NOP = 0,           // لا عمل
            LOAD_CONST,        // تحميل ثابت
            LOAD_VAR,          // تحميل متغير
            STORE_VAR,         // حفظ متغير
            LOAD_ARRAY,        // تحميل من مصفوفة
            STORE_ARRAY,       // حفظ في مصفوفة

            // عمليات حسابية
            ADD,               // جمع
            SUB,               // طرح
            MUL,               // ضرب
            DIV,               // قسمة
            MOD,               // باقي القسمة
            POW,               // أس

            // عمليات منطقية ومقارنة
            EQ,                // يساوي
            NE,                // لا يساوي
            LT,                // أصغر من
            GT,                // أكبر من
            LE,                // أصغر أو يساوي
            GE,                // أكبر أو يساوي
            AND,               // و
            OR,                // أو
            NOT,               // ليس

            // تدفق التحكم
            JMP,               // قفز
            JMP_IF_TRUE,       // قفز إذا صحيح
            JMP_IF_FALSE,      // قفز إذا خطأ
            CALL,              // استدعاء دالة
            RET,               // إرجاع
            CALL_NATIVE,       // استدعاء دالة أصلية

            // كائنات ومصفوفات
            NEW_ARRAY,         // إنشاء مصفوفة
            NEW_OBJECT,        // إنشاء كائن
            LOAD_PROP,         // تحميل خاصية
            STORE_PROP,        // حفظ خاصية
            LOAD_METHOD,       // تحميل دالة
            CALL_METHOD,       // استدعاء دالة كائن

            // عمليات متقدمة
            LOAD_CLOSURE,      // تحميل إغلاق
            MAKE_CLOSURE,      // إنشاء إغلاق
            LOAD_UPVALUE,      // تحميل قيمة علوية
            SET_UPVALUE,       // تعيين قيمة علوية

            // async/await
            ASYNC_CALL,        // استدعاء غير متزامن
            AWAIT,             // انتظار
            SPAWN_TASK,        // إنشاء مهمة
            YIELD,             // إنتاج

            // generics
            GENERIC_INSTANTIATE, // إنشاء نسخة عامة
            TYPE_CHECK,        // فحص نوع
            CAST,              // تحويل نوع

            // تحسينات
            OPTIMIZED_ADD,     // جمع محسّن
            OPTIMIZED_MUL,     // ضرب محسّن
            BATCH_LOAD,        // تحميل مجمع
            BATCH_STORE,       // حفظ مجمع
        };

        // ══════════════════════════════════════════════════════════════
        // 🏗️ بنية تعليمة البايت كود
        // ══════════════════════════════════════════════════════════════

        struct BytecodeInstruction {
            Opcode opcode;
            uint32_t operand1;  // معامل 1
            uint32_t operand2;  // معامل 2
            uint32_t operand3;  // معامل 3

            BytecodeInstruction(Opcode op = Opcode::NOP, uint32_t op1 = 0, uint32_t op2 = 0, uint32_t op3 = 0)
                : opcode(op), operand1(op1), operand2(op2), operand3(op3) {}

            std::string toString() const;
        };

        // ══════════════════════════════════════════════════════════════
        // 📦 بنية البايت كود المجمع
        // ══════════════════════════════════════════════════════════════

        struct CompiledBytecode {
            std::string sourceHash;                    // هاش الكود المصدري
            std::vector<BytecodeInstruction> code;     // البايت كود
            std::vector<Value> constants;              // الثوابت
            std::vector<std::string> variableNames;    // أسماء المتغيرات
            std::unordered_map<std::string, size_t> functionTable; // جدول الدوال
            std::chrono::steady_clock::time_point compiledAt; // وقت التجميع
            size_t executionCount = 0;                // عدد مرات التنفيذ
            double averageExecutionTime = 0.0;        // متوسط وقت التنفيذ

            size_t getSize() const {
                return code.size() * sizeof(BytecodeInstruction) +
                       constants.size() * sizeof(Value) +
                       variableNames.size() * 32; // تقدير حجم السلاسل
            }
        };

        // ══════════════════════════════════════════════════════════════
        // 💾 نظام التخزين المؤقت (Cache)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief نظام تخزين مؤقت ذكي للبايت كود
         */
        class BytecodeCache {
        private:
            struct CacheEntry {
                std::shared_ptr<CompiledBytecode> bytecode;
                std::chrono::steady_clock::time_point lastAccess;
                size_t accessCount = 0;

                CacheEntry(std::shared_ptr<CompiledBytecode> bc = nullptr)
                    : bytecode(std::move(bc)), lastAccess(std::chrono::steady_clock::now()) {}
            };

            std::unordered_map<std::string, CacheEntry> cache;
            mutable std::mutex cacheMutex;
            size_t maxSize = 100 * 1024 * 1024; // 100 ميجابايت
            size_t currentSize = 0;
            std::atomic<size_t> hits{0};
            std::atomic<size_t> misses{0};

            void evictLRU();
            void cleanupExpired();

        public:
            std::shared_ptr<CompiledBytecode> get(const std::string& hash);
            void put(const std::string& hash, std::shared_ptr<CompiledBytecode> bytecode);
            void clear();
            size_t getSize() const { return currentSize; }
            double getHitRate() const {
                size_t total = hits.load() + misses.load();
                return total > 0 ? (hits.load() * 100.0 / total) : 0.0;
            }

            std::vector<std::string> getStats() const;
        };

        // ══════════════════════════════════════════════════════════════
        // ⚡ مترجم JIT
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مترجم JIT مع تحسينات متقدمة
         */
        class JITCompiler {
        private:
            std::unordered_map<std::string, std::function<void*(const CompiledBytecode&)>> compiledFunctions;
            std::mutex jitMutex;

        public:
            void* compile(const CompiledBytecode& bytecode);
            void optimize(CompiledBytecode& bytecode);
            void clearCache();
            size_t getCacheSize() const { return compiledFunctions.size(); }
        };

        // ══════════════════════════════════════════════════════════════
        // 🎯 واجهة الاستخدام العامة
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تجميع كود عربي إلى بايت كود
         */
        std::shared_ptr<CompiledBytecode> compileToBytecode(
            const std::string& sourceCode,
            const std::string& filename = "");

        /**
         * @brief تنفيذ بايت كود مجمع
         */
        Value executeBytecode(const std::shared_ptr<CompiledBytecode>& bytecode);

        /**
         * @brief تحسين بايت كود
         */
        void optimizeBytecode(std::shared_ptr<CompiledBytecode>& bytecode);

        /**
         * @brief حفظ بايت كود في ملف
         */
        bool saveBytecode(const std::shared_ptr<CompiledBytecode>& bytecode, const std::string& filename);

        /**
         * @brief تحميل بايت كود من ملف
         */
        std::shared_ptr<CompiledBytecode> loadBytecode(const std::string& filename);

        // إدارة التخزين المؤقت
        BytecodeCache& getCache() { return cache; }
        void clearCache() { cache.clear(); }

        // إحصائيات الأداء
        std::vector<std::string> getPerformanceStats() const;

        // تحسينات JIT
        JITCompiler& getJIT() { return jitCompiler; }

    private:
        BytecodeCache cache;
        JITCompiler jitCompiler;
        std::hash<std::string> hasher;

        ArabicJITCompiler() = default;
        ~ArabicJITCompiler() = default;

        // دوال مساعدة للتجميع
        std::string computeHash(const std::string& source) const;
        std::vector<BytecodeInstruction> generateBytecode(const std::vector<std::shared_ptr<Command>>& commands);
        void optimizeInstructions(std::vector<BytecodeInstruction>& instructions);
        void inlineFunctions(std::vector<BytecodeInstruction>& instructions);
        void extractConstantsAndVariables(
            const std::vector<std::shared_ptr<Command>>& commands,
            CompiledBytecode& bytecode);

        // دوال مساعدة للتنفيذ
        Value executeInstruction(const BytecodeInstruction& instr,
                                std::vector<Value>& stack,
                                std::unordered_map<std::string, Value>& variables);
    };

    // ══════════════════════════════════════════════════════════════
    // 🏃‍♂️ محرك تنفيذ JIT
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief محرك تنفيذ محسّن مع دعم JIT
     */
    class JITExecutionEngine {
    private:
        ArabicJITCompiler& compiler;
        std::vector<Value> valueStack;
        std::unordered_map<std::string, Value> globalVariables;
        std::vector<std::unordered_map<std::string, Value>> callStack;
        size_t instructionPointer = 0;

        // إحصائيات الأداء
        std::atomic<size_t> totalInstructions{0};
        std::atomic<size_t> optimizedInstructions{0};

    public:
        explicit JITExecutionEngine(ArabicJITCompiler& comp) : compiler(comp) {}

        Value execute(const std::shared_ptr<ArabicJITCompiler::CompiledBytecode>& bytecode);
        void reset();

        // إحصائيات
        size_t getTotalInstructions() const { return totalInstructions.load(); }
        size_t getOptimizedInstructions() const { return optimizedInstructions.load(); }
        double getOptimizationRatio() const {
            size_t total = totalInstructions.load();
            return total > 0 ? (optimizedInstructions.load() * 100.0 / total) : 0.0;
        }

        // مراقبة
        void enableProfiling(bool enable);
        std::vector<std::string> getExecutionStats() const;
    };

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة عامة
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief إنشاء محرك تنفيذ JIT جديد
     */
    JITExecutionEngine createJITEngine();

    /**
     * @brief تجميع وتنفيذ كود عربي باستخدام JIT
     */
    Value compileAndExecuteJIT(const std::string& sourceCode, const std::string& filename = "");

    /**
     * @brief تسخين التخزين المؤقت لتحسين الأداء
     */
    void warmupJITCache(const std::vector<std::string>& commonCode);

    /**
     * @brief تنظيف موارد JIT
     */
    void cleanupJITResources();

} // namespace ArabicLanguage
