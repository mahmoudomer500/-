#include "ArabicGenerics.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <numeric>

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // 🎯 تنفيذ نظام الجينيريك
    // ══════════════════════════════════════════════════════════════

    // Singleton instance
    ArabicGenerics& ArabicGenerics::getInstance() {
        static ArabicGenerics instance;
        return instance;
    }

    // ══════════════════════════════════════════════════════════════
    // 🏷️ تنفيذ معلمات النوع
    // ══════════════════════════════════════════════════════════════

    std::string ArabicGenerics::TypeParameter::toString() const {
        std::stringstream ss;
        ss << name;
        if (!constraints.empty()) {
            ss << ": ";
            for (size_t i = 0; i < constraints.size(); ++i) {
                if (i > 0) ss << " + ";
                ss << constraints[i];
            }
        }
        if (!defaultType.empty()) {
            ss << " = " << defaultType;
        }
        return ss.str();
    }

    bool ArabicGenerics::TypeParameter::satisfiesConstraints(const std::string& typeName) const {
        if (constraints.empty()) return true;

        // فحص بسيط للقيود (في الإصدار الكامل سيتم تحليل أعمق)
        for (const auto& constraint : constraints) {
            if (constraint == "كائن" && typeName != "رقم" && typeName != "نص" && typeName != "منطق") {
                return true; // الكائنات الأخرى ترث من كائن
            }
            if (constraint == "قابل_للمقارنة" && (typeName == "رقم" || typeName == "نص")) {
                return true;
            }
            if (constraint == "قابل_للجمع" && typeName == "رقم") {
                return true;
            }
        }
        return false;
    }

    // ══════════════════════════════════════════════════════════════
    // 📦 تنفيذ النوع العام
    // ══════════════════════════════════════════════════════════════

    std::string ArabicGenerics::GenericType::getSignature() const {
        std::stringstream ss;
        ss << name << "<";
        for (size_t i = 0; i < typeParameters.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << typeParameters[i].toString();
        }
        ss << ">";
        return ss.str();
    }

    std::string ArabicGenerics::GenericType::instantiate(const std::vector<std::string>& typeArgs) const {
        if (!canInstantiateWith(typeArgs)) {
            throw std::runtime_error("لا يمكن إنشاء النوع العام مع المعطيات المحددة");
        }

        std::stringstream ss;
        ss << "// إنشاء النوع: " << name << "\n";
        ss << "template<>\n";
        ss << "class " << name;

        if (!typeArgs.empty()) {
            ss << "<";
            for (size_t i = 0; i < typeArgs.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << typeArgs[i];
            }
            ss << ">";
        }

        ss << " {\n";

        // إضافة الخصائص
        for (const auto& prop : properties) {
            ss << "    " << prop.second << " " << prop.first << ";\n";
        }

        // إضافة الدوال
        for (const auto& method : methods) {
            ss << "    " << method.second << "\n";
        }

        ss << "};\n";
        return ss.str();
    }

    bool ArabicGenerics::GenericType::canInstantiateWith(const std::vector<std::string>& typeArgs) const {
        if (typeArgs.size() != typeParameters.size()) {
            return false;
        }

        for (size_t i = 0; i < typeArgs.size(); ++i) {
            if (!typeParameters[i].satisfiesConstraints(typeArgs[i])) {
                return false;
            }
        }

        return true;
    }

    // ══════════════════════════════════════════════════════════════
    // 🔧 تنفيذ الدالة العامة
    // ══════════════════════════════════════════════════════════════

    std::string ArabicGenerics::GenericFunction::getSignature() const {
        std::stringstream ss;
        ss << "template<";
        for (size_t i = 0; i < typeParameters.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << "typename " << typeParameters[i].name;
        }
        ss << ">\n";

        if (!returnType.empty()) {
            ss << returnType << " ";
        } else {
            ss << "void ";
        }

        ss << name << "(";

        for (size_t i = 0; i < parameters.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << parameters[i].second << " " << parameters[i].first;
        }

        ss << ")";
        return ss.str();
    }

    std::string ArabicGenerics::GenericFunction::instantiate(const std::vector<std::string>& typeArgs) const {
        if (!canInstantiateWith(typeArgs)) {
            throw std::runtime_error("لا يمكن إنشاء الدالة العامة مع المعطيات المحددة");
        }

        std::stringstream ss;
        ss << getSignature() << " {\n";
        ss << "    " << body << "\n";
        ss << "}\n";

        return ss.str();
    }

    bool ArabicGenerics::GenericFunction::canInstantiateWith(const std::vector<std::string>& typeArgs) const {
        if (typeArgs.size() != typeParameters.size()) {
            return false;
        }

        for (size_t i = 0; i < typeArgs.size(); ++i) {
            if (!typeParameters[i].satisfiesConstraints(typeArgs[i])) {
                return false;
            }
        }

        return true;
    }

    std::vector<std::string> ArabicGenerics::GenericFunction::getTypeDependencies() const {
        std::vector<std::string> deps;

        // استخراج التبعيات من المعطيات والإرجاع
        for (const auto& param : parameters) {
            if (std::find(deps.begin(), deps.end(), param.second) == deps.end()) {
                deps.push_back(param.second);
            }
        }

        if (!returnType.empty() &&
            std::find(deps.begin(), deps.end(), returnType) == deps.end()) {
            deps.push_back(returnType);
        }

        return deps;
    }

    // ══════════════════════════════════════════════════════════════
    // 💾 تنفيذ التخزين المؤقت للأنواع المُنشأة
    // ══════════════════════════════════════════════════════════════

    std::string ArabicGenerics::InstantiatedTypeCache::get(const std::string& key) {
        std::lock_guard<std::mutex> lock(cacheMutex);

        auto it = cache.find(key);
        if (it != cache.end()) {
            it->second.accessCount++;
            return it->second.instantiatedCode;
        }

        return "";
    }

    void ArabicGenerics::InstantiatedTypeCache::put(const std::string& key, const std::string& code) {
        std::lock_guard<std::mutex> lock(cacheMutex);

        if (cache.size() >= maxEntries) {
            cleanup();
        }

        cache[key] = CacheEntry(code);
    }

    void ArabicGenerics::InstantiatedTypeCache::clear() {
        std::lock_guard<std::mutex> lock(cacheMutex);
        cache.clear();
    }

    void ArabicGenerics::InstantiatedTypeCache::cleanup() {
        auto now = std::chrono::steady_clock::now();
        auto expiry = std::chrono::hours(1); // انتهاء صلاحية بعد ساعة

        for (auto it = cache.begin(); it != cache.end(); ) {
            auto age = now - it->second.created;
            if (age > expiry || cache.size() > maxEntries) {
                it = cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    // ══════════════════════════════════════════════════════════════
    // 🔍 تنفيذ محلل الجينيريك
    // ══════════════════════════════════════════════════════════════

    ArabicGenerics::GenericParser::GenericParser()
        : typeParamRegex(R"(<(\w+(?:\s*:\s*\w+(?:\s*\+\s*\w+)*)?(?:\s*=\s*\w+)?(?:\s*,\s*\w+(?:\s*:\s*\w+(?:\s*\+\s*\w+)*)?(?:\s*=\s*\w+)?)*>)"),
          genericTypeRegex(R"((\w+)\s*<(.+?)>\s*\{)"),
          constraintRegex(R"(:(\s*\w+(?:\s*\+\s*\w+)*))") {
    }

    std::vector<ArabicGenerics::TypeParameter> ArabicGenerics::GenericParser::parseTypeParameters(const std::string& paramList) {
        std::vector<TypeParameter> params;

        // إزالة < >
        std::string cleanList = paramList;
        if (cleanList.front() == '<') cleanList = cleanList.substr(1);
        if (cleanList.back() == '>') cleanList = cleanList.substr(0, cleanList.size() - 1);

        // تقسيم المعطيات
        std::stringstream ss(cleanList);
        std::string param;
        while (std::getline(ss, param, ',')) {
            // تنظيف المسافات
            param.erase(param.begin(), std::find_if(param.begin(), param.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            param.erase(std::find_if(param.rbegin(), param.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), param.end());

            TypeParameter tp(param);

            // فحص القيود
            std::smatch match;
            if (std::regex_search(param, match, constraintRegex)) {
                std::string constraintStr = match[1].str();
                std::stringstream constraintSs(constraintStr);
                std::string constraint;
                while (std::getline(constraintSs, constraint, '+')) {
                    constraint.erase(constraint.begin(), std::find_if(constraint.begin(), constraint.end(),
                        [](unsigned char ch) { return !std::isspace(ch); }));
                    constraint.erase(std::find_if(constraint.rbegin(), constraint.rend(),
                        [](unsigned char ch) { return !std::isspace(ch); }).base(), constraint.end());
                    tp.constraints.push_back(constraint);
                }
            }

            params.push_back(tp);
        }

        return params;
    }

    ArabicGenerics::GenericType ArabicGenerics::GenericParser::parseGenericType(const std::string& typeDefinition) {
        GenericType type;

        std::smatch match;
        if (std::regex_search(typeDefinition, match, genericTypeRegex)) {
            type.name = match[1].str();
            std::string paramStr = "<" + match[2].str() + ">";
            type.typeParameters = parseTypeParameters(paramStr);
        }

        return type;
    }

    ArabicGenerics::GenericFunction ArabicGenerics::GenericParser::parseGenericFunction(const std::string& functionDefinition) {
        GenericFunction func;

        // تحليل بسيط (في الإصدار الكامل سيتم تحليل أعمق)
        size_t templatePos = functionDefinition.find("دالة عامة");
        if (templatePos != std::string::npos) {
            size_t nameStart = functionDefinition.find(' ', templatePos + 10);
            if (nameStart != std::string::npos) {
                size_t nameEnd = functionDefinition.find('<', nameStart);
                if (nameEnd != std::string::npos) {
                    func.name = functionDefinition.substr(nameStart + 1, nameEnd - nameStart - 1);

                    size_t paramEnd = functionDefinition.find('>', nameEnd);
                    if (paramEnd != std::string::npos) {
                        std::string paramStr = functionDefinition.substr(nameEnd, paramEnd - nameEnd + 1);
                        func.typeParameters = parseTypeParameters(paramStr);
                    }
                }
            }
        }

        return func;
    }

    ArabicGenerics::GenericClass ArabicGenerics::GenericParser::parseGenericClass(const std::string& classDefinition) {
        GenericClass cls;

        // تحليل بسيط للصنف العام
        cls = GenericClass(parseGenericType(classDefinition).name);
        cls.typeParameters = parseGenericType(classDefinition).typeParameters;

        return cls;
    }

    bool ArabicGenerics::GenericParser::isGenericType(const std::string& typeName) {
        return typeName.find('<') != std::string::npos && typeName.find('>') != std::string::npos;
    }

    bool ArabicGenerics::GenericParser::isGenericFunction(const std::string& functionSignature) {
        return functionSignature.find("دالة عامة") != std::string::npos;
    }

    std::vector<std::string> ArabicGenerics::GenericParser::extractTypeArguments(const std::string& genericUsage) {
        std::vector<std::string> args;

        size_t start = genericUsage.find('<');
        size_t end = genericUsage.find('>', start);

        if (start != std::string::npos && end != std::string::npos) {
            std::string argStr = genericUsage.substr(start + 1, end - start - 1);

            std::stringstream ss(argStr);
            std::string arg;
            while (std::getline(ss, arg, ',')) {
                // تنظيف المسافات
                arg.erase(arg.begin(), std::find_if(arg.begin(), arg.end(),
                    [](unsigned char ch) { return !std::isspace(ch); }));
                arg.erase(std::find_if(arg.rbegin(), arg.rend(),
                    [](unsigned char ch) { return !std::isspace(ch); }).base(), arg.end());
                args.push_back(arg);
            }
        }

        return args;
    }

    // ══════════════════════════════════════════════════════════════
    // ⚡ تنفيذ مُنشئ الأنواع
    // ══════════════════════════════════════════════════════════════

    std::string ArabicGenerics::TypeInstantiator::instantiateType(const std::string& genericTypeName,
                                                                 const std::vector<std::string>& typeArgs) {
        std::string key = genericTypeName + "<" +
            std::accumulate(typeArgs.begin(), typeArgs.end(), std::string(),
                [](const std::string& a, const std::string& b) {
                    return a + (a.empty() ? "" : ",") + b;
                }) + ">";

        // فحص التخزين المؤقت
        std::string cached = cache.get(key);
        if (!cached.empty()) {
            return cached;
        }

        // البحث عن النوع
        auto it = typeRegistry.find(genericTypeName);
        if (it == typeRegistry.end()) {
            throw std::runtime_error("النوع العام غير موجود: " + genericTypeName);
        }

        // إنشاء النسخة
        std::string instantiated = it->second.instantiate(typeArgs);

        // حفظ في التخزين المؤقت
        cache.put(key, instantiated);

        return instantiated;
    }

    std::string ArabicGenerics::TypeInstantiator::instantiateFunction(const std::string& genericFunctionName,
                                                                     const std::vector<std::string>& typeArgs) {
        std::string key = genericFunctionName + "<" +
            std::accumulate(typeArgs.begin(), typeArgs.end(), std::string(),
                [](const std::string& a, const std::string& b) {
                    return a + (a.empty() ? "" : ",") + b;
                }) + ">";

        // فحص التخزين المؤقت
        std::string cached = cache.get(key);
        if (!cached.empty()) {
            return cached;
        }

        // البحث عن الدالة
        auto it = functionRegistry.find(genericFunctionName);
        if (it == functionRegistry.end()) {
            throw std::runtime_error("الدالة العامة غير موجودة: " + genericFunctionName);
        }

        // إنشاء النسخة
        std::string instantiated = it->second.instantiate(typeArgs);

        // حفظ في التخزين المؤقت
        cache.put(key, instantiated);

        return instantiated;
    }

    std::string ArabicGenerics::TypeInstantiator::instantiateClass(const std::string& genericClassName,
                                                                  const std::vector<std::string>& typeArgs) {
        // نفس منطق instantiateType لكن للأصناف
        return instantiateType(genericClassName, typeArgs);
    }

    bool ArabicGenerics::TypeInstantiator::registerGenericType(const GenericType& type) {
        typeRegistry[type.name] = type;
        return true;
    }

    bool ArabicGenerics::TypeInstantiator::registerGenericFunction(const GenericFunction& func) {
        functionRegistry[func.name] = func;
        return true;
    }

    bool ArabicGenerics::TypeInstantiator::registerGenericClass(const GenericClass& cls) {
        typeRegistry[cls.name] = cls;
        return true;
    }

    std::vector<std::string> ArabicGenerics::TypeInstantiator::getAvailableTypes() const {
        std::vector<std::string> types;
        for (const auto& pair : typeRegistry) {
            types.push_back(pair.first);
        }
        return types;
    }

    std::vector<std::string> ArabicGenerics::TypeInstantiator::getAvailableFunctions() const {
        std::vector<std::string> functions;
        for (const auto& pair : functionRegistry) {
            functions.push_back(pair.first);
        }
        return functions;
    }

    // ══════════════════════════════════════════════════════════════
    // 🎯 تنفيذ واجهة الاستخدام العامة
    // ══════════════════════════════════════════════════════════════

    bool ArabicGenerics::registerGenericType(const std::string& typeDefinition) {
        try {
            auto type = parser.parseGenericType(typeDefinition);
            return instantiator.registerGenericType(type);
        } catch (const std::exception&) {
            return false;
        }
    }

    bool ArabicGenerics::registerGenericFunction(const std::string& functionDefinition) {
        try {
            auto func = parser.parseGenericFunction(functionDefinition);
            return instantiator.registerGenericFunction(func);
        } catch (const std::exception&) {
            return false;
        }
    }

    bool ArabicGenerics::registerGenericClass(const std::string& classDefinition) {
        try {
            auto cls = parser.parseGenericClass(classDefinition);
            return instantiator.registerGenericClass(cls);
        } catch (const std::exception&) {
            return false;
        }
    }

    std::string ArabicGenerics::instantiateType(const std::string& genericName,
                                              const std::vector<std::string>& typeArgs) {
        try {
            return instantiator.instantiateType(genericName, typeArgs);
        } catch (const std::exception& e) {
            return "// خطأ في إنشاء النوع: " + std::string(e.what());
        }
    }

    std::string ArabicGenerics::instantiateFunction(const std::string& genericName,
                                                   const std::vector<std::string>& typeArgs) {
        try {
            return instantiator.instantiateFunction(genericName, typeArgs);
        } catch (const std::exception& e) {
            return "// خطأ في إنشاء الدالة: " + std::string(e.what());
        }
    }

    std::string ArabicGenerics::instantiateClass(const std::string& genericName,
                                                const std::vector<std::string>& typeArgs) {
        try {
            return instantiator.instantiateClass(genericName, typeArgs);
        } catch (const std::exception& e) {
            return "// خطأ في إنشاء الصنف: " + std::string(e.what());
        }
    }

    std::vector<std::string> ArabicGenerics::getStats() const {
        std::vector<std::string> stats;
        stats.push_back("=== إحصائيات نظام الجينيريك ===");
        stats.push_back("أنواع مسجلة: " + std::to_string(instantiator.getAvailableTypes().size()));
        stats.push_back("دوال مسجلة: " + std::to_string(instantiator.getAvailableFunctions().size()));
        stats.push_back("حجم التخزين المؤقت: " + std::to_string(typeCache.size()));

        return stats;
    }

    std::vector<std::string> ArabicGenerics::getRegisteredTypes() const {
        return instantiator.getAvailableTypes();
    }

    std::vector<std::string> ArabicGenerics::getRegisteredFunctions() const {
        return instantiator.getAvailableFunctions();
    }

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة للجينيريك
    // ══════════════════════════════════════════════════════════════

    bool validateGenericUsage(const std::string& genericCode) {
        // فحص بسيط لصحة الكود العام
        if (genericCode.find('<') == std::string::npos ||
            genericCode.find('>') == std::string::npos) {
            return false;
        }

        // فحص التوازن
        int balance = 0;
        for (char c : genericCode) {
            if (c == '<') balance++;
            else if (c == '>') balance--;
            if (balance < 0) return false;
        }

        return balance == 0;
    }

    std::string generateGenericCppCode(const std::string& arabicGenericCode) {
        // تحويل بسيط من العربية إلى C++
        std::string cppCode = arabicGenericCode;

        // استبدال الكلمات العربية الأساسية
        std::unordered_map<std::string, std::string> replacements = {
            {"دالة عامة", "template<typename"},
            {"صنف عام", "template<typename"},
            {"نوع", "typename"},
            {"ارجع", "return"},
            {"إذا", "if"},
            {"لكل", "for"},
            {"في", "in"}
        };

        for (const auto& pair : replacements) {
            size_t pos = 0;
            while ((pos = cppCode.find(pair.first, pos)) != std::string::npos) {
                cppCode.replace(pos, pair.first.length(), pair.second);
                pos += pair.second.length();
            }
        }

        return cppCode;
    }

    std::string optimizeGenericCode(const std::string& genericCode) {
        // تحسينات بسيطة
        std::string optimized = genericCode;

        // إزالة التعليقات
        size_t commentPos = 0;
        while ((commentPos = optimized.find("//", commentPos)) != std::string::npos) {
            size_t endLine = optimized.find('\n', commentPos);
            if (endLine != std::string::npos) {
                optimized.erase(commentPos, endLine - commentPos + 1);
            } else {
                optimized.erase(commentPos);
            }
        }

        // تنظيف المسافات الزائدة
        // (يمكن إضافة المزيد من التحسينات)

        return optimized;
    }

    std::unordered_map<std::string, std::string> extractTypeInfo(const std::string& code) {
        std::unordered_map<std::string, std::string> typeInfo;

        // استخراج بسيط لمعلومات الأنواع
        ArabicGenerics::GenericParser parser;
        if (parser.isGenericType(code)) {
            auto args = parser.extractTypeArguments(code);
            for (size_t i = 0; i < args.size(); ++i) {
                typeInfo["param_" + std::to_string(i)] = args[i];
            }
        }

        return typeInfo;
    }

    void initializeStandardGenericLibrary() {
        auto& generics = ArabicGenerics::getInstance();

        // تسجيل المكتبة القياسية
        using namespace StandardGenerics;

        generics.registerGenericType(LIST_DEFINITION);
        generics.registerGenericType(MAP_DEFINITION);
        generics.registerGenericFunction(FIND_FUNCTION);
        generics.registerGenericFunction(TRANSFORM_FUNCTION);

        std::cout << "✅ تم تسجيل مكتبة الجينيريك القياسية\n";
    }

    // ══════════════════════════════════════════════════════════════
    // 📚 تنفيذ مكتبة الجينيريك القياسية
    // ══════════════════════════════════════════════════════════════

    namespace StandardGenerics {

        void registerAll() {
            initializeStandardGenericLibrary();
        }

    } // namespace StandardGenerics

} // namespace ArabicLanguage
