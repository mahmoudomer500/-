// AlBiruniIDE.cpp - بيئة تطوير البيروني v2.0.0 (محسّنة)
// Al-Biruni IDE - Enhanced with syntax highlighting, error analysis, examples

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <set>
#include <commdlg.h>
#include <Richedit.h>
#include <commctrl.h>
#include <direct.h>
#include <io.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

// ══════════════════════════════════════════════════════════════
// Globals
// ══════════════════════════════════════════════════════════════
HWND g_editor = NULL;
HWND g_output = NULL;
HWND g_status = NULL;
HWND g_hMainWnd = NULL;
HMENU g_hMenu = NULL;
HINSTANCE g_hInst = NULL;
std::string g_currentFile = "";
bool g_modified = false;
std::string g_compilerPath = "";

// Menu IDs
#define IDM_FILE_NEW      4001
#define IDM_FILE_OPEN     4002
#define IDM_FILE_SAVE     4003
#define IDM_FILE_SAVEAS   4004
#define IDM_FILE_EXIT     4005
#define IDM_EDIT_RUN      4010
#define IDM_EDIT_COMPILE  4011
#define IDM_EDIT_FORMAT   4012
#define IDM_EDIT_SELFCHECK 4013
#define IDM_ANALYZE       4030
#define IDM_EXAMPLE_HELLO  4020
#define IDM_EXAMPLE_VARS   4021
#define IDM_EXAMPLE_IF     4022
#define IDM_EXAMPLE_LOOP   4023
#define IDM_EXAMPLE_FUNC   4024
#define IDM_EXAMPLE_ARRAY  4025
#define IDM_EXAMPLE_CLASS  4026
#define IDM_EXAMPLE_FILE   4027
#define IDM_EXAMPLE_MATH   4028
#define IDM_EXAMPLE_OOP    4029
#define IDM_HELP_ABOUT     4040

// ══════════════════════════════════════════════════════════════
// Forward declarations
// ══════════════════════════════════════════════════════════════
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void UpdateTitle();
void AppendOutput(const char* text);
void RunCode();
void CompileCode();
void FormatCode();
void SelfCheck();
void LoadExample(int id);
void runErrorAnalysis();
void applySyntaxHighlighting();
HMENU CreateMainMenu();
std::string GetCompilerPath();
std::string RunCommand(const char* cmd);
std::string ReadFileContent(const std::string& path);
const char* getExample(int id);

// ══════════════════════════════════════════════════════════════
// Syntax Highlighter
// ══════════════════════════════════════════════════════════════

struct TokenInfo {
    std::string text;
    int startCol;
    COLORREF color;
};

std::set<std::string> g_keywords = {
    "اطبع", "مت", "دالة", "أرجع", "إذا", "وإلا", "بينما", "لكل", "صف",
    "جديد", "خاص", "عام", "محمي", "استورد", "صحيح", "خطأ", "و", "أو",
    "ليس", "في", "توقف", "استمر", "نهاية", "هذا", "حاول", "امسك",
    "لا_شيء", "رقم", "نص", "منطقي", "مصفوفة", "قاموس"
};

std::set<std::string> g_functions = {
    "جذر", "قوة", "مطلق", "طول", "نوع", "هل_رقم", "اكتب_ملف", "اقرأ_ملف",
    "هل_ملف_موجود", "افتح_ملف", "أغلق_ملف", "اقرأ_سطر", "نص_استبدل",
    "نص_فرعي", "نص_إلى_رقم", "رقم_إلى_نص", "لون_النص", "لون_الخلفية",
    "إعادة_لون", "لون_عشوائي", "انتظر", "أضف", "قاموس_جديد", "قاموس_ضع",
    "قاموس_اجلب", "قاموس_حجم", "قاموس_يحتوي", "قاموس_احذف"
};

std::set<std::string> g_operators = {
    "+", "-", "*", "/", "%", "==", "!=", ">", "<", ">=", "<=", "=", "!"
};

std::vector<TokenInfo> tokenizeLine(const std::string& line) {
    std::vector<TokenInfo> tokens;
    std::string current;
    int col = 0;

    for (size_t i = 0; i < line.size(); i++) {
        unsigned char c = static_cast<unsigned char>(line[i]);

        if (c == '#' || (c == '/' && i + 1 < line.size() && static_cast<unsigned char>(line[i+1]) == '/')) {
            if (!current.empty()) {
                tokens.push_back({current, col, RGB(200,200,200)});
                current.clear();
            }
            tokens.push_back({line.substr(i), (int)i, RGB(100,140,100)});
            break;
        }

        if (c == '"') {
            if (!current.empty()) {
                tokens.push_back({current, col, RGB(200,200,200)});
                current.clear();
            }
            size_t start = i;
            i++;
            while (i < line.size() && line[i] != '"') i++;
            tokens.push_back({line.substr(start, i - start + 1), (int)start, RGB(206,145,120)});
            col = (int)i + 1;
            continue;
        }

        if (std::isdigit(c)) {
            if (!current.empty()) {
                tokens.push_back({current, col, RGB(200,200,200)});
                current.clear();
            }
            size_t start = i;
            while (i < line.size() && (std::isdigit(static_cast<unsigned char>(line[i])) || line[i] == '.')) i++;
            tokens.push_back({line.substr(start, i - start), (int)start, RGB(181,206,168)});
            col = (int)i;
            if (i < line.size()) i--;
            continue;
        }

        if (std::isspace(c)) {
            if (!current.empty()) {
                COLORREF clr = RGB(200,200,200);
                if (g_keywords.count(current)) clr = RGB(86,156,214);
                else if (g_functions.count(current)) clr = RGB(220,220,170);
                else if (g_operators.count(current)) clr = RGB(200,160,120);
                tokens.push_back({current, col, clr});
                current.clear();
            }
            col++;
            continue;
        }

        current += (char)c;
    }

    if (!current.empty()) {
        COLORREF clr = RGB(200,200,200);
        if (g_keywords.count(current)) clr = RGB(86,156,214);
        else if (g_functions.count(current)) clr = RGB(220,220,170);
        else if (g_operators.count(current)) clr = RGB(200,160,120);
        tokens.push_back({current, col, clr});
    }

    return tokens;
}

void applySyntaxHighlighting() {
    if (!g_editor) return;
    int totalLen = GetWindowTextLengthA(g_editor);
    if (totalLen == 0) return;

    char* allText = new char[totalLen + 1];
    GetWindowTextA(g_editor, allText, totalLen + 1);

    CHARRANGE cr;
    SendMessage(g_editor, EM_EXGETSEL, 0, (LPARAM)&cr);
    SendMessage(g_editor, WM_SETREDRAW, FALSE, 0);

    CHARFORMAT2W cfDef;
    ZeroMemory(&cfDef, sizeof(cfDef));
    cfDef.cbSize = sizeof(cfDef);
    cfDef.dwMask = CFM_COLOR;
    cfDef.crTextColor = RGB(200,200,200);

    CHARRANGE allRange = {0, totalLen};
    SendMessage(g_editor, EM_EXSETSEL, 0, (LPARAM)&allRange);
    SendMessage(g_editor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfDef);

    std::string text(allText);
    delete[] allText;

    int lineStart = 0;
    size_t pos = 0;
    int charPos = 0;

    while (pos <= text.size()) {
        if (pos == text.size() || text[pos] == '\n') {
            std::string line = text.substr(lineStart, pos - lineStart);
            if (!line.empty() && line.back() == '\r') line.pop_back();

            auto tokens = tokenizeLine(line);
            for (const auto& tok : tokens) {
                int charCol = 0;
                for (int ci = 0; ci < tok.startCol && ci < (int)line.size(); ci++) {
                    if ((static_cast<unsigned char>(line[ci]) & 0xC0) != 0x80) charCol++;
                }
                int charLen = 0;
                for (size_t ci = 0; ci < tok.text.size(); ci++) {
                    if ((static_cast<unsigned char>(tok.text[ci]) & 0xC0) != 0x80) charLen++;
                }

                CHARRANGE tr = {charPos + charCol, charPos + charCol + charLen};
                SendMessage(g_editor, EM_EXSETSEL, 0, (LPARAM)&tr);

                CHARFORMAT2W cf;
                ZeroMemory(&cf, sizeof(cf));
                cf.cbSize = sizeof(cf);
                cf.dwMask = CFM_COLOR;
                cf.crTextColor = tok.color;
                SendMessage(g_editor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
            }
            charPos += (int)(pos - lineStart) + 1;
            lineStart = (int)pos + 1;
        }
        pos++;
    }

    SendMessage(g_editor, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessage(g_editor, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_editor, nullptr, TRUE);
}

// ══════════════════════════════════════════════════════════════
// Error Analyzer
// ══════════════════════════════════════════════════════════════

struct CodeIssue {
    std::string message;
    int line;
    bool isWarning;
};

std::vector<CodeIssue> analyzeCode(const std::string& code) {
    std::vector<CodeIssue> issues;
    std::istringstream ss(code);
    std::string line;
    int lineNum = 0;
    int parenDepth = 0;
    bool inString = false;

    while (std::getline(ss, line)) {
        lineNum++;
        for (char c : line) {
            if (c == '(' && !inString) parenDepth++;
            if (c == ')' && !inString) parenDepth--;
            if (c == '"') inString = !inString;
        }

        size_t start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos) continue;
        std::string trimmed = line.substr(start);
        if (trimmed.empty() || trimmed[0] == '#' || (trimmed.size() >= 2 && trimmed[0] == '/' && trimmed[1] == '/'))
            continue;

        if (parenDepth < 0) { issues.push_back({"قوس ')' زائد", lineNum, false}); parenDepth = 0; }
        if (trimmed.find("دالة") == 0 && trimmed.find(":") == std::string::npos && trimmed.find("نهاية") == std::string::npos)
            issues.push_back({"تحذير: دالة بدون ':' في النهاية", lineNum, true});
        if (trimmed.find("إذا") == 0 && trimmed.find(":") == std::string::npos && trimmed.find("نهاية") == std::string::npos)
            issues.push_back({"تحذير: شرط بدون ':' في النهاية", lineNum, true});
        if (trimmed.find("بينما") == 0 && trimmed.find(":") == std::string::npos && trimmed.find("نهاية") == std::string::npos)
            issues.push_back({"تحذير: حلقة بدون ':' في النهاية", lineNum, true});
        if (trimmed.find("لكل") == 0 && trimmed.find("في") == std::string::npos)
            issues.push_back({"خطأ: 'لكل' تحتاج كلمة 'في'", lineNum, false});
    }

    if (parenDepth > 0) issues.push_back({"قوس '(' غير مغلق", lineNum, false});
    return issues;
}

void runErrorAnalysis() {
    int len = GetWindowTextLengthA(g_editor);
    if (len == 0) { AppendOutput("ℹ️ المحرر فارغ\n\n"); return; }
    char* code = new char[len + 1];
    GetWindowTextA(g_editor, code, len + 1);
    auto issues = analyzeCode(code);
    delete[] code;

    AppendOutput("🔍 تحليل الكود:\n═══════════════════════════════════════════\n");
    if (issues.empty()) { AppendOutput("✅ لا توجد مشاكل واضحة في الكود\n\n"); return; }
    for (const auto& issue : issues) {
        std::string prefix = issue.isWarning ? "⚠️" : "❌";
        AppendOutput((prefix + " سطر " + std::to_string(issue.line) + ": " + issue.message + "\n").c_str());
    }
    AppendOutput("\n");
}

// ══════════════════════════════════════════════════════════════
// Examples
// ══════════════════════════════════════════════════════════════

const char* getExample(int id) {
    switch (id) {
        case IDM_EXAMPLE_HELLO: return "# مرحباً بالعالم\r\nاطبع(\"مرحباً بالعالم!\")\r\n";
        case IDM_EXAMPLE_VARS: return "# المتغيرات\r\nمت اسم = \"أحمد\"\r\nمت العمر = 25\r\nمت طالب = صحيح\r\n\r\nاطبع(\"الاسم: \" + اسم)\r\nاطبع(\"العمر: \" + رقم_إلى_نص(العمر))\r\n";
        case IDM_EXAMPLE_IF: return "# الشروط\r\nمت العمر = 20\r\n\r\nاذا العمر >= 18:\r\n    اطبع(\"بالغ\")\r\nوإلا اذا العمر >= 12:\r\n    اطبع(\"مراهق\")\r\nوإلا:\r\n    اطبع(\"طفل\")\r\nنهاية\r\n";
        case IDM_EXAMPLE_LOOP: return "# الحلقات\r\nمت ع = 0\r\n\r\nبينما ع < 5:\r\n    اطبع(ع)\r\n    ع = ع + 1\r\nنهاية\r\n\r\nلكل رقم في [1، 2، 3، 4، 5]:\r\n    اطبع(رقم)\r\nنهاية\r\n";
        case IDM_EXAMPLE_FUNC: return "# الدوال\r\nدالة جمع(أ، ب):\r\n    أرجع أ + ب\r\nنهاية\r\n\r\nدالة مرحبا(اسم = \"عالم\"):\r\n    اطبع(\"مرحباً يا \" + اسم)\r\nنهاية\r\n\r\nاطبع(جمع(3، 4))\r\nمرحبا(\"أحمد\")\r\n";
        case IDM_EXAMPLE_ARRAY: return "# المصفوفات\r\nمت ارقام = [1، 2، 3، 4، 5]\r\n\r\nلكل رقم في ارقام:\r\n    اطبع(رقم)\r\nنهاية\r\n\r\nاطبع(\"الطول: \" + رقم_إلى_نص(طول(ارقام)))\r\n";
        case IDM_EXAMPLE_CLASS: return "# الصفوف (OOP)\r\nصف نقطة:\r\n    خاص:\r\n        رقم س\r\n        رقم ص\r\n\r\n    عام:\r\n        دالة جديد(س، ص):\r\n            هذا.س = س\r\n            هذا.ص = ص\r\n\r\n        دالة المسافة():\r\n            أرجع جذر(هذا.س * هذا.س + هذا.ص * هذا.ص)\r\nنهاية\r\n\r\nمت ن = جديد نقطة(3، 4)\r\nاطبع(ن.المسافة())\r\n";
        case IDM_EXAMPLE_FILE: return "# الملفات\r\nاكتب_ملف(\"test.txt\", \"مرحباً!\")\r\n\r\nاذا هل_ملف_موجود(\"test.txt\"):\r\n    مت محتوى = اقرأ_ملف(\"test.txt\")\r\n    اطبع(محتوى)\r\nنهاية\r\n";
        case IDM_EXAMPLE_MATH: return "# الرياضيات\r\nاطبع(\"جذر 16 = \" + رقم_إلى_نص(جذر(16)))\r\nاطبع(\"2 أس 8 = \" + رقم_إلى_نص(قوة(2، 8)))\r\nاطبع(\"مطلق -5 = \" + رقم_إلى_نص(مطلق(-5)))\r\n";
        case IDM_EXAMPLE_OOP: return "# وراثة الأصناف\r\nصف حيوان:\r\n    خاص:\r\n        نص اسم\r\n    عام:\r\n        دالة جديد(اسم):\r\n            هذا.اسم = اسم\r\n        دالة تكلم():\r\n            اطبع(\"...\")\r\nنهاية\r\n\r\nصف كلب يرث حيوان:\r\n    عام:\r\n        دالة جديد(اسم):\r\n            هذا.اسم = اسم\r\n        دالة تكلم():\r\n            اطبع(\"هاو هاو!\")\r\nنهاية\r\n\r\nمت ك = جديد كلب(\"ركس\")\r\nك.تكلم()\r\n";
        default: return "# مثال غير معروف\r\n";
    }
}

// ══════════════════════════════════════════════════════════════
// Utility Functions
// ══════════════════════════════════════════════════════════════

std::string GetExeDir() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string s(path);
    size_t pos = s.find_last_of("\\/");
    return s.substr(0, pos);
}

std::string GetCompilerPath() {
    std::string exeDir = GetExeDir();
    std::string paths[] = {
        exeDir + "\\arabic_compiler.exe",
        exeDir + "\\..\\arabic_compiler.exe",
        exeDir + "\\..\\..\\build_release\\Release\\arabic_compiler.exe",
        exeDir + "\\build_release\\Release\\arabic_compiler.exe",
        "arabic_compiler.exe"
    };
    for (int i = 0; i < 5; i++) {
        if (_access(paths[i].c_str(), 0) == 0) return paths[i];
    }
    return "";
}

std::string RunCommand(const char* cmd) {
    std::string result = "";
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return "خطأ: فشل في إنشاء الأنبوب\n";

    STARTUPINFOA si = {0}; si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite; si.hStdError = hWrite; si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {0};
    char cmdCopy[8192];
    strncpy(cmdCopy, cmd, sizeof(cmdCopy) - 1);
    cmdCopy[sizeof(cmdCopy) - 1] = '\0';

    if (CreateProcessA(NULL, cmdCopy, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hWrite);
        char buffer[4096]; DWORD bytesRead;
        while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0'; result += buffer;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    } else { result = "خطأ: فشل في تشغيل الأمر\n"; }
    CloseHandle(hRead);
    return result;
}

std::string ReadFileContent(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close(); return content;
}

void AppendOutput(const char* text) {
    int len = GetWindowTextLengthA(g_output);
    SendMessageA(g_output, EM_SETSEL, len, len);
    SendMessageA(g_output, EM_REPLACESEL, FALSE, (LPARAM)text);
}

void UpdateTitle() {
    std::string title = "البيروني - بيئة التطوير العربية v2.0.0";
    if (!g_currentFile.empty()) {
        size_t pos = g_currentFile.find_last_of("\\/");
        title += " - " + g_currentFile.substr(pos + 1);
    }
    if (g_modified) title += " *";
    SetWindowTextA(g_hMainWnd, title.c_str());
}

void LoadExample(int id) {
    const char* code = getExample(id);
    SetWindowTextA(g_editor, code);
    g_currentFile = ""; g_modified = false;
    UpdateTitle(); applySyntaxHighlighting();
    std::string name = "";
    switch (id) {
        case IDM_EXAMPLE_HELLO: name = "مرحباً بالعالم"; break;
        case IDM_EXAMPLE_VARS: name = "المتغيرات"; break;
        case IDM_EXAMPLE_IF: name = "الشروط"; break;
        case IDM_EXAMPLE_LOOP: name = "الحلقات"; break;
        case IDM_EXAMPLE_FUNC: name = "الدوال"; break;
        case IDM_EXAMPLE_ARRAY: name = "المصفوفات"; break;
        case IDM_EXAMPLE_CLASS: name = "الصفوف"; break;
        case IDM_EXAMPLE_FILE: name = "الملفات"; break;
        case IDM_EXAMPLE_MATH: name = "الرياضيات"; break;
        case IDM_EXAMPLE_OOP: name = "الوراثة"; break;
    }
    AppendOutput(("📂 تم تحميل مثال: " + name + "\n\n").c_str());
}

// ══════════════════════════════════════════════════════════════
// Menu
// ══════════════════════════════════════════════════════════════

HMENU CreateMainMenu() {
    HMENU hMenu = CreateMenu();
    HMENU hFile = CreatePopupMenu();
    AppendMenuA(hFile, MF_STRING, IDM_FILE_NEW, "جديد\tCtrl+N");
    AppendMenuA(hFile, MF_STRING, IDM_FILE_OPEN, "فتح\tCtrl+O");
    AppendMenuA(hFile, MF_STRING, IDM_FILE_SAVE, "حفظ\tCtrl+S");
    AppendMenuA(hFile, MF_STRING, IDM_FILE_SAVEAS, "حفظ باسم");
    AppendMenuA(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hFile, MF_STRING, IDM_FILE_EXIT, "خروج");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hFile, "ملف");

    HMENU hRun = CreatePopupMenu();
    AppendMenuA(hRun, MF_STRING, IDM_EDIT_RUN, "تشغيل\tF5");
    AppendMenuA(hRun, MF_STRING, IDM_EDIT_COMPILE, "تجميع\tF7");
    AppendMenuA(hRun, MF_STRING, IDM_EDIT_FORMAT, "تنسيق\tCtrl+F");
    AppendMenuA(hRun, MF_STRING, IDM_EDIT_SELFCHECK, "فحص ذاتي");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hRun, "تشغيل");

    HMENU hAnalysis = CreatePopupMenu();
    AppendMenuA(hAnalysis, MF_STRING, IDM_ANALYZE, "تحليل الكود\tCtrl+A");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hAnalysis, "تحليل");

    HMENU hExamples = CreatePopupMenu();
    AppendMenuA(hExamples, MF_STRING, IDM_EXAMPLE_HELLO, "مرحباً بالعالم");
    AppendMenuA(hExamples, MF_STRING, IDM_EXAMPLE_VARS, "المتغيرات");
    AppendMenuA(hExamples, MF_STRING, IDM_EXAMPLE_IF, "الشروط");
    AppendMenuA(hExamples, MF_STRING, IDM_EXAMPLE_LOOP, "الحلقات");
    AppendMenuA(hExamples, MF_STRING, IDM_EXAMPLE_FUNC, "الدوال");
    AppendMenuA(hExamples, MF_STRING, IDM_EXAMPLE_ARRAY, "المصفوفات");
    AppendMenuA(hExamples, MF_STRING, IDM_EXAMPLE_CLASS, "الصفوف");
    AppendMenuA(hExamples, MF_STRING, IDM_EXAMPLE_FILE, "الملفات");
    AppendMenuA(hExamples, MF_STRING, IDM_EXAMPLE_MATH, "الرياضيات");
    AppendMenuA(hExamples, MF_STRING, IDM_EXAMPLE_OOP, "الوراثة");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hExamples, "أمثلة");

    HMENU hHelp = CreatePopupMenu();
    AppendMenuA(hHelp, MF_STRING, IDM_HELP_ABOUT, "حول");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hHelp, "مساعدة");

    return hMenu;
}

// ══════════════════════════════════════════════════════════════
// Actions
// ══════════════════════════════════════════════════════════════

void RunCode() {
    if (g_compilerPath.empty()) {
        AppendOutput("❌ خطأ: لم يتم العثور على المترجم!\n   ضع arabic_compiler.exe في نفس مجلد IDE\n\n");
        return;
    }
    int len = GetWindowTextLengthA(g_editor);
    char* code = new char[len + 1];
    GetWindowTextA(g_editor, code, len + 1);
    std::ofstream temp("temp_run.عربي", std::ios::binary);
    temp.write(code, len); temp.close();
    AppendOutput("▶️  جاري التشغيل...\n═══════════════════════════════════════════\n");
    std::string cmd = "\"" + g_compilerPath + "\" --mode run temp_run.عربي 2>&1";
    AppendOutput(RunCommand(cmd.c_str()).c_str());
    AppendOutput("\n═══════════════════════════════════════════\n✅ انتهى التشغيل\n\n");
    delete[] code; DeleteFileA("temp_run.عربي");
}

void CompileCode() {
    if (g_compilerPath.empty()) {
        AppendOutput("❌ خطأ: لم يتم العثور على المترجم!\n\n"); return;
    }
    int len = GetWindowTextLengthA(g_editor);
    char* code = new char[len + 1];
    GetWindowTextA(g_editor, code, len + 1);
    std::ofstream temp("temp_compile.عربي", std::ios::binary);
    temp.write(code, len); temp.close();
    AppendOutput("🔨 جاري التجميع...\n═══════════════════════════════════════════\n");
    std::string cmd = "\"" + g_compilerPath + "\" --mode compiler temp_compile.عربي temp_output 2>&1";
    AppendOutput(RunCommand(cmd.c_str()).c_str());
    if (_access("temp_output.exe", 0) == 0) {
        AppendOutput("\n✅ تم التجميع بنجاح!\n▶️  جاري التشغيل...\n═══════════════════════════════════════════\n");
        STARTUPINFOA si = {0}; si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi = {0};
        char runCmd[] = "temp_output.exe";
        if (CreateProcessA(NULL, runCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            HANDLE hRead, hWrite;
            SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
            CreatePipe(&hRead, &hWrite, &sa, 0);
            si.hStdOutput = hWrite; si.hStdError = hWrite;
            si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            CloseHandle(hWrite);
            char buf[4096]; DWORD br;
            while (ReadFile(hRead, buf, sizeof(buf) - 1, &br, NULL) && br > 0) {
                buf[br] = '\0'; AppendOutput(buf);
            }
            CloseHandle(hRead);
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
        }
        AppendOutput("\n═══════════════════════════════════════════\n✅ انتهى\n\n");
        DeleteFileA("temp_output.exe");
    } else {
        AppendOutput("\n❌ فشل التجميع\n\n");
    }
    delete[] code; DeleteFileA("temp_compile.عربي");
}

void FormatCode() {
    int len = GetWindowTextLengthA(g_editor);
    char* code = new char[len + 1];
    GetWindowTextA(g_editor, code, len + 1);
    std::string input(code); delete[] code;
    std::string result = ""; int indent = 0;
    size_t pos = 0;
    while (pos < input.size()) {
        size_t end = input.find('\n', pos);
        if (end == std::string::npos) end = input.size();
        std::string line = input.substr(pos, end - pos);
        size_t start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos) { pos = end + 1; continue; }
        line = line.substr(start);
        size_t endTrim = line.find_last_not_of(" \t\r");
        if (endTrim != std::string::npos) line = line.substr(0, endTrim + 1);
        if (line.empty()) { pos = end + 1; continue; }
        if (line == "نهاية") { indent--; if (indent < 0) indent = 0; }
        std::string indented = "";
        for (int i = 0; i < indent; i++) indented += "    ";
        indented += line + "\r\n"; result += indented;
        if (line.find("دالة") == 0 || line.find("إذا") == 0 || line.find("وإلا") == 0 ||
            line.find("بينما") == 0 || line.find("لكل") == 0 || line.find("صف") == 0 ||
            line.find("صنف") == 0 || line.find("عام:") == 0 || line.find("خاص:") == 0) {
            indent++;
        }
        pos = end + 1;
    }
    SetWindowTextA(g_editor, result.c_str());
    g_modified = true; UpdateTitle(); applySyntaxHighlighting();
    AppendOutput("✅ تم تنسيق الكود بنجاح\n\n");
}

void SelfCheck() {
    if (g_compilerPath.empty()) {
        AppendOutput("❌ خطأ: لم يتم العثور على المترجم!\n\n"); return;
    }
    AppendOutput("🔍 فحص ذاتي - Self-Hosting Check\n═══════════════════════════════════════════\n");
    std::string cmd = "\"" + g_compilerPath + "\" --mode selfhost 2>&1";
    AppendOutput("جاري تشغيل البناء الذاتي...\n");
    AppendOutput(("الأمر: " + cmd + "\n\n").c_str());
    AppendOutput(RunCommand(cmd.c_str()).c_str());
    AppendOutput("\n═══════════════════════════════════════════\n");
    if (_access("self_hosted_output/lexer.exe", 0) == 0) AppendOutput("✅ المحلل اللغوي: lexer.exe\n");
    else AppendOutput("⚠️ المحلل اللغوي: لم يتم إنتاج EXE\n");
    if (_access("self_hosted_output/codegen.exe", 0) == 0) AppendOutput("✅ مولد الكود: codegen.exe\n");
    else AppendOutput("⚠️ مولد الكود: لم يتم إنتاج EXE\n");
    if (_access("self_hosted_output/compiler_v3.exe", 0) == 0) AppendOutput("✅ المترجم الذاتي: compiler_v3.exe\n");
    else AppendOutput("⚠️ المترجم الذاتي: لم يتم إنتاج EXE\n");
    AppendOutput("\n✅ انتهى الفحص الذاتي\n\n");
}

// ══════════════════════════════════════════════════════════════
// Main & Window Proc
// ══════════════════════════════════════════════════════════════

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInst = hInstance;
    g_compilerPath = GetCompilerPath();

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX); wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance; wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AlBiruniIDE";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&wc);

    g_hMainWnd = CreateWindowEx(0, "AlBiruniIDE",
        "البيروني - بيئة التطوير العربية v2.0.0",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 1400, 900, NULL, NULL, hInstance, NULL);
    if (!g_hMainWnd) { MessageBoxA(NULL, "Failed", "Error", MB_ICONERROR); return 1; }

    g_hMenu = CreateMainMenu();
    SetMenu(g_hMainWnd, g_hMenu);

    LoadLibrary("RichEd20.dll");
    INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);

    HFONT hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ARABIC_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Consolas");

    g_editor = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS,
        "# مرحباً بك في البيروني v2.0.0!\r\nاطبع(\"مرحباً بالعالم!\")\r\n",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
        ES_AUTOHSCROLL | WS_HSCROLL | WS_VSCROLL | ES_WANTRETURN,
        10, 10, 700, 800, g_hMainWnd, NULL, hInstance, NULL);
    SendMessage(g_editor, WM_SETFONT, (WPARAM)hFont, TRUE);

    CreateWindow("STATIC", "المخرجات والتحليل:",
        WS_CHILD | WS_VISIBLE, 730, 10, 200, 20, g_hMainWnd, NULL, hInstance, NULL);

    g_output = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
        ES_READONLY | WS_HSCROLL | WS_VSCROLL,
        730, 30, 640, 780, g_hMainWnd, NULL, hInstance, NULL);
    SendMessage(g_output, WM_SETFONT, (WPARAM)hFont, TRUE);

    g_status = CreateWindow("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 820, 1360, 25, g_hMainWnd, NULL, hInstance, NULL);
    SendMessage(g_status, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    std::string statusText = "جاهز";
    if (!g_compilerPath.empty()) statusText += " | المترجم: " + g_compilerPath;
    else statusText += " | لم يتم العثور على المترجم";
    SetWindowTextA(g_status, statusText.c_str());

    std::string welcome = "═══════════════════════════════════════════════════════\n";
    welcome += "  مرحباً بك في بيئة التطوير العربية - البيروني v2.0.0\n";
    welcome += "═══════════════════════════════════════════════════════\n\n";
    if (!g_compilerPath.empty()) welcome += "✅ المترجم: " + g_compilerPath + "\n";
    else { welcome += "⚠️ لم يتم العثور على المترجم.\n"; welcome += "   ضع arabic_compiler.exe في نفس مجلد IDE\n"; }
    welcome += "\nالأدوات المتاحة:\n";
    welcome += "  [ملف]     → جديد، فتح، حفظ\n";
    welcome += "  [تشغيل]   → تشغيل الكود مباشرة\n";
    welcome += "  [تجميع]   → تجميع إلى ملف .exe\n";
    welcome += "  [تنسيق]   → تنسيق الكود العربي\n";
    welcome += "  [تحليل]   → فحص أخطاء الكود\n";
    welcome += "  [فحص ذاتي] → اختبار البناء الذاتي\n";
    welcome += "  [أمثلة]   → 10 أمثلة جاهزة\n";
    AppendOutput(welcome.c_str());
    applySyntaxHighlighting();

    ShowWindow(g_hMainWnd, SW_SHOW);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_FILE_NEW:
                    SetWindowTextA(g_editor, "# ملف جديد\r\n");
                    g_currentFile = ""; g_modified = false;
                    UpdateTitle(); applySyntaxHighlighting(); break;

                case IDM_FILE_OPEN: {
                    OPENFILENAMEA ofn = {0}; char szFile[MAX_PATH] = {0};
                    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "Arabic Files\0*.عربي;*.arabic;*.ar\0All Files\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    if (GetOpenFileNameA(&ofn)) {
                        std::string content = ReadFileContent(szFile);
                        if (!content.empty()) {
                            SetWindowTextA(g_editor, content.c_str());
                            g_currentFile = szFile; g_modified = false;
                            UpdateTitle(); applySyntaxHighlighting();
                            AppendOutput(("تم فتح: " + std::string(szFile)).c_str());
                        }
                    } break; }

                case IDM_FILE_SAVE: {
                    if (g_currentFile.empty()) {
                        OPENFILENAMEA ofn = {0}; char szFile[MAX_PATH] = {0};
                        ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
                        ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile);
                        ofn.lpstrFilter = "Arabic Files\0*.عربي\0All Files\0*.*\0";
                        ofn.lpstrDefExt = "عربي";
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
                        if (GetSaveFileNameA(&ofn)) g_currentFile = szFile;
                        else break;
                    }
                    int len = GetWindowTextLengthA(g_editor);
                    char* buffer = new char[len + 1];
                    GetWindowTextA(g_editor, buffer, len + 1);
                    std::ofstream file(g_currentFile, std::ios::binary);
                    if (file.is_open()) {
                        file.write(buffer, len); file.close();
                        g_modified = false; UpdateTitle();
                        AppendOutput(("تم الحفظ: " + g_currentFile).c_str());
                    }
                    delete[] buffer; break; }

                case IDM_FILE_SAVEAS: {
                    OPENFILENAMEA ofn = {0}; char szFile[MAX_PATH] = {0};
                    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "Arabic Files\0*.عربي\0All Files\0*.*\0";
                    ofn.lpstrDefExt = "عربي";
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
                    if (GetSaveFileNameA(&ofn)) {
                        g_currentFile = szFile;
                        int len = GetWindowTextLengthA(g_editor);
                        char* buffer = new char[len + 1];
                        GetWindowTextA(g_editor, buffer, len + 1);
                        std::ofstream file(g_currentFile, std::ios::binary);
                        if (file.is_open()) {
                            file.write(buffer, len); file.close();
                            g_modified = false; UpdateTitle();
                            AppendOutput(("تم الحفظ: " + g_currentFile).c_str());
                        }
                        delete[] buffer;
                    } break; }

                case IDM_FILE_EXIT: SendMessage(hwnd, WM_CLOSE, 0, 0); break;
                case IDM_EDIT_RUN: RunCode(); break;
                case IDM_EDIT_COMPILE: CompileCode(); break;
                case IDM_EDIT_FORMAT: FormatCode(); break;
                case IDM_EDIT_SELFCHECK: SelfCheck(); break;
                case IDM_ANALYZE: runErrorAnalysis(); break;

                case IDM_EXAMPLE_HELLO: case IDM_EXAMPLE_VARS: case IDM_EXAMPLE_IF:
                case IDM_EXAMPLE_LOOP: case IDM_EXAMPLE_FUNC: case IDM_EXAMPLE_ARRAY:
                case IDM_EXAMPLE_CLASS: case IDM_EXAMPLE_FILE: case IDM_EXAMPLE_MATH:
                case IDM_EXAMPLE_OOP: LoadExample(LOWORD(wParam)); break;

                case IDM_HELP_ABOUT:
                    MessageBoxA(hwnd,
                        "البيروني - بيئة التطوير العربية v2.0.0\n\n"
                        "لغة برمجة عربية كاملة مع القدرة على تجميع نفسها\n\n"
                        "الميزات:\n"
                        "- تلوين الصيغة التلقائي\n"
                        "- تحليل الأخطاء\n"
                        "- تنسيق الكود\n"
                        "- أمثلة جاهزة\n"
                        "- البناء الذاتي\n\n"
                        "صُنع بـ \xe2\x9d\xa4\xef\xb8\x8f للمطورين العرب",
                        "حول", MB_OK | MB_ICONINFORMATION);
                    break;
            } break;

        case WM_SIZE: {
            RECT rc; GetClientRect(hwnd, &rc);
            int w = rc.right - rc.left; int h = rc.bottom - rc.top;
            MoveWindow(g_editor, 10, 10, w / 2 - 20, h - 60, TRUE);
            MoveWindow(g_output, w / 2 + 10, 30, w / 2 - 20, h - 80, TRUE);
            MoveWindow(g_status, 10, h - 35, w - 20, 25, TRUE);
            break; }

        case WM_CLOSE:
            if (g_modified) {
                int result = MessageBoxA(hwnd, "هل تريد حفظ التغييرات؟", "تنبيه", MB_YESNOCANCEL | MB_ICONQUESTION);
                if (result == IDYES) SendMessage(hwnd, WM_COMMAND, IDM_FILE_SAVE, 0);
                else if (result == IDCANCEL) return 0;
            }
            DestroyWindow(hwnd); break;

        case WM_DESTROY: PostQuitMessage(0); break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
