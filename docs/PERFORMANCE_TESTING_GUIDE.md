# Performance and Profiling Guide
# دليل الأداء والتحليل الديناميكي

## محتوى الدليل

1. [متطلبات التشغيل](#متطلبات-التشغيل)
2. [بناء المشروع](#بناء-المشروع)
3. [تشغيل الاختبارات](#تشغيل-الاختبارات)
4. [تحليل الأداء](#تحليل-الأداء)
5. [اكتشاف تسرب الذاكرة](#اكتشاف-تسرب-الذاكرة)
6. [نتائج المعايير](#نتائج-المعايير)

---

## متطلبات التشغيل

### Windows
```
- MinGW or Visual Studio (g++ or cl.exe)
- CMake 3.10+
- Git Bash (optional, for shell scripts)
```

### Linux/macOS
```
- GCC or Clang
- CMake 3.10+
- Valgrind (for memory profiling)
- perf (for performance analysis)
```

---

## بناء المشروع

### خطوة 1: استخدام CMake

```bash
# Windows
cd <project-root>
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make

# Linux
cd <project-root>
mkdir build
cd build
cmake ..
make
```

### خطوة 2: استخدام Build Script

```bash
# Windows
.\scripts\build_and_test.bat

# Linux/macOS
bash scripts/build_and_test.sh
```

### خطوة 3: التحقق من البناء

```bash
# Windows
build\arabic_v2.exe --version

# Linux
./build/arabic_v2 --version
```

---

## تشغيل الاختبارات

### اختبارات الوحدة (Unit Tests)

```bash
# بناء واختبار
test-build\test_array_operations.exe

# أو في Linux
./test-build/test_array_operations
```

**النتيجة المتوقعة**:
```
════════════════════════════════════════════════════════
   Arabic Programming Language - Array Test Suite
════════════════════════════════════════════════════════

╔════════════════════════════════════════════════╗
║ Test Suite: Array Operations Tests
╚════════════════════════════════════════════════╝
✅ PASS: Create empty array
✅ PASS: Array has 3 elements
...
✅ PASS: All tests passed
```

### اختبارات المترجم (Interpreter Tests)

```bash
# الاختبار البسيط
arabic_v2.exe test_simple_array.عربي

# اختبار متقدم
arabic_v2.exe examples/test_arrays_advanced.عربي
```

**النتائج المتوقعة**:
```
✅ تم إنشاء مصفوفة: arr (الطول: 2, السعة: 2)
✅ تم إضافة عنصر. الطول الجديد: 3
...
✅ انتهى الاختبار بنجاح
```

---

## تحليل الأداء

### القياس الأساسي (Baseline Benchmark)

```عربي
# benchmark_arrays.عربي
متغير sizes = [100, 1000, 10000, 100000]

لأجل (size_idx = 0; size_idx < sizes.طول(); size_idx = size_idx + 1) {
    متغير size = sizes[size_idx]
    متغير arr = []
    
    # القياس: الإضافة
    لأجل (i = 0; i < size; i = i + 1) {
        arr.أضف(i)
    }
    
    اطبع("حجم المصفوفة: " + size + ", الطول: " + arr.طول())
}
```

### استخراج معلومات الأداء

```bash
# تشغيل مع قياس الوقت
time arabic_v2.exe benchmark_arrays.عربي

# مع تتبع التعقيد الزمني
arabic_v2.exe --profile benchmark_arrays.عربي
```

### نتائج الأداء المتوقعة

| حجم المصفوفة | وقت الإضافة | استخدام الذاكرة |
|-----------|-----------|----------------|
| 100 | ~1ms | ~1KB |
| 1,000 | ~5ms | ~10KB |
| 10,000 | ~30ms | ~100KB |
| 100,000 | ~200ms | ~1MB |

---

## اكتشاف تسرب الذاكرة

### استخدام Valgrind (Linux)

```bash
# تثبيت (إن لم يكن مثبتاً)
apt-get install valgrind

# التشغيل مع Valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --verbose \
         ./build/arabic_v2 test_simple_array.عربي
```

**النتيجة المتوقعة**:
```
==12345== HEAP SUMMARY:
==12345==     in use at exit: 0 bytes in 0 blocks
==12345==   total heap alloc: 50,234 bytes in 1,234 blocks
==12345==   total heap freed: 50,234 bytes in 1,234 blocks
==12345==   total heap waste: 0 bytes
==12345== ERROR SUMMARY: 0 errors from 0 contexts
```

### استخدام Dr.Memory (Windows)

```batch
REM تنزيل من: http://www.drmemory.org/

drmemory -log_dir drmem_results -- build\arabic_v2.exe test_simple_array.عربي

REM عرض النتائج
type drmem_results\drmemory.txt
```

### استخدام Address Sanitizer (GCC/Clang)

```bash
# بناء مع Address Sanitizer
g++ -fsanitize=address -g -O1 \
    test/test_array_operations.cpp \
    -o test_asan

# التشغيل
./test_asan
```

---

## مراقبة الأداء أثناء التشغيل

### استخدام perf (Linux)

```bash
# تسجيل الأداء
perf record ./build/arabic_v2 test_simple_array.عربي

# عرض النتائج
perf report
```

### استخدام Profiler مدمج

أضفنا وظيفة تطبع إحصائيات الذاكرة:

```عربي
# في نهاية البرنامج
طباعة_ذاكرة()  # يطبع معلومات استخدام الذاكرة
```

---

## شاشة تحكم الجودة (QA Checklist)

### اختبارات الصحة (Sanity Tests)
- [ ] المصفوفة الفارغة تعمل
- [ ] إضافة عناصر تزيد الطول
- [ ] الفهرسة تعطي القيمة الصحيحة
- [ ] لا توجد أخطاء عند التجميع

### اختبارات الحدود (Boundary Tests)
- [ ] مصفوفة بحجم 0
- [ ] مصفوفة بعنصر واحد
- [ ] مصفوفة بـ 1000+ عنصر
- [ ] الوصول إلى الحد الأول والأخير

### اختبارات الأخطاء (Error Tests)
- [ ] فهرس سالب يعطي خطأ
- [ ] فهرس خارج الحدود يعطي خطأ
- [ ] إزالة من مصفوفة فارغة تعطي خطأ

### اختبارات الذاكرة (Memory Tests)
- [ ] لا توجد تسريبات ذاكرة
- [ ] إعادة التخصيص تحافظ على البيانات
- [ ] الحد الأقصى من الذاكرة معقول

### اختبارات الأداء (Performance Tests)
- [ ] إضافة 1000 عنصر في < 100ms
- [ ] الوصول العشوائي في O(1)
- [ ] الإزالة من الوسط تعمل بشكل صحيح

---

## استكشاف الأخطاء وإصلاحها

### المشكلة: تسرب الذاكرة

**الأعراض**:
```
ERROR SUMMARY: 100 bytes in 5 blocks are definitely lost
```

**الحل**:
1. تأكد من استدعاء `delete[]` لكل `new[]`
2. تحقق من حلقات إعادة التخصيص
3. استخدم smart pointers بدلاً من raw pointers

### المشكلة: بطء الأداء

**الأعراض**:
```
إضافة 1000 عنصر يستغرق > 1 ثانية
```

**الحل**:
1. قلل معامل النمو (من 1.5 إلى 1.25)
2. استخدم pre-allocation عند الحاجة
3. تحقق من وجود نسخ غير ضرورية

### المشكلة: فشل الاختبار

**الأعراض**:
```
❌ FAIL: Out of bounds detection works
```

**الحل**:
1. راجع منطق التحقق من الفهرس
2. أضف رسائل تصحيح (debug messages)
3. اختبر الحالات الحدية يدويًا

---

## تقارير الأداء

### إنشاء تقرير

```bash
# Linux
bash scripts/generate_performance_report.sh

# Windows
call scripts\generate_performance_report.bat
```

### محتوى التقرير

```
═════════════════════════════════════════════
  Array Implementation Performance Report
═════════════════════════════════════════════

1. UNIT TEST RESULTS
   Passed: 50/50 ✅
   Failed: 0 ❌

2. MEMORY STATISTICS
   Total Allocated: 50MB
   Peak Usage: 25MB
   Memory Leaks: None ✅

3. PERFORMANCE BENCHMARKS
   Add 1000 items: 5ms
   Remove item: 0.1ms
   Access element: 0.001ms

4. OPTIMIZATION RECOMMENDATIONS
   - Consider using reserve() for large datasets
   - Growth factor is optimal
   - No major memory inefficiencies

═════════════════════════════════════════════
```

---

## الموارد الإضافية

- [C++ Performance Best Practices](https://www.cplusplus.com/articles/sD6014pR/)
- [Valgrind Documentation](http://valgrind.org/docs/manual/quick-start.html)
- [perf Examples](https://perf.wiki.kernel.org/index.php/Main_Page)
- [Arabic Unicode Handling](https://www.unicode.org/reports/tr9/)

---

**الآخر**: يناير 2024
