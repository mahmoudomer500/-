// ArabicOOP.cpp - تطبيق كامل لنظام OOP - ✅ مصحح
// ✅ إزالة طباعة الأخطاء من Validators
// ✅ استخدام ClassDefinitionEx و ClassMethodEx

#include "ArabicOOP.h"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace ArabicLanguage {

    // ════════════════════════════════════════════════════════════
    // 📝 تطبيق ClassDefinitionEx::generateCppClass()
    // ════════════════════════════════════════════════════════════

    std::string ClassDefinitionEx::generateCppClass() const {
        std::ostringstream oss;

        oss << "// ✅ الصف: " << name << "\n";
        oss << "class " << name;
        
        if (!baseName.empty()) {
            oss << " : public " << baseName;
        }
        
        oss << " {\n";
        oss << "public:\n";

        // ═══ الخصائص العامة ═══
        for (const auto& prop : properties) {
            if (!prop.isPrivate) {
                if (prop.typeStr == "رقم") {
                    oss << "    long long " << prop.name << " = 0;\n";
                } 
                else if (prop.typeStr == "نص") {
                    oss << "    std::string " << prop.name << ";\n";
                } 
                else if (prop.typeStr == "منطق") {
                    oss << "    bool " << prop.name << " = false;\n";
                }
                else {
                    oss << "    // خاصية: " << prop.name << " (" << prop.typeStr << ")\n";
                }
            }
        }

        // ═══ المنشئ ═══
        if (hasConstructor) {
            oss << "\n    // المنشئ\n";
            oss << "    " << name << "() {\n";
            
            for (const auto& prop : properties) {
                if (!prop.isPrivate) {
                    if (prop.typeStr == "رقم") {
                        oss << "        " << prop.name << " = 0;\n";
                    }
                    else if (prop.typeStr == "نص") {
                        oss << "        " << prop.name << " = \"\";\n";
                    }
                }
            }
            
            oss << "    }\n";
        }

        // ═══ الدوال العامة ═══
        oss << "\n    // الدوال العامة\n";
        for (const auto& method : methods) {
            if (!method.isPrivate) {
                oss << "    void " << method.name << "() {\n";
                oss << "        // جسم الدالة\n";
                oss << "    }\n";
            }
        }

        // ═══ الخصائص الخاصة ═══
        oss << "\nprivate:\n";
        
        bool hasPrivateProps = false;
        for (const auto& prop : properties) {
            if (prop.isPrivate) {
                hasPrivateProps = true;
                if (prop.typeStr == "رقم") {
                    oss << "    long long " << prop.name << " = 0;\n";
                }
                else if (prop.typeStr == "نص") {
                    oss << "    std::string " << prop.name << ";\n";
                }
            }
        }
        
        if (!hasPrivateProps) {
            oss << "    // لا توجد خصائص خاصة\n";
        }

        // ═══ الدوال الخاصة ═══
        bool hasPrivateMethods = false;
        for (const auto& method : methods) {
            if (method.isPrivate) {
                hasPrivateMethods = true;
                oss << "    void " << method.name << "() { }\n";
            }
        }

        oss << "};\n";

        return oss.str();
    }

    // ════════════════════════════════════════════════════════════
    // 🔄 تطبيق ClassDefinitionEx::generateVTable()
    // ════════════════════════════════════════════════════════════

    std::string ClassDefinitionEx::generateVTable() const {
        std::ostringstream oss;

        oss << "\n// جدول الدوال الافتراضية (VTable) للصف " << name << "\n";
        oss << "struct " << name << "_VTable {\n";

        if (methods.empty()) {
            oss << "    // لا توجد دوال افتراضية\n";
        }
        else {
            for (const auto& method : methods) {
                oss << "    void (*" << method.name << ")(void*) = nullptr;\n";
            }
        }

        oss << "};\n";

        return oss.str();
    }

    // ════════════════════════════════════════════════════════════
    // 🔧 تطبيق OOPValidator::validateClass() - ✅ بدون طباعة
    // ════════════════════════════════════════════════════════════

    bool OOPValidator::validateClass(const std::shared_ptr<ClassDefinitionEx>& classdef) {
        // ✅ فقط التحقق والإرجاع - بدون طباعة
        if (!classdef) {
            return false;
        }

        if (classdef->name.empty()) {
            return false;
        }

        return true;
    }

    // ════════════════════════════════════════════════════════════
    // 👨‍👩‍👧 تطبيق OOPValidator::validateInheritance() - ✅ بدون طباعة
    // ════════════════════════════════════════════════════════════

    bool OOPValidator::validateInheritance(const std::string& derived, 
                                          const std::string& base,
                                          const ClassTable& classTable) {
        // ✅ فقط التحقق والإرجاع - بدون طباعة
        auto baseClass = classTable.getClass(base);
        if (!baseClass) {
            return false;
        }

        auto derivedClass = classTable.getClass(derived);
        if (!derivedClass) {
            return false;
        }

        if (derivedClass->baseName != base) {
            return false;
        }

        return true;
    }

    // ════════════════════════════════════════════════════════════
    // 🔍 تطبيق OOPValidator::validatePropertyAccess() - ✅ بدون طباعة
    // ════════════════════════════════════════════════════════════

    bool OOPValidator::validatePropertyAccess(const std::string& className,
                                             const std::string& propertyName,
                                             const ClassTable& classTable) {
        // ✅ فقط التحقق والإرجاع - بدون طباعة
        auto classdef = classTable.getClass(className);
        if (!classdef) {
            return false;
        }

        const ClassProperty* prop = classdef->getProperty(propertyName);
        if (!prop) {
            return false;
        }

        if (prop->isPrivate) {
            return false;
        }

        return true;
    }

    // ════════════════════════════════════════════════════════════
    // 📞 تطبيق OOPValidator::validateMethodCall() - ✅ بدون طباعة
    // ════════════════════════════════════════════════════════════

    bool OOPValidator::validateMethodCall(const std::string& className,
                                         const std::string& methodName,
                                         const std::vector<std::string>& argTypes,
                                         const ClassTable& classTable) {
        // ✅ فقط التحقق والإرجاع - بدون طباعة
        auto classdef = classTable.getClass(className);
        if (!classdef) {
            return false;
        }

        const ClassMethodEx* method = classdef->getMethod(methodName);
        if (!method) {
            return false;
        }

        if (method->parameters.size() != argTypes.size()) {
            return false;
        }

        for (size_t i = 0; i < argTypes.size(); i++) {
            if (method->parameters[i].second != argTypes[i]) {
                return false;
            }
        }

        if (method->isPrivate) {
            return false;
        }

        return true;
    }

    // ════════════════════════════════════════════════════════════
    // 📝 تطبيق OOPCodeGenerator::generateClassCode()
    // ════════════════════════════════════════════════════════════

    std::string OOPCodeGenerator::generateClassCode(const std::shared_ptr<ClassDefinitionEx>& classdef) {
        if (!classdef) {
            return "";
        }

        std::ostringstream oss;

        oss << "// ════════════════════════════════════════\n";
        oss << "// 🎯 توليد كود الصف: " << classdef->name << "\n";
        oss << "// ════════════════════════════════════════\n\n";

        oss << classdef->generateCppClass();
        oss << classdef->generateVTable();

        oss << "\n// 🎯 تم توليد الصف بنجاح\n";

        return oss.str();
    }

    // ════════════════════════════════════════════════════════════
    // 🏗️ تطبيق OOPCodeGenerator::generateConstructor()
    // ════════════════════════════════════════════════════════════

    std::string OOPCodeGenerator::generateConstructor(const std::shared_ptr<ClassDefinitionEx>& classdef) {
        if (!classdef) return "";

        std::ostringstream oss;

        oss << "\n// المنشئ (Constructor)\n";
        oss << classdef->name << "::" << classdef->name << "() {\n";

        for (const auto& prop : classdef->properties) {
            if (prop.typeStr == "رقم") {
                oss << "    this->" << prop.name << " = 0;\n";
            }
            else if (prop.typeStr == "نص") {
                oss << "    this->" << prop.name << " = \"\";\n";
            }
            else if (prop.typeStr == "منطق") {
                oss << "    this->" << prop.name << " = false;\n";
            }
        }

        oss << "}\n";

        return oss.str();
    }

    // ════════════════════════════════════════════════════════════
    // ⚙️ تطبيق OOPCodeGenerator::generateMethodCode()
    // ════════════════════════════════════════════════════════════

    std::string OOPCodeGenerator::generateMethodCode(const std::shared_ptr<ClassDefinitionEx>& classdef,
                                                    const ClassMethodEx& method) {
        std::ostringstream oss;

        oss << "\n// دالة الصف: " << method.name << "\n";
        oss << "void " << classdef->name << "::" << method.name << "() {\n";

        if (!method.codeBlock.empty()) {
            oss << method.codeBlock;
        } else {
            oss << "    // جسم الدالة (فارغ)\n";
        }

        oss << "}\n";

        return oss.str();
    }

    // ════════════════════════════════════════════════════════════
    // 📖 تطبيق OOPCodeGenerator::generatePropertyAccessor()
    // ════════════════════════════════════════════════════════════

    std::string OOPCodeGenerator::generatePropertyAccessor(const std::shared_ptr<ClassDefinitionEx>& classdef,
                                                         const ClassProperty& prop) {
        std::ostringstream oss;

        std::string cppType;
        if (prop.typeStr == "رقم") {
            cppType = "long long";
        }
        else if (prop.typeStr == "نص") {
            cppType = "std::string";
        }
        else if (prop.typeStr == "منطق") {
            cppType = "bool";
        }
        else {
            cppType = "void";
        }

        // Getter
        oss << "\n// الحصول على قيمة الخاصية\n";
        oss << cppType << " " << classdef->name << "::get_" << prop.name << "() const {\n";
        oss << "    return this->" << prop.name << ";\n";
        oss << "}\n";

        // Setter
        oss << "\n// تعيين قيمة الخاصية\n";
        oss << "void " << classdef->name << "::set_" << prop.name 
            << "(" << cppType << " value) {\n";
        oss << "    this->" << prop.name << " = value;\n";
        oss << "}\n";

        return oss.str();
    }

    // ════════════════════════════════════════════════════════════
    // 🆕 تطبيق OOPCodeGenerator::generateObjectInstantiation()
    // ════════════════════════════════════════════════════════════

    std::string OOPCodeGenerator::generateObjectInstantiation(const std::string& className,
                                                            const std::string& varName) {
        std::ostringstream oss;

        oss << "\n// إنشاء كائن من الصف " << className << "\n";
        oss << className << " " << varName << ";\n";
        oss << varName << "." << className << "();  // استدعاء المنشئ\n";

        return oss.str();
    }

    // ════════════════════════════════════════════════════════════
    // 📞 تطبيق OOPCodeGenerator::generateMethodCall()
    // ════════════════════════════════════════════════════════════

    std::string OOPCodeGenerator::generateMethodCall(const std::string& objectName,
                                                    const std::string& methodName,
                                                    const std::vector<std::string>& args) {
        std::ostringstream oss;

        oss << "\n// استدعاء دالة الصف\n";
        oss << objectName << "." << methodName << "(";

        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) oss << ", ";
            oss << args[i];
        }

        oss << ");\n";

        return oss.str();
    }

} // namespace ArabicLanguage