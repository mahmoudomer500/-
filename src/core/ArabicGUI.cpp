// ArabicGUI.cpp - تنفيذ واجهة المستخدم الرسومية العربية
// Arabic GUI Implementation - Win32 API

#include "ArabicGUI.h"
#include "ArabicRuntime.h"
#include <commctrl.h>
#include <richedit.h>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")

namespace ArabicLanguage {

// ══════════════════════════════════════════════════════════════
// ArabicWidget Implementation
// ══════════════════════════════════════════════════════════════

ArabicWidget::ArabicWidget() {}

void ArabicWidget::setPosition(int x, int y) {
    m_x = x; m_y = y;
    if (m_hwnd) MoveWindow(m_hwnd, x, y, m_width, m_height, TRUE);
}

void ArabicWidget::setSize(int w, int h) {
    m_width = w; m_height = h;
    if (m_hwnd) MoveWindow(m_hwnd, m_x, m_y, w, h, TRUE);
}

void ArabicWidget::show() {
    m_visible = true;
    if (m_hwnd) ShowWindow(m_hwnd, SW_SHOW);
}

void ArabicWidget::hide() {
    m_visible = false;
    if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
}

void ArabicWidget::enable() {
    m_enabled = true;
    if (m_hwnd) EnableWindow(m_hwnd, TRUE);
}

void ArabicWidget::disable() {
    m_enabled = false;
    if (m_hwnd) EnableWindow(m_hwnd, FALSE);
}

void ArabicWidget::setText(const std::string& text) {
    m_text = text;
    if (m_hwnd) {
        int wLen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        std::wstring wText(wLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wText[0], wLen);
        SetWindowTextW(m_hwnd, wText.c_str());
    }
}

std::string ArabicWidget::getText() const {
    if (!m_hwnd) return m_text;
    int len = GetWindowTextLengthW(m_hwnd);
    if (len == 0) return "";
    std::wstring wText(len + 1, 0);
    GetWindowTextW(m_hwnd, &wText[0], len + 1);
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string text(utf8Len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, &text[0], utf8Len, nullptr, nullptr);
    if (!text.empty() && text.back() == '\0') text.pop_back();
    return text;
}

void ArabicWidget::onClick(ArabicEventCallback callback) { m_onClick = callback; }
void ArabicWidget::onChange(ArabicEventCallback callback) { m_onChange = callback; }

// ══════════════════════════════════════════════════════════════
// ArabicWindow Implementation
// ══════════════════════════════════════════════════════════════

std::unordered_map<HWND, ArabicWindow*> ArabicWindow::s_windowMap;

ArabicWindow::ArabicWindow() {
    m_type = ArabicWidgetType::WIDGET_WINDOW;
}

ArabicWindow::ArabicWindow(const std::string& title, int width, int height) {
    m_type = ArabicWidgetType::WIDGET_WINDOW;
    m_title = title;
    m_width = width;
    m_height = height;
}

ArabicWindow::~ArabicWindow() {
    destroy();
}

void ArabicWindow::setTitle(const std::string& title) {
    m_title = title;
    if (m_hwnd) {
        int wLen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
        std::wstring wTitle(wLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wTitle[0], wLen);
        SetWindowTextW(m_hwnd, wTitle.c_str());
    }
}

std::string ArabicWindow::getTitle() const { return m_title; }

void ArabicWindow::registerWindow() {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ArabicGUIWindow";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);
}

void ArabicWindow::unregisterWindow() {
    UnregisterClassW(L"ArabicGUIWindow", GetModuleHandle(NULL));
}

void ArabicWindow::create() {
    registerWindow();

    int wTitleLen = MultiByteToWideChar(CP_UTF8, 0, m_title.c_str(), -1, nullptr, 0);
    std::wstring wTitle(wTitleLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, m_title.c_str(), -1, &wTitle[0], wTitleLen);

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!m_resizable) style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    m_hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"ArabicGUIWindow",
        wTitle.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        m_width, m_height,
        NULL, NULL, GetModuleHandle(NULL), this
    );

    if (m_hwnd) {
        s_windowMap[m_hwnd] = this;
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
    }
}

void ArabicWindow::destroy() {
    if (m_hwnd) {
        s_windowMap.erase(m_hwnd);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

void ArabicWindow::addWidget(std::shared_ptr<ArabicWidget> widget) {
    m_widgets.push_back(widget);
}

void ArabicWindow::run() {
    m_running = true;
    MSG msg;
    while (m_running && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void ArabicWindow::close() {
    m_running = false;
    if (m_hwnd) PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
}

void ArabicWindow::setBackgroundColor(int r, int g, int b) {
    m_bgR = r; m_bgG = g; m_bgB = b;
    if (m_hwnd) InvalidateRect(m_hwnd, NULL, TRUE);
}

void ArabicWindow::centerOnScreen() {
    if (!m_hwnd) return;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int x = (sw - m_width) / 2;
    int y = (sh - m_height) / 2;
    SetWindowPos(m_hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void ArabicWindow::setResizable(bool resizable) { m_resizable = resizable; }

void ArabicWindow::setIcon(const std::string& iconPath) {
    int wLen = MultiByteToWideChar(CP_UTF8, 0, iconPath.c_str(), -1, nullptr, 0);
    std::wstring wPath(wLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, iconPath.c_str(), -1, &wPath[0], wLen);
    HICON hIcon = (HICON)LoadImageW(NULL, wPath.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    if (hIcon && m_hwnd) {
        SendMessageW(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessageW(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
}

void ArabicWindow::maximize() { if (m_hwnd) ShowWindow(m_hwnd, SW_MAXIMIZE); }
void ArabicWindow::minimize() { if (m_hwnd) ShowWindow(m_hwnd, SW_MINIMIZE); }
void ArabicWindow::restore() { if (m_hwnd) ShowWindow(m_hwnd, SW_RESTORE); }

LRESULT CALLBACK ArabicWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ArabicWindow* win = nullptr;

    if (msg == WM_CREATE) {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        win = (ArabicWindow*)cs->lpCreateParams;
        s_windowMap[hwnd] = win;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)win);
    } else {
        win = (ArabicWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    if (!win) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
        case WM_COMMAND: {
            HWND hCtrl = (HWND)lParam;
            if (hCtrl && HIWORD(wParam) == BN_CLICKED) {
                for (auto& widget : win->m_widgets) {
                    if (widget->getHandle() == hCtrl) {
                        // Store callback and call it
                        auto cb = widget->getOnClick();
                        if (cb) cb();
                        break;
                    }
                }
            }
            break;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, TRANSPARENT);
            HBRUSH hBrush = CreateSolidBrush(RGB(win->m_bgR, win->m_bgG, win->m_bgB));
            return (INT_PTR)hBrush;
        }
        case WM_CLOSE:
            win->m_running = false;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            s_windowMap.erase(hwnd);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ══════════════════════════════════════════════════════════════
// ArabicButton Implementation
// ══════════════════════════════════════════════════════════════

ArabicButton::ArabicButton() { m_type = ArabicWidgetType::WIDGET_BUTTON; }

ArabicButton::ArabicButton(const std::string& text, int x, int y, int w, int h) {
    m_type = ArabicWidgetType::WIDGET_BUTTON;
    m_text = text; m_x = x; m_y = y; m_width = w; m_height = h;
}

void ArabicButton::create(HWND parent) {
    int wTextLen = MultiByteToWideChar(CP_UTF8, 0, m_text.c_str(), -1, nullptr, 0);
    std::wstring wText(wTextLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, m_text.c_str(), -1, &wText[0], wTextLen);

    m_hwnd = CreateWindowExW(0, L"BUTTON", wText.c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        m_x, m_y, m_width, m_height,
        parent, NULL, GetModuleHandle(NULL), NULL);

    HFONT hFont = CreateFontW(m_fontSize, 0, 0, 0, m_fontBold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE, ARABIC_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void ArabicButton::setButtonColor(int r, int g, int b) { m_btnR = r; m_btnG = g; m_btnB = b; }
void ArabicButton::setTextColor(int r, int g, int b) { m_txtR = r; m_txtG = g; m_txtB = b; }
void ArabicButton::setFontSize(int size) { m_fontSize = size; }
void ArabicButton::setFontBold(bool bold) { m_fontBold = bold; }

// ══════════════════════════════════════════════════════════════
// ArabicLabel Implementation
// ══════════════════════════════════════════════════════════════

ArabicLabel::ArabicLabel() { m_type = ArabicWidgetType::WIDGET_LABEL; }

ArabicLabel::ArabicLabel(const std::string& text, int x, int y, int w, int h) {
    m_type = ArabicWidgetType::WIDGET_LABEL;
    m_text = text; m_x = x; m_y = y; m_width = w; m_height = h;
}

void ArabicLabel::create(HWND parent) {
    DWORD style = WS_CHILD | WS_VISIBLE | SS_LEFT;
    if (m_alignment == "وسط") style = WS_CHILD | WS_VISIBLE | SS_CENTER;
    else if (m_alignment == "يمين") style = WS_CHILD | WS_VISIBLE | SS_RIGHT;

    int wTextLen = MultiByteToWideChar(CP_UTF8, 0, m_text.c_str(), -1, nullptr, 0);
    std::wstring wText(wTextLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, m_text.c_str(), -1, &wText[0], wTextLen);

    m_hwnd = CreateWindowExW(0, L"STATIC", wText.c_str(), style,
        m_x, m_y, m_width, m_height,
        parent, NULL, GetModuleHandle(NULL), NULL);

    HFONT hFont = CreateFontW(m_fontSize, 0, 0, 0, m_fontBold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE, ARABIC_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void ArabicLabel::setAlignment(const std::string& align) { m_alignment = align; }
void ArabicLabel::setFontSize(int size) { m_fontSize = size; }
void ArabicLabel::setFontColor(int r, int g, int b) { m_fontR = r; m_fontG = g; m_fontB = b; }
void ArabicLabel::setFontBold(bool bold) { m_fontBold = bold; }

// ══════════════════════════════════════════════════════════════
// ArabicTextBox Implementation
// ══════════════════════════════════════════════════════════════

ArabicTextBox::ArabicTextBox() { m_type = ArabicWidgetType::WIDGET_TEXTBOX; }

ArabicTextBox::ArabicTextBox(const std::string& placeholder, int x, int y, int w, int h) {
    m_type = ArabicWidgetType::WIDGET_TEXTBOX;
    m_text = placeholder; m_x = x; m_y = y; m_width = w; m_height = h;
}

void ArabicTextBox::create(HWND parent) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    if (m_readOnly) style |= ES_READONLY;
    if (m_password) style |= ES_PASSWORD;

    int wTextLen = MultiByteToWideChar(CP_UTF8, 0, m_text.c_str(), -1, nullptr, 0);
    std::wstring wText(wTextLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, m_text.c_str(), -1, &wText[0], wTextLen);

    m_hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wText.c_str(), style,
        m_x, m_y, m_width, m_height,
        parent, NULL, GetModuleHandle(NULL), NULL);

    HFONT hFont = CreateFontW(m_fontSize, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, ARABIC_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void ArabicTextBox::setReadOnly(bool readOnly) { m_readOnly = readOnly; }
void ArabicTextBox::setPasswordMode(bool password) { m_password = password; }
void ArabicTextBox::setMaxLength(int maxLen) { m_maxLength = maxLen; }
void ArabicTextBox::selectAll() { if (m_hwnd) SendMessageW(m_hwnd, EM_SETSEL, 0, -1); }
void ArabicTextBox::setFontSize(int size) { m_fontSize = size; }

std::string ArabicTextBox::getSelectedText() const {
    if (!m_hwnd) return "";
    DWORD start, end;
    SendMessageW(m_hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
    int len = end - start;
    if (len <= 0) return "";
    std::wstring wText(len + 1, 0);
    SendMessageW(m_hwnd, EM_GETSELTEXT, 0, (LPARAM)&wText[0]);
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string text(utf8Len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, &text[0], utf8Len, nullptr, nullptr);
    if (!text.empty() && text.back() == '\0') text.pop_back();
    return text;
}

// ══════════════════════════════════════════════════════════════
// ArabicCheckBox Implementation
// ══════════════════════════════════════════════════════════════

ArabicCheckBox::ArabicCheckBox() { m_type = ArabicWidgetType::WIDGET_CHECKBOX; }

ArabicCheckBox::ArabicCheckBox(const std::string& text, int x, int y) {
    m_type = ArabicWidgetType::WIDGET_CHECKBOX;
    m_text = text; m_x = x; m_y = y; m_width = 150; m_height = 25;
}

void ArabicCheckBox::create(HWND parent) {
    int wTextLen = MultiByteToWideChar(CP_UTF8, 0, m_text.c_str(), -1, nullptr, 0);
    std::wstring wText(wTextLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, m_text.c_str(), -1, &wText[0], wTextLen);

    m_hwnd = CreateWindowExW(0, L"BUTTON", wText.c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        m_x, m_y, m_width, m_height,
        parent, NULL, GetModuleHandle(NULL), NULL);
}

void ArabicCheckBox::setChecked(bool checked) {
    m_checked = checked;
    if (m_hwnd) SendMessageW(m_hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool ArabicCheckBox::isChecked() const {
    if (!m_hwnd) return m_checked;
    return SendMessageW(m_hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

// ══════════════════════════════════════════════════════════════
// ArabicComboBox Implementation
// ══════════════════════════════════════════════════════════════

ArabicComboBox::ArabicComboBox() { m_type = ArabicWidgetType::WIDGET_COMBOBOX; }

ArabicComboBox::ArabicComboBox(int x, int y, int w, int h) {
    m_type = ArabicWidgetType::WIDGET_COMBOBOX;
    m_x = x; m_y = y; m_width = w; m_height = h;
}

void ArabicComboBox::create(HWND parent) {
    m_hwnd = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        m_x, m_y, m_width, m_height,
        parent, NULL, GetModuleHandle(NULL), NULL);
}

void ArabicComboBox::addItem(const std::string& item) {
    m_items.push_back(item);
    if (m_hwnd) {
        int wLen = MultiByteToWideChar(CP_UTF8, 0, item.c_str(), -1, nullptr, 0);
        std::wstring wItem(wLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, item.c_str(), -1, &wItem[0], wLen);
        SendMessageW(m_hwnd, CB_ADDSTRING, 0, (LPARAM)wItem.c_str());
    }
}

void ArabicComboBox::addItems(const std::vector<std::string>& items) {
    for (const auto& item : items) addItem(item);
}

void ArabicComboBox::clearItems() {
    m_items.clear();
    if (m_hwnd) SendMessageW(m_hwnd, CB_RESETCONTENT, 0, 0);
}

int ArabicComboBox::getItemCount() const { return (int)m_items.size(); }

void ArabicComboBox::setSelectedIndex(int index) {
    m_selectedIndex = index;
    if (m_hwnd) SendMessageW(m_hwnd, CB_SETCURSEL, index, 0);
}

int ArabicComboBox::getSelectedIndex() const {
    if (!m_hwnd) return m_selectedIndex;
    return (int)SendMessageW(m_hwnd, CB_GETCURSEL, 0, 0);
}

std::string ArabicComboBox::getSelectedItem() const {
    if (!m_hwnd || m_items.empty()) return "";
    int idx = getSelectedIndex();
    if (idx < 0 || idx >= (int)m_items.size()) return "";
    return m_items[idx];
}

// ══════════════════════════════════════════════════════════════
// ArabicProgressBar Implementation
// ══════════════════════════════════════════════════════════════

ArabicProgressBar::ArabicProgressBar() { m_type = ArabicWidgetType::WIDGET_PROGRESSBAR; }

ArabicProgressBar::ArabicProgressBar(int x, int y, int w, int h) {
    m_type = ArabicWidgetType::WIDGET_PROGRESSBAR;
    m_x = x; m_y = y; m_width = w; m_height = h;
}

void ArabicProgressBar::create(HWND parent) {
    INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX), ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&icc);

    m_hwnd = CreateWindowExW(0, PROGRESS_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        m_x, m_y, m_width, m_height,
        parent, NULL, GetModuleHandle(NULL), NULL);

    SendMessageW(m_hwnd, PBM_SETRANGE, 0, MAKELPARAM(m_min, m_max));
}

void ArabicProgressBar::setRange(int min, int max) {
    m_min = min; m_max = max;
    if (m_hwnd) SendMessageW(m_hwnd, PBM_SETRANGE, 0, MAKELPARAM(min, max));
}

void ArabicProgressBar::setValue(int value) {
    m_value = value;
    if (m_hwnd) SendMessageW(m_hwnd, PBM_SETPOS, value, 0);
}

int ArabicProgressBar::getValue() const { return m_value; }

void ArabicProgressBar::step() {
    if (m_hwnd) SendMessageW(m_hwnd, PBM_STEPIT, 0, 0);
}

// ══════════════════════════════════════════════════════════════
// ArabicGUI Manager Implementation
// ══════════════════════════════════════════════════════════════

ArabicGUI::ArabicGUI() {}

ArabicGUI::~ArabicGUI() {
    closeAllWindows();
}

std::shared_ptr<ArabicWindow> ArabicGUI::createWindow(const std::string& title, int width, int height) {
    auto win = std::make_shared<ArabicWindow>(title, width, height);
    win->create();
    m_windows.push_back(win);
    m_activeWindow = win.get();
    return win;
}

std::shared_ptr<ArabicButton> ArabicGUI::createButton(const std::string& text, int x, int y, int w, int h) {
    auto btn = std::make_shared<ArabicButton>(text, x, y, w, h);
    if (m_activeWindow && m_activeWindow->getHandle()) {
        btn->create(m_activeWindow->getHandle());
        m_activeWindow->addWidget(btn);
    }
    return btn;
}

std::shared_ptr<ArabicLabel> ArabicGUI::createLabel(const std::string& text, int x, int y, int w, int h) {
    auto lbl = std::make_shared<ArabicLabel>(text, x, y, w, h);
    if (m_activeWindow && m_activeWindow->getHandle()) {
        lbl->create(m_activeWindow->getHandle());
        m_activeWindow->addWidget(lbl);
    }
    return lbl;
}

std::shared_ptr<ArabicTextBox> ArabicGUI::createTextBox(const std::string& placeholder, int x, int y, int w, int h) {
    auto tb = std::make_shared<ArabicTextBox>(placeholder, x, y, w, h);
    if (m_activeWindow && m_activeWindow->getHandle()) {
        tb->create(m_activeWindow->getHandle());
        m_activeWindow->addWidget(tb);
    }
    return tb;
}

std::shared_ptr<ArabicCheckBox> ArabicGUI::createCheckBox(const std::string& text, int x, int y) {
    auto cb = std::make_shared<ArabicCheckBox>(text, x, y);
    if (m_activeWindow && m_activeWindow->getHandle()) {
        cb->create(m_activeWindow->getHandle());
        m_activeWindow->addWidget(cb);
    }
    return cb;
}

std::shared_ptr<ArabicComboBox> ArabicGUI::createComboBox(int x, int y, int w, int h) {
    auto cb = std::make_shared<ArabicComboBox>(x, y, w, h);
    if (m_activeWindow && m_activeWindow->getHandle()) {
        cb->create(m_activeWindow->getHandle());
        m_activeWindow->addWidget(cb);
    }
    return cb;
}

std::shared_ptr<ArabicProgressBar> ArabicGUI::createProgressBar(int x, int y, int w, int h) {
    auto pb = std::make_shared<ArabicProgressBar>(x, y, w, h);
    if (m_activeWindow && m_activeWindow->getHandle()) {
        pb->create(m_activeWindow->getHandle());
        m_activeWindow->addWidget(pb);
    }
    return pb;
}

void ArabicGUI::runMainLoop() {
    if (!m_windows.empty()) {
        m_windows[0]->run();
    }
}

void ArabicGUI::showMessage(const std::string& title, const std::string& message, const std::string& type) {
    int wTitleLen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
    std::wstring wTitle(wTitleLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wTitle[0], wTitleLen);

    int wMsgLen = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, nullptr, 0);
    std::wstring wMsg(wMsgLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, &wMsg[0], wMsgLen);

    UINT flags = MB_OK;
    if (type == "تحذير") flags = MB_OK | MB_ICONWARNING;
    else if (type == "خطأ") flags = MB_OK | MB_ICONERROR;
    else if (type == "سؤال") flags = MB_OK | MB_ICONQUESTION;
    else flags = MB_OK | MB_ICONINFORMATION;

    MessageBoxW(NULL, wMsg.c_str(), wTitle.c_str(), flags);
}

std::string ArabicGUI::showInputDialog(const std::string& title, const std::string& prompt) {
    // Simple implementation using MessageBox
    showMessage(title, prompt);
    return "";
}

bool ArabicGUI::showConfirmDialog(const std::string& title, const std::string& message) {
    int wTitleLen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
    std::wstring wTitle(wTitleLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wTitle[0], wTitleLen);

    int wMsgLen = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, nullptr, 0);
    std::wstring wMsg(wMsgLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, &wMsg[0], wMsgLen);

    int result = MessageBoxW(NULL, wMsg.c_str(), wTitle.c_str(), MB_YESNO | MB_ICONQUESTION);
    return result == IDYES;
}

void ArabicGUI::refresh() {
    for (auto& win : m_windows) {
        if (win->getHandle()) InvalidateRect(win->getHandle(), NULL, TRUE);
    }
}

void ArabicGUI::closeAllWindows() {
    for (auto& win : m_windows) {
        win->close();
    }
    m_windows.clear();
}

void ArabicGUI::processEvents() {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

// ══════════════════════════════════════════════════════════════
// Register Functions for Arabic Compiler
// ══════════════════════════════════════════════════════════════

void ArabicGUI::registerFunctions(std::shared_ptr<ArabicRuntime> runtime,
                                  std::function<void(const std::string&)> outputCallback) {
    if (!runtime) return;

    auto gui = std::make_shared<ArabicGUI>();
    runtime->defineNativeFunction("رسالة", [gui, outputCallback](const std::vector<ArabicValue>& args) -> ArabicValue {
        if (args.size() >= 2) {
            std::string title = args[0].string_value;
            std::string msg = args[1].string_value;
            std::string type = args.size() >= 3 ? args[2].string_value : "معلومات";
            int wTitleLen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
            std::wstring wTitle(wTitleLen, 0);
            MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wTitle[0], wTitleLen);
            int wMsgLen = MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, nullptr, 0);
            std::wstring wMsg(wMsgLen, 0);
            MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, &wMsg[0], wMsgLen);
            UINT flags = MB_OK | MB_ICONINFORMATION;
            if (type == "تحذير") flags = MB_OK | MB_ICONWARNING;
            else if (type == "خطأ") flags = MB_OK | MB_ICONERROR;
            MessageBoxW(NULL, wMsg.c_str(), wTitle.c_str(), flags);
        }
        return ArabicValue();
    });

    runtime->defineNativeFunction("تأكيد", [gui, outputCallback](const std::vector<ArabicValue>& args) -> ArabicValue {
        if (args.size() >= 2) {
            std::string title = args[0].string_value;
            std::string msg = args[1].string_value;
            int wTitleLen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
            std::wstring wTitle(wTitleLen, 0);
            MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wTitle[0], wTitleLen);
            int wMsgLen = MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, nullptr, 0);
            std::wstring wMsg(wMsgLen, 0);
            MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, &wMsg[0], wMsgLen);
            int result = MessageBoxW(NULL, wMsg.c_str(), wTitle.c_str(), MB_YESNO | MB_ICONQUESTION);
            return ArabicValue(result == IDYES);
        }
        return ArabicValue(false);
    });
}

} // namespace ArabicLanguage
