#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "SafeWindows.h"
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

#include "ArabicRuntime.h"
#include "ArabicIO.h"
#include "ArabicTextUtils.h"
#include <unordered_map>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <mutex>
#include <algorithm>
#include <cctype>

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // 🧠 نظام إدارة الذاكرة المحسن
    // ══════════════════════════════════════════════════════════════

    class MemoryManager {
    public:
        enum class DataType {
            UNKNOWN,
            STRING,
            ARRAY,
            OBJECT,
            BUFFER
        };

        struct MemoryBlock {
            void* address;
            size_t size;
            std::string allocationLocation;
            std::chrono::system_clock::time_point allocationTime;
            bool isFreed;
            DataType dataType;
            size_t referenceCount;  // للجامع القمامة البسيط
        };

        std::unordered_map<void*, MemoryBlock> allocatedBlocks;
        size_t totalAllocated;
        size_t totalFreed;
        size_t peakMemoryUsage;
        bool enableTracking;
        bool garbageCollectionEnabled;
        std::chrono::milliseconds gcInterval;  // فترة تشغيل الجامع القمامة
        mutable std::mutex memoryMutex;

    public:
        MemoryManager() : totalAllocated(0), totalFreed(0), peakMemoryUsage(0),
                         enableTracking(true), garbageCollectionEnabled(false),
                         gcInterval(5000) {}  // 5 ثوان

        void setTrackingEnabled(bool enabled) { enableTracking = enabled; }
        void setGarbageCollectionEnabled(bool enabled) { garbageCollectionEnabled = enabled; }
        void setGCInterval(std::chrono::milliseconds interval) { gcInterval = interval; }

        void* allocate(size_t size, const std::string& location = "unknown", DataType type = DataType::UNKNOWN) {
            std::lock_guard<std::mutex> lock(memoryMutex);
            void* ptr = malloc(size);
            if (!ptr) {
                throw std::runtime_error("❌ فشل تخصيص الذاكرة - حجم: " + std::to_string(size));
            }

            if (enableTracking) {
                MemoryBlock block = {ptr, size, location,
                                   std::chrono::system_clock::now(), false, type, 1};
                allocatedBlocks[ptr] = block;
                totalAllocated += size;
                if (totalAllocated - totalFreed > peakMemoryUsage) {
                    peakMemoryUsage = totalAllocated - totalFreed;
                }

                // تشغيل الجامع القمامة إذا لزم الأمر
                if (garbageCollectionEnabled) {
                    checkGarbageCollection();
                }
            }

            return ptr;
        }

        // زيادة عدد المراجع (لمنع التحرير المبكر)
        void incrementRef(void* ptr) {
            if (!enableTracking) return;
            auto it = allocatedBlocks.find(ptr);
            if (it != allocatedBlocks.end() && !it->second.isFreed) {
                it->second.referenceCount++;
            }
        }

        // تقليل عدد المراجع
        void decrementRef(void* ptr) {
            if (!enableTracking) return;
            auto it = allocatedBlocks.find(ptr);
            if (it != allocatedBlocks.end() && !it->second.isFreed) {
                it->second.referenceCount--;
                if (it->second.referenceCount <= 0 && garbageCollectionEnabled) {
                    // يمكن تحرير هذا الكائن
                    deallocate(ptr);
                }
            }
        }

        // جامع قمامة بسيط - يتحقق من الكتل القديمة جداً
        void runGarbageCollection() {
            if (!enableTracking || !garbageCollectionEnabled) return;

            auto now = std::chrono::system_clock::now();
            size_t collected = 0;

            for (auto it = allocatedBlocks.begin(); it != allocatedBlocks.end(); ) {
                if (!it->second.isFreed &&
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - it->second.allocationTime) > std::chrono::minutes(5) &&
                    it->second.referenceCount == 0) {

                    free(it->first);
                    totalFreed += it->second.size;
                    it = allocatedBlocks.erase(it);
                    collected++;
                } else {
                    ++it;
                }
            }

            if (collected > 0) {
                std::cout << "🗑️  جامع القمامة: تم تحرير " << collected << " كتل ذاكرة" << std::endl;
            }
        }

        // فحص دوري لتشغيل الجامع القمامة
        void checkGarbageCollection() {
            static auto lastGC = std::chrono::system_clock::now();

            auto now = std::chrono::system_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastGC) > gcInterval) {
                runGarbageCollection();
                lastGC = now;
            }
        }

        void deallocate(void* ptr) {
            if (!ptr) return;
            std::lock_guard<std::mutex> lock(memoryMutex);

            if (enableTracking) {
                auto it = allocatedBlocks.find(ptr);
                if (it != allocatedBlocks.end() && !it->second.isFreed) {
                    totalFreed += it->second.size;
                    it->second.isFreed = true;
                } else if (it == allocatedBlocks.end()) {
                    std::cerr << "⚠️  تحرير ذاكرة غير مُتتعة: " << ptr << std::endl;
                } else if (it->second.isFreed) {
                    std::cerr << "⚠️  تحرير ذاكرة مُحررة مسبقاً: " << ptr << std::endl;
                }
            }

            free(ptr);
        }

        // التحقق من صحة المؤشر
        bool isValidPointer(void* ptr) {
            if (!enableTracking) return true;
            auto it = allocatedBlocks.find(ptr);
            return it != allocatedBlocks.end() && !it->second.isFreed;
        }

        // الحصول على معلومات عن كتلة ذاكرة
        std::string getMemoryInfo(void* ptr) {
            if (!enableTracking) return "تتبع الذاكرة معطل";

            auto it = allocatedBlocks.find(ptr);
            if (it == allocatedBlocks.end()) {
                return "مؤشر غير مُتتبع";
            }

            const auto& block = it->second;
            std::stringstream ss;
            ss << "حجم: " << block.size << " بايت, موقع: " << block.allocationLocation;
            ss << ", نوع: " << getDataTypeName(block.dataType);
            ss << ", مراجع: " << block.referenceCount;
            ss << ", محرر: " << (block.isFreed ? "نعم" : "لا");
            return ss.str();
        }

        std::string getDataTypeName(DataType type) {
            switch (type) {
                case DataType::STRING: return "سلسلة نصية";
                case DataType::ARRAY: return "مصفوفة";
                case DataType::OBJECT: return "كائن";
                case DataType::BUFFER: return "مخزن مؤقت";
                default: return "غير معروف";
            }
        }

        void reportMemoryLeaks() {
            if (!enableTracking) {
                std::cout << "📊 تتبع الذاكرة معطل" << std::endl;
                return;
            }

            std::cout << "\n╔══════════════════════════════════════════════════════════════════════════════╗" << std::endl;
            std::cout << "║                           📊 تقرير إدارة الذاكرة                             ║" << std::endl;
            std::cout << "╠══════════════════════════════════════════════════════════════════════════════╣" << std::endl;

            size_t leaksCount = 0;
            size_t leaksSize = 0;
            size_t activeBlocks = 0;

            for (const auto& pair : allocatedBlocks) {
                if (!pair.second.isFreed) {
                    activeBlocks++;
                    // التحقق من الكتل القديمة جداً
                    auto now = std::chrono::system_clock::now();
                    auto age = std::chrono::duration_cast<std::chrono::minutes>(
                        now - pair.second.allocationTime);

                    if (age > std::chrono::minutes(10)) {  // كتلة قديمة جداً
                        leaksCount++;
                        leaksSize += pair.second.size;
                        std::cout << "║ ❌ تسريب محتمل: " << std::setw(8) << pair.second.size << " بايت @ "
                                  << pair.first << " (" << pair.second.allocationLocation << ")" << std::endl;
                        std::cout << "║    عمر: " << age.count() << " دقائق, نوع: " << getDataTypeName(pair.second.dataType) << std::endl;
                    }
                }
            }

            std::cout << "╠══════════════════════════════════════════════════════════════════════════════╣" << std::endl;
            std::cout << "║ 📈 إجمالي المخصص: " << std::setw(10) << formatBytes(totalAllocated) << std::endl;
            std::cout << "║ 📉 إجمالي المُحرر: " << std::setw(10) << formatBytes(totalFreed) << std::endl;
            std::cout << "║ 🏔️  ذروة الاستخدام: " << std::setw(10) << formatBytes(peakMemoryUsage) << std::endl;
            std::cout << "║ 📦 الكتل النشطة: " << std::setw(10) << activeBlocks << std::endl;
            std::cout << "║ 💧 التسريبات المحتملة: " << std::setw(3) << leaksCount << " كتل (" << formatBytes(leaksSize) << ")" << std::endl;

            if (garbageCollectionEnabled) {
                std::cout << "║ 🗑️  جامع القمامة: مفعل (فترة: " << gcInterval.count() << "ms)" << std::endl;
            } else {
                std::cout << "║ 🗑️  جامع القمامة: معطل" << std::endl;
            }

            if (leaksCount == 0) {
                std::cout << "║ ✅ إدارة الذاكرة تبدو سليمة!" << std::endl;
            } else {
                std::cout << "║ ⚠️  يُنصح بمراجعة التخصيصات طويلة الأمد" << std::endl;
            }

            std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝" << std::endl;
        }

        std::string formatBytes(size_t bytes) {
            if (bytes < 1024) return std::to_string(bytes) + " B";
            if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
            return std::to_string(bytes / (1024 * 1024)) + " MB";
        }

        void cleanup() {
            // تحرير جميع الكتل غير المُحررة (للتنظيف النهائي)
            for (auto& pair : allocatedBlocks) {
                if (!pair.second.isFreed) {
                    free(pair.first);
                    pair.second.isFreed = true;
                    totalFreed += pair.second.size;
                }
            }
            allocatedBlocks.clear();
        }

        ~MemoryManager() {
            cleanup();
        }
    };

    // Instance عالمي للمدير
    static MemoryManager globalMemoryManager;

    // دوال واجهة عامة لإدارة الذاكرة المحسنة
    void* arabic_malloc(size_t size, const std::string& location) {
        return globalMemoryManager.allocate(size, location, MemoryManager::DataType::UNKNOWN);
    }

    void* arabic_malloc_string(size_t size, const std::string& location) {
        return globalMemoryManager.allocate(size, location, MemoryManager::DataType::STRING);
    }

    void* arabic_malloc_array(size_t size, const std::string& location) {
        return globalMemoryManager.allocate(size, location, MemoryManager::DataType::ARRAY);
    }

    void* arabic_malloc_object(size_t size, const std::string& location) {
        return globalMemoryManager.allocate(size, location, MemoryManager::DataType::OBJECT);
    }

    void arabic_free(void* ptr) {
        globalMemoryManager.deallocate(ptr);
    }

    void arabic_increment_ref(void* ptr) {
        globalMemoryManager.incrementRef(ptr);
    }

    void arabic_decrement_ref(void* ptr) {
        globalMemoryManager.decrementRef(ptr);
    }

    bool arabic_is_valid_pointer(void* ptr) {
        return globalMemoryManager.isValidPointer(ptr);
    }

    const char* arabic_get_memory_info(void* ptr) {
        static std::string info;
        info = globalMemoryManager.getMemoryInfo(ptr);
        return info.c_str();
    }

    void arabic_memory_report() {
        globalMemoryManager.reportMemoryLeaks();
    }

    void arabic_enable_gc() {
        globalMemoryManager.setGarbageCollectionEnabled(true);
    }

    void arabic_disable_gc() {
        globalMemoryManager.setGarbageCollectionEnabled(false);
    }

    void arabic_memory_cleanup() {
        globalMemoryManager.cleanup();
    }

    void arabic_set_memory_tracking(bool enabled) {
        globalMemoryManager.setTrackingEnabled(enabled);
    }

    ArabicRuntime::ArabicRuntime() {
        // تأكد من وجود نطاق افتراضي
        symbolTable.clear();
        hasReturnValue = false;
        shouldBreak = false;
        shouldContinue = false;
        returnValue = Value(ValueType::NONE);
        
        // تسجيل المكتبة القياسية
        initializeStandardLibrary();
    }

    void ArabicRuntime::initializeStandardLibrary() {
        // تعريف الثوابت العالمية
        defineVariable("لاشيء",   Value(ValueType::NONE));
        defineVariable("لا_شيء", Value(ValueType::NONE));   // ✅ بالشرطة السفلية
        defineVariable("None",    Value(ValueType::NONE));
        defineVariable("عدم",     Value(ValueType::NONE));
        defineVariable("صحيح",   Value(ValueType::NUMBER, "1"));
        defineVariable("نعم",     Value(ValueType::NUMBER, "1"));
        defineVariable("خطأ",     Value(ValueType::NUMBER, "0"));
        defineVariable("لا",      Value(ValueType::NUMBER, "0"));

#ifdef _WIN32
        // ✅ تهيئة وحدة التحكم لدعم اليونيكود (UTF-8) في بيئة التشغيل
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        
        // تفعيل دعم الألوان (ANSI) في Windows 10+
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
#endif

        // 1. مصفوفات ونصوص
        defineNativeFunction("طول", [](const std::vector<Value>& args) {
            if (args.empty()) return Value(ValueType::NUMBER, "0");
            if (args[0].type == ValueType::ARRAY) {
                return Value(ValueType::NUMBER, std::to_string(args[0].elements.size()));
            }
            if (args[0].type == ValueType::STRING) {
                // ✅ عدّ حروف UTF-8 الفعلية لا البايتات
                const std::string& s = args[0].string_value;
                size_t count = 0;
                for (size_t i = 0; i < s.size(); ) {
                    unsigned char c = (unsigned char)s[i];
                    if      (c < 0x80)  { i += 1; }
                    else if (c < 0xE0)  { i += 2; }
                    else if (c < 0xF0)  { i += 3; }
                    else                { i += 4; }
                    count++;
                }
                return Value(ValueType::NUMBER, std::to_string(count));
            }
            if (args[0].type == ValueType::GENERIC_MAP || args[0].type == ValueType::OBJECT) {
                return Value(ValueType::NUMBER, std::to_string(args[0].map_elements.size()));
            }
            return Value(ValueType::NUMBER, "0");
        });
        nativeFunctions["طول_المصفوفة"] = nativeFunctions["طول"];
        nativeFunctions["طول_النص"]     = nativeFunctions["طول"];

        defineNativeFunction("أضف", [](const std::vector<Value>& args) {
            if (args.size() < 2) return Value(ValueType::NONE);
            // ملاحظة: بما أن Value تُمرر بالقيمة، سنقوم بتعديل المصفوفة وإعادتها
            Value arr = args[0];
            if (arr.type == ValueType::ARRAY) {
                arr.elements.push_back(args[1]);
                return arr;
            }
            return Value(ValueType::NONE);
        });
        nativeFunctions["أضيف"] = nativeFunctions["أضف"];

        // 2. الطباعة كدوال أصلية لدعم الاستدعاء المرن
        auto consolePrint = [](const std::vector<Value>& args) {
            for (const auto& arg : args) {
                if (arg.type == ValueType::STRING) std::cout << arg.string_value;
                else if (arg.type == ValueType::NUMBER) std::cout << arg.value;
                else if (arg.type == ValueType::ARRAY) {
                    std::cout << "[";
                    for (size_t i = 0; i < arg.elements.size(); ++i) {
                        if (arg.elements[i].type == ValueType::STRING) std::cout << "\"" << arg.elements[i].string_value << "\"";
                        else std::cout << arg.elements[i].value;
                        if (i < arg.elements.size() - 1) std::cout << ", ";
                    }
                    std::cout << "]";
                }
                else std::cout << arg.value;
            }
            std::cout << std::endl;
            return Value(ValueType::NONE);
        };

        defineNativeFunction("اطبع", consolePrint);
        
        // 3. رياضيات
        defineNativeFunction("جذر", [](const std::vector<Value>& args) {
            if (args.empty()) return Value(ValueType::NUMBER, "0");
            double val = 0;
            try { val = std::stod(args[0].string_value.empty() ? args[0].value : args[0].string_value); } catch(...) {}
            double res = std::sqrt(val);
            return Value(ValueType::NUMBER, std::to_string(res));
        });

        defineNativeFunction("قوة", [](const std::vector<Value>& args) {
            if (args.size() < 2) return Value(ValueType::NUMBER, "0");
            double base = 0, exp = 0;
            try { 
                base = std::stod(args[0].string_value.empty() ? args[0].value : args[0].string_value);
                exp = std::stod(args[1].string_value.empty() ? args[1].value : args[1].string_value);
            } catch(...) {}
            double res = std::pow(base, exp);
            return Value(ValueType::NUMBER, std::to_string(res));
        });

        defineNativeFunction("مطلق", [](const std::vector<Value>& args) {
        if (args.empty()) return Value(ValueType::NUMBER, "0");
        double val = 0;
        try { val = std::stod(args[0].string_value.empty() ? args[0].value : args[0].string_value); } catch(...) {}
        double res = std::abs(val);
        return Value(ValueType::NUMBER, std::to_string(res));
    });

    defineNativeFunction("انتظر", [](const std::vector<Value>& args) {
        if (!args.empty()) {
            try {
                int ms = (int)std::stod(args[0].string_value.empty() ? args[0].value : args[0].string_value);
                if (ms > 0) {
#ifdef _WIN32
                    Sleep(ms);
#else
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
                }
            } catch(...) {}
        }
        return Value(ValueType::NONE);
    });

    // 4. حجم المصفوفة أو النص
    defineNativeFunction("حجم", [](const std::vector<Value>& args) {
        if (args.empty()) return Value(ValueType::NUMBER, "0");
        const auto& arg = args[0];
        if (arg.type == ValueType::ARRAY) {
            return Value(ValueType::NUMBER, std::to_string(arg.elements.size()));
        } else if (arg.type == ValueType::STRING) {
            // عد حروف UTF-8 الفعلية بدلاً من البايتات
            size_t count = 0;
            const std::string& s = arg.string_value;
            for (size_t i = 0; i < s.length(); ++i) {
                if (((unsigned char)s[i] & 0xC0) != 0x80) {
                    count++;
                }
            }
            return Value(ValueType::NUMBER, std::to_string(count));
        } else if (arg.type == ValueType::OBJECT) {
            return Value(ValueType::NUMBER, std::to_string(arg.map_elements.size()));
        }
        return Value(ValueType::NUMBER, "0");
    });

    // 4. إدخال/إخراج (I/O)
    static std::map<std::string, std::shared_ptr<ArabicLanguage::StdLib::ArabicIO::File>> openFiles;
    static int nextFileId = 1;

    defineNativeFunction("افتح_ملف", [](const std::vector<Value>& args) {
        if (args.size() < 2 || args[0].type != ValueType::STRING || args[1].type != ValueType::STRING)
            return Value(ValueType::STRING, "0");
        
        std::string path = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        std::string mode = args[1].string_value.empty() ? args[1].value : args[1].string_value;
        
        auto file = std::make_shared<ArabicLanguage::StdLib::ArabicIO::File>(path, mode);
        if (file->open()) {
            std::string handle = "file_" + std::to_string(nextFileId++);
            openFiles[handle] = file;
            return Value(ValueType::STRING, handle);
        }
        return Value(ValueType::STRING, "0");
    });

    defineNativeFunction("اقرأ_سطر", [](const std::vector<Value>& args) {
        if (args.empty() || args[0].type != ValueType::STRING) return Value(ValueType::STRING, "");
        std::string handle = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        if (openFiles.count(handle)) {
            return Value(ValueType::STRING, openFiles[handle]->readLine());
        }
        return Value(ValueType::STRING, "");
    });

    defineNativeFunction("اقرأ_كل", [](const std::vector<Value>& args) {
        if (args.empty() || args[0].type != ValueType::STRING) return Value(ValueType::STRING, "");
        std::string handle = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        if (openFiles.count(handle)) {
            return Value(ValueType::STRING, openFiles[handle]->readAll());
        }
        return Value(ValueType::STRING, "");
    });

    defineNativeFunction("اكتب_ملف", [](const std::vector<Value>& args) {
        if (args.size() < 2) return Value(ValueType::NUMBER, "0");
        
        // قراءة المسار من string_value أو value (أيهما غير فارغ)
        std::string handle  = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        std::string content = args[1].string_value.empty() ? args[1].value : args[1].string_value;
        
        if (handle.empty()) return Value(ValueType::NUMBER, "0");
        
        if (openFiles.count(handle)) {
            bool success = openFiles[handle]->write(content);
            return Value(ValueType::NUMBER, success ? "1" : "0");
        }
        // كتابة مباشرة للملف
        bool success = ArabicLanguage::StdLib::ArabicIO::getInstance().writeFileText(handle, content);
        return Value(ValueType::NUMBER, success ? "1" : "0");
    });

    defineNativeFunction("اغلق_ملف", [](const std::vector<Value>& args) {
        if (args.empty() || args[0].type != ValueType::STRING) return Value(ValueType::NUMBER, "0");
        std::string handle = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        if (openFiles.count(handle)) {
            openFiles[handle]->close();
            openFiles.erase(handle);
            return Value(ValueType::NUMBER, "1");
        }
        return Value(ValueType::NUMBER, "0");
    });

    // 4.5. عمليات المجلدات والملفات
    defineNativeFunction("قائمة_الملفات", [](const std::vector<Value>& args) {
        if (args.empty() || args[0].type != ValueType::STRING) return Value(ValueType::ARRAY);
        std::string path = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        std::vector<std::string> files = ArabicLanguage::StdLib::ArabicIO::FileOperations::listDirectory(path);
        Value result(ValueType::ARRAY);
        for (const auto& f : files) {
            result.elements.push_back(Value(ValueType::STRING, f));
        }
        return result;
    });

    defineNativeFunction("هل_ملف_موجود", [](const std::vector<Value>& args) {
            if (args.empty() || args[0].type != ValueType::STRING) return Value(ValueType::NUMBER, "0");
            std::string p = args[0].string_value.empty() ? args[0].value : args[0].string_value;
            bool exists = ArabicLanguage::StdLib::ArabicIO::FileOperations::exists(p);
            return Value(ValueType::NUMBER, exists ? "1" : "0");
    });

    defineNativeFunction("المجلد_الحالي", [](const std::vector<Value>& args) {
            return Value(ValueType::STRING, ArabicLanguage::StdLib::ArabicIO::FileOperations::getCurrentDirectory());
    });

    defineNativeFunction("غير_المجلد", [](const std::vector<Value>& args) {
            if (args.empty() || args[0].type != ValueType::STRING) return Value(ValueType::NUMBER, "0");
            bool success = ArabicLanguage::StdLib::ArabicIO::FileOperations::setCurrentDirectory(args[0].string_value);
            return Value(ValueType::NUMBER, success ? "1" : "0");
    });

    // 5. معالجة النصوص المتقدمة
    defineNativeFunction("نص_استبدل", [](const std::vector<Value>& args) {
        if (args.size() < 3) return Value(ValueType::STRING, "");
        std::string str = args[0].string_value;
        std::string from = args[1].string_value;
        std::string to = args[2].string_value;
        
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return Value(ValueType::STRING, str);
    });

    defineNativeFunction("نص_قسم", [](const std::vector<Value>& args) {
        if (args.size() < 2) return Value(ValueType::ARRAY);
        std::string str = args[0].string_value;
        std::string delimiter = args[1].string_value;
        
        Value result(ValueType::ARRAY);
        size_t pos = 0;
        std::string token;
        while ((pos = str.find(delimiter)) != std::string::npos) {
            token = str.substr(0, pos);
            result.elements.push_back(Value(ValueType::STRING, token));
            str.erase(0, pos + delimiter.length());
        }
        result.elements.push_back(Value(ValueType::STRING, str));
        return result;
    });

    defineNativeFunction("نص_فرعي", [](const std::vector<Value>& args) {
        if (args.size() < 2) return Value(ValueType::STRING, "");
        std::string str = args[0].string_value;
        int start = 0;
        try { start = std::stoi(args[1].string_value.empty() ? args[1].value : args[1].string_value); } catch(...) {}
        
        if (args.size() >= 3) {
            int len = 0;
            try { len = std::stoi(args[2].string_value.empty() ? args[2].value : args[2].string_value); } catch(...) {}
            return Value(ValueType::STRING, str.substr(start, len));
        }
        return Value(ValueType::STRING, str.substr(start));
    });

    defineNativeFunction("نص_ابحث", [](const std::vector<Value>& args) {
        if (args.size() < 2) return Value(ValueType::NUMBER, "-1");
        std::string str = args[0].string_value;
        std::string target = args[1].string_value;
        
        size_t pos = str.find(target);
        if (pos == std::string::npos) return Value(ValueType::NUMBER, "-1");
        return Value(ValueType::NUMBER, std::to_string(pos));
    });

    // 5.5. تحويلات إضافية
    defineNativeFunction("نص_إلى_رقم", [](const std::vector<Value>& args) {
        if (args.empty()) return Value(ValueType::NUMBER, "0");
        std::string s = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        try {
            // تنظيف النص من أي مسافات
            s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c){ return std::isspace(c); }), s.end());
            if (s.empty()) return Value(ValueType::NUMBER, "0");
            return Value(ValueType::NUMBER, std::to_string(std::stoi(s)));
        } catch(...) {
            return Value(ValueType::NUMBER, "0");
        }
    });

    defineNativeFunction("رقم_إلى_نص", [](const std::vector<Value>& args) {
        if (args.empty()) return Value(ValueType::STRING, "0");
        std::string s = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        // إذا كان رقماً أصلاً، نعيده كما هو
        return Value(ValueType::STRING, s);
    });

    defineNativeFunction("اكتب", [consolePrint](const std::vector<Value>& args) {
        // إذا كان هناك وسيطان والأول يبدأ بـ "file_" فهو غالباً ملف مفتوح
        if (args.size() >= 2 && args[0].type == ValueType::STRING && 
            args[0].string_value.find("file_") == 0) {
            
            std::string handle = args[0].string_value;
            std::string content = args[1].string_value;
            if (openFiles.count(handle)) {
                openFiles[handle]->write(content);
                return Value(ValueType::NONE);
            }
        }
        
        // غير ذلك، اعتبرها طباعة عادية للكونسول (مثل اطبع)
        return consolePrint(args);
    });

    defineNativeFunction("أغلق_ملف", [](const std::vector<Value>& args) {
        if (args.empty() || args[0].type != ValueType::STRING) return Value(ValueType::NONE);
        std::string handle = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        if (openFiles.count(handle)) {
            openFiles[handle]->close();
            openFiles.erase(handle);
        }
        return Value(ValueType::NONE);
    });

    defineNativeFunction("اقرأ_ملف", [](const std::vector<Value>& args) {
        if (args.empty()) return Value(ValueType::STRING, "");
        std::string path = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        if (path.empty()) return Value(ValueType::STRING, "");
        return Value(ValueType::STRING, ArabicLanguage::StdLib::ArabicIO::getInstance().readFileText(path));
    });

    defineNativeFunction("نص_حرف_عند", [](const std::vector<Value>& args) {
        if (args.size() < 2) return Value(ValueType::STRING, "");
        std::string str = args[0].string_value;
        int index = 0;
        try { index = std::stoi(args[1].string_value.empty() ? args[1].value : args[1].string_value); } catch(...) {}
        
        if (index < 0 || index >= (int)str.length()) return Value(ValueType::STRING, "");
        
        // Handling UTF-8 basic (this is still byte-based, but enough for symbols)
        return Value(ValueType::STRING, std::string(1, str[index]));
    });

    // ✅ إضافة: كود_الحرف — تُرجع الكود Unicode لأول حرف في النص
    defineNativeFunction("كود_الحرف", [](const std::vector<Value>& args) {
        if (args.empty()) return Value(ValueType::NUMBER, "0");
        const std::string& s = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        if (s.empty()) return Value(ValueType::NUMBER, "0");
        // فك ترميز UTF-8 للحصول على أول نقطة ترميز Unicode
        unsigned char c0 = (unsigned char)s[0];
        uint32_t codepoint = 0;
        if (c0 < 0x80) {
            codepoint = c0;
        } else if ((c0 & 0xE0) == 0xC0 && s.size() >= 2) {
            codepoint = ((c0 & 0x1F) << 6) | ((unsigned char)s[1] & 0x3F);
        } else if ((c0 & 0xF0) == 0xE0 && s.size() >= 3) {
            codepoint = ((c0 & 0x0F) << 12) | (((unsigned char)s[1] & 0x3F) << 6) | ((unsigned char)s[2] & 0x3F);
        } else if ((c0 & 0xF8) == 0xF0 && s.size() >= 4) {
            codepoint = ((c0 & 0x07) << 18) | (((unsigned char)s[1] & 0x3F) << 12)
                      | (((unsigned char)s[2] & 0x3F) << 6) | ((unsigned char)s[3] & 0x3F);
        }
        return Value(ValueType::NUMBER, std::to_string(codepoint));
    });
    nativeFunctions["charCode"] = nativeFunctions["كود_الحرف"];

    defineNativeFunction("هل_رقم", [](const std::vector<Value>& args) {
        if (args.empty()) return Value(ValueType::NUMBER, "0");
        std::string s = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        if (s.empty()) return Value(ValueType::NUMBER, "0");
        for (char c : s) if (!std::isdigit(static_cast<unsigned char>(c))) return Value(ValueType::NUMBER, "0");
        return Value(ValueType::NUMBER, "1");
    });

    defineNativeFunction("هل_حرف", [](const std::vector<Value>& args) {
        if (args.empty()) return Value(ValueType::NUMBER, "0");
        std::string s = args[0].string_value;
        if (s.empty()) return Value(ValueType::NUMBER, "0");
        unsigned char c = (unsigned char)s[0];
        if (std::isalpha(c) || c > 127) return Value(ValueType::NUMBER, "1");
        return Value(ValueType::NUMBER, "0");
    });

    defineNativeFunction("هل_فراغ", [](const std::vector<Value>& args) {
        if (args.empty()) return Value(ValueType::NUMBER, "0");
        std::string s = args[0].string_value;
        if (s.empty()) return Value(ValueType::NUMBER, "1");
        for (char c : s) if (!std::isspace(static_cast<unsigned char>(c))) return Value(ValueType::NUMBER, "0");
        return Value(ValueType::NUMBER, "1");
    });

    defineNativeFunction("نوع", [](const std::vector<Value>& args) {
        if (args.empty()) return Value(ValueType::STRING, "لا_شيء");
        const Value& val = args[0];
        
        switch (val.type) {
            case ValueType::NUMBER: return Value(ValueType::STRING, "رقم");
            case ValueType::STRING: return Value(ValueType::STRING, "نص");
            case ValueType::VARIABLE: return Value(ValueType::STRING, "متغير");
            case ValueType::ARRAY: return Value(ValueType::STRING, "مصفوفة");
            case ValueType::OBJECT: 
            case ValueType::CLASS_INSTANCE: return Value(ValueType::STRING, "كائن");
            case ValueType::FUNCTION_CALL: return Value(ValueType::STRING, "دالة");
            case ValueType::OPERATION: return Value(ValueType::STRING, "عملية");
            case ValueType::NONE: return Value(ValueType::STRING, "لا_شيء");
            default: return Value(ValueType::STRING, "غير_معروف");
        }
    });

    // 🎨 مكتبة الألوان
    defineNativeFunction("لون_النص", [](const std::vector<Value>& args) {
        if (args.empty() || args[0].type != ValueType::NUMBER) return Value(ValueType::NONE);
        int color = (int)args[0].number_value;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, color);
        return Value(ValueType::NONE);
    });

    defineNativeFunction("لون_الخلفية", [](const std::vector<Value>& args) {
        if (args.empty() || args[0].type != ValueType::NUMBER) return Value(ValueType::NONE);
        int bg = (int)args[0].number_value;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        WORD attr = (csbi.wAttributes & 0x000F) | (bg << 4);
        SetConsoleTextAttribute(hConsole, attr);
        return Value(ValueType::NONE);
    });

    defineNativeFunction("إعادة_لون", [](const std::vector<Value>&) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 7); // أبيض على أسود
        return Value(ValueType::NONE);
    });

    defineNativeFunction("لون_عشوائي", [](const std::vector<Value>&) {
        int color = 1 + (rand() % 15);
        return Value(ValueType::NUMBER, std::to_string(color));
    });

    // 📦 مكتبة الحاويات (قاموس) - using handle-based approach
    static std::unordered_map<std::string, std::unordered_map<std::string, Value>> dictionaries;
    static int nextDictId = 0;

    defineNativeFunction("قاموس_جديد", [](const std::vector<Value>&) {
        std::string handle = "dict_" + std::to_string(nextDictId++);
        dictionaries[handle] = std::unordered_map<std::string, Value>();
        return Value(ValueType::STRING, handle);
    });

    defineNativeFunction("قاموس_ضع", [](const std::vector<Value>& args) {
        if (args.size() < 3) return Value(ValueType::NUMBER, "0");
        std::string handle = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        std::string key = args[1].string_value.empty() ? args[1].value : args[1].string_value;
        if (dictionaries.find(handle) == dictionaries.end()) return Value(ValueType::NUMBER, "0");
        dictionaries[handle][key] = args[2];
        return Value(ValueType::NUMBER, "1");
    });

    defineNativeFunction("قاموس_اجلب", [](const std::vector<Value>& args) {
        if (args.size() < 2) return Value(ValueType::NONE);
        std::string handle = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        std::string key = args[1].string_value.empty() ? args[1].value : args[1].string_value;
        if (dictionaries.find(handle) == dictionaries.end()) return Value(ValueType::NONE);
        auto& dict = dictionaries[handle];
        auto it = dict.find(key);
        if (it != dict.end()) return it->second;
        return Value(ValueType::NONE);
    });

    defineNativeFunction("قاموس_حجم", [](const std::vector<Value>& args) {
        if (args.empty()) return Value(ValueType::NUMBER, "0");
        std::string handle = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        if (dictionaries.find(handle) == dictionaries.end()) return Value(ValueType::NUMBER, "0");
        return Value(ValueType::NUMBER, std::to_string(dictionaries[handle].size()));
    });

    defineNativeFunction("قاموس_احذف", [](const std::vector<Value>& args) {
        if (args.size() < 2) return Value(ValueType::NUMBER, "0");
        std::string handle = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        std::string key = args[1].string_value.empty() ? args[1].value : args[1].string_value;
        if (dictionaries.find(handle) == dictionaries.end()) return Value(ValueType::NUMBER, "0");
        return Value(ValueType::NUMBER, std::to_string(dictionaries[handle].erase(key)));
    });

    defineNativeFunction("قاموس_يحتوي", [](const std::vector<Value>& args) {
        if (args.size() < 2) return Value(ValueType::NUMBER, "0");
        std::string handle = args[0].string_value.empty() ? args[0].value : args[0].string_value;
        std::string key = args[1].string_value.empty() ? args[1].value : args[1].string_value;
        if (dictionaries.find(handle) == dictionaries.end()) return Value(ValueType::NUMBER, "0");
        return Value(ValueType::NUMBER, dictionaries[handle].count(key) ? "1" : "0");
    });

    // 🧠 وظيفة لتمكين المكتبات من الوصول لأحجام المصفوفات المنخفضة المستوى
    defineNativeFunction("اقرأ_ذاكرة", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(ValueType::NUMBER, "0");
        try {
            // تحويل المعامل إلى رقم (عنوان الذاكرة)
            uintptr_t addr = 0;
            if (args[0].type == ValueType::NUMBER) {
                addr = static_cast<uintptr_t>(args[0].number_value);
            } else {
                addr = static_cast<uintptr_t>(std::stoull(args[0].string_value));
            }
            
            if (addr == 0) return Value(ValueType::NUMBER, "0");
            
            // قراءة 4 بايت كـ uint32_t (مناسب لـ length / capacity)
            uint32_t* ptr = reinterpret_cast<uint32_t*>(addr);
            return Value(ValueType::NUMBER, std::to_string(*ptr));
        } catch (...) {
            return Value(ValueType::NUMBER, "0");
        }
    });
}


    void ArabicRuntime::initialize() {
        // التهيئة الأساسية تمت بالفعل في المشيد
    }

    Value ArabicRuntime::execute(const std::vector<std::shared_ptr<Command>>& commands) {
        // ملاحظة: التنفيذ الفعلي يتم عادة عبر ArabicExecutor
        // ولكن نوفر هذه الواجهة للتوافق مع الأنظمة الفرعية
        return Value(ValueType::NONE);
    }

    void ArabicRuntime::shutdown() {
        reset();
    }

    void* ArabicRuntime::allocateMemory(size_t size, const std::string& location) {
        return globalMemoryManager.allocate(size, location);
    }

    void ArabicRuntime::freeMemory(void* ptr) {
        globalMemoryManager.deallocate(ptr);
    }

    void ArabicRuntime::runGarbageCollection() {
        globalMemoryManager.runGarbageCollection();
    }

    void ArabicRuntime::defineClass(const std::string& name, const ClassDefinition& def) {
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        classManager.defineClass(name, def);
    }

    void ArabicRuntime::defineInterface(const std::string& name, const InterfaceDefinition& def) {
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        classManager.defineInterface(name, def);
    }

    bool ArabicRuntime::hasInterface(const std::string& name) const {
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        return classManager.interfaceExists(name);
    }

    const InterfaceDefinition* ArabicRuntime::getInterfaceInfo(const std::string& name) const {
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        return classManager.getInterfaceInfo(name);
    }

    bool ArabicRuntime::hasClass(const std::string& name) const {
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        return classManager.classExists(name);
    }

    const ClassDefinition* ArabicRuntime::getClassInfo(const std::string& name) const {
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        return classManager.getClassInfo(name);
    }

    bool ArabicRuntime::classHasMethod(const std::string& className, const std::string& methodName) const {
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        return classManager.hasMethod(className, methodName);
    }

    std::shared_ptr<ObjectInstance> ArabicRuntime::createObjectInstance(const std::string& classType) {
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        // 
        return classManager.createObject(classType);
    }

    std::shared_ptr<ObjectInstance> ArabicRuntime::getObjectInstance(const std::string& uniqueId) {
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        auto it = objects.find(uniqueId);
        if (it == objects.end()) return nullptr;
        return it->second;
    }

    void ArabicRuntime::registerObjectInstance(const std::shared_ptr<ObjectInstance>& instance) {
        if (!instance) return;
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        objects[instance->unique_id] = instance;
    }

    Value ArabicRuntime::getObjectProperty(const std::string& objectId, const std::string& propName) {
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        auto inst = getObjectInstance(objectId);
        if (!inst) throw std::runtime_error("❌ معرّف الكائن غير معروف: " + objectId);
        return classManager.getProperty(*inst, propName);
    }

    void ArabicRuntime::setObjectProperty(const std::string& objectId, const std::string& propName, const Value& val) {
        std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
        auto inst = getObjectInstance(objectId);
        if (!inst) throw std::runtime_error("❌ معرّف الكائن غير معروف: " + objectId);
        classManager.setProperty(*inst, propName, val);
    }

} // namespace ArabicLanguage
