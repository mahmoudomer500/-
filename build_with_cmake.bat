@echo off
chcp 65001 > nul 2>&1
setlocal EnableDelayedExpansion

echo ════════════════════════════════════════════════════════
echo  بناء المشروع الكامل عبر CMake
echo  Full Project Build via CMake
echo ════════════════════════════════════════════════════════
echo.

:: ════════════════════════════════════════════════════════
:: الخطوة 1: التحقق من CMake
:: ════════════════════════════════════════════════════════
echo [1/3] التحقق من CMake...
cmake --version > nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ❌ CMake غير موجود في PATH
    echo    قم بتثبيته من: https://cmake.org/download/
    echo    أو استخدم build_ide.bat للبناء عبر g++
    pause
    exit /b 1
)
for /f "tokens=3" %%v in ('cmake --version ^| findstr /i "version"') do (
    echo ✅ CMake الإصدار: %%v
)
echo.

:: ════════════════════════════════════════════════════════
:: الخطوة 2: تهيئة مجلد البناء
:: ════════════════════════════════════════════════════════
echo [2/3] تهيئة مجلد البناء...
if exist "build_release" (
    echo 🗑️ حذف مجلد البناء القديم...
    rmdir /s /q "build_release" > nul 2>&1
)

cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CONFIGURATION_TYPES=Release 2> build_release\cmake_config.log
if %ERRORLEVEL% NEQ 0 (
    echo ❌ فشل تهيئة CMake
    type build_release\cmake_config.log | findstr /i "error"
    pause
    exit /b 1
)
echo ✅ تهيئة CMake نجحت
echo.

:: ════════════════════════════════════════════════════════
:: الخطوة 3: البناء
:: ════════════════════════════════════════════════════════
echo [3/3] بدء البناء (قد يستغرق بضع دقائق)...
cmake --build build_release --config Release --parallel 4 2> build_release\build_errors.log
if %ERRORLEVEL% NEQ 0 (
    echo ❌ فشل البناء
    echo ──────────────────────────────────────────
    powershell -Command "Get-Content 'build_release\build_errors.log' | Select-Object -Last 20"
    echo ──────────────────────────────────────────
    pause
    exit /b 1
)

echo.
echo ════════════════════════════════════════════════════════
echo  ✅ تم البناء بنجاح عبر CMake!
echo ════════════════════════════════════════════════════════
echo.
echo  الملفات المُنتَجة:
if exist "build_release\Release\arabic_compiler.exe" (
    for %%A in ("build_release\Release\arabic_compiler.exe") do (
        set /a SIZE=%%~zA / 1024
        echo   📦 arabic_compiler.exe  (!SIZE! KB)
    )
)
if exist "build_release\Release\AlBiruniIDE.exe" (
    for %%A in ("build_release\Release\AlBiruniIDE.exe") do (
        set /a SIZE=%%~zA / 1024
        echo   🖥️  AlBiruniIDE.exe  (!SIZE! KB)
    )
)
echo.
echo ════════════════════════════════════════════════════════
pause
