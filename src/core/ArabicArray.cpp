// ArabicArray.cpp - Implementation of dynamic arrays with 16-byte header
// Implements methods: أضف (add), طول (length), and internal memory management

#include "ArabicArray.h"
#include "ArabicRuntime.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace ArabicLanguage {

    // ═══════════════════════════════════════════════════════════
    // Internal Array Manager
    // ═══════════════════════════════════════════════════════════
    
    class ArrayMemoryManager {
    public:
        static ArrayMemoryManager& getInstance() {
            static ArrayMemoryManager instance;
            return instance;
        }
        
        // تخصيص مصفوفة جديدة
        void* allocateArray(uint32_t initialCapacity = 2) {
            uint32_t capacity = std::max(initialCapacity, static_cast<uint32_t>(2));
            
            // حساب الحجم: رأس (16 بايت) + عناصر (كل عنصر = Value)
            size_t headerSize = sizeof(ArrayHeader);
            size_t elementSize = sizeof(Value);
            size_t totalSize = headerSize + (capacity * elementSize);
            
            // تخصيص الذاكرة
            uint8_t* memory = new uint8_t[totalSize];
            std::memset(memory, 0, totalSize);
            
            // إنشاء الرأس
            ArrayHeader* header = reinterpret_cast<ArrayHeader*>(memory);
            new (header) ArrayHeader();
            header->capacity = capacity;
            header->length = 0;
            header->element_type = 4; // MIXED type
            header->is_dynamic = 1;
            header->is_readonly = 0;
            
            // تتبع الذاكرة المخصصة
            totalAllocated += totalSize;
            allocationCount++;
            
            return memory;
        }
        
        // إعادة تخصيص مصفوفة موجودة
        void* reallocateArray(void* oldMemory, uint32_t newCapacity) {
            if (!oldMemory) {
                return allocateArray(newCapacity);
            }
            
            ArrayHeader* oldHeader = reinterpret_cast<ArrayHeader*>(oldMemory);
            uint32_t oldLength = oldHeader->length;
            uint32_t oldCapacity = oldHeader->capacity;
            
            // تخصيص ذاكرة جديدة
            void* newMemory = allocateArray(newCapacity);
            ArrayHeader* newHeader = reinterpret_cast<ArrayHeader*>(newMemory);
            
            // نسخ العناصر القديمة
            if (oldLength > 0) {
                Value* oldElements = reinterpret_cast<Value*>(
                    reinterpret_cast<uint8_t*>(oldMemory) + sizeof(ArrayHeader)
                );
                Value* newElements = reinterpret_cast<Value*>(
                    reinterpret_cast<uint8_t*>(newMemory) + sizeof(ArrayHeader)
                );
                
                for (uint32_t i = 0; i < oldLength; ++i) {
                    newElements[i] = oldElements[i];
                }
            }
            
            // نسخ معلومات الرأس
            newHeader->length = oldLength;
            newHeader->element_type = oldHeader->element_type;
            newHeader->is_readonly = oldHeader->is_readonly;
            
            // تحرير الذاكرة القديمة
            deallocateArray(oldMemory);
            
            return newMemory;
        }
        
        // تحرير مصفوفة
        void deallocateArray(void* memory) {
            if (!memory) return;
            
            ArrayHeader* header = reinterpret_cast<ArrayHeader*>(memory);
            
            // حساب الحجم المحرر
            size_t headerSize = sizeof(ArrayHeader);
            size_t elementSize = sizeof(Value);
            size_t totalSize = headerSize + (header->capacity * elementSize);
            
            totalAllocated -= totalSize;
            allocationCount--;
            
            delete[] reinterpret_cast<uint8_t*>(memory);
        }
        
        // الحصول على إحصائيات الذاكرة
        void printStatistics() const {
            std::cout << "═══════════════════════════════════════════════════" << std::endl;
            std::cout << "Array Memory Management Statistics:" << std::endl;
            std::cout << "  Total Allocated: " << totalAllocated << " bytes" << std::endl;
            std::cout << "  Total Allocations: " << allocationCount << std::endl;
            std::cout << "═══════════════════════════════════════════════════" << std::endl;
        }
        
    private:
        ArrayMemoryManager() = default;
        
        size_t totalAllocated = 0;
        size_t allocationCount = 0;
    };

    // ═══════════════════════════════════════════════════════════
    // Public Array Functions (callable from interpreter)
    // ═══════════════════════════════════════════════════════════
    
    // تحويل Value إلى مصفوفة داخلية
    void* valueToArrayPointer(const Value& v) {
        if (v.type != ValueType::ARRAY || v.elements.empty()) {
            return nullptr;
        }
        
        // البحث عن مؤشر الذاكرة المخزن في بيانات إضافية
        // في الحالة الحالية، نعيد nullptr وسننشئ نظام تتبع منفصل
        return nullptr;
    }
    
    // إنشاء Value من مصفوفة
    Value arrayToValue(void* arrayPtr) {
        if (!arrayPtr) {
            Value empty(ValueType::ARRAY);
            return empty;
        }
        
        ArrayHeader* header = reinterpret_cast<ArrayHeader*>(arrayPtr);
        Value result(ValueType::ARRAY);
        result.elements.reserve(header->length);
        
        Value* elements = reinterpret_cast<Value*>(
            reinterpret_cast<uint8_t*>(arrayPtr) + sizeof(ArrayHeader)
        );
        
        for ( uint32_t i = 0; i < header->length; ++i) {
            result.elements.push_back(elements[i]);
        }
        
        return result;
    }
    
    // ────────────────────────────────────────────────────────────
    // Implementation of: generateArrayAssignment
    // الدالة الأساسية لإنشاء وتعيين المصفوفات
    // ────────────────────────────────────────────────────────────
    
    Value generateArrayAssignment(
        const std::string& arrayName,
        const std::vector<Value>& initialValues,
        ArabicRuntime* runtime
    ) {
        // 1. تخصيص مصفوفة جديدة
        auto& memManager = ArrayMemoryManager::getInstance();
        uint32_t initialCapacity = std::max(
            static_cast<uint32_t>(initialValues.size() + 2),
            ArrayHeader::MIN_CAPACITY
        );
        
        void* arrayPtr = memManager.allocateArray(initialCapacity);
        ArrayHeader* header = reinterpret_cast<ArrayHeader*>(arrayPtr);
        Value* elements = reinterpret_cast<Value*>(
            reinterpret_cast<uint8_t*>(arrayPtr) + sizeof(ArrayHeader)
        );
        
        // 2. نسخ القيم الأولية
        for (size_t i = 0; i < initialValues.size(); ++i) {
            elements[i] = initialValues[i];
            header->length++;
        }
        
        // 3. إنشاء Value من النتيجة
        Value arrayValue(ValueType::ARRAY);
        arrayValue.elements = initialValues;
        
        // 4. حفظ المصفوفة في بيئة التشغيل
        if (runtime) {
            // نخزن مؤشر الذاكرة الخام في متغير خاص
            Value pointerValue(ValueType::NUMBER);
            pointerValue.number_value = reinterpret_cast<uint64_t>(arrayPtr);
            runtime->setVariable("__array_" + arrayName, pointerValue);
        }
        
        std::cout << "✅ تم إنشاء مصفوفة: " << arrayName 
                  << " (الطول: " << header->length 
                  << ", السعة: " << header->capacity << ")" << std::endl;
        
        return arrayValue;
    }
    
    // ────────────────────────────────────────────────────────────
    // Implementation of: طول (length) method
    // ────────────────────────────────────────────────────────────
    
    Value arrayLength(const Value& arrayValue, ArabicRuntime* runtime) {
        // في الوقت الحالي، نعود بطول الـ elements
        Value result(ValueType::NUMBER);
        result.number_value = static_cast<double>(arrayValue.elements.size());
        return result;
    }
    
    // ────────────────────────────────────────────────────────────
    // Implementation of: أضف (add) method
    // ────────────────────────────────────────────────────────────
    
    Value arrayAdd(Value& arrayValue, const Value& newElement, ArabicRuntime* runtime) {
        if (arrayValue.type != ValueType::ARRAY) {
            throw std::runtime_error("❌ خطأ: المتغير ليس مصفوفة");
        }
        
        // إضافة العنصر الجديد
        arrayValue.elements.push_back(newElement);
        
        std::cout << "✅ تم إضافة عنصر. الطول الجديد: " << arrayValue.elements.size() << std::endl;
        
        // إرجاع المصفوفة المحدثة
        return arrayValue;
    }
    
    // ────────────────────────────────────────────────────────────
    // Implementation of: سعة (capacity) property
    // ────────────────────────────────────────────────────────────
    
    Value arrayCapacity(const Value& arrayValue, ArabicRuntime* runtime) {
        Value result(ValueType::NUMBER);
        // سعة حالية مرتبطة بحجم القائمة المتاح
        result.number_value = static_cast<double>(arrayValue.elements.capacity());
        return result;
    }
    
    // ────────────────────────────────────────────────────────────
    // Implementation of: أزل (remove) method
    // ────────────────────────────────────────────────────────────
    
    Value arrayRemove(Value& arrayValue, const Value& indexValue, ArabicRuntime* runtime) {
        if (arrayValue.type != ValueType::ARRAY) {
            throw std::runtime_error("❌ خطأ: المتغير ليس مصفوفة");
        }
        
        uint32_t index = static_cast<uint32_t>(indexValue.number_value);
        
        if (index >= arrayValue.elements.size()) {
            throw std::runtime_error("❌ خطأ: الفهرس خارج الحدود");
        }
        
        arrayValue.elements.erase(arrayValue.elements.begin() + index);
        
        std::cout << "✅ تم إزالة العنصر في الموضع: " << index 
                  << ". الطول الجديد: " << arrayValue.elements.size() << std::endl;
        
        return arrayValue;
    }
    
    // ────────────────────────────────────────────────────────────
    // Implementation of: أدخل (insert) method
    // ────────────────────────────────────────────────────────────
    
    Value arrayInsert(Value& arrayValue, const Value& indexValue, const Value& newElement, ArabicRuntime* runtime) {
        if (arrayValue.type != ValueType::ARRAY) {
            throw std::runtime_error("❌ خطأ: المتغير ليس مصفوفة");
        }
        
        uint32_t index = static_cast<uint32_t>(indexValue.number_value);
        
        if (index > arrayValue.elements.size()) {
            throw std::runtime_error("❌ خطأ: الفهرس خارج الحدود");
        }
        
        arrayValue.elements.insert(arrayValue.elements.begin() + index, newElement);
        
        std::cout << "✅ تم إدراج عنصر في الموضع: " << index 
                  << ". الطول الجديد: " << arrayValue.elements.size() << std::endl;
        
        return arrayValue;
    }
    
    // ────────────────────────────────────────────────────────────
    // Implementation of: طباعة_ذاكرة (printMemoryStats)
    // ────────────────────────────────────────────────────────────
    
    void printArrayMemoryStats() {
        auto& memManager = ArrayMemoryManager::getInstance();
        memManager.printStatistics();
    }

} // namespace ArabicLanguage
