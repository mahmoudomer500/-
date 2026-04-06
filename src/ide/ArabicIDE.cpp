// ArabicIDE.cpp - تطبيق بيئة التطوير المتكاملة العربية
// Arabic Integrated Development Environment Implementation

#include "ArabicIDE.h"
#include <fstream>
#include <sstream>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <richedit.h>
#include <algorithm>
#include <thread>
#include <atomic>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

#include "ArabicIDE.h"
#include "../modules/ui/ArabicCodeEditor.h" // للتلوين والتحليل
#include "../core/PackageManager.h"
#include "../core/ArabicJITCompiler.h"
#include "../utils/debug/InteractiveDebugger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <string>

namespace ArabicLanguage {

// أمثلة الكود الجاهزة
static const std::vector<std::pair<std::string, std::string>> CODE_EXAMPLES = {
    {"مرحبا بالعالم", 
R"(// مثال: مرحبا بالعالم
اطبع("مرحباً بالعالم!")
)"},
    {"المتغيرات والعمليات",
R"(// مثال: المتغيرات والعمليات الحسابية
س = 10
ص = 20
المجموع = س + ص
اطبع("المجموع:", المجموع)

الفرق = س - ص
اطبع("الفرق:", الفرق)
)"},
    {"الشروط",
R"(// مثال: العبارات الشرطية
العمر = 18

إذا العمر >= 18:
    اطبع("أنت بالغ")
وإلا:
    اطبع("أنت قاصر")
نهاية
)"},
    {"الحلقات",
R"(// مثال: الحلقات التكرارية
اطبع("العد من 1 إلى 5:")

لكل ع من 1 إلى 5:
    اطبع(ع)
نهاية

// حلقة طالما
ن = 0
طالما ن < 3:
    اطبع("العدد:", ن)
    ن = ن + 1
نهاية
)"},
    {"الدوال",
R"(// مثال: تعريف واستدعاء الدوال
دالة جمع(أ، ب):
    أرجع أ + ب
نهاية

دالة تحية(الاسم):
    اطبع("مرحباً يا", الاسم)
نهاية

النتيجة = جمع(5, 3)
اطبع("نتيجة الجمع:", النتيجة)

تحية("أحمد")
)"},
    {"المصفوفات",
R"(// مثال: المصفوفات
الأرقام = [1, 2, 3, 4, 5]

اطبع("العنصر الأول:", الأرقام[0])
اطبع("عدد العناصر:", طول(الأرقام))

// إضافة عنصر
أضف(الأرقام, 6)

// طباعة جميع العناصر
لكل رقم في الأرقام:
    اطبع(رقم)
نهاية
)"},
    {"الكائنات والفئات",
R"(// مثال: البرمجة الكائنية
صنف شخص:
    خاصية الاسم
    خاصية العمر
    
    دالة تعريف():
        اطبع("أنا", هذا.الاسم, "عمري", هذا.العمر)
    نهاية
نهاية

أحمد = جديد شخص()
أحمد.الاسم = "أحمد"
أحمد.العمر = 25
أحمد.تعريف()
)"},
    {"قراءة الملفات",
R"(// مثال: قراءة ملف نصي
// سنقوم أولاً بإنشاء الملف للتأكد من وجوده
اكتب_ملف("بيانات.txt", "أهلاً بك في لغة البرمجة العربية!")

متغير المحتوى = اقرأ_ملف("بيانات.txt")
اطبع("محتوى الملف:", المحتوى)
)"},
    {"الرياضيات",
R"(// مثال: الدوال الرياضية
س = 16
الجذر = جذر(س)
اطبع("جذر", س, "=", الجذر)

القوة = قوة(2, 10)
اطبع("2 أس 10 =", القوة)

القيمة_المطلقة = مطلق(-5)
اطبع("القيمة المطلقة لـ -5 =", القيمة_المطلقة)
)"},
    {"رؤية حاسوبية: التقاط صورة", 
        R"(// مثال بسيط: التقاط صورة وحفظها
        استورد "الرؤية_الحاسوبية"
        
        إذا افتح_كاميرا(0):
            اطبع("✅ تم فتح الكاميرا")
            صورة = التقط_صورة()
            إذا صورة != لاشيء:
                اطبع("✅ تم التقاط الصورة")
                إذا احفظ_صورة(صورة, "لقطة.png"):
                    اطبع("✅ تم الحفظ في لقطة.png")
                وإلا:
                    اطبع("❌ فشل حفظ الصورة")
                نهاية
            نهاية
            أغلق_كاميرا()
        نهاية
        )"},
    {"رؤية حاسوبية: رسم دائرة",
        R"(// مثال: إنشاء صورة ورسم دائرة عليها
        استورد "الرؤية_الحاسوبية"
        
        // إنشاء صورة سوداء بحجم 500x500
        صورة = انشئ_صورة(500, 500)
        
        // رسم دائرة زرقاء في المنتصف
        صورة = ارسم_دائرة(صورة, 250, 250, 100, [0, 0, 255], 5)
        
        // رسم دائرة حمراء ممتلئة
        صورة = ارسم_دائرة(صورة, 100, 100, 50, [255, 0, 0], -1)
        
        // عرض النتيجة
        انشئ_نافذة("رسم دائرة")
        عرض_صورة("رسم دائرة", صورة)
        انتظر_مفتاح(0)
        أغلق_كل_النوافذ()
        )"},

    {"رؤية حاسوبية: رسم بالماوس",
        R"(// مثال: الرسم بالماوس على نافذة
        استورد "الرؤية_الحاسوبية"
        
        // إنشاء صورة بيضاء بحجم 800x600 للرسم عليها
        لوحة = انشئ_صورة(800, 600)
        لوحة = ارسم_مستطيل(لوحة, 0, 0, 800, 600, [255, 255, 255], -1)
        
        دالة عند_النقر(حدث, س, ص, أعلام, بيانات):
            // إذا تم الضغط على الزر الأيسر
            إذا حدث == حدث_ماوس_يسار_سفل:
                اطبع("رسم دائرة في: " + س + "، " + ص)
                // رسم دائرة خضراء صغيرة على اللوحة
                لوحة = ارسم_دائرة(لوحة, س, ص, 10, [0, 200, 0], -1)
                عرض_صورة("لوحة الرسم", لوحة)
            نهاية
        نهاية
        
        انشئ_نافذة("لوحة الرسم")
        عرض_صورة("لوحة الرسم", لوحة)
        حدد_حدث_الماوس("لوحة الرسم", "عند_النقر")
        
        اطبع("استخدم الماوس للرسم (الزر الأيسر). اضغط ESC أو X للخروج.")
        
        طالما 1:
            ك = انتظر_مفتاح(10)
            إذا ك == 27: // مفتاح ESC
                توقف
            نهاية
            إذا ك == -1: // تم إغلاق النافذة بـ X
                توقف
            نهاية
        نهاية
        
        أغلق_كل_النوافذ()
        )"},

    // ══════════════════════════════════════════════════════════════
    // 🧪 الاختبارات الشاملة
    // ══════════════════════════════════════════════════════════════

    {"🧪 اختبار الرياضيات",
R"TEST(// اختبار شامل لمكتبة الرياضيات
اطبع("═══════════════════════════════════════════")
اطبع("  اختبار مكتبة الرياضيات")
اطبع("═══════════════════════════════════════════")

// العمليات الأساسية
ج = جذر(144)
اطبع("✓ جذر(144) =", ج)

ق = قوة(2, 10)
اطبع("✓ قوة(2, 10) =", ق)

م = مطلق(-42)
اطبع("✓ مطلق(-42) =", م)

// الإحصائيات
متغير بيانات = [10, 20, 30, 40, 50]

مجموع = 0
لكل عنصر في بيانات:
    مجموع = مجموع + عنصر
نهاية
اطبع("✓ مجموع =", مجموع)
اطبع("✓ متوسط =", مجموع / طول(بيانات))

// المعادلات
مميز = قوة(-5, 2) - 4 * 1 * 6
جذر_مميز = جذر(مميز)
س1 = (5 + جذر_مميز) / 2
س2 = (5 - جذر_مميز) / 2
اطبع("✓ جذور x²-5x+6:", س1, "و", س2)

اطبع("✅ اكتمل اختبار الرياضيات")
)TEST"},

    {"🧪 اختبار النصوص",
R"TEST(// اختبار شامل لمكتبة النصوص
اطبع("═══════════════════════════════════════════")
اطبع("  اختبار مكتبة النصوص")
اطبع("═══════════════════════════════════════════")

متغير جملة = "مرحبا بالعالم"
اطبع("✓ الجملة:", جملة)
اطبع("✓ الطول:", طول(جملة))

مستبدل = نص_استبدل(جملة, "عالم", "برمجة")
اطبع("✓ بعد الاستبدال:", مستبدل)

فرعي = نص_فرعي(جملة, 0, 6)
اطبع("✓ النص الفرعي:", فرعي)

رقم_محول = نص_إلى_رقم("42")
اطبع("✓ نص_إلى_رقم =", رقم_محول)

نص_رقم = رقم_إلى_نص(123)
اطبع("✓ رقم_إلى_نص =", نص_رقم)

اطبع("✓ هل_رقم =", هل_رقم("123"))
اطبع("✓ نوع(42) =", نوع(42))
اطبع("✓ نوع نص =", نوع("نص"))

اطبع("✅ اكتمل اختبار النصوص")
)TEST"},

    {"🧪 اختبار الملفات",
R"TEST(// اختبار شامل لمكتبة الملفات
اطبع("═══════════════════════════════════════════")
اطبع("  اختبار مكتبة الملفات")
اطبع("═══════════════════════════════════════════")

// كتابة وقراءة
اكتب_ملف("اختبار.txt", "محتوى تجريبي")
محتوى = اقرأ_ملف("اختبار.txt")
اطبع("✓ كتابة وقراءة:", محتوى)

// التحقق من الوجود
اطبع("✓ هل موجود:", هل_ملف_موجود("اختبار.txt"))
اطبع("✓ غير موجود:", هل_ملف_موجود("غير_موجود.txt"))

// فتح وقراءة سطر
متغير مقبض = افتح_ملف("اختبار.txt", "r")
إذا مقبض != "0":
    سطر = اقرأ_سطر(مقبض)
    اطبع("✓ قراءة سطر:", سطر)
    أغلق_ملف(مقبض)
وإلا:
    اطبع("✗ فشل فتح الملف")

اطبع("✅ اكتمل اختبار الملفات")
)TEST"},

    {"🧪 اختبار المصفوفات",
R"TEST(// اختبار شامل لمكتبة المصفوفات
اطبع("═══════════════════════════════════════════")
اطبع("  اختبار مكتبة المصفوفات")
اطبع("═══════════════════════════════════════════")

// إنشاء
متغير أرقام = [5, 3, 8, 1, 9]
اطبع("✓ المصفوفة:", أرقام)
اطبع("✓ الطول:", طول(أرقام))

// وصول وتعديل
اطبع("✓ الأول:", أرقام[0])
اطبع("✓ الأخير:", أرقام[4])
أرقام[2] = 100
اطبع("✓ بعد التعديل:", أرقام)

// فرز Bubble Sort
متغير حجم = طول(أرقام)
متغير خارجي = 0
طالما خارجي < حجم:
    متغير داخلي = 0
    طالما داخلي < حجم - 1:
        إذا أرقام[داخلي] > أرقام[داخلي + 1]:
            متغير مؤقت = أرقام[داخلي]
            أرقام[داخلي] = أرقام[داخلي + 1]
            أرقام[داخلي + 1] = مؤقت
        نهاية
        داخلي = داخلي + 1
    نهاية
    خارجي = خارجي + 1
نهاية
اطبع("✓ بعد الفرز:", أرقام)

// مصفوفة نصوص
متغير أسماء = ["أحمد", "محمد", "فاطمة"]
اطبع("✓ عدد الأسماء:", طول(أسماء))
اطبع("✓ الاسم الأول:", أسماء[0])

اطبع("✅ اكتمل اختبار المصفوفات")
)TEST"},

    {"🧪 اختبار النظام",
R"TEST(// اختبار شامل لمكتبة النظام
اطبع("═══════════════════════════════════════════")
اطبع("  اختبار مكتبة النظام")
اطبع("═══════════════════════════════════════════")

// الانتظار
اطبع("✓ قبل الانتظار...")
انتظر(50)
اطبع("✓ بعد الانتظار 50ms")

// الثوابت
اطبع("✓ صحيح =", صحيح)
اطبع("✓ خطأ =", خطأ)

// الحسابيات
اطبع("✓ 7 + 3 =", 7 + 3)
اطبع("✓ 7 * 3 =", 7 * 3)
اطبع("✓ 7 - 3 =", 7 - 3)

// الحلقات
لكل i من 1 إلى 3:
    اطبع("  تكرار:", i)
نهاية

// الشروط
درجة = 85
إذا درجة > 80:
    اطبع("✓ جيد جداً")
نهاية

اطبع("✅ اكتمل اختبار النظام")
)TEST"},

    {"🧪 اختبار الألوان",
R"TEST(// اختبار مكتبة الألوان
اطبع("═══════════════════════════════════════════")
اطبع("  اختبار مكتبة الألوان")
اطبع("═══════════════════════════════════════════")

// تغيير لون النص
لون_النص(14)
اطبع("✓ هذا نص أصفر")
لون_النص(12)
اطبع("✓ هذا نص أحمر")
لون_النص(10)
اطبع("✓ هذا نص أخضر")
لون_النص(11)
اطبع("✓ هذا نص سماوي")
لون_النص(13)
اطبع("✓ هذا نص أرجواني")
لون_النص(15)
اطبع("✓ هذا نص أبيض")
إعادة_لون()
اطبع("✓ العودة للون الافتراضي")

// تغيير لون الخلفية
لون_الخلفية(1)
اطبع("✓ خلفية زرقاء")
لون_الخلفية(2)
اطبع("✓ خلفية خضراء")
لون_الخلفية(4)
اطبع("✓ خلفية حمراء")
إعادة_لون()
اطبع("✓ العودة للخلفية الافتراضية")

// لون عشوائي
لكل i من 0 إلى 4:
    لون_النص(لون_عشوائي())
    اطبع("  لون عشوائي", i)
نهاية
إعادة_لون()

اطبع("✅ اكتمل اختبار الألوان")
)TEST"},

    {"🧪 اختبار الحاويات",
R"TEST(// اختبار مكتبة الحاويات (القاموس)
اطبع("═══════════════════════════════════════════")
اطبع("  اختبار مكتبة الحاويات")
اطبع("═══════════════════════════════════════════")

// إنشاء قاموس
متغير طلاب = قاموس_جديد()

// إضافة عناصر
قاموس_ضع(طلاب, "أحمد", 95)
قاموس_ضع(طلاب, "فاطمة", 88)
قاموس_ضع(طلاب, "محمد", 76)

// التحقق من الحجم
اطبع("✓ عدد الطلاب:", قاموس_حجم(طلاب))

// جلب عناصر
متغير درجة_أحمد = قاموس_اجلب(طلاب, "أحمد")
متغير درجة_فاطمة = قاموس_اجلب(طلاب, "فاطمة")
متغير درجة_محمد = قاموس_اجلب(طلاب, "محمد")
اطبع("✓ درجة أحمد:", درجة_أحمد)
اطبع("✓ درجة فاطمة:", درجة_فاطمة)
اطبع("✓ درجة محمد:", درجة_محمد)

// التحقق من الوجود
اطبع("✓ هل أحمد موجود:", قاموس_يحتوي(طلاب, "أحمد"))
اطبع("✓ هل علي موجود:", قاموس_يحتوي(طلاب, "علي"))

// حذف عنصر
قاموس_احذف(طلاب, "محمد")
اطبع("✓ بعد حذف محمد، العدد:", قاموس_حجم(طلاب))
اطبع("✓ هل محمد موجود:", قاموس_يحتوي(طلاب, "محمد"))

اطبع("✅ اكتمل اختبار الحاويات")
)TEST"},

    {"🧪 اختبار شامل للمكتبات",
R"EXAMPLE(// اختبار شامل لجميع المكتبات
اطبع("╔═══════════════════════════════════════════╗")
اطبع("║    اختبار شامل لجميع المكتبات           ║")
اطبع("╚═══════════════════════════════════════════╝")

// 1. الرياضيات
اطبع("📐 الرياضيات:")
اطبع("  جذر(144) =", جذر(144))
اطبع("  قوة(2,8) =", قوة(2, 8))
اطبع("  مطلق(-99) =", مطلق(-99))

// 2. النصوص
اطبع("📝 النصوص:")
متغير جملة = "مرحبا بالعالم"
اطبع("  الطول:", طول(جملة))

// 3. الملفات
اطبع("📁 الملفات:")
اكتب_ملف("اختبار_شامل.txt", "نجاح")
متغير محتوى = اقرأ_ملف("اختبار_شامل.txt")
اطبع("  كتابة/قراءة:", محتوى)

// 4. المصفوفات
اطبع("📊 المصفوفات:")
متغير قائمة = [10, 20, 30]
اطبع("  المصفوفة:", قائمة)
اطبع("  الطول:", طول(قائمة))

// 5. الدوال
دالة مربع(ن):
    أرجع ن * ن
اطبع("🔧 الدوال:")
اطبع("  مربع(7) =", مربع(7))

// 6. الكائنات
صنف حيوان:
    خاص الاسم
    دالة تهيئة(اسم):
        هذا.الاسم = اسم
    دالة صوت():
        أرجع هذا.الاسم + " يصدر صوتاً"
قطة = جديد حيوان("القطة")
اطبع("🏗️ الكائنات:")
اطبع(" ", قطة.صوت())

// 7. معالجة الأخطاء
اطبع("🛡️ معالجة الأخطاء:")
حاول:
    نتيجة = 10 / 2
    اطبع("  10/2 =", نتيجة)
وإلا:
    اطبع("  خطأ!")

اطبع("╔═══════════════════════════════════════════╗")
اطبع("║     ✅ اكتمل الاختبار الشامل بنجاح       ║")
اطبع("╚═══════════════════════════════════════════╝")
)EXAMPLE"}
};

// مؤشر للنافذة الرئيسية
static ArabicIDE* g_pIDE = nullptr;

ArabicIDE::ArabicIDE()
    : state(IDEState::STATE_IDLE)
    , mainWindow(nullptr)
    , editControl(nullptr)
    , outputControl(nullptr)
    , statusBar(nullptr)
    , toolbar(nullptr)
    , hasUnsavedChanges(false)
    , compileBeforeRun(false)
    , isFormatting(false)
{
    compiler = std::make_unique<ArabicCompiler>();
    highlighter = std::make_unique<ArabicSyntaxHighlighter>();
    errorAnalyzer = std::make_unique<ArabicErrorAnalyzer>();

    // ✅ تهيئة المكونات المربوطة حديثاً
    packageManager = createPackageManager();

    debugger = std::make_unique<InteractiveDebugger>();
    debugger->initialize();
    debugger->setOutputCallback([this](const std::string& msg) {
        this->addOutput(OutputMessage(OutputMessage::INFO, "[DEBUG] " + msg));
    });

    // JIT: Singleton - يُهيأ عند الاستخدام الأول فقط
    
    // ✅ إعداد استقبال المخرجات من المترجم
    compiler->setOutputCallback([this](const std::string& msg) {
        std::string text = msg;
        // إزالة السطر الجديد لأن addOutput يضيف سطراً جديداً
        if (!text.empty() && text.back() == '\n') {
            text.pop_back();
        }
        // كل رسالة في سطر مستقل - سواء كانت اطبع أو اكتب
        if (!text.empty()) {
            this->addOutput(OutputMessage(OutputMessage::RESULT, text));
        }
    });
}

ArabicIDE::~ArabicIDE() {
    shutdown();
}

bool ArabicIDE::initialize(HINSTANCE hInstance, const IDEConfig& cfg) {
    config = cfg;
    
    // تسجيل فئة النافذة
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ArabicIDEWindow";
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"فشل تسجيل فئة النافذة", L"خطأ", MB_ICONERROR);
        return false;
    }
    
    // تهيئة Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    // تحميل RichEdit control
    LoadLibraryW(L"Msftedit.dll");
    
    // إنشاء النافذة الرئيسية
    g_pIDE = this;
    if (!createMainWindow(hInstance)) {
        return false;
    }
    
    // إنشاء عناصر الواجهة
    createToolbar();
    createEditor();
    createOutputPanel();
    createStatusBar();
    createMenus();
    
    // عرض النافذة
    ShowWindow(mainWindow, SW_SHOW);
    UpdateWindow(mainWindow);
    
    // تحديث شريط الحالة
    updateStatusBar("جاهز - اكتب كود عربي أو اضغط على 'أمثلة' للبدء");
    
    // إضافة رسالة ترحيب في لوحة المخرجات
    addOutput(OutputMessage(OutputMessage::INFO, "مرحباً بك في بيئة البيروني للتطوير بالعربية!"));
    addOutput(OutputMessage(OutputMessage::INFO, "اكتب كودك في المحرر أو اختر مثالاً من قائمة الأمثلة."));
    
    return true;
}

bool ArabicIDE::createMainWindow(HINSTANCE hInstance) {
    // تحويل العنوان لـ Unicode
    int wLen = MultiByteToWideChar(CP_UTF8, 0, config.windowTitle.c_str(), -1, nullptr, 0);
    std::wstring wTitle(wLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, config.windowTitle.c_str(), -1, &wTitle[0], wLen);
    
    mainWindow = CreateWindowExW(
        WS_EX_APPWINDOW, // Removed WS_EX_LAYOUTRTL temporarily
        L"ArabicIDEWindow",
        wTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        config.windowWidth, config.windowHeight,
        nullptr, nullptr, hInstance, nullptr
    );
    
    if (!mainWindow) {
        MessageBoxW(nullptr, L"فشل إنشاء النافذة الرئيسية", L"خطأ", MB_ICONERROR);
        return false;
    }
    
    return true;
}

void ArabicIDE::createToolbar() {
    // إنشاء شريط الأدوات
    toolbar = CreateWindowExW(
        0, TOOLBARCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST | CCS_TOP,
        0, 0, 0, 0,
        mainWindow, (HMENU)ID_TOOLBAR, GetModuleHandle(nullptr), nullptr
    );
    
    SendMessage(toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    
    // إضافة الصور القياسية
    TBADDBITMAP tbab = {HINST_COMMCTRL, IDB_STD_SMALL_COLOR};
    SendMessage(toolbar, TB_ADDBITMAP, 0, (LPARAM)&tbab);
    
    // إضافة نصوص الأزرار عبر TB_ADDSTRINGW (كل نص ينتهي بـ \0\0)
    // الترتيب مهم: الفهرس 0=جديد، 1=فتح، 2=حفظ، 3=تشغيل
    const wchar_t* stringsW = L"جديد\0فتح\0حفظ\0تشغيل\0";
    SendMessageW(toolbar, TB_ADDSTRINGW, 0, (LPARAM)stringsW);
    // الفهارس: جديد=0، فتح=1، حفظ=2، تشغيل=3
    
    // إضافة أزرار شريط الأدوات مع فهارس النصوص الصحيحة
    TBBUTTON buttons[] = {
        {STD_FILENEW,  ID_FILE_NEW,  TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_SHOWTEXT, {0}, 0, 0},
        {STD_FILEOPEN, ID_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_SHOWTEXT, {0}, 0, 1},
        {STD_FILESAVE, ID_FILE_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_SHOWTEXT, {0}, 0, 2},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0, 0}, // فاصل
        {I_IMAGENONE,  ID_RUN_RUN,   TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_SHOWTEXT, {0}, 0, 3},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0, 0}, // فاصل
    };
    
    SendMessage(toolbar, TB_ADDBUTTONS, 6, (LPARAM)buttons);
    
    // Checkbox للترجمة والتشغيل
    chkCompileRun = CreateWindowExW(
        0, L"BUTTON", L"ترجمة ثم تشغيل",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        400, 5, 150, 25,
        toolbar, (HMENU)ID_CHK_COMPILE_RUN, GetModuleHandle(nullptr), nullptr
    );
    
    // زر الأمثلة
    CreateWindowExW(
        0, L"BUTTON", L"أمثلة",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        560, 3, 80, 28,
        toolbar, (HMENU)ID_HELP_DOCS, GetModuleHandle(nullptr), nullptr
    );
    
    SendMessage(toolbar, TB_AUTOSIZE, 0, 0);
}

void ArabicIDE::createEditor() {
    RECT rc;
    GetClientRect(mainWindow, &rc);
    
    int toolbarHeight = 40;
    int statusHeight = 25;
    int outputHeight = 150;
    int editorHeight = rc.bottom - toolbarHeight - statusHeight - outputHeight - 10;
    
    // إنشاء محرر النصوص (RichEdit)
    editControl = CreateWindowExW(
        WS_EX_CLIENTEDGE | WS_EX_RIGHT, // Removed WS_EX_LAYOUTRTL
        MSFTEDIT_CLASS,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN | ES_RIGHT,
        5, toolbarHeight,
        rc.right - 10, editorHeight,
        mainWindow, (HMENU)ID_EDIT_CONTROL, GetModuleHandle(nullptr), nullptr
    );
    
    // تعيين الخط
    CHARFORMAT2W cf = {0};
    cf.cbSize = sizeof(CHARFORMAT2W);
    cf.dwMask = CFM_FACE | CFM_SIZE | CFM_CHARSET;
    
    // تحويل اسم الخط من الإعدادات
    int fLen = MultiByteToWideChar(CP_UTF8, 0, config.defaultFontName.c_str(), -1, nullptr, 0);
    std::wstring wFont(fLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, config.defaultFontName.c_str(), -1, &wFont[0], fLen);
    
    wcscpy_s(cf.szFaceName, wFont.c_str());
    cf.yHeight = 280; // 14pt
    cf.bCharSet = ARABIC_CHARSET;
    SendMessage(editControl, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    
    // تعيين لون الخلفية (الوضع الداكن)
    if (config.darkMode) {
        SendMessage(editControl, EM_SETBKGNDCOLOR, 0, RGB(30, 30, 30));
        
        // تعيين لون النص والخط
        cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
        cf.crTextColor = RGB(200, 200, 200);
        SendMessage(editControl, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    }
    
    // السماح بالكتابة
    SendMessage(editControl, EM_SETREADONLY, FALSE, 0);
    
    // تفعيل RTL بشكل كامل
    // SendMessage(editControl, EM_SETEDITSTYLE, SES_BIDI, SES_BIDI); // يمكن تفعيله إذا لزم الأمر
}

void ArabicIDE::createOutputPanel() {
    RECT rc;
    GetClientRect(mainWindow, &rc);
    
    int toolbarHeight = 40;
    int statusHeight = 25;
    int outputHeight = 150;
    int editorHeight = rc.bottom - toolbarHeight - statusHeight - outputHeight - 10;
    
    // عنوان لوحة المخرجات
    CreateWindowExW(
        0, L"STATIC", L" 📤 المخرجات:",
        WS_CHILD | WS_VISIBLE,
        5, rc.bottom - outputHeight - statusHeight - 20,
        rc.right - 10, 20,
        mainWindow, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    // إنشاء لوحة المخرجات (RichEdit)
    // ✅ إزالة ES_RIGHT و WS_EX_RIGHT لمنع مشاكل BiDi مع النصوص المختلطة
    outputControl = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        MSFTEDIT_CLASS,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        5, rc.bottom - outputHeight - statusHeight,
        rc.right - 10, outputHeight,
        mainWindow, (HMENU)ID_OUTPUT_CONTROL, GetModuleHandle(nullptr), nullptr
    );
    
    // تعيين الخط للوحة المخرجات
    CHARFORMAT2W cf = {0};
    cf.cbSize = sizeof(CHARFORMAT2W);
    cf.dwMask = CFM_FACE | CFM_SIZE | CFM_CHARSET;
    wcscpy_s(cf.szFaceName, L"Courier New");
    cf.yHeight = 240; // 12pt
    cf.bCharSet = ARABIC_CHARSET;
    SendMessage(outputControl, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    
    // ✅ إعدادات RTL/BiDi للنصوص المختلطة (عربي + إنجليزي)
    // تعيين اتجاه الفقرة لليمين لليسار
    PARAFORMAT2 pf = {0};
    pf.cbSize = sizeof(PARAFORMAT2);
    pf.dwMask = PFM_ALIGNMENT;
    pf.wAlignment = PFA_RIGHT;
    SendMessageW(outputControl, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
    
    // وضع داكن للوحة المخرجات
    if (config.darkMode) {
        SendMessage(outputControl, EM_SETBKGNDCOLOR, 0, RGB(20, 20, 20));
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = RGB(200, 200, 200);
        SendMessage(outputControl, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    }
}

void ArabicIDE::createStatusBar() {
    statusBar = CreateStatusWindowW(
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        L"جاهز",
        mainWindow,
        ID_STATUS_BAR
    );
}

void ArabicIDE::createMenus() {
    HMENU hMenu = CreateMenu();
    
    // قائمة ملف
    HMENU hFile = CreatePopupMenu();
    AppendMenuW(hFile, MF_STRING, ID_FILE_NEW, L"جديد\tCtrl+N");
    AppendMenuW(hFile, MF_STRING, ID_FILE_OPEN, L"فتح...\tCtrl+O");
    AppendMenuW(hFile, MF_STRING, ID_FILE_SAVE, L"حفظ\tCtrl+S");
    AppendMenuW(hFile, MF_STRING, ID_FILE_SAVEAS, L"حفظ باسم...");
    AppendMenuW(hFile, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFile, MF_STRING, ID_FILE_EXIT, L"خروج");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFile, L"ملف");
    
    // قائمة تشغيل
    HMENU hRun = CreatePopupMenu();
    AppendMenuW(hRun, MF_STRING, ID_RUN_RUN, L"تشغيل\tF5");
    AppendMenuW(hRun, MF_STRING, ID_RUN_COMPILE, L"ترجمة فقط\tF6");
    AppendMenuW(hRun, MF_STRING, ID_RUN_STOP, L"إيقاف\tShift+F5");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hRun, L"تشغيل");
    
    // قائمة أدوات
    HMENU hTools = CreatePopupMenu();
    AppendMenuW(hTools, MF_STRING, ID_TOOLS_SELF_UPDATE, L"التحديث الذاتي للمترجم (Self-Hosting)");
    AppendMenuW(hTools, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hTools, MF_STRING, ID_TOOLS_PACKAGES,   L"📦 مدير الحزم...\tCtrl+Shift+P");
    AppendMenuW(hTools, MF_STRING, ID_RUN_JIT,          L"⚡ تشغيل بـ JIT\tCtrl+Shift+J");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hTools, L"أدوات");

    // ✅ قائمة التصحيح
    HMENU hDebug = CreatePopupMenu();
    AppendMenuW(hDebug, MF_STRING, ID_DEBUG_START,     L"▶ بدء التصحيح\tF5");
    AppendMenuW(hDebug, MF_STRING, ID_DEBUG_STOP,      L"■ إيقاف التصحيح\tShift+F5");
    AppendMenuW(hDebug, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hDebug, MF_STRING, ID_DEBUG_STEP,      L"→ تنفيذ سطر\tF10");
    AppendMenuW(hDebug, MF_STRING, ID_DEBUG_STEP_OVER, L"↓ تنفيذ دالة\tF11");
    AppendMenuW(hDebug, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hDebug, MF_STRING, ID_DEBUG_TOGGLE_BP, L"🔴 تبديل نقطة توقف\tF9");
    AppendMenuW(hDebug, MF_STRING, ID_DEBUG_SHOW_BP,   L"📋 عرض نقاط التوقف");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hDebug, L"تصحيح");

    // قائمة مساعدة
    HMENU hHelp = CreatePopupMenu();
    AppendMenuW(hHelp, MF_STRING, ID_HELP_DOCS,      L"الأمثلة...");
    AppendMenuW(hHelp, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hHelp, MF_STRING, ID_HELP_ABOUT,     L"🔧 دليل المصحح والأدوات...");
    AppendMenuW(hHelp, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hHelp, MF_STRING, ID_HELP_ABOUT_APP, L"حول البيروني");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hHelp, L"مساعدة");
    
    SetMenu(mainWindow, hMenu);
}

int ArabicIDE::run() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

void ArabicIDE::shutdown() {
    // إغلاق أي موارد
}

LRESULT CALLBACK ArabicIDE::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            // أي تهيئة إضافية
            break;
            
        case WM_COMMAND:
            if (g_pIDE) {
                g_pIDE->handleCommand(wParam);
            }
            break;

        case WM_KEYDOWN:
            if (g_pIDE) {
                switch (wParam) {
                    case VK_F9:  g_pIDE->toggleBreakpoint(); break;
                    case VK_F10: g_pIDE->stepDebug();        break;
                    case VK_F11: g_pIDE->stepOverDebug();    break;
                }
            }
            break;

        case WM_USER + 1:
            // نتيجة التنفيذ قادمة من execution thread
            if (g_pIDE) {
                g_pIDE->state = IDEState::STATE_IDLE;
                if (wParam == 1) {
                    g_pIDE->addOutput(OutputMessage(OutputMessage::SUCCESS, "✅ اكتمل التنفيذ بنجاح."));
                    g_pIDE->updateStatusBar("✅ اكتمل التنفيذ");
                } else {
                    g_pIDE->addOutput(OutputMessage(OutputMessage::ERROR_MSG, "❌ توقف التنفيذ بسبب خطأ."));
                    g_pIDE->updateStatusBar("❌ خطأ في التنفيذ");
                }
            }
            break;
            
        case WM_SIZE:
            if (g_pIDE) {
                g_pIDE->handleResize(LOWORD(lParam), HIWORD(lParam));
            }
            break;
            
        case WM_CLOSE:
            if (g_pIDE && g_pIDE->hasUnsavedChanges) {
                if (!g_pIDE->confirmSaveChanges()) {
                    return 0; // إلغاء الإغلاق
                }
            }
            PostQuitMessage(0);
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

void ArabicIDE::handleCommand(WPARAM wParam) {
    int cmd = LOWORD(wParam);
    int notify = HIWORD(wParam);

    // مراقبة تغييرات كود المحرر
    if (notify == EN_CHANGE && cmd == ID_EDIT_CONTROL) {
        if (!isFormatting) {
            isFormatting = true;
            applySyntaxHighlighting();
            analyzeCodeErrors();
            isFormatting = false;
        }
        hasUnsavedChanges = true;
        return;
    }
    
    // أوامر الملف
    if (cmd == ID_FILE_NEW) { newFile(); return; }
    if (cmd == ID_FILE_OPEN) { openFile(); return; }
    if (cmd == ID_FILE_SAVE) { saveFile(); return; }
    if (cmd == ID_FILE_SAVEAS) { saveFileAs(); return; }
    if (cmd == ID_FILE_EXIT) { 
        PostMessage(mainWindow, WM_CLOSE, 0, 0); 
        return; 
    }
    
    // أوامر التشغيل
    if (cmd == ID_RUN_RUN) {
        if (compileBeforeRun) {
            compileAndRun();
        } else {
            runCode();
        }
        return;
    }
    if (cmd == ID_RUN_COMPILE) { compile(); return; }
    if (cmd == ID_RUN_STOP) { stopExecution(); return; }
    
    // Checkbox للترجمة والتشغيل
    if (cmd == ID_CHK_COMPILE_RUN) {
        compileBeforeRun = (SendMessage(chkCompileRun, BM_GETCHECK, 0, 0) == BST_CHECKED);
        return;
    }
    
    // زر الأمثلة
    if (cmd == ID_HELP_DOCS) {
        showExamplesMenu();
        return;
    }
    
    // أوامر الأمثلة
    if (cmd >= ID_EXAMPLE_BASE && cmd < ID_EXAMPLE_BASE + (int)CODE_EXAMPLES.size()) {
        loadExample(cmd - ID_EXAMPLE_BASE);
        return;
    }
    
    // أوامر الأدوات
    if (cmd == ID_TOOLS_SELF_UPDATE) {
        addOutput(OutputMessage(OutputMessage::INFO, "⚙️ بدء عملية التحديث الذاتي..."));
        
        bool success = compiler->buildSelfHosting();
        
        if (success) {
            addOutput(OutputMessage(OutputMessage::SUCCESS, "🎉 تم تحديث المترجم بنجاح باستخدام كود البرمجة العربي!"));
            MessageBoxW(mainWindow, L"تم تحديث النظام بنجاح!", L"تحديث موفق", MB_OK | MB_ICONINFORMATION);
        } else {
            addOutput(OutputMessage(OutputMessage::ERROR_MSG, "❌ فشلت عملية التحديث الذاتي. راجع سجل الأخطاء."));
            MessageBoxW(mainWindow, L"فشل تحديث النظام", L"خطأ", MB_OK | MB_ICONERROR);
        }
        return;
    }

    if (cmd == ID_TOOLS_PACKAGES) {
        showPackageManager();
        return;
    }

    if (cmd == ID_RUN_JIT) {
        toggleJIT();
        if (useJIT) {
            std::string code = getCodeFromEditor();
            if (!code.empty()) runWithJIT(code);
        }
        return;
    }

    // أوامر التصحيح
    if (cmd == ID_DEBUG_START)    { startDebugging();  return; }
    if (cmd == ID_DEBUG_STOP)     { stopDebugging();   return; }
    if (cmd == ID_DEBUG_STEP)     { stepDebug();       return; }
    if (cmd == ID_DEBUG_STEP_OVER){ stepOverDebug();   return; }
    if (cmd == ID_DEBUG_TOGGLE_BP){ toggleBreakpoint();return; }
    if (cmd == ID_DEBUG_SHOW_BP)  { showBreakpoints(); return; }

    // أوامر المساعدة
    if (cmd == ID_HELP_ABOUT) {      // "دليل المصحح والأدوات"
        showHelpDialog();
        return;
    }

    if (cmd == ID_HELP_ABOUT_APP) {  // "حول البيروني"
        MessageBoxW(mainWindow,
            L"البيروني - بيئة التطوير العربية\n\n"
            L"الإصدار 2.0.0\n\n"
            L"مكونات النظام:\n"
            L"  • مترجم اللغة العربية\n"
            L"  • مصحح تفاعلي (Debugger)\n"
            L"  • مُجمِّع JIT للأداء العالي\n"
            L"  • مدير الحزم\n"
            L"  • تلوين الصيغة في الوقت الحقيقي\n\n"
            L"للمساعدة: قائمة مساعدة > دليل المصحح والأدوات",
            L"حول البيروني", MB_ICONINFORMATION);
        return;
    }

    if (cmd == ID_HELP_DOCS) {
        showExamplesMenu();
        return;
    }
}

void ArabicIDE::showExamplesMenu() {
    HMENU hMenu = CreatePopupMenu();
    
    for (size_t i = 0; i < CODE_EXAMPLES.size(); ++i) {
        int wLen = MultiByteToWideChar(CP_UTF8, 0, CODE_EXAMPLES[i].first.c_str(), -1, nullptr, 0);
        std::wstring wName(wLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, CODE_EXAMPLES[i].first.c_str(), -1, &wName[0], wLen);
        AppendMenuW(hMenu, MF_STRING, ID_EXAMPLE_BASE + i, wName.c_str());
    }
    
    POINT pt;
    GetCursorPos(&pt);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, mainWindow, nullptr);
    DestroyMenu(hMenu);
}

void ArabicIDE::showHelpDialog() {

    // ════════════════════════════════════════════
    // نافذة 1: الدليل الرئيسي
    // ════════════════════════════════════════════
    int choice = MessageBoxW(mainWindow,
        L"╔══════════════════════════════════════════════════╗\n"
        L"║      البيروني - بيئة التطوير العربية v2.0      ║\n"
        L"╚══════════════════════════════════════════════════╝\n\n"
        L"اختر موضوع المساعدة:\n\n"
        L"• اضغط [نعم]    ← 🔧 المصحح (Debugger) + أمثلة\n"
        L"• اضغط [لا]     ← ⚡ JIT + 📦 مدير الحزم + أمثلة\n"
        L"• اضغط [إلغاء]  ← 📖 دليل عام سريع",
        L"دليل الاستخدام - اختر موضوعاً",
        MB_YESNOCANCEL | MB_ICONQUESTION);

    // ════════════════════════════════════════════
    // نافذة 2: المصحح
    // ════════════════════════════════════════════
    if (choice == IDYES) {
        MessageBoxW(mainWindow,
            L"🔧 دليل المصحح التفاعلي (Debugger)\n"
            L"══════════════════════════════════════\n\n"

            L"📌 ما هو المصحح؟\n"
            L"  أداة تتيح لك تتبع تنفيذ كودك سطراً بسطر\n"
            L"  ومراقبة قيم المتغيرات في كل خطوة.\n\n"

            L"🚀 خطوات استخدام المصحح:\n"
            L"  1️⃣  احفظ ملفك أولاً  (Ctrl+S)\n"
            L"  2️⃣  ضع نقطة توقف عند السطر المطلوب  (F9)\n"
            L"      ← السطر يُمييَّز بـ 🔴 في المخرجات\n"
            L"  3️⃣  ابدأ التصحيح  (قائمة تصحيح > بدء التصحيح)\n"
            L"      ← سيتوقف البرنامج عند أول نقطة توقف\n"
            L"  4️⃣  تنقل بين الأسطر:\n"
            L"      F10  = تنفيذ السطر الحالي والانتقال للتالي\n"
            L"      F11  = الدخول داخل الدالة المستدعاة\n"
            L"  5️⃣  لإيقاف التصحيح: قائمة تصحيح > إيقاف التصحيح\n\n"

            L"⌨️  اختصارات لوحة المفاتيح:\n"
            L"  F9           تبديل نقطة توقف (إضافة/إزالة)\n"
            L"  F10          تنفيذ سطر (تخطي الدوال)\n"
            L"  F11          تنفيذ سطر (الدخول في الدوال)\n"
            L"  Shift+F5     إيقاف التصحيح\n\n"

            L"💡 مثال عملي - ضع نقاط توقف في هذا الكود:\n"
            L"  ──────────────────────────────────────\n"
            L"  دالة احسب_مجموع(أ، ب):\n"
            L"      النتيجة = أ + ب          ← F9 هنا\n"
            L"      أرجع النتيجة\n"
            L"  نهاية\n\n"
            L"  س = 5\n"
            L"  ص = 10\n"
            L"  الجواب = احسب_مجموع(س، ص)  ← F9 هنا\n"
            L"  اطبع(\"النتيجة:\", الجواب)    ← F9 هنا\n"
            L"  ──────────────────────────────────────\n"
            L"  → احفظ، ثم اضغط F9 على أي سطر أعلاه\n"
            L"  → ثم اختر: تصحيح > بدء التصحيح\n"
            L"  → استخدم F10 للتنقل سطراً بسطر\n\n"

            L"📋 عرض نقاط التوقف:\n"
            L"  قائمة تصحيح > عرض نقاط التوقف\n"
            L"  ← تظهر في لوحة المخرجات بالأسفل\n\n"

            L"🎯 نصائح احترافية:\n"
            L"  • ضع نقاط توقف قبل الأجزاء التي تشك فيها\n"
            L"  • استخدم F11 لفحص ما يجري داخل الدوال\n"
            L"  • راقب لوحة المخرجات لرؤية تقدم التنفيذ",
            L"🔧 دليل المصحح (Debugger)",
            MB_OK | MB_ICONINFORMATION);

        return;
    }

    // ════════════════════════════════════════════
    // نافذة 3: JIT + مدير الحزم
    // ════════════════════════════════════════════
    if (choice == IDNO) {
        MessageBoxW(mainWindow,
            L"⚡ JIT (Just-In-Time Compilation)\n"
            L"══════════════════════════════════════\n\n"

            L"📌 ما هو JIT؟\n"
            L"  تقنية تُجمِّع الكود إلى تعليمات Bytecode\n"
            L"  محسَّنة قبل التنفيذ، مما يجعله أسرع\n"
            L"  من التفسير المباشر خاصةً للعمليات المتكررة.\n\n"

            L"🚀 كيفية الاستخدام:\n"
            L"  1️⃣  اكتب كودك في المحرر كالمعتاد\n"
            L"  2️⃣  اختر: أدوات > ⚡ تشغيل بـ JIT\n"
            L"  3️⃣  سيظهر في المخرجات:\n"
            L"      ⚡ جاري التجميع والتنفيذ بـ JIT...\n"
            L"      ⚡ JIT: تم التجميع - N تعليمة bytecode\n"
            L"      ✅ اكتمل التنفيذ بـ JIT بنجاح\n\n"

            L"💡 مثال: اختبر JIT مع حلقة مكثفة:\n"
            L"  ──────────────────────────────────────\n"
            L"  // حلقة مليون تكرار - اختبر الأداء!\n"
            L"  مجموع = 0\n"
            L"  لكل ع من 1 إلى 1000000:\n"
            L"      مجموع = مجموع + ع\n"
            L"  نهاية\n"
            L"  اطبع(\"المجموع:\", مجموع)\n"
            L"  ──────────────────────────────────────\n"
            L"  → شغّله بـ F5  (تفسير عادي)\n"
            L"  → ثم بـ JIT    (أدوات > تشغيل بـ JIT)\n"
            L"  → قارن السرعة!\n\n"

            L"━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n"

            L"📦 مدير الحزم (Package Manager)\n"
            L"══════════════════════════════════════\n\n"

            L"📌 ما هو مدير الحزم؟\n"
            L"  نظام لإدارة مكتبات اللغة العربية:\n"
            L"  تثبيت، تحديث، وإزالة المكتبات الخارجية.\n\n"

            L"🚀 كيفية الاستخدام:\n"
            L"  اختر: أدوات > 📦 مدير الحزم\n"
            L"  ← يعرض الحزم المثبتة في لوحة المخرجات\n\n"

            L"📦 مثال: كيف تستورد حزمة في كودك:\n"
            L"  ──────────────────────────────────────\n"
            L"  // استيراد مكتبة الرياضيات المتقدمة\n"
            L"  استورد \"رياضيات\"\n\n"
            L"  // استخدام دوال من المكتبة\n"
            L"  ناتج = جذر(144)\n"
            L"  اطبع(\"جذر 144 =\", ناتج)\n\n"
            L"  // استيراد مكتبة الشبكات\n"
            L"  استورد \"شبكات\"\n"
            L"  ──────────────────────────────────────\n\n"

            L"📚 الحزم المتاحة حالياً:\n"
            L"  • رياضيات       - العمليات الرياضية المتقدمة\n"
            L"  • نصوص          - معالجة النصوص\n"
            L"  • مصفوفات       - عمليات المصفوفات\n"
            L"  • مدخلات_مخرجات - قراءة وكتابة الملفات\n"
            L"  • شبكات         - الاتصال بالشبكة\n"
            L"  • جامع_القمامة  - إدارة الذاكرة التلقائية\n\n"

            L"🎯 ملاحظة:\n"
            L"  واجهة مدير الحزم الكاملة (تثبيت/حذف)\n"
            L"  قيد التطوير وستتوفر في الإصدار القادم.",
            L"⚡ JIT و 📦 مدير الحزم",
            MB_OK | MB_ICONINFORMATION);

        return;
    }

    // ════════════════════════════════════════════
    // نافذة 4: الدليل العام السريع (إلغاء)
    // ════════════════════════════════════════════
    MessageBoxW(mainWindow,
        L"📖 الدليل العام السريع - البيروني v2.0\n"
        L"══════════════════════════════════════════\n\n"

        L"⌨️  اختصارات أساسية:\n"
        L"  Ctrl+N     ملف جديد\n"
        L"  Ctrl+O     فتح ملف\n"
        L"  Ctrl+S     حفظ\n"
        L"  F5         تشغيل الكود\n"
        L"  F6         ترجمة فقط\n"
        L"  Shift+F5   إيقاف التنفيذ\n\n"

        L"🔧 اختصارات التصحيح:\n"
        L"  F9         نقطة توقف\n"
        L"  F10        تنفيذ سطر\n"
        L"  F11        دخول دالة\n\n"

        L"🎯 سير عمل سريع:\n"
        L"  1. اكتب كودك أو اختر مثالاً من 'أمثلة'\n"
        L"  2. اضغط F5 للتشغيل\n"
        L"  3. راقب النتائج في لوحة المخرجات\n\n"

        L"🔧 للتصحيح:\n"
        L"  F9 ← F5 (تصحيح) ← F10\n\n"

        L"⚡ للأداء الأعلى:\n"
        L"  أدوات > تشغيل بـ JIT\n\n"

        L"📦 للمكتبات:\n"
        L"  أدوات > مدير الحزم\n\n"

        L"💡 مثال بسيط للبدء:\n"
        L"  ──────────────────\n"
        L"  اسم = \"أحمد\"\n"
        L"  اطبع(\"مرحباً يا\", اسم)\n"
        L"  ──────────────────\n"
        L"  → اكتبه ثم اضغط F5",
        L"📖 الدليل السريع",
        MB_OK | MB_ICONINFORMATION);
}

void ArabicIDE::loadExample(int index) {
    if (index < 0 || index >= (int)CODE_EXAMPLES.size()) return;
    
    // تحذير إذا كان هناك تغييرات غير محفوظة
    if (hasUnsavedChanges) {
        int result = MessageBoxW(mainWindow,
            L"هناك تغييرات غير محفوظة. هل تريد استبدال المحتوى؟",
            L"تحذير", MB_YESNO | MB_ICONWARNING);
        if (result != IDYES) return;
    }
    
    // تحويل الكود للـ Unicode
    const std::string& code = CODE_EXAMPLES[index].second;
    int wLen = MultiByteToWideChar(CP_UTF8, 0, code.c_str(), -1, nullptr, 0);
    std::wstring wCode(wLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, code.c_str(), -1, &wCode[0], wLen);
    
    // وضع الكود في المحرر
    isFormatting = true;
    SetWindowTextW(editControl, wCode.c_str());
    isFormatting = false;
    
    // تنفيذ التلوين يدوياً بعد وضع النص لضمان تحديثه
    applySyntaxHighlighting();
    
    // تحديث الحالة
    hasUnsavedChanges = true;
    currentFileName = CODE_EXAMPLES[index].first + ".عربي";
    
    // رسالة في المخرجات
    addOutput(OutputMessage(OutputMessage::INFO, "تم تحميل مثال: " + CODE_EXAMPLES[index].first));
    
    updateStatusBar("تم تحميل المثال - اضغط F5 للتشغيل");
}

void ArabicIDE::handleResize(int width, int height) {
    if (!mainWindow) return;
    
    int toolbarHeight = 40;
    int statusHeight = 25;
    int outputHeight = 150;
    int editorHeight = height - toolbarHeight - statusHeight - outputHeight - 35;
    
    // تحديث حجم شريط الأدوات
    SendMessage(toolbar, TB_AUTOSIZE, 0, 0);
    
    // تحديث حجم المحرر
    if (editControl) {
        SetWindowPos(editControl, nullptr, 5, toolbarHeight, width - 10, editorHeight, SWP_NOZORDER);
    }
    
    // تحديث حجم لوحة المخرجات
    if (outputControl) {
        SetWindowPos(outputControl, nullptr, 5, height - outputHeight - statusHeight, width - 10, outputHeight, SWP_NOZORDER);
    }
    
    // تحديث حجم شريط الحالة
    if (statusBar) {
        SendMessage(statusBar, WM_SIZE, 0, 0);
    }
}

bool ArabicIDE::newFile() {
    if (hasUnsavedChanges) {
        if (!confirmSaveChanges()) return false;
    }
    
    SetWindowTextW(editControl, L"");
    currentFilePath = "";
    currentFileName = "بدون_عنوان.عربي";
    hasUnsavedChanges = false;
    updateStatusBar("ملف جديد");
    return true;
}

bool ArabicIDE::openFile(const std::string& path) {
    std::string finalPath = path;
    if (finalPath.empty()) {
        finalPath = showOpenDialog();
    }
    
    if (finalPath.empty()) return false;
    
    std::ifstream file(finalPath);
    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();
        
        // تحويل للـ Unicode
        int wLen = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, nullptr, 0);
        std::wstring wContent(wLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, &wContent[0], wLen);
        
        SetWindowTextW(editControl, wContent.c_str());
        currentFilePath = finalPath;
        
        size_t lastSlash = finalPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            currentFileName = finalPath.substr(lastSlash + 1);
        } else {
            currentFileName = finalPath;
        }
        
        hasUnsavedChanges = false;
        updateStatusBar("تم فتح: " + currentFileName);
        return true;
    }
    return false;
}

bool ArabicIDE::saveFile() {
    if (currentFilePath.empty()) {
        return saveFileAs();
    }
    
    std::string code = getCodeFromEditor();
    std::ofstream file(currentFilePath);
    if (file.is_open()) {
        file << code;
        hasUnsavedChanges = false;
        updateStatusBar("تم الحفظ: " + currentFileName);
        return true;
    }
    return false;
}

bool ArabicIDE::saveFileAs() {
    std::string path = showSaveDialog();
    if (path.empty()) return false;
    
    currentFilePath = path;
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        currentFileName = path.substr(lastSlash + 1);
    } else {
        currentFileName = path;
    }
    
    return saveFile();
}

bool ArabicIDE::runCode() {
    std::string code = getCodeFromEditor();
    if (code.empty()) return false;

    // منع التشغيل المتزامن
    if (state == IDEState::STATE_RUNNING) {
        addOutput(OutputMessage(OutputMessage::WARNING, "⚠️ يوجد كود قيد التشغيل بالفعل."));
        return false;
    }

    state = IDEState::STATE_RUNNING;
    addOutput(OutputMessage(OutputMessage::INFO, "🚀 بدء التنفيذ..."));

    bool success = false;
    try {
        std::cerr << "[DEBUG] About to call executeIntermediate" << std::endl;
        success = compiler->executeIntermediate(code);
        std::cerr << "[DEBUG] executeIntermediate returned: " << success << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DEBUG] Exception: " << e.what() << std::endl;
        addOutput(OutputMessage(OutputMessage::ERROR_MSG, std::string("❌ خطأ: ") + e.what()));
        success = false;
    } catch (...) {
        std::cerr << "[DEBUG] Unknown exception" << std::endl;
        success = false;
    }

    if (success) {
        state = IDEState::STATE_IDLE;
        addOutput(OutputMessage(OutputMessage::SUCCESS, "✅ اكتمل التنفيذ بنجاح."));
        updateStatusBar("✅ اكتمل التنفيذ");
    } else {
        state = IDEState::STATE_IDLE;
        addOutput(OutputMessage(OutputMessage::ERROR_MSG, "❌ توقف التنفيذ بسبب خطأ."));
        updateStatusBar("❌ خطأ في التنفيذ");
    }

    return success;
}

bool ArabicIDE::compile() {
    std::string code = getCodeFromEditor();
    if (code.empty()) return false;
    
    state = IDEState::STATE_COMPILING;
    addOutput(OutputMessage(OutputMessage::INFO, "🔨 جاري الترجمة..."));
    
    bool success = compiler->compile(code, "برنامج_مؤقت.exe");
    
    state = IDEState::STATE_IDLE;
    if (success) {
        addOutput(OutputMessage(OutputMessage::SUCCESS, "✅ تمت الترجمة بنجاح."));
    } else {
        addOutput(OutputMessage(OutputMessage::ERROR_MSG, "❌ فشلت عملية الترجمة."));
    }
    
    return success;
}

bool ArabicIDE::compileAndRun() {
    if (compile()) {
        std::string outputName = currentFileName;
        if (outputName.empty()) outputName = "برنامج_مؤقت";
        size_t dotPos = outputName.find_last_of('.');
        if (dotPos != std::string::npos) outputName = outputName.substr(0, dotPos);
        
        std::string exePath = outputName + ".exe";
        
        addOutput(OutputMessage(OutputMessage::INFO, "▶ تشغيل: " + exePath));
        
        // تشغيل الملف التنفيذي
        ShellExecuteA(nullptr, "open", exePath.c_str(), nullptr, nullptr, SW_SHOW);
        
        return true;
    }
    return false;
}

void ArabicIDE::stopExecution() {
    if (state == IDEState::STATE_RUNNING) {
        compiler->stopExecution();
    }
    state = IDEState::STATE_IDLE;
    addOutput(OutputMessage(OutputMessage::WARNING, "⏹ تم إيقاف التنفيذ"));
    updateStatusBar("تم الإيقاف");
}

// واجهة المستخدم
void ArabicIDE::clearOutput() {
    outputMessages.clear();
    SetWindowTextW(outputControl, L"");
}

void ArabicIDE::addOutput(const OutputMessage& msg) {
    outputMessages.push_back(msg);
    
    // تحويل الرسالة للـ Unicode مع دعم النصوص العربية بكلا الترميزين
    // محاولة UTF-8 أولاً - إذا فشلت أو كانت النتيجة غير صحيحة، نستخدم CP_ACP (Windows-1256)
    std::wstring wMsg;
    const char* src = msg.message.c_str();
    int srcLen = -1;
    
    // جرّب UTF-8 أولاً - MB_ERR_INVALID_CHARS للكشف عن أخطاء الترميز
    int wLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, srcLen, nullptr, 0);
    if (wLen > 1) {
        // النص صحيح UTF-8
        wMsg.resize(wLen);
        MultiByteToWideChar(CP_UTF8, 0, src, srcLen, &wMsg[0], wLen);
        wMsg.resize(wLen - 1);
    } else {
        // ليس UTF-8 صحيحاً - استخدم ترميز النظام (Windows-1256 على الويندوز العربي)
        wLen = MultiByteToWideChar(CP_ACP, 0, src, srcLen, nullptr, 0);
        if (wLen > 1) {
            wMsg.resize(wLen);
            MultiByteToWideChar(CP_ACP, 0, src, srcLen, &wMsg[0], wLen);
            wMsg.resize(wLen - 1);
        } else {
            // آخر محاولة: CP_UTF8 بدون تحقق
            wLen = MultiByteToWideChar(CP_UTF8, 0, src, srcLen, nullptr, 0);
            if (wLen > 1) {
                wMsg.resize(wLen);
                MultiByteToWideChar(CP_UTF8, 0, src, srcLen, &wMsg[0], wLen);
                wMsg.resize(wLen - 1);
            }
        }
    }
    
    // إضافة محرف تحكم RTL في البداية لإصلاح ترتيب النصوص المختلطة
    wMsg = L"\u200F" + wMsg;
    wMsg += L"\r\n";
    
    // إضافة للنهاية
    int len = GetWindowTextLengthW(outputControl);
    SendMessage(outputControl, EM_SETSEL, len, len);
    
    // تعيين اللون حسب نوع الرسالة
    CHARFORMAT2W cf = {0};
    cf.cbSize = sizeof(CHARFORMAT2W);
    cf.dwMask = CFM_COLOR | CFM_BOLD;
    
    switch (msg.type) {
        case OutputMessage::INFO:
            cf.crTextColor = RGB(150, 150, 255);
            break;
        case OutputMessage::SUCCESS:
            cf.crTextColor = RGB(100, 255, 100);
            cf.dwEffects |= CFE_BOLD;
            break;
        case OutputMessage::WARNING:
            cf.crTextColor = RGB(255, 255, 100);
            break;
        case OutputMessage::ERROR_MSG:
            cf.crTextColor = RGB(255, 100, 100);
            cf.dwEffects |= CFE_BOLD;
            break;
        case OutputMessage::RESULT:
            cf.crTextColor = RGB(255, 255, 255);
            break;
    }
    
    SendMessage(outputControl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageW(outputControl, EM_REPLACESEL, FALSE, (LPARAM)wMsg.c_str());
    
    // تمرير للنهاية
    SendMessage(outputControl, WM_VSCROLL, SB_BOTTOM, 0);
}

void ArabicIDE::applySyntaxHighlighting() {
    if (!editControl) return;

    // تجميد التحديث لمنع الوميض
    SendMessage(editControl, WM_SETREDRAW, FALSE, 0);

    // حفظ الموقع الحالي
    CHARRANGE cr;
    SendMessage(editControl, EM_EXGETSEL, 0, (LPARAM)&cr);

    std::string text = getCodeFromEditor();
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);

    // تلوين كل سطر
    for (size_t i = 0; i < lines.size(); ++i) {
        // ✅ الطريقة الصحيحة: EM_LINEINDEX يعطي موضع البداية مباشرة من RichEdit
        long lineStart = (long)SendMessage(editControl, EM_LINEINDEX, (WPARAM)i, 0);
        if (lineStart < 0) break; // السطر غير موجود

        auto tokens = highlighter->highlightLine(lines[i], i);

        for (const auto& token : tokens) {
            // تحويل byte offset → char offset داخل السطر
            int charColumn = 0;
            for (size_t ci = 0; ci < token.column && ci < lines[i].size(); ) {
                unsigned char c = static_cast<unsigned char>(lines[i][ci]);
                if      ((c & 0x80) == 0x00) ci += 1;  // ASCII
                else if ((c & 0xE0) == 0xC0) ci += 2;  // 2-byte (عربي)
                else if ((c & 0xF0) == 0xE0) ci += 3;  // 3-byte (رموز BMP)
                else if ((c & 0xF8) == 0xF0) ci += 4;  // 4-byte (emoji)
                else                          ci += 1;
                charColumn++;
            }

            // تحويل byte length → char length للرمز
            int charTokenLen = 0;
            for (size_t ci = 0; ci < token.text.size(); ) {
                unsigned char c = static_cast<unsigned char>(token.text[ci]);
                if      ((c & 0x80) == 0x00) ci += 1;
                else if ((c & 0xE0) == 0xC0) ci += 2;
                else if ((c & 0xF0) == 0xE0) ci += 3;
                else if ((c & 0xF8) == 0xF0) ci += 4;
                else                          ci += 1;
                charTokenLen++;
            }

            if (charTokenLen == 0) continue;

            // اختيار النطاق وتطبيق اللون
            CHARRANGE tokenRange;
            tokenRange.cpMin = lineStart + charColumn;
            tokenRange.cpMax = lineStart + charColumn + charTokenLen;
            SendMessage(editControl, EM_EXSETSEL, 0, (LPARAM)&tokenRange);

            // UIColor تخزّن القيم كـ float (0.0~1.0)، لذا نضرب في 255
            CHARFORMAT2W cf = {0};
            cf.cbSize    = sizeof(CHARFORMAT2W);
            cf.dwMask    = CFM_COLOR | CFM_EFFECTS;
            cf.dwEffects = 0; // مسح CFE_AUTOCOLOR
            cf.crTextColor = RGB(
                (int)(token.color.r * 255.0f + 0.5f),
                (int)(token.color.g * 255.0f + 0.5f),
                (int)(token.color.b * 255.0f + 0.5f)
            );
            SendMessage(editControl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        }
    }

    // استعادة الموقع الأصلي
    SendMessage(editControl, EM_EXSETSEL, 0, (LPARAM)&cr);
    
    // تعيين لون افتراضي للنص التالي
    CHARFORMAT2W cfNormal = {0};
    cfNormal.cbSize = sizeof(CHARFORMAT2W);
    cfNormal.dwMask = CFM_COLOR;
    cfNormal.crTextColor = RGB(200, 200, 200);
    SendMessage(editControl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfNormal);

    SendMessage(editControl, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(editControl, nullptr, TRUE);
}

void ArabicIDE::analyzeCodeErrors() {
    std::string text = getCodeFromEditor();
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);

    auto errors = errorAnalyzer->analyzeCode(lines);
    if (!errors.empty()) {
        updateStatusBar("⚠️ تنبيه: تم العثور على " + std::to_string(errors.size()) + " مشاكل محتملة في الكود العربي.");
    }
}

void ArabicIDE::updateStatusBar(const std::string& text) {
    if (!statusBar) return;
    int wLen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring wText;
    if (wLen > 1) {
        wText.resize(wLen);
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wText[0], wLen);
        wText.resize(wLen - 1);
    }
    SendMessageW(statusBar, SB_SETTEXTW, 0, (LPARAM)wText.c_str());
}

std::string ArabicIDE::getCodeFromEditor() {
    int len = GetWindowTextLengthW(editControl);
    if (len == 0) return "";

    std::wstring wCode(len + 1, 0);
    GetWindowTextW(editControl, &wCode[0], len + 1);
    wCode.resize(len); // حذف null terminator

    // تحويل للـ UTF-8
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wCode.c_str(), (int)wCode.size(), nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 0) return "";

    std::string code(utf8Len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wCode.c_str(), (int)wCode.size(), &code[0], utf8Len, nullptr, nullptr);

    // حذف المسافات البيضاء من البداية والنهاية للتحقق من الكود الفارغ
    size_t first = code.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";

    return code;
}

void ArabicIDE::setCodeInEditor(const std::string& code) {
    int wLen = MultiByteToWideChar(CP_UTF8, 0, code.c_str(), -1, nullptr, 0);
    std::wstring wCode;
    if (wLen > 1) {
        wCode.resize(wLen);
        MultiByteToWideChar(CP_UTF8, 0, code.c_str(), -1, &wCode[0], wLen);
        wCode.resize(wLen - 1);
    }
    SetWindowTextW(editControl, wCode.c_str());
}

std::string ArabicIDE::showOpenDialog() {
    OPENFILENAMEW ofn;
    wchar_t szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = mainWindow;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"ملفات عربية (*.عربي;*.ar)\0*.عربي;*.ar\0الكل (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameW(&ofn)) {
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, nullptr, 0, nullptr, nullptr);
        std::string path(utf8Len, 0);
        WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, &path[0], utf8Len, nullptr, nullptr);
        return path;
    }
    return "";
}

std::string ArabicIDE::showSaveDialog() {
    OPENFILENAMEW ofn;
    wchar_t szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = mainWindow;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"ملفات عربية (*.عربي;*.ar)\0*.عربي;*.ar\0الكل (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"عربي";
    
    if (GetSaveFileNameW(&ofn)) {
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, nullptr, 0, nullptr, nullptr);
        std::string path(utf8Len, 0);
        WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, &path[0], utf8Len, nullptr, nullptr);
        return path;
    }
    return "";
}

bool ArabicIDE::confirmSaveChanges() {
    int result = MessageBoxW(mainWindow, 
        L"هل تريد حفظ التغييرات في الملف الحالي؟", 
        L"حفظ التغييرات", 
        MB_YESNOCANCEL | MB_ICONQUESTION);
    
    if (result == IDYES) {
        saveFile();
        return true;
    } else if (result == IDNO) {
        return true;
    }
    return false;
}


// ════════════════════════════════════════════════════════════
// ✅ دوال المصحح (Debugger)
// ════════════════════════════════════════════════════════════

void ArabicIDE::startDebugging() {
    if (!debugger) return;

    // احفظ الملف أولاً إذا لزم
    if (hasUnsavedChanges) saveFile();

    if (currentFilePath.empty()) {
        addOutput(OutputMessage(OutputMessage::WARNING,
            "⚠️ احفظ الملف أولاً ثم ابدأ التصحيح"));
        return;
    }

    isDebugging = true;
    debugger->initialize();
    debugger->startDebugging(currentFilePath);
    debugLine = 1;
    highlightDebugLine(debugLine);

    addOutput(OutputMessage(OutputMessage::SUCCESS,
        "▶ بدأ التصحيح للملف: " + currentFilePath));
    addOutput(OutputMessage(OutputMessage::INFO,
        "💡 F10 = سطر تالٍ  |  F11 = دخول دالة  |  F9 = نقطة توقف  |  Shift+F5 = إيقاف"));
    updateStatusBar("🔧 وضع التصحيح - السطر 1");
}

void ArabicIDE::stopDebugging() {
    if (!debugger) return;
    isDebugging = false;
    debugLine   = -1;
    debugger->stopDebugging();

    // إزالة تمييز سطر التصحيح
    highlightDebugLine(-1);

    addOutput(OutputMessage(OutputMessage::INFO, "■ انتهى التصحيح"));
    updateStatusBar("جاهز");
}

void ArabicIDE::stepDebug() {
    if (!debugger || !isDebugging) {
        addOutput(OutputMessage(OutputMessage::WARNING,
            "⚠️ ابدأ التصحيح أولاً (قائمة تصحيح > بدء التصحيح)"));
        return;
    }
    debugger->stepOver();
    debugLine = debugger->getCurrentLine();
    highlightDebugLine(debugLine);
    updateStatusBar("🔧 التصحيح - السطر " + std::to_string(debugLine));
}

void ArabicIDE::stepOverDebug() {
    if (!debugger || !isDebugging) return;
    debugger->stepInto();
    debugLine = debugger->getCurrentLine();
    highlightDebugLine(debugLine);
    updateStatusBar("🔧 التصحيح (داخل دالة) - السطر " + std::to_string(debugLine));
}

void ArabicIDE::toggleBreakpoint() {
    if (!debugger || currentFilePath.empty()) return;

    // رقم السطر الحالي لمؤشر المحرر (0-based من EM_LINEFROMCHAR)
    DWORD selStart = 0;
    SendMessage(editControl, EM_GETSEL, (WPARAM)&selStart, 0);
    int lineNum = (int)SendMessage(editControl, EM_LINEFROMCHAR, selStart, 0) + 1;

    // تحقق إن كانت نقطة التوقف موجودة فعلاً
    Breakpoint* existing = debugger->findBreakpoint(currentFilePath, lineNum);
    if (existing) {
        debugger->removeBreakpoint(existing->id);
        breakpointLines.erase(lineNum);
        addOutput(OutputMessage(OutputMessage::INFO,
            "⬜ أُزيلت نقطة التوقف من السطر " + std::to_string(lineNum)));
    } else {
        debugger->addBreakpoint(currentFilePath, lineNum);
        breakpointLines.insert(lineNum);
        addOutput(OutputMessage(OutputMessage::INFO,
            "🔴 نقطة توقف عند السطر " + std::to_string(lineNum)));
    }
}

void ArabicIDE::showBreakpoints() {
    if (!debugger) return;
    auto bps = debugger->getBreakpoints();
    if (bps.empty()) {
        addOutput(OutputMessage(OutputMessage::INFO, "📋 لا توجد نقاط توقف مضافة"));
        return;
    }
    addOutput(OutputMessage(OutputMessage::INFO,
        "📋 نقاط التوقف (" + std::to_string(bps.size()) + "):"));
    for (const auto& bp : bps) {
        std::string info = "  🔴 السطر " + std::to_string(bp.lineNumber);
        if (!bp.filePath.empty()) info += "  [" + bp.filePath + "]";
        if (!bp.condition.empty()) info += "  (شرط: " + bp.condition + ")";
        addOutput(OutputMessage(OutputMessage::INFO, info));
    }
}

void ArabicIDE::highlightDebugLine(int line) {
    if (!editControl) return;

    // أعد لون كل النص للوضع الافتراضي أولاً
    CHARRANGE all = {0, -1};
    SendMessage(editControl, EM_EXSETSEL, 0, (LPARAM)&all);
    CHARFORMAT2W cfReset = {0};
    cfReset.cbSize      = sizeof(CHARFORMAT2W);
    cfReset.dwMask      = CFM_BACKCOLOR;
    cfReset.crBackColor = RGB(30, 30, 30); // لون الخلفية الداكن
    SendMessage(editControl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfReset);

    if (line < 1) return; // -1 = أزل التمييز فقط

    // احسب نطاق السطر المطلوب
    long lineStart = (long)SendMessage(editControl, EM_LINEINDEX, (WPARAM)(line - 1), 0);
    long lineEnd   = (long)SendMessage(editControl, EM_LINEINDEX, (WPARAM)(line),     0);
    if (lineEnd < 0) lineEnd = GetWindowTextLengthW(editControl);

    CHARRANGE cr = {lineStart, lineEnd};
    SendMessage(editControl, EM_EXSETSEL, 0, (LPARAM)&cr);
    CHARFORMAT2W cfHL = {0};
    cfHL.cbSize      = sizeof(CHARFORMAT2W);
    cfHL.dwMask      = CFM_BACKCOLOR;
    cfHL.crBackColor = RGB(80, 60, 0); // أصفر داكن = سطر التصحيح الحالي
    SendMessage(editControl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfHL);

    // أعد المؤشر للسطر
    SendMessage(editControl, EM_SETSEL, lineStart, lineStart);
    SendMessage(editControl, EM_SCROLLCARET, 0, 0);
}

// ════════════════════════════════════════════════════════════
// ✅ مدير الحزم
// ════════════════════════════════════════════════════════════

void ArabicIDE::showPackageManager() {
    if (!packageManager) {
        addOutput(OutputMessage(OutputMessage::ERROR_MSG, "❌ مدير الحزم غير مهيأ"));
        return;
    }

    auto installed = packageManager->listInstalledPackages();
    std::string info = "📦 الحزم المثبتة (" + std::to_string(installed.size()) + "):\n";
    for (const auto& pkg : installed) {
        info += "  • " + pkg.name + " v" + pkg.version.toString() + "\n";
    }
    if (installed.empty()) info += "  (لا توجد حزم مثبتة)\n";
    info += "\n💡 لتثبيت حزمة استخدم: مدير الحزم > تثبيت";
    addOutput(OutputMessage(OutputMessage::INFO, info));

    std::wstring dlgMsg = L"📦 مدير الحزم\n\nالحزم المثبتة: ";
    dlgMsg += std::to_wstring(installed.size());
    dlgMsg += L"\n\nواجهة مدير الحزم الكاملة قيد التطوير.\nاستخدم لوحة المخرجات لرؤية الحزم المثبتة.";
    MessageBoxW(mainWindow, dlgMsg.c_str(), L"مدير الحزم", MB_OK | MB_ICONINFORMATION);
}

// ════════════════════════════════════════════════════════════
// ✅ JIT
// ════════════════════════════════════════════════════════════

void ArabicIDE::toggleJIT() {
    useJIT = !useJIT;
    std::string status = useJIT ? "⚡ JIT مُفعَّل - الكود سيُشغَّل بتحسين JIT"
                                : "🐢 JIT مُعطَّل - الكود سيُشغَّل كمُفسِّر";
    addOutput(OutputMessage(OutputMessage::INFO, status));
    updateStatusBar(status);
}

bool ArabicIDE::runWithJIT(const std::string& code) {
    if (code.empty()) return false;

    addOutput(OutputMessage(OutputMessage::INFO, "⚡ جاري التجميع والتنفيذ بـ JIT..."));

    try {
        auto& jit = ArabicJITCompiler::getInstance();
        const std::string fname = currentFilePath.empty() ? "stdin" : currentFilePath;

        // ترجمة إلى bytecode
        auto bytecode = jit.compileToBytecode(code, fname);
        if (!bytecode) {
            addOutput(OutputMessage(OutputMessage::ERROR_MSG, "❌ فشل تجميع JIT"));
            return false;
        }

        addOutput(OutputMessage(OutputMessage::INFO,
            "⚡ JIT: تم التجميع - " + std::to_string(bytecode->code.size()) + " تعليمة bytecode"));

        // تحسين ثم تنفيذ
        jit.optimizeBytecode(bytecode);
        jit.executeBytecode(bytecode);

        addOutput(OutputMessage(OutputMessage::SUCCESS, "✅ اكتمل التنفيذ بـ JIT بنجاح"));
        return true;
    } catch (const std::exception& e) {
        addOutput(OutputMessage(OutputMessage::ERROR_MSG,
            std::string("❌ خطأ JIT: ") + e.what()));
        return false;
    }
}

} // namespace ArabicLanguage