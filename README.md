# لغة البرمجة العربية 🌍

<div align="center">

![Version](https://img.shields.io/badge/Version-1.0.0--beta-blue)
![License](https://img.shields.io/badge/License-MIT-green)
![Platform](https://img.shields.io/badge/Platform-Windows%2064--bit-lightgrey)
![C++](https://img.shields.io/badge/C%2B%2B-17-orange)

**مترجم لغة برمجة عربية كاملة مع بيئة تطوير متكاملة (IDE)**

[English](#english) | [العربية](#-لغة-البرمجة-العربية)

</div>

---

## 🎯 نظرة عامة

لغة البرمجة العربية هي لغة برمجة عالية المستوى مُصممة خصيصاً للناطقين بالعربية. تتيح لك كتابة برامج كاملة بالعربية، من المتغيرات والدوال إلى الصفوف والوراثة.

### ✨ الميزات الرئيسية

- 🔤 **برمجة بالكامل بالعربية** — جميع الكلمات المفتاحية والدوال بالعربية
- 🚀 **ذاتية التجميع (Self-Hosting)** — المترجم يكتب بنفسه بالعربية!
- 🎯 **توليد كود الآلة** — يُنتج ملفات تنفيذية مباشرة (PE64 لـ Windows)
- 🏗️ **OOP كامل** — صفوف، وراثة، واجهات، دوال مجهولة
- 📊 **مكتبة قياسية** — مصفوفات، نصوص، ملفات، شبكات
- 🌐 **دعم RTL** — مصممة للغة العربية من اليمين لليسار
- 🖥️ **IDE متكامل** — بيئة تطوير البيروني مع تلوين الصيغة وتحليل الأخطاء

---

## 📥 البناء من المصدر

### المتطلبات

- **Windows** 10/11 (64-bit)
- **GCC 10+** (MinGW64) أو **MSVC 2019+**
- **CMake** (اختياري — للبناء المتقدم)

### كيفية فتح المجلد في سطر الأوامر

#### الطريقة 1: من مستكشف الملفات (الأسهل)
1. افتح مجلد المشروع في مستكشف الملفات
2. اكتب `cmd` في شريط العنوان بالأعلى واضغط Enter
3. سيفتح سطر الأوامر في نفس المجلد

#### الطريقة 2: من PowerShell
1. افتح مجلد المشروع في مستكشف الملفات
2. اكتب `powershell` في شريط العنوان بالأعلى واضغط Enter
3. سيفتح PowerShell في نفس المجلد

#### الطريقة 3: انقر بزر الماوس الأيمن
1. انقر بزر الماوس الأيمن على خلفية المجلد (ليس على ملف)
2. اختر "Open in Terminal" أو "فتح في الطرفية"

### الطريقة السريعة (g++)

#### بناء المترجم:
```cmd
scripts\build_full.bat
```

#### بناء IDE:
```cmd
build_ide.bat
```
أو مباشرة من سطر الأوامر:
```cmd
g++ @build_ide.rsp
```
> **ملاحظة:** `build_ide.rsp` هو ملف استجابة يحتوي على كل إعدادات البناء.
> يعمل من cmd و PowerShell.

### البناء باستخدام CMake

```cmd
build_with_cmake.bat
```

أو يدويًا:
```bash
cmake -S . -B build_release
cmake --build build_release --config Release
```

---

## 🚀 البدء السريع

### أول برنامج

أنشئ ملف `مرحبا.عربي`:

```arabic
اطبع("مرحباً بالعالم!")
```

ثم شغّله:

```bash
build_release\Release\arabic_compiler.exe --mode run مرحبا.عربي
```

### تجميع إلى ملف تنفيذي

```bash
build_release\Release\arabic_compiler.exe --mode compiler مرحبا.عربي مرحبا
# ينتج: مرحبا.exe
```

### البناء الذاتي (Self-Hosting)

```bash
build_release\Release\arabic_compiler.exe --mode selfhost
```

---

## 📚 أساسيات اللغة

### المتغيرات

```arabic
مت اسم = "أحمد"          # نص
مت العمر = 25             # رقم
مت طالب = صحيح            # منطقي
مت ارقام = [1، 2، 3]      # مصفوفة
```

### الشروط

```arabic
دالة فحص_العمر(عمر):
    اذا عمر >= 18:
        اطبع("بالغ")
    والا اذا عمر >= 12:
        اطبع("مراهق")
    والا:
        اطبع("طفل")
    نهاية
نهاية
```

### الحلقات

```arabic
# حلقة while
مت ع = 0
بينما ع < 5:
    اطبع(ع)
    ع = ع + 1
نهاية

# حلقة for
لكل رقم في [1، 2، 3، 4، 5]:
    اطبع(رقم)
نهاية
```

### الدوال

```arabic
دالة جمع(أ، ب):
    أرجع أ + ب
نهاية

دالة مرحبا(اسم = "عالم"):
    اطبع("مرحباً يا " + اسم)
نهاية
```

### الصفوف (OOP)

```arabic
صف نقطة:
    خاص:
        رقم س
        رقم ص

    عام:
        دالة جديد(س، ص):
            هذا.س = س
            هذا.ص = ص

        دالة المسافة():
            أرجع جذر(هذا.س * هذا.س + هذا.ص * هذا.ص)
نهاية

# استخدام
مت ن1 = جديد نقطة(3، 4)
مت ن2 = جديد نقطة(0، 0)
اطبع(ن1.المسافة())
```

---

## 📁 هيكل المشروع

```
Arabic-Programming-Language/
├── src/
│   ├── main.cpp              # نقطة الدخول الرئيسية
│   ├── core/                 # جوهر المترجم (Lexer, Parser, CodeGenerator, Executor)
│   ├── runtime/              # بيئة التشغيل
│   ├── include/              # مكتبات خارجية (GLM, OpenGL, ...)
│   ├── libraries/            # مكتبات عربية C++
│   ├── modules/              # وحدات متخصصة (AI, Web, Graphics, UI, ...)
│   ├── stdlib/               # المكتبة القياسية C++
│   ├── api/                  # واجهات API
│   ├── utils/                # أدوات مساعدة (Cache, Profiler, Backup)
│   └── ide/                  # بيئة التطوير (ArabicIDE, ide_main)
├── examples/                 # أمثلة اللغة
│   ├── basic/                # أمثلة أساسية (hello, variables, loops...)
│   ├── advanced/             # أمثلة متقدمة (OOP, files, graphics...)
│   └── tests/                # اختبارات
├── docs/                     # التوثيق التقني
├── test/                     # اختبارات الوحدات (C++)
├── scripts/                  # سكريبتات البناء
│   ├── build_full.bat        # بناء المترجم (g++)
│   ├── build_and_test.bat    # بناء + اختبار (CMake)
│   ├── build_and_test.sh     # بناء + اختبار (Linux)
│   ├── build_direct.sh       # بناء IDE (Linux)
│   ├── install.bat           # تثبيت
│   └── setup_path.bat        # إعداد PATH
├── stdlib/                   # المكتبة القياسية العربية (.عربي)
├── tools/                    # أدوات المطور
├── اللغة_العربية/            # ملفات اللغة الأساسية (للبناء الذاتي)
├── المرحلة_الخامسة/          # مرحلة التطوير الخامسة
├── build_release/Release/    # ناتج البناء
│   ├── arabic_compiler.exe   # المترجم
│   └── AlBiruniIDE.exe    # بيئة التطوير
├── build_ide.bat             # بناء IDE (g++)
├── build_with_cmake.bat      # بناء كامل (CMake)
├── CMakeLists.txt            # نظام البناء
├── CMakePresets.json         # إعدادات CMake
├── README.md                 # هذا الملف
├── LANGUAGE_SPEC.md          # مواصفات اللغة
├── CHANGELOG.md              # سجل التغييرات
├── CONTRIBUTING.md           # دليل المساهمة
└── LICENSE                   # الرخصة
```

---

## 🔧 التجميع الذاتي (Self-Hosting)

واحدة من أهم ميزات هذه اللغة هي قدرتها على تجميع نفسها!

### المكونات الذاتية

| المكون | الملف | الوصف | الحالة |
|--------|-------|-------|--------|
| المحلل اللغوي | `محلل_لغوي.عربي` | يحول الكود إلى رموز | ✅ يُنتج EXE |
| المحلل النحوي | `محلل_نحوي.عربي` | يحول الرموز إلى شجرة تحليل | ⚠️ تفسير |
| مولد الكود | `مولد_كود.عربي` | يولّد كود الآلة | ✅ يُنتج EXE |
| المترجم الكامل | `SelfHostingCompiler.arabic` | يدمج جميع المكونات | ⚠️ تفسير |

### تشغيل البناء الذاتي

```bash
build_release\Release\arabic_compiler.exe --mode selfhost
```

---

## 📊 الكلمات المفتاحية

| الكلمة | الوصف |
|--------|-------|
| `مت` | تعريف متغير |
| `دالة` | تعريف دالة |
| `صف` | تعريف صف |
| `اذا` | شرط if |
| `بينما` | حلقة while |
| `لكل...في` | حلقة for-each |
| `أرجع` | إرجاع قيمة |
| `جديد` | إنشاء كائن |
| `اطبع` | طباعة |
| `هذا` | الإشارة للكائن الحالي (this) |
| `عام` | وصول عام (public) |
| `خاص` | وصول خاص (private) |
| `محمي` | وصول محمي (protected) |
| `صحيح` | قيمة منطقية true |
| `خطأ` | قيمة منطقية false |
| `لا_شيء` | قيمة فارغة (null/none) |
| `استورد` | استيراد وحدة |
| `حاول` / `امسك` | معالجة الأخطاء (try/catch) |

---

## 🖥️ بيئة التطوير (Al-Biruni IDE)

بيئة تطوير متكاملة مبنية بالكامل:

| الميزة | الوصف |
|--------|-------|
| 🎨 تلوين الصيغة | تلوين تلقائي للكلمات المفتاحية والدوال والنصوص والأرقام |
| 🔍 تحليل الأخطاء | اكتشاف مشاكل الصيغة أثناء الكتابة |
| 📂 أمثلة جاهزة | 10+ مثال جاهز (مرحباً، متغيرات، شروط، حلقات، دوال، OOP...) |
| 📋 شريط قوائم | ملف، تشغيل، تحليل، أمثلة، مساعدة |
| ⌨️ اختصارات | Ctrl+N جديد، Ctrl+O فتح، Ctrl+S حفظ، F5 تشغيل، F7 تجميع |
| 🔧 تنسيق الكود | إصلاح المسافات البادئة تلقائياً |
| 🔍 فحص ذاتي | اختبار البناء الذاتي من داخل IDE |

### تشغيل IDE:
```bash
build_release\Release\AlBiruniIDE.exe
```

---

## 🧪 الاختبارات

```bash
# تشغيل ملف عربي
build_release\Release\arabic_compiler.exe --mode run examples/basic/hello.عربي

# تجميع ملف عربي إلى EXE
build_release\Release\arabic_compiler.exe --mode compiler examples/basic/hello.عربي hello

# البناء الذاتي
build_release\Release\arabic_compiler.exe --mode selfhost
```

---

## 🤝 المساهمة

نرحب بمساهماتكم! يرجى قراءة [CONTRIBUTING.md](CONTRIBUTING.md) للمزيد.

### خطوات المساهمة

1. Fork المشروع
2. أنشئ فرع للميزة (`git checkout -b feature/amazing-feature`)
3. Commit التغييرات (`git commit -m 'إضافة ميزة رائعة'`)
4. Push للفرع (`git push origin feature/amazing-feature`)
5. افتح Pull Request

---

## 📝 خارطة الطريق

- [x] **IDE متكامل** — بيئة تطوير البيروني ✅
- [ ] **v1.0.0** — إصدار رسمي مستقر
- [ ] **v1.1.0** — تحسين البناء الذاتي الكامل
- [ ] **v2.0.0** — مترجم ذاتي كامل (جميع المراحل تُنتج EXE)
- [ ] **Debugger** — مُصحح أخطاء تفاعلي
- [ ] **Package Manager** — مدير حزم
- [ ] **JIT Compilation** — ترجمة فورية

---

## ❓ الأسئلة الشائعة

**س: هل يمكنني استخدام اللغة على Linux؟**
ج: الكود المصدري قابل للبناء على Linux. استخدم `scripts/build_and_test.sh`. الملفات التنفيذية PE64 مخصصة لـ Windows.

**س: هل اللغة مجانية؟**
ج: نعم، تحت رخصة MIT.

**س: ما هو البناء الذاتي (Self-Hosting)؟**
ج: يعني أن المترجم مكتوب بلغته العربية نفسها، فيمكنه تجميع كوده المصدري.

**س: ما الفرق بين `build_ide.bat` و `build_with_cmake.bat`؟**
ج: `build_ide.bat` يبني IDE فقط باستخدام g++ (أسرع). `build_with_cmake.bat` يبني المترجم + IDE باستخدام CMake (يحتاج CMake مثبت).

---

## 📄 الرخصة

هذا المشروع مرخص تحت [MIT License](LICENSE).

---

## 🙏 شكر خاص

- مجتمع المطورين العرب
- مشروع GCC
- جميع المساهمين

---

<div align="center">

**صُنع بـ ❤️ للمطورين العرب**

</div>

---

## English

### Arabic Programming Language

A high-level programming language designed specifically for Arabic speakers. Write complete programs in Arabic — from variables and functions to classes and inheritance.

**Features:**
- Full Arabic syntax (keywords, variables, functions)
- Self-hosting capability (the compiler compiles itself)
- Native PE64 executable generation
- Full OOP support (classes, inheritance, interfaces)
- Standard library (arrays, strings, files, networks)
- RTL support built-in
- Full IDE with syntax highlighting, error analysis, and examples

**Quick Start:**

```bash
# Clone
git clone https://github.com/YOUR_USERNAME/Arabic-Programming-Language.git
cd Arabic-Programming-Language

# Build (requires g++ / MinGW64)
g++ @build_ide.rsp         # Build IDE
scripts\build_full.bat     # Build compiler

# Or with CMake
build_with_cmake.bat

# Run
build_release\Release\arabic_compiler.exe --mode run examples/basic/hello.عربي
```

**License:** MIT
