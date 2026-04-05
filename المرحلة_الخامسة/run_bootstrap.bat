@echo off
chcp 65001 >nul
echo ╔══════════════════════════════════════════════════════════╗
echo ║        تشغيل اختبار Bootstrap الذاتي على Windows        ║
echo ╚══════════════════════════════════════════════════════════╝
echo.

cd /d "%~dp0"

echo [1/4] التحقق من وجود الملفات...
echo.

if not exist "self_hosted_output\test_bootstrap.exe" (
    echo ❌ خطأ: ملف test_bootstrap.exe غير موجود!
    echo يرجى تجميع الملف أولاً باستخدام:
    echo   arabic_v17.exe --mode compiler "examples\bootstrap_ذاتي.عربي" "self_hosted_output\test_bootstrap"
    pause
    exit /b 1
)

if not exist "self_hosted_output\test_translator.exe" (
    echo ❌ خطأ: ملف test_translator.exe غير موجود!
    echo يرجى تجميع الملف أولاً باستخدام:
    echo   arabic_v17.exe --mode compiler "examples\مترجم_ذاتي.عربي" "self_hosted_output\test_translator"
    pause
    exit /b 1
)

echo ✅ تم العثور على جميع الملفات المطلوبة
echo.

echo [2/4] تشغيل اختبار Bootstrap...
echo.
echo ═══════════════════════════════════════════════════════════
echo.

self_hosted_output\test_bootstrap.exe

echo.
echo ═══════════════════════════════════════════════════════════
echo.

echo [3/4] تشغيل المترجم الذاتي...
echo.

self_hosted_output\test_translator.exe

echo.
echo [4/4] اختبار تجميع المكونات...
echo.

if exist "arabic_v17.exe" (
    echo تجميع المحلل اللغوي...
    arabic_v17.exe --mode compiler "examples\محلل_لغوي.عربي" "self_hosted_output\محلل_لغوي_اختبار"
    if exist "self_hosted_output\محلل_لغوي_اختبار.exe" (
        echo ✅ تم تجميع المحلل اللغوي بنجاح!
    ) else (
        echo ❌ فشل في تجميع المحلل اللغوي
    )
    
    echo.
    echo تجميع المحلل النحوي...
    arabic_v17.exe --mode compiler "examples\المحلل_القواعدي.عربي" "self_hosted_output\المحلل_القواعدي_اختبار"
    if exist "self_hosted_output\المحلل_القواعدي_اختبار.exe" (
        echo ✅ تم تجميع المحلل النحوي بنجاح!
    ) else (
        echo ❌ فشل في تجميع المحلل النحوي
    )
    
    echo.
    echo تجميع مولد الكود...
    arabic_v17.exe --mode compiler "examples\مولد_كود.عربي" "self_hosted_output\مولد_كود_اختبار"
    if exist "self_hosted_output\مولد_كود_اختبار.exe" (
        echo ✅ تم تجميع مولد الكود بنجاح!
    ) else (
        echo ❌ فشل في تجميع مولد الكود
    )
) else (
    echo ⚠️  لم يتم العثور على arabic_v17.exe
    echo تأكد من وجود الملف في المجلد الحالي
)

echo.
echo ═══════════════════════════════════════════════════════════
echo.
echo ✅ اكتمل اختبار Bootstrap!
echo.
echo الملفات الناتجة:
dir /b self_hosted_output\*.exe 2>nul
echo.

pause
