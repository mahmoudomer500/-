// CodeGenerator.h - مولد الكود من AST إلى x64
// المرحلة 2: ترجمة أوامر اللغة العربية إلى تعليمات x64

#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "X64Encoder.h"
#include "SymbolTable.h"
#include "ArabicParser.h"  // للحصول على Command structure
#include <vector>
#include <string>
#include <memory>
#include <map>

namespace ArabicAssembler {

// Import Command type from ArabicLanguage namespace
using ArabicLanguage::Command;
using ArabicLanguage::CommandType;
using ArabicLanguage::Value;
using ArabicLanguage::ValueType;
using ArabicLanguage::SymbolTable;
using ArabicLanguage::Symbol;
using ArabicLanguage::SymbolType;
using ArabicLanguage::SymbolDataType;
using ArabicLanguage::ClassInfo;
using ArabicLanguage::RelocationTable;
using ArabicLanguage::RelocationType;
// Instruction and InstructionType are in ArabicAssembler namespace

// ════════════════════════════════════════════════════════════
// 🎯 مولد الكود الرئيسي
// ════════════════════════════════════════════════════════════

class CodeGenerator {
protected:
    X64Encoder encoder;
    SymbolTable symbols;
    RelocationTable relocations;
    
    std::vector<uint8_t> code;
    std::vector<uint8_t> dataSection;
    
    // Data labels
    std::map<std::string, uint32_t> stringLiterals;
    uint32_t nextStringId = 0;
    
    // OOP Context
    bool isInsideFunction = false;
    std::string currentClassName; // ✅ Track current class context
    
    // ✅ Loop Control Context - for break/continue
    struct LoopContext {
        std::string startLabel;  // Label to jump back to (continue)
        std::string endLabel;    // Label to jump forward to (break)
    };
    std::vector<LoopContext> loopStack;  // Stack of nested loops
    
    // ═══ Helper Functions ═══
    
    virtual void emit(const Instruction& instr);
    
    void emitAll(const std::vector<Instruction>& instructions) {
        for (const auto& instr : instructions) {
            emit(instr);
        }
    }
    
    uint32_t getCurrentOffset() const {
        return static_cast<uint32_t>(code.size());
    }
    
    
    std::string addStringLiteral(const std::string& str) {
        std::string label = "str_" + std::to_string(nextStringId++);
        
        // ✅ NEW: Store offset in dataSection
        uint32_t dataOffset = static_cast<uint32_t>(dataSection.size());
        
        // Add string to dataSection (not code!)
        for (char c : str) {
            dataSection.push_back(static_cast<uint8_t>(c));
        }
        dataSection.push_back(0);  // null terminator
        
        // Register label with .data RVA (0x2000 + offset)
        // .data section starts at RVA 0x2000
        uint32_t dataBaseRVA = 0x2000;
        uint32_t labelRVA = dataBaseRVA + dataOffset;
        
        // ✅ FIX: Use addLabel (now adds to global symbols)
        symbols.addLabel(label, labelRVA);
        
        return label;
    }
    
    // ═══ Code Generation Functions ═══
    
    void prepareImports();
    void generateFunctionPrologue();
    void generateFunctionEpilogue();
    
    void generateVariableDeclaration(const Command& cmd);
    void generateAssignment(const Command& cmd);
    void generateExpression(const Value& val);
    void generateCondition(const Value& cond, const std::string& jumpLabel);
    void generateIfRecursive(const Command& cmd, const std::string& endLabel);
    void generateElseIf(const Command& cmd);
    void generateElse(const Command& cmd);
    void generateLambdaDef(const Command& cmd);
    InstructionType getInverseJump(const std::string& op);
    
    void generatePrint(const Command& cmd);
    void generateIf(const Command& cmd);
    void generateWhile(const Command& cmd);
    void generateFor(const Command& cmd);
    void generateForEach(const Command& cmd);
    void generateBreak(const Command& cmd);      // ✅ توقف
    void generateContinue(const Command& cmd);   // ✅ استمر
    void generateImport(const Command& cmd);     // ✅ استورد
    
    // Phase 5: Integer I/O
    void generateIntegerToString();  // Generates itoa routine
    void generatePrintInteger(const Value& val);  // Prints an integer value
    
    // Phase 4: Functions
    void generateFunctionDefinition(const Command& cmd);
    void generateFunctionCall(const Command& cmd);
    void generateReturn(const Command& cmd);
    
    // File I/O operations
    void generateFileOpen(const Command& cmd);
    void generateFileRead(const Command& cmd);
    void generateFileWrite(const Command& cmd);
    void generateFileClose(const Command& cmd);
    
    // Array operations
    void generateArrayDeclaration(const Command& cmd);
    void generateArrayAccess(const Value& val);  // For reading arr[i]
    void generateArrayAssignment(const Command& cmd);  // For arr[i] = value
    
    // OOP Operations
    void generateClassDefinition(const Command& cmd);
    void generateNewObject(const Command& cmd);
    void generateMethodCall(const Command& cmd);
    void generatePropertyAssignment(const Command& cmd);
    void generatePropertyAccess(const Value& val);
    void generatePropertyArrayAssignment(const Command& cmd); // ✅ obj.prop[i] = val
    
    // Function Table: Name -> Offset
    std::map<std::string, uint32_t> functionTable;
    
public:
    CodeGenerator() = default;
    virtual ~CodeGenerator() = default;
    
    // ═══ Main Generation Function ═══
    
    virtual std::vector<uint8_t> generate(const std::vector<std::shared_ptr<Command>>& commands);
    
    // ═══ Symbol Table Access ═══
    SymbolTable& getSymbols() { return symbols; }
    const SymbolTable& getSymbols() const { return symbols; }
    
    // ═══ Build Executable ═══
    bool buildExecutable(const std::string& outputPath);
    
    // ═══ Command Processing ═══
    
    void generateCommand(const Command& cmd);
    
    // ═══ Setup Functions ═══
    
    void setupImports() {
        // KERNEL32.DLL
        symbols.addImport("ExitProcess", 0);
        symbols.addImport("GetStdHandle", 1);
        symbols.addImport("WriteFile", 2);
        symbols.addImport("Sleep", 3);
        symbols.addImport("GetProcessHeap", 36);
        symbols.addImport("HeapAlloc", 37);
        symbols.addImport("HeapFree", 38);
        symbols.addImport("HeapReAlloc", 39);
        
        // msvcrt.dll (Making these optional or replacing them)
        symbols.addImport("fopen", 4);
        symbols.addImport("fclose", 5);
        symbols.addImport("fgets", 6);
        symbols.addImport("fread", 7);
        symbols.addImport("fprintf", 8);
        symbols.addImport("printf", 9);
        symbols.addImport("malloc", 10);
        symbols.addImport("free", 11);
        symbols.addImport("exit", 12);
        symbols.addImport("strlen", 13);
        symbols.addImport("strcpy", 14);
        symbols.addImport("strcat", 15);
        symbols.addImport("memcpy", 16);
        symbols.addImport("_access", 17);
        symbols.addImport("remove", 18);
        symbols.addImport("rename", 19);
        symbols.addImport("time", 20);
        symbols.addImport("getenv", 21);
        symbols.addImport("system", 22);
        symbols.addImport("pow", 23);
        symbols.addImport("rand", 24);
        symbols.addImport("srand", 25);
        symbols.addImport("fseek", 26);
        symbols.addImport("ftell", 27);
        symbols.addImport("strncmp", 32);
        symbols.addImport("toupper", 33);
        symbols.addImport("tolower", 34);

        // Arabic Runtime / GC imports (if linked externally)
        symbols.addImport("arabic_malloc", 28);
        symbols.addImport("arabic_free", 29);
        symbols.addImport("arabic_increment_ref", 30);
        symbols.addImport("arabic_decrement_ref", 31);
        symbols.addImport("realloc", 35);
    }
    
    // ═══ Getters ═══
    
    const std::vector<uint8_t>& getCode() const { return code; }
    const std::vector<uint8_t>& getData() const { return dataSection; }
    
    // Statistics
    std::map<std::string, int> getStatistics() const;

private:
    // فاحص مساحة الظل للتحقق من توافق ABI
    void runShadowSpaceCheck();
};

} // namespace ArabicAssembler

#endif // CODE_GENERATOR_H
