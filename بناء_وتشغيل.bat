@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion

echo.
echo ╔══════════════════════════════════════════════════════════╗
echo ║       المترجم العربي — بناء كامل وتشغيل ذاتي           ║
echo ╚══════════════════════════════════════════════════════════╝
echo.

set PROJECT_DIR=%~dp0
cd /d "%PROJECT_DIR%"

:: ══════════════════════════════════════════════════
:: الخطوة 1: البحث عن CMake
:: ══════════════════════════════════════════════════
echo [1/4] البحث عن CMake...
cmake --version > nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [خطأ] CMake غير موجود في PATH
    echo        يرجى تثبيته من: https://cmake.org/download/
    pause
    exit /b 1
)
for /f "tokens=3" %%v in ('cmake --version ^| findstr /i "version"') do (
    echo [✓] CMake الإصدار: %%v
)

:: ══════════════════════════════════════════════════
:: الخطوة 2: تهيئة مجلد البناء
:: ══════════════════════════════════════════════════
echo.
echo [2/4] تهيئة مجلد البناء...

if not exist "build_release" mkdir "build_release"

cmake -S . -B build_release ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_CONFIGURATION_TYPES=Release ^
    2> build_release\cmake_config.log

if %ERRORLEVEL% NEQ 0 (
    echo [خطأ] فشل تهيئة CMake — راجع: build_release\cmake_config.log
    type build_release\cmake_config.log | findstr /i "error\|خطأ"
    pause
    exit /b 1
)
echo [✓] تهيئة CMake نجحت

:: ══════════════════════════════════════════════════
:: الخطوة 3: البناء
:: ══════════════════════════════════════════════════
echo.
echo [3/4] بدء البناء (قد يستغرق بضع دقائق)...

cmake --build build_release --config Release --parallel 4 ^
    2> build_release\build_errors.log

if %ERRORLEVEL% NEQ 0 (
    echo [خطأ] فشل البناء — آخر 20 سطر من الأخطاء:
    echo ──────────────────────────────────────────
    powershell -Command "Get-Content 'build_release\build_errors.log' | Select-Object -Last 20"
    echo ──────────────────────────────────────────
    pause
    exit /b 1
)
echo [✓] البناء نجح!

:: ══════════════════════════════════════════════════
:: الخطوة 4: تحديد الملف التنفيذي
:: ══════════════════════════════════════════════════
echo.
echo [4/4] تحديد المترجم...

set EXE=
if exist "build_release\Release\arabic_compiler.exe" set EXE=build_release\Release\arabic_compiler.exe
if not defined EXE if exist "build_release\Release\AlBiruniIDE.exe" set EXE=build_release\Release\AlBiruniIDE.exe
if not defined EXE if exist "build\Release\arabic_compiler.exe"         set EXE=build\Release\arabic_compiler.exe
if not defined EXE if exist "build\Release\AlBiruniIDE.exe"          set EXE=build\Release\AlBiruniIDE.exe
if not defined EXE if exist "arabic_compiler_v2.exe"                    set EXE=arabic_compiler_v2.exe
if not defined EXE if exist "arabic_compiler.exe"                       set EXE=arabic_compiler.exe

if not defined EXE (
    echo [خطأ] لم يُنتج أي ملف تنفيذي
    dir /b build_release\Release\*.exe 2>nul
    pause
    exit /b 1
)
echo [✓] المترجم: %EXE%

:: نسخ الملف التنفيذي للجذر للسهولة
copy /y "%EXE%" "arabic_compiler_latest.exe" > nul
echo [✓] تم نسخه إلى: arabic_compiler_latest.exe

:: ══════════════════════════════════════════════════
:: تشغيل البناء الذاتي
:: ══════════════════════════════════════════════════
echo.
echo ╔══════════════════════════════════════════════════════════╗
echo ║              تشغيل مراحل البناء الذاتي                  ║
echo ╚══════════════════════════════════════════════════════════╝
echo.

set FAILED=0

echo [مرحلة 1/3] المحلل اللغوي...
"%EXE%" --mode run examples\محلل_لغوي.عربي
if %ERRORLEVEL% NEQ 0 (set FAILED=1 & echo [✗] فشلت) else echo [✓] نجحت

echo.
echo [مرحلة 2/3] المحلل النحوي...
"%EXE%" --mode run examples\محلل_نحوي.عربي
if %ERRORLEVEL% NEQ 0 (set FAILED=1 & echo [✗] فشلت) else echo [✓] نجحت

echo.
echo [مرحلة 3/3] المترجم الذاتي المتكامل...
"%EXE%" --mode run src\core\SelfHostingCompiler.arabic
if %ERRORLEVEL% NEQ 0 (set FAILED=1 & echo [✗] فشلت) else echo [✓] نجحت

:: ══════════════════════════════════════════════════
:: النتيجة النهائية
:: ══════════════════════════════════════════════════
echo.
echo ══════════════════════════════════════════════════════════
if %FAILED% EQU 0 (
    echo   ✅ البناء الكامل والبناء الذاتي نجحا!
    echo   المترجم جاهز: %EXE%
) else (
    echo   ⚠️  البناء نجح لكن بعض مراحل البناء الذاتي فشلت
    echo   راجع المخرجات أعلاه للتفاصيل
)
echo ══════════════════════════════════════════════════════════
echo.
pause
