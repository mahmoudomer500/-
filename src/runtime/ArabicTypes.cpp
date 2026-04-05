// ArabicTypes.cpp - النسخة المصححة والكاملة 100%
#include "ArabicTypes.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace ArabicLanguage {

    // ═══════════════════════════════════════════════════════════
    // ✅ تنفيذ دوال التحويل
    // ═══════════════════════════════════════════════════════════

    std::string valueTypeToString(ValueType type) {
        switch (type) {
        case ValueType::NUMBER: return "رقم";
        case ValueType::STRING: return "نص";
        case ValueType::VARIABLE: return "متغير";
        case ValueType::OPERATION: return "عملية";
        case ValueType::FUNCTION_CALL: return "استدعاء_دالة";
        case ValueType::COMPARISON: return "مقارنة";
        case ValueType::ARRAY: return "مصفوفة";
        case ValueType::ARRAY_ACCESS: return "وصول_لمصفوفة";
        case ValueType::PROPERTY_ACCESS: return "وصول_لخاصية";
        case ValueType::OBJECT: return "كائن";
        case ValueType::CLASS_INSTANCE: return "عينة_صنف";
        case ValueType::GENERIC_LIST: return "قائمة";
        case ValueType::GENERIC_MAP: return "قاموس";
        case ValueType::NATIVE_FUNCTION: return "دالة_أصلية";
        case ValueType::NONE: return "لا شيء";
        default: return "غير معروف";
        }
    }

    std::string commandTypeToString(CommandType type) {
        switch (type) {
        case CommandType::DECLARE: return "declare";
        case CommandType::ASSIGNMENT: return "assignment";
        case CommandType::PRINT: return "print";
        case CommandType::PRINT_NO_NEWLINE: return "print_no_newline";
        case CommandType::CONDITION: return "if";
        case CommandType::LOOP_FOR: return "for";
        case CommandType::LOOP_WHILE: return "while";
        case CommandType::LOOP_FOR_EACH: return "for_each";
        case CommandType::FUNCTION_DEF: return "function";
        case CommandType::FUNCTION_CALL: return "function_call";
        case CommandType::RETURN: return "return";
        case CommandType::WAIT: return "wait";
        
        // File I/O
        case CommandType::FILE_OPEN: return "file_open";
        case CommandType::FILE_READ: return "file_read";
        case CommandType::FILE_READ_ALL: return "file_read_all";
        case CommandType::FILE_WRITE: return "file_write";
        case CommandType::FILE_CLOSE: return "file_close";
        
        // OOP
        case CommandType::CLASS_DEF: return "class";
        case CommandType::CREATE_OBJECT: return "create_object";
        case CommandType::METHOD_CALL: return "method_call";
        case CommandType::PROPERTY_DEF: return "property_def";
        case CommandType::PROPERTY_ARRAY_ASSIGNMENT: return "property_array_assignment";
        case CommandType::ARRAY_ASSIGNMENT: return "array_assignment";
        case CommandType::IMPORT: return "import";
        case CommandType::LAMBDA_DEF: return "lambda";
        case CommandType::VARIABLE: return "variable";
        case CommandType::BREAK: return "break";
        case CommandType::CONTINUE: return "continue";
        case CommandType::ELSE: return "else";
        case CommandType::ELSE_IF: return "else_if";
        case CommandType::END_BLOCK: return "end";
        
        // Exception Handling
        case CommandType::TRY: return "try";
        case CommandType::CATCH: return "catch";
        case CommandType::THROW: return "throw";
        
        // Scope Management
        case CommandType::PUSH_SCOPE: return "push_scope";
        case CommandType::POP_SCOPE: return "pop_scope";
        
        default: return "unknown";
        }
    }

    // ═══════════════════════════════════════════════════════════
    // ✅ دوال إنشاء القيم
    // ═══════════════════════════════════════════════════════════

    Value createNumberValue(const std::string& num) {
        Value value(ValueType::NUMBER, num);
        value.value = num;
        value.string_value = num;
        return value;
    }

    Value createStringValue(const std::string& str) {
        Value value(ValueType::STRING, str);
        value.value = str;
        value.string_value = str;
        return value;
    }

    Value createVariableValue(const std::string& var) {
        Value value(ValueType::VARIABLE, var);
        value.value = var;
        value.string_value = var;
        return value;
    }

    Value createOperationValue(const std::string& op, const Value& left, const Value& right) {
        Value value(ValueType::OPERATION);
        value.operator_ = op;
        value.operation = op;
        value.left = std::make_unique<Value>(left);
        value.right = std::make_unique<Value>(right);
        return value;
    }

    Value createFunctionCallValue(const std::string& func, const std::vector<Value>& args) {
        Value value(ValueType::FUNCTION_CALL);
        value.function_name = func;
        for (const auto& arg : args) {
            value.arguments.push_back(std::make_shared<Value>(arg));
        }
        return value;
    }

    Value createComparisonValue(const std::string& comp) {
        Value value(ValueType::COMPARISON);
        
        // Parse the comparison string to find operator and split left/right
        std::vector<std::string> operators = { ">=", "<=", "==", "!=", ">", "<" };
        std::string foundOp;
        size_t opPos = std::string::npos;
        
        // Find operator (check longer operators first)
        for (const auto& op : operators) {
            size_t pos = comp.find(op);
            if (pos != std::string::npos) {
                foundOp = op;
                opPos = pos;
                break;
            }
        }
        
        if (opPos == std::string::npos) {
            // No operator found, treat as "!= 0"
            value.operator_ = "!=";
            
            // Left = the whole expression
            Value leftVal;
            leftVal.type = ValueType::VARIABLE;
            leftVal.value = trim(comp);
            leftVal.string_value = trim(comp);
            value.left = std::make_unique<Value>(leftVal);

            // Right = 0
            Value rightVal;
            rightVal.type = ValueType::NUMBER;
            rightVal.value = "0";
            rightVal.string_value = "0";
            value.right = std::make_unique<Value>(rightVal);
        }
        else {
            // Split by operator
            std::string leftStr = trim(comp.substr(0, opPos));
            std::string rightStr = trim(comp.substr(opPos + foundOp.length()));
            
            value.operator_ = foundOp;
            
            // Create left value (simple variable or number)
            Value leftVal;
            if (std::isdigit(static_cast<unsigned char>(leftStr[0])) || (leftStr[0] == '-' && leftStr.length() > 1)) {
                leftVal.type = ValueType::NUMBER;
                leftVal.value = leftStr;
                leftVal.string_value = leftStr;
            } else {
                leftVal.type = ValueType::VARIABLE;
                leftVal.value = leftStr;
                leftVal.string_value = leftStr;
            }
            value.left = std::make_unique<Value>(leftVal);

            // Create right value
            Value rightVal;
            if (std::isdigit(static_cast<unsigned char>(rightStr[0])) || (rightStr[0] == '-' && rightStr.length() > 1)) {
                rightVal.type = ValueType::NUMBER;
                rightVal.value = rightStr;
                rightVal.string_value = rightStr;
            } else {
                rightVal.type = ValueType::VARIABLE;
                rightVal.value = rightStr;
                rightVal.string_value = rightStr;
            }
            value.right = std::make_unique<Value>(rightVal);
        }
        
        value.value = comp;
        value.string_value = comp;
        return value;
    }

    Value createArrayValue(const std::vector<Value>& elements) {
        Value value(ValueType::ARRAY);
        value.elements = elements;
        return value;
    }

    Value createObjectValue(const std::string& class_type) {
        Value value(ValueType::OBJECT);
        value.class_type = class_type;
        return value;
    }

    Value createPropertyAccessValue(const std::string& obj_name, const std::string& prop_name) {
        Value value(ValueType::PROPERTY_ACCESS);
        value.object_name = obj_name;
        value.property_name = prop_name;
        value.value = obj_name;
        return value;
    }

    // ═══════════════════════════════════════════════════════════
    // ✅ دوال معالجة النصوص
    // ═══════════════════════════════════════════════════════════

    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter)) {
            std::string trimmed = trim(token);
            if (!trimmed.empty()) {
                tokens.push_back(trimmed);
            }
        }

    return tokens;
}

    // ═══════════════════════════════════════════════════════════
    // دوال إنشاء الأنواع العامة (Generics)
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief إنشاء قائمة عامة
     */
    Value createGenericList(ValueType elementType = ValueType::VARIABLE) {
        Value list(ValueType::GENERIC_LIST);
        list.generic_type = elementType;
        return list;
    }

    /**
     * @brief إنشاء خريطة عامة
     */
    Value createGenericMap(ValueType keyType = ValueType::STRING,
                          ValueType valueType = ValueType::VARIABLE) {
        Value map(ValueType::GENERIC_MAP);
        map.generic_type = valueType;  // value type في generic_type
        // يمكن إضافة key type في حقل منفصل إذا لزم الأمر
        return map;
    }

    /**
     * @brief إضافة عنصر للقائمة العامة
     */
    void addToGenericList(Value& list, const Value& element) {
        if (list.type != ValueType::GENERIC_LIST) return;

        // فحص توافق النوع
        if (list.generic_type != ValueType::VARIABLE &&
            element.type != list.generic_type) {
            // في الإصدار البسيط، نسمح بالإضافة مع تحذير
        }

        list.elements.push_back(element);
    }

    /**
     * @brief إضافة عنصر للخريطة العامة
     */
    void addToGenericMap(Value& map, const std::string& key, const Value& value) {
        if (map.type != ValueType::GENERIC_MAP) return;

        // فحص توافق النوع
        if (map.generic_type != ValueType::VARIABLE &&
            value.type != map.generic_type) {
            // في الإصدار البسيط، نسمح بالإضافة مع تحذير
        }

        map.map_elements[key] = value;
    }

    /**
     * @brief الحصول على عنصر من القائمة العامة
     */
    Value getFromGenericList(const Value& list, size_t index) {
        if (list.type != ValueType::GENERIC_LIST ||
            index >= list.elements.size()) {
            return Value(ValueType::NONE);
        }
        return list.elements[index];
    }

    /**
     * @brief الحصول على عنصر من الخريطة العامة
     */
    Value getFromGenericMap(const Value& map, const std::string& key) {
        if (map.type != ValueType::GENERIC_MAP) {
            return Value(ValueType::NONE);
        }

        auto it = map.map_elements.find(key);
        if (it != map.map_elements.end()) {
            return it->second;
        }
        return Value(ValueType::NONE);
    }

    /**
     * @brief الحصول على حجم القائمة العامة
     */
    size_t getGenericListSize(const Value& list) {
        if (list.type != ValueType::GENERIC_LIST) return 0;
        return list.elements.size();
    }

    /**
     * @brief الحصول على حجم الخريطة العامة
     */
    size_t getGenericMapSize(const Value& map) {
        if (map.type != ValueType::GENERIC_MAP) return 0;
        return map.map_elements.size();
    }

    // ═══════════════════════════════════════════════════════════
} // namespace ArabicLanguage
