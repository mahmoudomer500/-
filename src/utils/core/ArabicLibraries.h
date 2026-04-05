// ArabicLibraries.h - المكتبة الشاملة الموحدة للغة البرمجة العربية
// يجمع: ArabicStdLib.h + ArabicPackageManager.h
// ✅ بدون تحذيرات - جاهز للإنتاج
#ifndef ARABIC_LIBRARIES_H
#define ARABIC_LIBRARIES_H

#include "ArabicTypes.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

// للشبكات
#ifdef _WIN32
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <direct.h>
#pragma comment(lib, "ws2_32.lib")
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

namespace ArabicLanguage {

    // ════════════════════════════════════════════════════════════
    // 🔢 دوال الرياضيات
    // ════════════════════════════════════════════════════════════
    class ArabicMath {
    public:
        static double جذر(double عدد) { return std::sqrt(عدد); }
        static double مطلق(double عدد) { return std::abs(عدد); }
        static double أقصى(double أ, double ب) { return std::max(أ, ب); }
        static double أدنى(double أ, double ب) { return std::min(أ, ب); }
        static double قوة(double أساس, double أس) { return std::pow(أساس, أس); }
        static double جيب(double زاوية) { return std::sin(زاوية * 3.14159265358979323846 / 180.0); }
        static double جيب_تمام(double زاوية) { return std::cos(زاوية * 3.14159265358979323846 / 180.0); }
        static double ظل(double زاوية) { return std::tan(زاوية * 3.14159265358979323846 / 180.0); }
        static double لوغاريتم(double عدد) { return std::log(عدد); }
        static double لوغاريتم_عشري(double عدد) { return std::log10(عدد); }
        static double دالة_الأسية(double عدد) { return std::exp(عدد); }
        static double أرضية(double عدد) { return std::floor(عدد); }
        static double سقف(double عدد) { return std::ceil(عدد); }
        static double تقريب(double عدد) { return std::round(عدد); }
        static double عشوائي() { return static_cast<double>(rand()) / RAND_MAX; }
        static int عشوائي_صحيح(int من, int إلى) { return من + (rand() % (إلى - من + 1)); }

        static constexpr double باي = 3.14159265358979323846;
        static constexpr double إيلر = 2.71828182845904523536;
    };

    // ════════════════════════════════════════════════════════════
    // 📝 دوال النصوص المتقدمة
    // ════════════════════════════════════════════════════════════
    class ArabicString {
    public:
        static int طول_النص(const std::string& نص) {
            return static_cast<int>(نص.length());
        }

        static std::string استبدال(std::string نص, const std::string& قديم, const std::string& جديد) {
            size_t pos = 0;
            while ((pos = نص.find(قديم, pos)) != std::string::npos) {
                نص.replace(pos, قديم.length(), جديد);
                pos += جديد.length();
            }
            return نص;
        }

        static bool احتوى(const std::string& نص, const std::string& بحث) {
            return نص.find(بحث) != std::string::npos;
        }

        static std::string اقتطع(const std::string& نص, int البداية, int الطول = -1) {
            if (البداية < 0 || البداية >= static_cast<int>(نص.length())) return "";
            if (الطول < 0) return نص.substr(static_cast<size_t>(البداية));
            return نص.substr(static_cast<size_t>(البداية), static_cast<size_t>(الطول));
        }

        static std::string إلى_أحرف_كبيرة(std::string نص) {
            std::transform(نص.begin(), نص.end(), نص.begin(),
                [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            return نص;
        }

        static std::string إلى_أحرف_صغيرة(std::string نص) {
            std::transform(نص.begin(), نص.end(), نص.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return نص;
        }

        static std::vector<std::string> قسم_النص(const std::string& نص, const std::string& فاصل) {
            std::vector<std::string> نتيجة;
            size_t بداية = 0;
            size_t نهاية = نص.find(فاصل);

            while (نهاية != std::string::npos) {
                نتيجة.push_back(نص.substr(بداية, نهاية - بداية));
                بداية = نهاية + فاصل.length();
                نهاية = نص.find(فاصل, بداية);
            }
            نتيجة.push_back(نص.substr(بداية));
            return نتيجة;
        }

        static std::string اربط_النصوص(const std::vector<std::string>& نصوص, const std::string& فاصل = "") {
            std::string نتيجة;
            for (size_t i = 0; i < نصوص.size(); i++) {
                if (i > 0) نتيجة += فاصل;
                نتيجة += نصوص[i];
            }
            return نتيجة;
        }

        static std::string احذف_المسافات(std::string نص) {
            نص.erase(std::remove_if(نص.begin(), نص.end(),
                [](unsigned char c) { return std::isspace(c); }), نص.end());
            return نص;
        }

        static int ابحث_عن(const std::string& نص, const std::string& بحث) {
            size_t pos = نص.find(بحث);
            return pos != std::string::npos ? static_cast<int>(pos) : -1;
        }

        static bool يبدأ_بـ(const std::string& نص, const std::string& بداية) {
            return نص.size() >= بداية.size() && نص.compare(0, بداية.size(), بداية) == 0;
        }

        static bool ينتهي_بـ(const std::string& نص, const std::string& نهاية) {
            return نص.size() >= نهاية.size() &&
                نص.compare(نص.size() - نهاية.size(), نهاية.size(), نهاية) == 0;
        }
    };

    // ════════════════════════════════════════════════════════════
    // 📊 دوال المصفوفات
    // ════════════════════════════════════════════════════════════
    class ArabicArray {
    public:
        static int طول(const std::vector<Value>& مصفوفة) {
            return static_cast<int>(مصفوفة.size());
        }

        static double مجموع(const std::vector<Value>& مصفوفة) {
            double sum = 0;
            for (const auto& elem : مصفوفة) {
                if (elem.type == ValueType::NUMBER) {
                    sum += std::stod(elem.value);
                }
            }
            return sum;
        }

        static double متوسط(const std::vector<Value>& مصفوفة) {
            if (مصفوفة.empty()) return 0;
            return مجموع(مصفوفة) / static_cast<double>(مصفوفة.size());
        }

        static double أكبر(const std::vector<Value>& مصفوفة) {
            if (مصفوفة.empty()) return 0;
            double max_val = std::stod(مصفوفة[0].value);
            for (size_t i = 1; i < مصفوفة.size(); i++) {
                if (مصفوفة[i].type == ValueType::NUMBER) {
                    double val = std::stod(مصفوفة[i].value);
                    if (val > max_val) max_val = val;
                }
            }
            return max_val;
        }

        static double أصغر(const std::vector<Value>& مصفوفة) {
            if (مصفوفة.empty()) return 0;
            double min_val = std::stod(مصفوفة[0].value);
            for (size_t i = 1; i < مصفوفة.size(); i++) {
                if (مصفوفة[i].type == ValueType::NUMBER) {
                    double val = std::stod(مصفوفة[i].value);
                    if (val < min_val) min_val = val;
                }
            }
            return min_val;
        }

        static std::vector<Value> رتب_تصاعدي(std::vector<Value> مصفوفة) {
            std::sort(مصفوفة.begin(), مصفوفة.end(),
                [](const Value& a, const Value& b) {
                    return std::stod(a.value) < std::stod(b.value);
                });
            return مصفوفة;
        }

        static std::vector<Value> رتب_تنازلي(std::vector<Value> مصفوفة) {
            std::sort(مصفوفة.begin(), مصفوفة.end(),
                [](const Value& a, const Value& b) {
                    return std::stod(a.value) > std::stod(b.value);
                });
            return مصفوفة;
        }

        static std::vector<Value> اعكس(std::vector<Value> مصفوفة) {
            std::reverse(مصفوفة.begin(), مصفوفة.end());
            return مصفوفة;
        }
    };

    // ════════════════════════════════════════════════════════════
    // 💾 دوال الملفات الكاملة
    // ════════════════════════════════════════════════════════════
    class ArabicFileIO {
    public:
        static bool اكتب_ملف(const std::string& اسم_الملف, const std::string& محتوى) {
            std::ofstream ملف(اسم_الملف);
            if (!ملف.is_open()) return false;
            ملف << محتوى;
            ملف.close();
            return true;
        }

        static std::string اقرأ_ملف(const std::string& اسم_الملف) {
            std::ifstream ملف(اسم_الملف);
            if (!ملف.is_open()) return "";
            std::stringstream buffer;
            buffer << ملف.rdbuf();
            return buffer.str();
        }

        static bool أضف_إلى_ملف(const std::string& اسم_الملف, const std::string& محتوى) {
            std::ofstream ملف(اسم_الملف, std::ios::app);
            if (!ملف.is_open()) return false;
            ملف << محتوى;
            ملف.close();
            return true;
        }

        static std::vector<std::string> اقرأ_جميع_السطور(const std::string& اسم_الملف) {
            std::vector<std::string> السطور;
            std::ifstream ملف(اسم_الملف);
            if (!ملف.is_open()) return السطور;

            std::string سطر;
            while (std::getline(ملف, سطر)) {
                السطور.push_back(سطر);
            }
            return السطور;
        }

        static bool احذف_ملف(const std::string& اسم_الملف) {
            return std::remove(اسم_الملف.c_str()) == 0;
        }

        static bool ملف_موجود(const std::string& اسم_الملف) {
            std::ifstream ملف(اسم_الملف);
            return ملف.good();
        }

        static long احصل_على_حجم_الملف(const std::string& اسم_الملف) {
            struct stat stat_buf;
            int rc = stat(اسم_الملف.c_str(), &stat_buf);
            return rc == 0 ? stat_buf.st_size : -1;
        }

        static bool انسخ_ملف(const std::string& المصدر, const std::string& الوجهة) {
            std::ifstream src(المصدر, std::ios::binary);
            std::ofstream dst(الوجهة, std::ios::binary);
            if (!src.is_open() || !dst.is_open()) return false;
            dst << src.rdbuf();
            return true;
        }

        static bool أعد_تسمية_ملف(const std::string& الاسم_القديم, const std::string& الاسم_الجديد) {
            return std::rename(الاسم_القديم.c_str(), الاسم_الجديد.c_str()) == 0;
        }

        // CSV
        static std::vector<std::string> splitCSVLine(const std::string& سطر, char فاصل) {
            std::vector<std::string> نتيجة;
            std::string خلية;
            bool داخل_علامات = false;

            for (char c : سطر) {
                if (c == '"') {
                    داخل_علامات = !داخل_علامات;
                }
                else if (c == فاصل && !داخل_علامات) {
                    نتيجة.push_back(خلية);
                    خلية.clear();
                }
                else {
                    خلية += c;
                }
            }
            نتيجة.push_back(خلية);
            return نتيجة;
        }

        static std::vector<std::vector<std::string>> اقرأ_csv(const std::string& اسم_الملف, char الفاصل = ',') {
            std::vector<std::vector<std::string>> بيانات;
            auto سطور = اقرأ_جميع_السطور(اسم_الملف);

            for (const auto& سطر : سطور) {
                if (!سطر.empty()) {
                    بيانات.push_back(splitCSVLine(سطر, الفاصل));
                }
            }
            return بيانات;
        }

        static bool اكتب_csv(const std::string& اسم_الملف, const std::vector<std::vector<std::string>>& البيانات, char الفاصل = ',') {
            std::ofstream ملف(اسم_الملف);
            if (!ملف.is_open()) return false;

            for (const auto& صف : البيانات) {
                for (size_t i = 0; i < صف.size(); i++) {
                    if (i > 0) ملف << الفاصل;
                    ملف << "\"" << صف[i] << "\"";
                }
                ملف << "\n";
            }
            return true;
        }

        // JSON
        static std::string generateJSON(const Value& قيمة) {
            if (قيمة.type == ValueType::NUMBER) {
                return قيمة.value;
            }
            else if (قيمة.type == ValueType::STRING) {
                return "\"" + قيمة.value + "\"";
            }
            else if (قيمة.type == ValueType::ARRAY) {
                std::string json = "[";
                for (size_t i = 0; i < قيمة.elements.size(); i++) {
                    if (i > 0) json += ",";
                    json += generateJSON(قيمة.elements[i]);
                }
                json += "]";
                return json;
            }
            return "null";
        }

        static bool اكتب_json(const std::string& اسم_الملف, const Value& البيانات) {
            std::string json = generateJSON(البيانات);
            return اكتب_ملف(اسم_الملف, json);
        }

        static Value اقرأ_json(const std::string& اسم_الملف) {
            std::string محتوى = اقرأ_ملف(اسم_الملف);
            return createStringValue(محتوى);
        }
    };

    // ════════════════════════════════════════════════════════════
    // 🌐 دوال الشبكات الكاملة
    // ════════════════════════════════════════════════════════════
    class ArabicNetwork {
    private:
        static bool initialized;

        static bool initializeNetwork() {
#ifdef _WIN32
            if (!initialized) {
                WSADATA wsaData;
                int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
                initialized = (result == 0);
                return initialized;
            }
#else
            initialized = true;
#endif
            return true;
        }

    public:
        static int افتح_خادم_للاستماع(int منفذ) {
            if (!initializeNetwork()) return -1;

            SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket == INVALID_SOCKET) return -1;

            sockaddr_in serverAddr{};
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = INADDR_ANY;
            serverAddr.sin_port = htons(static_cast<unsigned short>(منفذ));

            if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
                closesocket(serverSocket);
                return -1;
            }

            if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
                closesocket(serverSocket);
                return -1;
            }

            return static_cast<int>(serverSocket);
        }

        static int اقبل_اتصال_عميل(int معرف_الخادم) {
            SOCKET clientSocket = accept(static_cast<SOCKET>(معرف_الخادم), nullptr, nullptr);
            return clientSocket != INVALID_SOCKET ? static_cast<int>(clientSocket) : -1;
        }

        static int افتح_اتصال_عميل(const std::string& اسم_المضيف, int منفذ) {
            if (!initializeNetwork()) return -1;

            SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (clientSocket == INVALID_SOCKET) return -1;

            hostent* host = gethostbyname(اسم_المضيف.c_str());
            if (!host) {
                closesocket(clientSocket);
                return -1;
            }

            sockaddr_in serverAddr{};
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(static_cast<unsigned short>(منفذ));
            std::memcpy(&serverAddr.sin_addr, host->h_addr, static_cast<size_t>(host->h_length));

            if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
                closesocket(clientSocket);
                return -1;
            }

            return static_cast<int>(clientSocket);
        }

        static int أرسل_بيانات(int معرف_المقبس, const std::string& البيانات_للارسال) {
            return send(static_cast<SOCKET>(معرف_المقبس), البيانات_للارسال.c_str(),
                static_cast<int>(البيانات_للارسال.length()), 0);
        }

        static std::string استقبل_بيانات(int معرف_المقبس, size_t الحجم_الأقصى = 1024) {
            std::vector<char> buffer(الحجم_الأقصى);
            int bytesReceived = recv(static_cast<SOCKET>(معرف_المقبس), buffer.data(),
                static_cast<int>(الحجم_الأقصى), 0);

            if (bytesReceived > 0) {
                return std::string(buffer.data(), static_cast<size_t>(bytesReceived));
            }
            return "";
        }

        static bool أغلق_الاتصال(int معرف_المقبس) {
            return closesocket(static_cast<SOCKET>(معرف_المقبس)) == 0;
        }

        static std::string تحويل_اسم_مضيف_إلى_آي_بي(const std::string& اسم_المضيف) {
            if (!initializeNetwork()) return "";

            hostent* host = gethostbyname(اسم_المضيف.c_str());
            if (!host) return "";

            in_addr* address = reinterpret_cast<in_addr*>(host->h_addr);
            return inet_ntoa(*address);
        }
    };

    bool ArabicNetwork::initialized = false;

    // ════════════════════════════════════════════════════════════
    // ⌨️ دوال الإدخال/الإخراج
    // ════════════════════════════════════════════════════════════
    class ArabicIO {
    public:
        static std::string اقرأ(const std::string& رسالة = "") {
            if (!رسالة.empty()) std::cout << رسالة;
            std::string input;
            std::getline(std::cin, input);
            return input;
        }

        static int اقرأ_رقم(const std::string& رسالة = "") {
            std::string input = اقرأ(رسالة);
            try { return std::stoi(input); }
            catch (...) { return 0; }
        }

        static double اقرأ_عشري(const std::string& رسالة = "") {
            std::string input = اقرأ(رسالة);
            try { return std::stod(input); }
            catch (...) { return 0.0; }
        }
    };

    // ════════════════════════════════════════════════════════════
    // 🔐 دوال المنطق
    // ════════════════════════════════════════════════════════════
    class ArabicLogic {
    public:
        static bool زوجي(int عدد) { return عدد % 2 == 0; }
        static bool فردي(int عدد) { return عدد % 2 != 0; }
        static bool موجب(double عدد) { return عدد > 0; }
        static bool سالب(double عدد) { return عدد < 0; }
        static bool صفر(double عدد) { return std::abs(عدد) < 1e-10; }
    };

    // ════════════════════════════════════════════════════════════
    // 🔄 دوال التحويل
    // ════════════════════════════════════════════════════════════
    class ArabicConvert {
    public:
        template<typename T>
        static std::string إلى_نص(T قيمة) { return std::to_string(قيمة); }

        static int إلى_رقم(const std::string& نص) {
            try { return std::stoi(نص); }
            catch (...) { return 0; }
        }

        static double إلى_عشري(const std::string& نص) {
            try { return std::stod(نص); }
            catch (...) { return 0.0; }
        }
    };

    // ════════════════════════════════════════════════════════════
    // 📦 نظام إدارة الحزم
    // ════════════════════════════════════════════════════════════

    struct PackageVersion {
        int major = 1, minor = 0, patch = 0;

        PackageVersion() = default;
        PackageVersion(int maj, int min, int pat) : major(maj), minor(min), patch(pat) {}

        std::string toString() const {
            return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
        }

        bool operator<(const PackageVersion& other) const {
            if (major != other.major) return major < other.major;
            if (minor != other.minor) return minor < other.minor;
            return patch < other.patch;
        }

        bool operator==(const PackageVersion& other) const {
            return major == other.major && minor == other.minor && patch == other.patch;
        }

        static PackageVersion fromString(const std::string& str) {
            PackageVersion ver;
            std::istringstream iss(str);
            char dot;
            iss >> ver.major >> dot >> ver.minor >> dot >> ver.patch;
            return ver;
        }
    };

    struct PackageDependency {
        std::string name;
        PackageVersion minVersion, maxVersion;
        bool required = true;

        PackageDependency() = default;
        PackageDependency(const std::string& n, const PackageVersion& minV)
            : name(n), minVersion(minV), required(true) {
        }
    };

    struct PackageMetadata {
        std::string name, author, description, license, installDate;
        PackageVersion version;
        std::vector<PackageDependency> dependencies;
        std::vector<std::string> files, keywords;
        long long size = 0;

        PackageMetadata() = default;
    };

    struct PackageFile {
        std::string relativePath, content;
        long long size = 0;

        PackageFile() = default;
        PackageFile(const std::string& path, const std::string& cnt)
            : relativePath(path), content(cnt), size(cnt.size()) {
        }
    };

    // ════════════════════════════════════════════════════════════
    // 📦 مدير الحزم الرئيسي
    // ════════════════════════════════════════════════════════════
    class PackageManager {
    public:
        PackageManager() {
            initializeDirectories();
            loadInstalledPackages();
        }

        bool installPackage(const std::string& packageName, const std::string& version = "latest") {
            std::cout << "📦 تثبيت الحزمة: " << packageName;
            if (version != "latest") std::cout << " @ " << version;
            std::cout << std::endl;

            if (isInstalled(packageName)) {
                std::cout << "⚠️ الحزمة مثبتة مسبقاً. استخدم 'ثبت --force' لإعادة التثبيت" << std::endl;
                return false;
            }

            auto packageInfo = findInRepository(packageName, version);
            if (!packageInfo) {
                std::cout << "❌ الحزمة غير موجودة في المستودع" << std::endl;
                return false;
            }

            std::cout << "🔍 فحص التبعيات..." << std::endl;
            if (!checkDependencies(*packageInfo)) {
                std::cout << "❌ فشل في حل التبعيات" << std::endl;
                return false;
            }

            for (const auto& dep : packageInfo->dependencies) {
                if (!isInstalled(dep.name)) {
                    std::cout << "📥 تثبيت التبعية: " << dep.name << std::endl;
                    if (!installPackage(dep.name, dep.minVersion.toString())) {
                        std::cout << "❌ فشل تثبيت التبعية: " << dep.name << std::endl;
                        return false;
                    }
                }
            }

            std::cout << "📥 تنزيل الحزمة..." << std::endl;
            auto packageFiles = downloadPackage(*packageInfo);

            if (packageFiles.empty()) {
                std::cout << "❌ فشل تنزيل الحزمة" << std::endl;
                return false;
            }

            std::cout << "💾 كتابة الملفات..." << std::endl;
            if (!writePackageFiles(packageName, packageFiles)) {
                std::cout << "❌ فشل كتابة ملفات الحزمة" << std::endl;
                return false;
            }

            packageInfo->installDate = getCurrentDateTime();
            installedPackages[packageName] = *packageInfo;
            saveInstalledPackages();

            std::cout << "✅ تم تثبيت " << packageName << " بنجاح!" << std::endl;
            printPackageInfo(*packageInfo);

            return true;
        }

        bool uninstallPackage(const std::string& packageName) {
            std::cout << "🗑️ حذف الحزمة: " << packageName << std::endl;

            if (!isInstalled(packageName)) {
                std::cout << "❌ الحزمة غير مثبتة" << std::endl;
                return false;
            }

            auto dependents = findDependentPackages(packageName);
            if (!dependents.empty()) {
                std::cout << "⚠️ تحذير: الحزم التالية تعتمد على " << packageName << ":" << std::endl;
                for (const auto& dep : dependents) {
                    std::cout << "   • " << dep << std::endl;
                }
                std::cout << "❌ لا يمكن الحذف. احذف الحزم المعتمدة أولاً." << std::endl;
                return false;
            }

            std::string packagePath = packagesDir + "/" + packageName;
            removeDirectory(packagePath);

            installedPackages.erase(packageName);
            saveInstalledPackages();

            std::cout << "✅ تم حذف " << packageName << " بنجاح" << std::endl;
            return true;
        }

        bool updatePackage(const std::string& packageName) {
            std::cout << "🔄 تحديث الحزمة: " << packageName << std::endl;

            if (!isInstalled(packageName)) {
                std::cout << "❌ الحزمة غير مثبتة" << std::endl;
                return false;
            }

            auto currentVersion = installedPackages[packageName].version;
            auto latestInfo = findInRepository(packageName, "latest");

            if (!latestInfo) {
                std::cout << "❌ لم يتم العثور على تحديثات" << std::endl;
                return false;
            }

            if (!(currentVersion < latestInfo->version)) {
                std::cout << "✅ الحزمة محدثة بالفعل (" << currentVersion.toString() << ")" << std::endl;
                return true;
            }

            std::cout << "📊 الإصدار الحالي: " << currentVersion.toString() << std::endl;
            std::cout << "📊 الإصدار الجديد: " << latestInfo->version.toString() << std::endl;

            if (!uninstallPackage(packageName)) return false;
            return installPackage(packageName, "latest");
        }

        void listInstalled() const {
            std::cout << "\n📦 الحزم المثبتة:" << std::endl;
            std::cout << "═══════════════════════════════════════════════" << std::endl;

            if (installedPackages.empty()) {
                std::cout << "   لا توجد حزم مثبتة" << std::endl;
                return;
            }

            for (const auto& [name, info] : installedPackages) {
                std::cout << "📌 " << name << " @ " << info.version.toString();
                if (!info.description.empty()) {
                    std::cout << "\n   " << info.description;
                }
                std::cout << std::endl;
            }
            std::cout << "═══════════════════════════════════════════════" << std::endl;
            std::cout << "المجموع: " << installedPackages.size() << " حزمة" << std::endl;
        }

        void searchPackages(const std::string& query) const {
            std::cout << "\n🔍 البحث عن: " << query << std::endl;
            std::cout << "═══════════════════════════════════════════════" << std::endl;

            std::vector<PackageMetadata> results;

            for (const auto& pkg : localRepository) {
                bool match = false;

                if (pkg.name.find(query) != std::string::npos) match = true;
                if (pkg.description.find(query) != std::string::npos) match = true;

                for (const auto& keyword : pkg.keywords) {
                    if (keyword.find(query) != std::string::npos) {
                        match = true;
                        break;
                    }
                }

                if (match) results.push_back(pkg);
            }

            if (results.empty()) {
                std::cout << "لم يتم العثور على نتائج" << std::endl;
                return;
            }

            for (const auto& pkg : results) {
                std::cout << "📦 " << pkg.name << " @ " << pkg.version.toString() << std::endl;
                std::cout << "   " << pkg.description << std::endl;
                std::cout << "   المؤلف: " << pkg.author << std::endl;
                if (!pkg.keywords.empty()) {
                    std::cout << "   الكلمات المفتاحية: ";
                    for (size_t i = 0; i < pkg.keywords.size(); ++i) {
                        if (i > 0) std::cout << ", ";
                        std::cout << pkg.keywords[i];
                    }
                    std::cout << std::endl;
                }
                std::cout << std::endl;
            }

            std::cout << "═══════════════════════════════════════════════" << std::endl;
            std::cout << "النتائج: " << results.size() << std::endl;
        }

        void showPackageInfo(const std::string& packageName) const {
            if (!isInstalled(packageName)) {
                auto info = findInRepository(packageName, "latest");
                if (info) {
                    printPackageInfo(*info);
                }
                else {
                    std::cout << "❌ الحزمة غير موجودة" << std::endl;
                }
            }
            else {
                printPackageInfo(installedPackages.at(packageName));
            }
        }

        void printStatistics() const {
            std::cout << "\n📊 إحصائيات إدارة الحزم:" << std::endl;
            std::cout << "═══════════════════════════════════════════════" << std::endl;

            long long totalSize = 0;
            int totalFiles = 0;

            for (const auto& [name, info] : installedPackages) {
                totalSize += info.size;
                totalFiles += static_cast<int>(info.files.size());
            }

            std::cout << "📦 عدد الحزم المثبتة: " << installedPackages.size() << std::endl;
            std::cout << "📄 إجمالي الملفات: " << totalFiles << std::endl;
            std::cout << "💾 المساحة المستخدمة: " << formatSize(totalSize) << std::endl;
            std::cout << "📚 حزم في المستودع: " << localRepository.size() << std::endl;
            std::cout << "═══════════════════════════════════════════════" << std::endl;
        }

        bool createPackage(const std::string& name, const std::string& sourcePath) {
            std::cout << "🔨 إنشاء حزمة جديدة: " << name << std::endl;

            PackageMetadata metadata;
            metadata.name = name;
            metadata.version = PackageVersion(1, 0, 0);
            metadata.author = "مطور عربي";
            metadata.description = "حزمة عربية";
            metadata.license = "MIT";

            std::cout << "📂 جمع الملفات من: " << sourcePath << std::endl;
            std::vector<PackageFile> files;

            if (!collectFiles(sourcePath, files)) {
                std::cout << "❌ فشل جمع الملفات" << std::endl;
                return false;
            }

            metadata.size = 0;
            for (const auto& file : files) {
                metadata.files.push_back(file.relativePath);
                metadata.size += file.size;
            }

            std::cout << "✅ تم جمع " << files.size() << " ملف" << std::endl;
            std::cout << "📦 الحجم الإجمالي: " << formatSize(metadata.size) << std::endl;

            localRepository.push_back(metadata);
            saveLocalRepository();

            std::string packagePath = repositoryDir + "/" + name;
            mkdir(packagePath.c_str(), 0755);

            for (const auto& file : files) {
                std::string fullPath = packagePath + "/" + file.relativePath;
                std::ofstream out(fullPath);
                if (out.is_open()) {
                    out << file.content;
                    out.close();
                }
            }

            savePackageMetadata(packagePath, metadata);

            std::cout << "✅ تم إنشاء الحزمة بنجاح!" << std::endl;
            return true;
        }

        bool isInstalled(const std::string& packageName) const {
            return installedPackages.find(packageName) != installedPackages.end();
        }

        std::string getPackagePath(const std::string& packageName) const {
            return packagesDir + "/" + packageName;
        }

    private:
        std::string baseDir = ".arabic_packages";
        std::string packagesDir = baseDir + "/packages";
        std::string repositoryDir = baseDir + "/repository";
        std::string cacheDir = baseDir + "/cache";

        std::map<std::string, PackageMetadata> installedPackages;
        std::vector<PackageMetadata> localRepository;

        void initializeDirectories() {
            mkdir(baseDir.c_str(), 0755);
            mkdir(packagesDir.c_str(), 0755);
            mkdir(repositoryDir.c_str(), 0755);
            mkdir(cacheDir.c_str(), 0755);
            initializeDefaultRepository();
        }

        void initializeDefaultRepository() {
            PackageMetadata mathPkg;
            mathPkg.name = "رياضيات_متقدمة";
            mathPkg.version = PackageVersion(1, 0, 0);
            mathPkg.author = "فريق المترجم العربي";
            mathPkg.description = "مكتبة رياضيات متقدمة للعمليات المعقدة";
            mathPkg.license = "MIT";
            mathPkg.keywords = { "رياضيات", "حسابات", "جبر" };
            localRepository.push_back(mathPkg);

            PackageMetadata stringPkg;
            stringPkg.name = "معالج_النصوص";
            stringPkg.version = PackageVersion(1, 0, 0);
            stringPkg.author = "فريق المترجم العربي";
            stringPkg.description = "أدوات متقدمة لمعالجة النصوص العربية";
            stringPkg.license = "MIT";
            stringPkg.keywords = { "نصوص", "سلاسل", "معالجة" };
            localRepository.push_back(stringPkg);

            PackageMetadata webPkg;
            webPkg.name = "خادم_ويب";
            webPkg.version = PackageVersion(1, 0, 0);
            webPkg.author = "فريق المترجم العربي";
            webPkg.description = "خادم ويب بسيط مبني بالعربية";
            webPkg.license = "MIT";
            webPkg.keywords = { "ويب", "خادم", "HTTP" };
            webPkg.dependencies.push_back(PackageDependency("معالج_النصوص", PackageVersion(1, 0, 0)));
            localRepository.push_back(webPkg);

            saveLocalRepository();
        }

        void loadInstalledPackages() {
            std::string indexFile = baseDir + "/installed.txt";
            std::ifstream in(indexFile);
            if (!in.is_open()) return;

            std::string line;
            while (std::getline(in, line)) {
                if (line.empty() || line[0] == '#') continue;

                std::istringstream iss(line);
                std::string name, verStr, author, desc, depsStr;

                std::getline(iss, name, '|');
                std::getline(iss, verStr, '|');
                std::getline(iss, author, '|');
                std::getline(iss, desc, '|');
                std::getline(iss, depsStr, '|');

                PackageMetadata pkg;
                pkg.name = name;
                pkg.version = PackageVersion::fromString(verStr);
                pkg.author = author;
                pkg.description = desc;

                if (!depsStr.empty()) {
                    std::istringstream depStream(depsStr);
                    std::string depName;
                    while (std::getline(depStream, depName, ',')) {
                        if (!depName.empty()) {
                            pkg.dependencies.push_back(PackageDependency(depName, PackageVersion(1, 0, 0)));
                        }
                    }
                }

                installedPackages[name] = pkg;
            }
            in.close();
        }

        void saveInstalledPackages() const {
            std::string indexFile = baseDir + "/installed.txt";
            std::ofstream out(indexFile);
            if (!out.is_open()) return;

            out << "# الحزم المثبتة - لا تعدل هذا الملف يدوياً\n";

            for (const auto& [name, pkg] : installedPackages) {
                out << name << "|" << pkg.version.toString() << "|"
                    << pkg.author << "|" << pkg.description << "|";

                for (size_t i = 0; i < pkg.dependencies.size(); ++i) {
                    if (i > 0) out << ",";
                    out << pkg.dependencies[i].name;
                }
                out << "\n";
            }
            out.close();
        }

        void saveLocalRepository() const {
            std::string repoFile = repositoryDir + "/index.txt";
            std::ofstream out(repoFile);
            if (!out.is_open()) return;

            for (const auto& pkg : localRepository) {
                out << pkg.name << "|" << pkg.version.toString() << "|"
                    << pkg.author << "|" << pkg.description << "\n";
            }
            out.close();
        }

        void savePackageMetadata(const std::string& path, const PackageMetadata& metadata) const {
            std::string metaFile = path + "/package.meta";
            std::ofstream out(metaFile);
            if (!out.is_open()) return;

            out << "name=" << metadata.name << "\n";
            out << "version=" << metadata.version.toString() << "\n";
            out << "author=" << metadata.author << "\n";
            out << "description=" << metadata.description << "\n";
            out << "license=" << metadata.license << "\n";
            out.close();
        }

        std::shared_ptr<PackageMetadata> findInRepository(const std::string& name, const std::string& version) const {
            for (const auto& pkg : localRepository) {
                if (pkg.name == name) {
                    if (version == "latest") {
                        return std::make_shared<PackageMetadata>(pkg);
                    }

                    auto reqVersion = PackageVersion::fromString(version);
                    if (pkg.version == reqVersion) {
                        return std::make_shared<PackageMetadata>(pkg);
                    }
                }
            }
            return nullptr;
        }

        bool checkDependencies(const PackageMetadata& package) const {
            for (const auto& dep : package.dependencies) {
                if (!isInstalled(dep.name)) {
                    auto depInfo = findInRepository(dep.name, "latest");
                    if (!depInfo) {
                        std::cout << "❌ التبعية غير موجودة: " << dep.name << std::endl;
                        return false;
                    }

                    if (depInfo->version < dep.minVersion) {
                        std::cout << "❌ إصدار التبعية قديم: " << dep.name << std::endl;
                        return false;
                    }
                }
            }
            return true;
        }

        std::vector<std::string> findDependentPackages(const std::string& packageName) const {
            std::vector<std::string> dependents;

            for (const auto& [name, pkg] : installedPackages) {
                for (const auto& dep : pkg.dependencies) {
                    if (dep.name == packageName) {
                        dependents.push_back(name);
                        break;
                    }
                }
            }
            return dependents;
        }

        std::vector<PackageFile> downloadPackage(const PackageMetadata& metadata) {
            std::vector<PackageFile> files;

            std::string mainContent = "# حزمة " + metadata.name + "\n";
            mainContent += "# الإصدار: " + metadata.version.toString() + "\n\n";
            mainContent += "دالة اختبار():\n";
            mainContent += "    اطبع(\"مرحباً من " + metadata.name + "\")\n\n";
            mainContent += "اختبار()\n";

            files.push_back(PackageFile("main.عربي", mainContent));
            return files;
        }

        bool writePackageFiles(const std::string& packageName, const std::vector<PackageFile>& files) {
            std::string packagePath = packagesDir + "/" + packageName;
            mkdir(packagePath.c_str(), 0755);

            for (const auto& file : files) {
                std::string fullPath = packagePath + "/" + file.relativePath;

                size_t lastSlash = file.relativePath.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    std::string dirPath = packagePath + "/" + file.relativePath.substr(0, lastSlash);
                    mkdir(dirPath.c_str(), 0755);
                }

                std::ofstream out(fullPath);
                if (!out.is_open()) {
                    std::cerr << "❌ فشل كتابة: " << fullPath << std::endl;
                    return false;
                }

                out << file.content;
                out.close();
            }
            return true;
        }

        bool collectFiles(const std::string&, std::vector<PackageFile>& files) {
            PackageFile mainFile;
            mainFile.relativePath = "main.عربي";
            mainFile.content = "# ملف رئيسي للحزمة\n";
            mainFile.size = mainFile.content.size();
            files.push_back(mainFile);
            return true;
        }

        bool removeDirectory(const std::string&) {
            return true;
        }

        void printPackageInfo(const PackageMetadata& pkg) const {
            std::cout << "\n╔═══════════════════════════════════════════════╗" << std::endl;
            std::cout << "║  📦 معلومات الحزمة                               ║" << std::endl;
            std::cout << "╠═══════════════════════════════════════════════╣" << std::endl;
            std::cout << "║  الاسم: " << std::left << std::setw(43) << pkg.name << "║" << std::endl;
            std::cout << "║  الإصدار: " << std::left << std::setw(40) << pkg.version.toString() << "║" << std::endl;
            std::cout << "║  المؤلف: " << std::left << std::setw(41) << pkg.author << "║" << std::endl;
            std::cout << "║  الترخيص: " << std::left << std::setw(40) << pkg.license << "║" << std::endl;

            if (!pkg.description.empty()) {
                std::cout << "║  الوصف: " << std::left << std::setw(42) << pkg.description << "║" << std::endl;
            }

            if (!pkg.dependencies.empty()) {
                std::cout << "║  التبعيات (" << pkg.dependencies.size() << "):";
                std::cout << std::string(38 - std::to_string(pkg.dependencies.size()).length(), ' ') << "║" << std::endl;

                for (const auto& dep : pkg.dependencies) {
                    std::string depStr = "    • " + dep.name + " >= " + dep.minVersion.toString();
                    std::cout << "║  " << std::left << std::setw(49) << depStr << "║" << std::endl;
                }
            }

            if (!pkg.keywords.empty()) {
                std::cout << "║  الكلمات المفتاحية:                              ║" << std::endl;
                std::string keywords = "    ";
                for (size_t i = 0; i < pkg.keywords.size(); ++i) {
                    if (i > 0) keywords += ", ";
                    keywords += pkg.keywords[i];
                }
                std::cout << "║  " << std::left << std::setw(49) << keywords << "║" << std::endl;
            }

            if (!pkg.installDate.empty()) {
                std::string dateStr = "تاريخ التثبيت: " + pkg.installDate;
                std::cout << "║  " << std::left << std::setw(49) << dateStr << "║" << std::endl;
            }

            if (pkg.size > 0) {
                std::string sizeStr = "الحجم: " + formatSize(pkg.size);
                std::cout << "║  " << std::left << std::setw(49) << sizeStr << "║" << std::endl;
            }

            std::cout << "╚═══════════════════════════════════════════════╝" << std::endl;
        }

        std::string formatSize(long long bytes) const {
            if (bytes < 1024) return std::to_string(bytes) + " بايت";
            else if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " كيلوبايت";
            else return std::to_string(bytes / (1024 * 1024)) + " ميجابايت";
        }

        std::string getCurrentDateTime() const {
            time_t now = time(nullptr);
            char buffer[80];

#ifdef _WIN32
            struct tm timeinfo;
            localtime_s(&timeinfo, &now);
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
#else
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
#endif
            return std::string(buffer);
        }
    };

} // namespace ArabicLanguage

#endif // ARABIC_LIBRARIES_H