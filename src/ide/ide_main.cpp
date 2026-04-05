// ide_main.cpp - نقطة الدخول لبيئة التطوير العربية البيروني
// Arabic IDE Entry Point

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "../core/SafeWindows.h"
#include <cstdio>
#include "ArabicIDE.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    
    // إعداد UTF-8 للـ Console (للتصحيح)
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    // إنشاء بيئة التطوير
    ArabicLanguage::ArabicIDE ide;
    
    // إعداد التكوين
    ArabicLanguage::IDEConfig config;
    config.windowWidth = 1200;
    config.windowHeight = 800;
    config.windowTitle = "البيروني - بيئة التطوير العربية";
    config.darkMode = true;
    config.autoSave = true;
    
    // تهيئة البيئة
    if (!ide.initialize(hInstance, config)) {
        MessageBoxW(nullptr, L"فشل تهيئة بيئة التطوير", L"خطأ", MB_ICONERROR);
        return 1;
    }
    
    // تشغيل حلقة الرسائل
    int result = ide.run();
    
    // إغلاق البيئة
    ide.shutdown();
    
    return result;
}

// نقطة دخول بديلة للـ Console (للتصحيح)
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    return wWinMain(GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOW);
}
