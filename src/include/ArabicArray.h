// ArabicArray.h - Public interface for array operations
#ifndef ARABIC_ARRAY_H
#define ARABIC_ARRAY_H

#include "../runtime/ArabicTypes.h"
#include "../include/ArabicArrayInternal.h"
#include <vector>
#include <string>

namespace ArabicLanguage {
    
    class ArabicRuntime;
    
    // ═══════════════════════════════════════════════════════════
    // Array Operations
    // ═══════════════════════════════════════════════════════════
    
    // Create a new array with initial values
    // إنشاء مصفوفة جديدة مع القيم الأولية
    Value generateArrayAssignment(
        const std::string& arrayName,
        const std::vector<Value>& initialValues,
        ArabicRuntime* runtime = nullptr
    );
    
    // Get the length of an array
    // الحصول على طول المصفوفة
    Value arrayLength(const Value& arrayValue, ArabicRuntime* runtime = nullptr);
    
    // Add an element to an array
    // إضافة عنصر إلى المصفوفة
    Value arrayAdd(Value& arrayValue, const Value& newElement, ArabicRuntime* runtime = nullptr);
    
    // Get the capacity of an array
    // الحصول على سعة المصفوفة
    Value arrayCapacity(const Value& arrayValue, ArabicRuntime* runtime = nullptr);
    
    // Remove an element from an array by index
    // إزالة عنصر من المصفوفة حسب الفهرس
    Value arrayRemove(Value& arrayValue, const Value& indexValue, ArabicRuntime* runtime = nullptr);
    
    // Insert an element at a specific index
    // إدراج عنصر في موضع محدد
    Value arrayInsert(Value& arrayValue, const Value& indexValue, const Value& newElement, ArabicRuntime* runtime = nullptr);
    
    // Print memory statistics for arrays
    // طباعة إحصائيات الذاكرة للمصفوفات
    void printArrayMemoryStats();

} // namespace ArabicLanguage

#endif // ARABIC_ARRAY_H
