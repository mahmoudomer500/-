#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include <variant>
#include <optional>
#include "ArabicTypes.h"
#include "ArabicMemoryManager.h"

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // 🎯 نظام استنتاج الأنواع المتقدم
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief نظام شامل لاستنتاج وفحص الأنواع في اللغة العربية
     */
    class ArabicTypeInference {
    public:
        // Singleton pattern
        static ArabicTypeInference& getInstance();

        // منع النسخ والتعيين
        ArabicTypeInference(const ArabicTypeInference&) = delete;
        ArabicTypeInference& operator=(const ArabicTypeInference&) = delete;

        // ══════════════════════════════════════════════════════════════
        // 📊 بنية معلومات النوع
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief معلومات نوع شاملة مع قيود وعلاقات
         */
        struct TypeInfo {
            std::string name;                    // اسم النوع
            ValueType baseType;                 // النوع الأساسي
            std::vector<std::string> constraints; // قيود النوع
            std::unordered_map<std::string, std::string> properties; // خصائص النوع
            std::unordered_map<std::string, std::string> methods;    // دوال النوع
            bool isGeneric = false;             // هل هو نوع عام؟
            bool isNullable = false;            // هل يمكن أن يكون null؟
            bool isMutable = true;              // هل هو قابل للتغيير؟
            std::optional<std::string> superType; // نوع الأب

            TypeInfo(const std::string& n = "", ValueType bt = ValueType::NONE)
                : name(n), baseType(bt) {}

            std::string toString() const;
            bool isCompatibleWith(const TypeInfo& other) const;
            bool canBeAssignedFrom(const TypeInfo& other) const;
            TypeInfo getCommonType(const TypeInfo& other) const;
        };

        /**
         * @brief نتيجة استنتاج النوع
         */
        struct InferenceResult {
            TypeInfo inferredType;
            std::vector<std::string> assumptions;  // افتراضات تم عملها
            std::vector<std::string> constraints;  // قيود تم فرضها
            double confidence = 1.0;              // درجة الثقة (0.0 - 1.0)
            bool isDefinite = true;               // هل الاستنتاج نهائي؟

            InferenceResult() = default;
            InferenceResult(const TypeInfo& type, double conf = 1.0)
                : inferredType(type), confidence(conf), isDefinite(conf >= 0.9) {}
        };

        // ══════════════════════════════════════════════════════════════
        // 🔍 مستنتج الأنواع
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مستنتج أنواع ذكي يستخدم خوارزميات متقدمة
         */
        class TypeInferencer {
        private:
            std::unordered_map<std::string, TypeInfo> typeTable;
            std::unordered_map<std::string, InferenceResult> variableTypes;
            std::vector<std::string> activeConstraints;
            int recursionDepth = 0;
            static constexpr int MAX_RECURSION = 10;

        public:
            /**
             * @brief استنتاج نوع من قيمة
             */
            InferenceResult inferFromValue(const Value& value);

            /**
             * @brief استنتاج نوع من تعبير
             */
            InferenceResult inferFromExpression(const std::string& expression,
                                              const std::unordered_map<std::string, TypeInfo>& context);

            /**
             * @brief استنتاج نوع من استدعاء دالة
             */
            InferenceResult inferFromFunctionCall(const std::string& functionName,
                                                const std::vector<TypeInfo>& argTypes);

            /**
             * @brief استنتاج نوع من عملية ثنائية
             */
            InferenceResult inferFromBinaryOp(const std::string& op,
                                             const TypeInfo& leftType,
                                             const TypeInfo& rightType);

            /**
             * @brief استنتاج نوع من عملية أحادية
             */
            InferenceResult inferFromUnaryOp(const std::string& op, const TypeInfo& operandType);

            /**
             * @brief حل قيود الأنواع
             */
            bool resolveConstraints();

            /**
             * @brief فحص توافق الأنواع
             */
            bool checkTypeCompatibility(const TypeInfo& source, const TypeInfo& target);

            // إدارة الجدول
            void addType(const std::string& name, const TypeInfo& type);
            void addVariable(const std::string& name, const InferenceResult& result);
            std::optional<TypeInfo> getType(const std::string& name) const;
            std::optional<InferenceResult> getVariableType(const std::string& name) const;

        private:
            InferenceResult inferFromLiteral(const std::string& literal);
            InferenceResult inferFromVariable(const std::string& varName);
            InferenceResult inferFromArrayLiteral(const std::vector<TypeInfo>& elementTypes);
            InferenceResult inferFromObjectLiteral(const std::unordered_map<std::string, TypeInfo>& props);

            TypeInfo getBinaryOpResultType(const std::string& op,
                                          const TypeInfo& left,
                                          const TypeInfo& right);
            bool unifyTypes(TypeInfo& t1, const TypeInfo& t2);
        };

        // ══════════════════════════════════════════════════════════════
        // ✅ فاحص الأنواع
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief فاحص أنواع شامل مع رسائل خطأ مفصلة
         */
        class TypeChecker {
        private:
            TypeInferencer& inferencer;
            std::vector<std::string> errors;
            std::vector<std::string> warnings;
            bool strictMode = true;

        public:
            explicit TypeChecker(TypeInferencer& inf) : inferencer(inf) {}

            /**
             * @brief فحص أنواع الأوامر
             */
            bool checkCommands(const std::vector<std::shared_ptr<Command>>& commands);

            /**
             * @brief فحص نوع التعبير
             */
            bool checkExpression(const std::string& expression,
                               const std::unordered_map<std::string, TypeInfo>& context);

            /**
             * @brief فحص استدعاء دالة
             */
            bool checkFunctionCall(const std::string& functionName,
                                 const std::vector<std::string>& argExpressions,
                                 const std::unordered_map<std::string, TypeInfo>& context);

            /**
             * @brief فحص التعيين
             */
            bool checkAssignment(const std::string& varName,
                               const std::string& expression,
                               const std::unordered_map<std::string, TypeInfo>& context);

            // التقارير والإحصائيات
            const std::vector<std::string>& getErrors() const { return errors; }
            const std::vector<std::string>& getWarnings() const { return warnings; }
            void clearMessages();

            void setStrictMode(bool strict) { strictMode = strict; }
            bool isStrictMode() const { return strictMode; }

        private:
            bool checkCommand(const std::shared_ptr<Command>& command,
                            std::unordered_map<std::string, TypeInfo>& context);
            bool checkIfStatement(const std::shared_ptr<Command>& command,
                                std::unordered_map<std::string, TypeInfo>& context);
            bool checkLoop(const std::shared_ptr<Command>& command,
                         std::unordered_map<std::string, TypeInfo>& context);
            bool checkFunctionDefinition(const std::shared_ptr<Command>& command,
                                       std::unordered_map<std::string, TypeInfo>& context);

            void addError(const std::string& message, int line = -1);
            void addWarning(const std::string& message, int line = -1);
        };

        // ══════════════════════════════════════════════════════════════
        // 📚 مكتبة الأنواع القياسية
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief مكتبة الأنواع المدمجة في اللغة
         */
        class StandardTypeLibrary {
        private:
            std::unordered_map<std::string, TypeInfo> builtinTypes;

        public:
            StandardTypeLibrary();

            /**
             * @brief إضافة نوع قياسي
             */
            void addBuiltinType(const std::string& name, const TypeInfo& type);

            /**
             * @brief الحصول على نوع قياسي
             */
            std::optional<TypeInfo> getBuiltinType(const std::string& name) const;

            /**
             * @brief فحص ما إذا كان النوع قياسياً
             */
            bool isBuiltinType(const std::string& name) const;

            /**
             * @brief الحصول على جميع الأنواع القياسية
             */
            std::vector<std::string> getAllBuiltinTypes() const;

            /**
             * @brief إنشاء أنواع مركبة قياسية
             */
            TypeInfo createArrayType(const TypeInfo& elementType);
            TypeInfo createFunctionType(const std::vector<TypeInfo>& paramTypes,
                                      const TypeInfo& returnType);
            TypeInfo createOptionalType(const TypeInfo& baseType);
        };

        // ══════════════════════════════════════════════════════════════
        // 🎯 واجهة الاستخدام العامة
        // ══════════════════════════════════════════════════════════════

        /**
         * @brief استنتاج نوع من كود عربي
         */
        InferenceResult inferType(const std::string& code,
                                const std::unordered_map<std::string, TypeInfo>& context = {});

        /**
         * @brief فحص أنواع الكود
         */
        bool checkTypes(const std::vector<std::shared_ptr<Command>>& commands,
                       std::vector<std::string>& errors,
                       std::vector<std::string>& warnings);

        /**
         * @brief تحسين الأنواع (إزالة الأنواع غير المستخدمة)
         */
        void optimizeTypes(std::vector<std::shared_ptr<Command>>& commands);

        /**
         * @brief إضافة تعليقات نوع للكود
         */
        std::string addTypeAnnotations(const std::string& code);

        // إدارة الأنواع
        TypeInferencer& getInferencer() { return inferencer; }
        TypeChecker& getChecker() { return checker; }
        StandardTypeLibrary& getStandardLibrary() { return stdLibrary; }

        // إحصائيات ومراقبة
        std::vector<std::string> getStats() const;
        void reset();

    public:
        ArabicTypeInference();
        ~ArabicTypeInference() = default;

    private:
        TypeInferencer inferencer;
        TypeChecker checker{inferencer};
        StandardTypeLibrary stdLibrary;
    };

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة لاستنتاج الأنواع
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief استنتاج نوع سريع من قيمة
     */
    ArabicTypeInference::InferenceResult quickTypeInference(const Value& value);

    /**
     * @brief فحص توافق نوعين
     */
    bool areTypesCompatible(const ArabicTypeInference::TypeInfo& t1,
                          const ArabicTypeInference::TypeInfo& t2);

    /**
     * @brief الحصول على نوع أساسي من سلسلة
     */
    ValueType stringToValueType(const std::string& typeStr);

    /**
     * @brief تحويل نوع إلى سلسلة
     */
    std::string valueTypeToArabicString(ValueType type);

    /**
     * @brief إنشاء نوع عام
     */
    ArabicTypeInference::TypeInfo createGenericType(const std::string& name,
                                                   const std::vector<std::string>& typeParams);

    /**
     * @brief تخصيص نوع عام
     */
    ArabicTypeInference::TypeInfo specializeGenericType(const ArabicTypeInference::TypeInfo& genericType,
                                                      const std::vector<std::string>& typeArgs);

} // namespace ArabicLanguage
