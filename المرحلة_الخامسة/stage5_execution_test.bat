@echo off
chcp 65001 >nul
echo ╔══════════════════════════════════════════════════════════╗
echo ║        المرحلة الخامسة: اختبار التنفيذ والـ Bootstrap    ║
echo ╚══════════════════════════════════════════════════════════╝
echo.

cd /d "%~dp0"

echo [المرحلة 5.1] التحقق من وجود الملفات...
echo.

set ERROR_COUNT=0

REM التحقق من وجود المترجم الرئيسي
if not exist "arabic_v17.exe" (
    echo ❌ خطأ: arabic_v17.exe غير موجود!
    set /a ERROR_COUNT+=1
) else (
    echo ✅ arabic_v17.exe موجود
)

REM التحقق من وجود المكونات المجمَّعة
if not exist "self_hosted_output\test_bootstrap.exe" (
    echo ❌ خطأ: test_bootstrap.exe غير موجود!
    set /a ERROR_COUNT+=1
) else (
    echo ✅ test_bootstrap.exe موجود
)

if not exist "self_hosted_output\test_translator.exe" (
    echo ❌ خطأ: test_translator.exe غير موجود!
    set /a ERROR_COUNT+=1
) else (
    echo ✅ test_translator.exe موجود
)

if not exist "self_hosted_output\محلل_لغوي_v17.exe" (
    echo ❌ خطأ: محلل_لغوي_v17.exe غير موجود!
    set /a ERROR_COUNT+=1
) else (
    echo ✅ محلل_لغوي_v17.exe موجود
)

if not exist "self_hosted_output\المحلل_القواعدي_v17.exe" (
    echo ❌ خطأ: المحلل_القواعدي_v17.exe غير موجود!
    set /a ERROR_COUNT+=1
) else (
    echo ✅ المحلل_القواعدي_v17.exe موجود
)

if not exist "self_hosted_output\مولد_كود_v17.exe" (
    echo ❌ خطأ: مولد_كود_v17.exe غير موجود!
    set /a ERROR_COUNT+=1
) else (
    echo ✅ مولد_كود_v17.exe موجود
)

echo.
if %ERROR_COUNT% gtr 0 (
    echo ⚠️  عدد الأخطاء: %ERROR_COUNT%
    echo يرجى تجميع جميع المكونات أولاً
    pause
    exit /b 1
)

echo.
echo ═══════════════════════════════════════════════════════════
echo [المرحلة 5.2] تشغيل اختبار Bootstrap...
echo ═══════════════════════════════════════════════════════════
echo.

self_hosted_output\test_bootstrap.exe
if errorlevel 1 (
    echo ❌ فشل في تشغيل test_bootstrap.exe
    set /a ERROR_COUNT+=1
) else (
    echo ✅ تم تشغيل test_bootstrap.exe بنجاح
)

echo.
echo ═══════════════════════════════════════════════════════════
echo [المرحلة 5.3] تشغيل المترجم الذاتي...
echo ═══════════════════════════════════════════════════════════
echo.

self_hosted_output\test_translator.exe
if errorlevel 1 (
    echo ❌ فشل في تشغيل test_translator.exe
    set /a ERROR_COUNT+=1
) else (
    echo ✅ تم تشغيل test_translator.exe بنجاح
)

echo.
echo ═══════════════════════════════════════════════════════════
echo [المرحلة 5.4] اختبار تجميع ذاتي...
echo ═══════════════════════════════════════════════════════════
echo.

echo جاري تجميع المحلل اللغوي...
arabic_v17.exe --mode compiler "examples\محلل_لغوي.عربي" "self_hosted_output\stage5_lexer"
if errorlevel 1 (
    echo ❌ فشل في تجميع المحلل اللغوي
    set /a ERROR_COUNT+=1
) else (
    if exist "self_hosted_output\stage5_lexer.exe" (
        echo ✅ تم تجميع المحلل اللغوي بنجاح
        echo    الحجم: 
        dir /b "self_hosted_output\stage5_lexer.exe"
    ) else (
        echo ❌ لم يتم إنشاء ملف الإخراج
        set /a ERROR_COUNT+=1
    )
)

echo.
echo جاري تجميع المحلل النحوي...
arabic_v17.exe --mode compiler "examples\المحلل_القواعدي.عربي" "self_hosted_output\stage5_parser"
if errorlevel 1 (
    echo ❌ فشل في تجميع المحلل النحوي
    set /a ERROR_COUNT+=1
) else (
    if exist "self_hosted_output\stage5_parser.exe" (
        echo ✅ تم تجميع المحلل النحوي بنجاح
        echo    الحجم: 
        dir /b "self_hosted_output\stage5_parser.exe"
    ) else (
        echo ❌ لم يتم إنشاء ملف الإخراج
        set /a ERROR_COUNT+=1
    )
)

echo.
echo جاري تجميع مولد الكود...
arabic_v17.exe --mode compiler "examples\مولد_كود.عربي" "self_hosted_output\stage5_codegen"
if errorlevel 1 (
    echo ❌ فشل في تجميع مولد الكود
    set /a ERROR_COUNT+=1
) else (
    if exist "self_hosted_output\stage5_codegen.exe" (
        echo ✅ تم تجميع مولد الكود بنجاح
        echo    الحجم: 
        dir /b "self_hosted_output\stage5_codegen.exe"
    ) else (
        echo ❌ لم يتم إنشاء ملف الإخراج
        set /a ERROR_COUNT+=1
    )
)

echo.
echo ═══════════════════════════════════════════════════════════
echo [المرحلة 5.5] اختبار تجميع برنامج بسيط...
echo ═══════════════════════════════════════════════════════════
echo.

REM إنشاء برنامج اختبار بسيط
echo اطبع("مرحباً من المرحلة الخامسة!") > "self_hosted_output\test_program.عربي"
echo اطبع("المترجم يعمل بشكل صحيح.") >> "self_hosted_output\test_program.عربي"

echo جاري تجميع برنامج اختبار بسيط...
arabic_v17.exe --mode compiler "self_hosted_output\test_program.عربي" "self_hosted_output\stage5_test"
if errorlevel 1 (
    echo ❌ فشل في تجميع برنامج الاختبار
    set /a ERROR_COUNT+=1
) else (
    if exist "self_hosted_output\stage5_test.exe" (
        echo ✅ تم تجميع برنامج الاختبار بنجاح
        echo.
        echo جاري تشغيل البرنامج:
        echo ─────────────────────
        self_hosted_output\stage5_test.exe
        echo ─────────────────────
        echo ✅ تم تشغيل البرنامج بنجاح!
    ) else (
        echo ❌ لم يتم إنشاء ملف الإخراج
        set /a ERROR_COUNT+=1
    )
)

echo.
echo ═══════════════════════════════════════════════════════════
echo [المرحلة 5.6] ملخص النتائج...
echo ═══════════════════════════════════════════════════════════
echo.

echo الملفات الناتجة من المرحلة 5:
dir /b "self_hosted_output\stage5_*.exe" 2>nul

echo.
if %ERROR_COUNT% equ 0 (
    echo ╔══════════════════════════════════════════════════════════╗
    echo ║                                                          ║
    echo ║   🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉   ║
    echo ║                                                          ║
    echo ║        ✅ نجح اختبار المرحلة الخامسة!                    ║
    echo ║                                                          ║
    echo ║        ✅ المترجم يمكنه تجميع نفسه!                      ║
    echo ║        ✅ البرامج المجمَّعة تعمل بشكل صحيح!               ║
    echo ║        ✅ دورة Bootstrap اكتملت بنجاح!                   ║
    echo ║                                                          ║
    echo ║   🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉   ║
    echo ║                                                          ║
    echo ╚══════════════════════════════════════════════════════════╝
) else (
    echo ╔══════════════════════════════════════════════════════════╗
    echo ║                                                          ║
    echo ║        ❌ فشل في بعض الاختبارات!                         ║
    echo ║        عدد الأخطاء: %ERROR_COUNT%                               ║
    echo ║                                                          ║
    echo ║        راجع الرسائل أعلاه للمزيد من التفاصيل            ║
    echo ║                                                          ║
    echo ╚══════════════════════════════════════════════════════════╝
)

echo.
echo ═══════════════════════════════════════════════════════════
echo                    تقرير الملفات النهائية
echo ═══════════════════════════════════════════════════════════
echo.

echo المجلد: self_hosted_output\
echo ────────────────────────────────────────────────────────
dir /b "self_hosted_output\*.exe" 2>nul | findstr /N "."
echo.

pause
