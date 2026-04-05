# Array Implementation Documentation
# وثائق تنفيذ المصفوفات - اللغة العربية البرمجية

## نظرة عامة (Overview)

هذا المستند يصف بنية وتنفيذ المصفوفات الديناميكية في لغة البرمجة العربية. المصفوفات تستخدم رأس بحجم 16 بايت لتخزين البيانات الوصفية (metadata).

---

## 1. بنية الذاكرة (Memory Layout)

### 1.1 رأس المصفوفة (16 bytes)

```
┌─────────────────────────────────────────────────────────┐
│                    Array Header (16 bytes)              │
├──────────────┬──────────────┬────┬────┬────┬────────────┤
│   Length     │  Capacity    │Type│Dyn │RO  │ Reserved   │
│   (4 bytes)  │  (4 bytes)   │(1) │(1) │(1) │  (3 bytes) │
├──────────────┼──────────────┼────┼────┼────┼────────────┤
│ uint32_t     │  uint32_t    │u8  │u8  │u8  │   uint8_t  │
└──────────────┴──────────────┴────┴────┴────┴────────────┘

Memory Layout:
┌─────────────────────┐
│  Array Header       │  16 bytes
│  (metadata)         │
├─────────────────────┤
│  Element [0]        │  Variable size
│  Element [1]        │
│  ...                │
│  Element [N-1]      │
├─────────────────────┤
│  Unallocated space  │  For future growth
│  (capacity - length)│
└─────────────────────┘
```

### 1.2 حقول الرأس (Header Fields)

| Field Name | Size | Type | Description |
|-----------|------|------|-------------|
| `length` | 4 bytes | uint32_t | الطول الفعلي للمصفوفة (عدد العناصر المستخدمة) |
| `capacity` | 4 bytes | uint32_t | السعة المخصصة (عدد العناصر التي يمكن تخزينها) |
| `element_type` | 1 byte | uint8_t | نوع العناصر (0=رقم، 1=نص، 2=منطقي، 3=مصفوفة، 4=مختلط) |
| `is_dynamic` | 1 byte | uint8_t | علم يشير إلى أن المصفوفة ديناميكية |
| `is_readonly` | 1 byte | uint8_t | علم الحماية من الكتابة |
| `reserved` | 3 bytes | uint8_t[3] | حقول محجوزة للمستقبل |

---

## 2. العمليات الأساسية (Core Operations)

### 2.1 إنشاء مصفوفة (generateArrayAssignment)

```cpp
Value generateArrayAssignment(
    const std::string& arrayName,
    const std::vector<Value>& initialValues,
    ArabicRuntime* runtime = nullptr
);
```

**الوصف**: إنشاء مصفوفة جديدة مع قيم أولية

**المعاملات**:
- `arrayName`: اسم المصفوفة
- `initialValues`: القيم الأولية
- `runtime`: مؤشر إلى بيئة التشغيل

**الإرجاع**: كائن `Value` من النوع `ARRAY`

**مثال**:
```عربي
متغير arr = [1, 2, 3, 4, 5]
```

### 2.2 الحصول على الطول (طول - length)

```cpp
Value arrayLength(const Value& arrayValue, ArabicRuntime* runtime = nullptr);
```

**الوصف**: الحصول على عدد العناصر الفعلي في المصفوفة

**المثال**:
```عربي
متغير arr = [1, 2, 3]
متغير len = arr.طول()  # يرجع 3
```

### 2.3 إضافة عنصر (أضف - add)

```cpp
Value arrayAdd(Value& arrayValue, const Value& newElement, ArabicRuntime* runtime = nullptr);
```

**الوصف**: إضافة عنصر جديد إلى نهاية المصفوفة

**السلوك**:
- إذا كان `length < capacity`: أضف العنصر مباشرة
- إذا كان `length == capacity`: أعد تخصيص الذاكرة بسعة جديدة

**المثال**:
```عربي
متغير arr = [1, 2]
arr.أضف(3)
arr.أضف(4)  # arr الآن يحتوي على [1, 2, 3, 4]
```

### 2.4 الحصول على السعة (سعة - capacity)

```cpp
Value arrayCapacity(const Value& arrayValue, ArabicRuntime* runtime = nullptr);
```

**الوصف**: الحصول على إجمالي السعة المخصصة

**المثال**:
```عربي
متغير arr = [1, 2]  # قد تكون السعة 4 مثلاً
اطبع(arr.سعة())  # يطبع السعة المخصصة
```

### 2.5 إزالة عنصر (أزل - remove)

```cpp
Value arrayRemove(Value& arrayValue, const Value& indexValue, ArabicRuntime* runtime = nullptr);
```

**الوصف**: حذف عنصر من المصفوفة حسب الفهرس

**معالجة الأخطاء**:
- خطأ إذا كان الفهرس خارج الحدود
- خطأ إذا كانت المصفوفة فارغة

**المثال**:
```عربي
متغير arr = [1, 2, 3, 4, 5]
arr.أزل(2)  # إزالة العنصر في الموضع 2 (القيمة 3)
# arr الآن [1, 2, 4, 5]
```

### 2.6 إدراج عنصر (أدخل - insert)

```cpp
Value arrayInsert(Value& arrayValue, const Value& indexValue, const Value& newElement, ArabicRuntime* runtime = nullptr);
```

**الوصف**: إدراج عنصر في موضع محدد

**المثال**:
```عربي
متغير arr = [1, 2, 4, 5]
arr.أدخل(2, 3)  # إدراج 3 في الموضع 2
# arr الآن [1, 2, 3, 4, 5]
```

---

## 3. إعادة التخصيص الديناميكي (Dynamic Resizing)

### 3.1 معامل النمو (Growth Factor)

عند الحاجة إلى توسيع المصفوفة:

```
السعة الجديدة = السعة الحالية × 1.5
```

**الفائدة**: توازن بين استخدام الذاكرة والأداء

### 3.2 عملية الإعادة (Reallocation Process)

```
1. تخصيص ذاكرة جديدة بحجم أكبر
2. نسخ جميع العناصر من الذاكرة القديمة
3. تحديث رأس المصفوفة الجديد
4. تحرير الذاكرة القديمة
5. تحديث المؤشر للإشارة إلى الذاكرة الجديدة
```

---

## 4. معالجة الأخطاء (Error Handling)

### 4.1 أخطاء الفهرس خارج الحدود (Out-of-Bounds)

```عربي
متغير arr = [1, 2, 3]
# محاولة الوصول إلى arr[5] -> خطأ
# محاولة الإزالة من موضع 10 -> خطأ
```

### 4.2 الفهارس السالبة (Negative Indices)

في الإصدار الحالي، الفهارس السالبة غير مدعومة:

```عربي
arr[-1]  # غير مدعوم (لا يرجع الحد الأخير كما في Python)
```

### 4.3 أنواع مختلطة (Mixed Types)

المصفوفات تدعم العناصر من أنواع مختلفة:

```عربي
متغير mixed = [1, "نص", صحيح, 3.14]
```

---

## 5. الاختبار (Testing)

### 5.1 اختبارات الوحدة (Unit Tests)

تشغيل الاختبارات:

```bash
g++ -O2 -std=c++17 -o test_array_operations.exe test/test_array_operations.cpp
./test_array_operations.exe
```

### 5.2 اختبارات المترجم (Interpreter Tests)

```bash
# اختبار بسيط
arabic_v2.exe test_simple_array.عربي

# اختبار متقدم
arabic_v2.exe examples/test_arrays_advanced.عربي
```

### 5.3 اختبارات الذاكرة (Memory Tests)

```bash
# على Linux مع Valgrind
./scripts/test_memory.sh

# أو يدويًا
valgrind --leak-check=full ./build/arabic_v2.exe test_simple_array.عربي
```

---

## 6. حالات الاستخدام (Use Cases)

### 6.1 معالجة البيانات

```عربي
متغير data = []
لأجل (i = 1; i <= 100; i = i + 1) {
    data.أضف(i * i)
}
```

### 6.2 تجميع النتائج

```عربي
دالة جمع_أرقام(القائمة) {
    متغير total = 0
    لأجل (i = 0; i < القائمة.طول(); i = i + 1) {
        total = total + القائمة[i]
    }
    أرجع total
}
```

### 6.3 معالجة النصوص

```عربي
متغير names = ["أحمد", "فاطمة", "محمد"]
لأجل (i = 0; i < names.طول(); i = i + 1) {
    اطبع(names[i])
}
```

---

## 7. معايير الأداء (Performance Guidelines)

| العملية | التعقيد الزمني | الملاحظات |
|--------|---------------|---------|
| `طول()` | O(1) | وصول مباشر للرأس |
| `أضف()` | O(1) amortized | قد يكون O(n) عند إعادة التخصيص |
| `أزل()` | O(n) | تحتاج إلى تحريك العناصر |
| `أدخل()` | O(n) | تحتاج إلى تحريك العناصر |
| `سعة()` | O(1) | وصول مباشر للرأس |

---

## 8. المستقبل (Future Enhancements)

- [ ] دعم الفهارس السالبة (Python-style)
- [ ] iterators للحلقات المتقدمة
- [ ] دوال مرتبة وتصفية (sort, filter)
- [ ] خريطة (map) ومجموعة (set)
- [ ] متوازي (parallel) processing
- [ ] ضغط البيانات (compression)
- [ ] serialize/deserialize للملفات

---

## 9. قائمة الاختبار (Test Checklist)

- [x] إنشاء مصفوفة فارغة
- [x] إنشاء مصفوفة بقيم أولية
- [x] إضافة عناصر
- [x] الحصول على الطول
- [x] إعادة التخصيص عند تجاوز السعة
- [x] إزالة عناصر
- [x] إدراج عناصر
- [x] معالجة الأخطاء (خارج الحدود)
- [x] معالجة الفهارس السالبة
- [x] الأنواع المختلطة
- [x] اختبارات الذاكرة
- [x] معايير الأداء

---

## 10. المراجع والموارد

- [C++ STL Vector Documentation](https://en.cppreference.com/w/cpp/container/vector)
- [Memory Management Best Practices](https://www.cprogramming.com/tutorial/memory_management.html)
- [Arabic Unicode Support](https://www.unicode.org/charts/PDF/U0600.pdf)

---

**آخر تحديث**: 2024
**الإصدار**: 2.0
**حالة التطوير**: نشط (Active)
