#include "ArabicTypes.h"
#include <iostream>
#include <sstream>

namespace ArabicLanguage {

    std::string ClassManager::generateUniqueID() {
        std::ostringstream ss;
        ss << "obj_" << object_counter++;
        return ss.str();
    }

    bool ClassManager::defineClass(const std::string& class_name, const ClassDefinition& class_def) {
        if (defined_classes.find(class_name) != defined_classes.end()) return false;
        
        // إنشاء نسخة للقيام بالتعديلات
        ClassDefinition final_def = class_def;
        
        // ✅ بناء جدول الدوال الافتراضية (Virtual Method Table) للمرحلة 6
        // 1. وراثة الدوال من الأب أولاً
        if (!final_def.parent_class.empty()) {
            auto itParent = defined_classes.find(final_def.parent_class);
            if (itParent != defined_classes.end()) {
                final_def.virtual_methods = itParent->second.virtual_methods;
            }
        }
        
        // 2. حفظ الصنف في الخريطة أولاً للحصول على عناوين ذاكرة ثابتة للدوال
        defined_classes[class_name] = final_def;
        ClassDefinition& stored_def = defined_classes[class_name];

        // 3. إضافة/تجاوز (override) الدوال من الصنف الحالي باستخدام عناوين من الخريطة
        for (auto& it : stored_def.methods) {
            stored_def.virtual_methods[it.first] = &(it.second);
        }
        
        // التحقق من تنفيذ الواجهات
        if (!validateInterfaceImplementation(class_name)) {
            defined_classes.erase(class_name);
            return false;
        }

        // ✅ التحقق من تنفيذ الدوال المجردة (Stage 6)
        if (!stored_def.is_abstract) {
            for (auto const& [name, methodPtr] : stored_def.virtual_methods) {
                if (methodPtr->is_abstract) {
                    defined_classes.erase(class_name);
                    throw std::runtime_error("❌ الصنف '" + class_name + "' يجب أن ينفذ الدالة المجردة '" + 
                        name + "' الموروثة");
                }
            }
        }
        
        return true;
    }

    bool ClassManager::validateInterfaceImplementation(const std::string& class_name) {
        auto itClass = defined_classes.find(class_name);
        if (itClass == defined_classes.end()) return false;
        
        const auto& class_def = itClass->second;
        
        // إذا كان الصنف مجرداً، لا نشترط تنفيذ جميع الدوال حالياً
        // (يمكن للصنف المجرد أن يترك دوال الواجهة دون تنفيذ)
        if (class_def.is_abstract) return true;
        
        for (const auto& interface_name : class_def.interfaces) {
            auto itInterface = defined_interfaces.find(interface_name);
            if (itInterface == defined_interfaces.end()) {
                throw std::runtime_error("❌ الواجهة غير معرّفة: " + interface_name);
            }
            
            for (const auto& method_signature : itInterface->second.methods) {
                // البحث عن الدالة في الصنف الحالي أو الأصناف الأب
                if (!hasMethod(class_name, method_signature)) {
                    throw std::runtime_error("❌ الصنف '" + class_name + "' لا ينفذ الدالة '" + 
                        method_signature + "' المطلوبة في الواجهة '" + interface_name + "'");
                }
            }
        }
        
        return true;
    }

    bool ClassManager::defineInterface(const std::string& interface_name, const InterfaceDefinition& interface_def) {
        if (defined_interfaces.find(interface_name) != defined_interfaces.end()) return false;
        defined_interfaces[interface_name] = interface_def;
        return true;
    }

    std::shared_ptr<ObjectInstance> ClassManager::createObject(const std::string& class_type) {
        auto it = defined_classes.find(class_type);
        if (it == defined_classes.end()) return nullptr;
        
        // ✅ منع إنشاء كائن من صنف مجرد
        if (it->second.is_abstract) {
            throw std::runtime_error("❌ لا يمكن إنشاء كائن من صنف مجرد: " + class_type);
        }
        
        auto instance = std::make_shared<ObjectInstance>();
        instance->class_type = class_type;
        instance->unique_id = generateUniqueID();
        
        // جمع كل الخصائص من خلال تسلسل الوراثة
        std::string currentClass = class_type;
        while (!currentClass.empty()) {
            auto itClass = defined_classes.find(currentClass);
            if (itClass == defined_classes.end()) break;
            
            for (const auto& pair : itClass->second.properties) {
                const auto& p = pair.second;
                // إذا لم تكن الخاصية موجودة بالفعل (لتجنب التكرار في حال إعادة التعريف)
                    if (instance->properties.find(p.name) == instance->properties.end()) {
                        if (verboseOutput) {
                            
                        } else {
                            // طباعة دائماً للمساعدة في التصحيح حالياً
                            // std::cout << "📝 تهيئة خاصية: " << p.name << " في كائن من نوع " << class_type << std::endl;
                        }
                        
                        // 1. البحث في خريطة القيم الافتراضية (للتوافق مع الكود القديم)
                        auto itDefault = itClass->second.default_values.find(p.name);
                        if (itDefault != itClass->second.default_values.end()) {
                            instance->properties[p.name] = std::make_shared<Value>(itDefault->second);
                        } 
                        // 2. البحث في كائن معلومات الخاصية نفسه (النهج الجديد)
                        else if (p.default_value) {
                            instance->properties[p.name] = std::make_shared<Value>(*p.default_value);
                        }
                        // 3. القيمة الافتراضية بناءً على النوع
                        else {
                            instance->properties[p.name] = std::make_shared<Value>(Value(p.type, ""));
                        }
                    }
            }
            
            currentClass = itClass->second.parent_class;
        }
        
        return instance;
    }

    Value ClassManager::getProperty(const ObjectInstance& obj, const std::string& prop_name) {
        auto it = obj.properties.find(prop_name);
        if (it == obj.properties.end()) throw std::runtime_error("❌ الخاصية غير موجودة: " + prop_name);
        return *(it->second);
    }

    void ClassManager::setProperty(ObjectInstance& obj, const std::string& prop_name, const Value& val) {
        auto it = obj.properties.find(prop_name);
        if (it == obj.properties.end()) {
            // ✅ دعم الخصائص الديناميكية: إذا لم تكن موجودة، نقوم بإنشائها
            obj.properties[prop_name] = std::make_shared<Value>(val);
            if (verboseOutput) {
                
            }
        } else {
            *(it->second) = val;
        }
    }

    Value ClassManager::callMethod(ObjectInstance& obj, const std::string& method_name,
        const std::vector<Value>& args) {
        auto itClass = defined_classes.find(obj.class_type);
        if (itClass == defined_classes.end()) throw std::runtime_error("❌ صنف غير معرّف: " + obj.class_type);
        auto it = itClass->second.methods.find(method_name);
        if (it == itClass->second.methods.end()) throw std::runtime_error("❌ دالة عضو غير معرّفة: " + method_name);
        // For now, methods are stored as ClassMethod and executed by executor using Command body
        // This manager only prepares call context; actual execution lives in executor.
        return Value(ValueType::NONE);
    }

    bool ClassManager::classExists(const std::string& class_name) const {
        return defined_classes.find(class_name) != defined_classes.end();
    }

    bool ClassManager::interfaceExists(const std::string& interface_name) const {
        return defined_interfaces.find(interface_name) != defined_interfaces.end();
    }

    const ClassDefinition* ClassManager::getClassInfo(const std::string& class_name) const {
        auto it = defined_classes.find(class_name);
        if (it == defined_classes.end()) return nullptr;
        return &it->second;
    }

    const InterfaceDefinition* ClassManager::getInterfaceInfo(const std::string& interface_name) const {
        auto it = defined_interfaces.find(interface_name);
        if (it == defined_interfaces.end()) return nullptr;
        return &it->second;
    }

    std::vector<std::string> ClassManager::getAllInterfaceNames() const {
        std::vector<std::string> names;
        for (const auto& [key, _] : defined_interfaces) {
            names.push_back(key);
        }
        return names;
    }

    bool ClassManager::createInheritance(const std::string& child_class, const std::string& parent_class) {
        if (!classExists(child_class) || !classExists(parent_class)) return false;
        inheritance_map[child_class] = parent_class;
        return true;
    }

    void ClassManager::printClassInfo(const std::string& class_name) const {
        auto it = defined_classes.find(class_name);
        if (it == defined_classes.end()) return;
        std::cout << "Class: " << class_name << std::endl;
        for (const auto& p : it->second.properties) {
            std::cout << "  prop: " << p.first << std::endl;
        }
    }

    void ClassManager::setVerbose(bool verbose) { verboseOutput = verbose; }

    void ClassManager::reset() {
        defined_classes.clear();
        inheritance_map.clear();
        object_counter = 0;
    }

    std::vector<std::string> ClassManager::getAllClassNames() const {
        std::vector<std::string> names;
        for (const auto& kv : defined_classes) names.push_back(kv.first);
        return names;
    }

    size_t ClassManager::getClassCount() const { return defined_classes.size(); }

    bool ClassManager::hasProperty(const std::string& class_name, const std::string& prop_name) const {
        std::string currentClass = class_name;
        while (!currentClass.empty()) {
            auto it = defined_classes.find(currentClass);
            if (it == defined_classes.end()) break;
            
            if (it->second.properties.find(prop_name) != it->second.properties.end()) return true;
            
            currentClass = it->second.parent_class;
        }
        return false;
    }

    bool ClassManager::hasMethod(const std::string& class_name, const std::string& method_name) const {
        std::string currentClass = class_name;
        while (!currentClass.empty()) {
            auto it = defined_classes.find(currentClass);
            if (it == defined_classes.end()) break;
            
            if (it->second.methods.find(method_name) != it->second.methods.end()) return true;
            
            currentClass = it->second.parent_class;
        }
        return false;
    }

} // namespace ArabicLanguage
