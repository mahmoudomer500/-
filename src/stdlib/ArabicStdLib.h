#pragma once

/**
 * @file ArabicStdLib.h
 * @brief مكتبة لغة البرمجة العربية القياسية
 *
 * هذه المكتبة تحتوي على جميع المكونات الأساسية للغة البرمجة العربية:
 * - I/O: عمليات الإدخال والإخراج
 * - Collections: الحاويات والبيانات (قوائم، خرائط، مجموعات)
 * - Math: الرياضيات المتقدمة (متجهات، مصفوفات، إحصائيات)
 *
 * @author فريق تطوير لغة البرمجة العربية
 * @version 1.0.0
 * @date ديسمبر 2025
 */

#include "io/ArabicIO.h"
#include "collections/ArabicCollections.h"
#include "math/ArabicMath.h"

namespace ArabicLanguage {

    /**
     * @namespace StdLib
     * @brief مساحة أسماء المكتبة القياسية
     *
     * تحتوي على جميع المكونات الأساسية للغة البرمجة العربية
     */
    namespace StdLib {

        // ══════════════════════════════════════════════════════════════
        // 🎯 واجهة المستخدم الرئيسية
        // ══════════════════════════════════════════════════════════════

        /**
         * @class ArabicStandardLibrary
         * @brief الفئة الرئيسية للمكتبة القياسية
         *
         * توفر وصول موحد لجميع مكونات المكتبة القياسية
         */
        class ArabicStandardLibrary {
        public:
            // Singleton pattern
            static ArabicStandardLibrary& getInstance();

            // منع النسخ والتعيين
            ArabicStandardLibrary(const ArabicStandardLibrary&) = delete;
            ArabicStandardLibrary& operator=(const ArabicStandardLibrary&) = delete;

            // ══════════════════════════════════════════════════════════════
            // 📁 مكونات المكتبة
            // ══════════════════════════════════════════════════════════════

            /**
             * @brief الحصول على مكتبة I/O
             * @return مرجع لمكتبة الإدخال/الإخراج
             */
            ArabicIO& getIO() { return ioInstance; }

            /**
             * @brief الحصول على مكتبة الحاويات
             * @return مرجع لمكتبة الحاويات
             */
            ArabicCollections& getCollections() { return collectionsInstance; }

            /**
             * @brief الحصول على مكتبة الرياضيات
             * @return مرجع لمكتبة الرياضيات
             */
            ArabicMath& getMath() { return mathInstance; }

            // ══════════════════════════════════════════════════════════════
            // 📊 معلومات المكتبة
            // ══════════════════════════════════════════════════════════════

            /**
             * @brief الحصول على معلومات الإصدار
             * @return معلومات الإصدار كنص
             */
            static std::string getVersion() { return "1.0.0"; }

            /**
             * @brief الحصول على معلومات المكتبة
             * @return معلومات شاملة عن المكتبة
             */
            std::vector<std::string> getLibraryInfo() const;

            /**
             * @brief تشغيل اختبارات المكتبة
             * @return نتائج الاختبارات
             */
            std::vector<std::string> runTests() const;

            /**
             * @brief الحصول على تقرير الأداء
             * @return تقرير أداء جميع مكونات المكتبة
             */
            std::vector<std::string> getPerformanceReport() const;

        private:
            ArabicIO& ioInstance;
            ArabicCollections& collectionsInstance;
            ArabicMath& mathInstance;

            ArabicStandardLibrary();
            ~ArabicStandardLibrary() = default;
        };

        // ══════════════════════════════════════════════════════════════
        // 🔧 دوال مساعدة عامة
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تهيئة المكتبة القياسية
         * @return true إذا نجحت التهيئة
         */
        bool initializeStdLib();

        /**
         * @brief إنهاء المكتبة القياسية
         */
        void shutdownStdLib();

        /**
         * @brief التحقق من صحة المكتبة
         * @return true إذا كانت المكتبة تعمل بشكل صحيح
         */
        bool validateStdLib();

        // ══════════════════════════════════════════════════════════════
        // 📚 مساحات أسماء فرعية
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief دوال I/O المباشرة
         */
        namespace IOLib = ArabicLanguage::StdLib::IO;

        /**
         * @brief دوال الحاويات المباشرة
         */
        namespace CollectionsLib = ArabicLanguage::StdLib::Collections;

        /**
         * @brief دوال الرياضيات المباشرة
         */
        namespace MathLib = ArabicLanguage::StdLib::Math;

    } // namespace StdLib

    // ══════════════════════════════════════════════════════════════
    // 🌟 دوال عامة للوصول السريع
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief الحصول على نسخة المكتبة القياسية
     * @return مرجع للمكتبة القياسية
     */
    inline StdLib::ArabicStandardLibrary& stdlib() {
        return StdLib::ArabicStandardLibrary::getInstance();
    }

    /**
     * @brief الحصول على مكتبة I/O
     * @return مرجع لمكتبة I/O
     */
    inline StdLib::ArabicIO& io() {
        return stdlib().getIO();
    }

    /**
     * @brief الحصول على مكتبة الحاويات
     * @return مرجع لمكتبة الحاويات
     */
    inline StdLib::ArabicCollections& collections() {
        return stdlib().getCollections();
    }

    /**
     * @brief الحصول على مكتبة الرياضيات
     * @return مرجع لمكتبة الرياضيات
     */
    inline StdLib::ArabicMath& math() {
        return stdlib().getMath();
    }

} // namespace ArabicLanguage

// ══════════════════════════════════════════════════════════════
// 📝 ملاحظات الاستخدام
// ══════════════════════════════════════════════════════════════

/*
طريقة الاستخدام الأساسية:

1. تضمين المكتبة:
   #include "ArabicStdLib.h"

2. تهيئة المكتبة:
   ArabicLanguage::StdLib::initializeStdLib();

3. استخدام المكونات:
   // I/O
   ArabicLanguage::io().writeConsoleLine("مرحبا بالعالم");

   // الحاويات
   auto قائمة = ArabicLanguage::collections().List<int>();
   قائمة.add(42);

   // الرياضيات
   double جذر = ArabicLanguage::math().BasicMath::sqrt(16.0);

4. إنهاء المكتبة:
   ArabicLanguage::StdLib::shutdownStdLib();

أو الطريقة المختصرة:
   #include "ArabicStdLib.h"

   ArabicLanguage::io().writeConsoleLine("مرحبا");
   auto قائمة = ArabicLanguage::collections().List<int>();
   double جذر = ArabicLanguage::math().BasicMath::sqrt(16.0);
*/
