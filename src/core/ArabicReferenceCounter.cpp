#include "ArabicReferenceCounter.h"
#include <iostream>
#include <algorithm>

namespace ArabicLanguage {

    // لا يحتاج هذا الملف إلى تطبيق إضافي
    // جميع الطرق inline في الملف الرأسي

    /**
     * دالة مساعدة لطباعة تقرير الإحصائيات
     */
    void printReferenceCounterStats(const ArabicReferenceCounter& counter) {
        auto stats = counter.getStats();

        std::cout << "=== إحصائيات نظام العد المرجعي ===" << std::endl;
        std::cout << "الكائنات النشطة: " << stats["active_objects"] << std::endl;
        std::cout << "إجمالي المنشأ: " << stats["total_created"] << std::endl;
        std::cout << "إجمالي المدمر: " << stats["total_destroyed"] << std::endl;
        std::cout << "الذروة: " << stats["peak_objects"] << std::endl;
        std::cout << "الحالي: " << stats["current_objects"] << std::endl;
    }

    /**
     * دالة مساعدة لجمع القمامة مع إحصائيات
     */
    size_t collectGarbageWithStats(ArabicReferenceCounter& counter) {
        size_t before = counter.getObjectCount();
        size_t collected = counter.collectGarbage();
        size_t after = counter.getObjectCount();

        std::cout << "جمع القمامة: تم تدمير " << collected << " كائن ("
                  << before << " → " << after << ")" << std::endl;

        return collected;
    }

} // namespace ArabicLanguage
