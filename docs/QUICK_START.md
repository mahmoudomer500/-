# Quick Start Guide - Arabic Arrays
# دليل البدء السريع - المصفوفات العربية

## 📋 Table of Contents

1. [Installation](#installation)
2. [Basic Usage](#basic-usage)
3. [Common Operations](#common-operations)
4. [Examples](#examples)
5. [Troubleshooting](#troubleshooting)
6. [Next Steps](#next-steps)

---

## Installation

### Windows

```batch
# 1. Clone the repository
git clone <repository-url>
cd arabic-programming-language

# 2. Run the build script
scripts\build_and_test.bat

# 3. Verify installation
build\arabic_v2.exe --version
```

### Linux/macOS

```bash
# 1. Clone the repository
git clone <repository-url>
cd arabic-programming-language

# 2. Make script executable
chmod +x scripts/build_and_test.sh

# 3. Run the build script
./scripts/build_and_test.sh

# 4. Verify installation
./build/arabic_v2 --version
```

---

## Basic Usage

### Creating an Array

```عربي
# Create an array with initial values
متغير myArray = [1, 2, 3, 4, 5]

# Create an empty array
متغير emptyArray = []

# Create an array with different types
متغير mixed = [1, "نص", صحيح, 3.14]
```

### Getting Array Length

```عربي
متغير arr = [10, 20, 30]
متغير len = arr.طول()
اطبع("الطول: " + len)  # Output: الطول: 3
```

### Adding Elements

```عربي
متغير arr = [1, 2, 3]
arr.أضف(4)
arr.أضف(5)
اطبع(arr.طول())  # Output: 5
```

### Accessing Elements

```عربي
متغير arr = [10, 20, 30, 40, 50]

# Access by index
اطبع(arr[0])     # Output: 10
اطبع(arr[2])     # Output: 30
اطبع(arr[4])     # Output: 50
```

### Iterating Through Array

```عربي
متغير arr = ["أحمد", "فاطمة", "محمد"]

لأجل (i = 0; i < arr.طول(); i = i + 1) {
    اطبع(arr[i])
}
```

---

## Common Operations

### Operation Reference

| Arabic | English | Example |
|--------|---------|---------|
| `طول()` | length | `arr.طول()` |
| `أضف()` | add | `arr.أضف(5)` |
| `سعة()` | capacity | `arr.سعة()` |
| `أزل()` | remove | `arr.أزل(0)` |
| `أدخل()` | insert | `arr.أدخل(1, 99)` |

### Detailed Examples

#### 1. Get Array Length

```عربي
متغير scores = [95, 87, 92, 88, 91]
اطبع("عدد الدرجات: " + scores.طول())
# Output: عدد الدرجات: 5
```

#### 2. Add Multiple Elements

```عربي
متغير queue = []
queue.أضف("أحمد")
queue.أضف("فاطمة")
queue.أضف("محمد")
اطبع("في الطابور: " + queue.طول() + " أشخاص")
```

#### 3. Remove Element

```عربي
متغير items = [1, 2, 3, 4, 5]
items.أزل(2)      # Remove element at index 2 (value 3)
# items is now [1, 2, 4, 5]
```

#### 4. Insert Element

```عربي
متغير ranks = [1, 2, 4, 5]
ranks.أدخل(2, 3)  # Insert 3 at index 2
# ranks is now [1, 2, 3, 4, 5]
```

---

## Examples

### Example 1: Simple List

```عربي
# numbers.عربي
# Simple list of numbers

متغير numbers = [10, 20, 30, 40, 50]

اطبع("الأرقام:")
لأجل (i = 0; i < numbers.طول(); i = i + 1) {
    اطبع("رقم " + i + ": " + numbers[i])
}

متغير total = 0
لأجل (i = 0; i < numbers.طول(); i = i + 1) {
    total = total + numbers[i]
}

اطبع("المجموع: " + total)
```

**Run it**:
```bash
arabic_v2.exe numbers.عربي
```

**Expected Output**:
```
الأرقام:
رقم 0: 10
رقم 1: 20
رقم 2: 30
رقم 3: 40
رقم 4: 50
المجموع: 150
```

### Example 2: Dynamic Growth

```عربي
# growth_test.عربي
# Test array dynamic growth

متغير arr = []

اطبع("إضافة عناصر...")
لأجل (i = 1; i <= 10; i = i + 1) {
    arr.أضف(i * 10)
    اطبع("الطول الحالي: " + arr.طول())
}

اطبع("النتيجة النهائية:")
لأجل (i = 0; i < arr.طول(); i = i + 1) {
    اطبع(arr[i])
}
```

### Example 3: String Array

```عربي
# names.عربي
# Working with string arrays

متغير names = ["أحمد", "فاطمة", "محمد", "نور"]

اطبع("عدد الأسماء: " + names.طول())

اطبع("قائمة الأسماء:")
لأجل (i = 0; i < names.طول(); i = i + 1) {
    اطبع((i + 1) + ". " + names[i])
}

# Add more names
names.أضف("سارة")
names.أضف("علي")

اطبع("العدد الجديد: " + names.طول())
```

### Example 4: Array of Arrays (Nested)

```عربي
# matrix.عربي
# Simple 2D array

متغير matrix = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
]

اطبع("عرض المصفوفة:")
لأجل (row = 0; row < matrix.طول(); row = row + 1) {
    لأجل (col = 0; col < matrix[row].طول(); col = col + 1) {
        اطبع(matrix[row][col])
    }
}
```

---

## Troubleshooting

### Problem: "خطأ: الفهرس خارج الحدود" (Index out of bounds)

**Cause**: Trying to access an index that doesn't exist

```عربي
متغير arr = [1, 2, 3]
اطبع(arr[5])  # ❌ Error! Index 5 doesn't exist
```

**Solution**: Check array length first

```عربي
متغير arr = [1, 2, 3]
إذا (5 < arr.طول()) {
    اطبع(arr[5])
} إلا {
    اطبع("الفهرس غير موجود")
}
```

### Problem: "خطأ: المتغير ليس مصفوفة" (Variable is not an array)

**Cause**: Trying to use array methods on non-array

```عربي
متغير x = 5
x.أضف(10)  # ❌ Error! x is a number, not an array
```

**Solution**: Make sure you're using an array

```عربي
متغير arr = [5]
arr.أضف(10)  # ✅ OK
```

### Problem: Array operations are slow

**Cause**: Inefficient loops or large allocations

**Solution**: Use `سعة()` to preallocate

```عربي
متغير arr = []

# Bad: Grows repeatedly
لأجل (i = 1; i <= 10000; i = i + 1) {
    arr.أضف(i)
}

# Consider alternative approaches for large datasets
```

---

## Performance Tips

### 1. Use Appropriate Initial Size

```عربي
# If you know you need many elements, consider:
متغير arr = []
# Then add elements as needed
```

### 2. Avoid Unnecessary Operations

```عربي
# Good: Direct access
متغير value = arr[5]

# Less efficient: Frequent length checks
لأجل (i = 0; i < arr.طول(); i = i + 1) {
    اطبع(arr[i])
}
```

### 3. Reuse Variables

```عربي
# Good: Store length once
متغير len = arr.طول()
لأجل (i = 0; i < len; i = i + 1) {
    اطبع(arr[i])
}

# Less efficient: Check length each iteration
لأجل (i = 0; i < arr.طول(); i = i + 1) {
    اطبع(arr[i])
}
```

---

## Next Steps

### Learn More

1. **Read the [Array Implementation Docs](../docs/ARRAY_IMPLEMENTATION.md)**
   - Detailed technical information
   - 16-byte header structure
   - Memory management details

2. **Explore [Performance Guide](../docs/PERFORMANCE_TESTING_GUIDE.md)**
   - How to benchmark your code
   - Memory profiling techniques
   - Optimization strategies

3. **Check the [Roadmap](../docs/ROADMAP.md)**
   - Planned features
   - Enhancement timeline
   - Future capabilities

### Practice Exercises

1. **Exercise 1**: Create an array of numbers 1-10 and print them
2. **Exercise 2**: Create an array and double each element
3. **Exercise 3**: Implement a simple search function
4. **Exercise 4**: Build a calculator that stores operations in an array

### Advanced Topics

- [ ] Array slicing (when available)
- [ ] Functional operations (map, filter, reduce)
- [ ] Multi-dimensional arrays
- [ ] Array serialization
- [ ] Performance optimization

---

## Common Questions

**Q: What's the maximum array size?**
A: Limited by available memory. Typically millions of elements.

**Q: Can I store different types in one array?**
A: Yes! Use mixed-type arrays: `[1, "text", true]`

**Q: How do I clear an array?**
A: Reassign it: `arr = []`

**Q: Is array access O(1)?**
A: Yes, direct indexing is constant time.

**Q: Do arrays support negative indices?**
A: Not yet. Use `arr[arr.طول() - 1]` for last element.

---

## Getting Help

- **Documentation**: See `docs/` directory
- **Examples**: Check `examples/` directory
- **Issues**: Report bugs on GitHub
- **Community**: Join our Discord/Forum

---

**Last Updated**: January 2024
**Version**: 1.0
**Difficulty**: Beginner Friendly
