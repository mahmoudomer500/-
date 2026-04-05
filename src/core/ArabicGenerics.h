#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include <regex>
#include "ArabicTypes.h"
#include "ArabicMemoryManager.h"

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // 🎯 نظام الجينيريك (Generics) للغة العربية
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief نظام شامل لدعم الجينيريك والقوالب في اللغة العربية
     */
    class ArabicGenerics {
    public:
        // Singleton pattern
        static ArabicGenerics& getInstance();

        // منع النسخ والتعيين
        ArabicGenerics(const ArabicGenerics&) = delete;
        ArabicGenerics& operator=(const ArabicGenerics&) = delete;

        // ══════════════════════════════════════════════════════════════
        // 🏷️ معلمات النوع (Type Parameters)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief معلمة نوع عامة
         */
        struct TypeParameter {
            std::string name;                    // اسم المعلمة (مثل "T", "U", "V")
            std::vector<std::string> constraints; // قيود على النوع
            std::string defaultType;             // نوع افتراضي
            bool isCovariant = false;            // تباين موجب
            bool isContravariant = false;        // تباين سالب

            TypeParameter(const std::string& n = "")
                : name(n) {}

            std::string toString() const;
            bool satisfiesConstraints(const std::string& typeName) const;
        };

        // ══════════════════════════════════════════════════════════════
        // 📦 نوع عام (Generic Type)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تعريف نوع عام
         */
        struct GenericType {
            std::string name;                           // اسم النوع العام
            std::vector<TypeParameter> typeParameters;  // معلمات النوع
            std::string baseType;                       // نوع أساسي إن وجد
            std::unordered_map<std::string, std::string> methods; // دوال النوع
            std::unordered_map<std::string, std::string> properties; // خصائص النوع
            bool isInterface = false;                   // هل هو واجهة؟
            bool isAbstract = false;                    // هل هو مجرد؟

            GenericType(const std::string& n = "")
                : name(n) {}

            std::string getSignature() const;           // توقيع النوع العام
            std::string instantiate(const std::vector<std::string>& typeArgs) const;
            bool canInstantiateWith(const std::vector<std::string>& typeArgs) const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🔧 دالة عامة (Generic Function)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تعريف دالة عامة
         */
        struct GenericFunction {
            std::string name;                           // اسم الدالة
            std::vector<TypeParameter> typeParameters;  // معلمات النوع
            std::vector<std::pair<std::string, std::string>> parameters; // (اسم, نوع)
            std::string returnType;                     // نوع الإرجاع
            std::string body;                           // جسم الدالة
            std::vector<std::string> constraints;       // قيود إضافية
            bool isStatic = false;                      // هل هي دالة ثابتة؟
            bool isInline = false;                      // هل هي دالة مضمنة؟

            GenericFunction(const std::string& n = "")
                : name(n) {}

            std::string getSignature() const;
            std::string instantiate(const std::vector<std::string>& typeArgs) const;
            bool canInstantiateWith(const std::vector<std::string>& typeArgs) const;
            std::vector<std::string> getTypeDependencies() const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🏗️ صنف عام (Generic Class)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief صنف عام يدعم الوراثة والواجهات
         */
        struct GenericClass : public GenericType {
            std::vector<std::string> implementedInterfaces; // الواجهات المنفذة
            std::unordered_map<std::string, GenericFunction> methods; // الدوال
            std::unordered_map<std::string, std::string> fields; // الحقول
            std::vector<std::string> constructors; // المنشئات

            GenericClass(const std::string& n = "")
                : GenericType(n) {}

            std::string generateCppCode(const std::vector<std::string>& typeArgs) const;
            bool implementsInterface(const std::string& interfaceName) const;
            std::vector<std::string> getAllMethods() const;
        };

        // ══════════════════════════════════════════════════════════════
        // 💾 نظام التخزين المؤقت للأنواع المُنشأة (Type Cache)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تخزين مؤقت للأنواع المُنشأة لتحسين الأداء
         */
        class InstantiatedTypeCache {
        private:
            struct CacheEntry {
                std::string instantiatedCode;
                std::chrono::steady_clock::time_point created;
                size_t accessCount = 0;

                CacheEntry(const std::string& code = "")
                    : instantiatedCode(code), created(std::chrono::steady_clock::now()) {}
            };

            std::unordered_map<std::string, CacheEntry> cache;
            std::mutex cacheMutex;
            size_t maxEntries = 1000;

        public:
            std::string get(const std::string& key);
            void put(const std::string& key, const std::string& code);
            void clear();
            size_t size() const { return cache.size(); }
            void cleanup();
        };

        // ══════════════════════════════════════════════════════════════
        // 🔍 محلل الجينيريك (Generic Parser)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief محلل متخصص للكود العام
         */
        class GenericParser {
        private:
            std::regex typeParamRegex;     // regex لمعلمات النوع
            std::regex genericTypeRegex;   // regex للأنواع العامة
            std::regex constraintRegex;    // regex للقيود

        public:
            GenericParser();

            std::vector<TypeParameter> parseTypeParameters(const std::string& paramList);
            GenericType parseGenericType(const std::string& typeDefinition);
            GenericFunction parseGenericFunction(const std::string& functionDefinition);
            GenericClass parseGenericClass(const std::string& classDefinition);

            bool isGenericType(const std::string& typeName);
            bool isGenericFunction(const std::string& functionSignature);
            std::vector<std::string> extractTypeArguments(const std::string& genericUsage);
        };

        // ══════════════════════════════════════════════════════════════
        // ⚡ مُنشئ الأنواع (Type Instantiator)
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مُنشئ الأنواع العامة مع التحقق من القيود
         */
        class TypeInstantiator {
        private:
            InstantiatedTypeCache& cache;
            std::unordered_map<std::string, GenericType> typeRegistry;
            std::unordered_map<std::string, GenericFunction> functionRegistry;

        public:
            explicit TypeInstantiator(InstantiatedTypeCache& c) : cache(c) {}

            std::string instantiateType(const std::string& genericTypeName,
                                      const std::vector<std::string>& typeArgs);
            std::string instantiateFunction(const std::string& genericFunctionName,
                                          const std::vector<std::string>& typeArgs);
            std::string instantiateClass(const std::string& genericClassName,
                                       const std::vector<std::string>& typeArgs);

            bool registerGenericType(const GenericType& type);
            bool registerGenericFunction(const GenericFunction& func);
            bool registerGenericClass(const GenericClass& cls);

            std::vector<std::string> getAvailableTypes() const;
            std::vector<std::string> getAvailableFunctions() const;
        };

        // ══════════════════════════════════════════════════════════════
        // 🎯 واجهة الاستخدام العامة
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief تحليل وتسجيل نوع عام
         */
        bool registerGenericType(const std::string& typeDefinition);

        /**
         * @brief تحليل وتسجيل دالة عامة
         */
        bool registerGenericFunction(const std::string& functionDefinition);

        /**
         * @brief تحليل وتسجيل صنف عام
         */
        bool registerGenericClass(const std::string& classDefinition);

        /**
         * @brief إنشاء نسخة من نوع عام
         */
        std::string instantiateType(const std::string& genericName,
                                  const std::vector<std::string>& typeArgs);

        /**
         * @brief إنشاء نسخة من دالة عامة
         */
        std::string instantiateFunction(const std::string& genericName,
                                      const std::vector<std::string>& typeArgs);

        /**
         * @brief إنشاء نسخة من صنف عام
         */
        std::string instantiateClass(const std::string& genericName,
                                   const std::vector<std::string>& typeArgs);

        // إدارة التخزين المؤقت
        InstantiatedTypeCache& getCache() { return typeCache; }
        void clearCache() { typeCache.clear(); }

        // إحصائيات ومراقبة
        std::vector<std::string> getStats() const;
        std::vector<std::string> getRegisteredTypes() const;
        std::vector<std::string> getRegisteredFunctions() const;

        // محلل الجينيريك
        GenericParser& getParser() { return parser; }

        // مُنشئ الأنواع
        TypeInstantiator& getInstantiator() { return instantiator; }

    private:
        GenericParser parser;
        InstantiatedTypeCache typeCache;
        TypeInstantiator instantiator{typeCache};

        ArabicGenerics() = default;
        ~ArabicGenerics() = default;
    };

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة للجينيريك
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief التحقق من صحة استخدام الجينيريك
     */
    bool validateGenericUsage(const std::string& genericCode);

    /**
     * @brief توليد كود C++ من كود عربي عام
     */
    std::string generateGenericCppCode(const std::string& arabicGenericCode);

    /**
     * @brief تحسين الكود العام
     */
    std::string optimizeGenericCode(const std::string& genericCode);

    /**
     * @brief استخراج معلومات النوع من الكود
     */
    std::unordered_map<std::string, std::string> extractTypeInfo(const std::string& code);

    /**
     * @brief إنشاء مكتبة جينيريك قياسية
     */
    void initializeStandardGenericLibrary();

    // ══════════════════════════════════════════════════════════════
    // 📚 مكتبة الجينيريك القياسية
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief دوال وأنواع جينيريك قياسية
     */
    namespace StandardGenerics {

        // قائمة عامة
        const std::string LIST_DEFINITION = R"(
قائمة<نوع> {
    إضافة(عنصر: نوع) -> صفر
    إزالة(فهرس: رقم) -> نوع
    احصل(فهرس: رقم) -> نوع
    طول -> رقم
}
)";

        // خريطة عامة
        const std::string MAP_DEFINITION = R"(
خريطة<مفتاح، قيمة> {
    ضع(مفتاح: مفتاح، قيمة: قيمة) -> صفر
    احصل(مفتاح: مفتاح) -> قيمة
    يحتوي(مفتاح: مفتاح) -> منطق
    حذف(مفتاح: مفتاح) -> منطق
}
)";

        // دالة عامة للبحث
        const std::string FIND_FUNCTION = R"(
دالة عامة ابحث<نوع>(قائمة: قائمة<نوع>، شرط: دالة(نوع)->منطق) -> نوع {
    لكل عنصر في قائمة {
        إذا شرط(عنصر) {
            ارجع عنصر
        }
    }
    ارجع لا_شيء
}
)";

        // دالة عامة للتحويل
        const std::string TRANSFORM_FUNCTION = R"(
دالة عامة حول<مدخل، مخرج>(قائمة: قائمة<مدخل>، دالة_تحويل: دالة(مدخل)->مخرج) -> قائمة<مخرج> {
    نتيجة = قائمة_جديدة<مخرج>()
    لكل عنصر في قائمة {
        أضف(نتيجة، دالة_تحويل(عنصر))
    }
    ارجع نتيجة
}
)";

        /**
         * @brief تسجيل جميع الأنواع والدوال القياسية
         */
        void registerAll();

    } // namespace StandardGenerics

} // namespace ArabicLanguage
