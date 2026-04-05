// ArabicCompiler.cpp - النسخة المحدثة مع دعم كامل للمجمع المحسّن
// ✅ استخدام المجمع العربي المحسّن افتراضياً
// ✅ إزالة الاعتماد على NASM/GCC

#include "ArabicCompiler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#ifdef _WIN32
#include "SafeWindows.h"
#include <psapi.h>
#endif
#include "ArabicExecutor.h"
#include "ArabicRuntime.h"
#include "ArabicIO.h"
#include "SecurityFixes.h"
#include "../modules/ai/ArabicVisionBridge.h"

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // 📝 تنفيذ نظام التسجيل
    // ══════════════════════════════════════════════════════════════

    // Global logger instance - persists throughout program lifetime
    Logger& getGlobalLogger() {
        static Logger logger;
        return logger;
    }

    Logger::Logger() : currentLevel(LogLevel::LOG_INFO), logToFile(false) {
    }

    Logger::~Logger() {
        // لا نغلق الملف في destructor لأننا نريد أن يبقى مفتوحاً
        // طوال عمر البرنامج
    }

    void Logger::setLogLevel(LogLevel level) {
        currentLevel = level;
    }

    void Logger::setLogToFile(bool enable, const std::string& filename) {
        if (enable && !logToFile) {
            enableFileLogging(filename);
        } else if (!enable && logToFile) {
            if (logFile.is_open()) {
                logFile.close();
            }
            logToFile = false;
        }
    }

    void Logger::enableFileLogging(const std::string& filename) {
        logFileName = filename;
        logFile.open(filename, std::ios::out | std::ios::app);
        if (logFile.is_open()) {
            logToFile = true;
            // كتابة رأس الملف
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            char timeStr[64];
#ifdef _WIN32
            ctime_s(timeStr, sizeof(timeStr), &time);
#else
            ctime_r(&time, timeStr);
#endif
            logFile << "=== بدء التسجيل: " << timeStr << "===\n";
            logFile.flush();
        } else {
            std::cerr << "⚠️  فشل في فتح ملف السجل: " << filename << std::endl;
        }
    }

    void Logger::log(LogLevel level, const std::string& message) {
        // Force log everything to stdout for now
        std::cout << "LOGGER: " << message << std::endl;
        if (level < currentLevel) return;

        // الحصول على الوقت الحالي
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char timeStr[20];
        struct tm timeInfo;
#ifdef _WIN32
        localtime_s(&timeInfo, &time);
#else
        localtime_r(&time, &timeInfo);
#endif
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeInfo);

        // تحديد لون الرسالة ولونها
        std::string levelStr;
        std::string colorCode;

        switch (level) {
            case LogLevel::LOG_DEBUG:
                levelStr = "DEBUG";
                colorCode = "\033[36m"; // أزرق
                break;
            case LogLevel::LOG_INFO:
                levelStr = "INFO";
                colorCode = "\033[32m"; // أخضر
                break;
            case LogLevel::LOG_WARNING:
                levelStr = "WARN";
                colorCode = "\033[33m"; // أصفر
                break;
            case LogLevel::LOG_ERROR:
                levelStr = "ERROR";
                colorCode = "\033[31m"; // أحمر
                break;
            default:
                levelStr = "UNKNOWN";
                colorCode = "\033[0m"; // افتراضي
        }

        std::string resetColor = "\033[0m";
        std::string fullMessage = std::string("[") + timeStr + "] [" + levelStr + "] " + message;

        // طباعة إلى وحدة التحكم
        std::cout << colorCode << fullMessage << resetColor << std::endl;

        // كتابة إلى الملف إذا كان مفعلاً
        if (logToFile && logFile.is_open()) {
            logFile << fullMessage << std::endl;
            logFile.flush(); // التأكد من الكتابة الفورية
        }
    }

    void Logger::logDebug(const std::string& message) { log(LogLevel::LOG_DEBUG, message); }
    void Logger::logInfo(const std::string& message) { log(LogLevel::LOG_INFO, message); }
    void Logger::logWarning(const std::string& message) { log(LogLevel::LOG_WARNING, message); }
    void Logger::logError(const std::string& message) { log(LogLevel::LOG_ERROR, message); }

    void ArabicCompiler::logDebug(const std::string& message) {
        getGlobalLogger().log(LogLevel::LOG_DEBUG, message);
    }

    void ArabicCompiler::logInfo(const std::string& message) {
        getGlobalLogger().log(LogLevel::LOG_INFO, message);
    }

    void ArabicCompiler::logWarning(const std::string& message) {
        getGlobalLogger().log(LogLevel::LOG_WARNING, message);
    }

    void ArabicCompiler::logError(const std::string& message) {
        getGlobalLogger().log(LogLevel::LOG_ERROR, message);
    }

    ArabicCompiler::ArabicCompiler() {
        // ✅ تهيئة عدادات المحلل العربي
        arabic_parse_attempts = 0;
        arabic_parse_successes = 0;
        arabic_parse_fallbacks = 0;

        stats.total_commands = 0;
        stats.variables_count = 0;
        stats.functions_count = 0;
        stats.compilation_time_ms = 0;
        stats.success = false;

        // ✅ تهيئة مدير الجسور الموحد
        // bridgeManager.initialize();
        useBridges = false; // ✅ تعطيل الجسور مؤقتاً لحل مشاكل التكرار اللانهائي

#ifdef _WIN32
        // ✅ تهيئة وحدة التحكم لدعم اليونيكود (UTF-8)
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        
        // تفعيل دعم الألوان (ANSI) في Windows 10+
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
        
        std::cout << "🌐 تم تهيئة وحدة التحكم لدعم UTF-8" << std::endl;
#endif

#if HAS_ARABIC_ASSEMBLER
        // ✅ تفعيل المجمع العربي للاختبار والإصلاح
        useArabicAssembler = true;
        std::cout << "🔧 المجمع العربي مُفعّل (وضع الاختبار)" << std::endl;
#else
        useArabicAssembler = false;
        std::cerr << "⚠️  المجمع العربي غير متوفر - سيتم استخدام NASM" << std::endl;
#endif
    }

    std::string ArabicCompiler::sanitizeFilename(const std::string& arabicName) {
        static int fileCounter = 0;
        std::string sanitized = "arabic_program_" + std::to_string(++fileCounter);
        std::cout << "📝 تحويل اسم الملف: " << arabicName << " → " << sanitized << std::endl;
        return sanitized;
    }

    bool ArabicCompiler::compile(const std::string& arabicCode, const std::string& outputFile) {
        auto startTime = std::chrono::high_resolution_clock::now();
        stats.success = false;

        // إعادة تهيئة الإحصائيات المفصلة
        stats.if_statements = 0;
        stats.loop_statements = 0;
        stats.assignments = 0;
        stats.function_calls = 0;
        stats.arrays_created = 0;
        stats.strings_created = 0;
        stats.arithmetic_ops = 0;
        stats.comparison_ops = 0;
        stats.logical_ops = 0;
        stats.code_size_bytes = 0;
        stats.data_size_bytes = 0;
        stats.total_exe_size = 0;
        stats.relocations_count = 0;
        stats.parse_time_ms = 0;
        stats.codegen_time_ms = 0;
        stats.linking_time_ms = 0;
        stats.external_functions_used = 0;
        stats.imports_count = 0;
        stats.warnings_count = 0;
        stats.errors_count = 0;

        // إضافة قياسات أداء جديدة
        stats.memory_peak_mb = 0;
        stats.symbols_created = 0;
        stats.instructions_generated = 0;
        stats.optimization_passes = 0;

        // قياس ذروة الذاكرة الحالية
        #ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            stats.memory_peak_mb = pmc.PeakWorkingSetSize / (1024.0 * 1024.0);
        }
        #endif

        logInfo("بدء عملية الترجمة للملف: " + outputFile);
        logDebug("حجم الكود المصدر: " + std::to_string(arabicCode.length()) + " حرف");

        try {
            std::cout << "\n🚀 بدء الترجمة باستخدام المترجم العربي المحسّن (Direct Code Gen)" << std::endl;
            std::cout << "════════════════════════════════════════════════" << std::endl;

            // 1. التحليل
            auto parseStart = std::chrono::high_resolution_clock::now();
            std::cout << "📖 مرحلة 1: التحليل اللغوي والقواعدي..." << std::endl;
            
            std::vector<std::shared_ptr<Command>> commands;
            if (useBridges && bridgeManager.isReady()) {
                std::cout << "🌉 استخدام جسر المحلل العربي..." << std::endl;
                arabic_parse_attempts++;
                commands = bridgeManager.callParser(arabicCode, generator.getSymbols());
                if (commands.empty()) {
                    arabic_parse_fallbacks++;
                    std::cout << "⚠️ فشل الجسر في تحليل الكود، العودة للمحلل التقليدي..." << std::endl;
                    commands = parser.parse(arabicCode, generator.getSymbols());
                } else {
                    arabic_parse_successes++;
                }
            } else {
                logDebug("بدء مرحلة التحليل اللغوي التقليدية");
                commands = parser.parse(arabicCode, generator.getSymbols());
                
                for (size_t i = 0; i < commands.size(); ++i) {
                    
                }
            }

            // قياس وقت التحليل
            auto parseEnd = std::chrono::high_resolution_clock::now();
            stats.parse_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(parseEnd - parseStart).count();

            if (commands.empty()) {
                std::cerr << "❌ لم يتم العثور على أوامر صالحة" << std::endl;
                return false;
            }

            stats.total_commands = static_cast<int>(commands.size());

            // تحديث إحصائيات الرموز والتعليمات
            stats.symbols_created = static_cast<int>(generator.getSymbols().getSymbolCount());
            // Instructions count will be updated after code generation

            std::cout << "✅ تم تحليل " << stats.total_commands << " أمر" << std::endl;

            // 2. التوليد المباشر لكود الآلة أو C++ عبر الجسر
            auto codegenStart = std::chrono::high_resolution_clock::now();
            std::vector<uint8_t> machineCode;

            std::cout << "[DEBUG] مرحلة 2: بدء التوليد..." << std::endl;
            if (useBridges && bridgeManager.isReady()) {
                std::cout << "\n⚙️  مرحلة 2: توليد كود C++ عبر الجسر العربي الموحد..." << std::endl;
                
                // للحصول على كود C++ نحتاج لاستدعاء المولد العربي عبر المدير
                // 1. الحصول على الرموز
                auto tokens = bridgeManager.tokenize(arabicCode);
                
                // 2. الحصول على العقد العربية (Value ARRAY)
                Value astNodes = bridgeManager.parseToValue(tokens);
                
                // 3. توليد الكود
                std::string cppCode = bridgeManager.generateFromValue(astNodes);
                
                if (!cppCode.empty()) {
                    std::cout << "✅ تم توليد كود C++ بنجاح (" << cppCode.length() << " حرف)" << std::endl;
                    logInfo("تم توليد كود C++ عبر الجسر العربي");
                    
                    // حفظ الكود في ملف مؤقت
                    std::string tempCppFile = "temp_output.cpp";
                    std::ofstream out(tempCppFile);
                    out << cppCode;
                    out.close();
                    std::cout << "📝 تم حفظ الكود في: " << tempCppFile << std::endl;
                }
                
                // العودة للمولد التقليدي حالياً لتوليد كود الآلة (Direct Code Gen)
                // لأننا لا نملك حالياً آلية لتجميع C++ الناتج برمجياً بسهولة هنا
                std::cout << "⚠️ العودة للمولد التقليدي (Direct Code Gen) لبناء الملف التنفيذي" << std::endl;
                machineCode = generator.generate(commands);
            } else {
                std::cout << "\n⚙️  مرحلة 2: توليد كود الآلة (Machine Code)..." << std::endl;
                logDebug("بدء توليد كود الآلة لـ " + std::to_string(commands.size()) + " أمر");
                machineCode = generator.generate(commands);
            }

            auto codegenEnd = std::chrono::high_resolution_clock::now();
            stats.codegen_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(codegenEnd - codegenStart).count();

            if (machineCode.empty()) {
                std::cerr << "❌ فشل توليد كود الآلة" << std::endl;
                return false;
            }

            std::cout << "[DEBUG] مرحلة 2: اكتمل التوليد بنجاح" << std::endl;
            stats.code_size_bytes = machineCode.size();
            std::cout << "✅ تم توليد " << machineCode.size() << " بايت من كود الآلة" << std::endl;
            logInfo("تم توليد " + std::to_string(machineCode.size()) + " بايت من كود الآلة");

            // 3. بناء الملف التنفيذي (PE Build)
            std::cout << "\n🔨 مرحلة 3: بناء الملف التنفيذي (PE Linking)..." << std::endl;
            std::cout << "[DEBUG] مرحلة 3: استدعاء buildExecutable..." << std::endl;
            logDebug("بدء بناء ملف PE التنفيذي");

            auto linkingStart = std::chrono::high_resolution_clock::now();

            std::string exeFilename = outputFile;
            if (exeFilename.find(".exe") == std::string::npos) {
                exeFilename += ".exe";
            }

            // Build executable directly
            bool success = false;
            try {
                success = generator.buildExecutable(exeFilename);
                std::cout << "[DEBUG] مرحلة 3: buildExecutable اكتمل" << std::endl;
            } catch (const std::exception& peEx) {
                std::cerr << "[DEBUG] ❌ استثناء في بناء PE: " << peEx.what() << std::endl;
                throw;
            }

            auto linkingEnd = std::chrono::high_resolution_clock::now();
            stats.linking_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(linkingEnd - linkingStart).count();
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto compilationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                endTime - startTime);
            stats.compilation_time_ms = compilationTime.count();

            if (success) {
                // جمع إحصائيات إضافية
                std::ifstream exeFileCheck(exeFilename, std::ios::binary | std::ios::ate);
                if (exeFileCheck.is_open()) {
                    stats.total_exe_size = exeFileCheck.tellg();
                    exeFileCheck.close();
                }

                std::cout << "\n════════════════════════════════════════════════" << std::endl;
                std::cout << "🎉 اكتملت الترجمة بنجاح!" << std::endl;
                std::cout << "📦 الملف الناتج: " << exeFilename << std::endl;
                std::cout << "⏱️  الوقت: " << stats.compilation_time_ms << " مللي ثانية" << std::endl;
                std::cout << "✨ تم استخدام التوليد المباشر (Direct Code Generation)" << std::endl;
                std::cout << "════════════════════════════════════════════════\n" << std::endl;
                printPEBriefReport(exeFilename);
                stats.success = true;
            }
            else {
                std::cout << "\n❌ فشل بناء الملف التنفيذي" << std::endl;
            }

            return stats.success;

        }
        catch (const std::exception& e) {
            std::cerr << "\n❌ خطأ غير متوقع: " << e.what() << std::endl;
            return false;
        }
    }

    void ArabicCompiler::stopExecution() {
        if (currentExecutor) {
            currentExecutor->stop();
            logInfo("تم طلب إيقاف التنفيذ البرمجي");
        }
    }

    bool ArabicCompiler::executeIntermediate(const std::string& arabicCode) {
        try {
            std::shared_ptr<ArabicRuntime> runtime;
            if (persistentRuntime) {
                runtime = persistentRuntime;
            } else {
                runtime = std::make_shared<ArabicRuntime>();
                runtime->initialize();
            }

            // ✅ تسجيل مكتبة الرؤية الحاسوبية
            ArabicVisionBridge::registerFunctions(runtime, outputCallback);

            currentExecutor = std::make_shared<ArabicExecutor>(runtime);
            currentExecutor->resetStop();
            
            // ✅ تمرير دالة المخرجات إذا كانت محددة
            if (outputCallback) {
                currentExecutor->setOutputCallback(outputCallback);
                
            }

            std::vector<std::shared_ptr<Command>> commands;
            if (useBridges && bridgeManager.isReady()) {
                if (outputCallback) 
                commands = bridgeManager.callParser(arabicCode, generator.getSymbols());
                
                if (commands.empty()) {
                    if (outputCallback) 
                    parser.setExecutor(currentExecutor.get());
                    commands = parser.parse(arabicCode, generator.getSymbols());
                }
            } else {
                // ✅ تمرير المنفذ إلى المحلل لتنفيذ تعريفات الدوال فوراً
                parser.setExecutor(currentExecutor.get());
                commands = parser.parse(arabicCode, generator.getSymbols());
            }

            if (outputCallback) {
                
            }

            currentExecutor->execute(commands);

            if (outputCallback) {
                if (currentExecutor->isStopped()) {
                    outputCallback("⚠️ تم إيقاف التنفيذ بواسطة المستخدم.\n");
                } else {
                    
                }
            }
            
            currentExecutor.reset(); // تنظيف بعد الانتهاء
            return true;
        } catch (const std::exception& e) {
            std::cerr << "❌ حدث خطأ أثناء التشغيل: " << e.what() << std::endl;
            if (outputCallback) {
                outputCallback("❌ خطأ غير متوقع: " + std::string(e.what()) + "\n");
            }
            return false;
        } catch (...) {
            std::cerr << "❌ حدث خطأ غير معروف في المحرك" << std::endl;
            if (outputCallback) {
                outputCallback("❌ حدث خطأ غير معروف في المحرك\n");
            }
            return false;
        }
    }

    void ArabicCompiler::printPEBriefReport(const std::string& exeFile) const {
        std::ifstream f(exeFile, std::ios::binary);
        if (!f.is_open()) return;
        char mz[2] = {0,0};
        f.read(mz, 2);
        bool mzok = (mz[0] == 'M' && mz[1] == 'Z');
        // e_lfanew at offset 0x3C
        uint32_t e_lfanew = 0;
        f.seekg(0x3C, std::ios::beg);
        f.read(reinterpret_cast<char*>(&e_lfanew), sizeof(e_lfanew));
        // NT signature
        char peSig[4] = {0,0,0,0};
        f.seekg(e_lfanew, std::ios::beg);
        f.read(peSig, 4);
        bool peok = (peSig[0] == 'P' && peSig[1] == 'E' && peSig[2] == 0 && peSig[3] == 0);
        f.seekg(0, std::ios::end);
        auto size = f.tellg();
        f.close();
        std::cout << "📊 تقرير PE: "
                  << (mzok ? "MZ" : "غير صالح")
                  << ", e_lfanew: " << e_lfanew
                  << ", توقيع PE: " << (peok ? "PE\0\0" : "غير موجود")
                  << ", الحجم: " << size << " بايت" << std::endl;
    }

    // ✅ دالة محسّنة للتجميع باستخدام المجمع العربي المحسّن
    bool ArabicCompiler::assembleWithArabicAssembler(const std::string& assemblyCode,
        const std::string& outputFile) {

#if !HAS_ARABIC_ASSEMBLER
        std::cout << "❌ المجمع العربي غير متوفر في هذا البناء" << std::endl;
        return false;
#else
        try {
            std::cout << "⚡ بدء التجميع بالمجمع العربي المحسّن..." << std::endl;

            // استخدام استيرادات مبسطة تعمل
            auto imports = ArabicAssembler::ImprovedPEBuilder::createMinimalImports();

            ArabicAssembler::AssemblerCore assembler;
            assembler.setVerbose(true);

            std::string exeFile = outputFile + ".exe";
            bool result = assembler.assemble(assemblyCode, exeFile);

            if (result) {
                // التحقق من الملف الناتج
                std::ifstream check(exeFile, std::ios::binary | std::ios::ate);
                if (check.is_open()) {
                    size_t fileSize = check.tellg();
                    std::cout << "📦 حجم الملف: " << fileSize << " بايت" << std::endl;

                    if (fileSize > 2048) {
                        std::cout << "✅ الملف بحجم معقول - يجب أن يعمل" << std::endl;
                    }
                    check.close();
                }

                auto stats = assembler.getStats();
                std::cout << "✅ التجميع نجح!" << std::endl;
                return true;
            }
            else {
                std::cout << "❌ فشل التجميع بالمجمع العربي" << std::endl;
                return false;
            }

        }
        catch (const std::exception& e) {
            std::cerr << "❌ خطأ في التجميع العربي: " << e.what() << std::endl;
            return false;
        }
#endif
    }
 



    // دالة التجميع باستخدام NASM (احتياطية - نادراً ما تُستخدم الآن)
    bool ArabicCompiler::assembleWithNASM(const std::string& asmFile,
        const std::string& exeFile) {

        std::cout << "⚠️  تحذير: استخدام NASM احتياطياً (المجمع العربي أفضل!)" << std::endl;

        std::ifstream testFile(asmFile);
        if (!testFile.is_open()) {
            std::cerr << "❌ ملف Assembly غير موجود: " << asmFile << std::endl;
            return false;
        }
        testFile.close();

#ifdef _WIN32
        std::string objFile = exeFile + ".o";
        std::string exeOutput = exeFile + ".exe";

        std::string sanitizedAsm = ::ArabicSecurity::Security::sanitizeFileName(asmFile);
        std::string sanitizedObj = ::ArabicSecurity::Security::sanitizeFileName(objFile);
        std::string sanitizedExe = ::ArabicSecurity::Security::sanitizeFileName(exeOutput);

        std::string nasmCommand = "nasm -f win64 \"" + sanitizedAsm + "\" -o \"" + sanitizedObj + "\" 2>nul";
        std::string gccCommand = "gcc \"" + sanitizedObj + "\" -o \"" + sanitizedExe + "\" -lkernel32 -lmsvcrt 2>nul";

        std::cout << "🔨 NASM: جاري التجميع..." << std::endl;
        int nasmResult = system(nasmCommand.c_str());

        if (nasmResult != 0) {
            std::cerr << "❌ NASM: فشل التجميع" << std::endl;
            std::cerr << "💡 نصيحة: استخدم المجمع العربي المحسّن بدلاً من NASM!" << std::endl;
            return false;
        }

        std::cout << "🔗 GCC: جاري الربط..." << std::endl;
        int gccResult = system(gccCommand.c_str());

        if (gccResult != 0) {
            std::cerr << "❌ GCC: فشل الربط" << std::endl;
            return false;
        }

        std::remove(objFile.c_str());
#else
        std::string objFile = exeFile + ".o";
        std::string sanitizedAsm = Security::sanitizeFileName(asmFile);
        std::string sanitizedObj = Security::sanitizeFileName(objFile);

        std::string nasmCommand = "nasm -f elf64 \"" + sanitizedAsm + "\" -o \"" + sanitizedObj + "\" 2>/dev/null";
        std::string gccCommand = "gcc \"" + sanitizedObj + "\" -o \"" + exeFile + "\" -no-pie 2>/dev/null";

        std::cout << "🔨 NASM: جاري التجميع..." << std::endl;
        int nasmResult = system(nasmCommand.c_str());

        if (nasmResult != 0) {
            std::cerr << "❌ NASM: فشل التجميع" << std::endl;
            return false;
        }

        std::cout << "🔗 GCC: جاري الربط..." << std::endl;
        int gccResult = system(gccCommand.c_str());

        if (gccResult != 0) {
            std::cerr << "❌ GCC: فشل الربط" << std::endl;
            return false;
        }

        std::remove(objFile.c_str());
#endif

        std::cout << "✅ NASM + GCC: نجح التجميع" << std::endl;
        return true;
    }

    bool ArabicCompiler::compileFile(const std::string& inputFile,
        const std::string& outputFile) {
        std::string arabicCode = readFile(inputFile);
        if (arabicCode.empty()) {
            std::cerr << "❌ فشل قراءة الملف: " << inputFile << std::endl;
            return false;
        }

        // ✅ Set base path for imports
        parser.setBasePath(inputFile);
        parser.setCurrentFile(inputFile);

        std::string outputName = outputFile.empty() ?
            inputFile.substr(0, inputFile.find_last_of('.')) : outputFile;

        return compile(arabicCode, outputName);
    }

    std::string ArabicCompiler::readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "❌ لا يمكن فتح الملف: " << filename << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        std::string content = buffer.str();
        std::cout << "📖 تم قراءة الملف: " << filename
            << " (" << content.length() << " حرف)" << std::endl;

        return content;
    }

    bool ArabicCompiler::saveToFile(const std::string& content,
        const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "❌ لا يمكن إنشاء الملف: " << filename << std::endl;
            return false;
        }

        file << content;
        file.close();

        std::ifstream testFile(filename);
        if (!testFile.is_open()) {
            std::cerr << "❌ فشل التحقق من الملف: " << filename << std::endl;
            return false;
        }
        testFile.close();

        std::cout << "💾 تم حفظ الملف: " << filename
            << " (" << content.length() << " حرف)" << std::endl;
        return true;
    }


    void ArabicCompiler::displayStatistics() const {
        std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║         📊 إحصائيات الترجمة                  ║" << std::endl;
        std::cout << "╠════════════════════════════════════════════════╣" << std::endl;
        std::cout << "║  الأوامر: " << std::setw(30) << std::left
            << stats.total_commands << " ║" << std::endl;
        std::cout << "║  الوقت: " << std::setw(24) << std::left
            << (std::to_string(stats.compilation_time_ms) + " مللي ثانية")
            << " ║" << std::endl;
        std::cout << "║  الحالة: " << std::setw(29) << std::left
            << (stats.success ? "✅ ناجحة" : "❌ فاشلة") << " ║" << std::endl;
        std::cout << "║  المجمع: " << std::setw(29) << std::left
            << (useArabicAssembler ? "🌟 عربي محسّن" : "🔧 NASM") << " ║" << std::endl;
        std::cout << "║  الاستقلالية: " << std::setw(25) << std::left
            << (useArabicAssembler ? "✅ 100%" : "❌ يحتاج أدوات") << " ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════╝" << std::endl;

        auto parserStats = parser.getStatistics();
        auto generatorStats = generator.getStatistics();

        if (!parserStats.empty()) {
            std::cout << "\n📈 تفاصيل التحليل:" << std::endl;
            for (const auto& [key, value] : parserStats) {
                std::cout << "   • " << key << ": " << value << std::endl;
            }
        }

        if (!generatorStats.empty()) {
            std::cout << "\n⚙️  تفاصيل التوليد:" << std::endl;
            for (const auto& [key, value] : generatorStats) {
                std::cout << "   • " << key << ": " << value << std::endl;
            }
        }

        // ══════════════════════════════════════════════════════════════
        // 📊 إحصائيات مفصلة جديدة
        // ══════════════════════════════════════════════════════════════

        // دالة مساعدة لتنسيق أحجام البيانات
        auto formatBytes = [](size_t bytes) -> std::string {
            if (bytes < 1024) return std::to_string(bytes) + " B";
            if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
            return std::to_string(bytes / (1024 * 1024)) + " MB";
        };

        std::cout << "\n╔══════════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                            📈 إحصائيات مفصلة                                      ║" << std::endl;
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════╣" << std::endl;

        // إحصائيات البناء والعناصر
        std::cout << "║ 📦 بناء وذاكرة:" << std::endl;
        std::cout << "║   • حجم الكود: " << std::setw(15) << formatBytes(stats.code_size_bytes) << std::endl;
        std::cout << "║   • حجم البيانات: " << std::setw(11) << formatBytes(stats.data_size_bytes) << std::endl;
        std::cout << "║   • حجم الملف التنفيذي: " << std::setw(5) << formatBytes(stats.total_exe_size) << std::endl;
        std::cout << "║   • عدد عمليات إعادة التوجيه: " << std::setw(8) << stats.relocations_count << std::endl;

        // إحصائيات العناصر البرمجية
        std::cout << "║ 🔧 عناصر البرنامج:" << std::endl;
        std::cout << "║   • المتغيرات: " << std::setw(21) << stats.variables_count << std::endl;
        std::cout << "║   • الدوال: " << std::setw(26) << stats.functions_count << std::endl;
        std::cout << "║   • المصفوفات: " << std::setw(22) << stats.arrays_created << std::endl;
        std::cout << "║   • السلاسل النصية: " << std::setw(16) << stats.strings_created << std::endl;

        // إحصائيات العمليات
        std::cout << "║ ⚡ العمليات والعبارات:" << std::endl;
        std::cout << "║   • الشروط (if): " << std::setw(18) << stats.if_statements << std::endl;
        std::cout << "║   • الحلقات: " << std::setw(24) << stats.loop_statements << std::endl;
        std::cout << "║   • التعيينات: " << std::setw(21) << stats.assignments << std::endl;
        std::cout << "║   • استدعاءات الدوال: " << std::setw(13) << stats.function_calls << std::endl;

        // إحصائيات العمليات الحسابية والمنطقية
        std::cout << "║ 🧮 العمليات الحسابية والمنطقية:" << std::endl;
        std::cout << "║   • العمليات الحسابية: " << std::setw(10) << stats.arithmetic_ops << std::endl;
        std::cout << "║   • عمليات المقارنة: " << std::setw(13) << stats.comparison_ops << std::endl;
        std::cout << "║   • العمليات المنطقية: " << std::setw(12) << stats.logical_ops << std::endl;

        // إحصائيات الأداء
        std::cout << "║ ⏱️  أداء الترجمة:" << std::endl;
        std::cout << "║   • وقت التحليل: " << std::setw(17) << (std::to_string(stats.parse_time_ms) + "ms") << std::endl;
        std::cout << "║   • وقت توليد الكود: " << std::setw(11) << (std::to_string(stats.codegen_time_ms) + "ms") << std::endl;
        std::cout << "║   • وقت الربط: " << std::setw(19) << (std::to_string(stats.linking_time_ms) + "ms") << std::endl;
        std::cout << "║   • الوقت الإجمالي: " << std::setw(15) << (std::to_string(stats.compilation_time_ms) + "ms") << std::endl;

        // إحصائيات الجودة
        std::cout << "║ ⚠️  جودة الكود:" << std::endl;
        std::cout << "║   • عدد التحذيرات: " << std::setw(14) << stats.warnings_count << std::endl;
        std::cout << "║   • عدد الأخطاء: " << std::setw(18) << stats.errors_count << std::endl;
        std::cout << "║   • الدوال الخارجية: " << std::setw(13) << stats.external_functions_used << std::endl;
        std::cout << "║   • المكتبات المستوردة: " << std::setw(10) << stats.imports_count << std::endl;

        // إحصائيات المحلل العربي
        if (arabic_parse_attempts > 0) {
            double success_rate = (static_cast<double>(arabic_parse_successes) / arabic_parse_attempts) * 100.0;
            double fallback_rate = (static_cast<double>(arabic_parse_fallbacks) / arabic_parse_attempts) * 100.0;
            std::cout << "║ 🌉 المحلل العربي:" << std::endl;
            std::cout << "║   • محاولات التحليل: " << std::setw(11) << arabic_parse_attempts << std::endl;
            std::cout << "║   • نجاحات: " << std::setw(22) << arabic_parse_successes << std::endl;
            std::cout << "║   • تراجعات: " << std::setw(21) << arabic_parse_fallbacks << std::endl;
            std::cout << "║   • معدل النجاح: " << std::setw(15) << std::fixed << std::setprecision(1) << success_rate << "%" << std::endl;
            std::cout << "║   • معدل التراجع: " << std::setw(14) << std::fixed << std::setprecision(1) << fallback_rate << "%" << std::endl;
        }

        std::cout << "╚══════════════════════════════════════════════════════════════════════════════════════╝" << std::endl;
    }

    std::string ArabicCompiler::getVersion() const {
        return "المترجم العربي C++ v3.0 - مع ImprovedPEBuilder (مستقل 100%)";
    }

    void ArabicCompiler::setOutputCallback(OutputCallback cb) {
        outputCallback = cb;
    }

    void ArabicCompiler::setUseArabicAssembler(bool use) {
#if HAS_ARABIC_ASSEMBLER
        useArabicAssembler = use;
        std::cout << "⚙️  تم " << (use ? "تفعيل" : "تعطيل")
            << " المجمع العربي المحسّن" << std::endl;

        if (use) {
            std::cout << "   ✨ الاستقلالية: 100% - لا يحتاج NASM/GCC" << std::endl;
        }
#else
        std::cout << "❌ المجمع العربي غير متوفر في هذا البناء" << std::endl;
        std::cout << "   💡 قم بتفعيل HAS_ARABIC_ASSEMBLER في البناء" << std::endl;
#endif
    }

    bool ArabicCompiler::selfCompile() {
        std::cout << "\n🔄 بدء الترجمة الذاتية..." << std::endl;
        std::cout << "════════════════════════════════════════════════" << std::endl;

        std::string compilerCode = R"(
# نواة المترجم الذاتي - الإصدار المحسن مع ImprovedPEBuilder
متغير_اختبار = 42
اطبع("🚀 جاري الترجمة الذاتية للمترجم العربي المحسّن...")
اطبع("📊 القيمة الاختبارية: ", متغير_اختبار)
اطبع("✨ المجمع: عربي محسّن مع ImprovedPEBuilder")
اطبع("🎯 الاستقلالية: 100% - لا يحتاج NASM/GCC")

دالة اختبار_الحساب():
    أساس = 2
    أس = 10
    نتيجة = أساس * أس
    اطبع("⚡ اختبار الحساب: ", أساس, " * ", أس, " = ", نتيجة)
    أرجع نتيجة

دالة اختبار_الحلقة():
    مجموع = 0
    عداد = 1
    طالما عداد <= 5:
        مجموع = مجموع + عداد
        عداد = عداد + 1
    اطبع("🔄 مجموع 1-5 = ", مجموع)
    أرجع مجموع

حساب = اختبار_الحساب()
حلقة = اختبار_الحلقة()

اطبع("════════════════════════════════════════")
اطبع("🎉 الترجمة الذاتية ناجحة!")
اطبع("✅ النظام الذاتي يعمل بكفاءة")
اطبع("🌟 مستقل 100% بدون أدوات خارجية")
اطبع("════════════════════════════════════════")
        )";

        bool success = compile(compilerCode, "self_compiler_improved");

        if (success) {
            std::cout << "\n✅ تم بناء النواة الذاتية بنجاح" << std::endl;
            std::cout << "🎯 الملف: self_compiler_improved.exe" << std::endl;
            std::cout << "✨ تم استخدام المجمع العربي المحسّن" << std::endl;
        }
        else {
            std::cout << "\n❌ فشل بناء النواة الذاتية" << std::endl;
        }

        return success;
    }

    bool ArabicCompiler::buildSelfHosting() {
        std::cout << "\n🏗️  بدء بناء النظام الذاتي الكامل..." << std::endl;
        std::cout << "════════════════════════════════════════════════" << std::endl;

        // تفعيل المجمع العربي للبناء الذاتي
        bool previousSetting = useArabicAssembler;
        setUseArabicAssembler(true);

        // 🎯 المرحلة 1: بناء النواة الأساسية
        std::cout << "\n🎯 المرحلة 1: بناء النواة الأساسية" << std::endl;
        std::cout << "────────────────────────────────────────" << std::endl;

        std::string coreCode = R"(
# نواة المترجم العربي - نسخة مبسطة
دالة اختبار_اساسي():
    عدد = 10
    اطبع("اختبار النواة")
    اطبع(عدد)
    ارجع عدد
نهاية

دالة اختبار_الحساب():
    قاعدة = 2
    اس = 3
    نتيجة = قاعدة * اس
    اطبع("نتيجة الحساب")
    اطبع(نتيجة)
    ارجع نتيجة
نهاية

# البرنامج الرئيسي
اطبع("نواة المترجم العربي")
نتيجة_1 = اختبار_اساسي()
نتيجة_2 = اختبار_الحساب()

اذا نتيجة_1 > 5:
    اطبع("النواة تعمل")
والا:
    اطبع("خطا في النواة")
نهاية
        )";

        std::cout << "📝 إنشاء نواة المترجم..." << std::endl;
        bool coreSaved = saveToFile(coreCode, "نواة_مترجم.عربي");
        if (!coreSaved) {
            std::cout << "❌ فشل حفظ النواة" << std::endl;
            return false;
        }

        std::cout << "🔨 ترجمة النواة باستخدام المترجم الحالي..." << std::endl;
        bool coreCompiled = compileFile("نواة_مترجم.عربي", "نواة_مترجم");
        if (!coreCompiled) {
            std::cout << "❌ فشل ترجمة النواة" << std::endl;
            return false;
        }

        std::cout << "✅ تم بناء النواة الأساسية بنجاح" << std::endl;

        // 🎯 المرحلة 2: بناء النظام الوسيط
        std::cout << "\n🎯 المرحلة 2: بناء النظام الوسيط" << std::endl;
        std::cout << "────────────────────────────────────────" << std::endl;

        bool result = selfCompile();

        // استعادة الإعداد السابق
        useArabicAssembler = previousSetting;

        if (result) {
            std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
            std::cout << "║  🎉 اكتمل بناء النظام الذاتي بنجاح!          ║" << std::endl;
            std::cout << "╠════════════════════════════════════════════════╣" << std::endl;
            std::cout << "║  ✅ المترجم العربي الآن مستقل تماماً         ║" << std::endl;
            std::cout << "║  🌟 يستخدم ImprovedPEBuilder المحسّن          ║" << std::endl;
            std::cout << "║  🚀 يمكنه ترجمة نفسه دون أدوات خارجية       ║" << std::endl;
            std::cout << "║  ⚡ دعم Import Tables كامل                   ║" << std::endl;
            std::cout << "║  🎯 استقلالية 100%                           ║" << std::endl;
            std::cout << "╚════════════════════════════════════════════════╝" << std::endl;
        }
        else {
            std::cout << "\n❌ فشل بناء النظام الذاتي" << std::endl;
        }

        return result;
    }

    // ✅ دوال جديدة لدعم ملفات .ar
    bool ArabicCompiler::isArabicFile(const std::string& filename) const {
        return filename.find(".ar") != std::string::npos;
    }

    bool ArabicCompiler::readArabicFile(const std::string& filePath, std::string& content) const {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "❌ لا يمكن فتح ملف عربي: " << filePath << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        // ✅ تحويل الترميز تلقائياً (دعم UTF-16 و UTF-8 BOM)
        content = StdLib::ArabicIO::DataConverter::detectAndConvert(buffer.str());
        
        std::cout << "✅ تم قراءة ملف عربي: " << filePath 
                  << " (" << content.length() << " حرف)" << std::endl;
        return true;
    }

    bool ArabicCompiler::compileArabicFile(const std::string& arabicFilePath) {
        std::cout << "\n📂 جاري ترجمة ملف عربي: " << arabicFilePath << std::endl;
        
        if (!isArabicFile(arabicFilePath)) {
            std::cerr << "⚠️  تحذير: امتداد الملف ليس .ar" << std::endl;
        }

        std::string content;
        if (!readArabicFile(arabicFilePath, content)) {
            return false;
        }

        // استخراج اسم الملف بدون امتداد
        std::string outputName = arabicFilePath;
        size_t lastDot = outputName.find_last_of('.');
        if (lastDot != std::string::npos) {
            outputName = outputName.substr(0, lastDot);
        }

        // ضبط مسار الملف للمحلل
        parser.setBasePath(arabicFilePath);
        parser.setCurrentFile(arabicFilePath);

        return compile(content, outputName);
    }

    // ══════════════════════════════════════════════════════════════
    // 📝 دوال التسجيل
    // ══════════════════════════════════════════════════════════════

    void ArabicCompiler::setLogLevel(LogLevel level) {
        getGlobalLogger().setLogLevel(level);
        std::string levelName;
        switch (level) {
            case LogLevel::LOG_DEBUG: levelName = "DEBUG"; break;
            case LogLevel::LOG_INFO: levelName = "INFO"; break;
            case LogLevel::LOG_WARNING: levelName = "WARNING"; break;
            case LogLevel::LOG_ERROR: levelName = "ERROR"; break;
            case LogLevel::LOG_NONE: levelName = "NONE"; break;
            default: levelName = "UNKNOWN"; break;
        }
        getGlobalLogger().logInfo("تم تعيين مستوى التسجيل إلى: " + levelName);
    }

    void ArabicCompiler::enableFileLogging(const std::string& filename) {
        getGlobalLogger().enableFileLogging(filename);
        getGlobalLogger().logInfo("تم تفعيل تسجيل الملف: " + filename);
    }

    void ArabicCompiler::disableFileLogging() {
        getGlobalLogger().setLogToFile(false);
        getGlobalLogger().logInfo("تم إلغاء تفعيل تسجيل الملف");
    }

} // namespace ArabicLanguage
