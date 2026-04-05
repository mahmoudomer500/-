#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "ArabicTypes.h"

namespace ArabicLanguage {

    // ════════════════════════════════════════════════════════════
    // 🎯 نظام OOP للغة العربية - ملحقات مخصصة
    // ════════════════════════════════════════════════════════════
    // ملاحظة: نحن نستخدم تعريفات ClassDefinition و ClassMethod
    // من ArabicTypes.h والتوسيع عليها هنا

    // ✅ هيكل خاصية الصف (مع معلومات إضافية)
    struct ClassProperty {
        std::string name;
        std::string typeStr;        // "رقم"، "نص"، "منطق"
        bool isPrivate = false;
        std::string defaultValue;

        ClassProperty() = default;
        ClassProperty(const std::string& n, const std::string& t, bool priv = false)
            : name(n), typeStr(t), isPrivate(priv) {}
    };

    // ✅ هيكل دالة الصف (مع معلومات إضافية)
    struct ClassMethodEx {
        std::string name;
        std::string returnTypeStr;  // "رقم"، "نص"، "صفر"، إلخ
        std::vector<std::pair<std::string, std::string>> parameters;  // (اسم، نوع)
        bool isPrivate = false;
        std::string codeBlock;      // جسم الدالة

        ClassMethodEx() = default;
        ClassMethodEx(const std::string& n, const std::string& ret, bool priv = false)
            : name(n), returnTypeStr(ret), isPrivate(priv) {}
    };

    // ✅ هيكل تعريف الصف (مع دعم كامل للـ OOP)
    struct ClassDefinitionEx {
        std::string name;
        std::string baseName;
        std::vector<ClassProperty> properties;
        std::vector<ClassMethodEx> methods;
        bool hasConstructor = false;
        bool hasDestructor = false;

        ClassDefinitionEx() = default;
        ClassDefinitionEx(const std::string& className)
            : name(className), baseName("") {}

        void addProperty(const ClassProperty& prop) {
            properties.push_back(prop);
        }

        void addMethod(const ClassMethodEx& method) {
            methods.push_back(method);
            if (method.name == name) hasConstructor = true;
            if (method.name == "~" + name) hasDestructor = true;
        }

        const ClassProperty* getProperty(const std::string& propName) const {
            for (const auto& prop : properties) {
                if (prop.name == propName) return &prop;
            }
            return nullptr;
        }

        const ClassMethodEx* getMethod(const std::string& methodName) const {
            for (const auto& method : methods) {
                if (method.name == methodName) return &method;
            }
            return nullptr;
        }

        int getPropertyIndex(const std::string& propName) const {
            for (size_t i = 0; i < properties.size(); i++) {
                if (properties[i].name == propName) return static_cast<int>(i);
            }
            return -1;
        }

        size_t calculateSize() const {
            size_t size = 0;
            for (const auto& prop : properties) {
                if (prop.typeStr == "رقم") size += 8;
                else if (prop.typeStr == "نص") size += 16;
                else if (prop.typeStr == "منطق") size += 1;
            }
            return size;
        }

        std::string generateCppClass() const;
        std::string generateVTable() const;
    };

    // ✅ جدول الأصناف
    class ClassTable {
    private:
        std::map<std::string, std::shared_ptr<ClassDefinitionEx>> classes;

    public:
        void addClass(std::shared_ptr<ClassDefinitionEx> classdef) {
            if (classdef) {
                classes[classdef->name] = classdef;
            }
        }

        std::shared_ptr<ClassDefinitionEx> getClass(const std::string& name) const {
            auto it = classes.find(name);
            if (it != classes.end()) return it->second;
            return nullptr;
        }

        bool hasClass(const std::string& name) const {
            return classes.find(name) != classes.end();
        }

        std::vector<std::string> getClassNames() const {
            std::vector<std::string> names;
            for (const auto& [key, _] : classes) {
                names.push_back(key);
            }
            return names;
        }

        size_t getClassCount() const {
            return classes.size();
        }
    };

    // ════════════════════════════════════════════════════════════
    // 🔧 مدقق OOP
    // ════════════════════════════════════════════════════════════

    class OOPValidator {
    public:
        // ✅ إرجاع true/false فقط - بدون طباعة أخطاء
        static bool validateClass(const std::shared_ptr<ClassDefinitionEx>& classdef);
        static bool validateInheritance(const std::string& derived, const std::string& base,
                                       const ClassTable& classTable);
        static bool validatePropertyAccess(const std::string& className,
                                          const std::string& propertyName,
                                          const ClassTable& classTable);
        static bool validateMethodCall(const std::string& className,
                                      const std::string& methodName,
                                      const std::vector<std::string>& argTypes,
                                      const ClassTable& classTable);
    };

    // ════════════════════════════════════════════════════════════
    // 📝 مولد كود OOP
    // ════════════════════════════════════════════════════════════

    class OOPCodeGenerator {
    public:
        static std::string generateClassCode(const std::shared_ptr<ClassDefinitionEx>& classdef);
        static std::string generateConstructor(const std::shared_ptr<ClassDefinitionEx>& classdef);
        static std::string generateMethodCode(const std::shared_ptr<ClassDefinitionEx>& classdef,
                                             const ClassMethodEx& method);
        static std::string generatePropertyAccessor(const std::shared_ptr<ClassDefinitionEx>& classdef,
                                                   const ClassProperty& prop);
        static std::string generateObjectInstantiation(const std::string& className,
                                                      const std::string& varName);
        static std::string generateMethodCall(const std::string& objectName,
                                            const std::string& methodName,
                                            const std::vector<std::string>& args);
    };

} // namespace ArabicLanguage