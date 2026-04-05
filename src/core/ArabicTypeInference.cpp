#include "ArabicTypeInference.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // 🎯 تنفيذ نظام استنتاج الأنواع
    // ══════════════════════════════════════════════════════════════

    // Singleton instance
    ArabicTypeInference& ArabicTypeInference::getInstance() {
        static ArabicTypeInference instance;
        return instance;
    }

    ArabicTypeInference::ArabicTypeInference() = default;

    // ══════════════════════════════════════════════════════════════
    // 📊 تنفيذ بنية معلومات النوع
    // ══════════════════════════════════════════════════════════════

    std::string ArabicTypeInference::TypeInfo::toString() const {
        std::stringstream ss;
        ss << name;

        if (isGeneric) {
            ss << "<generic>";
        }

        if (isNullable) {
            ss << "?";
        }

        if (!constraints.empty()) {
            ss << " : ";
            for (size_t i = 0; i < constraints.size(); ++i) {
                if (i > 0) ss << " + ";
                ss << constraints[i];
            }
        }

        if (superType) {
            ss << " extends " << *superType;
        }

        return ss.str();
    }

    bool ArabicTypeInference::TypeInfo::isCompatibleWith(const TypeInfo& other) const {
        // فحص التوافق الأساسي
        if (name == other.name) return true;

        // فحص الوراثة
        if (superType && *superType == other.name) return true;
        if (other.superType && *other.superType == name) return true;

        // فحص الأنواع الأساسية
        if (baseType == other.baseType) return true;

        // فحص التوافق العددي
        if ((baseType == ValueType::NUMBER && other.baseType == ValueType::NUMBER) ||
            (baseType == ValueType::STRING && other.baseType == ValueType::STRING)) {
            return true;
        }

        return false;
    }

    bool ArabicTypeInference::TypeInfo::canBeAssignedFrom(const TypeInfo& other) const {
        if (isCompatibleWith(other)) return true;

        // قواعد تعيين خاصة
        if (isNullable && other.name == "null") return true;
        if (baseType == ValueType::NUMBER && other.baseType == ValueType::NUMBER) return true;

        return false;
    }

    ArabicTypeInference::TypeInfo ArabicTypeInference::TypeInfo::getCommonType(const TypeInfo& other) const {
        if (name == other.name) return *this;

        // العثور على نوع مشترك
        if (superType && *superType == other.name) return other;
        if (other.superType && *other.superType == name) return *this;

        // نوع عام
        TypeInfo common("any", ValueType::VARIABLE);
        return common;
    }

    // ══════════════════════════════════════════════════════════════
    // 🔍 تنفيذ مستنتج الأنواع
    // ══════════════════════════════════════════════════════════════

    ArabicTypeInference::InferenceResult ArabicTypeInference::TypeInferencer::inferFromValue(const Value& value) {
        TypeInfo typeInfo;

        switch (value.type) {
            case ValueType::NUMBER:
                typeInfo = TypeInfo("رقم", ValueType::NUMBER);
                break;
            case ValueType::STRING:
                typeInfo = TypeInfo("نص", ValueType::STRING);
                break;
            case ValueType::VARIABLE:
                return inferFromVariable(value.value);
            case ValueType::ARRAY:
                {
                    std::vector<TypeInfo> elementTypes;
                    for (const auto& elem : value.elements) {
                        elementTypes.push_back(inferFromValue(elem).inferredType);
                    }
                    return inferFromArrayLiteral(elementTypes);
                }
            case ValueType::OBJECT:
                typeInfo = TypeInfo("كائن", ValueType::OBJECT);
                break;
            default:
                typeInfo = TypeInfo("غير_معروف", ValueType::NONE);
                break;
        }

        return InferenceResult(typeInfo);
    }

    ArabicTypeInference::InferenceResult ArabicTypeInference::TypeInferencer::inferFromExpression(
        const std::string& expression,
        const std::unordered_map<std::string, TypeInfo>& context) {

        // تبسيط للعرض - في الإصدار الكامل سيتم تحليل أعمق
        if (expression.find('+') != std::string::npos ||
            expression.find('-') != std::string::npos ||
            expression.find('*') != std::string::npos ||
            expression.find('/') != std::string::npos) {
            // عملية حسابية
            return InferenceResult(TypeInfo("رقم", ValueType::NUMBER), 0.8);
        }

        if (expression.find('"') != std::string::npos ||
            expression.find('\'') != std::string::npos) {
            // سلسلة نصية
            return InferenceResult(TypeInfo("نص", ValueType::STRING), 0.9);
        }

        if (expression == "صحيح" || expression == "خطأ") {
            // منطقي
            return InferenceResult(TypeInfo("منطق", ValueType::VARIABLE), 1.0);
        }

        // متغير أو استدعاء دالة
        if (context.count(expression)) {
            return InferenceResult(context.at(expression), 0.7);
        }

        return InferenceResult(TypeInfo("غير_معروف", ValueType::NONE), 0.1);
    }

    ArabicTypeInference::InferenceResult ArabicTypeInference::TypeInferencer::inferFromFunctionCall(
        const std::string& functionName, const std::vector<TypeInfo>& argTypes) {

        // دوال معروفة
        if (functionName == "طول" || functionName == "length") {
            return InferenceResult(TypeInfo("رقم", ValueType::NUMBER), 1.0);
        }

        if (functionName == "إضافة" || functionName == "push") {
            return InferenceResult(TypeInfo("صفر", ValueType::NONE), 0.9);
        }

        // دالة عامة
        InferenceResult result(TypeInfo("غير_معروف", ValueType::VARIABLE), 0.5);
        result.assumptions.push_back("نوع الإرجاع للدالة " + functionName + " غير محدد");
        return result;
    }

    ArabicTypeInference::InferenceResult ArabicTypeInference::TypeInferencer::inferFromBinaryOp(
        const std::string& op, const TypeInfo& leftType, const TypeInfo& rightType) {

        TypeInfo resultType = getBinaryOpResultType(op, leftType, rightType);

        double confidence = 0.9;
        if (leftType.name != rightType.name) {
            confidence = 0.7; // انخفاض الثقة للأنواع المختلفة
        }

        InferenceResult result(resultType, confidence);
        if (confidence < 0.9) {
            result.assumptions.push_back("تحويل تلقائي بين " + leftType.name + " و " + rightType.name);
        }

        return result;
    }

    ArabicTypeInference::InferenceResult ArabicTypeInference::TypeInferencer::inferFromUnaryOp(
        const std::string& op, const TypeInfo& operandType) {

        if (op == "!" || op == "ليس") {
            return InferenceResult(TypeInfo("منطق", ValueType::VARIABLE), 1.0);
        }

        if (op == "-" || op == "+") {
            if (operandType.baseType == ValueType::NUMBER) {
                return InferenceResult(operandType, 1.0);
            }
        }

        return InferenceResult(operandType, 0.8);
    }

    bool ArabicTypeInference::TypeInferencer::resolveConstraints() {
        // تبسيط - في الإصدار الكامل سيتم حل القيود فعلياً
        return activeConstraints.empty();
    }

    bool ArabicTypeInference::TypeInferencer::checkTypeCompatibility(const TypeInfo& source, const TypeInfo& target) {
        return source.canBeAssignedFrom(target);
    }

    void ArabicTypeInference::TypeInferencer::addType(const std::string& name, const TypeInfo& type) {
        typeTable[name] = type;
    }

    void ArabicTypeInference::TypeInferencer::addVariable(const std::string& name, const InferenceResult& result) {
        variableTypes[name] = result;
    }

    std::optional<ArabicTypeInference::TypeInfo> ArabicTypeInference::TypeInferencer::getType(const std::string& name) const {
        auto it = typeTable.find(name);
        if (it != typeTable.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::optional<ArabicTypeInference::InferenceResult> ArabicTypeInference::TypeInferencer::getVariableType(const std::string& name) const {
        auto it = variableTypes.find(name);
        if (it != variableTypes.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    ArabicTypeInference::InferenceResult ArabicTypeInference::TypeInferencer::inferFromLiteral(const std::string& literal) {
        // استنتاج من قيمة حرفية
        if (std::regex_match(literal, std::regex(R"(\d+)"))) {
            return InferenceResult(TypeInfo("رقم", ValueType::NUMBER), 1.0);
        }

        if (std::regex_match(literal, std::regex(R"(".*")"))) {
            return InferenceResult(TypeInfo("نص", ValueType::STRING), 1.0);
        }

        if (literal == "صحيح" || literal == "خطأ") {
            return InferenceResult(TypeInfo("منطق", ValueType::VARIABLE), 1.0);
        }

        return InferenceResult(TypeInfo("غير_معروف", ValueType::NONE), 0.1);
    }

    ArabicTypeInference::InferenceResult ArabicTypeInference::TypeInferencer::inferFromVariable(const std::string& varName) {
        if (auto varType = getVariableType(varName)) {
            return *varType;
        }

        InferenceResult result(TypeInfo("غير_معروف", ValueType::VARIABLE), 0.3);
        result.assumptions.push_back("نوع المتغير " + varName + " غير محدد");
        return result;
    }

    ArabicTypeInference::InferenceResult ArabicTypeInference::TypeInferencer::inferFromArrayLiteral(
        const std::vector<TypeInfo>& elementTypes) {

        if (elementTypes.empty()) {
            TypeInfo arrayType("مصفوفة", ValueType::ARRAY);
            arrayType.properties["عنصر"] = "غير_معروف";
            return InferenceResult(arrayType, 0.8);
        }

        // العثور على نوع مشترك للعناصر
        TypeInfo commonType = elementTypes[0];
        for (size_t i = 1; i < elementTypes.size(); ++i) {
            commonType = commonType.getCommonType(elementTypes[i]);
        }

        TypeInfo arrayType("مصفوفة", ValueType::ARRAY);
        arrayType.properties["عنصر"] = commonType.name;

        double confidence = elementTypes.size() == 1 ? 1.0 : 0.8;
        return InferenceResult(arrayType, confidence);
    }

    ArabicTypeInference::InferenceResult ArabicTypeInference::TypeInferencer::inferFromObjectLiteral(
        const std::unordered_map<std::string, TypeInfo>& props) {

        TypeInfo objectType("كائن", ValueType::OBJECT);
        for (const auto& prop : props) {
            objectType.properties[prop.first] = prop.second.name;
        }

        return InferenceResult(objectType, 1.0);
    }

    ArabicTypeInference::TypeInfo ArabicTypeInference::TypeInferencer::getBinaryOpResultType(
        const std::string& op, const TypeInfo& left, const TypeInfo& right) {

        // عمليات حسابية
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            if (left.baseType == ValueType::NUMBER && right.baseType == ValueType::NUMBER) {
                return TypeInfo("رقم", ValueType::NUMBER);
            }
            if (left.baseType == ValueType::STRING || right.baseType == ValueType::STRING) {
                return TypeInfo("نص", ValueType::STRING);
            }
        }

        // عمليات مقارنة
        if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
            return TypeInfo("منطق", ValueType::VARIABLE);
        }

        // عمليات منطقية
        if (op == "&&" || op == "||" || op == "و" || op == "أو") {
            return TypeInfo("منطق", ValueType::VARIABLE);
        }

        return TypeInfo("غير_معروف", ValueType::NONE);
    }

    bool ArabicTypeInference::TypeInferencer::unifyTypes(TypeInfo& t1, const TypeInfo& t2) {
        if (t1.name == t2.name) return true;

        // محاولة توحيد الأنواع
        if (t1.name == "غير_معروف") {
            t1 = t2;
            return true;
        }

        if (t2.name == "غير_معروف") {
            return true;
        }

        return t1.isCompatibleWith(t2);
    }

    // ══════════════════════════════════════════════════════════════
    // ✅ تنفيذ فاحص الأنواع
    // ══════════════════════════════════════════════════════════════

    bool ArabicTypeInference::TypeChecker::checkCommands(const std::vector<std::shared_ptr<Command>>& commands) {
        std::unordered_map<std::string, TypeInfo> context;
        bool success = true;

        for (const auto& command : commands) {
            if (!checkCommand(command, context)) {
                success = false;
                if (strictMode) break;
            }
        }

        return success;
    }

    bool ArabicTypeInference::TypeChecker::checkExpression(
        const std::string& expression,
        const std::unordered_map<std::string, TypeInfo>& context) {

        auto result = inferencer.inferFromExpression(expression, context);

        if (result.confidence < 0.5) {
            addWarning("استنتاج نوع غير مؤكد للتعبير: " + expression);
        }

        return result.confidence >= 0.3; // حد أدنى للقبول
    }

    bool ArabicTypeInference::TypeChecker::checkFunctionCall(
        const std::string& functionName,
        const std::vector<std::string>& argExpressions,
        const std::unordered_map<std::string, TypeInfo>& context) {

        std::vector<TypeInfo> argTypes;
        for (const auto& arg : argExpressions) {
            auto result = inferencer.inferFromExpression(arg, context);
            argTypes.push_back(result.inferredType);
        }

        auto result = inferencer.inferFromFunctionCall(functionName, argTypes);

        if (result.confidence < 0.7) {
            addWarning("استدعاء دالة غير مؤكد: " + functionName);
        }

        return result.confidence >= 0.5;
    }

    bool ArabicTypeInference::TypeChecker::checkAssignment(
        const std::string& varName,
        const std::string& expression,
        const std::unordered_map<std::string, TypeInfo>& context) {

        auto exprResult = inferencer.inferFromExpression(expression, context);

        if (context.count(varName)) {
            const auto& varType = context.at(varName);
            if (!inferencer.checkTypeCompatibility(varType, exprResult.inferredType)) {
                addError("تعيين غير متوافق: " + varType.name + " = " + exprResult.inferredType.name);
                return false;
            }
        } else {
            // إضافة المتغير للسياق
            const_cast<std::unordered_map<std::string, TypeInfo>&>(context)[varName] = exprResult.inferredType;
            inferencer.addVariable(varName, exprResult);
        }

        return true;
    }

    void ArabicTypeInference::TypeChecker::clearMessages() {
        errors.clear();
        warnings.clear();
    }

    bool ArabicTypeInference::TypeChecker::checkCommand(
        const std::shared_ptr<Command>& command,
        std::unordered_map<std::string, TypeInfo>& context) {

        switch (command->type) {
            case CommandType::ASSIGNMENT:
                return checkAssignment("var", "value", context); // تبسيط

            case CommandType::CONDITION:
                return checkIfStatement(command, context);

            case CommandType::LOOP_WHILE:
            case CommandType::LOOP_FOR:
                return checkLoop(command, context);

            case CommandType::FUNCTION_DEF:
                return checkFunctionDefinition(command, context);

            default:
                return true; // أوامر أخرى
        }
    }

    bool ArabicTypeInference::TypeChecker::checkIfStatement(
        const std::shared_ptr<Command>& command,
        std::unordered_map<std::string, TypeInfo>& context) {

        // فحص الشرط
        return checkExpression("condition", context); // تبسيط
    }

    bool ArabicTypeInference::TypeChecker::checkLoop(
        const std::shared_ptr<Command>& command,
        std::unordered_map<std::string, TypeInfo>& context) {

        // فحص الشرط والجسم
        return checkExpression("condition", context); // تبسيط
    }

    bool ArabicTypeInference::TypeChecker::checkFunctionDefinition(
        const std::shared_ptr<Command>& command,
        std::unordered_map<std::string, TypeInfo>& context) {

        // فحص تعريف الدالة
        return true; // تبسيط
    }

    void ArabicTypeInference::TypeChecker::addError(const std::string& message, int line) {
        std::string fullMessage = message;
        if (line >= 0) {
            fullMessage = "السطر " + std::to_string(line) + ": " + message;
        }
        errors.push_back(fullMessage);
    }

    void ArabicTypeInference::TypeChecker::addWarning(const std::string& message, int line) {
        std::string fullMessage = message;
        if (line >= 0) {
            fullMessage = "تحذير - السطر " + std::to_string(line) + ": " + message;
        }
        warnings.push_back(fullMessage);
    }

    // ══════════════════════════════════════════════════════════════
    // 📚 تنفيذ مكتبة الأنواع القياسية
    // ══════════════════════════════════════════════════════════════

    ArabicTypeInference::StandardTypeLibrary::StandardTypeLibrary() {
        // إضافة الأنواع الأساسية
        addBuiltinType("رقم", TypeInfo("رقم", ValueType::NUMBER));
        addBuiltinType("نص", TypeInfo("نص", ValueType::STRING));
        addBuiltinType("منطق", TypeInfo("منطق", ValueType::VARIABLE));
        addBuiltinType("مصفوفة", TypeInfo("مصفوفة", ValueType::ARRAY));
        addBuiltinType("كائن", TypeInfo("كائن", ValueType::OBJECT));

        // إضافة أنواع مركبة
        TypeInfo listType("قائمة", ValueType::ARRAY);
        listType.isGeneric = true;
        addBuiltinType("قائمة", listType);

        TypeInfo mapType("خريطة", ValueType::OBJECT);
        mapType.isGeneric = true;
        addBuiltinType("خريطة", mapType);
    }

    void ArabicTypeInference::StandardTypeLibrary::addBuiltinType(const std::string& name, const TypeInfo& type) {
        builtinTypes[name] = type;
    }

    std::optional<ArabicTypeInference::TypeInfo> ArabicTypeInference::StandardTypeLibrary::getBuiltinType(const std::string& name) const {
        auto it = builtinTypes.find(name);
        if (it != builtinTypes.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool ArabicTypeInference::StandardTypeLibrary::isBuiltinType(const std::string& name) const {
        return builtinTypes.count(name) > 0;
    }

    std::vector<std::string> ArabicTypeInference::StandardTypeLibrary::getAllBuiltinTypes() const {
        std::vector<std::string> types;
        for (const auto& pair : builtinTypes) {
            types.push_back(pair.first);
        }
        return types;
    }

    ArabicTypeInference::TypeInfo ArabicTypeInference::StandardTypeLibrary::createArrayType(const TypeInfo& elementType) {
        TypeInfo arrayType("مصفوفة", ValueType::ARRAY);
        arrayType.properties["عنصر"] = elementType.name;
        return arrayType;
    }

    ArabicTypeInference::TypeInfo ArabicTypeInference::StandardTypeLibrary::createFunctionType(
        const std::vector<TypeInfo>& paramTypes, const TypeInfo& returnType) {

        TypeInfo funcType("دالة", ValueType::FUNCTION_CALL);
        funcType.properties["إرجاع"] = returnType.name;

        for (size_t i = 0; i < paramTypes.size(); ++i) {
            funcType.properties["معامل_" + std::to_string(i)] = paramTypes[i].name;
        }

        return funcType;
    }

    ArabicTypeInference::TypeInfo ArabicTypeInference::StandardTypeLibrary::createOptionalType(const TypeInfo& baseType) {
        TypeInfo optionalType = baseType;
        optionalType.isNullable = true;
        optionalType.name += "?";
        return optionalType;
    }

    // ══════════════════════════════════════════════════════════════
    // 🎯 تنفيذ واجهة الاستخدام العامة
    // ══════════════════════════════════════════════════════════════

    ArabicTypeInference::InferenceResult ArabicTypeInference::inferType(
        const std::string& code,
        const std::unordered_map<std::string, TypeInfo>& context) {

        return inferencer.inferFromExpression(code, context);
    }

    bool ArabicTypeInference::checkTypes(
        const std::vector<std::shared_ptr<Command>>& commands,
        std::vector<std::string>& errors,
        std::vector<std::string>& warnings) {

        bool success = checker.checkCommands(commands);

        errors = checker.getErrors();
        warnings = checker.getWarnings();

        return success;
    }

    void ArabicTypeInference::optimizeTypes(std::vector<std::shared_ptr<Command>>& commands) {
        // تحسين الأنواع - إزالة الأنواع غير المستخدمة (تبسيط)
        // في الإصدار الكامل سيتم تحليل استخدام الأنواع
    }

    std::string ArabicTypeInference::addTypeAnnotations(const std::string& code) {
        // إضافة تعليقات نوع للكود (تبسيط)
        std::string annotated = "// نوع مستنتج: غير_معروف\n";
        annotated += code;
        return annotated;
    }

    std::vector<std::string> ArabicTypeInference::getStats() const {
        std::vector<std::string> stats;
        stats.push_back("=== إحصائيات نظام استنتاج الأنواع ===");

        auto builtinTypes = stdLibrary.getAllBuiltinTypes();
        stats.push_back("الأنواع القياسية: " + std::to_string(builtinTypes.size()));

        stats.push_back("أخطاء فحص الأنواع: " + std::to_string(checker.getErrors().size()));
        stats.push_back("تحذيرات فحص الأنواع: " + std::to_string(checker.getWarnings().size()));

        return stats;
    }

    void ArabicTypeInference::reset() {
        checker.clearMessages();
    }

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة لاستنتاج الأنواع
    // ══════════════════════════════════════════════════════════════

    ArabicTypeInference::InferenceResult quickTypeInference(const Value& value) {
        return ArabicTypeInference::getInstance().getInferencer().inferFromValue(value);
    }

    bool areTypesCompatible(const ArabicTypeInference::TypeInfo& t1, const ArabicTypeInference::TypeInfo& t2) {
        return t1.isCompatibleWith(t2);
    }

    ValueType stringToValueType(const std::string& typeStr) {
        if (typeStr == "رقم") return ValueType::NUMBER;
        if (typeStr == "نص") return ValueType::STRING;
        if (typeStr == "منطق") return ValueType::VARIABLE;
        if (typeStr == "مصفوفة") return ValueType::ARRAY;
        if (typeStr == "كائن") return ValueType::OBJECT;
        return ValueType::NONE;
    }

    std::string valueTypeToArabicString(ValueType type) {
        switch (type) {
            case ValueType::NUMBER: return "رقم";
            case ValueType::STRING: return "نص";
            case ValueType::VARIABLE: return "منطق";
            case ValueType::ARRAY: return "مصفوفة";
            case ValueType::OBJECT: return "كائن";
            default: return "غير_معروف";
        }
    }

    ArabicTypeInference::TypeInfo createGenericType(const std::string& name, const std::vector<std::string>& typeParams) {
        ArabicTypeInference::TypeInfo type(name, ValueType::VARIABLE);
        type.isGeneric = true;

        std::string paramsStr;
        for (size_t i = 0; i < typeParams.size(); ++i) {
            if (i > 0) paramsStr += ",";
            paramsStr += typeParams[i];
        }

        type.properties["parameters"] = paramsStr;
        return type;
    }

    ArabicTypeInference::TypeInfo specializeGenericType(
        const ArabicTypeInference::TypeInfo& genericType,
        const std::vector<std::string>& typeArgs) {

        if (!genericType.isGeneric) {
            return genericType;
        }

        ArabicTypeInference::TypeInfo specialized = genericType;
        specialized.isGeneric = false;
        specialized.name += "<";

        for (size_t i = 0; i < typeArgs.size(); ++i) {
            if (i > 0) specialized.name += ",";
            specialized.name += typeArgs[i];
        }
        specialized.name += ">";

        return specialized;
    }

} // namespace ArabicLanguage
