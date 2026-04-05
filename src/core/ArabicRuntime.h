// ArabicRuntime.h - بيئة التشغيل والتنفيذ
#ifndef ARABIC_RUNTIME_H
#define ARABIC_RUNTIME_H

#include "../runtime/ArabicTypes.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stack>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace ArabicLanguage {

    // ═══════════════════════════════════════════════════════════
    // جدول الرموز (Symbol Table) - إدارة المتغيرات
    // ═══════════════════════════════════════════════════════════
    
    class RuntimeSymbolTable {
    public:
        RuntimeSymbolTable() {
            scopes.push_back({}); // Global scope
        }
        
        // تعريف متغير جديد
        void defineVariable(const std::string& name, const Value& value) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            std::string cleanName = trim(name);
            if (scopes.empty()) {
                scopes.push_back({});
            }
            scopes.back()[cleanName] = value;
        }
        
        // الحصول على قيمة متغير
        Value getVariable(const std::string& name) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            std::string cleanName = trim(name);
            
            // البحث في النطاقات الحالية (من الأحدث إلى الأقدم)
            // نبحث في كل النطاقات وصولاً إلى النطاق العالمي
            // std::cout << "DEBUG: Looking for variable '" << cleanName << "' in " << scopes.size() << " scopes." << std::endl;
            for (int i = (int)scopes.size() - 1; i >= 0; --i) {
                auto found = scopes[i].find(cleanName);
                if (found != scopes[i].end()) {
                    // std::cout << "DEBUG: Found '" << cleanName << "' in scope " << i << std::endl;
                    return found->second;
                }
            }

            std::cout << "❌ [Runtime] متغير غير معرّف: " << cleanName << " (عدد النطاقات: " << scopes.size() << ")" << std::endl;
            // طباعة محتويات النطاقات للمساعدة في التصحيح
            /*
            for (int i = (int)scopes.size() - 1; i >= 0; --i) {
                std::cout << "  Scope " << i << " contains: ";
                for (const auto& pair : scopes[i]) {
                    std::cout << pair.first << ", ";
                }
                std::cout << std::endl;
            }
            */
            throw std::runtime_error("❌ متغير غير معرّف: " + cleanName);
        }
        
        // تعيين قيمة متغير
        void setVariable(const std::string& name, const Value& value) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            std::string cleanName = trim(name);

            // 1. البحث عن المتغير في جميع النطاقات لتحديثه إذا كان موجوداً
            for (int i = (int)scopes.size() - 1; i >= 0; --i) {
                auto found = scopes[i].find(cleanName);
                if (found != scopes[i].end()) {
                    scopes[i][cleanName] = value;
                    return;
                }
            }
            
            // 2. إذا لم يتم العثور عليه في أي مكان، أنشئه في النطاق الحالي (أعلى النطاق)
            if (!scopes.empty()) {
                scopes.back()[cleanName] = value;
            } else {
                scopes.push_back({});
                scopes.back()[cleanName] = value;
            }
        }
        
        // التحقق من وجود متغير
        bool hasVariable(const std::string& name) const {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            std::string cleanName = trim(name);
            
            for (int i = (int)scopes.size() - 1; i >= 0; --i) {
                if (scopes[i].find(cleanName) != scopes[i].end()) {
                    return true;
                }
            }
            
            return false;
        }
        
        // دفع نطاق جديد
        void pushScope() {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            scopes.push_back({});
            // std::cout << "DEBUG: Pushed scope. Count: " << scopes.size() << std::endl;
        }
        
        // إغلاق النطاق
        void popScope() {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            if (scopes.size() > 1) { // حافظ على النطاق العالمي دائماً
                scopes.pop_back();
                // std::cout << "DEBUG: Popped scope. Count: " << scopes.size() << std::endl;
            } else {
                std::cout << "WARNING: Attempted to pop global scope!" << std::endl;
            }
        }
        
        // عزل النطاقات (للمكالمات الدوال)
        void pushScopeBase() {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            scopeBases.push(scopes.size() - 1);
        }
        
        void popScopeBase() {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            if (!scopeBases.empty()) {
                scopeBases.pop();
            }
        }

        // مسح جميع المتغيرات
        void clear() {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            scopes.clear();
            scopes.push_back({});
            while(!scopeBases.empty()) scopeBases.pop();
        }
        
        size_t scopeCount() const {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            return scopes.size();
        }

    private:
        // مجموعة من الخرائط (واحدة لكل نطاق)
        std::vector<std::map<std::string, Value>> scopes;
        std::stack<size_t> scopeBases;
        mutable std::recursive_mutex mutex;
    };

    // ═══════════════════════════════════════════════════════════
    // بيئة التشغيل (Runtime Environment)
    // ═══════════════════════════════════════════════════════════
    
    class ArabicRuntime {
    public:
        ArabicRuntime();
        virtual ~ArabicRuntime() = default;
        
        // ────────────────────────────────────────────────────────
        // واجهات التشغيل الافتراضية
        // ────────────────────────────────────────────────────────
        
        virtual void initialize();
        virtual Value execute(const std::vector<std::shared_ptr<Command>>& commands);
        virtual void shutdown();
        
        // ────────────────────────────────────────────────────────
        // إدارة الذاكرة (Memory Management Interface)
        // ────────────────────────────────────────────────────────
        
        virtual void* allocateMemory(size_t size, const std::string& location = "unknown");
        virtual void freeMemory(void* ptr);
        virtual void runGarbageCollection();
        
        // ────────────────────────────────────────────────────────
        // إدارة المتغيرات
        // ────────────────────────────────────────────────────────
        
        void defineVariable(const std::string& name, const Value& value) {
            symbolTable.defineVariable(name, value);
        }
        
        Value getVariable(const std::string& name) {
            return symbolTable.getVariable(name);
        }
        
        void setVariable(const std::string& name, const Value& value) {
            symbolTable.setVariable(name, value);
        }
        
        bool hasVariable(const std::string& name) const {
            return symbolTable.hasVariable(name);
        }
        
        // ────────────────────────────────────────────────────────
        // إدارة الدوال
        // ────────────────────────────────────────────────────────
        
        void defineFunction(const std::string& name, 
                          const std::shared_ptr<Command>& funcCmd) {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            functions[name] = funcCmd;
        }

        void defineNativeFunction(const std::string& name, NativeFunction func) {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            // std::cout << "DEBUG: Registering native function: " << name << " (len: " << name.length() << ")" << std::endl;
            nativeFunctions[name] = func;
        }
        
        bool hasFunction(const std::string& name) const {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            return functions.find(name) != functions.end() || 
                   nativeFunctions.find(name) != nativeFunctions.end();
        }
        
        std::shared_ptr<Command> getFunction(const std::string& name) {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            auto it = functions.find(name);
            if (it != functions.end()) {
                return it->second;
            }
            return nullptr;
        }

        NativeFunction getNativeFunction(const std::string& name) {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            auto it = nativeFunctions.find(name);
            if (it != nativeFunctions.end()) {
                return it->second;
            }
            return nullptr;
        }
        
        // ────────────────────────────────────────────────────────
        // إدارة النطاقات (Scopes)
        // ────────────────────────────────────────────────────────
        
        void pushScope() {
            symbolTable.pushScope();
        }
        
        void popScope() {
            symbolTable.popScope();
        }

        void pushScopeBase() {
            symbolTable.pushScopeBase();
        }
        
        void popScopeBase() {
            symbolTable.popScopeBase();
        }

        size_t scopeCount() const {
            return symbolTable.scopeCount();
        }
        
        // ────────────────────────────────────────────────────────
        // إدارة Call Stack
        // ────────────────────────────────────────────────────────
        
        void pushCallFrame(const std::string& functionName) {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            callStack.push(functionName);
        }
        
        void popCallFrame() {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            if (!callStack.empty()) {
                callStack.pop();
            }
        }
        
        bool isInFunction() const {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            return !callStack.empty();
        }
        
        std::string getCurrentFunction() const {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            if (callStack.empty()) {
                return "main";
            }
            return callStack.top();
        }
        
        // ────────────────────────────────────────────────────────
        // إدارة قيمة الإرجاع (Return Value)
        // ────────────────────────────────────────────────────────
        
        void setReturnValue(const Value& value) {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            if (!returnStack.empty()) {
                returnStack.top().value = value;
                returnStack.top().hasReturn = true;
            } else {
                // Fallback for global context or if something went wrong
                returnValue = value;
                hasReturnValue = true;
            }
        }
        
        Value getReturnValue() const {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            if (!returnStack.empty()) return returnStack.top().value;
            return returnValue;
        }
        
        bool hasReturn() const {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            if (!returnStack.empty()) return returnStack.top().hasReturn;
            return hasReturnValue;
        }
        
        void clearReturn() {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            if (!returnStack.empty()) {
                returnStack.top().hasReturn = false;
                returnStack.top().value = Value(ValueType::NONE);
            } else {
                hasReturnValue = false;
                returnValue = Value(ValueType::NONE);
            }
        }

        // Return Stack management
        void pushReturnFrame() {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            returnStack.push({Value(ValueType::NONE), false});
        }

        void popReturnFrame() {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            if (!returnStack.empty()) {
                returnStack.pop();
            }
        }
        
        // ────────────────────────────────────────────────────────
        // إدارة الحلقات (Loop Control)
        // ────────────────────────────────────────────────────────
        
        void setBreak(bool value = true) {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            shouldBreak = value;
        }
        
        bool hasBreak() const {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            return shouldBreak;
        }
        
        void clearBreak() {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            shouldBreak = false;
        }
        
        void setContinue(bool value = true) {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            shouldContinue = value;
        }
        
        bool hasContinue() const {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            return shouldContinue;
        }
        
        void clearContinue() {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            shouldContinue = false;
        }
        
        // ────────────────────────────────────────────────────────
        // إدارة الأصناف والكائنات
        // ────────────────────────────────────────────────────────
        
        void defineClass(const std::string& name, const ClassDefinition& def);
        bool hasClass(const std::string& name) const;
        const ClassDefinition* getClassInfo(const std::string& name) const;
        bool classHasMethod(const std::string& className, const std::string& methodName) const;

        // Interface management
        void defineInterface(const std::string& name, const InterfaceDefinition& def);
        bool hasInterface(const std::string& name) const;
        const InterfaceDefinition* getInterfaceInfo(const std::string& name) const;

        std::shared_ptr<ObjectInstance> createObjectInstance(const std::string& classType);
        std::shared_ptr<ObjectInstance> getObjectInstance(const std::string& uniqueId);
        void registerObjectInstance(const std::shared_ptr<ObjectInstance>& instance);

        // property helpers
        Value getObjectProperty(const std::string& objectId, const std::string& propName);
        void setObjectProperty(const std::string& objectId, const std::string& propName, const Value& val);

        // ────────────────────────────────────────────────────────
        // إعادة تعيين البيئة
        // ────────────────────────────────────────────────────────
        
        void reset() {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            symbolTable.clear();
            functions.clear();
            // nativeFunctions.clear(); // لا نمسح الدوال الأصلية لأنها جزء من النظام
            while (!callStack.empty()) {
                callStack.pop();
            }
            hasReturnValue = false;
            shouldBreak = false;
            shouldContinue = false;
            returnValue = Value(ValueType::NONE);
            classManager.reset();
            objects.clear();
        }
        
        // ────────────────────────────────────────────────────────
        // إحصائيات
        // ────────────────────────────────────────────────────────
        
        size_t getFunctionCount() const {
            std::lock_guard<std::recursive_mutex> lock(runtimeMutex);
            return functions.size();
        }
        
    private:
        mutable std::recursive_mutex runtimeMutex;   // مزلاج المزامنة لبيئة التشغيل
        RuntimeSymbolTable symbolTable;             // جدول الرموز
        std::map<std::string, 
                 std::shared_ptr<Command>> functions;  // الدوال المعرّفة
        std::map<std::string, NativeFunction> nativeFunctions; // الدوال الأصلية (C++)
        std::stack<std::string> callStack;          // Call stack
        
        struct ReturnFrame {
            Value value;
            bool hasReturn;
        };
        std::stack<ReturnFrame> returnStack;        // Return stack for nested calls
        
        Value returnValue;                          // قيمة الإرجاع (Global)
        bool hasReturnValue = false;               // علم قيمة الإرجاع
        
        bool shouldBreak = false;                  // علم التوقف
        bool shouldContinue = false;               // علم المتابعة

        // Class & Object management
        ClassManager classManager;
        std::map<std::string, std::shared_ptr<ObjectInstance>> objects;

        // تهيئة المكتبة القياسية
        void initializeStandardLibrary();
    };

    // ══════════════════════════════════════════════════════════════
    // 🧠 دوال إدارة الذاكرة العامة
    // ══════════════════════════════════════════════════════════════

    // تخصيص ذاكرة مُتتبعة
    void* arabic_malloc(size_t size, const std::string& location = "unknown");

    // تحرير ذاكرة مُتتبعة
    void arabic_free(void* ptr);

    // تقرير تسريبات الذاكرة
    void arabic_memory_report();

    // تنظيف جميع الذاكرة المخصصة
    void arabic_memory_cleanup();

    // تفعيل/تعطيل تتبع الذاكرة
    void arabic_set_memory_tracking(bool enabled);

} // namespace ArabicLanguage

#endif // ARABIC_RUNTIME_H
