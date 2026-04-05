// SecurityFixes.h - إصلاحات أمنية للمترجم العربي
// Fecha: 2026-03-19

#ifndef SECURITY_FIXES_H
#define SECURITY_FIXES_H

#include <string>
#include <algorithm>
#include <cctype>

namespace ArabicSecurity {
namespace Security {

// ══════════════════════════════════════════════════════════════════
// دالة تنظيف اسم الملف لمنع هجوم path traversal
// ══════════════════════════════════════════════════════════════════
inline std::string sanitizeFileName(const std::string& input) {
    if (input.empty()) return "";
    
    std::string result = input;
    
    // إزالة الأحرف الخطرة
    const std::string dangerous = "<>:\"/\\|?*";
    for (char& c : result) {
        if (dangerous.find(c) != std::string::npos) {
            c = '_';
        }
    }
    
    // منع مسار خارج المجلد (..)
    size_t pos;
    while ((pos = result.find("..")) != std::string::npos) {
        result.replace(pos, 2, "__");
    }
    
    // منع المسارات المطلقة
    if (result.length() >= 2 && result[1] == ':') {
        result = "output_" + result.substr(2);
    }
    if (result.length() >= 1 && result[0] == '/') {
        result = "output_" + result.substr(1);
    }
    
    // الحد الأقصى للطول
    if (result.length() > 255) {
        result = result.substr(0, 255);
    }
    
    return result;
}

// ══════════════════════════════════════════════════════════════════
// التحقق من صحة اسم الملف
// ══════════════════════════════════════════════════════════════════
inline bool isValidFileName(const std::string& name) {
    if (name.empty()) return false;
    
    // التحقق من الطول
    if (name.length() > 255) return false;
    
    // التحقق من الأحرف المسموحة
    const std::string allowed = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
        "_-.\\/\u0600-\u06FF\u0750-\u077F"; // أحرف عربية
    
    for (char c : name) {
        if (allowed.find(c) == std::string::npos) {
            return false;
        }
    }
    
    // منع الكلمات المحجوزة
    const std::string reserved[] = {"CON", "PRN", "AUX", "NUL", 
        "COM1", "COM2", "COM3", "COM4", "LPT1", "LPT2", "LPT3"};
    for (const auto& r : reserved) {
        if (name == r || name.substr(0, 4) == r + ".") {
            return false;
        }
    }
    
    return true;
}

// ══════════════════════════════════════════════════════════════════
// التحقق من صحة المسار
// ══════════════════════════════════════════════════════════════════
inline bool isValidPath(const std::string& path) {
    if (path.empty()) return false;
    
    // منع المسارات المطلقة في غير Windows
    #ifdef _WIN32
    // في Windows، المسارات المطلقة مسموحة
    #else
    if (path[0] == '/' && path.length() > 1 && path[1] != '/') {
        return false; // مسارات مطلقة غير مسموحة في Linux
    }
    #endif
    
    // منع حركات .. المتعددة
    if (path.find("..") != std::string::npos) {
        return false;
    }
    
    return true;
}

// ══════════════════════════════════════════════════════════════════
// تنظيف مدخلات المستخدم للأوامر
// ══════════════════════════════════════════════════════════════════
inline std::string sanitizeCommandArg(const std::string& input) {
    std::string result;
    
    for (char c : input) {
        // السماح فقط بالأحرف الآمنة
        if (std::isalnum(c) || c == '_' || c == '-' || c == '.' || 
            c == '/' || c == '\\' || c == ' ') {
            result += c;
        }
        // السماح ببعض الأحرف العربية
        else if ((c >= 0x0600 && c <= 0x06FF) ||
                 (c >= 0x0750 && c <= 0x077F)) {
            result += c;
        }
    }
    
    return result;
}

// ══════════════════════════════════════════════════════════════════
// التحقق من أن الملف هو PE executable صالح
// ══════════════════════════════════════════════════════════════════
inline bool validatePEFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;
    
    // فحص رأس MZ
    char mz[2];
    file.read(mz, 2);
    if (mz[0] != 'M' || mz[1] != 'Z') {
        return false;
    }
    
    // فحص توقيع PE
    file.seekg(0x3C);
    char peOffset[4];
    file.read(peOffset, 4);
    int pePos = *reinterpret_cast<int*>(peOffset);
    
    file.seekg(pePos);
    char pe[4];
    file.read(pe, 4);
    
    if (pe[0] != 'P' || pe[1] != 'E' || pe[2] != '\0' || pe[3] != '\0') {
        return false;
    }
    
    return true;
}

} // namespace Security
} // namespace ArabicSecurity

#endif // SECURITY_FIXES_H
