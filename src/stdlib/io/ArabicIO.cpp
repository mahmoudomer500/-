#include "ArabicIO.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstdarg>
#include <cstring>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SafeWindows.h"
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

namespace ArabicLanguage::StdLib {

    // ══════════════════════════════════════════════════════════════
    // 📁 تنفيذ فئة الملف
    // ══════════════════════════════════════════════════════════════

    ArabicIO::File::File(const std::string& filePath, const std::string& openMode)
        : path(filePath), mode(openMode) {
    }

    ArabicIO::File::~File() {
        close();
    }

    bool ArabicIO::File::open() {
        if (isOpen) return true;

        try {
#ifdef _WIN32
            // تحويل المسار من UTF-8 إلى UTF-16
            int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
            if (wlen <= 0) return false;
            std::wstring wpath(wlen, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], wlen);
            wpath.resize(wcslen(wpath.c_str()));

            DWORD accessMode = 0;
            DWORD creationMode = 0;

            if (mode.find('w') != std::string::npos) {
                accessMode = GENERIC_WRITE;
                if (mode.find('+') != std::string::npos) accessMode |= GENERIC_READ;
                creationMode = CREATE_ALWAYS;
            } else if (mode.find('a') != std::string::npos) {
                accessMode = GENERIC_WRITE;
                if (mode.find('+') != std::string::npos) accessMode |= GENERIC_READ;
                creationMode = OPEN_ALWAYS;
            } else {
                accessMode = GENERIC_READ;
                creationMode = OPEN_EXISTING;
            }

            DWORD flags = FILE_ATTRIBUTE_NORMAL;
            if (mode.find('b') != std::string::npos) flags |= FILE_FLAG_SEQUENTIAL_SCAN;

            HANDLE hFile = CreateFileW(wpath.c_str(), accessMode, FILE_SHARE_READ, nullptr, creationMode, flags, nullptr);
            if (hFile == INVALID_HANDLE_VALUE) return false;

            int fd = _open_osfhandle((intptr_t)hFile, 0);
            if (fd < 0) {
                CloseHandle(hFile);
                return false;
            }

            FILE* fp = _fdopen(fd, mode.c_str());
            if (!fp) {
                CloseHandle(hFile);
                return false;
            }

            fileHandle = std::shared_ptr<void>(fp, [](void* p) { if (p) fclose((FILE*)p); });
            isOpen = true;
            return true;
#else
            std::ios_base::openmode openMode = (std::ios_base::openmode)0;
            if (mode.find('w') != std::string::npos) {
                openMode |= std::ios_base::out;
                if (mode.find('+') == std::string::npos && mode.find('a') == std::string::npos) {
                    openMode |= std::ios_base::trunc;
                }
            } else if (mode.find('a') != std::string::npos) {
                openMode |= std::ios_base::out;
            } else {
                openMode |= std::ios_base::in;
            }
            if (mode.find('a') != std::string::npos) openMode |= std::ios_base::app;
            if (mode.find('+') != std::string::npos) openMode |= std::ios_base::in | std::ios_base::out;
            if (mode.find('b') != std::string::npos) openMode |= std::ios_base::binary;

            auto stream = std::make_shared<std::fstream>(path, openMode);
            if (stream->is_open()) {
                fileHandle = stream;
                isOpen = true;
                return true;
            }
            return false;
#endif
        } catch (const std::exception&) {
            return false;
        }
    }

    bool ArabicIO::File::close() {
        if (!isOpen) return true;

        try {
#ifdef _WIN32
            FILE* fp = static_cast<FILE*>(fileHandle.get());
            if (fp) fclose(fp);
#else
            auto stream = static_cast<std::fstream*>(fileHandle.get());
            if (stream) stream->close();
#endif
            isOpen = false;
            fileHandle.reset();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    std::string ArabicIO::File::readAll() {
        if (!isOpen && !open()) return "";

        try {
#ifdef _WIN32
            FILE* fp = static_cast<FILE*>(fileHandle.get());
            if (!fp) return "";
            
            fseek(fp, 0, SEEK_END);
            long fsize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            
            if (fsize <= 0) return "";
            
            std::string content(fsize, '\0');
            size_t bytesRead = fread(&content[0], 1, fsize, fp);
            content.resize(bytesRead);
#else
            auto stream = static_cast<std::fstream*>(fileHandle.get());
            if (!stream || !stream->is_open()) return "";
            stream->clear();
            if (mode.find('a') == std::string::npos) {
                stream->seekg(0, std::ios::beg);
            }
            std::stringstream buffer;
            buffer << stream->rdbuf();
            std::string content = buffer.str();
#endif
            
            return DataConverter::detectAndConvert(content);
        } catch (const std::exception& e) {
            std::cerr << "❌ خطأ أثناء قراءة الملف: " << e.what() << std::endl;
            return "";
        }
    }

    std::vector<std::string> ArabicIO::File::readLines() {
        std::vector<std::string> lines;
        if (!isOpen && !open()) return lines;

        try {
#ifdef _WIN32
            FILE* fp = static_cast<FILE*>(fileHandle.get());
            if (!fp) return lines;
            char buf[4096];
            while (fgets(buf, sizeof(buf), fp)) {
                std::string line(buf);
                if (!line.empty() && line.back() == '\n') line.pop_back();
                if (!line.empty() && line.back() == '\r') line.pop_back();
                lines.push_back(line);
            }
#else
            auto stream = static_cast<std::fstream*>(fileHandle.get());
            if (!stream) return lines;
            std::string line;
            while (std::getline(*stream, line)) {
                lines.push_back(line);
            }
#endif
            return lines;
        } catch (const std::exception&) {
            return lines;
        }
    }

    std::string ArabicIO::File::readLine() {
        if (!isOpen && !open()) return "";

        try {
#ifdef _WIN32
            FILE* fp = static_cast<FILE*>(fileHandle.get());
            if (!fp) return "";
            char buf[4096];
            if (fgets(buf, sizeof(buf), fp)) {
                std::string line(buf);
                if (!line.empty() && line.back() == '\n') line.pop_back();
                if (!line.empty() && line.back() == '\r') line.pop_back();
                return line;
            }
            return "";
#else
            auto stream = static_cast<std::fstream*>(fileHandle.get());
            if (!stream) return "";
            std::string line;
            if (std::getline(*stream, line)) {
                return line;
            }
            return "";
#endif
        } catch (const std::exception&) {
            return "";
        }
    }

    std::vector<uint8_t> ArabicIO::File::readBytes(size_t count) {
        std::vector<uint8_t> data;
        if (!isOpen && !open()) return data;

        try {
#ifdef _WIN32
            FILE* fp = static_cast<FILE*>(fileHandle.get());
            if (!fp) return data;
            size_t bytesToRead = count > 0 ? count : getSize();
            data.resize(bytesToRead);
            size_t actualRead = fread(data.data(), 1, bytesToRead, fp);
            data.resize(actualRead);
#else
            auto stream = static_cast<std::fstream*>(fileHandle.get());
            if (!stream) return data;
            size_t bytesToRead = count > 0 ? count : getSize();
            data.resize(bytesToRead);
            stream->read(reinterpret_cast<char*>(data.data()), bytesToRead);
            size_t actualRead = stream->gcount();
            data.resize(actualRead);
#endif
            return data;
        } catch (const std::exception&) {
            return data;
        }
    }

    bool ArabicIO::File::write(const std::string& content) {
        if (!isOpen && !open()) return false;

        try {
#ifdef _WIN32
            FILE* fp = static_cast<FILE*>(fileHandle.get());
            if (!fp) return false;
            size_t written = fwrite(content.c_str(), 1, content.size(), fp);
            fflush(fp);
            return written == content.size();
#else
            auto stream = static_cast<std::fstream*>(fileHandle.get());
            if (!stream) return false;
            *stream << content;
            stream->flush();
            return stream->good();
#endif
        } catch (const std::exception&) {
            return false;
        }
    }

    bool ArabicIO::File::writeLine(const std::string& line) {
        if (!isOpen && !open()) return false;

        try {
#ifdef _WIN32
            FILE* fp = static_cast<FILE*>(fileHandle.get());
            if (!fp) return false;
            int result = fputs(line.c_str(), fp);
            if (result == EOF) return false;
            result = fputc('\n', fp);
            fflush(fp);
            return result != EOF;
#else
            auto stream = static_cast<std::fstream*>(fileHandle.get());
            if (!stream) return false;
            *stream << line << std::endl;
            stream->flush();
            return stream->good();
#endif
        } catch (const std::exception&) {
            return false;
        }
    }

    bool ArabicIO::File::writeBytes(const std::vector<uint8_t>& data) {
        if (!isOpen && !open()) return false;

        try {
#ifdef _WIN32
            FILE* fp = static_cast<FILE*>(fileHandle.get());
            if (!fp) return false;
            size_t written = fwrite(data.data(), 1, data.size(), fp);
            fflush(fp);
            return written == data.size();
#else
            auto stream = static_cast<std::fstream*>(fileHandle.get());
            if (!stream) return false;
            stream->write(reinterpret_cast<const char*>(data.data()), data.size());
            stream->flush();
            return stream->good();
#endif
        } catch (const std::exception&) {
            return false;
        }
    }

    size_t ArabicIO::File::getSize() const {
        try {
            return fs::file_size(path);
        } catch (const std::exception&) {
            return 0;
        }
    }

    bool ArabicIO::File::exists() const {
        return fs::exists(path);
    }

    bool ArabicIO::File::flush() {
        if (!isOpen) return false;

        try {
#ifdef _WIN32
            FILE* fp = static_cast<FILE*>(fileHandle.get());
            if (!fp) return false;
            return fflush(fp) == 0;
#else
            if (mode.find('w') != std::string::npos) {
                auto& stream = *static_cast<std::ofstream*>(fileHandle.get());
                stream.flush();
                return stream.good();
            }
            return true;
#endif
        } catch (const std::exception&) {
            return false;
        }
    }

    bool ArabicIO::File::seek(size_t position) {
        if (!isOpen) return false;

        try {
#ifdef _WIN32
            FILE* fp = static_cast<FILE*>(fileHandle.get());
            if (!fp) return false;
            return fseek(fp, position, SEEK_SET) == 0;
#else
            if (mode.find('w') != std::string::npos) {
                auto& stream = *static_cast<std::ofstream*>(fileHandle.get());
                stream.seekp(position);
                return stream.good();
            } else {
                auto& stream = *static_cast<std::ifstream*>(fileHandle.get());
                stream.seekg(position);
                return stream.good();
            }
#endif
        } catch (const std::exception&) {
            return false;
        }
    }

    size_t ArabicIO::File::tell() const {
        if (!isOpen) return 0;

        try {
            if (mode.find('w') != std::string::npos) {
                auto& stream = *static_cast<std::ofstream*>(fileHandle.get());
                return stream.tellp();
            } else {
                auto& stream = *static_cast<std::ifstream*>(fileHandle.get());
                return stream.tellg();
            }
        } catch (const std::exception&) {
            return 0;
        }
    }

    // ══════════════════════════════════════════════════════════════
    // 📁 تنفيذ عمليات الملفات الثابتة
    // ══════════════════════════════════════════════════════════════

    bool ArabicIO::FileOperations::exists(const std::string& path) {
        return fs::exists(path);
    }

    bool ArabicIO::FileOperations::remove(const std::string& path) {
        try {
            return fs::remove(path);
        } catch (const std::exception&) {
            return false;
        }
    }

    bool ArabicIO::FileOperations::rename(const std::string& oldPath, const std::string& newPath) {
        try {
            fs::rename(oldPath, newPath);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool ArabicIO::FileOperations::copy(const std::string& source, const std::string& destination) {
        try {
            fs::copy(source, destination);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    std::vector<std::string> ArabicIO::FileOperations::listDirectory(const std::string& path) {
        std::vector<std::string> entries;

        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                entries.push_back(entry.path().filename().string());
            }
        } catch (const std::exception&) {
            // إرجاع قائمة فارغة في حالة الخطأ
        }

        return entries;
    }

    bool ArabicIO::FileOperations::createDirectory(const std::string& path) {
        try {
            return fs::create_directory(path);
        } catch (const std::exception&) {
            return false;
        }
    }

    bool ArabicIO::FileOperations::removeDirectory(const std::string& path) {
        try {
            return fs::remove_all(path) > 0;
        } catch (const std::exception&) {
            return false;
        }
    }

    std::string ArabicIO::FileOperations::getCurrentDirectory() {
        try {
            return fs::current_path().string();
        } catch (const std::exception&) {
            return "";
        }
    }

    bool ArabicIO::FileOperations::setCurrentDirectory(const std::string& path) {
        try {
            fs::current_path(path);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    std::string ArabicIO::FileOperations::getAbsolutePath(const std::string& path) {
        try {
            return fs::absolute(path).string();
        } catch (const std::exception&) {
            return path;
        }
    }

    // ══════════════════════════════════════════════════════════════
    // ⌨️ تنفيذ وحدة التحكم
    // ══════════════════════════════════════════════════════════════

    std::string ArabicIO::Console::readLine() {
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    int ArabicIO::Console::readInt() {
        int value;
        std::cin >> value;
        std::cin.ignore(); // تجاهل باقي السطر
        return value;
    }

    double ArabicIO::Console::readDouble() {
        double value;
        std::cin >> value;
        std::cin.ignore();
        return value;
    }

    char ArabicIO::Console::readChar() {
        char value;
        std::cin >> value;
        std::cin.ignore();
        return value;
    }

    void ArabicIO::Console::write(const std::string& text) {
        std::cout << text;
    }

    void ArabicIO::Console::writeLine(const std::string& text) {
        std::cout << text << std::endl;
    }

    void ArabicIO::Console::writeInt(int value) {
        std::cout << value;
    }

    void ArabicIO::Console::writeDouble(double value) {
        std::cout << value;
    }

    void ArabicIO::Console::writeFormatted(const char* format, ...) {
        va_list args;
        va_start(args, format);

        char buffer[1024];
        vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);

        std::cout << buffer;
        va_end(args);
    }

    void ArabicIO::Console::setTextColor(int color) {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, color);
#else
        // تنفيذ بسيط للون على Linux/macOS
        switch (color) {
            case 1: std::cout << "\033[31m"; break; // أحمر
            case 2: std::cout << "\033[32m"; break; // أخضر
            case 3: std::cout << "\033[33m"; break; // أصفر
            case 4: std::cout << "\033[34m"; break; // أزرق
            default: break;
        }
#endif
    }

    void ArabicIO::Console::resetTextColor() {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 7); // لون افتراضي
#else
        std::cout << "\033[0m"; // إعادة تعيين اللون
#endif
    }

    int ArabicIO::Console::getWidth() {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        return csbi.dwSize.X;
#else
        // تنفيذ بسيط للـ Linux/macOS
        return 80; // عرض افتراضي
#endif
    }

    int ArabicIO::Console::getHeight() {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        return csbi.dwSize.Y;
#else
        return 24; // ارتفاع افتراضي
#endif
    }

    void ArabicIO::Console::clear() {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD count, cellCount;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        cellCount = csbi.dwSize.X * csbi.dwSize.Y;
        FillConsoleOutputCharacter(hConsole, ' ', cellCount, {0, 0}, &count);
        FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, {0, 0}, &count);
        SetConsoleCursorPosition(hConsole, {0, 0});
#else
        std::cout << "\033[2J\033[1;1H";
#endif
    }

    void ArabicIO::Console::pause() {
        std::cout << "اضغط Enter للمتابعة...";
        std::cin.ignore();
    }

    // ══════════════════════════════════════════════════════════════
    // 📊 تنفيذ محول البيانات
    // ══════════════════════════════════════════════════════════════

    std::string ArabicIO::DataConverter::intToString(int value) {
        return std::to_string(value);
    }

    std::string ArabicIO::DataConverter::doubleToString(double value, int precision) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << value;
        return ss.str();
    }

    int ArabicIO::DataConverter::stringToInt(const std::string& str) {
        try {
            return std::stoi(str);
        } catch (const std::exception&) {
            return 0;
        }
    }

    double ArabicIO::DataConverter::stringToDouble(const std::string& str) {
        try {
            return std::stod(str);
        } catch (const std::exception&) {
            return 0.0;
        }
    }

    std::string ArabicIO::DataConverter::utf8ToAnsi(const std::string& utf8) {
#ifdef _WIN32
        // تحويل UTF-8 إلى ANSI على Windows
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        std::wstring wide(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], size);

        int ansiSize = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string ansi(ansiSize, 0);
        WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, &ansi[0], ansiSize, nullptr, nullptr);

        return ansi;
#else
        // على Linux/macOS، UTF-8 هو الافتراضي
        return utf8;
#endif
    }

    std::string ArabicIO::DataConverter::ansiToUtf8(const std::string& ansi) {
#ifdef _WIN32
        // تحويل ANSI إلى UTF-8 على Windows
        int size = MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, nullptr, 0);
        std::wstring wide(size, 0);
        MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, &wide[0], size);

        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(utf8Size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], utf8Size, nullptr, nullptr);

        return utf8;
#else
        // على Linux/macOS، ANSI هو نفسه UTF-8 في التعامل البسيط
        return ansi;
#endif
    }

    std::string ArabicIO::DataConverter::utf16leToUtf8(const std::string& utf16) {
#ifdef _WIN32
        if (utf16.length() < 2) return "";
        
        // تجاوز BOM إذا وجد (FF FE)
        const char* data = utf16.c_str();
        size_t len = utf16.length();
        if (len >= 2 && (unsigned char)data[0] == 0xFF && (unsigned char)data[1] == 0xFE) {
            data += 2;
            len -= 2;
        }

        int wideLen = (int)(len / 2);
        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)data, wideLen, nullptr, 0, nullptr, nullptr);
        if (utf8Size <= 0) return "";

        std::string utf8(utf8Size, 0);
        WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)data, wideLen, &utf8[0], utf8Size, nullptr, nullptr);
        return utf8;
#else
        // تنفيذ بسيط لغير ويندوز (يتطلب مكتبة تحويل)
        return utf16; 
#endif
    }

    std::string ArabicIO::DataConverter::utf16beToUtf8(const std::string& utf16) {
        // UTF-16 BE يحتاج لتبديل البايتات أولاً ثم تحويله كـ LE
        std::string le = utf16;
        for (size_t i = 0; i + 1 < le.length(); i += 2) {
            std::swap(le[i], le[i+1]);
        }
        return utf16leToUtf8(le);
    }

    std::string ArabicIO::DataConverter::detectAndConvert(const std::string& data) {
        if (data.length() < 2) return data;

        const unsigned char* udata = (const unsigned char*)data.c_str();

        // 1. UTF-8 BOM (EF BB BF)
        if (data.length() >= 3 && udata[0] == 0xEF && udata[1] == 0xBB && udata[2] == 0xBF) {
            return data.substr(3);
        }

        // 2. UTF-16 LE BOM (FF FE)
        if (udata[0] == 0xFF && udata[1] == 0xFE) {
            return utf16leToUtf8(data);
        }

        // 3. UTF-16 BE BOM (FE FF)
        if (udata[0] == 0xFE && udata[1] == 0xFF) {
            return utf16beToUtf8(data);
        }

        // 4. التحقق من وجود أصفار (مؤشر على UTF-16 بدون BOM)
        bool hasZeros = false;
        for (size_t i = 0; i < std::min(data.length(), (size_t)100); ++i) {
            if (udata[i] == 0) {
                hasZeros = true;
                break;
            }
        }

        if (hasZeros) {
            // غالباً UTF-16 LE
            return utf16leToUtf8(data);
        }

        return data;
    }

    std::vector<uint8_t> ArabicIO::DataConverter::stringToBytes(const std::string& str) {
        return std::vector<uint8_t>(str.begin(), str.end());
    }

    std::string ArabicIO::DataConverter::bytesToString(const std::vector<uint8_t>& bytes) {
        return std::string(bytes.begin(), bytes.end());
    }

    std::string ArabicIO::DataConverter::encodeBase64(const std::string& data) {
        static const char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string encoded;
        int val = 0;
        int valb = -6;

        for (unsigned char c : data) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                encoded.push_back(base64Chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }

        if (valb > -6) {
            encoded.push_back(base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
        }

        while (encoded.size() % 4) {
            encoded.push_back('=');
        }

        return encoded;
    }

    std::string ArabicIO::DataConverter::decodeBase64(const std::string& encoded) {
        static const int base64Index[256] = {
            // ... (جدول البحث - تنفيذ كامل مطلوب للإنتاج)
        };

        std::string decoded;
        int val = 0;
        int valb = -8;

        for (unsigned char c : encoded) {
            if (c == '=') break;
            val = (val << 6) + base64Index[c];
            valb += 6;
            if (valb >= 0) {
                decoded.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }

        return decoded;
    }

    // ══════════════════════════════════════════════════════════════
    // 🔄 تنفيذ العمليات غير المتزامنة
    // ══════════════════════════════════════════════════════════════

    void ArabicIO::AsyncIO::readFileAsync(const std::string& path, FileCallback callback) {
        // تنفيذ بسيط - في الإصدار الكامل سيستخدم std::async
        try {
            std::ifstream file(path);
            std::stringstream buffer;
            buffer << file.rdbuf();
            if (callback) {
                callback(buffer.str());
            }
        } catch (const std::exception& e) {
            if (callback) {
                callback("خطأ في قراءة الملف: " + std::string(e.what()));
            }
        }
    }

    void ArabicIO::AsyncIO::readBytesAsync(const std::string& path, DataCallback callback) {
        try {
            std::ifstream file(path, std::ios::binary);
            std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
            if (callback) {
                callback(data);
            }
        } catch (const std::exception&) {
            if (callback) {
                callback({});
            }
        }
    }

    void ArabicIO::AsyncIO::writeFileAsync(const std::string& path, const std::string& content, FileCallback callback) {
        try {
            std::ofstream file(path);
            file << content;
            if (callback) {
                callback("تم كتابة الملف بنجاح");
            }
        } catch (const std::exception& e) {
            if (callback) {
                callback("خطأ في كتابة الملف: " + std::string(e.what()));
            }
        }
    }

    void ArabicIO::AsyncIO::writeBytesAsync(const std::string& path, const std::vector<uint8_t>& data, DataCallback callback) {
        try {
            std::ofstream file(path, std::ios::binary);
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
            if (callback) {
                callback(data); // إرجاع نفس البيانات للتأكيد
            }
        } catch (const std::exception&) {
            if (callback) {
                callback({});
            }
        }
    }

    // تنفيذ مراقبة الملفات (تبسيط)
    void ArabicIO::AsyncIO::watchFile(const std::string& path, FileCallback callback) {
        // في الإصدار الكامل سيتم تنفيذ مراقبة فعلية للملفات
        if (callback) {
            callback("مراقبة الملفات غير متاحة في هذا الإصدار");
        }
    }

    void ArabicIO::AsyncIO::watchDirectory(const std::string& path, FileCallback callback) {
        if (callback) {
            callback("مراقبة المجلدات غير متاحة في هذا الإصدار");
        }
    }

    // ══════════════════════════════════════════════════════════════
    // 📈 تنفيذ مراقب الأداء
    // ══════════════════════════════════════════════════════════════

    void ArabicIO::PerformanceMonitor::startOperation(const std::string& operation, size_t dataSize) {
        activeOperations.push_back({operation, std::chrono::steady_clock::now(), dataSize});
    }

    void ArabicIO::PerformanceMonitor::endOperation(const std::string& operation) {
        auto now = std::chrono::steady_clock::now();

        for (auto it = activeOperations.begin(); it != activeOperations.end(); ++it) {
            if (it->operation == operation) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->start).count();
                operationTimes[operation].push_back(duration);
                activeOperations.erase(it);
                break;
            }
        }
    }

    double ArabicIO::PerformanceMonitor::getAverageTime(const std::string& operation) const {
        auto it = operationTimes.find(operation);
        if (it == operationTimes.end() || it->second.empty()) {
            return 0.0;
        }

        double sum = 0.0;
        for (double time : it->second) {
            sum += time;
        }
        return sum / it->second.size();
    }

    size_t ArabicIO::PerformanceMonitor::getTotalOperations(const std::string& operation) const {
        auto it = operationTimes.find(operation);
        return (it != operationTimes.end()) ? it->second.size() : 0;
    }

    double ArabicIO::PerformanceMonitor::getThroughput(const std::string& operation) const {
        auto it = operationTimes.find(operation);
        if (it == operationTimes.end() || it->second.empty()) {
            return 0.0;
        }

        // حساب معدل النقل بناءً على البيانات المسجلة
        // (تبسيط - في الإصدار الكامل سيكون أدق)
        return 1000.0 / getAverageTime(operation); // عمليات في الثانية تقريباً
    }

    std::vector<std::string> ArabicIO::PerformanceMonitor::getActiveOperations() const {
        std::vector<std::string> operations;
        for (const auto& op : activeOperations) {
            operations.push_back(op.operation);
        }
        return operations;
    }

    std::vector<std::string> ArabicIO::PerformanceMonitor::getPerformanceReport() const {
        std::vector<std::string> report;
        report.push_back("=== تقرير أداء I/O ===");

        for (const auto& pair : operationTimes) {
            const std::string& operation = pair.first;
            const auto& times = pair.second;

            if (!times.empty()) {
                double avg = getAverageTime(operation);
                double throughput = getThroughput(operation);

                report.push_back("العملية: " + operation);
                report.push_back("  العدد الإجمالي: " + std::to_string(times.size()));
                report.push_back("  المتوسط الزمني: " + std::to_string(avg) + " ms");
                report.push_back("  معدل الأداء: " + std::to_string(throughput) + " op/s");
            }
        }

        if (operationTimes.empty()) {
            report.push_back("لا توجد عمليات مسجلة");
        }

        return report;
    }

    // ══════════════════════════════════════════════════════════════
    // 🎯 تنفيذ واجهة الاستخدام العامة
    // ══════════════════════════════════════════════════════════════

    ArabicIO& ArabicIO::getInstance() {
        static ArabicIO instance;
        return instance;
    }

    ArabicIO::File& ArabicIO::openFile(const std::string& path, const std::string& mode) {
        auto it = openFiles.find(path);
        if (it == openFiles.end()) {
            auto file = std::make_unique<File>(path, mode);
            it = openFiles.emplace(path, std::move(file)).first;
        }
        return *it->second;
    }

    bool ArabicIO::closeFile(const std::string& path) {
        auto it = openFiles.find(path);
        if (it != openFiles.end()) {
            openFiles.erase(it);
            return true;
        }
        return false;
    }

    std::string ArabicIO::readConsoleLine() {
        return Console::readLine();
    }

    void ArabicIO::writeConsole(const std::string& text) {
        Console::write(text);
    }

    void ArabicIO::writeConsoleLine(const std::string& text) {
        Console::writeLine(text);
    }

    std::string ArabicIO::readFileText(const std::string& path) {
        perfMonitor.startOperation("read_file_text");
        File file(path, "r");
        std::string content = file.readAll();
        perfMonitor.endOperation("read_file_text");
        return content;
    }

    bool ArabicIO::writeFileText(const std::string& path, const std::string& content) {
        perfMonitor.startOperation("write_file_text", content.size());
        File file(path, "w");
        bool success = file.write(content);
        perfMonitor.endOperation("write_file_text");
        return success;
    }

    std::vector<uint8_t> ArabicIO::readFileBytes(const std::string& path) {
        perfMonitor.startOperation("read_file_bytes");
        File file(path, "rb");
        auto data = file.readBytes();
        perfMonitor.endOperation("read_file_bytes");
        return data;
    }

    bool ArabicIO::writeFileBytes(const std::string& path, const std::vector<uint8_t>& data) {
        perfMonitor.startOperation("write_file_bytes", data.size());
        File file(path, "wb");
        bool success = file.writeBytes(data);
        perfMonitor.endOperation("write_file_bytes");
        return success;
    }

    std::vector<std::string> ArabicIO::getIOStats() const {
        std::vector<std::string> stats;
        stats.push_back("=== إحصائيات I/O ===");
        stats.push_back("الملفات المفتوحة: " + std::to_string(openFiles.size()));

        auto perfStats = perfMonitor.getPerformanceReport();
        stats.insert(stats.end(), perfStats.begin(), perfStats.end());

        return stats;
    }

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة للمكتبة القياسية
    // ══════════════════════════════════════════════════════════════

    namespace IO {

        std::string اقرأ_ملف(const std::string& مسار) {
            return ArabicIO::getInstance().readFileText(مسار);
        }

        bool اكتب_ملف(const std::string& مسار, const std::string& محتوى) {
            return ArabicIO::getInstance().writeFileText(مسار, محتوى);
        }

        std::vector<std::string> اقرأ_أسطر(const std::string& مسار) {
            ArabicIO::File file(مسار, "r");
            return file.readLines();
        }

        std::string اقرأ_سطر() {
            return ArabicIO::getInstance().readConsoleLine();
        }

        void اطبع(const std::string& نص) {
            ArabicIO::getInstance().writeConsole(نص);
        }

        void اطبع_سطر(const std::string& نص) {
            ArabicIO::getInstance().writeConsoleLine(نص);
        }

        bool ملف_موجود(const std::string& مسار) {
            return ArabicIO::FileOperations::exists(مسار);
        }

        bool أنشئ_مجلد(const std::string& مسار) {
            return ArabicIO::FileOperations::createDirectory(مسار);
        }

        std::vector<std::string> قائمة_مجلد(const std::string& مسار) {
            return ArabicIO::FileOperations::listDirectory(مسار);
        }

        std::string رقم_إلى_نص(int رقم) {
            return ArabicIO::DataConverter::intToString(رقم);
        }

        std::string عدد_إلى_نص(double عدد) {
            return ArabicIO::DataConverter::doubleToString(عدد);
        }

        int نص_إلى_رقم(const std::string& نص) {
            return ArabicIO::DataConverter::stringToInt(نص);
        }

        double نص_إلى_عدد(const std::string& نص) {
            return ArabicIO::DataConverter::stringToDouble(نص);
        }

    } // namespace IO

} // namespace ArabicLanguage::StdLib
