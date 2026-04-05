@echo off
chcp 65001 > nul
echo.
echo ████████████████████████████████████████
echo    بدء مراحل البناء الذاتي للمترجم العربي
echo ████████████████████████████████████████
echo.

:: ── اختيار المترجم المناسب تلقائياً ──────────────────────
set EXE=
if exist "build\Release\AlBiruniIDE.exe" set EXE=build\Release\AlBiruniIDE.exe
if not defined EXE if exist "arabic_compiler_v2.exe" set EXE=arabic_compiler_v2.exe
if not defined EXE if exist "arabic_compiler.exe"    set EXE=arabic_compiler.exe
if not defined EXE if exist "arabic_v17.exe"         set EXE=arabic_v17.exe
if not defined EXE if exist "arabic_v2.exe"          set EXE=arabic_v2.exe

if not defined EXE (
    echo [خطأ] لا يوجد مترجم! يرجى بناء المشروع أولاً.
    exit /b 1
)
echo [INFO] استخدام: %EXE%
echo.

:: ── المرحلة 1: المحلل اللغوي ──────────────────────────────
echo [1/3] تشغيل المحلل اللغوي...
%EXE% --mode run examples\محلل_لغوي.عربي
if %ERRORLEVEL% NEQ 0 (
    echo [فشل] المرحلة 1: المحلل اللغوي
    exit /b 1
)
echo [نجح] المرحلة 1

:: ── المرحلة 2: المحلل النحوي ──────────────────────────────
echo.
echo [2/3] تشغيل المحلل النحوي...
%EXE% --mode run examples\محلل_نحوي.عربي
if %ERRORLEVEL% NEQ 0 (
    echo [فشل] المرحلة 2: المحلل النحوي
    exit /b 1
)
echo [نجح] المرحلة 2

:: ── المرحلة 3: المترجم الذاتي المتكامل ───────────────────
echo.
echo [3/3] تشغيل المترجم الذاتي المتكامل...
%EXE% --mode run src\core\SelfHostingCompiler.arabic
if %ERRORLEVEL% NEQ 0 (
    echo [فشل] المرحلة 3: المترجم الذاتي
    exit /b 1
)
echo [نجح] المرحلة 3

echo.
echo ✅ جميع مراحل البناء الذاتي نجحت!
