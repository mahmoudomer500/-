// ArabicTypes.h - تعريف الأنواع الأساسية للغة العربية
// Core types and definitions for the Arabic Programming Language

#ifndef ARABIC_TYPES_H
#define ARABIC_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <any>
#include <functional>
#include <algorithm>
#include <cstdint>

using namespace std;

// -----------------------------------------------------------
// حل تعارض الأسماء مع Windows API
// -----------------------------------------------------------
#ifdef Value
#undef Value
#endif
#ifdef Command
#undef Command
#endif
#ifdef BOOLEAN
#undef BOOLEAN
#endif
#ifdef STRING
#undef STRING
#endif
#ifdef NUMBER
#undef NUMBER
#endif
#ifdef FILE_OPEN
#undef FILE_OPEN
#endif
#ifdef FILE_READ
#undef FILE_READ
#endif
#ifdef FILE_WRITE
#undef FILE_WRITE
#endif
#ifdef FILE_CLOSE
#undef FILE_CLOSE
#endif
#ifdef DELETE
#undef DELETE
#endif

namespace ArabicLanguage {
    // -----------------------------------------------------------
    // أنواع القيم المدعومة
    // -----------------------------------------------------------
    enum class ValueType {
        NONE,
        NUMBER,
        STRING,
        BOOLEAN,
        ARRAY,
        OBJECT,
        FUNCTION,
        NATIVE_FUNCTION,
        CLASS_DEFINITION,
        INTERFACE_DEFINITION,
        NULL_VALUE,
        GENERIC_LIST,
        GENERIC_MAP,
        CLASS_INSTANCE,
        VARIABLE,
        OPERATION,
        FUNCTION_CALL,
        COMPARISON,
        ARRAY_ACCESS,
        PROPERTY_ACCESS,
        ARRAY_LITERAL,
        OBJECT_LITERAL,
        SLICE
    };

    struct ArabicValue;
    struct ArabicCommand;
    
    using Value = ArabicLanguage::ArabicValue;
    using Command = ArabicLanguage::ArabicCommand;

    struct InterfaceDefinition;
    struct ClassDefinition;
    
    // تعريف نوع الدالة الأصلية
    using NativeFunction = std::function<ArabicLanguage::ArabicValue(const std::vector<ArabicLanguage::ArabicValue>&)>;

    // -----------------------------------------------------------
    // دالة مساعدة لإزالة المسافات (تدعم UTF-8)
    // -----------------------------------------------------------
    inline std::string trim(const std::string& str) {
        if (str.empty()) return str;
        
        // إزالة BOM إذا وجد في البداية
        std::string s = str;
        if (s.length() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) {
            s = s.substr(3);
        }

        if (s.empty()) return s;

        // دالة للتحقق مما إذا كان البايت جزءاً من مسافة بيضاء UTF-8
        auto isUtf8Space = [&](size_t pos) -> size_t {
            unsigned char c = (unsigned char)s[pos];
            if (c <= 32) return 1; // ASCII spaces and controls
            
            // Non-breaking space (U+00A0) in UTF-8: C2 A0
            if (c == 0xC2 && pos + 1 < s.length() && (unsigned char)s[pos+1] == 0xA0) return 2;
            
            // Zero-width spaces and other Unicode spaces (U+2000 to U+200F)
            if (c == 0xE2 && pos + 2 < s.length() && (unsigned char)s[pos+1] == 0x80) {
                unsigned char c2 = (unsigned char)s[pos+2];
                if (c2 >= 0x80 && c2 <= 0x8F) return 3;
            }
            
            return 0;
        };

        // البحث عن أول حرف غير مسافة
        size_t first = 0;
        while (first < s.length()) {
            size_t spaceLen = isUtf8Space(first);
            if (spaceLen == 0) break;
            first += spaceLen;
        }

        if (first == s.length()) return "";

        // البحث عن آخر حرف غير مسافة
        size_t last = s.length() - 1;
        while (last >= first) {
            unsigned char c = (unsigned char)s[last];
            bool foundSpace = false;
            
            if (c <= 32) {
                if (last == 0) { first = 1; break; } // All spaces
                last--;
                foundSpace = true;
            } else if (last >= 1 && (unsigned char)s[last] == 0xA0 && (unsigned char)s[last-1] == 0xC2) {
                last -= 2;
                foundSpace = true;
            } else if (last >= 2 && (unsigned char)s[last-2] == 0xE2 && (unsigned char)s[last-1] == 0x80) {
                unsigned char c3 = (unsigned char)s[last];
                if (c3 >= 0x80 && c3 <= 0x8F) {
                    last -= 3;
                    foundSpace = true;
                }
            }
            
            if (!foundSpace) break;
        }

        if (last < first && first > 0) return "";
        return s.substr(first, (last - first + 1));
    }

    // تعريف نوع دالة رد النداء لـ SIMD
    typedef void (*SIMDCallback)(double*, double*, double*, size_t);

    // -----------------------------------------------------------
    // بنية القيمة (ArabicValue)
    // -----------------------------------------------------------
    struct ArabicValue {
        ArabicLanguage::ValueType type;
        double number_value;
        std::string string_value;
        bool bool_value;
        std::vector<ArabicLanguage::ArabicValue> elements;
        std::map<std::string, ArabicLanguage::ArabicValue> map_elements;
        std::string function_name;
        std::string object_name;
        std::string class_type;
        std::string operator_;
        std::string operation;
        std::string property_name;
        std::vector<std::shared_ptr<ArabicLanguage::ArabicValue>> arguments;
        NativeFunction native_callback;
        std::vector<uint8_t> blob_value; // ✅ For binary data (Images, etc.)
        
        // روابط للأشجار (للاستخدام في المحلل)
        std::shared_ptr<ArabicLanguage::ArabicValue> left;
        std::shared_ptr<ArabicLanguage::ArabicValue> right;
        std::shared_ptr<ArabicLanguage::ArabicValue> index; // ✅ For array access
        std::string value; // القيمة النصية الأصلية
        
        bool is_new_object;
        ArabicLanguage::ValueType generic_type;

        ArabicValue() : type(ArabicLanguage::ValueType::NONE), number_value(0), bool_value(false), is_new_object(false), generic_type(ArabicLanguage::ValueType::NONE) {}
        
        ArabicValue(ArabicLanguage::ValueType t) : type(t), number_value(0), bool_value(false), is_new_object(false), generic_type(ArabicLanguage::ValueType::NONE) {}
        
        ArabicValue(ArabicLanguage::ValueType t, const std::string& v) : type(t), value(v), number_value(0), bool_value(false), is_new_object(false), generic_type(ArabicLanguage::ValueType::NONE) {
            if (t == ArabicLanguage::ValueType::NUMBER) {
                try { number_value = std::stod(v); } catch (...) { number_value = 0; }
            } else if (t == ArabicLanguage::ValueType::STRING) {
                string_value = v;
            } else if (t == ArabicLanguage::ValueType::BOOLEAN) {
                string_value = v;
                bool_value = (v == "1" || v == "true");
            }
        }
        
        ArabicValue(double v) : type(ArabicLanguage::ValueType::NUMBER), number_value(v), value(std::to_string(v)), bool_value(false), is_new_object(false), generic_type(ArabicLanguage::ValueType::NONE) {}
        
        ArabicValue(const std::string& v) : type(ArabicLanguage::ValueType::STRING), string_value(v), value(v), number_value(0), bool_value(false), is_new_object(false), generic_type(ArabicLanguage::ValueType::NONE) {}
        
        ArabicValue(bool v) : type(ArabicLanguage::ValueType::BOOLEAN), bool_value(v), number_value(0), is_new_object(false), generic_type(ArabicLanguage::ValueType::NONE) {}

        ArabicValue(const std::vector<uint8_t>& v) : type(ArabicLanguage::ValueType::OBJECT), blob_value(v), number_value(0), bool_value(false), is_new_object(false), generic_type(ArabicLanguage::ValueType::NONE) {}
    };

    // -----------------------------------------------------------
    // أنواع الأوامر (CommandType)
    // -----------------------------------------------------------
    enum class CommandType {
        UNKNOWN,
        DECLARE,
        ASSIGN,
        ASSIGNMENT,
        PRINT,
        PRINT_NO_NEWLINE,
        IF,
        CONDITION,
        WHILE,
        LOOP_WHILE,
        FOR,
        LOOP_FOR,
        LOOP_FOR_EACH,
        FUNCTION_DEF,
        FUNCTION_CALL,
        RETURN,
        CALL,
        CLASS_DEF,
        IMPORT,
        TRY_CATCH,
        TRY,
        CATCH,
        THROW,
        WAIT,
        FILE_OPEN,
        FILE_READ,
        FILE_READ_ALL,
        FILE_WRITE,
        FILE_CLOSE,
        CREATE_OBJECT,
        METHOD_CALL,
        PROPERTY_DEF,
        PROPERTY_ARRAY_ASSIGNMENT,
        ARRAY_ASSIGNMENT,
        LAMBDA_DEF,
        VARIABLE,
        BREAK,
        CONTINUE,
        ELSE,
        ELSE_IF,
        END_BLOCK,
        INTERFACE_DEF,
        INTERFACE_DEFINITION,
        CLASS_METHOD_DEF,
        PUSH_SCOPE,
        POP_SCOPE
    };

    // -----------------------------------------------------------
    // دوال التحويل المساعدة
    // -----------------------------------------------------------
    std::string valueTypeToString(ArabicLanguage::ValueType type);
    std::string commandTypeToString(ArabicLanguage::CommandType type);

    // -----------------------------------------------------------
    // بنية الأمر (ArabicCommand)
    // -----------------------------------------------------------
    struct ArabicCommand {
        ArabicLanguage::CommandType type;
        std::string name;
        std::string class_name;
        std::string object_name;
        std::string method_name;
        std::string function_name;
        std::string variable;
        std::string library;
        std::string interface_name;
        std::string property_name;   // ✅ For property assignment/access in advanced CodeGenerator
        std::string parent_class_name; // ✅ For inheritance support in advanced CodeGenerator
        ArabicLanguage::ArabicValue value;
        std::vector<std::string> parameters;
        std::shared_ptr<ArabicLanguage::ArabicCommand> initializer;
        std::shared_ptr<ArabicLanguage::ArabicCommand> increment;
        std::shared_ptr<ArabicLanguage::InterfaceDefinition> interface_def;
        std::shared_ptr<ArabicLanguage::ClassDefinition> class_def;
        std::shared_ptr<ArabicLanguage::ArabicValue> condition;
        std::vector<std::shared_ptr<ArabicLanguage::ArabicValue>> arguments;
        std::vector<std::shared_ptr<ArabicLanguage::ArabicCommand>> body;
        std::vector<std::shared_ptr<ArabicLanguage::ArabicCommand>> else_body;
        std::shared_ptr<ArabicLanguage::ArabicValue> expression;
        std::string lambda_params; // ✅ For lambda support in advanced CodeGenerator
        bool isConst;               // ✅ Support variable constants

        
        ArabicCommand() : type(ArabicLanguage::CommandType::UNKNOWN), isConst(false) {}
        ArabicCommand(ArabicLanguage::CommandType t) : type(t), isConst(false) {}
    };

    // -----------------------------------------------------------
    // أسماء بديلة لتسهيل الاستخدام
    // -----------------------------------------------------------
    // (Value and Command are now defined at the top of the namespace)

    // -----------------------------------------------------------
    // تعريفات البرمجة الكائنية
    // -----------------------------------------------------------
    struct PropertyInfo {
        std::string name;
        ArabicLanguage::ValueType type;
        std::shared_ptr<ArabicLanguage::ArabicValue> default_value;
        PropertyInfo() : type(ArabicLanguage::ValueType::NUMBER) {}
        PropertyInfo(const std::string& n, ArabicLanguage::ValueType t) : name(n), type(t) {}
    };

    struct ClassMethod {
        std::string name;
        std::vector<std::string> parameters;
        std::vector<std::shared_ptr<ArabicCommand>> body;
        bool is_static;
        bool is_abstract;
        ArabicLanguage::ValueType return_type;
        ClassMethod() : is_static(false), is_abstract(false), return_type(ArabicLanguage::ValueType::NONE) {}
    };

    struct InterfaceDefinition {
        std::string name;
        std::vector<std::string> methods;
        InterfaceDefinition() {}
        InterfaceDefinition(const std::string& name) : name(name) {}
    };

    struct ClassDefinition {
        std::string name;
        std::string class_name;
        std::string base_class;
        std::string parent_class;
        bool is_abstract;
        std::vector<std::string> interfaces;
        std::map<std::string, PropertyInfo> properties;
        std::map<std::string, ArabicLanguage::ArabicValue> default_values;
        std::map<std::string, ClassMethod> methods;
        std::map<std::string, ClassMethod*> virtual_methods;
        ClassDefinition() : is_abstract(false) {}
        ClassDefinition(const std::string& name) : name(name), class_name(name), is_abstract(false) {}
    };

    struct ObjectInstance {
        std::string class_name;
        std::string class_type;
        std::string unique_id;
        std::map<std::string, std::shared_ptr<ArabicLanguage::ArabicValue>> properties;
        ObjectInstance() {}
    };

    // -----------------------------------------------------------
    // مدير الأصناف
    // -----------------------------------------------------------
    class ClassManager {
    public:
        ClassManager() : object_counter(0), verboseOutput(false) {}
        
        void reset();
        
        bool defineClass(const std::string& class_name, const ClassDefinition& class_def);
        bool defineInterface(const std::string& interface_name, const InterfaceDefinition& interface_def);
        
        bool classExists(const std::string& name) const;
        bool interfaceExists(const std::string& name) const;
        
        bool hasMethod(const std::string& className, const std::string& methodName) const;
        bool hasProperty(const std::string& class_name, const std::string& prop_name) const;
        
        std::shared_ptr<ObjectInstance> createObject(const std::string& classType);
        
        ArabicLanguage::Value getProperty(const ObjectInstance& obj, const std::string& prop_name);
        void setProperty(ObjectInstance& obj, const std::string& prop_name, const ArabicLanguage::Value& val);
        
        ArabicLanguage::Value callMethod(ObjectInstance& obj, const std::string& method_name, const std::vector<ArabicLanguage::Value>& args);
        
        const ClassDefinition* getClassInfo(const std::string& class_name) const;
        const InterfaceDefinition* getInterfaceInfo(const std::string& interface_name) const;
        
        std::vector<std::string> getAllInterfaceNames() const;
        std::vector<std::string> getAllClassNames() const;
        size_t getClassCount() const;
        bool createInheritance(const std::string& child_class, const std::string& parent_class);
        
        void setVerbose(bool v);
        void printClassInfo(const std::string& class_name) const;

    private:
        std::string generateUniqueID();
        bool validateInterfaceImplementation(const std::string& class_name);

        std::map<std::string, ClassDefinition> defined_classes;
        std::map<std::string, InterfaceDefinition> defined_interfaces;
        std::map<std::string, std::string> inheritance_map;
        int object_counter;
        bool verboseOutput;
    };

} // namespace ArabicLanguage

#endif // ARABIC_TYPES_H

#ifdef Value
#undef Value
#endif
#ifdef Command
#undef Command
#endif
