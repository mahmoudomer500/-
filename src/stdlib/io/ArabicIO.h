#pragma once

#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <functional>
#include "ArabicTypes.h"
#include "ArabicMemoryManager.h"

namespace fs = std::filesystem;

namespace ArabicLanguage::StdLib {

    /**
     * @brief مكتبة الإدخال/الإخراج القياسية
     * توفر واجهة موحدة للتعامل مع الملفات والإدخال/الإخراج
     */
    class ArabicIO {
    public:
        // Singleton pattern
        static ArabicIO& getInstance();

        // منع النسخ والتعيين
        ArabicIO(const ArabicIO&) = delete;
        ArabicIO& operator=(const ArabicIO&) = delete;

        // ══════════════════════════════════════════════════════════════
        // 📁 عمليات الملفات
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief فئة تمثل ملفاً مفتوحاً
         */
        class File {
        private:
            std::string path;
            std::string mode;
            bool isOpen = false;
            std::shared_ptr<void> fileHandle; // ملف C++ FILE* أو ifstream

        public:
            File(const std::string& filePath, const std::string& openMode = "r");
            ~File();

            bool open();
            bool close();
            bool isOpened() const { return isOpen; }

            // قراءة
            std::string readAll();
            std::vector<std::string> readLines();
            std::string readLine();
            std::vector<uint8_t> readBytes(size_t count = 0);

            // كتابة
            bool write(const std::string& content);
            bool writeLine(const std::string& line);
            bool writeBytes(const std::vector<uint8_t>& data);

            // معلومات الملف
            std::string getPath() const { return path; }
            size_t getSize() const;
            bool exists() const;
            std::string getMode() const { return mode; }

            // العمليات
            bool flush();
            bool seek(size_t position);
            size_t tell() const;
        };

        /**
         * @brief عمليات الملفات الثابتة
         */
        class FileOperations {
        public:
            static bool exists(const std::string& path);
            static bool remove(const std::string& path);
            static bool rename(const std::string& oldPath, const std::string& newPath);
            static bool copy(const std::string& source, const std::string& destination);
            static std::vector<std::string> listDirectory(const std::string& path);
            static bool createDirectory(const std::string& path);
            static bool removeDirectory(const std::string& path);
            static std::string getCurrentDirectory();
            static bool setCurrentDirectory(const std::string& path);
            static std::string getAbsolutePath(const std::string& path);
        };

        // ══════════════════════════════════════════════════════════════
        // ⌨️ الإدخال من المستخدم
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief عمليات الإدخال من وحدة التحكم
         */
        class Console {
        public:
            // قراءة
            static std::string readLine();
            static int readInt();
            static double readDouble();
            static char readChar();

            // كتابة
            static void write(const std::string& text);
            static void writeLine(const std::string& text = "");
            static void writeInt(int value);
            static void writeDouble(double value);

            // تنسيق متقدم
            static void writeFormatted(const char* format, ...);
            static void setTextColor(int color);
            static void resetTextColor();

            // معلومات وحدة التحكم
            static int getWidth();
            static int getHeight();
            static void clear();
            static void pause();
        };

        // ══════════════════════════════════════════════════════════════
        // 📊 عمليات التنسيق والتحويل
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تحويل البيانات وتنسيقها
         */
        class DataConverter {
        public:
            // تحويل الأرقام
            static std::string intToString(int value);
            static std::string doubleToString(double value, int precision = 2);
            static int stringToInt(const std::string& str);
            static double stringToDouble(const std::string& str);

            // تحويل الترميز
            static std::string utf8ToAnsi(const std::string& utf8);
            static std::string ansiToUtf8(const std::string& ansi);
            static std::string utf16leToUtf8(const std::string& utf16);
            static std::string utf16beToUtf8(const std::string& utf16);
            static std::string detectAndConvert(const std::string& data);
            static std::vector<uint8_t> stringToBytes(const std::string& str);
            static std::string bytesToString(const std::vector<uint8_t>& bytes);

            // Base64
            static std::string encodeBase64(const std::string& data);
            static std::string decodeBase64(const std::string& encoded);
        };

        // ══════════════════════════════════════════════════════════════
        // 🔄 عمليات غير متزامنة
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief عمليات I/O غير متزامنة
         */
        class AsyncIO {
        public:
            using FileCallback = std::function<void(const std::string&)>;
            using DataCallback = std::function<void(const std::vector<uint8_t>&)>;

            // قراءة غير متزامنة
            static void readFileAsync(const std::string& path, FileCallback callback);
            static void readBytesAsync(const std::string& path, DataCallback callback);

            // كتابة غير متزامنة
            static void writeFileAsync(const std::string& path, const std::string& content, FileCallback callback);
            static void writeBytesAsync(const std::string& path, const std::vector<uint8_t>& data, DataCallback callback);

            // مراقبة الملفات
            static void watchFile(const std::string& path, FileCallback callback);
            static void watchDirectory(const std::string& path, FileCallback callback);
        };

        // ══════════════════════════════════════════════════════════════
        // 📈 مراقبة الأداء
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مراقبة أداء عمليات I/O
         */
        class PerformanceMonitor {
        private:
            struct IOOperation {
                std::string operation;
                std::chrono::steady_clock::time_point start;
                size_t dataSize = 0;
            };

            std::vector<IOOperation> activeOperations;
            std::unordered_map<std::string, std::vector<double>> operationTimes;

        public:
            void startOperation(const std::string& operation, size_t dataSize = 0);
            void endOperation(const std::string& operation);

            double getAverageTime(const std::string& operation) const;
            size_t getTotalOperations(const std::string& operation) const;
            double getThroughput(const std::string& operation) const; // bytes/second

            std::vector<std::string> getActiveOperations() const;
            std::vector<std::string> getPerformanceReport() const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🎯 واجهة الاستخدام العامة
        // ══════════════════════════════════════════════════════════════

        // File operations
        File& openFile(const std::string& path, const std::string& mode = "r");
        bool closeFile(const std::string& path);

        // Console operations
        std::string readConsoleLine();
        void writeConsole(const std::string& text);
        void writeConsoleLine(const std::string& text = "");

        // Utility functions
        std::string readFileText(const std::string& path);
        bool writeFileText(const std::string& path, const std::string& content);
        std::vector<uint8_t> readFileBytes(const std::string& path);
        bool writeFileBytes(const std::string& path, const std::vector<uint8_t>& data);

        // Performance monitoring
        PerformanceMonitor& getPerformanceMonitor() { return perfMonitor; }

        // Statistics
        std::vector<std::string> getIOStats() const;

    private:
        std::unordered_map<std::string, std::unique_ptr<File>> openFiles;
        PerformanceMonitor perfMonitor;

        ArabicIO() = default;
        ~ArabicIO() = default;
    };

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة للمكتبة القياسية
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief دوال I/O المباشرة للاستخدام السريع
     */
    namespace IO {

        // قراءة الملفات
        std::string اقرأ_ملف(const std::string& مسار);
        bool اكتب_ملف(const std::string& مسار, const std::string& محتوى);
        std::vector<std::string> اقرأ_أسطر(const std::string& مسار);

        // وحدة التحكم
        std::string اقرأ_سطر();
        void اطبع(const std::string& نص);
        void اطبع_سطر(const std::string& نص = "");

        // الملفات والمجلدات
        bool ملف_موجود(const std::string& مسار);
        bool أنشئ_مجلد(const std::string& مسار);
        std::vector<std::string> قائمة_مجلد(const std::string& مسار);

        // التحويل
        std::string رقم_إلى_نص(int رقم);
        std::string عدد_إلى_نص(double عدد);
        int نص_إلى_رقم(const std::string& نص);
        double نص_إلى_عدد(const std::string& نص);

    } // namespace IO

} // namespace ArabicLanguage::StdLib
