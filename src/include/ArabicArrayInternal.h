// ArabicArrayInternal.h - الهيكل الداخلي للمصفوفات الديناميكية
// Internal structure for dynamic Arabic arrays with 16-byte metadata header

#ifndef ARABIC_ARRAY_INTERNAL_H
#define ARABIC_ARRAY_INTERNAL_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <algorithm>

namespace ArabicLanguage {

    // ═══════════════════════════════════════════════════════════
    // رأس المصفوفة الديناميكية (16 بايت)
    // Dynamic Array Header Structure (16 bytes)
    // ═══════════════════════════════════════════════════════════
    
    #pragma pack(push, 1)
    struct ArrayHeader {
        // الحقل الأول: الطول الفعلي للمصفوفة (4 بايتات)
        uint32_t length;
        
        // الحقل الثاني: السعة المخصصة (4 بايتات)
        uint32_t capacity;
        
        // الحقل الثالث: نوع البيانات (1 بايت)
        // 0 = رقم (NUMBER)
        // 1 = نص (STRING)
        // 2 = منطقي (BOOLEAN)
        // 3 = مصفوفة (ARRAY)
        // 4 = مختلط (MIXED)
        uint8_t element_type;
        
        // الحقل الرابع: علم التخصيص الديناميكي (1 بايت)
        uint8_t is_dynamic;
        
        // الحقل الخامس: علم الحماية من الكتابة (1 بايت)
        uint8_t is_readonly;
        
        // المتبقي: حقول محجوزة للمستقبل (5 بايتات)
        uint8_t reserved[5];
        
        // الحد الأدنى للسعة الأولية
        static constexpr uint32_t MIN_CAPACITY = 2;
        
        // معامل النمو عند إعادة التخصيص
        static constexpr float GROWTH_FACTOR = 1.5f;
        
        // ────────────────────────────────────────────────────────
        // البنّاء
        // ────────────────────────────────────────────────────────
        ArrayHeader() 
            : length(0), capacity(MIN_CAPACITY), element_type(4), 
              is_dynamic(1), is_readonly(0) {
            std::memset(reserved, 0, sizeof(reserved));
        }
        
        // ────────────────────────────────────────────────────────
        // الدوال المساعدة
        // ────────────────────────────────────────────────────────
        
        // حساب السعة الجديدة المطلوبة
        static uint32_t calculateNewCapacity(uint32_t currentCapacity, uint32_t requiredMinimum) {
            if (requiredMinimum <= currentCapacity) {
                return currentCapacity;
            }
            
            uint32_t newCapacity = currentCapacity;
            while (newCapacity < requiredMinimum) {
                // استخدم معامل النمو مع ضمان حد أدنى
                uint32_t grown = static_cast<uint32_t>(newCapacity * GROWTH_FACTOR);
                if (grown <= newCapacity) {
                    // تجنب فائض الأعداد الصحيحة
                    grown = newCapacity + (newCapacity / 2);
                }
                newCapacity = grown;
            }
            
            return newCapacity;
        }
        
        // تحديث السعة
        void updateCapacity(uint32_t newCapacity) {
            if (newCapacity >= length) {
                capacity = newCapacity;
            }
        }
        
        // إضافة عنصر جديد
        void addElement() {
            if (length + 1 > capacity) {
                capacity = calculateNewCapacity(capacity, length + 1);
            }
            length++;
        }
        
        // الحصول على حجم البيانات المطلوبة بالبايتات
        uint32_t getDataSizeInBytes() const {
            // حجم واحد - يعتمد على نوع العنصر الفعلي
            // هذا تقدير تقريبي - سيتم حسابه بدقة في الفئة الفعلية
            return sizeof(ArrayHeader) + (capacity * sizeof(double));
        }
    };
    #pragma pack(pop)
    
    // ────────────────────────────────────────────────────────────
    // تحقق من حجم الرأس (يجب أن يكون 16 بايت بالضبط)
    // ────────────────────────────────────────────────────────────
    static_assert(sizeof(ArrayHeader) == 16, "ArrayHeader must be exactly 16 bytes");

} // namespace ArabicLanguage

#endif // ARABIC_ARRAY_INTERNAL_H
