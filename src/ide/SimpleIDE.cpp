// SimpleIDE.cpp - بيئة تطوير بسيطة بدون OpenGL
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <richedit.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "comctl32.lib")
#include <commctrl.h>

// Globals
HWND g_hMain, g_hEditor, g_hOutput, g_hStatus;
std::string g_CurrentFile;
bool g_Modified = false;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE h, HINSTANCE p, LPSTR cmd, int show) {
    // Set UTF-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    // Register
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = h;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 40));
    wc.lpszClassName = "AlBiruniIDE";
    RegisterClassEx(&wc);
    
    // Create window
    g_hMain = CreateWindowEx(0, "AlBiruniIDE",
        "🕌 البيروني - بيئة التطوير العربية v1.0.0",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 1400, 900, 0, 0, h, 0);
    
    if (!g_hMain) return 1;
    
    // Init RichEdit
    LoadLibrary("RichEd20.dll");
    
    // Create toolbar
    HWND hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, "", 
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT,
        0, 0, 0, 0, g_hMain, 0, h, 0);
    
    TBBUTTON buttons[] = {
        {0, 1, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 'N'},
        {1, 2, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 'O'},
        {2, 3, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 'S'},
        {0, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},
        {3, 4, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 'R'},
        {4, 5, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 'C'},
    };
    
    TBADDBITMAP tbAdd = {(HINSTANCE)0, 0};
    SendMessage(hToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbAdd);
    SendMessage(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(hToolbar, TB_ADDBUTTONS, 6, (LPARAM)buttons);
    SendMessage(hToolbar, TB_AUTOSIZE, 0, 0);
    
    // Create split windows
    HWND hSplitter = CreateWindow("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_BLACKFRAME,
        700, 40, 4, 800, g_hMain, 0, h, 0);
    
    // Editor panel label
    HWND hEditorLabel = CreateWindow("STATIC", "📝 محرر الكود",
        WS_CHILD | WS_VISIBLE,
        10, 45, 200, 25, g_hMain, 0, h, 0);
    SendMessage(hEditorLabel, WM_SETFONT, (WPARAM)CreateFont(16, 0, 0, 0, FW_BOLD, 0, 0, 0, 
        ARABIC_CHARSET, 0, 0, 0, 0, "Segoe UI"), 0);
    
    // Code editor
    g_hEditor = CreateWindowEx(0, RICHEDIT_CLASS, 
        "// 🕌 مرحباً بك في لغة البرمجة العربية!\n"
        "// ═══════════════════════════════════════════════════════\n\n"
        "// مثال 1: الطباعة\n"
        "اطبع(\"مرحباً يا أحمد!\")\n\n"
        "// مثال 2: المتغيرات\n"
        "مت اسم = \"محمد\"\n"
        "مت عمر = 25\n"
        "اطبع(\"الاسم: \" + اسم)\n"
        "اطبع(\"العمر: \" + عمر)\n\n"
        "// مثال 3: الدوال\n"
        "دالة جمع(أ، ب):\n"
        "    أرجع أ + ب\n"
        "نهاية\n\n"
        "// مثال 4: الشروط\n"
        "مت درجة = 85\n"
        "اذا درجة >= 90:\n"
        "    اطبع(\"ممتاز\")\n"
        "وإلا:\n"
        "    اطبع(\"جيد\")\n"
        "نهاية\n",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_HSCROLL | WS_VSCROLL,
        10, 75, 680, 750, g_hMain, 0, h, 0);
    
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, ARABIC_CHARSET, 0, 0, 0, 0, "Courier New");
    SendMessage(g_hEditor, WM_SETFONT, (WPARAM)hFont, 0);
    
    // Output panel label
    HWND hOutLabel = CreateWindow("STATIC", "📊 المخرجات",
        WS_CHILD | WS_VISIBLE,
        720, 45, 200, 25, g_hMain, 0, h, 0);
    SendMessage(hOutLabel, WM_SETFONT, (WPARAM)CreateFont(16, 0, 0, 0, FW_BOLD, 0, 0, 0,
        ARABIC_CHARSET, 0, 0, 0, 0, "Segoe UI"), 0);
    
    // Output panel
    g_hOutput = CreateWindowEx(0, RICHEDIT_CLASS,
        "═══════════════════════════════════════════════════════════\n"
        "  🕌 البيروني - بيئة التطوير العربية v1.0.0\n"
        "═══════════════════════════════════════════════════════════\n\n"
        "  ✅ المترجم جاهز للتشغيل\n"
        "  📝 اكتب الكود في المحرر على اليسار\n"
        "  ▶️ اضغط 'تشغيل' لتشغيل الكود مباشرة\n"
        "  🔨 اضغط 'تجميع' لتجميع وتشغيل\n\n"
        "───────────────────────────────────────────────────────────\n"
        "  أمثلة على الأوامر:\n"
        "  • اطبع(\"نص\")     - لطباعة نص\n"
        "  • مت س = 42       - لتعريف متغير\n"
        "  • دالة جمع(أ، ب)  - لتعريف دالة\n"
        "  • اذا ... :        - للشرط\n"
        "  • بينما ... :      - للحلقة\n"
        "───────────────────────────────────────────────────────────\n",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        715, 75, 660, 750, g_hMain, 0, h, 0);
    
    SendMessage(g_hOutput, WM_SETFONT, (WPARAM)hFont, 0);
    
    // Status bar
    g_hStatus = CreateWindow("STATIC", "  جاهز  |  UTF-8  |  السطر: 1",
        WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
        10, 830, 1360, 28, g_hMain, 0, h, 0);
    SendMessage(g_hStatus, WM_SETFONT, (WPARAM)CreateFont(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        ARABIC_CHARSET, 0, 0, 0, 0, "Segoe UI"), 0);
    
    // Set colors
    SendMessage(g_hEditor, EM_SETBKGNDCOLOR, 0, RGB(25, 25, 35));
    SendMessage(g_hOutput, EM_SETBKGNDCOLOR, 0, RGB(20, 20, 30));
    
    ShowWindow(g_hMain, SW_SHOW);
    UpdateWindow(g_hMain);
    
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

void AppendOutput(const char* text, bool success = true) {
    SetWindowText(g_hOutput, text);
}

std::string GetEditorText() {
    int len = GetWindowTextLength(g_hEditor);
    std::vector<char> buf(len + 1);
    GetWindowText(g_hEditor, buf.data(), len + 1);
    return std::string(buf.data(), len);
}

void SetStatus(const char* text) {
    SetWindowText(g_hStatus, text);
}

void RunCode() {
    std::string code = GetEditorText();
    if (code.empty()) {
        AppendOutput("❌ لا يوجد كود للتشغيل!");
        return;
    }
    
    // Save temp file
    std::ofstream f("temp_code.عربي");
    f << code;
    f.close();
    
    AppendOutput("▶️  جاري التشغيل...\n═══════════════════════════════════════════════════════════\n\n");
    SetStatus("  ▶️  جاري التشغيل...");
    
    // Run
    STARTUPINFO si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    
    char cmd[260];
    sprintf(cmd, "المترجم\\arabic_v17.exe --mode run temp_code.عربي");
    
    if (CreateProcess(0, cmd, 0, 0, 0, CREATE_NO_WINDOW, 0, 0, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 15000);
        
        // Read output
        std::ifstream out("temp_output.txt");
        std::stringstream ss;
        if (out) {
            ss << out.rdbuf();
            out.close();
        }
        
        std::string result = ss.str();
        if (result.empty()) result = "(لا يوجد مخرجات)";
        
        char buf[5000];
        sprintf(buf, "▶️  جاري التشغيل...\n═══════════════════════════════════════════════════════════\n\n%s\n\n═══════════════════════════════════════════════════════════\n✅ انتهى التشغيل بنجاح!", result.c_str());
        AppendOutput(buf, true);
        SetStatus("  ✅ انتهى التشغيل");
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        AppendOutput("❌ فشل في تشغيل الكود!\n\n💡 تأكد من وجود المترجم في مجلد 'المترجم'", false);
        SetStatus("  ❌ فشل التشغيل");
    }
    
    DeleteFile("temp_code.عربي");
    DeleteFile("temp_output.txt");
}

void CompileCode() {
    std::string code = GetEditorText();
    if (code.empty()) {
        AppendOutput("❌ لا يوجد كود للتجميع!");
        return;
    }
    
    std::ofstream f("temp_code.عربي");
    f << code;
    f.close();
    
    AppendOutput("🔨 جاري التجميع...\n═══════════════════════════════════════════════════════════\n");
    SetStatus("  🔨 جاري التجميع...");
    
    // Compile
    STARTUPINFO si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    
    char cmd[260];
    sprintf(cmd, "المترجم\\arabic_v17.exe --mode compiler temp_code.عربي temp_output");
    
    if (CreateProcess(0, cmd, 0, 0, 0, CREATE_NO_WINDOW, 0, 0, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 30000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (GetFileAttributes("temp_output.exe") != INVALID_FILE_ATTRIBUTES) {
            AppendOutput("✅ تم التجميع بنجاح!\n═══════════════════════════════════════════════════════════\n\n▶️  جاري التشغيل...\n\n");
            SetStatus("  ✅ تم التجميع - جاري التشغيل...");
            
            // Run compiled
            STARTUPINFO si2 = {};
            PROCESS_INFORMATION pi2 = {};
            si2.cb = sizeof(si2);
            
            if (CreateProcess(0, "temp_output.exe", 0, 0, 0, CREATE_NO_WINDOW, 0, 0, &si2, &pi2)) {
                WaitForSingleObject(pi2.hProcess, 15000);
                CloseHandle(pi2.hProcess);
                CloseHandle(pi2.hThread);
            }
            
            AppendOutput("✅ تم التجميع والتشغيل بنجاح!\n═══════════════════════════════════════════════════════════");
            SetStatus("  ✅ تم التجميع والتشغيل");
            DeleteFile("temp_output.exe");
        } else {
            AppendOutput("❌ فشل التجميع!\n\n💡 راجع الكود وأعد المحاولة", false);
            SetStatus("  ❌ فشل التجميع");
        }
    } else {
        AppendOutput("❌ فشل في تشغيل المترجم!", false);
        SetStatus("  ❌ فشل المترجم");
    }
    
    DeleteFile("temp_code.عربي");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case 1: { // New
                    SetWindowText(g_hEditor, "// ملف جديد\n");
                    g_CurrentFile = "";
                    g_Modified = false;
                    SetWindowText(g_hMain, "🕌 البيروني - بيئة التطوير العربية v1.0.0");
                    AppendOutput("✅ تم إنشاء ملف جديد\n═══════════════════════════════════════════════════════════");
                    break;
                }
                case 2: { // Open
                    OPENFILENAME ofn = {};
                    char file[260] = {};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = file;
                    ofn.nMaxFile = sizeof(file);
                    ofn.lpstrFilter = "ملفات عربية\0*.عربي;*.arabic\0كل الملفات\0*.*\0";
                    ofn.nFilterIndex = 1;
                    
                    if (GetOpenFileName(&ofn)) {
                        std::ifstream f(ofn.lpstrFile);
                        if (f) {
                            std::stringstream ss;
                            ss << f.rdbuf();
                            SetWindowText(g_hEditor, ss.str().c_str());
                            g_CurrentFile = ofn.lpstrFile;
                            g_Modified = false;
                            char title[300];
                            sprintf(title, "🕌 البيروني - %s", ofn.lpstrFile);
                            SetWindowText(g_hMain, title);
                            AppendOutput("✅ تم فتح الملف بنجاح!\n═══════════════════════════════════════════════════════════");
                        }
                    }
                    break;
                }
                case 3: { // Save
                    if (g_CurrentFile.empty()) {
                        OPENFILENAME ofn = {};
                        char file[260] = {};
                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = hwnd;
                        ofn.lpstrFile = file;
                        ofn.nMaxFile = sizeof(file);
                        ofn.lpstrFilter = "ملفات عربية\0*.عربي\0كل الملفات\0*.*\0";
                        ofn.lpstrDefExt = "عربي";
                        if (!GetSaveFileName(&ofn)) break;
                        g_CurrentFile = ofn.lpstrFile;
                    }
                    std::ofstream f(g_CurrentFile);
                    if (f) {
                        f << GetEditorText();
                        f.close();
                        g_Modified = false;
                        char title[300];
                        sprintf(title, "🕌 البيروني - %s", g_CurrentFile.c_str());
                        SetWindowText(g_hMain, title);
                        AppendOutput("✅ تم الحفظ بنجاح!\n═══════════════════════════════════════════════════════════");
                    }
                    break;
                }
                case 4: // Run
                    RunCode();
                    break;
                case 5: // Compile
                    CompileCode();
                    break;
            }
            break;
            
        case WM_SIZE: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            MoveWindow(g_hEditor, 10, 75, 680, rc.bottom - 115, TRUE);
            MoveWindow(g_hOutput, 715, 75, rc.right - 730, rc.bottom - 115, TRUE);
            MoveWindow(g_hStatus, 10, rc.bottom - 40, rc.right - 20, 28, TRUE);
            break;
        }
        
        case WM_CLOSE: {
            if (g_Modified) {
                int r = MessageBox(hwnd, "هل تريد حفظ التغييرات؟", "تنبيه", MB_YESNOCANCEL);
                if (r == IDYES) SendMessage(hwnd, WM_COMMAND, 3, 0);
                else if (r == IDCANCEL) return 0;
            }
            DestroyWindow(hwnd);
            break;
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
