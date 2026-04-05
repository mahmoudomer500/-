// ArabicGUI.h - واجهة المستخدم الرسومية العربية
// Arabic Graphical User Interface - Win32 API Wrapper

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../core/SafeWindows.h"
#include "../core/ArabicRuntime.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace ArabicLanguage {

// ══════════════════════════════════════════════════════════════
// أنواع الأدوات
// ══════════════════════════════════════════════════════════════

enum class ArabicWidgetType {
    WIDGET_WINDOW,
    WIDGET_BUTTON,
    WIDGET_LABEL,
    WIDGET_TEXTBOX,
    WIDGET_IMAGE,
    WIDGET_CHECKBOX,
    WIDGET_COMBOBOX,
    WIDGET_PROGRESSBAR,
    WIDGET_SLIDER,
    WIDGET_LISTBOX
};

// ══════════════════════════════════════════════════════════════
// معالج الأحداث (Event Handler)
// ══════════════════════════════════════════════════════════════

using ArabicEventCallback = std::function<void()>;

// ══════════════════════════════════════════════════════════════
// الأداة الأساسية (Base Widget)
// ══════════════════════════════════════════════════════════════

class ArabicWidget {
public:
    ArabicWidget();
    virtual ~ArabicWidget() = default;

    // الموقع والحجم
    void setPosition(int x, int y);
    void setSize(int w, int h);
    int getX() const { return m_x; }
    int getY() const { return m_y; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    // الظهور
    void show();
    void hide();
    bool isVisible() const { return m_visible; }

    // التفعيل
    void enable();
    void disable();
    bool isEnabled() const { return m_enabled; }

    // النص
    void setText(const std::string& text);
    std::string getText() const;

    // الأحداث
    void onClick(ArabicEventCallback callback);
    void onChange(ArabicEventCallback callback);
    ArabicEventCallback getOnClick() const { return m_onClick; }

    // HWND
    HWND getHandle() const { return m_hwnd; }
    void setHandle(HWND hwnd) { m_hwnd = hwnd; }

    // النوع
    ArabicWidgetType getType() const { return m_type; }

protected:
    HWND m_hwnd = NULL;
    int m_x = 0, m_y = 0;
    int m_width = 100, m_height = 30;
    bool m_visible = true;
    bool m_enabled = true;
    std::string m_text = "";
    ArabicWidgetType m_type = ArabicWidgetType::WIDGET_WINDOW;

    ArabicEventCallback m_onClick;
    ArabicEventCallback m_onChange;
};

// ══════════════════════════════════════════════════════════════
// النافذة الرئيسية (Main Window)
// ══════════════════════════════════════════════════════════════

class ArabicWindow : public ArabicWidget {
public:
    ArabicWindow();
    ArabicWindow(const std::string& title, int width = 800, int height = 600);
    ~ArabicWindow() override;

    // العنوان
    void setTitle(const std::string& title);
    std::string getTitle() const;

    // إنشاء النافذة
    void create();
    void destroy();

    // إضافة أدوات
    void addWidget(std::shared_ptr<ArabicWidget> widget);

    // تشغيل حلقة الأحداث
    void run();

    // إغلاق النافذة
    void close();

    // اللون
    void setBackgroundColor(int r, int g, int b);

    // مركز الشاشة
    void centerOnScreen();

    // تغيير الحجم
    void setResizable(bool resizable);

    // أيقونة
    void setIcon(const std::string& iconPath);

    // حالة النافذة
    void maximize();
    void minimize();
    void restore();

private:
    std::vector<std::shared_ptr<ArabicWidget>> m_widgets;
    std::string m_title = "نافذة عربية";
    bool m_running = false;
    int m_bgR = 240, m_bgG = 240, m_bgB = 240;
    bool m_resizable = true;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static std::unordered_map<HWND, ArabicWindow*> s_windowMap;

    void registerWindow();
    void unregisterWindow();
};

// ══════════════════════════════════════════════════════════════
// زر (Button)
// ══════════════════════════════════════════════════════════════

class ArabicButton : public ArabicWidget {
public:
    ArabicButton();
    ArabicButton(const std::string& text, int x = 0, int y = 0, int w = 100, int h = 30);

    void create(HWND parent);

    // لون الزر
    void setButtonColor(int r, int g, int b);
    void setTextColor(int r, int g, int b);

    // خط الزر
    void setFontSize(int size);
    void setFontBold(bool bold);

private:
    int m_btnR = 0, m_btnG = 120, m_btnB = 215;
    int m_txtR = 255, m_txtG = 255, m_txtB = 255;
    int m_fontSize = 14;
    bool m_fontBold = true;
};

// ══════════════════════════════════════════════════════════════
// نص (Label)
// ══════════════════════════════════════════════════════════════

class ArabicLabel : public ArabicWidget {
public:
    ArabicLabel();
    ArabicLabel(const std::string& text, int x = 0, int y = 0, int w = 200, int h = 30);

    void create(HWND parent);

    // التنسيق
    void setAlignment(const std::string& align); // "يمين"، "وسط"، "يسار"
    void setFontSize(int size);
    void setFontColor(int r, int g, int b);
    void setFontBold(bool bold);

private:
    std::string m_alignment = "يمين";
    int m_fontSize = 14;
    int m_fontR = 0, m_fontG = 0, m_fontB = 0;
    bool m_fontBold = false;
};

// ══════════════════════════════════════════════════════════════
// مربع نص (TextBox)
// ══════════════════════════════════════════════════════════════

class ArabicTextBox : public ArabicWidget {
public:
    ArabicTextBox();
    ArabicTextBox(const std::string& placeholder, int x = 0, int y = 0, int w = 200, int h = 30);

    void create(HWND parent);

    // خصائص
    void setReadOnly(bool readOnly);
    void setPasswordMode(bool password);
    void setMaxLength(int maxLen);
    void selectAll();
    std::string getSelectedText() const;

    // خط
    void setFontSize(int size);

private:
    bool m_readOnly = false;
    bool m_password = false;
    int m_maxLength = 0;
    int m_fontSize = 14;
};

// ══════════════════════════════════════════════════════════════
// مربع اختيار (CheckBox)
// ══════════════════════════════════════════════════════════════

class ArabicCheckBox : public ArabicWidget {
public:
    ArabicCheckBox();
    ArabicCheckBox(const std::string& text, int x = 0, int y = 0);

    void create(HWND parent);

    // الحالة
    void setChecked(bool checked);
    bool isChecked() const;

private:
    bool m_checked = false;
};

// ══════════════════════════════════════════════════════════════
// قائمة منسدلة (ComboBox)
// ══════════════════════════════════════════════════════════════

class ArabicComboBox : public ArabicWidget {
public:
    ArabicComboBox();
    ArabicComboBox(int x = 0, int y = 0, int w = 150, int h = 30);

    void create(HWND parent);

    // العناصر
    void addItem(const std::string& item);
    void addItems(const std::vector<std::string>& items);
    void clearItems();
    int getItemCount() const;

    // التحديد
    void setSelectedIndex(int index);
    int getSelectedIndex() const;
    std::string getSelectedItem() const;

private:
    std::vector<std::string> m_items;
    int m_selectedIndex = -1;
};

// ══════════════════════════════════════════════════════════════
// شريط تقدم (ProgressBar)
// ══════════════════════════════════════════════════════════════

class ArabicProgressBar : public ArabicWidget {
public:
    ArabicProgressBar();
    ArabicProgressBar(int x = 0, int y = 0, int w = 200, int h = 20);

    void create(HWND parent);

    // القيم
    void setRange(int min, int max);
    void setValue(int value);
    int getValue() const;
    void step();

private:
    int m_min = 0, m_max = 100;
    int m_value = 0;
};

// ══════════════════════════════════════════════════════════════
// مدير الواجهة الرسومية (GUI Manager)
// ══════════════════════════════════════════════════════════════

class ArabicGUI {
public:
    ArabicGUI();
    ~ArabicGUI();

    // إنشاء نافذة
    std::shared_ptr<ArabicWindow> createWindow(const std::string& title, int width = 800, int height = 600);

    // إنشاء أدوات
    std::shared_ptr<ArabicButton> createButton(const std::string& text, int x, int y, int w = 100, int h = 30);
    std::shared_ptr<ArabicLabel> createLabel(const std::string& text, int x, int y, int w = 200, int h = 30);
    std::shared_ptr<ArabicTextBox> createTextBox(const std::string& placeholder, int x, int y, int w = 200, int h = 30);
    std::shared_ptr<ArabicCheckBox> createCheckBox(const std::string& text, int x, int y);
    std::shared_ptr<ArabicComboBox> createComboBox(int x, int y, int w = 150, int h = 30);
    std::shared_ptr<ArabicProgressBar> createProgressBar(int x, int y, int w = 200, int h = 20);

    // تشغيل حلقة الأحداث الرئيسية
    void runMainLoop();

    // رسالة تنبيه
    void showMessage(const std::string& title, const std::string& message, const std::string& type = "معلومات");

    // رسالة إدخال
    std::string showInputDialog(const std::string& title, const std::string& prompt);

    // رسالة اختيار
    bool showConfirmDialog(const std::string& title, const std::string& message);

    // تحديث الشاشة
    void refresh();

    // إغلاق جميع النوافذ
    void closeAllWindows();

    // تسجيل الدوال في المترجم
    static void registerFunctions(std::shared_ptr<ArabicRuntime> runtime,
                                  std::function<void(const std::string&)> outputCallback = nullptr);

private:
    std::vector<std::shared_ptr<ArabicWindow>> m_windows;
    std::shared_ptr<ArabicRuntime> m_runtime;
    std::function<void(const std::string&)> m_outputCallback;

    // النافذة النشطة
    ArabicWindow* m_activeWindow = nullptr;

    // معالجة الأحداث
    void processEvents();
};

} // namespace ArabicLanguage
