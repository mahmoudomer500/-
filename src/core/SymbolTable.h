// SymbolTable.h - جدول الرموز للمجمع العربي
// إدارة Labels, Functions, Variables

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <map>
#include <set>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <iostream>  // For dump() function

namespace ArabicLanguage {

// Forward declaration
class SymbolTable;

// ════════════════════════════════════════════════════════════
// 📋 أنواع الرموز
// ════════════════════════════════════════════════════════════

enum class SymbolType {
    LABEL,          // تسمية للقفزات
    FUNCTION,       // دالة
    VARIABLE,       // متغير محلي
    PARAMETER,      // معامل دالة
    IMPORT          // دالة مستوردة
};

enum class SymbolDataType {
    INTEGER,
    STRING,
    FILE_HANDLE,
    OBJECT,         // كائن
    ARRAY,          // مصفوفة
    UNKNOWN
};

// ════════════════════════════════════════════════════════════
// 📊 معلومات الصنف
// ════════════════════════════════════════════════════════════

struct ClassInfo {
    std::string name;
    std::string parentClass; // اسم الصنف الأب (للوراثة)
    uint32_t size = 0;  // الحجم الكلي للكائن بالبايت
    std::map<std::string, uint32_t> propertyOffsets; // اسم الخاصية -> الإزاحة
    std::set<std::string> methods; // أسماء الدوال الأعضاء
    
    void addProperty(const std::string& propName) {
        propertyOffsets[propName] = size;
        size += 8; // كل خاصية 8 بايت حالياً
    }
    
    uint32_t getPropertyOffset(const std::string& propName) const {
        if (propertyOffsets.find(propName) != propertyOffsets.end()) {
            return propertyOffsets.at(propName);
        }
        throw std::runtime_error("Undefined property: " + propName);
    }
    
    bool hasProperty(const std::string& propName) const {
        return propertyOffsets.find(propName) != propertyOffsets.end();
    }

    bool inheritsFrom(const SymbolTable& symbols, const std::string& baseClassName) const;
};

struct Symbol {
    std::string name;
    SymbolType type;
    SymbolDataType dataType;
    uint32_t offset;        // RVA أو Stack offset
    uint32_t size;          // حجم البيانات
    bool resolved;          // هل تم حل العنوان؟
    std::string className;  // للكائنات: اسم الصنف
    bool isConst;           // هل المتغير ثابت؟
    
    Symbol() : type(SymbolType::LABEL), dataType(SymbolDataType::UNKNOWN), offset(0), size(0), resolved(false), isConst(false) {}
    
    Symbol(const std::string& n, SymbolType t, uint32_t off = 0, SymbolDataType dt = SymbolDataType::UNKNOWN)
        : name(n), type(t), dataType(dt), offset(off), size(0), resolved(false), isConst(false) {}
};

// ════════════════════════════════════════════════════════════
// 🗂️ جدول الرموز
// ════════════════════════════════════════════════════════════

class SymbolTable {
private:
    std::map<std::string, Symbol> globalSymbols;
    std::vector<std::map<std::string, Symbol>> scopes; // Stack of local scopes
    std::map<std::string, uint32_t> imports;  // اسم الدالة → IAT offset
    std::map<std::string, ClassInfo> classes; // اسم الصنف -> معلومات الصنف
    
    uint32_t currentStackOffset = 0;
    
public:
    SymbolTable() {
        // Global scope is always present (but functions/labels usually go into globalSymbols)
    }

    // ═══ إدارة النطاقات (Scopes) ═══
    
    void pushScope() {
        scopes.push_back({});
    }
    
    void popScope() {
        if (!scopes.empty()) {
            scopes.pop_back();
        }
    }

    // ═══ إدارة الأصناف ═══
    
    void addClass(const ClassInfo& info) {
        classes[info.name] = info;
    }
    
    bool hasClass(const std::string& name) const {
        return classes.find(name) != classes.end();
    }
    
    const ClassInfo& getClass(const std::string& name) const {
        if (hasClass(name)) {
            return classes.at(name);
        }
        throw std::runtime_error("Undefined class: " + name);
    }
    
    const std::map<std::string, ClassInfo>& getAllClasses() const {
        return classes;
    }

    // ═══ إضافة رموز ═══
    
    void addSymbol(const Symbol& sym) {
        if (scopes.empty()) {
            globalSymbols[sym.name] = sym;
        } else {
            scopes.back()[sym.name] = sym;
        }
    }
    
    void addLabel(const std::string& name, uint32_t offset = 0) {
        // ✅ FIX: Add to global symbols directly to prevent loss on scope pop
        // Labels must persist for the entire program lifetime
        Symbol sym(name, SymbolType::LABEL, offset);
        sym.resolved = true;
        globalSymbols[name] = sym;
    }
    
    std::string getNewLabel(const std::string& prefix) {
        static int labelCounter = 0;
        return prefix + "_" + std::to_string(++labelCounter);
    }

    
    void addFunction(const std::string& name, uint32_t offset = 0) {
        Symbol sym(name, SymbolType::FUNCTION, offset);
        sym.resolved = (offset != 0);
        addSymbol(sym);
    }
    
    void addVariable(const std::string& name, uint32_t size = 8, SymbolDataType dtype = SymbolDataType::INTEGER, const std::string& className = "", bool isConst = false) {
        currentStackOffset += size;
        Symbol sym(name, SymbolType::VARIABLE, currentStackOffset, dtype);
        sym.size = size;
        sym.resolved = true;
        sym.className = className;
        sym.isConst = isConst;
        addSymbol(sym);
    }
    
    void addImport(const std::string& name, uint32_t iatOffset) {
        imports[name] = iatOffset;
        Symbol sym(name, SymbolType::IMPORT, iatOffset);
        sym.resolved = true;
        globalSymbols[name] = sym; // Imports are always global
    }
    
    // ═══ حل الرموز ═══
    
    bool resolve(const std::string& name, uint32_t offset) {
        // Search from inner to outer
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->count(name)) {
                (*it)[name].offset = offset;
                (*it)[name].resolved = true;
                return true;
            }
        }
        
        if (globalSymbols.count(name)) {
            globalSymbols[name].offset = offset;
            globalSymbols[name].resolved = true;
            return true;
        }
        return false;
    }
    
    // ═══ البحث ═══

    bool hasSymbol(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->count(name)) return true;
        }
        return globalSymbols.count(name) > 0;
    }
    
    Symbol& getSymbol(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->count(name)) return (*it)[name];
        }
        
        auto it = globalSymbols.find(name);
        if (it == globalSymbols.end()) {
            throw std::runtime_error("Symbol not found: " + name);
        }
        return it->second;
    }

    Symbol getSymbol(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto fit = it->find(name);
            if (fit != it->end()) return fit->second;
        }
        
        auto it = globalSymbols.find(name);
        if (it == globalSymbols.end()) {
            throw std::runtime_error("Symbol not found: " + name);
        }
        return it->second;
    }
    
    bool isImport(const std::string& name) const {
        return imports.find(name) != imports.end();
    }
    
    uint32_t getImportOffset(const std::string& name) const {
        auto it = imports.find(name);
        if (it == imports.end()) {
            throw std::runtime_error("Import not found: " + name);
        }
        return it->second;
    }

    size_t getSymbolCount() const {
        size_t count = globalSymbols.size();
        for (const auto& scope : scopes) {
            count += scope.size();
        }
        return count;
    }

    std::vector<std::string> getImportNames() const {
        std::vector<std::string> names;
        for (const auto& pair : imports) {
            names.push_back(pair.first);
        }
        return names;
    }
    
    size_t getVariablesCount() const {
        size_t count = 0;
        for (const auto& pair : globalSymbols) {
            if (pair.second.type == SymbolType::VARIABLE) {
                count++;
            }
        }
        for (const auto& scope : scopes) {
            for (const auto& pair : scope) {
                if (pair.second.type == SymbolType::VARIABLE) {
                    count++;
                }
            }
        }
        return count;
    }
    
    // ═══ Stack Management ═══
    
    uint32_t getStackSize() const {
        return currentStackOffset;
    }
    
    void resetStack() {
        currentStackOffset = 0;
    }
    
    // ✅ NEW: Adjust all non-import symbols (LABELS) by offset
    // Used when merging code sections to convert offsets to RVAs
    // ⚠️ IMPORTANT: Skip data labels (RVA >= 0x2000) - they're already absolute!
    void adjustNonImportSymbols(uint32_t offsetToAdd) {
        std::set<std::string> externalImports;
        for (const auto& imp : imports) {
            externalImports.insert(imp.first);
        }

        // Adjust global symbols
        for (auto& pair : globalSymbols) {
            bool isExternalImport = externalImports.find(pair.first) != externalImports.end();
            if (!isExternalImport) {
                if (pair.second.offset < 0x2000) {
                    pair.second.offset += offsetToAdd;
                    pair.second.resolved = true;
                }
            }
        }

        // Adjust scoped symbols
        for (auto& scope : scopes) {
            for (auto& pair : scope) {
                bool isExternalImport = externalImports.find(pair.first) != externalImports.end();
                if (!isExternalImport) {
                    if (pair.second.offset < 0x2000) {
                        pair.second.offset += offsetToAdd;
                        pair.second.resolved = true;
                    }
                }
            }
        }
    }
    
    // ═══ Debugging ═══
    
    void dump() const {
        std::cout << "═══ Symbol Table (Global) ═══" << std::endl;
        for (const auto& pair : globalSymbols) {
            std::cout << pair.first << ": "
                      << "offset=0x" << std::hex << pair.second.offset
                      << ", resolved=" << pair.second.resolved
                      << std::dec << std::endl;
        }
        for (size_t i = 0; i < scopes.size(); ++i) {
            std::cout << "═══ Scope " << i << " ═══" << std::endl;
            for (const auto& pair : scopes[i]) {
                std::cout << pair.first << ": "
                          << "offset=0x" << std::hex << pair.second.offset
                          << ", resolved=" << pair.second.resolved
                          << std::dec << std::endl;
            }
        }
    }
};

// ════════════════════════════════════════════════════════════
// 🔧 Relocation Management
// ════════════════════════════════════════════════════════════

enum class RelocationType {
    REL32,          // 32-bit relative via reloc table (deprecated mostly)
    ABS64,          // 64-bit absolute address
    RIP_REL32       // RIP-relative addressing
};

struct Relocation {
    uint32_t offset;            // موقع التصحيح في الكود
    std::string symbol;         // اسم الرمز المستهدف
    RelocationType type;        // نوع التصحيح
    int32_t addend;             // إضافة للعنوان
    
    Relocation(uint32_t off, const std::string& sym, RelocationType t, int32_t add = 0)
        : offset(off), symbol(sym), type(t), addend(add) {}
};

class RelocationTable {
private:
    std::vector<Relocation> relocations;
    
public:
    void add(uint32_t offset, const std::string& symbol, 
             RelocationType type, int32_t addend = 0) {
        relocations.emplace_back(offset, symbol, type, addend);
    }
    
    // ✅ NEW: Get relocations for modification
    std::vector<Relocation>& getRelocations() { return relocations; }
    const std::vector<Relocation>& getRelocations() const { return relocations; }
    
    void resolve(std::vector<uint8_t>& code, const SymbolTable& symbols) {
        // ✅ UPDATED: Use RVA-based calculation
        // Assume code starts at RVA 0x1000 (.text section base)
        uint32_t codeBaseRVA = 0x1000;
        
        std::cout << "🔧 Resolving " << relocations.size() << " relocations..." << std::endl;
        
        for (const auto& reloc : relocations) {
            std::cout << "  Reloc @ offset " << reloc.offset
                      << " → symbol: " << reloc.symbol << std::endl;

            if (!symbols.hasSymbol(reloc.symbol)) {
                throw std::runtime_error("Undefined symbol: " + reloc.symbol);
            }
            
            Symbol sym = symbols.getSymbol(reloc.symbol);
            if (!sym.resolved) {
                throw std::runtime_error("Unresolved symbol: " + reloc.symbol);
            }
            
            std::cout << "    Symbol RVA: 0x" << std::hex << sym.offset << std::dec << std::endl;
            
            // ✅ Target is now an RVA (absolute virtual address)
            uint32_t targetRVA = sym.offset + reloc.addend;
            
            // ✅ Current RVA = base + offset in code
            uint32_t currentRVA = codeBaseRVA + reloc.offset;
            
            switch (reloc.type) {
                case RelocationType::REL32: {
                    // relative offset = targetRVA - (currentRVA + 4)
                    // +4 because displacement is relative to NEXT instruction
                    int32_t displacement = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 4);
                    std::cout << "    REL32: disp = " << displacement << std::endl;
                    *reinterpret_cast<int32_t*>(&code[reloc.offset]) = displacement;
                    break;
                }
                case RelocationType::ABS64: {
                    *reinterpret_cast<uint64_t*>(&code[reloc.offset]) = targetRVA;
                    break;
                }
                case RelocationType::RIP_REL32: {
                    // RIP-relative: targetRVA - (currentRVA + 4)
                    int32_t displacement = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 4);
                    std::cout << "    RIP_REL32: current=0x" << std::hex << currentRVA 
                              << " target=0x" << targetRVA 
                              << " disp=" << std::dec << displacement << std::endl;
                    *reinterpret_cast<int32_t*>(&code[reloc.offset]) = displacement;
                    break;
                }
            }
        }
        
        std::cout << "✅ Relocations resolved" << std::endl;
    }
    
    size_t count() const { return relocations.size(); }
    
    void clear() { relocations.clear(); }
};


} // namespace ArabicLanguage

// Implementation of ClassInfo::inheritsFrom after SymbolTable is fully defined
inline bool ArabicLanguage::ClassInfo::inheritsFrom(const ArabicLanguage::SymbolTable& symbols, const std::string& baseClassName) const {
    if (name == baseClassName) return true;
    if (parentClass.empty()) return false;
    if (symbols.hasClass(parentClass)) {
        return symbols.getClass(parentClass).inheritsFrom(symbols, baseClassName);
    }
    return false;
}

#endif // SYMBOL_TABLE_H
