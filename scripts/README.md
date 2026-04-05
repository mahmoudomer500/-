# دليل سكريبتات البناء - Scripts Documentation

## الملفات المتبقية في مجلد `scripts/`

### 1. `build_full.bat` (Windows)
**الوظيفة:** بناء المترجم الكامل (61 ملف C++)
**المخرجات:** `../arabic_v18_final.exe`
**الاستخدام:** انقر مزدوج أو `.\build_full.bat`
```
يبني المترجم مع كل الوحدات (AI, Graphics, UI, Web, Mobile, Edu, Enterprise)
```

### 2. `build_and_test.bat` (Windows)
**الوظيفة:** بناء + اختبار شامل (يتطلب CMake + g++)
**الاستخدام:** `.\build_and_test.bat`
```
1. يتحقق من وجود CMake و g++
2. يبني المشروع عبر CMake
3. يبني اختبارات C++
4. يشغل الاختبارات
```

### 3. `build_and_test.sh` (Linux)
**الوظيفة:** بناء المترجم + اختبار Hello World على Linux
**الاستخدام:** `chmod +x build_and_test.sh && ./build_and_test.sh`
```
1. يتحقق من g++
2. يبني المترجم (بدون وحدات Windows-specific)
3. يختبر بملف hello.عربي
```

### 4. `build_direct.sh` (Linux)
**الوظيفة:** بناء IDE الكامل على Linux
**الاستخدام:** `chmod +x build_direct.sh && ./build_direct.sh`
```
1. يتحقق من g++
2. يبني IDE (61 ملف مع OpenGL, OpenAL)
3. المخرج: build_release/Release/AlBiruniIDE
```

### 5. `build_ide.bat` (في الجذر - Windows)
**الوظيفة:** بناء IDE البيروني الكامل
**الاستخدام:** انقر مزدوج أو `.\build_ide.bat`
```
1. يحذف الملف القديم
2. يبني IDE (61 ملف)
3. يعرض الحجم والنتيجة
```

### 6. `install.bat` (Windows)
**الوظيفة:** تثبيت المترجم وIDE في مجلد النظام
**الاستخدام:** `.\install.bat`
```
ينسخ الملفات التنفيذية إلى مجلد قابل للوصول من أي مكان
```

### 7. `setup_path.bat` (Windows)
**الوظيفة:** إضافة مسار المترجم إلى PATH
**الاستخدام:** `.\setup_path.bat`
```
يضيف مجلد build_release/Release إلى PATH النظام
```

---

## ملخص سريع

| الملف | النظام | الوظيفة |
|-------|--------|---------|
| `build_full.bat` | Windows | بناء المترجم |
| `build_and_test.bat` | Windows | بناء + اختبار (CMake) |
| `build_and_test.sh` | Linux | بناء + اختبار |
| `build_direct.sh` | Linux | بناء IDE |
| `../build_ide.bat` | Windows | بناء IDE |
| `install.bat` | Windows | تثبيت |
| `setup_path.bat` | Windows | إعداد PATH |
