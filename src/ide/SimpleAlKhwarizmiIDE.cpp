// SimpleAlBiruniIDE.cpp - بيئة تطوير البيروني (محدثة)
// Al-Biruni IDE - Updated with integrated tools & auto-detect compiler

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <commdlg.h>
#include <Richedit.h>
#include <commctrl.h>
#include <direct.h>
#include <io.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

// Globals
HWND g_hwnd;
HWND g_editor;
HWND g_output;
HWND g_status;
HWND g_hMainWnd;
HINSTANCE g_hInst;
std::string g_currentFile = "";
bool g_modified = false;
std::string g_compilerPath = "";

// Button IDs
#define ID_EDIT 1001
#define ID_OUTPUT 1002
#define ID_BTN_NEW 2001
#define ID_BTN_OPEN 2002
#define ID_BTN_SAVE 2003
#define ID_BTN_SAVEAS 2004
#define ID_BTN_RUN 2005
#define ID_BTN_COMPILE 2006
#define ID_BTN_FORMAT 2007
#define ID_BTN_SELFCHECK 2008
#define ID_STATUS 3001

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void UpdateTitle();
void AppendOutput(const char* text);
void RunCode();
void CompileAndRun();
void FormatCode();
void SelfCheck();
std::string GetCompilerPath();
std::string GetExeDir();
std::string RunCommand(const char* cmd);
std::string ReadFileContent(const std::string& path);

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
        exeDir + "\\build_release\\Release\\arabic_compiler.exe",
        exeDir + "\\..\\build_release\\Release\\arabic_compiler.exe",
        "arabic_compiler.exe"
    };
    for (int i = 0; i < 4; i++) {
        if (_access(paths[i].c_str(), 0) == 0) {
            return "\"" + paths[i] + "\"";
        }
    }
    return "";
}

std::string RunCommand(const char* cmd) {
    std::string result = "";
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return "خطأ: فشل في إنشاء الأنبوب";

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {0};
    char cmdCopy[4096];
    strncpy(cmdCopy, cmd, sizeof(cmdCopy) - 1);
    cmdCopy[sizeof(cmdCopy) - 1] = '\0';

    if (CreateProcessA(NULL, cmdCopy, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hWrite);
        char buffer[4096];
        DWORD bytesRead;
        while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            result += buffer;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        result = "خطأ: فشل في تشغيل الأمر\n";
    }
    CloseHandle(hRead);
    return result;
}

std::string ReadFileContent(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return content;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;
    g_compilerPath = GetCompilerPath();

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AlBiruniIDE";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&wc);

    g_hMainWnd = CreateWindowEx(0, "AlBiruniIDE",
        "البيروني - بيئة التطوير العربية v1.0.0",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 1300, 850, NULL, NULL, hInstance, NULL);

    if (!g_hMainWnd) {
        MessageBoxA(NULL, "Failed to create window", "Error", MB_ICONERROR);
        return 1;
    }

    LoadLibrary("RichEd20.dll");
    INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);

    // Toolbar
    HWND hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, "",
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_TOP | TBSTYLE_TOOLTIPS,
        0, 0, 0, 0, g_hMainWnd, NULL, hInstance, NULL);

    HIMAGELIST hImgList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 8, 0);
    HICON icons[] = {
        LoadIcon(NULL, IDI_APPLICATION),
        LoadIcon(NULL, IDI_APPLICATION),
        LoadIcon(NULL, IDI_APPLICATION),
        LoadIcon(NULL, IDI_APPLICATION),
        LoadIcon(NULL, IDI_APPLICATION),
        LoadIcon(NULL, IDI_APPLICATION),
        LoadIcon(NULL, IDI_APPLICATION),
        LoadIcon(NULL, IDI_APPLICATION),
    };
    for (int i = 0; i < 8; i++) ImageList_AddIcon(hImgList, icons[i]);
    SendMessage(hToolbar, TB_SETIMAGELIST, 0, (LPARAM)hImgList);
    SendMessage(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    TBBUTTON buttons[] = {
        {0, ID_BTN_NEW, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"جديد"},
        {1, ID_BTN_OPEN, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"فتح"},
        {2, ID_BTN_SAVE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"حفظ"},
        {0, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},
        {3, ID_BTN_RUN, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"تشغيل"},
        {4, ID_BTN_COMPILE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"تجميع"},
        {0, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},
        {5, ID_BTN_FORMAT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"تنسيق"},
        {6, ID_BTN_SELFCHECK, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"فحص ذاتي"},
    };
    SendMessage(hToolbar, TB_ADDBUTTONS, 9, (LPARAM)buttons);
    SendMessage(hToolbar, TB_AUTOSIZE, 0, 0);

    // Editor (left panel)
    g_editor = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS,
        "# مرحباً بك في البيروني!\r\nاطبع(\"مرحباً بالعالم!\")",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
        ES_AUTOHSCROLL | WS_HSCROLL | WS_VSCROLL | ES_WANTRETURN,
        10, 40, 640, 700, g_hMainWnd, (HMENU)ID_EDIT, hInstance, NULL);
    SendMessage(g_editor, WM_SETFONT, (WPARAM)CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ARABIC_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Consolas"), TRUE);

    // Output (right panel)
    HWND hOutputLabel = CreateWindow("STATIC", "المخرجات:",
        WS_CHILD | WS_VISIBLE, 670, 40, 100, 20, g_hMainWnd, NULL, hInstance, NULL);
    SendMessage(hOutputLabel, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    g_output = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
        ES_READONLY | WS_HSCROLL | WS_VSCROLL,
        670, 60, 600, 680, g_hMainWnd, (HMENU)ID_OUTPUT, hInstance, NULL);
    SendMessage(g_output, WM_SETFONT, (WPARAM)CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ARABIC_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Consolas"), TRUE);

    // Status bar
    g_status = CreateWindow("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 750, 1260, 25, g_hMainWnd, (HMENU)ID_STATUS, hInstance, NULL);
    SendMessage(g_status, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    // Initial status
    std::string statusText = "جاهز";
    if (!g_compilerPath.empty()) {
        statusText += " | المترجم: " + g_compilerPath;
    } else {
        statusText += " | ⚠️ لم يتم العثور على المترجم";
    }
    SetWindowTextA(g_status, statusText.c_str());

    // Welcome message
    std::string welcome = "═══════════════════════════════════════════════════════\n";
    welcome += "  مرحباً بك في بيئة التطوير العربية - البيروني v1.0.0\n";
    welcome += "═══════════════════════════════════════════════════════\n\n";
    if (!g_compilerPath.empty()) {
        welcome += "✅ المترجم: " + g_compilerPath + "\n";
    } else {
        welcome += "⚠️ لم يتم العثور على المترجم. يرجى بناء المشروع أولاً.\n";
        welcome += "   g++ -std=c++17 -O2 -Isrc/include -Isrc/core -Isrc/runtime ...\n";
    }
    welcome += "\nالأزرار المتاحة:\n";
    welcome += "  [تشغيل]   - تشغيل الكود مباشرة\n";
    welcome += "  [تجميع]   - تجميع إلى ملف .exe\n";
    welcome += "  [تنسيق]   - تنسيق الكود العربي\n";
    welcome += "  [فحص ذاتي] - اختبار البناء الذاتي\n";
    AppendOutput(welcome.c_str());

    ShowWindow(g_hMainWnd, SW_SHOW);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_NEW:
                    SetWindowTextA(g_editor, "# ملف جديد\r\n");
                    g_currentFile = "";
                    g_modified = false;
                    UpdateTitle();
                    break;

                case ID_BTN_OPEN: {
                    OPENFILENAMEA ofn = {0};
                    char szFile[MAX_PATH] = {0};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "Arabic Files\0*.عربي;*.arabic;*.ar\0All Files\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                    if (GetOpenFileNameA(&ofn)) {
                        std::string content = ReadFileContent(szFile);
                        if (!content.empty()) {
                            SetWindowTextA(g_editor, content.c_str());
                            g_currentFile = szFile;
                            g_modified = false;
                            UpdateTitle();
                            std::string msg = "تم فتح: " + std::string(szFile);
                            AppendOutput(msg.c_str());
                        }
                    }
                    break;
                }

                case ID_BTN_SAVE: {
                    if (g_currentFile.empty()) {
                        OPENFILENAMEA ofn = {0};
                        char szFile[MAX_PATH] = {0};
                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = hwnd;
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = sizeof(szFile);
                        ofn.lpstrFilter = "Arabic Files\0*.عربي\0All Files\0*.*\0";
                        ofn.lpstrDefExt = "عربي";
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

                        if (GetSaveFileNameA(&ofn)) {
                            g_currentFile = szFile;
                        } else { break; }
                    }

                    int len = GetWindowTextLengthA(g_editor);
                    char* buffer = new char[len + 1];
                    GetWindowTextA(g_editor, buffer, len + 1);

                    std::ofstream file(g_currentFile, std::ios::binary);
                    if (file.is_open()) {
                        file.write(buffer, len);
                        file.close();
                        g_modified = false;
                        UpdateTitle();
                        std::string msg = "تم الحفظ: " + g_currentFile;
                        AppendOutput(msg.c_str());
                    }
                    delete[] buffer;
                    break;
                }

                case ID_BTN_RUN:
                    RunCode();
                    break;

                case ID_BTN_COMPILE:
                    CompileAndRun();
                    break;

                case ID_BTN_FORMAT:
                    FormatCode();
                    break;

                case ID_BTN_SELFCHECK:
                    SelfCheck();
                    break;
            }
            break;

        case WM_SIZE: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;

            MoveWindow(g_editor, 10, 40, w / 2 - 20, h - 100, TRUE);
            MoveWindow(g_output, w / 2 + 10, 60, w / 2 - 20, h - 120, TRUE);
            MoveWindow(g_status, 10, h - 40, w - 20, 25, TRUE);
            break;
        }

        case WM_CLOSE:
            if (g_modified) {
                int result = MessageBoxA(hwnd, "هل تريد حفظ التغييرات؟", "تنبيه", MB_YESNOCANCEL | MB_ICONQUESTION);
                if (result == IDYES) {
                    SendMessage(hwnd, WM_COMMAND, ID_BTN_SAVE, 0);
                } else if (result == IDCANCEL) {
                    return 0;
                }
            }
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void UpdateTitle() {
    std::string title = "البيروني - بيئة التطوير العربية v1.0.0";
    if (!g_currentFile.empty()) {
        size_t pos = g_currentFile.find_last_of("\\/");
        title += " - " + g_currentFile.substr(pos + 1);
    }
    if (g_modified) title += " *";
    SetWindowTextA(g_hMainWnd, title.c_str());
}

void AppendOutput(const char* text) {
    int len = GetWindowTextLengthA(g_output);
    SendMessageA(g_output, EM_SETSEL, len, len);
    SendMessageA(g_output, EM_REPLACESEL, FALSE, (LPARAM)text);
}

void RunCode() {
    if (g_compilerPath.empty()) {
        AppendOutput("❌ خطأ: لم يتم العثور على المترجم!\n");
        AppendOutput("   يرجى بناء المشروع أولاً باستخدام:\n");
        AppendOutput("   g++ -std=c++17 -O2 ... -o arabic_compiler.exe\n\n");
        return;
    }

    int len = GetWindowTextLengthA(g_editor);
    char* code = new char[len + 1];
    GetWindowTextA(g_editor, code, len + 1);

    std::ofstream temp("temp_run.عربي", std::ios::binary);
    temp.write(code, len);
    temp.close();

    AppendOutput("▶️  جاري التشغيل...\n═══════════════════════════════════════════\n");

    std::string cmd = g_compilerPath + " --mode run temp_run.عربي 2>&1";
    std::string output = RunCommand(cmd.c_str());
    AppendOutput(output.c_str());

    AppendOutput("\n═══════════════════════════════════════════\n✅ انتهى التشغيل\n\n");

    delete[] code;
    DeleteFileA("temp_run.عربي");
}

void CompileAndRun() {
    if (g_compilerPath.empty()) {
        AppendOutput("❌ خطأ: لم يتم العثور على المترجم!\n\n");
        return;
    }

    int len = GetWindowTextLengthA(g_editor);
    char* code = new char[len + 1];
    GetWindowTextA(g_editor, code, len + 1);

    std::ofstream temp("temp_compile.عربي", std::ios::binary);
    temp.write(code, len);
    temp.close();

    AppendOutput("🔨 جاري التجميع...\n═══════════════════════════════════════════\n");

    std::string cmd = g_compilerPath + " --mode compiler temp_compile.عربي temp_output 2>&1";
    std::string output = RunCommand(cmd.c_str());
    AppendOutput(output.c_str());

    if (_access("temp_output.exe", 0) == 0) {
        AppendOutput("\n✅ تم التجميع بنجاح!\n▶️  جاري التشغيل...\n═══════════════════════════════════════════\n");

        STARTUPINFOA si = {0};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi = {0};

        char runCmd[] = "temp_output.exe";
        if (CreateProcessA(NULL, runCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            HANDLE hRead, hWrite;
            SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
            CreatePipe(&hRead, &hWrite, &sa, 0);

            si.hStdOutput = hWrite;
            si.hStdError = hWrite;
            si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            CloseHandle(hWrite);
            char buf[4096];
            DWORD br;
            while (ReadFile(hRead, buf, sizeof(buf) - 1, &br, NULL) && br > 0) {
                buf[br] = '\0';
                AppendOutput(buf);
            }
            CloseHandle(hRead);

            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

        AppendOutput("\n═══════════════════════════════════════════\n✅ انتهى\n\n");
        DeleteFileA("temp_output.exe");
    } else {
        AppendOutput("\n❌ فشل التجميع\n\n");
    }

    delete[] code;
    DeleteFileA("temp_compile.عربي");
}

void FormatCode() {
    int len = GetWindowTextLengthA(g_editor);
    char* code = new char[len + 1];
    GetWindowTextA(g_editor, code, len + 1);

    std::string input(code);
    std::string result = "";
    int indent = 0;

    size_t pos = 0;
    while (pos < input.size()) {
        size_t end = input.find('\n', pos);
        if (end == std::string::npos) end = input.size();
        std::string line = input.substr(pos, end - pos);

        // Trim
        size_t start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos) { pos = end + 1; continue; }
        line = line.substr(start);
        size_t endTrim = line.find_last_not_of(" \t\r");
        if (endTrim != std::string::npos) line = line.substr(0, endTrim + 1);

        if (line.empty()) { pos = end + 1; continue; }

        // Decrease indent on نهاية
        if (line == "نهاية") {
            indent--;
            if (indent < 0) indent = 0;
        }

        // Build indentation
        std::string indented = "";
        for (int i = 0; i < indent; i++) indented += "    ";
        indented += line + "\r\n";
        result += indented;

        // Increase indent
        if (line.find("دالة") == 0 || line.find("إذا") == 0 ||
            line.find("وإلا") == 0 || line.find("بينما") == 0 ||
            line.find("لكل") == 0 || line.find("صف") == 0 ||
            line.find("صنف") == 0 || line.find("عام:") == 0 ||
            line.find("خاص:") == 0) {
            indent++;
        }

        pos = end + 1;
    }

    SetWindowTextA(g_editor, result.c_str());
    g_modified = true;
    UpdateTitle();

    AppendOutput("✅ تم تنسيق الكود بنجاح\n\n");
    delete[] code;
}

void SelfCheck() {
    if (g_compilerPath.empty()) {
        AppendOutput("❌ خطأ: لم يتم العثور على المترجم!\n\n");
        return;
    }

    AppendOutput("🔍 فحص ذاتي - Self-Hosting Check\n");
    AppendOutput("═══════════════════════════════════════════\n");

    std::string cmd = g_compilerPath + " --mode selfhost 2>&1";
    AppendOutput("جاري تشغيل البناء الذاتي...\n");
    AppendOutput(("الأمر: " + cmd + "\n\n").c_str());

    std::string output = RunCommand(cmd.c_str());
    AppendOutput(output.c_str());

    AppendOutput("\n═══════════════════════════════════════════\n");

    // Check for generated files
    if (_access("self_hosted_output/lexer.exe", 0) == 0) {
        AppendOutput("✅ المحلل اللغوي: lexer.exe\n");
    } else {
        AppendOutput("⚠️ المحلل اللغوي: لم يتم إنتاج EXE\n");
    }
    if (_access("self_hosted_output/codegen.exe", 0) == 0) {
        AppendOutput("✅ مولد الكود: codegen.exe\n");
    } else {
        AppendOutput("⚠️ مولد الكود: لم يتم إنتاج EXE\n");
    }
    if (_access("self_hosted_output/compiler_v3.exe", 0) == 0) {
        AppendOutput("✅ المترجم الذاتي: compiler_v3.exe\n");
    } else {
        AppendOutput("⚠️ المترجم الذاتي: لم يتم إنتاج EXE\n");
    }

    AppendOutput("\n✅ انتهى الفحص الذاتي\n\n");
}
