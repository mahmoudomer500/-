#include "ArabicTypeInferencer.h"
#include <algorithm>
#include <iostream>

namespace ArabicLanguage {

    ArabicTypeInferencer::ArabicTypeInferencer() {
        initializePatterns();
    }

    void ArabicTypeInferencer::initializePatterns() {
        // تهيئة أنماط التعبيرات المنتظمة
        numberPattern = std::regex(R"(^\d+(\.\d+)?$)");
        stringPattern = std::regex(R"(^".*"$|^'.*'$)");
        booleanPattern = std::regex(R"(^(صحيح|خطأ|true|false)$)");
        arrayPattern = std::regex(R"(^\[.*\]$)");
        functionCallPattern = std::regex(R"(^[a-zA-Z_][a-zA-Z0-9_]*\s*\(.*\)$)");
        arithmeticPattern = std::regex(R"(.*[\+\-\*\/\%].*)");
    }

    ArabicTypeInferencer::TypeInferenceResult ArabicTypeInferencer::inferType(
        const std::string& expression, const InferenceContext& context) {

        stats.totalInferences++;

        // التحقق من الذاكرة المؤقتة
        auto cached = inferenceCache.find(expression);
        if (cached != inferenceCache.end()) {
            stats.cacheHits++;
            return cached->second;
        }

        stats.cacheMisses++;

        TypeInferenceResult result;

        // إزالة المسافات البيضاء
        std::string trimmed = trim(expression);

        // فحص الأنواع الأساسية
        if (isNumericLiteral(trimmed)) {
            result.primaryType = ValueType::NUMBER;
            result.isDefinite = true;
            result.confidence = 1.0;
            result.reasoning = "رقم حرفي";
        }
        else if (isStringLiteral(trimmed)) {
            result.primaryType = ValueType::STRING;
            result.isDefinite = true;
            result.confidence = 1.0;
            result.reasoning = "نص حرفي";
        }
        else if (isBooleanLiteral(trimmed)) {
            result.primaryType = ValueType::NUMBER; // في العربية، القيم المنطقية تُعامل كأرقام
            result.isDefinite = true;
            result.confidence = 1.0;
            result.reasoning = "قيمة منطقية";
        }
        else if (isArrayLiteral(trimmed)) {
            result.primaryType = ValueType::ARRAY;
            result.isDefinite = true;
            result.confidence = 0.9;
            result.reasoning = "مصفوفة حرفية";
        }
        else {
            // محاولة استنتاج نوع أكثر تعقيداً
            TypeInferenceResult varResult = inferVariableType(trimmed, context);
            if (varResult.isDefinite) {
                result = varResult;
            }
            else {
                TypeInferenceResult funcResult = inferFunctionCallType(trimmed, context);
                if (funcResult.isDefinite) {
                    result = funcResult;
                }
                else {
                    TypeInferenceResult arithResult = inferArithmeticType(trimmed, context);
                    if (arithResult.isDefinite) {
                        result = arithResult;
                    }
                    else {
                        TypeInferenceResult compResult = inferComparisonType(trimmed, context);
                        if (compResult.isDefinite) {
                            result = compResult;
                        }
                        else {
                            // نوع غير محدد
                            result.primaryType = ValueType::NONE;
                            result.isDefinite = false;
                            result.confidence = 0.0;
                            result.reasoning = "تعبير غير معروف";
                            stats.failedInferences++;
                        }
                    }
                }
            }
        }

        return result;
    }

    ArabicTypeInferencer::TypeInferenceResult ArabicTypeInferencer::inferTypeCached(
        const std::string& expression, const InferenceContext& context) {

        TypeInferenceResult result = inferType(expression, context);

        // حفظ في الذاكرة المؤقتة
        inferenceCache[expression] = result;

        return result;
    }

    bool ArabicTypeInferencer::isNumericLiteral(const std::string& expr) const {
        return std::regex_match(expr, numberPattern);
    }

    bool ArabicTypeInferencer::isStringLiteral(const std::string& expr) const {
        return std::regex_match(expr, stringPattern);
    }

    bool ArabicTypeInferencer::isBooleanLiteral(const std::string& expr) const {
        return std::regex_match(expr, booleanPattern);
    }

    bool ArabicTypeInferencer::isArrayLiteral(const std::string& expr) const {
        return std::regex_match(expr, arrayPattern);
    }

    ArabicTypeInferencer::TypeInferenceResult ArabicTypeInferencer::inferVariableType(
        const std::string& varName, const InferenceContext& context) const {

        TypeInferenceResult result;

        // البحث في سياق المتغيرات
        auto it = context.variableTypes.find(varName);
        if (it != context.variableTypes.end()) {
            result.primaryType = it->second;
            result.isDefinite = true;
            result.confidence = 1.0;
            result.reasoning = "متغير معرّف في السياق";
        }
        else {
            // استنتاج بناءً على اسم المتغير
            if (varName.find("مصفوفة") != std::string::npos ||
                varName.find("قائمة") != std::string::npos) {
                result.primaryType = ValueType::ARRAY;
                result.isDefinite = false;
                result.confidence = 0.7;
                result.reasoning = "اسم المتغير يشير إلى مصفوفة";
            }
            else if (varName.find("نص") != std::string::npos ||
                     varName.find("رسالة") != std::string::npos) {
                result.primaryType = ValueType::STRING;
                result.isDefinite = false;
                result.confidence = 0.6;
                result.reasoning = "اسم المتغير يشير إلى نص";
            }
            else {
                result.primaryType = ValueType::NUMBER;
                result.isDefinite = false;
                result.confidence = 0.5;
                result.reasoning = "افتراض رقمي";
            }
        }

        return result;
    }

    ArabicTypeInferencer::TypeInferenceResult ArabicTypeInferencer::inferFunctionCallType(
        const std::string& funcCall, const InferenceContext& context) const {

        TypeInferenceResult result;

        // استخراج اسم الدالة
        size_t parenPos = funcCall.find('(');
        if (parenPos != std::string::npos) {
            std::string funcName = trim(funcCall.substr(0, parenPos));

            // البحث في سياق الدوال
            auto it = context.functionReturnTypes.find(funcName);
            if (it != context.functionReturnTypes.end()) {
                // يمكن تحسين هذا لتحليل نوع الإرجاع من السلسلة
                result.primaryType = ValueType::NUMBER; // افتراضي
                result.isDefinite = true;
                result.confidence = 0.9;
                result.reasoning = "دالة معرّفة في السياق";
            }
            else {
                // استنتاج بناءً على اسم الدالة
                if (funcName.find("اطبع") != std::string::npos) {
                    result.primaryType = ValueType::NONE; // لا ترجع شيئاً
                    result.isDefinite = true;
                    result.confidence = 1.0;
                    result.reasoning = "دالة الطباعة";
                }
                else {
                    result.primaryType = ValueType::NUMBER;
                    result.isDefinite = false;
                    result.confidence = 0.5;
                    result.reasoning = "دالة عامة - افتراض رقمي";
                }
            }
        }

        return result;
    }

    ArabicTypeInferencer::TypeInferenceResult ArabicTypeInferencer::inferArithmeticType(
        const std::string& expr, const InferenceContext& context) const {

        TypeInferenceResult result;

        if (std::regex_match(expr, arithmeticPattern)) {
            result.primaryType = ValueType::NUMBER;
            result.isDefinite = true;
            result.confidence = 0.95;
            result.reasoning = "عملية حسابية";
        }

        return result;
    }

    ArabicTypeInferencer::TypeInferenceResult ArabicTypeInferencer::inferComparisonType(
        const std::string& expr, const InferenceContext& context) const {

        TypeInferenceResult result;

        // فحص عوامل المقارنة
        if (expr.find("==") != std::string::npos ||
            expr.find("!=") != std::string::npos ||
            expr.find("<") != std::string::npos ||
            expr.find(">") != std::string::npos ||
            expr.find("<=") != std::string::npos ||
            expr.find(">=") != std::string::npos) {

            result.primaryType = ValueType::NUMBER; // في العربية، المقارنات ترجع أرقام
            result.isDefinite = true;
            result.confidence = 1.0;
            result.reasoning = "عملية مقارنة";
        }

        return result;
    }

    bool ArabicTypeInferencer::areTypesCompatible(ValueType from, ValueType to,
                                                 const InferenceContext& context) const {
        if (from == to) return true;

        // قواعد التحويل التلقائي
        if (context.allowTypePromotion) {
            if ((from == ValueType::NUMBER && to == ValueType::STRING) ||
                (from == ValueType::STRING && to == ValueType::NUMBER)) {
                return true;
            }
        }

        return false;
    }

    ValueType ArabicTypeInferencer::suggestTypeConversion(ValueType from, ValueType to) const {
        // اقتراحات بسيطة للتحويل
        if (from == ValueType::NUMBER && to == ValueType::STRING) {
            return ValueType::STRING;
        }
        if (from == ValueType::STRING && to == ValueType::NUMBER) {
            return ValueType::NUMBER;
        }

        return from; // لا تحويل
    }

    void ArabicTypeInferencer::updateVariableType(const std::string& varName, ValueType type,
                                                 InferenceContext& context) const {
        context.variableTypes[varName] = type;
    }

    void ArabicTypeInferencer::clearCache() {
        inferenceCache.clear();
    }

    std::vector<std::string> ArabicTypeInferencer::getSupportedTypesInfo() const {
        return {
            "NUMBER - أرقام",
            "STRING - نصوص",
            "VARIABLE - متغيرات",
            "ARRAY - مصفوفات",
            "FUNCTION_CALL - استدعاءات دوال",
            "OPERATION - عمليات حسابية",
            "COMPARISON - مقارنات"
        };
    }

    std::vector<std::string> ArabicTypeInferencer::diagnoseInference(
        const std::string& expression, const InferenceContext& context) {

        std::vector<std::string> diagnostics;

        TypeInferenceResult result = inferType(expression, context);

        diagnostics.push_back("التعبير: " + expression);
        diagnostics.push_back("النوع المستنتج: " + valueTypeToString(result.primaryType));
        diagnostics.push_back("مستوى الثقة: " + std::to_string(result.confidence));
        diagnostics.push_back("السبب: " + result.reasoning);

        if (!result.isDefinite) {
            diagnostics.push_back("تحذير: النوع غير محدد تماماً");
        }

        return diagnostics;
    }

    // دوال مساعدة في namespace TypeInferenceHelpers

    std::string TypeInferenceHelpers::extractVariableName(const std::string& expr) {
        // استخراج بسيط لاسم المتغير
        size_t pos = 0;
        while (pos < expr.length() && (std::isalnum(expr[pos]) || expr[pos] == '_')) {
            pos++;
        }
        return expr.substr(0, pos);
    }

    std::string TypeInferenceHelpers::extractFunctionName(const std::string& funcCall) {
        size_t parenPos = funcCall.find('(');
        if (parenPos != std::string::npos) {
            return trim(funcCall.substr(0, parenPos));
        }
        return funcCall;
    }

    std::vector<std::string> TypeInferenceHelpers::tokenizeExpression(const std::string& expr) {
        std::vector<std::string> tokens;
        std::string current;

        for (char c : expr) {
            if (c == ' ' || c == '\t') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
            }
            else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '(' || c == ')') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                tokens.push_back(std::string(1, c));
            }
            else {
                current += c;
            }
        }

        if (!current.empty()) {
            tokens.push_back(current);
        }

        return tokens;
    }

    int TypeInferenceHelpers::getOperatorPrecedence(const std::string& op) {
        if (op == "*" || op == "/") return 2;
        if (op == "+" || op == "-") return 1;
        if (op == "=") return 0;
        return -1;
    }

    bool TypeInferenceHelpers::isValidExpression(const std::string& expr) {
        // فحص بسيط لصحة التعبير
        int parenCount = 0;
        for (char c : expr) {
            if (c == '(') parenCount++;
            else if (c == ')') parenCount--;
            if (parenCount < 0) return false;
        }
        return parenCount == 0;
    }

} // namespace ArabicLanguage