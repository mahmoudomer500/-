// ArabicCompiler.h - الفئة الموحدة للمترجم مع المجمع العربي
#ifndef ARABIC_COMPILER_H
#define ARABIC_COMPILER_H

#include "ArabicParser.h"
#include "CodeGenerator.h"
#include "BridgeManager.h"
#include <string>
#include <fstream>
#include <memory>
#include <functional>
#include <iomanip>
#include <sstream>

// ✅ دعم شرطي للمجمع العربي
#ifdef NO_ARABIC_ASSEMBLER
#define HAS_ARABIC_ASSEMBLER 0
#else
#include "ArabicAssembler.h"
#define HAS_ARABIC_ASSEMBLER 1
#endif

namespace ArabicLanguage {

    using OutputCallback = std::function<void(const std::string&)>;

    // ══════════════════════════════════════════════════════════════
    // 📝 نظام التسجيل المحسن
    // ══════════════════════════════════════════════════════════════
    enum class LogLevel {
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARNING,
        LOG_ERROR,
        LOG_NONE
    };

    class Logger {
    private:
        LogLevel currentLevel;
        bool logToFile;
        std::string logFileName;
        std::ofstream logFile;

    public:
        Logger();
        ~Logger();

        void setLogLevel(LogLevel level);
        void setLogToFile(bool enable, const std::string& filename = "arabic_compiler.log");
        void enableFileLogging(const std::string& filename = "arabic_compiler.log");

        void logDebug(const std::string& message);
        void logInfo(const std::string& message);
        void logWarning(const std::string& message);
        void logError(const std::string& message);

        void log(LogLevel level, const std::string& message);

        LogLevel getCurrentLevel() const { return currentLevel; }
        bool isLoggingToFile() const { return logToFile; }
    };

    // Global logger getter
    Logger& getGlobalLogger();

    class ArabicCompiler {
    public:
        ArabicCompiler();

        // الدوال الرئيسية
        bool compile(const std::string& arabicCode, const std::string& outputFile = "program");
        bool compileFile(const std::string& inputFile, const std::string& outputFile = "");
        
        // ✅ دوال جديدة لدعم ملفات .ar
        bool compileArabicFile(const std::string& arabicFilePath);
        bool isArabicFile(const std::string& filename) const;
        bool readArabicFile(const std::string& filePath, std::string& content) const;
        bool executeIntermediate(const std::string& arabicCode);
        void stopExecution(); // ✅ إضافة دالة لإيقاف التنفيذ
        void setPersistentRuntime(std::shared_ptr<ArabicRuntime> runtime) { persistentRuntime = runtime; }
        std::shared_ptr<ArabicRuntime> getPersistentRuntime() { return persistentRuntime; }

        // دوال التجميع الذاتي
        bool selfCompile();
        bool buildSelfHosting();

        // معلومات وإحصائيات
        void displayStatistics() const;
        std::string getVersion() const;

        // 📝 دوال التسجيل
        void setLogLevel(LogLevel level);
        void enableFileLogging(const std::string& filename = "arabic_compiler.log");
        void disableFileLogging();

        // دوال التسجيل المباشرة
        void logDebug(const std::string& message);
        void logInfo(const std::string& message);
        void logWarning(const std::string& message);
        void logError(const std::string& message);

        // ✅ إعدادات المجمع
        void setUseArabicAssembler(bool use);
        void setOutputCallback(OutputCallback cb); // تعيين دالة المخرجات
        bool isUsingArabicAssembler() const {
#if HAS_ARABIC_ASSEMBLER
            return useArabicAssembler;
#else
            return false;
#endif
        }

        // ✅ نظام الجسور الجديد (Phase 8)
        void setUseBridges(bool use) { useBridges = use; }
        bool isUsingBridges() const { return useBridges; }

    private:
        ArabicParser parser;
        std::shared_ptr<ArabicRuntime> persistentRuntime;
        std::shared_ptr<ArabicExecutor> currentExecutor; // ✅ تتبع المنفذ الحالي

        // مدير الجسور الموحد
        BridgeManager& bridgeManager = BridgeManager::getInstance();
        bool useBridges = false;

        // إحصائيات التراجع
        int arabic_parse_attempts = 0;
        int arabic_parse_successes = 0;
        int arabic_parse_fallbacks = 0;

        // Performance optimizations
        std::unordered_map<std::string, uint32_t> stringCache; // Cache for string literals
        std::vector<std::string> stringPool; // Pool of unique strings



        ArabicAssembler::CodeGenerator generator; // ✅ استخدام المولد المحسّن
        OutputCallback outputCallback; // دالة رد النداء للمخرجات

#if HAS_ARABIC_ASSEMBLER
        bool useArabicAssembler = true;  // ✅ علم استخدام المجمع العربي
#else
        bool useArabicAssembler = false; // ❌ معطل إذا لم يكن موجوداً
#endif

        // دوال مساعدة
        std::string readFile(const std::string& filename);
        bool saveToFile(const std::string& content, const std::string& filename);
        void printPEBriefReport(const std::string& exeFile) const;

        // ✅ دوال التجميع
        bool assembleWithArabicAssembler(const std::string& assemblyCode,
            const std::string& outputFile);
        bool assembleWithNASM(const std::string& asmFile,
            const std::string& exeFile);

        std::string sanitizeFilename(const std::string& arabicName);

        // إحصائيات مفصلة
        struct CompilerStats {
            // إحصائيات أساسية
            int total_commands = 0;
            int variables_count = 0;
            int functions_count = 0;
            long long compilation_time_ms = 0;
            bool success = false;

            // إحصائيات مفصلة جديدة
            int if_statements = 0;           // عدد الشروط (if)
            int loop_statements = 0;         // عدد الحلقات
            int assignments = 0;             // عدد التعيينات
            int function_calls = 0;          // عدد استدعاءات الدوال
            int arrays_created = 0;          // عدد المصفوفات المُنشأة
            int strings_created = 0;         // عدد السلاسل النصية المُنشأة
            int arithmetic_ops = 0;          // عدد العمليات الحسابية
            int comparison_ops = 0;          // عدد عمليات المقارنة
            int logical_ops = 0;             // عدد العمليات المنطقية

            // إحصائيات الذاكرة والكود
            size_t code_size_bytes = 0;      // حجم الكود المولد
            size_t data_size_bytes = 0;      // حجم البيانات
            size_t total_exe_size = 0;       // الحجم الإجمالي للملف التنفيذي
            int relocations_count = 0;       // عدد عمليات إعادة التوجيه

            // إحصائيات الأداء
            int parse_time_ms = 0;           // وقت التحليل
            int codegen_time_ms = 0;         // وقت توليد الكود
            int linking_time_ms = 0;         // وقت الربط

            // إحصائيات متقدمة
            int external_functions_used = 0; // عدد الدوال الخارجية المستخدمة
            int imports_count = 0;           // عدد المكتبات المستوردة
            int warnings_count = 0;          // عدد التحذيرات
            int errors_count = 0;            // عدد الأخطاء

            // إحصائيات الأداء المتقدمة
            double memory_peak_mb = 0;       // ذروة استخدام الذاكرة بالميجابايت
            int symbols_created = 0;          // عدد الرموز المُنشأة
            int instructions_generated = 0;   // عدد التعليمات المولدة
            int optimization_passes = 0;      // عدد تمريرات التحسين
        } stats;

        // نظام التسجيل - استخدام global logger
    };

} // namespace ArabicLanguage

#endif
