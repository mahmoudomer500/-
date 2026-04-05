#pragma once

#include "ArabicTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <memory>

namespace ArabicLanguage {

    /**
     * @brief نظام استنتاج الأنواع المتقدم للمترجم العربي
     *
     * يقوم بتحليل التعبيرات واستنتاج الأنواع بشكل ذكي
     * مع دعم الأنواع المعقدة والتحويلات التلقائية.
     */
    class ArabicTypeInferencer {
    public:
        /**
         * @brief نتيجة استنتاج النوع
         */
        struct TypeInferenceResult {
            ValueType primaryType;
            std::vector<ValueType> possibleTypes;  // للأنواع المتعددة الممكنة
            bool isDefinite;                       // هل النوع محدد تماماً؟
            double confidence;                     // درجة الثقة (0.0 - 1.0)
            std::string reasoning;                // تفسير الاستنتاج

            TypeInferenceResult(ValueType type = ValueType::NONE,
                              bool definite = false,
                              double conf = 0.0,
                              const std::string& reason = "")
                : primaryType(type), isDefinite(definite), confidence(conf), reasoning(reason) {}
        };

        /**
         * @brief معلومات السياق للاستنتاج
         */
        struct InferenceContext {
            std::unordered_map<std::string, ValueType> variableTypes;
            std::unordered_map<std::string, std::string> functionReturnTypes;
            bool allowTypePromotion;
            bool strictMode;

            InferenceContext(bool promotion = true, bool strict = false)
                : allowTypePromotion(promotion), strictMode(strict) {}
        };

    private:
        // تعبيرات منتظمة للكشف عن الأنواع
        std::regex numberPattern;
        std::regex stringPattern;
        std::regex booleanPattern;
        std::regex arrayPattern;
        std::regex functionCallPattern;
        std::regex arithmeticPattern;

        // ذاكرة تخزين مؤقت للنتائج
        std::unordered_map<std::string, TypeInferenceResult> inferenceCache;

        // إحصائيات الأداء
        struct InferenceStats {
            size_t totalInferences;
            size_t cacheHits;
            size_t cacheMisses;
            size_t typePromotions;
            size_t failedInferences;

            InferenceStats() : totalInferences(0), cacheHits(0), cacheMisses(0),
                             typePromotions(0), failedInferences(0) {}
        } stats;

        /**
         * @brief تهيئة أنماط التعبيرات المنتظمة
         */
        void initializePatterns();

        /**
         * @brief التحقق من أن النص يمثل رقماً
         */
        bool isNumericLiteral(const std::string& expr) const;

        /**
         * @brief التحقق من أن النص يمثل نصاً
         */
        bool isStringLiteral(const std::string& expr) const;

        /**
         * @brief التحقق من أن النص يمثل قيمة منطقية
         */
        bool isBooleanLiteral(const std::string& expr) const;

        /**
         * @brief التحقق من أن النص يمثل مصفوفة
         */
        bool isArrayLiteral(const std::string& expr) const;

        /**
         * @brief استنتاج نوع المتغير من اسمه
         */
        TypeInferenceResult inferVariableType(const std::string& varName,
                                            const InferenceContext& context) const;

        /**
         * @brief استنتاج نوع الدالة من اسمها
         */
        TypeInferenceResult inferFunctionCallType(const std::string& funcCall,
                                                 const InferenceContext& context) const;

        /**
         * @brief استنتاج نوع العملية الحسابية
         */
        TypeInferenceResult inferArithmeticType(const std::string& expr,
                                              const InferenceContext& context) const;

        /**
         * @brief استنتاج نوع المقارنة
         */
        TypeInferenceResult inferComparisonType(const std::string& expr,
                                              const InferenceContext& context) const;

        /**
         * @brief دمج نتائج استنتاج متعددة
         */
        TypeInferenceResult mergeResults(const std::vector<TypeInferenceResult>& results) const;

        /**
         * @brief تحسين النتيجة بناءً على السياق
         */
        TypeInferenceResult optimizeResult(TypeInferenceResult result,
                                         const InferenceContext& context) const;

    public:
        ArabicTypeInferencer();
        ~ArabicTypeInferencer() = default;

        // منع النسخ والتعيين
        ArabicTypeInferencer(const ArabicTypeInferencer&) = delete;
        ArabicTypeInferencer& operator=(const ArabicTypeInferencer&) = delete;

        /**
         * @brief استنتاج نوع التعبير
         * @param expression التعبير المراد استنتاج نوعه
         * @param context السياق المتاح (اختياري)
         * @return نتيجة الاستنتاج
         */
        TypeInferenceResult inferType(const std::string& expression,
                                    const InferenceContext& context = InferenceContext());

        /**
         * @brief استنتاج نوع التعبير مع ذاكرة تخزين مؤقت
         */
        TypeInferenceResult inferTypeCached(const std::string& expression,
                                          const InferenceContext& context = InferenceContext());

        /**
         * @brief التحقق من توافق الأنواع
         */
        bool areTypesCompatible(ValueType from, ValueType to,
                              const InferenceContext& context = InferenceContext()) const;

        /**
         * @brief اقتراح تحويل نوع
         */
        ValueType suggestTypeConversion(ValueType from, ValueType to) const;

        /**
         * @brief تحديث سياق المتغيرات
         */
        void updateVariableType(const std::string& varName, ValueType type,
                              InferenceContext& context) const;

        /**
         * @brief تنظيف الذاكرة المؤقتة
         */
        void clearCache();

        /**
         * @brief الحصول على إحصائيات الاستنتاج
         */
        InferenceStats getStats() const { return stats; }

        /**
         * @brief إعادة تعيين الإحصائيات
         */
        void resetStats() { stats = InferenceStats(); }

        /**
         * @brief الحصول على معلومات مفصلة عن الأنواع المدعومة
         */
        std::vector<std::string> getSupportedTypesInfo() const;

        /**
         * @brief تشخيص مشاكل الاستنتاج
         */
        std::vector<std::string> diagnoseInference(const std::string& expression,
                                                 const InferenceContext& context = InferenceContext());
    };

    /**
     * @brief دوال مساعدة للاستنتاج
     */
    namespace TypeInferenceHelpers {
        /**
         * @brief استخراج اسم المتغير من التعبير
         */
        std::string extractVariableName(const std::string& expr);

        /**
         * @brief استخراج اسم الدالة من استدعاء الدالة
         */
        std::string extractFunctionName(const std::string& funcCall);

        /**
         * @brief تقسيم التعبير إلى أجزاء
         */
        std::vector<std::string> tokenizeExpression(const std::string& expr);

        /**
         * @brief تحليل الأولوية في التعبير
         */
        int getOperatorPrecedence(const std::string& op);

        /**
         * @brief التحقق من صحة التعبير
         */
        bool isValidExpression(const std::string& expr);
    }

} // namespace ArabicLanguage
