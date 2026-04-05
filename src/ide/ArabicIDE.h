// ArabicIDE.h - بيئة التطوير المتكاملة العربية "البيروني"
// Arabic Integrated Development Environment

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../core/SafeWindows.h" // Explicitly include windows.h

#include "../modules/ui/ArabicUI.h"
#include "../modules/ui/ArabicCodeEditor.h"
#include "../modules/graphics/ArabicGameEngine.h"
#include "../core/ArabicCompiler.h"
#include "../core/PackageManager.h"
#include "../core/ArabicJITCompiler.h"
#include "../utils/debug/InteractiveDebugger.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <set>

namespace ArabicLanguage {

/**
 * @brief إعدادات البيئة
 */
struct IDEConfig {
    int windowWidth = 1200;
    int windowHeight = 800;
    std::string windowTitle = "البيروني - بيئة التطوير العربية";
    std::string defaultFontName = "Courier New";
    float defaultFontSize = 14.0f;
    bool darkMode = true;
    bool autoSave = true;
    int autoSaveInterval = 60; // ثانية
    
    IDEConfig() = default;
};

/**
 * @brief بيئة التطوير المتكاملة العربية
 * Arabic Integrated Development Environment
 */
class ArabicIDE {
public:
    /**
     * @brief حالة البيئة
     */
    enum class IDEState {
        STATE_IDLE,           // جاهز
        STATE_EDITING,        // تحرير
        STATE_COMPILING,      // ترجمة
        STATE_RUNNING,        // تشغيل
        STATE_ERROR           // خطأ
    };

    /**
     * @brief رسالة في لوحة المخرجات
     */
    struct OutputMessage {
        enum Type { INFO, SUCCESS, WARNING, ERROR_MSG, RESULT };
        Type type;
        std::string message;
        int lineNumber; // -1 if not applicable
        
        OutputMessage(Type t, const std::string& msg, int line = -1)
            : type(t), message(msg), lineNumber(line) {}
    };

private:
    // الحالة
    IDEState state;
    IDEConfig config;
    
    // المكونات الرئيسية
    std::shared_ptr<ArabicGameEngine> engine;
    std::shared_ptr<ArabicGraphics> graphics;
    std::shared_ptr<ArabicUI> ui;
    std::shared_ptr<ArabicCodeEditor> codeEditor;
    std::unique_ptr<ArabicCompiler> compiler;
    
    // ✅ مكونات الذكاء المدمجة
    std::unique_ptr<class ArabicSyntaxHighlighter> highlighter;
    std::unique_ptr<class ArabicErrorAnalyzer> errorAnalyzer;

    // ✅ المكونات المربوطة حديثاً
    std::unique_ptr<PackageManager>      packageManager;
    std::unique_ptr<InteractiveDebugger> debugger;
    bool useJIT        = false;   // تشغيل عبر JIT
    bool isDebugging   = false;   // وضع التصحيح نشط
    int  debugLine     = -1;      // السطر الحالي في التصحيح
    std::set<int> breakpointLines; // أرقام السطور التي بها نقاط توقف
    
    // الملفات
    std::string currentFilePath;
    std::string currentFileName;
    bool hasUnsavedChanges;
    
    // المخرجات
    std::vector<OutputMessage> outputMessages;
    
    // عناصر الواجهة
    HWND mainWindow;
    HWND editControl;
    HWND outputControl;
    HWND statusBar;
    HWND toolbar;
    
    // خيارات التشغيل
    bool compileBeforeRun; // ✅ = ترجمة ثم تشغيل، ❌ = تشغيل فقط
    bool isFormatting;     // حماية من التكرار اللانهائي أثناء التنسيق
    
    // أزرار الواجهة
    HWND btnNew;
    HWND btnOpen;
    HWND btnSave;
    HWND btnRun;
    HWND chkCompileRun; // Checkbox للترجمة والتشغيل

public:
    ArabicIDE();
    ~ArabicIDE();
    
    // منع النسخ
    ArabicIDE(const ArabicIDE&) = delete;
    ArabicIDE& operator=(const ArabicIDE&) = delete;
    
    /**
     * @brief تهيئة البيئة
     */
    bool initialize(HINSTANCE hInstance, const IDEConfig& cfg = IDEConfig());
    
    /**
     * @brief تشغيل البيئة
     */
    int run();
    
    /**
     * @brief إغلاق البيئة
     */
    void shutdown();
    
    // إدارة الملفات
    bool newFile();
    bool openFile(const std::string& path = "");
    bool saveFile();
    bool saveFileAs();
    
    // الترجمة والتشغيل
    bool compile();
    bool runCode();
    bool compileAndRun();
    void stopExecution();
    
    // واجهة المستخدم
    void clearOutput();
    void addOutput(const OutputMessage& msg);
    void updateStatusBar(const std::string& text);
    
    // ✅ وظائف المحرر الذكي
    void applySyntaxHighlighting();
    void analyzeCodeErrors();

    // ✅ وظائف المصحح (Debugger)
    void startDebugging();
    void stopDebugging();
    void stepDebug();
    void stepOverDebug();
    void toggleBreakpoint();          // F9 - نقطة توقف عند السطر الحالي
    void showBreakpoints();
    void highlightDebugLine(int line);

    // ✅ دليل المساعدة
    void showHelpDialog();

    // ✅ وظائف مدير الحزم
    void showPackageManager();

    // ✅ وظائف JIT
    void toggleJIT();
    bool runWithJIT(const std::string& code);
    
    // الحصول على الحالة
    IDEState getState() const { return state; }
    bool hasChanges() const { return hasUnsavedChanges; }
    const std::string& getCurrentFile() const { return currentFilePath; }
    
    // إعدادات التشغيل
    void setCompileBeforeRun(bool value) { compileBeforeRun = value; }
    bool getCompileBeforeRun() const { return compileBeforeRun; }

private:
    // إنشاء الواجهة
    bool createMainWindow(HINSTANCE hInstance);
    void createToolbar();
    void createEditor();
    void createOutputPanel();
    void createStatusBar();
    void createMenus();
    
    // معالجات الأحداث
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void handleCommand(WPARAM wParam);
    void handleResize(int width, int height);
    
    // مساعدات
    std::string getCodeFromEditor();
    void setCodeInEditor(const std::string& code);
    void highlightErrorLine(int lineNumber);
    std::string showOpenDialog();
    std::string showSaveDialog();
    bool confirmSaveChanges();
    void showExamplesMenu();
    void loadExample(int index);
};

// ثوابت معرفات الأوامر
constexpr int ID_FILE_NEW = 1001;
constexpr int ID_FILE_OPEN = 1002;
constexpr int ID_FILE_SAVE = 1003;
constexpr int ID_FILE_SAVEAS = 1004;
constexpr int ID_FILE_EXIT = 1005;

constexpr int ID_EDIT_UNDO = 2001;
constexpr int ID_EDIT_REDO = 2002;
constexpr int ID_EDIT_CUT = 2003;
constexpr int ID_EDIT_COPY = 2004;
constexpr int ID_EDIT_PASTE = 2005;

constexpr int ID_RUN_RUN = 3001;
constexpr int ID_RUN_COMPILE = 3002;
constexpr int ID_RUN_STOP = 3003;
constexpr int ID_RUN_TOGGLE = 3004; // Checkbox للتبديل بين التشغيل والترجمة

constexpr int ID_TOOLS_SELF_UPDATE   = 4003;
constexpr int ID_TOOLS_PACKAGES      = 4010;  // مدير الحزم
constexpr int ID_RUN_JIT             = 4011;  // تشغيل بـ JIT

// ✅ قائمة التصحيح
constexpr int ID_DEBUG_START         = 4020;
constexpr int ID_DEBUG_STOP          = 4021;
constexpr int ID_DEBUG_STEP          = 4022;  // F10
constexpr int ID_DEBUG_STEP_OVER     = 4023;  // F11
constexpr int ID_DEBUG_TOGGLE_BP     = 4024;  // F9
constexpr int ID_DEBUG_SHOW_BP       = 4025;

constexpr int ID_HELP_ABOUT   = 5001;
constexpr int ID_HELP_ABOUT_APP = 5003; // حول البرنامج فقط
constexpr int ID_HELP_DOCS    = 5002;

// معرفات عناصر الواجهة
constexpr int ID_EDIT_CONTROL = 6001;
constexpr int ID_CODE_EDITOR = 6002; // ✅ New custom editor
constexpr int ID_OUTPUT_CONTROL = 6003;
constexpr int ID_STATUS_BAR = 6004;
constexpr int ID_TOOLBAR = 6005;
constexpr int ID_CHK_COMPILE_RUN = 6006;

// قاعدة معرفات الأمثلة (لتجنب التداخل مع معرفات العناصر)
constexpr int ID_EXAMPLE_BASE = 7000;

} // namespace ArabicLanguage
