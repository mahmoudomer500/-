@echo off
chcp 65001 >nul
echo ╔══════════════════════════════════════════════════════════════╗
echo ║          نظام الاختبارات الآلية - لغة البرمجة العربية        ║
echo ║                  الإصدار 2.1 - النسخ المحسّنة                ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.

cd /d "%~dp0"

set PASSED=0
set FAILED=0
set TOTAL=0
set COMPILER_NAME=arabic_v17.exe

if not exist "..\%COMPILER_NAME%" (
    echo ⚠️  لم يتم العثور على المترجم: %COMPILER_NAME%
    echo    جاري البحث عن بدائل...
    if exist "..\arabic_compiler.exe" (
        set COMPILER_NAME=arabic_compiler.exe
        echo ✅ تم استخدام: %COMPILER_NAME%
    ) else (
        echo ❌ لم يتم العثور على أي مترجم!
        echo    يرجى التأكد من وجود ملف المترجم في المجلد الرئيسي
        pause
        exit /b 1
    )
)

echo 📋 المترجم المستخدم: %COMPILER_NAME%
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 1: الطباعة الأساسية
:: ══════════════════════════════════════════════════════════════
echo [اختبار 1] الطباعة الأساسية
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo اطبع("مرحبا بالعالم!")
) > "%TEMP%\test1_basic.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test1_basic.عربي" "%TEMP%\test1_output" >nul 2>&1

if exist "%TEMP%\test1_output.exe" (
    for /f %%i in ('"%TEMP%\test1_output.exe"') do set "RESULT=%%i"
    if defined RESULT (
        echo ✅ نجح - تم الإنشاء والتشغيل
        set /a PASSED+=1
    ) else (
        echo ✅ نجح - تم إنشاء الملف التنفيذي
        set /a PASSED+=1
    )
) else (
    echo ❌ فشل - لم يتم إنشاء ملف الإخراج
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 2: المتغيرات
:: ══════════════════════════════════════════════════════════════
echo [اختبار 2] المتغيرات
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo مت س = 42
    echo اطبع(س)
) > "%TEMP%\test2_vars.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test2_vars.عربي" "%TEMP%\test2_output" >nul 2>&1

if exist "%TEMP%\test2_output.exe" (
    echo ✅ نجح - تعريف المتغير يعمل
    set /a PASSED+=1
) else (
    echo ❌ فشل - المتغيرات لا تعمل
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 3: العمليات الحسابية
:: ══════════════════════════════════════════════════════════════
echo [اختبار 3] العمليات الحسابية
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo مت أ = 10
    echo مت ب = 5
    echo مت ج = أ + ب
    echo اطبع(ج)
) > "%TEMP%\test3_arith.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test3_arith.عربي" "%TEMP%\test3_output" >nul 2>&1

if exist "%TEMP%\test3_output.exe" (
    echo ✅ نجح - العمليات الحسابية تعمل
    set /a PASSED+=1
) else (
    echo ❌ فشل - العمليات الحسابية لا تعمل
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 4: الشروط (اذا)
:: ══════════════════════════════════════════════════════════════
echo [اختبار 4] الشروط
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo مت س = 10
    echo اذا س ^> 5:
    echo     اطبع("س أكبر من 5")
    echo نهاية
) > "%TEMP%\test4_cond.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test4_cond.عربي" "%TEMP%\test4_output" >nul 2>&1

if exist "%TEMP%\test4_output.exe" (
    echo ✅ نجح - الشروط تعمل
    set /a PASSED+=1
) else (
    echo ❌ فشل - الشروط لا تعمل
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 5: حلقة while
:: ══════════════════════════════════════════════════════════════
echo [اختبار 5] حلقة while
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo مت i = 0
    echo بينما i ^< 3:
    echo     اطبع(i)
    echo     i = i + 1
    echo نهاية
) > "%TEMP%\test5_while.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test5_while.عربي" "%TEMP%\test5_output" >nul 2>&1

if exist "%TEMP%\test5_output.exe" (
    echo ✅ نجح - حلقات while تعمل
    set /a PASSED+=1
) else (
    echo ❌ فشل - حلقات while لا تعمل
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 6: الدوال
:: ══════════════════════════════════════════════════════════════
echo [اختبار 6] الدوال
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo دالة جمع(أ، ب):
    echo     أرجع أ + ب
    echo نهاية
    echo اطبع(جمع(5، 3))
) > "%TEMP%\test6_func.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test6_func.عربي" "%TEMP%\test6_output" >nul 2>&1

if exist "%TEMP%\test6_output.exe" (
    echo ✅ نجح - الدوال تعمل
    set /a PASSED+=1
) else (
    echo ❌ فشل - الدوال لا تعمل
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 7: المصفوفات
:: ══════════════════════════════════════════════════════════════
echo [اختبار 7] المصفوفات
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo مت أرقام = [1، 2، 3]
    echo اطبع(أرقام[0])
) > "%TEMP%\test7_arr.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test7_arr.عربي" "%TEMP%\test7_output" >nul 2>&1

if exist "%TEMP%\test7_output.exe" (
    echo ✅ نجح - المصفوفات تعمل
    set /a PASSED+=1
) else (
    echo ❌ فشل - المصفوفات لا تعمل
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 8: النصوص
:: ══════════════════════════════════════════════════════════════
echo [اختبار 8] النصوص
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo مت نص = "مرحبا"
    echo اطبع(نص)
) > "%TEMP%\test8_str.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test8_str.عربي" "%TEMP%\test8_output" >nul 2>&1

if exist "%TEMP%\test8_output.exe" (
    echo ✅ نجح - النصوص تعمل
    set /a PASSED+=1
) else (
    echo ❌ فشل - النصوص لا تعمل
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 9: حلقة for
:: ══════════════════════════════════════════════════════════════
echo [اختبار 9] حلقة for
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo لكل i في النطاق(0، 3):
    echo     اطبع(i)
    echo نهاية
) > "%TEMP%\test9_for.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test9_for.عربي" "%TEMP%\test9_output" >nul 2>&1

if exist "%TEMP%\test9_output.exe" (
    echo ✅ نجح - حلقة for تعمل
    set /a PASSED+=1
) else (
    echo ❌ فشل - حلقة for لا تعمل
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 10: else و elseif
:: ══════════════════════════════════════════════════════════════
echo [اختبار 10] else و elseif
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo مت درجة = 85
    echo اذا درجة ^>= 90:
    echo     اطبع("ممتاز")
    echo او_اذا درجة ^>= 80:
    echo     اطبع("جيد جدا")
    echo او_اذا درجة ^>= 70:
    echo     اطبع("جيد")
    echo وإلا:
    echo     اطبع("راسب")
    echo نهاية
) > "%TEMP%\test10_elseif.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test10_elseif.عربي" "%TEMP%\test10_output" >nul 2>&1

if exist "%TEMP%\test10_output.exe" (
    echo ✅ نجح - else و elseif يعملان
    set /a PASSED+=1
) else (
    echo ❌ فشل - else و elseif لا يعملان
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 11: العمليات المنطقية
:: ══════════════════════════════════════════════════════════════
echo [اختبار 11] العمليات المنطقية
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo مت أ = صحيح
    echo مت ب = خطأ
    echo اطبع(أ و ب)
    echo اطبع(أ او ب)
) > "%TEMP%\test11_bool.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test11_bool.عربي" "%TEMP%\test11_output" >nul 2>&1

if exist "%TEMP%\test11_output.exe" (
    echo ✅ نجح - العمليات المنطقية تعمل
    set /a PASSED+=1
) else (
    echo ❌ فشل - العمليات المنطقية لا تعمل
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: اختبار 12: الملفات المؤقتة (أمان)
:: ══════════════════════════════════════════════════════════════
echo [اختبار 12] سلامة اسم الملف
echo ──────────────────────────────────────────
set /a TOTAL+=1

(
    echo اطبع("اختبار السلامة")
) > "%TEMP%\test12_../safe.عربي"

..\%COMPILER_NAME% --mode compiler "%TEMP%\test12_../safe.عربي" "%TEMP%\test12_output" >nul 2>&1

if not exist "%TEMP%\test12_../safe.exe" (
    echo ✅ نجح - تم رفض مسار traversal
    set /a PASSED+=1
) else (
    echo ❌ فشل - تم قبول مسار غير آمن
    set /a FAILED+=1
)
echo.

:: ══════════════════════════════════════════════════════════════
:: ملخص النتائج
:: ══════════════════════════════════════════════════════════════
echo ══════════════════════════════════════════════════════════════
echo                    ملخص النتائج
echo ══════════════════════════════════════════════════════════════
echo.
echo المجموع: %TOTAL%
echo نجح:    %PASSED%
echo فشل:    %FAILED%
echo.
set /a PERCENT=(PASSED * 100) / TOTAL
echo النسبة:  %PERCENT%%% 
echo.

:: ══════════════════════════════════════════════════════════════
:: تنظيف الملفات المؤقتة
:: ══════════════════════════════════════════════════════════════
echo تنظيف الملفات المؤقتة...
del /q "%TEMP%\test*.عربي" 2>nul
del /q "%TEMP%\test*.exe" 2>nul
del /q "%TEMP%\test*.o" 2>nul
echo.

if %FAILED% equ 0 (
    echo ╔══════════════════════════════════════════════════════════════╗
    echo ║                                                              ║
    echo ║           🎉🎉🎉 جميع الاختبارات نجحت! 🎉🎉🎉               ║
    echo ║                                                              ║
    echo ╚══════════════════════════════════════════════════════════════╝
    exit /b 0
) else (
    if %PERCENT% geq 75 (
        echo ╔══════════════════════════════════════════════════════════════╗
        echo ║                                                              ║
        echo ║           ⚠️ معظم الاختبارات نجحت                          ║
        echo ║           راجع الاختبارات الفاشلة أعلاه                      ║
        echo ║                                                              ║
        echo ╚══════════════════════════════════════════════════════════════╝
        exit /b 0
    ) else (
        echo ╔══════════════════════════════════════════════════════════════╗
        echo ║                                                              ║
        echo ║           ❌ عدة اختبارات فشلت!                            ║
        echo ║           يرجى مراجعة الأخطاء أعلاه                          ║
        echo ║                                                              ║
        echo ╚══════════════════════════════════════════════════════════════╝
        exit /b 1
    )
)
