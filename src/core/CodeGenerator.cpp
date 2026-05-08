// CodeGenerator.cpp - تنفيذ مولد الكود

#include "CodeGenerator.h"
#include "PECompiler.h"
// #include "ShadowSpaceChecker.h"
#include <iostream>
#include <cstdint>
#include <fstream>
#include <sstream>

namespace ArabicAssembler {

using namespace ArabicLanguage;

// ════════════════════════════════════════════════════════════
// 🎯 Main Generation Function
// ════════════════════════════════════════════════════════════

// ════════════════════════════════════════════════════════════
// 🎯 Main Generation Function
// ════════════════════════════════════════════════════════════

std::vector<uint8_t> CodeGenerator::generate(const std::vector<std::shared_ptr<Command>>& commands) {
    // Performance optimization: Pre-allocate vectors with estimated sizes
    code.reserve(1024); // Reserve 1KB for code initially
    dataSection.reserve(256); // Reserve 256 bytes for data initially

    code.clear();
    dataSection.clear();
    this->symbols.resetStack();
    this->relocations.clear();
    
    // إضافة imports الأساسية
    setupImports();
    
    // Split commands into classes, functions, lambdas, imports and main code
    std::vector<std::shared_ptr<Command>> classes;
    std::vector<std::shared_ptr<Command>> functions;
    std::vector<std::shared_ptr<Command>> lambdas;
    std::vector<std::shared_ptr<Command>> imports;
    std::vector<std::shared_ptr<Command>> mainCode;

    for (const auto& cmd : commands) {
        if (cmd->type == CommandType::CLASS_DEF) {
            classes.push_back(cmd);
        } else if (cmd->type == CommandType::FUNCTION_DEF) {
            functions.push_back(cmd);
        } else if (cmd->type == CommandType::LAMBDA_DEF) {
            lambdas.push_back(cmd);
        } else if (cmd->type == CommandType::IMPORT) {
            imports.push_back(cmd);
        } else {
            mainCode.push_back(cmd);
        }
    }

    // ✅ إصلاح: تهيئة جدول الدوال مسبقاً لضمان وجودها في functionTable
    for (const auto& func : functions) {
        std::string funcName = func->variable;
        if (funcName.empty() && !func->function_name.empty()) {
            funcName = func->function_name;
        }
        if (!funcName.empty()) {
            functionTable[funcName] = 0;  // سيتم تحديثه عند التوليد
            // أضف أيضاً إلى symbols للتأكد من وجودها مع resolved = true
            Symbol sym(funcName, SymbolType::FUNCTION, 0);
            sym.resolved = true;  // تعليمها كمحلولة من البداية
            this->symbols.addSymbol(sym);
        }
    }
    
    // Function prologue
    this->generateFunctionPrologue();
    
    // If there are classes/functions, jump over them to main code
    std::string mainLabel = "main_code";
    if (!classes.empty() || !functions.empty()) {
        uint32_t jmpOffset = this->getCurrentOffset();
        this->emit(X64Encoder::JMP(500)); // Force Near JMP
        this->relocations.add(jmpOffset + 1, mainLabel, RelocationType::REL32);
    }
    
    // ✅ Process imports FIRST so parent classes are registered before child classes
    for (const auto& imp : imports) {
        this->generateCommand(*imp);
    }
    
    // Generate class definitions first (their methods become functions)
    for (const auto& cls : classes) {
        this->generateCommand(*cls);
    }
    
    // Generate function definitions (they won't execute immediately)
    for (const auto& func : functions) {
        this->generateCommand(*func);
    }

    // Generate lambda definitions
    for (const auto& lambda : lambdas) {
        this->generateCommand(*lambda);
    }

    // Phase 2: Update all function symbols with their final offsets
    // This ensures that any relocations created during main code generation will find the correct addresses
    for (const auto& func : functions) {
        std::string funcName = func->variable;
        if (funcName.empty() && !func->function_name.empty()) {
            funcName = func->function_name;
        }
        if (!funcName.empty() && functionTable.find(funcName) != functionTable.end()) {
            uint32_t funcOffset = functionTable[funcName];
            if (this->symbols.hasSymbol(funcName)) {
                Symbol& sym = this->symbols.getSymbol(funcName);
                sym.offset = funcOffset;
                sym.resolved = true;
                
            }
        }
    }

    // Label for main code
    if (!classes.empty() || !functions.empty()) {
        this->symbols.addLabel(mainLabel, this->getCurrentOffset());
    }

    // Generate main code
    for (const auto& cmd : mainCode) {
        this->generateCommand(*cmd);
    }
    
    // 🎯 الخطوة الأخيرة: فحص Shadow Space لضمان توافق ABI
    runShadowSpaceCheck();
    
    // وضع وسم النهاية للمغادرة النظيفة
    this->symbols.addLabel("global_exit", this->getCurrentOffset());
    this->generateFunctionEpilogue();

    return code;
}

// ════════════════════════════════════════════════════════════
// 🏗️ Build Executable
// ════════════════════════════════════════════════════════════

bool CodeGenerator::buildExecutable(const std::string& outputPath) {
    // 1. Ensure code is generated
    if (code.empty()) {
        std::cerr << "Error: No code generated yet. Call generate() first." << std::endl;
        return false;
    }
    
    // 2. Prepare Imports
    // 2. Prepare Imports
    std::vector<ImprovedPEBuilder::ImportDLL> peImports;
    
    ImprovedPEBuilder::ImportDLL kernel32;
    kernel32.name = "KERNEL32.DLL";
    
    ImprovedPEBuilder::ImportDLL msvcrt;
    msvcrt.name = "MSVCRT.DLL";
    
    auto importNames = this->symbols.getImportNames();
    for (const auto& name : importNames) {
        // Check if function belongs to MSVCRT
        if (name == "fopen" || name == "fclose" || name == "fgets" || 
            name == "fread" || name == "fprintf" || name == "printf" || 
            name == "malloc" || name == "realloc" || name == "free" || name == "exit" ||
            name == "strlen" || name == "strcpy" || name == "strcat" ||
            name == "memcpy" || name == "_access" || name == "remove" ||
            name == "rename" || name == "time" || name == "getenv" ||
            name == "system" || name == "pow" || name == "rand" ||
            name == "srand" || name == "fseek" || name == "ftell") {
            msvcrt.functions.push_back({name, 0});
        } else if (name == "arabic_malloc" || name == "arabic_free" || 
                   name == "arabic_increment_ref" || name == "arabic_decrement_ref") {
            // These could be in a separate runtime DLL or linked statically
            // For now, let's assume they are in msvcrt for simplicity or handled elsewhere
            msvcrt.functions.push_back({name, 0});
        } else {
            // Default to KERNEL32 (ExitProcess, GetStdHandle, WriteFile, Sleep, etc.)
            kernel32.functions.push_back({name, 0});
        }
    }
    
    if (!kernel32.functions.empty()) {
        peImports.push_back(kernel32);
    }
    
    // ✅ MSVCRT is now only included if it has functions actually being used
    if (!msvcrt.functions.empty()) {
        peImports.push_back(msvcrt);
    }
    
    // 4. Call PECompiler with symbols, relocations, and dataSection
    // ✅ PECompiler now handles relocation resolution after merging thunks + mainCode
    return PECompiler::compile(outputPath, code, dataSection, peImports, symbols, relocations);
}

// ════════════════════════════════════════════════════════════
// 🎯 Function Prologue/Epilogue
// ════════════════════════════════════════════════════════════

void CodeGenerator::generateFunctionPrologue() {
    // Standard x64 function prologue
    this->emit(X64Encoder::PUSH(Register::RBP));
    this->emit(X64Encoder::MOV(Register::RBP, Register::RSP));
    this->emit(Instruction(InstructionType::SUB, 
                     Operand::Reg(Register::RSP), 
                     Operand::Imm(72)));  // Reserve stack space (72 + 8 push = 80, 16-aligned)
}

void CodeGenerator::generateFunctionEpilogue() {
    // Standard x64 function epilogue
    // Value in RAX (if any) should be moved to RCX for ExitProcess
    this->emit(Instruction(InstructionType::MOV, 
                     Operand::Reg(Register::RCX), 
                     Operand::Reg(Register::RAX)));
    
    // Call ExitProcess
    uint32_t callOffset = this->getCurrentOffset() + 1;
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(callOffset, "ExitProcess", RelocationType::REL32);
}

// ════════════════════════════════════════════════════════════
// 📝 Command Processing
// ════════════════════════════════════════════════════════════

void CodeGenerator::generateCommand(const Command& cmd) {
    switch (cmd.type) {
        case CommandType::ASSIGN:
        case CommandType::ASSIGNMENT:
            generateVariableDeclaration(cmd);
            break;
            
        case CommandType::PRINT:
        case CommandType::PRINT_NO_NEWLINE:
            generatePrint(cmd);
            break;
            
        case CommandType::CONDITION:
            generateIf(cmd);
            break;

        case CommandType::ELSE_IF:
            this->generateElseIf(cmd);
            break;

        case CommandType::ELSE:
            this->generateElse(cmd);
            break;
            
        case CommandType::LOOP_WHILE:
            this->generateWhile(cmd);
            break;
        case CommandType::LOOP_FOR:
            this->generateFor(cmd);
            break;
        case CommandType::LOOP_FOR_EACH:
            this->generateForEach(cmd);
            break;
            
        case CommandType::FUNCTION_CALL:
            this->generateFunctionCall(cmd);
            break;
            
        case CommandType::FUNCTION_DEF:
            this->generateFunctionDefinition(cmd);
            break;

        case CommandType::LAMBDA_DEF:
            this->generateLambdaDef(cmd);
            break;

        case CommandType::RETURN:
            generateReturn(cmd);
            break;
        
        // File I/O operations
        case CommandType::FILE_OPEN:
            generateFileOpen(cmd);
            break;
            
        case CommandType::FILE_READ:
        case CommandType::FILE_READ_ALL:
            generateFileRead(cmd);
            break;
            
        case CommandType::FILE_WRITE:
            generateFileWrite(cmd);
            break;
            
        case CommandType::FILE_CLOSE:
            generateFileClose(cmd);
            break;
            
        // OOP Operations
        case CommandType::CLASS_DEF:
            generateClassDefinition(cmd);
            break;
            
        case CommandType::CREATE_OBJECT:
            generateNewObject(cmd);
            break;
            
        case CommandType::METHOD_CALL:
            generateMethodCall(cmd);
            break;
            
        case CommandType::ARRAY_ASSIGNMENT:
            this->generateArrayAssignment(cmd);
            break;

        case CommandType::PROPERTY_ARRAY_ASSIGNMENT:
            this->generatePropertyArrayAssignment(cmd);
            break;

        // ✅ Loop Control
        case CommandType::BREAK:
            generateBreak(cmd);
            break;

        case CommandType::CONTINUE:
            generateContinue(cmd);
            break;
        
        case CommandType::IMPORT: {
            std::string importPath = cmd.variable;
            
            
            std::string fullPath = importPath;
            if (importPath.find('/') == std::string::npos && importPath.find('\\') == std::string::npos) {
                std::string searchPaths[] = {
                    "examples/" + importPath,
                    "./examples/" + importPath,
                    "../examples/" + importPath
                };
                for (const auto& path : searchPaths) {
                    std::ifstream testFile(path);
                    if (testFile.is_open()) {
                        fullPath = path;
                        testFile.close();
                        break;
                    }
                }
            }
            
            std::ifstream importFile(fullPath);
            if (importFile.is_open()) {
                std::stringstream buffer;
                buffer << importFile.rdbuf();
                std::string importContent = buffer.str();
                importFile.close();
                
                // Parse the imported content
                ArabicParser tempParser;
                tempParser.setBasePath(importPath);
                SymbolTable tempSymbols;
                auto importedCmds = tempParser.parse(importContent, tempSymbols);
                
                // Add all classes from imported file to our symbol table
                for (const auto& importedCmd : importedCmds) {
                    if (importedCmd && importedCmd->type == CommandType::CLASS_DEF) {
                        // Generate the class first to register it
                        this->generateCommand(*importedCmd);
                    }
                }
                
                // Also process body commands from parser's import handling
                for (const auto& bodyCmd : cmd.body) {
                    this->generateCommand(*bodyCmd);
                }
            } else {
                std::cerr << "Warning: Could not open imported file: " << importPath << std::endl;
            }
            break;
        }
        
        default:
            // Handle any unsupported command types
            std::cerr << "Warning: Unsupported command type encountered: " 
                      << static_cast<int>(cmd.type) << std::endl;
            std::cerr << "This command type is not yet implemented in code generation." 
                      << std::endl;
            
            // Provide helpful debug information
            if (!cmd.variable.empty()) {
                std::cerr << "  Variable: " << cmd.variable << std::endl;
            }
            if (!cmd.function_name.empty()) {
                std::cerr << "  Function: " << cmd.function_name << std::endl;
            }
            if (!cmd.class_name.empty()) {
                std::cerr << "  Class: " << cmd.class_name << std::endl;
            }
            
            // For now, we silently continue rather than throwing an error
            // This allows the compiler to generate code for supported features
            // while logging what needs to be implemented
            break;
    }
}

// ════════════════════════════════════════════════════════════
// 🔀 Control Flow Helpers
// ════════════════════════════════════════════════════════════

InstructionType CodeGenerator::getInverseJump(const std::string& op) {
    if (op == "==") return InstructionType::JNE;
    if (op == "!=") return InstructionType::JE;
    if (op == "<")  return InstructionType::JGE;
    if (op == "<=") return InstructionType::JG;
    if (op == ">")  return InstructionType::JLE;
    if (op == ">=") return InstructionType::JL;
    throw std::runtime_error("Unknown comparison operator: " + op);
}

void CodeGenerator::generateCondition(const Value& cond, const std::string& jumpLabel) {
    // Evaluate condition and jump to jumpLabel if FALSE
    
    if (cond.type == ValueType::OPERATION || cond.type == ValueType::COMPARISON) {
        // ✅ Check for logical operators first
        std::string op = !cond.operation.empty() ? cond.operation : cond.operator_;
        
        // Handle AND (و) - both conditions must be true
        if (op == "و") {
            // If left is false, jump to fail label
            // If right is false, jump to fail label
            // Both must be true to continue
            if (cond.left) this->generateCondition(*cond.left, jumpLabel);
            if (cond.right) this->generateCondition(*cond.right, jumpLabel);
            return;
        }
        
        // Handle OR (أو) - short-circuit: if left is true, skip right
        if (op == "أو") {
            static int orCounter = 0;
            std::string id = std::to_string(orCounter++);
            std::string successLabel = "or_success_" + id;
            std::string evalRightLabel = "or_eval_right_" + id;

            if (cond.left) {
                // If left is false, jump to evalRightLabel; otherwise jump to successLabel
                this->generateCondition(*cond.left, evalRightLabel);
                uint32_t jmpOffset = this->getCurrentOffset();
                this->emit(X64Encoder::JMP(500));
                this->relocations.add(jmpOffset + 1, successLabel, RelocationType::REL32);
                this->symbols.addLabel(evalRightLabel, this->getCurrentOffset());
            }

            if (cond.right) {
                // If right is false, jump to fail
                this->generateCondition(*cond.right, jumpLabel);
            }

            this->symbols.addLabel(successLabel, this->getCurrentOffset());
            return;
        }
        
        // Handle NOT (ليس) - negate the condition
        if (op == "ليس") {
            static int notCounter = 0;
            std::string continueLabel = "not_continue_" + std::to_string(notCounter++);
            
            if (cond.left && cond.left->type == ValueType::COMPARISON) {
                // NOT of comparison: reverse jump logic
                if (cond.left->left) this->generateExpression(*cond.left->left);
                this->emit(X64Encoder::PUSH(Register::RAX));
                if (cond.left->right) this->generateExpression(*cond.left->right);
                this->emit(X64Encoder::MOV(Register::RBX, Register::RAX));
                this->emit(X64Encoder::POP(Register::RAX));
                this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RAX), Operand::Reg(Register::RBX)));
                InstructionType jumpToFail = InstructionType::JE;
                if (cond.left->operator_ == "==") jumpToFail = InstructionType::JE;
                else if (cond.left->operator_ == "!=") jumpToFail = InstructionType::JNE;
                else if (cond.left->operator_ == "<") jumpToFail = InstructionType::JL;
                else if (cond.left->operator_ == ">") jumpToFail = InstructionType::JG;
                else if (cond.left->operator_ == "<=") jumpToFail = InstructionType::JLE;
                else if (cond.left->operator_ == ">=") jumpToFail = InstructionType::JGE;
                uint32_t jumpOffset = this->getCurrentOffset();
                this->emit(Instruction(jumpToFail, Operand::Imm(0)));
                this->relocations.add(jumpOffset + 2, jumpLabel, RelocationType::REL32);
            } else if (cond.right) {
                // NOT of expression: evaluate, jump to fail if true (non-zero)
                try {
                    this->generateExpression(*cond.right);
                } catch (const std::exception& ex) {
                    
                    throw;
                }
                this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RAX), Operand::Imm(0)));
                uint32_t jeOffset = this->getCurrentOffset();
                this->emit(Instruction(InstructionType::JE, Operand::Imm(0)));
                this->relocations.add(jeOffset + 2, continueLabel, RelocationType::REL32);
                this->emit(X64Encoder::JMP(500)); // Use X64Encoder for consistent encoding
                this->relocations.add(this->getCurrentOffset() + 1, jumpLabel, RelocationType::REL32);
                this->symbols.addLabel(continueLabel, this->getCurrentOffset());
            }
            return;
        }
        
        // ✅ Standard comparison operators
        if (!cond.left || !cond.right) {
            throw std::runtime_error("Condition has null operand! Left=" + 
                                   std::string(cond.left ? "OK" : "NULL") + 
                                   ", Right=" + std::string(cond.right ? "OK" : "NULL"));
        }
        
        // 1. Evaluate Left -> RAX
        this->generateExpression(*cond.left);
        this->emit(X64Encoder::PUSH(Register::RAX));
        
        // 2. Evaluate Right -> RAX
        this->generateExpression(*cond.right);
        this->emit(X64Encoder::MOV(Register::RBX, Register::RAX));
        
        // 3. Pop Left -> RAX
        
        this->emit(X64Encoder::POP(Register::RAX));
        
        // 4. CMP RAX, RBX
        
        this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RAX), Operand::Reg(Register::RBX)));
        
        // 5. Jump if FALSE (Inverse Logic)
        std::string cmpOp = !cond.operator_.empty() ? cond.operator_ : cond.operation;
        
        InstructionType jumpType = this->getInverseJump(cmpOp);
        uint32_t jumpOffset = this->getCurrentOffset();
        
        
        Instruction instr(jumpType, Operand::Imm(0));
        this->emit(instr);
        
        // Jcc is 0F 8x cd cd cd cd (6 bytes, operand at +2)
        this->relocations.add(jumpOffset + 2, jumpLabel, RelocationType::REL32);
    } else {
        // Simple expression (FUNCTION_CALL, VARIABLE, etc.): evaluate and jump if false (zero)
        this->generateExpression(cond);
        this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RAX), Operand::Imm(0)));
        uint32_t jeOffset = this->getCurrentOffset();
        this->emit(Instruction(InstructionType::JE, Operand::Imm(0)));
        this->relocations.add(jeOffset + 2, jumpLabel, RelocationType::REL32);
    }
}

void CodeGenerator::generateIf(const Command& cmd) {
    // إذا (condition) { body } [وإلا { else_body }]
    
    if (!cmd.condition) {
        throw std::runtime_error("generateIf: condition is null");
    }
    
    static int labelCounter = 0;
    std::string id = std::to_string(labelCounter++);
    std::string endLabel = "if_end_" + id;
    std::string elseLabel = "if_else_" + id;
    
    // 1. Determine Jump Target (Else or End)
    std::string jumpTarget = cmd.else_body.empty() ? endLabel : elseLabel;
    
    // 2. Generate Condition (Jumps to jumpTarget if false)
    if (cmd.condition) {
        this->generateCondition(*cmd.condition, jumpTarget);
    } else {
        std::cerr << "Warning: If statement missing condition" << std::endl;
        this->emit(X64Encoder::JMP(500));
        this->relocations.add(this->getCurrentOffset() + 1, jumpTarget, RelocationType::REL32);
    }
    
    // 3. Generate If Body
    for (const auto& bodyCmd : cmd.body) {
        this->generateCommand(*bodyCmd);
    }
    
    // 4. Handle Else Block
    if (!cmd.else_body.empty()) {
        // Jump to End after If Body
        uint32_t jmpOffset = this->getCurrentOffset();
        this->emit(X64Encoder::JMP(500)); // Force Near JMP
        this->relocations.add(jmpOffset + 1, endLabel, RelocationType::REL32);
        
        // Place Else Label
        this->symbols.addLabel(elseLabel, this->getCurrentOffset());
        
        // Generate Else Body
        for (const auto& elseCmd : cmd.else_body) {
            this->generateCommand(*elseCmd);
        }
    }
    
    // 5. Place End Label
    this->symbols.addLabel(endLabel, this->getCurrentOffset());
}

void CodeGenerator::generateElseIf(const Command& cmd) {
    // وإلا إذا (condition) { body } [وإلا { else_body }]
    
    static int elseifCounter = 0;
    std::string id = std::to_string(elseifCounter++);
    std::string endLabel = "elseif_end_" + id;
    std::string elseLabel = "elseif_else_" + id;

    // 1. Determine Jump Target (Next ElseIf/Else or End)
    std::string jumpTarget = cmd.else_body.empty() ? endLabel : elseLabel;

    // 2. Generate Condition (Jumps to jumpTarget if false)
    this->generateCondition(*cmd.condition, jumpTarget);

    // 3. Generate ElseIf Body
    for (const auto& bodyCmd : cmd.body) {
        this->generateCommand(*bodyCmd);
    }

    // 4. Handle nested ElseIf/Else
    if (!cmd.else_body.empty()) {
        // Jump to end of current ElseIf block
        uint32_t jmpOffset = this->getCurrentOffset();
        this->emit(X64Encoder::JMP(500)); 
        this->relocations.add(jmpOffset + 1, endLabel, RelocationType::REL32);

        // Place Else Label
        this->symbols.addLabel(elseLabel, this->getCurrentOffset());

        // Generate Else/ElseIf Body
        for (const auto& elseCmd : cmd.else_body) {
            this->generateCommand(*elseCmd);
        }
    }

    // 5. Place End Label
    this->symbols.addLabel(endLabel, this->getCurrentOffset());
}

void CodeGenerator::generateElse(const Command& cmd) {
    // وإلا { body }
    // This is just the else body without condition

    // Generate Else Body
    for (const auto& bodyCmd : cmd.body) {
        this->generateCommand(*bodyCmd);
    }
}

void CodeGenerator::generateWhile(const Command& cmd) {
    // طالما (condition) { body }
    
    static int loopCounter = 0;
    std::string startLabel = "while_start_" + std::to_string(loopCounter);
    std::string endLabel = "while_end_" + std::to_string(loopCounter);
    loopCounter++;
    
    // ✅ Push loop context for break/continue
    this->loopStack.push_back({startLabel, endLabel});
    
    // 1. Place Start Label
    uint32_t startOffset = this->getCurrentOffset();
    this->symbols.addLabel(startLabel, startOffset);
    
    // 2. Generate Condition (Jumps to jumpTarget if false)
    if (cmd.condition) {
        this->generateCondition(*cmd.condition, endLabel);
    } else {
        // Infinite loop if no condition? Standard for 'طالما' would be a crash or error in grammar.
        // For now, let's treat null as false to avoid crash.
        std::cerr << "Warning: While loop missing condition" << std::endl;
        this->emit(X64Encoder::JMP(500));
        this->relocations.add(this->getCurrentOffset() + 1, endLabel, RelocationType::REL32);
    }
    
    // 3. Generate Body
    for (const auto& bodyCmd : cmd.body) {
        this->generateCommand(*bodyCmd);
    }
    
    // 4. Jump back to Start
    uint32_t jmpOffset = this->getCurrentOffset();
    // Use a large placeholder (e.g., 500) to force Near JMP (5 bytes)
    // X64Encoder uses Short JMP (2 bytes) if offset fits in 8 bits.
    // We need 32-bit relocation space.
    this->emit(X64Encoder::JMP(500)); 
    
    // JMP is E9 cd cd cd cd (1 byte opcode, operand at +1)
    this->relocations.add(jmpOffset + 1, startLabel, RelocationType::REL32);
    
    // 5. Place End Label
    uint32_t endOffset = this->getCurrentOffset();
    this->symbols.addLabel(endLabel, endOffset);
    
    // ✅ Pop loop context
    this->loopStack.pop_back();
}

void CodeGenerator::generateFor(const Command& cmd) {
    static int loopCounter = 0;
    std::string startLabel = "for_start_" + std::to_string(loopCounter);
    std::string endLabel = "for_end_" + std::to_string(loopCounter);
    std::string ascLabel = "for_asc_" + std::to_string(loopCounter);
    std::string descLabel = "for_desc_" + std::to_string(loopCounter);
    loopCounter++;

    std::string varName = cmd.variable;

    if (!this->symbols.hasSymbol(varName)) {
        this->symbols.addVariable(varName, 8, SymbolDataType::INTEGER);
    }

    uint32_t varOffset = this->symbols.getSymbol(varName).offset;
    
    if (!cmd.condition) {
        std::cerr << "Warning: For loop missing 'to' expression (condition)" << std::endl;
        return;
    }

    // 1. Evaluate 'from' -> RAX -> [var]
    this->generateExpression(cmd.value);
    this->emit(Instruction(InstructionType::MOV,
                     Operand::Mem(Register::RBP, -static_cast<int32_t>(varOffset)),
                     Operand::Reg(Register::RAX)));

    // 2. Evaluate 'step' (default 1) -> RAX -> PUSH onto stack
    if (!cmd.arguments.empty()) {
        this->generateExpression(*cmd.arguments[0]);
    } else {
        this->emit(X64Encoder::MOV(Register::RAX, 1));
    }
    this->emit(X64Encoder::PUSH(Register::RAX)); // Step is now at [RSP]

    this->loopStack.push_back({startLabel, endLabel});

    this->symbols.addLabel(startLabel, this->getCurrentOffset());

    // 3. Peak step from stack into RCX for sign checking
    this->emit(X64Encoder::MOV(Register::RCX, Operand::Mem(Register::RSP, 0)));
    this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RCX), Operand::Imm(0)));
    uint32_t jlOffset = this->getCurrentOffset();
    this->emit(Instruction(InstructionType::JL, Operand::Imm(0)));
    this->relocations.add(jlOffset + 2, descLabel, RelocationType::REL32);

    // --- Ascending path ---
    this->symbols.addLabel(ascLabel, this->getCurrentOffset());
    // i > to ? break
    this->emit(Instruction(InstructionType::MOV,
                     Operand::Reg(Register::RAX),
                     Operand::Mem(Register::RBP, -static_cast<int32_t>(varOffset))));
    this->emit(X64Encoder::PUSH(Register::RAX));
    this->generateExpression(*cmd.condition); // Evaluating 'to'
    this->emit(X64Encoder::MOV(Register::RBX, Register::RAX));
    this->emit(X64Encoder::POP(Register::RAX));
    this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RAX), Operand::Reg(Register::RBX)));
    uint32_t jgOffset = this->getCurrentOffset();
    this->emit(Instruction(InstructionType::JG, Operand::Imm(0)));
    this->relocations.add(jgOffset + 2, endLabel, RelocationType::REL32);

    for (const auto& bodyCmd : cmd.body) {
        this->generateCommand(*bodyCmd);
    }

    // i = i + step
    this->emit(Instruction(InstructionType::MOV,
                     Operand::Reg(Register::RAX),
                     Operand::Mem(Register::RBP, -static_cast<int32_t>(varOffset))));
    this->emit(X64Encoder::MOV(Register::RCX, Operand::Mem(Register::RSP, 0))); // Restore step
    this->emit(Instruction(InstructionType::ADD,
                     Operand::Reg(Register::RAX),
                     Operand::Reg(Register::RCX)));
    this->emit(Instruction(InstructionType::MOV,
                     Operand::Mem(Register::RBP, -static_cast<int32_t>(varOffset)),
                     Operand::Reg(Register::RAX)));

    uint32_t jmpOffset = this->getCurrentOffset();
    this->emit(X64Encoder::JMP(500));
    this->relocations.add(jmpOffset + 1, startLabel, RelocationType::REL32);

    // --- Descending path ---
    this->symbols.addLabel(descLabel, this->getCurrentOffset());
    // i < to ? break
    this->emit(Instruction(InstructionType::MOV,
                     Operand::Reg(Register::RAX),
                     Operand::Mem(Register::RBP, -static_cast<int32_t>(varOffset))));
    this->emit(X64Encoder::PUSH(Register::RAX));
    this->generateExpression(*cmd.condition); // Evaluating 'to'
    this->emit(X64Encoder::MOV(Register::RBX, Register::RAX));
    this->emit(X64Encoder::POP(Register::RAX));
    this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RAX), Operand::Reg(Register::RBX)));
    uint32_t jl2Offset = this->getCurrentOffset();
    this->emit(Instruction(InstructionType::JL, Operand::Imm(0)));
    this->relocations.add(jl2Offset + 2, endLabel, RelocationType::REL32);

    for (const auto& bodyCmd : cmd.body) {
        this->generateCommand(*bodyCmd);
    }

    // i = i + step
    this->emit(Instruction(InstructionType::MOV,
                     Operand::Reg(Register::RAX),
                     Operand::Mem(Register::RBP, -static_cast<int32_t>(varOffset))));
    this->emit(X64Encoder::MOV(Register::RCX, Operand::Mem(Register::RSP, 0))); // Restore step
    this->emit(Instruction(InstructionType::ADD,
                     Operand::Reg(Register::RAX),
                     Operand::Reg(Register::RCX)));
    this->emit(Instruction(InstructionType::MOV,
                     Operand::Mem(Register::RBP, -static_cast<int32_t>(varOffset)),
                     Operand::Reg(Register::RAX)));

    uint32_t jmp2Offset = this->getCurrentOffset();
    this->emit(X64Encoder::JMP(500));
    this->relocations.add(jmp2Offset + 1, startLabel, RelocationType::REL32);

    this->symbols.addLabel(endLabel, this->getCurrentOffset());

    this->emit(X64Encoder::POP(Register::RAX)); // Clean up step from stack
    this->loopStack.pop_back();
}

void CodeGenerator::generateForEach(const Command& cmd) {
    static int loopCounter = 0;
    std::string startLabel = "foreach_start_" + std::to_string(loopCounter);
    std::string endLabel = "foreach_end_" + std::to_string(loopCounter);
    loopCounter++;

    std::string itemVar = cmd.variable;
    std::string arrayExpr = "";
    if (cmd.condition) {
        if (cmd.condition->object_name.empty()) {
            arrayExpr = cmd.condition->value;
        } else {
            arrayExpr = cmd.condition->object_name + "." + cmd.condition->property_name;
        }
    }

    if (!this->symbols.hasSymbol(itemVar)) {
        this->symbols.addVariable(itemVar, 8, SymbolDataType::INTEGER);
    }
    uint32_t itemOffset = this->symbols.getSymbol(itemVar).offset;

    this->loopStack.push_back({startLabel, endLabel});

    if (!arrayExpr.empty()) {
        this->generateExpression(*cmd.condition);
    }
    this->emit(X64Encoder::PUSH(Register::RAX));

    this->symbols.addLabel(startLabel, this->getCurrentOffset());
    this->emit(X64Encoder::MOV(Register::RCX, Operand::Mem(Register::RSP, 0)));
    this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RCX), Operand::Imm(0)));
    uint32_t jeOffset = this->getCurrentOffset();
    this->emit(Instruction(InstructionType::JE, Operand::Imm(0)));
    this->relocations.add(jeOffset + 2, endLabel, RelocationType::REL32);

    this->emit(X64Encoder::POP(Register::RCX));
    this->emit(Instruction(InstructionType::SUB,
                     Operand::Reg(Register::RCX), Operand::Imm(1)));
    this->emit(X64Encoder::PUSH(Register::RCX));

    this->emit(Instruction(InstructionType::MOV,
                     Operand::Reg(Register::RAX), Operand::Imm(0)));
    this->emit(X64Encoder::MOV(Register::RBX, Operand::Imm(8)));
    this->emit(Instruction(InstructionType::IMUL,
                     Operand::Reg(Register::RBX)));
    this->emit(Instruction(InstructionType::ADD,
                     Operand::Reg(Register::RAX), Operand::Reg(Register::RCX)));
    this->emit(Instruction(InstructionType::MOV,
                     Operand::Reg(Register::RBX), Operand::Reg(Register::RAX)));
    this->emit(Instruction(InstructionType::ADD,
                     Operand::Reg(Register::RBX), Operand::Imm(16)));
    this->emit(Instruction(InstructionType::MOV,
                     Operand::Reg(Register::RAX), Operand::Mem(Register::RBX, 0)));
    this->emit(Instruction(InstructionType::MOV,
                     Operand::Mem(Register::RBP, -static_cast<int32_t>(itemOffset)),
                     Operand::Reg(Register::RAX)));

    for (const auto& bodyCmd : cmd.body) {
        this->generateCommand(*bodyCmd);
    }

    uint32_t jmpOffset = this->getCurrentOffset();
    this->emit(X64Encoder::JMP(500));
    this->relocations.add(jmpOffset + 1, startLabel, RelocationType::REL32);

    this->symbols.addLabel(endLabel, this->getCurrentOffset());
    this->emit(X64Encoder::POP(Register::RAX));
    this->loopStack.pop_back();
}

// ✅ توقف (break) - jump to end of current loop
void CodeGenerator::generateBreak(const Command& /*cmd*/) {
    if (this->loopStack.empty()) {
        throw std::runtime_error("'توقف' (break) used outside of loop");
    }
    
    const LoopContext& ctx = this->loopStack.back();
    
    // Jump to end label
    uint32_t jmpOffset = this->getCurrentOffset();
    this->emit(X64Encoder::JMP(500)); // Force near jump
    this->relocations.add(jmpOffset + 1, ctx.endLabel, RelocationType::REL32);
}

// ✅ استمر (continue) - jump to start of current loop
void CodeGenerator::generateContinue(const Command& /*cmd*/) {
    if (this->loopStack.empty()) {
        throw std::runtime_error("'استمر' (continue) used outside of loop");
    }
    
    const LoopContext& ctx = this->loopStack.back();
    
    // Jump to start label
    uint32_t jmpOffset = this->getCurrentOffset();
    this->emit(X64Encoder::JMP(500)); // Force near jump
    this->relocations.add(jmpOffset + 1, ctx.startLabel, RelocationType::REL32);
}

// ════════════════════════════════════════════════════════════
// 🧮 Expression Evaluation
// ════════════════════════════════════════════════════════════

void CodeGenerator::generateExpression(const Value& val) {
    // Result will be in RAX
    
    
    
    if (val.type == ValueType::NONE) {
        return; 
    }
    
    if (val.type == ValueType::NUMBER) {
        // Immediate value - try both value and string_value for compatibility
        std::string numStr = !val.value.empty() ? val.value : val.string_value;
        if (numStr.empty()) {
            
            throw std::runtime_error("NUMBER value is empty!");
        }
        int64_t num = std::stoll(numStr);
        this->emit(X64Encoder::MOV(Register::RAX, num));
    }
    else if (val.type == ValueType::STRING) {
        // ✅ String literal - add to data section and load address
        std::string str = val.string_value;
        std::string label = this->addStringLiteral(str);
        
        // LEA RAX, [RIP + label]
        uint32_t leaOffset = this->getCurrentOffset() + 3; // LEA is 7 bytes, RIP offset at +3
        this->emit(Instruction(InstructionType::LEA,
                       Operand::Reg(Register::RAX),
                       Operand::Label(label)));
        this->relocations.add(leaOffset, label, RelocationType::RIP_REL32);
    }
    else if (val.type == ValueType::VARIABLE) {
        // Load variable value
        std::string varName = val.value;
        if (!this->symbols.hasSymbol(varName)) {
            throw std::runtime_error("Undefined variable: " + varName);
        }
        uint32_t stackOffset = this->symbols.getSymbol(varName).offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RAX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    }
    else if (val.type == ValueType::ARRAY_ACCESS) {
        // Array access: arr[index]
        this->generateArrayAccess(val);
        // Result will be in RAX
    }
    else if (val.type == ValueType::OBJECT_LITERAL) {
        // ✅ New: Handle { "key": value }
        static int objCounter = 0;
        std::string objId = std::to_string(objCounter++);
        
        // 1. Allocate memory: HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)
        size_t totalBytes = 16 + (val.map_elements.size() * 16);

        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        uint32_t gphOffsetObj = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(gphOffsetObj, "GetProcessHeap", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));

        this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));
        this->emit(X64Encoder::MOV(Register::RDX, 8)); // HEAP_ZERO_MEMORY
        this->emit(X64Encoder::MOV(Register::R8, static_cast<int64_t>(totalBytes)));

        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        uint32_t haOffsetObj = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(haOffsetObj, "HeapAlloc", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
        
        // RAX has object pointer. Move to RBX to preserve during evaluations.
        this->emit(X64Encoder::MOV(Register::RBX, Register::RAX));
        
        // 2. Set Header (Capacity, Count)
        this->emit(X64Encoder::MOV(Operand::Mem(Register::RBX, 0), static_cast<int64_t>(val.map_elements.size())));
        this->emit(X64Encoder::MOV(Operand::Mem(Register::RBX, 8), static_cast<int64_t>(val.map_elements.size())));
        
        // 3. Store properties
        int i = 0;
        for (const auto& pair : val.map_elements) {
            // Evaluated Value -> RAX
            this->generateExpression(pair.second);
            
            // Reload Object Pointer to RBX (in case generateExpression changed it)
            // Wait, we need to save RBX on stack if generateExpression might change it.
            this->emit(X64Encoder::PUSH(Register::RBX));
            this->emit(X64Encoder::PUSH(Register::RAX)); // Save value
            
            // Get Key String Pointer
            std::string keyLabel = this->addStringLiteral(pair.first);
            uint32_t leaOffset = this->getCurrentOffset() + 3;
            this->emit(Instruction(InstructionType::LEA, Operand::Reg(Register::RAX), Operand::Label(keyLabel)));
            this->relocations.add(leaOffset, keyLabel, RelocationType::RIP_REL32);
            
            this->emit(X64Encoder::MOV(Register::RCX, Register::RAX)); // RCX = KeyPtr
            this->emit(X64Encoder::POP(Register::RDX)); // RDX = Value
            this->emit(X64Encoder::POP(Register::RBX)); // RBX = ObjPtr
            
            // Store [KeyPtr, Value] at [RBX + 16 + i*16]
            this->emit(Instruction(InstructionType::MOV, Operand::Mem(Register::RBX, 16 + i*16), Operand::Reg(Register::RCX)));
            this->emit(Instruction(InstructionType::MOV, Operand::Mem(Register::RBX, 16 + i*16 + 8), Operand::Reg(Register::RDX)));
            i++;
        }
        
        // Result is ObjPtr in RBX -> RAX
        this->emit(X64Encoder::MOV(Register::RAX, Register::RBX));
    }
    else if (val.type == ValueType::PROPERTY_ACCESS) {
        // Property access: obj.prop
        this->generatePropertyAccess(val);
        // Result will be in RAX
    }
    else if (val.type == ValueType::OPERATION) {
        if (!val.left || !val.right) {
            if (val.operation == "ليس" && val.right) {
                this->generateExpression(*val.right);
                this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RAX), Operand::Imm(0)));
                this->emit(X64Encoder::MOV(Register::RAX, Operand::Imm(1)));
                this->code.push_back(0x0F); this->code.push_back(0x94); this->code.push_back(0xC0); // SETE AL
                this->code.push_back(0x48); this->code.push_back(0x0F); this->code.push_back(0xB6); this->code.push_back(0xC0); // MOVZX
                return; // NOT: 1 if zero, 0 if non-zero
            }
            throw std::runtime_error("Operation has null operand(s)");
        }

        // 1) Evaluate Left → RAX
        this->generateExpression(*val.left);

        // 2) Fast path: right is immediate for + or -
        std::string op = val.operation.empty() ? val.operator_ : val.operation;
        if (val.right->type == ValueType::NUMBER && (op == "+" || op == "-")) {
            std::string numStr = !val.right->value.empty() ? val.right->value : val.right->string_value;
            int64_t imm = std::stoll(numStr);
            if (op == "+") {
                this->emit(X64Encoder::ADD(Register::RAX, Operand::Imm(imm)));
            } else {
                this->emit(X64Encoder::SUB(Register::RAX, Operand::Imm(imm)));
            }
            return;
        }

        // 3) General path
        this->emit(X64Encoder::PUSH(Register::RAX));
        this->generateExpression(*val.right);
        this->emit(X64Encoder::MOV(Register::RBX, Register::RAX));
        this->emit(X64Encoder::POP(Register::RAX));

        if (op == "+") {
            this->emit(X64Encoder::ADD(Register::RAX, Register::RBX));
        }
        else if (op == "-") {
            this->emit(X64Encoder::SUB(Register::RAX, Register::RBX));
        }
        else if (op == "*") {
            this->emit(X64Encoder::IMUL(Register::RAX, Register::RBX));
        }
        else if (op == "/") {
            this->emit(X64Encoder::CQO());
            this->emit(X64Encoder::IDIV(Register::RBX));
        }
        else if (op == "%" || op == "٪") {
            this->emit(X64Encoder::CQO());
            this->emit(X64Encoder::IDIV(Register::RBX));
            this->emit(X64Encoder::MOV(Register::RAX, Register::RDX));
        }
        else if (op == "&") {
            this->emit(X64Encoder::AND(Register::RAX, Register::RBX));
        }
        else if (op == "|") {
            this->emit(X64Encoder::OR(Register::RAX, Register::RBX));
        }
        else if (op == "^") {
            this->emit(X64Encoder::XOR(Register::RAX, Register::RBX));
        }
        else if (op == "و" || op == "&&") {
            this->emit(X64Encoder::AND(Register::RAX, Register::RBX));
        }
        else if (op == "أو" || op == "||") {
            this->emit(X64Encoder::OR(Register::RAX, Register::RBX));
        }
    }
    else if (val.type == ValueType::OPERATION && val.operation == "ليس") {
        // Unary NOT: ليس X
        if (!val.right) throw std::runtime_error("Unary NOT missing operand");
        this->generateExpression(*val.right);
        
        // CMP RAX, 0
        this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RAX), Operand::Imm(0)));
        // SETE AL
        this->code.push_back(0x0F); this->code.push_back(0x94); this->code.push_back(0xC0);
        // MOVZX RAX, AL
        this->code.push_back(0x48); this->code.push_back(0x0F); this->code.push_back(0xB6); this->code.push_back(0xC0);
    }
    else if (val.type == ValueType::FUNCTION_CALL) {
        // Function call or Method call in expression
        
        if (!val.object_name.empty()) {
            // It's a method call: obj.method(args)
            Command callCmd(CommandType::METHOD_CALL);
            callCmd.object_name = val.object_name;
            callCmd.method_name = val.function_name;
            callCmd.arguments = val.arguments;
            
            this->generateMethodCall(callCmd);
        } else {
            // Regular function call: func(args)
            Command callCmd(CommandType::FUNCTION_CALL);
            callCmd.variable = val.value; // Function name
            callCmd.arguments = val.arguments;
            
            this->generateFunctionCall(callCmd);
        }
        // Result will be in RAX after the call
    }
}

// ════════════════════════════════════════════════════════════
// 💾 Variable Declaration
// ════════════════════════════════════════════════════════════

void CodeGenerator::generateVariableDeclaration(const Command& cmd) {
   // متغير س = 5 + 3
   // OR: مصفوفة أرقام = [1، 2، 3]
    
    std::string varName = cmd.variable;
    
    // Check if it's a property assignment (obj.prop = val)
    if (varName.find('.') != std::string::npos) {
        this->generatePropertyAssignment(cmd);
        return;
    }
    
    // Check if it's an array declaration
    if (cmd.value.type == ValueType::ARRAY) {
        this->generateArrayDeclaration(cmd);
        return;
    }
    
    // ✅ Detect variable type from value
    SymbolDataType dataType = SymbolDataType::INTEGER;
    if (cmd.value.type == ValueType::STRING) {
        dataType = SymbolDataType::STRING;
    }
    
    // Regular variable
    // If exists and is constant, only allow initialization via 'ثابت'
    if (this->symbols.hasSymbol(varName)) {
        Symbol existing = this->symbols.getSymbol(varName);
        if (existing.isConst && !cmd.isConst) {
            std::cerr << "Warning: Modifying constant variable: " << varName << std::endl;
        }
    } else {
        this->symbols.addVariable(varName, 8, dataType, "", cmd.isConst);
    }
    
    // Evaluate expression -> Result in RAX
    this->generateExpression(cmd.value);
    
    // Store RAX to stack
    uint32_t stackOffset = this->symbols.getSymbol(varName).offset;
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset)),
                   Operand::Reg(Register::RAX)));
}

void CodeGenerator::generateAssignment(const Command& cmd) {
    // س = 10
    
    std::string varName = cmd.variable;
    if (!this->symbols.hasSymbol(varName)) {
        throw std::runtime_error("Undefined variable: " + varName);
    }
    if (this->symbols.getSymbol(varName).isConst) {
        throw std::runtime_error("Cannot assign to constant: " + varName);
    }
    
    // Evaluate expression -> Result in RAX
    this->generateExpression(cmd.value);
    
    // Store RAX to stack
    uint32_t stackOffset = this->symbols.getSymbol(varName).offset;
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset)),
                   Operand::Reg(Register::RAX)));
}

// ════════════════════════════════════════════════════════════
// 🖨️ Print Statement
// ════════════════════════════════════════════════════════════

// Helper: fold constant string concatenation at compile time (returns empty if any part is non-constant)
static std::string foldConstantStringConcat(const Value& val) {
    if (val.type == ValueType::STRING) return val.string_value;
    if (val.type == ValueType::OPERATION && (val.operation == "+" || val.operator_ == "+")) {
        if (val.left && val.right) {
            std::string l = foldConstantStringConcat(*val.left);
            std::string r = foldConstantStringConcat(*val.right);
            if (!l.empty() && !r.empty()) return l + r; // Both constant
            if (!l.empty() || !r.empty()) return std::string(); // Partial - need runtime
        }
    }
    return std::string();
}

// Helper: print each part of a + expression (avoids wrong ADD for string pointers)
static void collectPrintParts(const Value& val, std::vector<const Value*>& parts) {
    if (val.type == ValueType::OPERATION && (val.operation == "+" || val.operator_ == "+")) {
        if (val.left) collectPrintParts(*val.left, parts);
        if (val.right) collectPrintParts(*val.right, parts);
    } else {
        parts.push_back(&val);
    }
}

void CodeGenerator::generatePrint(const Command& cmd) {
    // اطبع("النص")
    
    for (const auto& argPtr : cmd.arguments) {
        if (!argPtr) continue; // Null safety
        const Value& arg = *argPtr;
        if (arg.type == ValueType::STRING) {
            // ✅ Already using GetStdHandle/WriteFile logic which is good.
            // Keeping it but ensuring it's robust.
            std::string label = this->addStringLiteral(arg.string_value);
            size_t strLen = arg.string_value.length();
            
            this->emit(X64Encoder::MOV(Register::RCX, -11)); // STD_OUTPUT_HANDLE
            this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
            uint32_t gshOffset = this->getCurrentOffset() + 1;
            this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
            this->relocations.add(gshOffset, "GetStdHandle", RelocationType::REL32);
            this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
            
            this->emit(X64Encoder::MOV(Register::RCX, Register::RAX)); // handle
            uint32_t leaOffsetStr = this->getCurrentOffset() + 3;
            this->emit(Instruction(InstructionType::LEA, Operand::Reg(Register::RDX), Operand::Label(label)));
            this->relocations.add(leaOffsetStr, label, RelocationType::RIP_REL32);
            this->emit(X64Encoder::MOV(Register::R8, static_cast<int64_t>(strLen)));
            
            this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(64)));
            this->emit(Instruction(InstructionType::LEA, Operand::Reg(Register::R9), Operand::Mem(Register::RSP, 40)));
            this->emit(Instruction(InstructionType::MOV, Operand::Mem(Register::RSP, 32), Operand::Imm(0)));
            
            uint32_t wfOffset = this->getCurrentOffset() + 1;
            this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
            this->relocations.add(wfOffset, "WriteFile", RelocationType::REL32);
            this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(64)));
        }
        else if (arg.type == ValueType::OPERATION && (arg.operation == "+" || arg.operator_ == "+")) {
            // String concatenation: try constant fold first
            std::string folded = foldConstantStringConcat(arg);
            if (!folded.empty()) {
                std::string label = this->addStringLiteral(folded);
                size_t strLen = folded.length();
                this->emit(X64Encoder::MOV(Register::RCX, -11));
                this->emit(Instruction(InstructionType::SUB, Operand::Reg(Register::RSP), Operand::Imm(32)));
                uint32_t co1 = this->getCurrentOffset() + 1;
                this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
                this->relocations.add(co1, "GetStdHandle", RelocationType::REL32);
                this->emit(Instruction(InstructionType::ADD, Operand::Reg(Register::RSP), Operand::Imm(32)));
                this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));
                uint32_t leaOff = this->getCurrentOffset() + 3;
                this->emit(Instruction(InstructionType::LEA, Operand::Reg(Register::RDX), Operand::Label(label)));
                this->relocations.add(leaOff, label, RelocationType::RIP_REL32);
                this->emit(X64Encoder::MOV(Register::R8, static_cast<int64_t>(strLen)));
                this->emit(Instruction(InstructionType::SUB, Operand::Reg(Register::RSP), Operand::Imm(64)));
                this->emit(Instruction(InstructionType::LEA, Operand::Reg(Register::R9), Operand::Mem(Register::RSP, 40)));
                this->emit(Instruction(InstructionType::MOV, Operand::Mem(Register::RSP, 32), Operand::Imm(0)));
                uint32_t co2 = this->getCurrentOffset() + 1;
                this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
                this->relocations.add(co2, "WriteFile", RelocationType::REL32);
                this->emit(Instruction(InstructionType::ADD, Operand::Reg(Register::RSP), Operand::Imm(64)));
            } else {
                // Runtime concat: print each part separately (avoids ADD on string pointers)
                std::vector<const Value*> parts;
                collectPrintParts(arg, parts);
                for (const Value* p : parts) {
                    if (p && p->type == ValueType::STRING) {
                        std::string label = this->addStringLiteral(p->string_value);
                        size_t strLen = p->string_value.length();
                        this->emit(X64Encoder::MOV(Register::RCX, -11));
                        this->emit(Instruction(InstructionType::SUB, Operand::Reg(Register::RSP), Operand::Imm(32)));
                        uint32_t co1 = this->getCurrentOffset() + 1;
                        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
                        this->relocations.add(co1, "GetStdHandle", RelocationType::REL32);
                        this->emit(Instruction(InstructionType::ADD, Operand::Reg(Register::RSP), Operand::Imm(32)));
                        this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));
                        uint32_t leaOff = this->getCurrentOffset() + 3;
                        this->emit(Instruction(InstructionType::LEA, Operand::Reg(Register::RDX), Operand::Label(label)));
                        this->relocations.add(leaOff, label, RelocationType::RIP_REL32);
                        this->emit(X64Encoder::MOV(Register::R8, static_cast<int64_t>(strLen)));
                        this->emit(Instruction(InstructionType::SUB, Operand::Reg(Register::RSP), Operand::Imm(64)));
                        this->emit(Instruction(InstructionType::LEA, Operand::Reg(Register::R9), Operand::Mem(Register::RSP, 40)));
                        this->emit(Instruction(InstructionType::MOV, Operand::Mem(Register::RSP, 32), Operand::Imm(0)));
                        uint32_t co2 = this->getCurrentOffset() + 1;
                        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
                        this->relocations.add(co2, "WriteFile", RelocationType::REL32);
                        this->emit(Instruction(InstructionType::ADD, Operand::Reg(Register::RSP), Operand::Imm(64)));
                    } else if (p) {
                        this->generatePrintInteger(*p);
                    }
                }
            }
        }
        else {
            // NUMBER, VARIABLE, PROPERTY_ACCESS, etc.
            this->generatePrintInteger(arg);
        }
    }
}


// ════════════════════════════════════════════════════════════
// 🔢 Phase 4: Function Definition
// ════════════════════════════════════════════════════════════

void CodeGenerator::generateFunctionDefinition(const Command& cmd) {
    this->isInsideFunction = true;
    std::string funcName = cmd.variable;

    // 0. Jump over function body to avoid executing it when defined
    std::string skipLabel = "func_skip_" + funcName;
    uint32_t jmpOffset = this->getCurrentOffset();
    this->emit(X64Encoder::JMP(500)); 
    this->relocations.add(jmpOffset + 1, skipLabel, RelocationType::REL32);

    uint32_t funcOffset = this->getCurrentOffset();

    // ✅ إصلاح: تحديث الرمز إذا كان موجوداً مسبقاً، أو إضافته إذا لم يكن
    if (this->symbols.hasSymbol(funcName)) {
        Symbol& sym = this->symbols.getSymbol(funcName);
        sym.offset = funcOffset;
        sym.resolved = true;
    } else {
        this->symbols.addLabel(funcName, funcOffset);
    }
    this->functionTable[funcName] = funcOffset;
    
    // 1. Start Scope
    this->symbols.pushScope();

    // 2. Prologue
    // PUSH RBP
    this->emit(X64Encoder::PUSH(Register::RBP));
    // MOV RBP, RSP
    this->emit(X64Encoder::MOV(Register::RBP, Register::RSP));
    
    // 3. Calculate required stack space dynamically
    // Count parameters
    std::vector<std::string> paramNames;
    if (!cmd.parameters.empty()) {
        paramNames = cmd.parameters;
    } else {
        for (const auto& arg : cmd.arguments) {
            paramNames.push_back(arg->value);
        }
    }
    int paramCount = (int)paramNames.size();
    
    // Calculate stack space needed:
    // - 32 bytes shadow space (Windows x64 ABI)
    // - 8 bytes per parameter beyond the first 4 (RCX, RDX, R8, R9)
    // - 8 bytes per local variable (estimated from body commands)
    // - Align to 16 bytes
    int stackSpace = 32; // Shadow space
    if (paramCount > 4) {
        stackSpace += (paramCount - 4) * 8;
    }
    // Estimate local variables from body (rough estimate: 16 vars max)
    stackSpace += 16 * 8; // 128 bytes for local variables
    // Align to 16 bytes
    stackSpace = (stackSpace + 15) & ~15;
    
    // 4. Allocate Stack Space for Local Variables
    if (stackSpace > 0) {
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(stackSpace)));
    }
    
    // 5. Handle Parameters
    int paramIndex = 0;

    for (const auto& paramName : paramNames) {
        // Add parameter as variable in NEW scope
        if (!this->symbols.hasSymbol(paramName)) {
            this->symbols.addVariable(paramName, 8, SymbolDataType::INTEGER);
        }
        
        uint32_t offset = this->symbols.getSymbol(paramName).offset;
        
        Register srcReg;
        bool inRegister = true;
        
        switch (paramIndex) {
            case 0: srcReg = Register::RCX; break;
            case 1: srcReg = Register::RDX; break;
            case 2: srcReg = Register::R8;  break;
            case 3: srcReg = Register::R9;  break;
            default: inRegister = false; break;
        }
        
        if (inRegister) {
            // MOV [RBP-offset], srcReg
            this->emit(Instruction(InstructionType::MOV, 
                           Operand::Mem(Register::RBP, -static_cast<int32_t>(offset)),
                           Operand::Reg(srcReg)));
        } else {
            // Parameter is on stack (caller's frame)
            int32_t callerOffset = 48 + (paramIndex - 4) * 8;
            
            // MOV RAX, [RBP + callerOffset]
            this->emit(Instruction(InstructionType::MOV,
                           Operand::Reg(Register::RAX),
                           Operand::Mem(Register::RBP, callerOffset)));
                          
            // MOV [RBP - offset], RAX
            this->emit(Instruction(InstructionType::MOV,
                           Operand::Mem(Register::RBP, -static_cast<int32_t>(offset)),
                           Operand::Reg(Register::RAX)));
        }
        paramIndex++;
    }
    
    // 5. Body
    for (const auto& bodyCmd : cmd.body) {
        this->generateCommand(*bodyCmd);
    }
    
    // 6. End Scope
    this->symbols.popScope();

    // 7. Epilogue (Implicit return if not present)
    this->emit(X64Encoder::MOV(Register::RSP, Register::RBP));
    this->emit(X64Encoder::POP(Register::RBP));
    this->emit(X64Encoder::RET());
    
    // 8. Place skip label
    this->symbols.addLabel(skipLabel, this->getCurrentOffset());
    this->isInsideFunction = false;
}

void CodeGenerator::generateReturn(const Command& cmd) {
    
    if (cmd.expression) {
        
        this->generateExpression(*cmd.expression);
    } else if (cmd.value.type != ValueType::NONE) {
        this->generateExpression(cmd.value);
    }
    
    if (this->isInsideFunction) {
        // 2. Function Epilogue
        this->emit(X64Encoder::MOV(Register::RSP, Register::RBP));
        this->emit(X64Encoder::POP(Register::RBP));
        
        // 3. RET
        this->emit(X64Encoder::RET());
    } else {
        // 2. Top-level Exit (Jump to global_exit)
        uint32_t jmpOffset = this->getCurrentOffset();
        this->emit(X64Encoder::JMP(500));
        this->relocations.add(jmpOffset + 1, "global_exit", RelocationType::REL32);
    }
}

void CodeGenerator::generateFunctionCall(const Command& cmd) {
    // 
    // 1. Evaluate Arguments in REVERSE order (Microsoft x64 Calling Convention)
    for (auto it = cmd.arguments.rbegin(); it != cmd.arguments.rend(); ++it) {
        if (*it) {
            this->generateExpression(**it);
            this->emit(X64Encoder::PUSH(Register::RAX));
        } else {
            std::cerr << "Warning: Null argument in function call: " << cmd.variable << std::endl;
            this->emit(X64Encoder::MOV(Register::RAX, Operand::Imm(0)));
            this->emit(X64Encoder::PUSH(Register::RAX));
        }
    }
    
    // 2. Pop first 4 arguments into RCX, RDX, R8, R9
    int argCount = static_cast<int>(cmd.arguments.size());
    
    for (int i = 0; i < argCount; ++i) {
        if (i < 4) {
            Register targetReg;
            switch (i) {
                case 0: targetReg = Register::RCX; break;
                case 1: targetReg = Register::RDX; break;
                case 2: targetReg = Register::R8;  break;
                case 3: targetReg = Register::R9;  break;
            }
            this->emit(X64Encoder::POP(targetReg));
        } else {
            // ✅ FIX: Do NOT pop stack arguments (>4)!
            // They must remain on the stack for the callee.
            // Since we pushed in reverse order, Arg 5 is at the top, Arg 6 below it, etc.
            break; 
        }
    }
    
    // 3. Allocate Shadow Space (32 bytes)
    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
    
    // 4. CALL
    std::string funcName = cmd.variable; 
    
    // ✅ Fix for "Undefined symbol" error:
    // If cmd.variable is empty (which happens for standalone function calls parsed by ArabicParser),
    // try to get the name from function_name or value
    if (funcName.empty()) {
        if (!cmd.function_name.empty()) {
            funcName = cmd.function_name;
        } else if (cmd.value.type == ValueType::FUNCTION_CALL && !cmd.value.value.empty()) {
             funcName = cmd.value.value;
        }
    }
    
    // ✅ Handle builtin functions
    if (funcName == "طول") {
        funcName = "strlen";  // Map طول to strlen
    }
    else if (funcName == "جزء") {
        // ✅ Special handling for جزء(نص، من، طول)
        // RCX=string, RDX=start, R8=length (already set up by the arg popping loop above)
        
        // Save args to non-volatile registers
        this->emit(X64Encoder::PUSH(Register::RBX));  // Save RBX
        this->emit(X64Encoder::PUSH(Register::RSI));  // Save RSI
        this->emit(X64Encoder::PUSH(Register::RDI));  // Save RDI
        
        this->emit(X64Encoder::MOV(Register::RBX, Register::RCX)); // RBX = string
        this->emit(X64Encoder::MOV(Register::RSI, Register::RDX)); // RSI = start
        this->emit(X64Encoder::MOV(Register::RDI, Register::R8));  // RDI = length
        
        // 1. Call HeapAlloc(GetProcessHeap(), 8, length + 1)
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        uint32_t gphOffsetSub = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(gphOffsetSub, "GetProcessHeap", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));

        this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));
        this->emit(X64Encoder::MOV(Register::RDX, 8)); // HEAP_ZERO_MEMORY
        this->emit(X64Encoder::MOV(Register::R8, Register::RDI)); // size = length
        this->emit(X64Encoder::ADD(Register::R8, Operand::Imm(1))); // +1 for null terminator

        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        uint32_t haOffsetSub = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(haOffsetSub, "HeapAlloc", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
        
        // RAX = allocated buffer
        this->emit(X64Encoder::PUSH(Register::RAX)); // Save buffer pointer
        
        // 2. Call memcpy(dest=RAX, src=string+start, count=length)
        this->emit(X64Encoder::MOV(Register::RCX, Register::RAX)); // dest = buffer
        this->emit(X64Encoder::MOV(Register::RDX, Register::RBX)); // src = string
        this->emit(X64Encoder::ADD(Register::RDX, Register::RSI)); // src += start
        this->emit(X64Encoder::MOV(Register::R8, Register::RDI));  // count = length
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        
        uint32_t memcpyOffset = this->getCurrentOffset();
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(memcpyOffset + 1, "memcpy", RelocationType::REL32);
        
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
        
        // 3. Null terminate: buffer[length] = 0
        this->emit(X64Encoder::POP(Register::RAX)); // Restore buffer pointer
        // MOV BYTE [RAX + RDI], 0
        this->code.push_back(0xC6);
        this->code.push_back(0x04);
        this->code.push_back(0x38);
        this->code.push_back(0x00);
        
        // Restore saved registers
        this->emit(X64Encoder::POP(Register::RDI));
        this->emit(X64Encoder::POP(Register::RSI));
        this->emit(X64Encoder::POP(Register::RBX));
        
        // Result in RAX = pointer to substring
        return;
    }
    else if (funcName == "ملف_موجود") {
        // موجود(مسار) -> _access(path, 0) != -1
        // We will call _access.
        funcName = "_access";
        // _access takes (path, mode). We have path. Mode check existence = 0.
        // We need to push 0 as second argument.
        // Arguments are popped in generic loop. RCX has path.
        // Move RCX to RCX (path).
        // Set RDX = 0.
         this->emit(X64Encoder::MOV(Register::RDX, 0));
    }
    else if (funcName == "احذف_ملف") {
        funcName = "remove";
    }
    else if (funcName == "انقل_ملف") {
        funcName = "rename";
    }
    else if (funcName == "الوقت_الحالي") {
        funcName = "time";
        // time(NULL)
        this->emit(X64Encoder::MOV(Register::RCX, 0));
    }
    else if (funcName == "انتظر") {
        funcName = "Sleep"; // Windows API Sleep(ms)
    }
    else if (funcName == "احصل_بيئة") {
        funcName = "getenv";
    }
    else if (funcName == "نفذ_أمر") {
        funcName = "system";
    }
    else if (funcName == "مقارنة_جزء") {
        // مقارنة_جزء(نص، الفهرس، بحث) -> return 1 if match, 0 if not
        // This is complex. strncmp(text+index, search, strlen(search)) == 0
        // We can't easily implement this rewrite here without complex logic.
        // For now, let's map to a custom helper "check_substring" if we had one in runtime.
        // Or implement inline.
        
        // Args: RCX=text, RDX=index, R8=search
        
        this->emit(X64Encoder::PUSH(Register::RBX));
        this->emit(X64Encoder::PUSH(Register::RSI));
        this->emit(X64Encoder::PUSH(Register::RDI));
        
        // Save args
        this->emit(X64Encoder::MOV(Register::RBX, Register::RCX)); // Text
        this->emit(X64Encoder::MOV(Register::RSI, Register::RDX)); // Index
        this->emit(X64Encoder::MOV(Register::RDI, Register::R8));  // Search
        
        // Calculate length of 'search'
        this->emit(X64Encoder::MOV(Register::RCX, Register::RDI));
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32))); 
        uint32_t strlenOffset = this->getCurrentOffset();
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(strlenOffset + 1, "strlen", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
        
        // RAX = length of search. Move to R8 (limit for strncmp)
        this->emit(X64Encoder::MOV(Register::R8, Register::RAX));
        
        // Prepare strncmp(text+index, search, length)
        // RCX = text + index
        this->emit(X64Encoder::MOV(Register::RCX, Register::RBX));
        this->emit(X64Encoder::ADD(Register::RCX, Register::RSI));
        
        // RDX = search
        this->emit(X64Encoder::MOV(Register::RDX, Register::RDI));
        
        // Call strncmp
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32))); 
        uint32_t strncmpOffset = this->getCurrentOffset();
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(strncmpOffset + 1, "strncmp", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
        
        // RAX == 0 means match. We want 1 if match, 0 otherwise.
        
        // CMP RAX, 0
        this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RAX), Operand::Imm(0)));
        
        // SETE AL (Set byte if equal to 0)
        this->code.push_back(0x0F); this->code.push_back(0x94); this->code.push_back(0xC0); // SETE AL
        
        // MOVZX RAX, AL
        this->code.push_back(0x48); this->code.push_back(0x0F); this->code.push_back(0xB6); this->code.push_back(0xC0);
        
        this->emit(X64Encoder::POP(Register::RDI));
        this->emit(X64Encoder::POP(Register::RSI));
        this->emit(X64Encoder::POP(Register::RBX));
        
        return; // Done
    }
    
    else if (funcName == "طول_حقيقي") {
        funcName = "strlen";
    }
    else if (funcName == "استخرج_حرف") {
        // استخرج_حرف(نص، فهرس) -> String(1 char)
        // RCX: Text, RDX: Index
        
        // Save non-volatile registers
        this->emit(X64Encoder::PUSH(Register::RBX));
        
        // 1. Save Text and Index
        this->emit(X64Encoder::PUSH(Register::RCX));
        this->emit(X64Encoder::PUSH(Register::RDX));
        
        // 2. Allocate 2 bytes using HeapAlloc
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        uint32_t gphOffsetChar = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(gphOffsetChar, "GetProcessHeap", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));

        this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));
        this->emit(X64Encoder::MOV(Register::RDX, 8));
        this->emit(X64Encoder::MOV(Register::R8, 2));

        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        uint32_t haOffsetChar = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(haOffsetChar, "HeapAlloc", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
        
        // RAX has new buffer. Move to RBX to save.
        this->emit(X64Encoder::MOV(Register::RBX, Register::RAX));
        
        // 3. Restore Index (RDX) and Text (RCX)
        this->emit(X64Encoder::POP(Register::RDX));
        this->emit(X64Encoder::POP(Register::RCX));
        
        // 4. Get char at Text[Index]
        this->emit(X64Encoder::ADD(Register::RCX, Register::RDX)); // RCX = &Text[Index]
        this->emit(X64Encoder::MOV(Register::AL, Operand::Mem(Register::RCX, 0))); // AL = Char
        
        // 5. Store in new buffer
        this->emit(X64Encoder::MOV(Operand::Mem(Register::RBX, 0), Register::AL));
        this->emit(X64Encoder::MOV(Operand::Mem(Register::RBX, 1), Operand::Imm(0))); // Null terminate
        
        // 6. Return buffer (RBX -> RAX)
        this->emit(X64Encoder::MOV(Register::RAX, Register::RBX));
        
        // Restore non-volatile registers
        this->emit(X64Encoder::POP(Register::RBX));
        return;
    }
    else if (funcName == "تحويل_أعلى") {
        // تحويل_أعلى(نص) -> String(1 char upper)
        // RCX: Text
        
        // Save non-volatile registers
        this->emit(X64Encoder::PUSH(Register::RBX));
        
        // 1. Get first char
        this->emit(X64Encoder::MOV(Register::AL, Operand::Mem(Register::RCX, 0)));
        this->emit(X64Encoder::MOVZX(Register::RCX, Register::AL)); // RCX = char (int)
        
        // 2. Call toupper
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32))); 
        uint32_t toupperOffset = this->getCurrentOffset();
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(toupperOffset + 1, "toupper", RelocationType::REL32); 
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
        // RAX has upper char (int)
        
        this->emit(X64Encoder::PUSH(Register::RAX)); // Save result
        
        // 3. Allocate 2 bytes using HeapAlloc
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        uint32_t gphOffsetUp = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(gphOffsetUp, "GetProcessHeap", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));

        this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));
        this->emit(X64Encoder::MOV(Register::RDX, 8));
        this->emit(X64Encoder::MOV(Register::R8, 2));

        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        uint32_t haOffsetUp = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(haOffsetUp, "HeapAlloc", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
        
        // RAX has buffer.
        this->emit(X64Encoder::MOV(Register::RBX, Register::RAX));
        this->emit(X64Encoder::POP(Register::RAX)); // Restore upper char
        
        // 4. Store
        this->emit(X64Encoder::MOV(Operand::Mem(Register::RBX, 0), Register::AL));
        this->emit(X64Encoder::MOV(Operand::Mem(Register::RBX, 1), Operand::Imm(0)));
        
        // 5. Return
        this->emit(X64Encoder::MOV(Register::RAX, Register::RBX));
        
        // Restore non-volatile registers
        this->emit(X64Encoder::POP(Register::RBX));
        return;
    }
    else if (funcName == "تحويل_أسفل") {
        // تحويل_أسفل(نص) -> String(1 char lower)
        // RCX: Text
        
        // Save non-volatile registers
        this->emit(X64Encoder::PUSH(Register::RBX));
        
        this->emit(X64Encoder::MOV(Register::AL, Operand::Mem(Register::RCX, 0)));
        this->emit(X64Encoder::MOVZX(Register::RCX, Register::AL)); 
        
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32))); 
        uint32_t tolowerOffset = this->getCurrentOffset();
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(tolowerOffset + 1, "tolower", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
        
        this->emit(X64Encoder::PUSH(Register::RAX));
        
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        uint32_t gphOffsetDown = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(gphOffsetDown, "GetProcessHeap", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));

        this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));
        this->emit(X64Encoder::MOV(Register::RDX, 8));
        this->emit(X64Encoder::MOV(Register::R8, 2));

        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        uint32_t haOffsetDown = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(haOffsetDown, "HeapAlloc", RelocationType::REL32);
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
        
        this->emit(X64Encoder::MOV(Register::RBX, Register::RAX));
        this->emit(X64Encoder::POP(Register::RAX));
        
        this->emit(X64Encoder::MOV(Operand::Mem(Register::RBX, 0), Register::AL));
        this->emit(X64Encoder::MOV(Operand::Mem(Register::RBX, 1), Operand::Imm(0)));
        
        this->emit(X64Encoder::MOV(Register::RAX, Register::RBX));
        
        // Restore non-volatile registers
        this->emit(X64Encoder::POP(Register::RBX));
        return;
    }
    
    if (funcName.empty()) {

        std::cerr << "Error: Function name is empty in generateFunctionCall!" << std::endl;
        funcName = "UNKNOWN_FUNCTION"; 
    }

    uint32_t callOffset = this->getCurrentOffset();
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(callOffset + 1, funcName, RelocationType::REL32);

    // Ensure the function symbol exists in the symbol table
    // Note: Functions are pre-registered in parser, so they should exist
    // If not found, add as import (for external functions)
    if (!this->symbols.hasSymbol(funcName)) {
        // Add as import - will be resolved later
        this->symbols.addImport(funcName, 0); // RVA will be set during linking
    }
    
    // 5. Deallocate Shadow Space + Stack Arguments
    int extraArgs = std::max(0, argCount - 4);
    int totalCleanup = 32 + (extraArgs * 8);
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(totalCleanup)));
}



// ════════════════════════════════════════════════════════════


// ════════════════════════════════════════════════════════════
// 🔢 Phase 5: Integer Printing
// ════════════════════════════════════════════════════════════

void CodeGenerator::generatePrintInteger(const Value& val) {
    // Print an integer value (from NUMBER or VARIABLE)
    
    // Save non-volatile registers
    this->emit(X64Encoder::PUSH(Register::RBX));
    this->emit(X64Encoder::PUSH(Register::RDI));
    this->emit(X64Encoder::PUSH(Register::RSI));
    this->emit(X64Encoder::PUSH(Register::R12)); // Will use to save buffer start
    
    // 1. Evaluate the value into RAX
    this->generateExpression(val);
    
    // 2. Convert integer in RAX to string
    // Allocate buffer on stack (32 bytes)
    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
    
    // R12 = buffer start (SAVE IT!)
    this->emit(X64Encoder::MOV(Register::R12, Register::RSP));
    
    // RDI = current write position (starts at buffer start)
    this->emit(X64Encoder::MOV(Register::RDI, Register::R12));
    
    // RBX = digit counter
    this->emit(X64Encoder::MOV(Register::RBX, 0));
    
    // Check for negative
    this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RAX), Operand::Imm(0)));
    
    std::string positiveLabel = this->symbols.getNewLabel("positive");
    std::string convertLoopLabel = this->symbols.getNewLabel("convert_loop");
    std::string reverseLoopLabel = this->symbols.getNewLabel("reverse_loop");
    std::string reverseDoneLabel = this->symbols.getNewLabel("reverse_done");
    
    uint32_t jgeOffset = this->getCurrentOffset();
    this->emit(Instruction(InstructionType::JGE, Operand::Imm(0)));
    this->relocations.add(jgeOffset + 2, positiveLabel, RelocationType::REL32);
    
    // Negative: negate and store '-'
    this->emit(Instruction(InstructionType::NEG, Operand::Reg(Register::RAX)));
    this->emit(Instruction(InstructionType::MOV, Operand::Mem(Register::RDI, 0), Operand::Imm('-')));
    this->emit(X64Encoder::ADD(Register::RDI, Operand::Imm(1)));
    
    // Positive label
    this->symbols.addLabel(positiveLabel, this->getCurrentOffset());
    
    // RCX = 10 (divisor)
    this->emit(X64Encoder::MOV(Register::RCX, 10));
    
    // Convert loop: extract digits by dividing by 10
    this->symbols.addLabel(convertLoopLabel, this->getCurrentOffset());
    
    // RDX = 0 (clear for division)
    this->emit(X64Encoder::MOV(Register::RDX, 0));
    
    // IDIV RCX: RAX = quotient, RDX = remainder
    this->emit(X64Encoder::IDIV(Register::RCX));
    
    // Convert remainder to ASCII: RDX + '0'
    this->emit(X64Encoder::ADD(Register::RDX, Operand::Imm('0')));
    
    // Push digit to stack (we'll reverse later)
    this->emit(X64Encoder::PUSH(Register::RDX));
    
    // Increment counter
    this->emit(X64Encoder::ADD(Register::RBX, Operand::Imm(1)));
    
    // Check if quotient is 0
    this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RAX), Operand::Imm(0)));
    
    uint32_t jneOffset = this->getCurrentOffset();
    this->emit(Instruction(InstructionType::JNE, Operand::Imm(0)));
    this->relocations.add(jneOffset + 2, convertLoopLabel, RelocationType::REL32);
    
    // Now RBX = number of digits, stack has digits (reversed)
    // RSI = current position in buffer (RDI points after last char or after '-')
    this->emit(X64Encoder::MOV(Register::RSI, Register::RDI));
    
    // Reverse loop: pop digits and store
    this->symbols.addLabel(reverseLoopLabel, this->getCurrentOffset());
    
    this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::RBX), Operand::Imm(0)));
    
    uint32_t jleOffset = this->getCurrentOffset();
    this->emit(Instruction(InstructionType::JLE, Operand::Imm(0)));
    this->relocations.add(jleOffset + 2, reverseDoneLabel, RelocationType::REL32);
    
    // Pop digit
    this->emit(X64Encoder::POP(Register::RAX));
    
    // Store to buffer
    this->emit(Instruction(InstructionType::MOV, Operand::Mem(Register::RSI, 0), Operand::Reg(Register::RAX)));
    
    // Advance buffer pointer
    this->emit(X64Encoder::ADD(Register::RSI, Operand::Imm(1)));
    
    // Decrement counter
    this->emit(X64Encoder::SUB(Register::RBX, Operand::Imm(1)));
    
    uint32_t jmpRevLoopOffset = this->getCurrentOffset();
    this->emit(X64Encoder::JMP(500));
    this->relocations.add(jmpRevLoopOffset + 1, reverseLoopLabel, RelocationType::REL32);
    
    this->symbols.addLabel(reverseDoneLabel, this->getCurrentOffset());
    
    // Now buffer contains the string, RSI points past last character
    // Length = RSI - R12 (R12 still holds buffer start!)
    this->emit(X64Encoder::MOV(Register::R8, Register::RSI));
    this->emit(X64Encoder::SUB(Register::R8, Register::R12));
    
    // Save Length (R8) since GetStdHandle will corrupt it
    this->emit(X64Encoder::PUSH(Register::R8));
    
    // 3. Call GetStdHandle
    this->emit(X64Encoder::MOV(Register::RCX, -11));
    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
    
    uint32_t getStdHandleOffset = this->getCurrentOffset() + 1;
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(getStdHandleOffset, "GetStdHandle", RelocationType::REL32);
    
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
    
    // Restore Length (R8)
    this->emit(X64Encoder::POP(Register::R8));
    
    // 4. Call WriteFile
    // RCX = handle (in RAX)
    this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));
    
    // RDX = buffer pointer (R12 holds buffer start)
    this->emit(X64Encoder::MOV(Register::RDX, Register::R12));
    
    // R8 = length (already in R8)
    // R8 = length (already in R8)
    
    // Allocate 64 bytes for stack args + shadow space + temp vars
    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(64)));
    
    // R9 = &bytesWritten (at RSP + 40)
    this->emit(Instruction(InstructionType::LEA, Operand::Reg(Register::R9), Operand::Mem(Register::RSP, 40)));
    
    // 5th param: lpOverlapped = NULL (at RSP+32)
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Mem(Register::RSP, 32),
                   Operand::Imm(0)));
    
    // Call WriteFile
    uint32_t callOffset2 = this->getCurrentOffset() + 1;
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(callOffset2, "WriteFile", RelocationType::REL32);
    
    // Cleanup: 64 bytes
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(64)));
    
    // Cleanup buffer (32 bytes)
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
    
    // Restore non-volatile registers
    this->emit(X64Encoder::POP(Register::R12));
    this->emit(X64Encoder::POP(Register::RSI));
    this->emit(X64Encoder::POP(Register::RDI));
    this->emit(X64Encoder::POP(Register::RBX));
}

void CodeGenerator::generateFileOpen(const Command& cmd) {
    // متغير = افتح_ملف("اسم_الملف", "الوضع")
    // Use Win32 CreateFileA instead of fopen
    
    if (cmd.arguments.size() < 1) {
        throw std::runtime_error("FILE_OPEN requires at least filename");
    }
    
    const Value& filenameVal = *cmd.arguments[0];
    std::string filenameLabel = this->addStringLiteral(filenameVal.string_value);
    
    // CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
    //             dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile)
    
    // 1. lpFileName (RCX)
    uint32_t leaOffset = this->getCurrentOffset() + 3;
    this->emit(Instruction(InstructionType::LEA, Operand::Reg(Register::RCX), Operand::Label(filenameLabel)));
    this->relocations.add(leaOffset, filenameLabel, RelocationType::RIP_REL32);
    
    // 2. dwDesiredAccess (RDX) - Generic Read (0x80000000) or Write (0x40000000)
    // For now, let's assume Generic Read/Write
    this->emit(X64Encoder::MOV(Register::RDX, 0xC0000000));

    // 3. dwShareMode (R8) - Share Read (1)
    this->emit(X64Encoder::MOV(Register::R8, 1));
    
    // 4. lpSecurityAttributes (R9) - NULL (0)
    this->emit(X64Encoder::MOV(Register::R9, 0));

    // Stack arguments (5, 6, 7)
    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(64))); // 32 shadow + 24 args + 8 alignment

    // 5. dwCreationDisposition - OPEN_ALWAYS (4)
    this->emit(Instruction(InstructionType::MOV, Operand::Mem(Register::RSP, 32), Operand::Imm(4)));

    // 6. dwFlagsAndAttributes - FILE_ATTRIBUTE_NORMAL (128)
    this->emit(Instruction(InstructionType::MOV, Operand::Mem(Register::RSP, 40), Operand::Imm(128)));

    // 7. hTemplateFile - NULL (0)
    this->emit(Instruction(InstructionType::MOV, Operand::Mem(Register::RSP, 48), Operand::Imm(0)));

    // Call CreateFileA
    uint32_t callOffset = this->getCurrentOffset() + 1;
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(callOffset, "CreateFileA", RelocationType::REL32);
    this->symbols.addImport("CreateFileA", 40);

    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(64)));
    
    // 5. Store result (FILE* in RAX) to variable
    std::string varName = cmd.variable;
    if (!this->symbols.hasSymbol(varName)) {
        this->symbols.addVariable(varName, 8, SymbolDataType::FILE_HANDLE);  // FILE* is 8 bytes
    }
    
    uint32_t stackOffset = this->symbols.getSymbol(varName).offset;
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset)),
                   Operand::Reg(Register::RAX)));
}

void CodeGenerator::generateFileRead(const Command& cmd) {
    // سطر = اقرأ_سطر(ملف)  OR  محتوى = اقرأ_كل(ملف)
    // For simplicity, we'll implement fgets for single line
    // For read_all, we can read in a loop or use fread
    
    if (cmd.arguments.size() < 1) {
        throw std::runtime_error("FILE_READ requires file handle argument");
    }
    
    const Value& fileHandleVal = *cmd.arguments[0];
    
    // 1. Load file handle into RCX
    if (fileHandleVal.type == ValueType::VARIABLE) {
        std::string varName = fileHandleVal.value;
        if (!this->symbols.hasSymbol(varName)) {
            throw std::runtime_error("Undefined file handle: " + varName);
        }
        uint32_t stackOffset = this->symbols.getSymbol(varName).offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RCX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    }
    
    // 2. Allocate buffer on stack (4096 bytes for read)
    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(4096)));
    
    // 3. For read_all (fread) - read entire file
    if (cmd.type == CommandType::FILE_READ_ALL) {
        // fread(buffer, 1, 4096, file)
        // RCX = buffer (RSP)
        this->emit(X64Encoder::MOV(Register::RCX, Register::RSP));
        
        // RDX = size (1)
        this->emit(X64Encoder::MOV(Register::RDX, 1));
        
        // R8 = count (4096)
        this->emit(X64Encoder::MOV(Register::R8, 4096));
        
        // R9 = file handle (already loaded, push it)
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(48))); // Shadow + align
        
        // Move file handle from RCX to R9 (but we need to reload it)
        // Actually, fread signature is: fread(ptr, size, count, file)
        // Let's use RCX=ptr, RDX=size, R8=count, R9=file
        // We need to push file handle to R9
        if (fileHandleVal.type == ValueType::VARIABLE) {
            std::string varName = fileHandleVal.value;
            uint32_t stackOffset = this->symbols.getSymbol(varName).offset;
            this->emit(Instruction(InstructionType::MOV,
                           Operand::Reg(Register::R9),
                           Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
        }
        
        uint32_t callOffset = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(callOffset, "fread", RelocationType::REL32);
        
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(48)));
    }
    else {
        // fgets(buffer, 4096, file) - read one line
        // RCX = buffer
        this->emit(X64Encoder::MOV(Register::RCX, Register::RSP));
        
        // RDX = size
        this->emit(X64Encoder::MOV(Register::RDX, 4096));
        
        // R8 = file
        if (fileHandleVal.type == ValueType::VARIABLE) {
            std::string varName = fileHandleVal.value;
            uint32_t stackOffset = this->symbols.getSymbol(varName).offset;
            this->emit(Instruction(InstructionType::MOV,
                           Operand::Reg(Register::R8),
                           Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
        }
        
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        
        uint32_t callOffset = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(callOffset, "fgets", RelocationType::REL32);
        
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
    }
    
    // 4. Store buffer pointer to variable (result string in stack buffer)
    // For now, we'll store the stack pointer (not ideal, but works for demo)
    std::string varName = cmd.variable;
    if (!this->symbols.hasSymbol(varName)) {
        this->symbols.addVariable(varName, 8, SymbolDataType::STRING);
    }
    
    // Store pointer to buffer
    uint32_t stackOffset = this->symbols.getSymbol(varName).offset;
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset)),
                   Operand::Reg(Register::RSP)));
    
    // Cleanup: 4096 bytes
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(4096)));
}

void CodeGenerator::generateFileWrite(const Command& cmd) {
    // اكتب(ملف, "نص")
    
    if (cmd.arguments.size() < 2) {
        throw std::runtime_error("FILE_WRITE requires 2 arguments");
    }
    
    const Value& fileHandleVal = *cmd.arguments[0];
    const Value& dataVal = *cmd.arguments[1];
    
    // 1. Load file handle into R9 (4th param for fprintf)
    if (fileHandleVal.type == ValueType::VARIABLE) {
        std::string varName = fileHandleVal.value;
        if (!this->symbols.hasSymbol(varName)) {
            throw std::runtime_error("Undefined file handle: " + varName);
        }
        uint32_t stackOffset = this->symbols.getSymbol(varName).offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::R9),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    }
    
    // 2. Add data string to data section
    std::string dataLabel;
    if (dataVal.type == ValueType::STRING) {
        dataLabel = this->addStringLiteral(dataVal.string_value);
    }
    
    // 3. Prepare fprintf(file, "%s", data)
    // Actually fprintf signature: fprintf(FILE*, format, ...)
    // RCX = file
    this->emit(X64Encoder::MOV(Register::RCX, Register::R9));
    
    // RDX = format string "%s"
    std::string formatLabel = this->addStringLiteral("%s");
    uint32_t leaFormatOffset = this->getCurrentOffset() + 3;
    this->emit(Instruction(InstructionType::LEA,
                   Operand::Reg(Register::RDX),
                   Operand::Label(formatLabel)));
    this->relocations.add(leaFormatOffset, formatLabel, RelocationType::RIP_REL32);
    
    // R8 = data string
    if (dataVal.type == ValueType::STRING) {
        uint32_t leaDataOffset = this->getCurrentOffset() + 3;
        this->emit(Instruction(InstructionType::LEA,
                       Operand::Reg(Register::R8),
                       Operand::Label(dataLabel)));
        this->relocations.add(leaDataOffset, dataLabel, RelocationType::RIP_REL32);
    }
    
    // 4. Call fprintf
    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
    
    uint32_t callOffset = this->getCurrentOffset() + 1;
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(callOffset, "fprintf", RelocationType::REL32);
    
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
}

void CodeGenerator::generateFileClose(const Command& cmd) {
    // أغلق(ملف) - Use CloseHandle instead of fclose
    
    if (cmd.arguments.size() < 1) {
        throw std::runtime_error("FILE_CLOSE requires file handle argument");
    }
    
    const Value& fileHandleVal = *cmd.arguments[0];
    
    // 1. Load file handle into RCX
    if (fileHandleVal.type == ValueType::VARIABLE) {
        std::string varName = fileHandleVal.value;
        if (!this->symbols.hasSymbol(varName)) {
            throw std::runtime_error("Undefined file handle: " + varName);
        }
        uint32_t stackOffset = this->symbols.getSymbol(varName).offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RCX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    }
    
    // 2. Call CloseHandle
    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
    
    uint32_t callOffset = this->getCurrentOffset() + 1;
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(callOffset, "CloseHandle", RelocationType::REL32);
    this->symbols.addImport("CloseHandle", 41);
    
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
}

// ════════════════════════════════════════════════════════════
// 📊 Array Operations  
// ════════════════════════════════════════════════════════════

void CodeGenerator::generateArrayDeclaration(const Command& cmd) {
    // مصفوفة أعداد[5] = [1, 2, 3, 4, 5]
    // مصفوفة فارغة[10]

    std::string arrayName = cmd.variable;
    size_t arraySize = 8; // Default capacity

    // Get array size from command if specified (e.g., [10] as a placeholder for capacity)
    if (cmd.value.type == ValueType::NUMBER) {
        arraySize = std::stoi(cmd.value.string_value);
    } else if (cmd.value.type == ValueType::ARRAY && !cmd.value.elements.empty()) {
        arraySize = std::max((size_t)8, cmd.value.elements.size());
    }

    // Calculate total memory: (Capacity * 8) + 16 bytes header
    // Header layout: [Capacity (8 bytes)][Count (8 bytes)]
    size_t totalBytes = (arraySize * 8) + 16;

    // 1. Allocate memory using Win32 HeapAlloc (instead of malloc)
    // First, get default process heap
    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32))); // Shadow space
    uint32_t gphOffset = this->getCurrentOffset() + 1;
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(gphOffset, "GetProcessHeap", RelocationType::REL32);
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));

    // RAX now has hHeap. Prepare HeapAlloc(hHeap, dwFlags, dwBytes)
    this->emit(X64Encoder::MOV(Register::RCX, Register::RAX)); // hHeap
    this->emit(X64Encoder::MOV(Register::RDX, 8));            // dwFlags = HEAP_ZERO_MEMORY (0x8)
    this->emit(X64Encoder::MOV(Register::R8, static_cast<int64_t>(totalBytes))); // dwBytes

    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32))); // Shadow space
    uint32_t haOffset = this->getCurrentOffset() + 1;
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(haOffset, "HeapAlloc", RelocationType::REL32);
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));

    // 2. Initialize Header
    // RAX has the base pointer
    // Capacity at offset 0
    this->emit(X64Encoder::MOV(Operand::Mem(Register::RAX, 0), static_cast<int64_t>(arraySize)));
    // Count at offset 8
    size_t initialCount = (cmd.value.type == ValueType::ARRAY) ? cmd.value.elements.size() : 0;
    this->emit(X64Encoder::MOV(Operand::Mem(Register::RAX, 8), static_cast<int64_t>(initialCount)));

    // 3. Store pointer in local variable
    this->symbols.addVariable(arrayName, 8, SymbolDataType::ARRAY);
    uint32_t stackOffset = this->symbols.getSymbol(arrayName).offset;

    this->emit(Instruction(InstructionType::MOV,
                   Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset)),
                   Operand::Reg(Register::RAX)));

    // 3. Initialize array elements if provided
    if (cmd.value.type == ValueType::ARRAY && !cmd.value.elements.empty()) {
        // Save array pointer in RBX
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RBX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));

        for (size_t i = 0; i < cmd.value.elements.size(); ++i) {
            // Generate value expression -> RAX
            this->generateExpression(cmd.value.elements[i]);

            // Push value to preserve it
            this->emit(X64Encoder::PUSH(Register::RAX));

            // Reload array pointer from stack (in case generateExpression modified RBX)
            this->emit(Instruction(InstructionType::MOV,
                           Operand::Reg(Register::RBX),
                           Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));

            // Pop value back to RAX
            this->emit(X64Encoder::POP(Register::RAX));

            // Store value at array[i]: [RBX + 16 + i*8] = RAX
            this->emit(Instruction(InstructionType::MOV,
                           Operand::Mem(Register::RBX, static_cast<int32_t>(16 + i * 8)),
                           Operand::Reg(Register::RAX)));
        }
    }
}

void CodeGenerator::generateArrayAccess(const Value& val) {
    // arr[index] - result will be in RAX
    // val.left = array variable
    // val.right = index expression

    if (!val.left || !val.right) {
        throw std::runtime_error("Invalid array access");
    }

    std::string arrayName;
    const Symbol* symPtr = nullptr;
    bool isNested = false;

    // Support nested access like arr[i][j] or obj.prop[i]
    if (val.left->type == ValueType::ARRAY_ACCESS || val.left->type == ValueType::PROPERTY_ACCESS || val.left->type == ValueType::FUNCTION_CALL) {
        this->generateExpression(*val.left);
        this->emit(X64Encoder::MOV(Register::RBX, Register::RAX)); // RBX = Base Pointer
        isNested = true;
    } else {
        arrayName = val.left->value;
        if (arrayName.empty()) {
             throw std::runtime_error("Array name is empty in access");
        }
        
        if (!this->symbols.hasSymbol(arrayName)) {
            throw std::runtime_error("Undefined array: " + arrayName);
        }

        symPtr = &this->symbols.getSymbol(arrayName);
        uint32_t stackOffset = symPtr->offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RBX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    }

    // 1. Evaluate index -> RAX
    this->generateExpression(*val.right);

    // 2. Save index in RCX
    this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));

    // 3. Load array pointer to RBX (only if not nested)
    if (!isNested) {
        if (!this->symbols.hasSymbol(arrayName)) {
            throw std::runtime_error("Undefined array: " + arrayName);
        }

        const Symbol& sym = this->symbols.getSymbol(arrayName);
        uint32_t stackOffset = sym.offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RBX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
        symPtr = &sym;
    }

    // ✅ Check if this is a string variable (byte-level access)
    bool isString = false;
    if (symPtr) {
        isString = (symPtr->dataType == SymbolDataType::STRING);
    }

    if (isString) {
        // String: index * 1 (no multiplication needed), read 1 byte
        // 5. Load byte from [RBX + RCX] -> RAX using MOVZX
        this->emit(X64Encoder::ADD(Register::RBX, Register::RCX)); // RBX = Base + Index
        // MOVZX RAX, BYTE [RBX] - zero-extend byte to 64-bit
        // Encoded as: 48 0F B6 03 (REX.W movzx rax, byte ptr [rbx])
        this->code.push_back(0x48);  // REX.W
        this->code.push_back(0x0F);  // Two-byte opcode prefix
        this->code.push_back(0xB6);  // MOVZX r, r/m8
        this->code.push_back(0x03);  // ModR/M: RAX, [RBX]
    } else {
        // Array: index * 8 for 64-bit elements
        this->emit(X64Encoder::MOV(Register::RDX, static_cast<int64_t>(8)));
        this->emit(X64Encoder::IMUL(Register::RCX, Register::RDX));

        // 5. Load value from [RBX + 16 + RCX] -> RAX
        // RBX is base, 16 is header size, RCX is byte offset (index * 8)
        this->emit(X64Encoder::ADD(Register::RBX, Operand::Imm(16)));
        this->emit(X64Encoder::ADD(Register::RBX, Register::RCX)); // RBX = Base + 16 + Offset
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RAX),
                       Operand::Mem(Register::RBX, 0)));
    }
}

// ✅ Generate code for arr[index] = value
void CodeGenerator::generateArrayAssignment(const Command& cmd) {
    // cmd.variable = array name
    // cmd.condition = index expression
    // cmd.value = value to assign
    
    std::string arrayName = cmd.variable;
    
    // 1. Load array pointer to RBX
    if (!this->symbols.hasSymbol(arrayName)) {
        throw std::runtime_error("Undefined array: " + arrayName);
    }
    
    uint32_t stackOffset = this->symbols.getSymbol(arrayName).offset;
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Reg(Register::RBX),
                   Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    
    // 2. Evaluate index -> RAX
    this->generateExpression(*cmd.condition);
    
    // 3. Calculate offset: index * 8, store in RCX
    this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));
    this->emit(X64Encoder::MOV(Register::RDX, static_cast<int64_t>(8)));
    this->emit(X64Encoder::IMUL(Register::RCX, Register::RDX));
    
    // 4. Calculate target address: RBX = Base + Offset
    // Add header size (16) to skip Capacity and Count, then add element offset
    this->emit(X64Encoder::ADD(Register::RBX, Operand::Imm(16)));
    this->emit(X64Encoder::ADD(Register::RBX, Register::RCX));
    
    // 5. Evaluate value -> RAX
    this->generateExpression(cmd.value);
    
    // 6. Store value at [RBX]
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Mem(Register::RBX, 0),
                   Operand::Reg(Register::RAX)));
}

// ════════════════════════════════════════════════════════════
// 🏗️ OOP Operations Implementation
// ════════════════════════════════════════════════════════════

void CodeGenerator::generateClassDefinition(const Command& cmd) {
    // صنف نقطة:
    //     خاصية س
    //     خاصية ص
    
    if (!cmd.class_def) {
        throw std::runtime_error("Missing class definition data");
    }
    
    ClassInfo info;
    info.name = cmd.class_name;
    info.parentClass = cmd.class_def->parent_class; // Support inheritance
    info.size = 0;
    
    // If this class extends another class, inherit its properties first
    if (!info.parentClass.empty() && this->symbols.hasClass(info.parentClass)) {
        const ClassInfo& parentInfo = this->symbols.getClass(info.parentClass);
        // Copy parent properties
        for (const auto& propPair : parentInfo.propertyOffsets) {
            info.propertyOffsets[propPair.first] = propPair.second;
            info.size = std::max(info.size, propPair.second + 8); // Update size
        }
        std::cout << "📦 الصنف " << info.name << " يرث " << parentInfo.propertyOffsets.size()
                  << " خاصية من " << info.parentClass << std::endl;
    }

    // Process properties to calculate size and offsets
    for (const auto& prop : cmd.class_def->properties) {
        info.addProperty(prop.first);
    }
    
    // ✅ FIX: Copy methods from cmd.class_def to ClassInfo
    // so that generateNewObject can check if constructor exists
    for (const auto& methodPair : cmd.class_def->methods) {
        info.methods.insert(methodPair.first);
    }
    
    // Register class in SymbolTable
    this->symbols.addClass(info);
    
    // Generate code for methods
    for (const auto& methodPair : cmd.class_def->methods) {
        const auto& method = methodPair.second;
        
        // Create a function command for the method
        Command funcCmd(CommandType::FUNCTION_DEF);
        funcCmd.function_name = cmd.class_name + "_" + method.name; // Mangled name: Class_Method
        funcCmd.variable = funcCmd.function_name; // Set variable field (used by generateFunctionDefinition)
        funcCmd.parameters = method.parameters;
        
        
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            std::cout << "  Param[" << i << "]: " << method.parameters[i] << std::endl;
        }
        
        // Add 'this' as implicit first parameter
        funcCmd.parameters.insert(funcCmd.parameters.begin(), "هذا");
        
        
        for (size_t i = 0; i < funcCmd.parameters.size(); ++i) {
            std::cout << "  funcCmd.Param[" << i << "]: " << funcCmd.parameters[i] << std::endl;
        }
        
        // Convert shared_ptr<Command> to shared_ptr<Command> (copy body)
        funcCmd.body = method.body; // ✅ Fix: Use 'body' field which generateFunctionDefinition iterates over
        
        

        // ✅ Set current class context before generating method
        std::string oldContext = this->currentClassName;
        this->currentClassName = cmd.class_name;
        
        this->generateFunctionDefinition(funcCmd);
        
        // ✅ Restore context
        this->currentClassName = oldContext;
    }
}

void CodeGenerator::generateNewObject(const Command& cmd) {
    // متغير ن = جديد نقطة(10، 20)

    std::string className = cmd.class_name;
    std::string varName = cmd.object_name; // Variable to store the object

    
    

    if (!this->symbols.hasClass(className)) {
        // Debug: List all available classes
        
        for (const auto& cls : this->symbols.getAllClasses()) {
            std::cout << "  - " << cls.first << std::endl;
        }
        throw std::runtime_error("Undefined class: " + className);
    }
    
    const ClassInfo& info = this->symbols.getClass(className);
    size_t objectSize = info.size;
    
    if (objectSize == 0) objectSize = 8; // Minimum size
    
    // 1. Allocate memory: HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)
    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
    uint32_t gphOffset2 = this->getCurrentOffset() + 1;
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(gphOffset2, "GetProcessHeap", RelocationType::REL32);
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));

    this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));
    this->emit(X64Encoder::MOV(Register::RDX, 8)); // HEAP_ZERO_MEMORY
    this->emit(X64Encoder::MOV(Register::R8, static_cast<int64_t>(objectSize)));

    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
    uint32_t haOffset2 = this->getCurrentOffset() + 1;
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(haOffset2, "HeapAlloc", RelocationType::REL32);
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
    
    // 2. Store pointer in variable
    // RAX has the pointer
    if (!this->symbols.hasSymbol(varName)) {
        this->symbols.addVariable(varName, 8, SymbolDataType::OBJECT, className);
    }
    
    // Hack: Access map directly or update addVariable
    // Since SymbolTable is private member, we can't easily change it without modifying SymbolTable.h
    // But we are inside CodeGenerator which has 'symbols' member.
    // SymbolTable::addVariable adds to map. We need a way to set className.
    // Let's assume we'll fix SymbolTable later or use a workaround.
    
    uint32_t stackOffset = this->symbols.getSymbol(varName).offset;
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset)),
                   Operand::Reg(Register::RAX)));
                   
    // 3. Call Constructor if exists (Class_بناء)
    std::string ctorName = className + "_بناء"; // Constructor name convention
    // Check if constructor exists in function table (needs to be populated first)
    // For now, generate call anyway and let linker/relocation handle it?
    // Or check if method exists in class definition
    
    // 3. Call Constructor if exists (Class_بناء)
    // std::string ctorName = className + "_بناء"; // Already defined above
    
    if (this->symbols.getClass(className).methods.count("بناء")) {
        
        // Prepare arguments for register-based calling convention (Microsoft x64)
        // Order: RCX (This), RDX (Arg1), R8 (Arg2), R9 (Arg3), Stack...
        
        // 1. Push 'This' pointer (First Argument)
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RAX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
        this->emit(X64Encoder::PUSH(Register::RAX));
        
        // 2. Push other arguments (Forward Order)
        for (const auto& arg : cmd.arguments) {
            
            this->generateExpression(*arg);
            this->emit(X64Encoder::PUSH(Register::RAX));
        }
        
        // 3. Pop into Registers (Reverse Order)
        // Total args = 1 (This) + cmd.arguments.size()
        int totalArgs = 1 + cmd.arguments.size();
        
        for (int i = totalArgs - 1; i >= 0; --i) {
            if (i >= 4) {
                 // Stack args handling (simplified - just keep on stack?)
                 // If we keep on stack, we shouldn't have popped them?
                 // But we pushed them to evaluate.
                 // In x64, stack args are above shadow space.
                 // We need to pop them and move them to correct stack slot if needed?
                 // Or just leave them if we pushed them in correct order?
                 // Pushed: This, Arg1, Arg2, Arg3, Arg4.
                 // Stack Top: Arg4.
                 // Arg4 is 5th argument. Should be at [RSP + 32 + 8*0].
                 // But we have Shadow Space (32 bytes) to allocate.
                 
                 // For now, let's assume < 4 arguments for simplicity or handle registers only.
                 // If > 4, we need complex stack shuffling.
                 this->emit(X64Encoder::POP(Register::RAX)); // Just pop to clear stack for now (FIXME for >4 args)
            } else {
                this->emit(X64Encoder::POP(Register::RAX));
                switch (i) {
                    case 0: this->emit(X64Encoder::MOV(Register::RCX, Register::RAX)); break; // This
                    case 1: this->emit(X64Encoder::MOV(Register::RDX, Register::RAX)); break; // Arg1
                    case 2: this->emit(X64Encoder::MOV(Register::R8,  Register::RAX)); break; // Arg2
                    case 3: this->emit(X64Encoder::MOV(Register::R9,  Register::RAX)); break; // Arg3
                }
            }
        }
        
        // 4. Allocate Shadow Space
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
        
        // 5. Call Constructor
        uint32_t callOffset = this->getCurrentOffset() + 1;
        this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
        this->relocations.add(callOffset, ctorName, RelocationType::REL32);
        
        // 6. Deallocate Shadow Space
        this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));
    }
}

void CodeGenerator::generatePropertyAccess(const Value& val) {
    // هذا.س  or  obj.x
    // val.value = property name
    // val.object_name = object variable name
    
    // 1. Get Object Pointer -> RBX
    std::string objVar = val.object_name;
    
    // Fallback: empty object in class context => assume "هذا" (parser may mis-group (expr).prop)
    if (objVar.empty() && !this->currentClassName.empty()) {
        objVar = "هذا";
    }
    
    if (objVar == "هذا") {
        // 'this' is a parameter named "هذا"
        if (!this->symbols.hasSymbol("هذا")) {
            std::cerr << "Warning: 'هذا' (this) not available in current context for property access." << std::endl;
            return;
        }
        uint32_t stackOffset = this->symbols.getSymbol("هذا").offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RBX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    } else {
        if (!this->symbols.hasSymbol(objVar)) {
            
            throw std::runtime_error("Undefined object: " + objVar);
        }
        uint32_t stackOffset = this->symbols.getSymbol(objVar).offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RBX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    }
    
    // 2. Get Property Offset
    std::string propName = val.property_name;
    
    std::string className;
    if (objVar == "هذا") {
        // ✅ Use current class context
        if (this->currentClassName.empty()) {
             throw std::runtime_error("'this' used outside of class context");
        }
        className = this->currentClassName;
    } else {
        // ✅ Get class name from symbol table
        if (!this->symbols.hasSymbol(objVar)) {
            throw std::runtime_error("Undefined object: " + objVar);
        }
        className = this->symbols.getSymbol(objVar).className;
        
        if (className.empty()) {
             // Fallback: Search all classes (temporary hack for legacy code)
             for (const auto& clsPair : this->symbols.getAllClasses()) {
                 if (clsPair.second.propertyOffsets.count(propName)) {
                     className = clsPair.first;
                     break;
                 }
             }
        }
    }
    
    if (className.empty()) {
        throw std::runtime_error("Cannot determine class for property: " + propName);
    }
    
    const ClassInfo& info = this->symbols.getClass(className);
    uint32_t offset = info.getPropertyOffset(propName);
    
    // 3. Read Value: [RBX + Offset] -> RAX
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Reg(Register::RAX),
                   Operand::Mem(Register::RBX, static_cast<int32_t>(offset))));
}

void CodeGenerator::generatePropertyAssignment(const Command& cmd) {
    // obj.x = 10
    
    // 1. Evaluate Value -> RAX
    this->generateExpression(cmd.value);
    this->emit(X64Encoder::PUSH(Register::RAX)); // Save value
    
    // 2. Get Object Pointer -> RBX
    std::string fullVar = cmd.variable; // "obj.x"
    size_t dotPos = fullVar.find('.');
    std::string objVar = fullVar.substr(0, dotPos);
    std::string propName = fullVar.substr(dotPos + 1);
    
    // Debug print
    // 
    
    if (objVar == "هذا") {
        // 'this' is a parameter named "هذا"
        if (!this->symbols.hasSymbol("هذا")) {
            std::cerr << "Warning: 'هذا' (this) not available in current context for property access." << std::endl;
            return;
        }
        uint32_t stackOffset = this->symbols.getSymbol("هذا").offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RBX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    } else {
        if (!this->symbols.hasSymbol(objVar)) {
            throw std::runtime_error("Undefined object: " + objVar);
        }
        uint32_t stackOffset = this->symbols.getSymbol(objVar).offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RBX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    }
                   
    // 3. Get Offset
    std::string className;
    if (objVar == "هذا") {
        // ✅ Use current class context
        if (this->currentClassName.empty()) {
             throw std::runtime_error("'this' used outside of class context");
        }
        className = this->currentClassName;
    } else {
        // Search all classes (temporary hack)
        for (const auto& clsPair : this->symbols.getAllClasses()) {
             if (clsPair.second.propertyOffsets.count(propName)) {
                 className = clsPair.first;
                 break;
             }
        }
    }
    
    if (className.empty()) {
        throw std::runtime_error("Cannot determine class for property: " + propName);
    }
    
    const ClassInfo& info = this->symbols.getClass(className);
    uint32_t offset = info.getPropertyOffset(propName);
    
    // 4. Restore Value -> RAX
    this->emit(X64Encoder::POP(Register::RAX));
    
    // 5. Write: [RBX + Offset] = RAX
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Mem(Register::RBX, static_cast<int32_t>(offset)),
                   Operand::Reg(Register::RAX)));
}

void CodeGenerator::generatePropertyArrayAssignment(const Command& cmd) {
    // obj.prop[index] = value
    
    // 1. Get Object Pointer -> RBX
    std::string objVar = cmd.object_name;
    
    if (objVar == "هذا") {
        if (!this->symbols.hasSymbol("هذا")) {
             throw std::runtime_error("'this' (هذا) not found in current context");
        }
        uint32_t stackOffset = this->symbols.getSymbol("هذا").offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RBX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    } else {
        if (!this->symbols.hasSymbol(objVar)) {
            throw std::runtime_error("Undefined object: " + objVar);
        }
        uint32_t stackOffset = this->symbols.getSymbol(objVar).offset;
        this->emit(Instruction(InstructionType::MOV,
                       Operand::Reg(Register::RBX),
                       Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    }
    
    // 2. Get Property Offset
    std::string propName = cmd.property_name;
    std::string className;
    
    if (objVar == "هذا") {
        if (this->currentClassName.empty()) {
             throw std::runtime_error("'this' used outside of class context");
        }
        className = this->currentClassName;
    } else {
        className = this->symbols.getSymbol(objVar).className;
        if (className.empty()) {
             // Fallback search
             for (const auto& clsPair : this->symbols.getAllClasses()) {
                 if (clsPair.second.propertyOffsets.count(propName)) {
                     className = clsPair.first;
                     break;
                 }
             }
        }
    }
    
    if (className.empty()) {
        throw std::runtime_error("Cannot determine class for property: " + propName);
    }
    
    const ClassInfo& info = this->symbols.getClass(className);
    uint32_t offset = info.getPropertyOffset(propName);
    
    // 3. Load Array Pointer from Property -> RBX
    // [RBX + Offset] contains the address of the array
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Reg(Register::RBX),
                   Operand::Mem(Register::RBX, static_cast<int32_t>(offset))));
                   
    // 4. Evaluate Index -> RAX
    // Save RBX (Array Pointer)
    this->emit(X64Encoder::PUSH(Register::RBX));
    
    if (cmd.condition) {
        this->generateExpression(*cmd.condition); // Index
    } else {
        std::cerr << "Warning: Property array assignment missing index" << std::endl;
        this->emit(X64Encoder::MOV(Register::RAX, 0));
    }
    
    // Restore RBX to RDX (temp) then move back? No, just pop to RBX
    // But we need RAX (index).
    // Move RAX to RCX (index)
    this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));
    this->emit(X64Encoder::POP(Register::RBX)); // Restore Array Pointer
    
    // 5. Calculate Element Address
    // Offset = Index * 8
    this->emit(X64Encoder::MOV(Register::RDX, static_cast<int64_t>(8)));
    this->emit(X64Encoder::IMUL(Register::RCX, Register::RDX));
    
    // Target = Base + Offset
    this->emit(X64Encoder::ADD(Register::RBX, Register::RCX));
    
    // 6. Evaluate Value -> RAX
    // Save RBX (Target Address)
    this->emit(X64Encoder::PUSH(Register::RBX));
    
    this->generateExpression(cmd.value);
    
    this->emit(X64Encoder::POP(Register::RBX)); // Restore Target Address
    
    // 7. Store Value: [RBX] = RAX
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Mem(Register::RBX, 0),
                   Operand::Reg(Register::RAX)));
}

void CodeGenerator::generateMethodCall(const Command& cmd) {
    // obj.method(args)

    

    // ✅ Handle constructor calls: ClassName.جديد(args) -> CREATE_OBJECT
    if (cmd.method_name == "جديد" && this->symbols.hasClass(cmd.object_name)) {
        // This is a constructor call like "رمز.جديد(...)"
        // Convert to CREATE_OBJECT command
        Command createCmd(CommandType::CREATE_OBJECT);
        createCmd.class_name = cmd.object_name;
        createCmd.arguments = cmd.arguments;
        createCmd.object_name = cmd.object_name; // For consistency
        this->generateNewObject(createCmd);
        return;
    }

    // 1. Push Args (Reverse Order)
    for (auto it = cmd.arguments.rbegin(); it != cmd.arguments.rend(); ++it) {
        if (!*it) continue; // Null safety
        
        this->generateExpression(**it);
        this->emit(X64Encoder::PUSH(Register::RAX));
    }

    // 2. Push 'this' (Object Pointer)
    std::string objVar = cmd.object_name;
    if (!this->symbols.hasSymbol(objVar)) {
         
         throw std::runtime_error("Undefined object: " + objVar);
    }
    uint32_t stackOffset = this->symbols.getSymbol(objVar).offset;
    this->emit(Instruction(InstructionType::MOV,
                   Operand::Reg(Register::RAX),
                   Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset))));
    this->emit(X64Encoder::PUSH(Register::RAX)); // 'this' is last pushed (first arg)

    // 3. Pop into Registers (Microsoft x64 calling convention)
    int totalArgs = 1 + cmd.arguments.size(); // This + Args

    // Stack has: [This, Arg1, Arg2, ...] (Top is This)
    // We pop in order: This -> RCX, Arg1 -> RDX, etc.

    for (int i = 0; i < totalArgs; ++i) {
        if (i < 4) {
            Register targetReg;
            switch (i) {
                case 0: targetReg = Register::RCX; break; // This
                case 1: targetReg = Register::RDX; break; // Arg1
                case 2: targetReg = Register::R8;  break; // Arg2
                case 3: targetReg = Register::R9;  break; // Arg3
            }
            this->emit(X64Encoder::POP(targetReg));
        } else {
            // ✅ FIX: Do NOT pop stack arguments (>4)!
            // They must remain on the stack for the callee.
            break;
        }
    }

    // 3.5 Handle Native Array Methods
    const Symbol& sym = this->symbols.getSymbol(objVar);
    if (sym.dataType == SymbolDataType::ARRAY) {
        if (cmd.method_name == "طول") {
            // Length: Return Count from [RCX + 8]
            this->emit(Instruction(InstructionType::MOV,
                           Operand::Reg(Register::RAX),
                           Operand::Mem(Register::RCX, 8)));
            return;
        } else if (cmd.method_name == "أضف") {
            // Append: RCX = Array Base, RDX = Value to add

            // Save original array pointer (RCX will be overwritten by realloc)
            this->emit(X64Encoder::PUSH(Register::RCX));
            this->emit(X64Encoder::PUSH(Register::RDX)); // Save value to add

            // 1. Check Capacity vs Count
            this->emit(X64Encoder::MOV(Register::R8, Operand::Mem(Register::RCX, 0))); // R8 = Capacity
            this->emit(X64Encoder::MOV(Register::R9, Operand::Mem(Register::RCX, 8))); // R9 = Count

            this->emit(Instruction(InstructionType::CMP, Operand::Reg(Register::R9), Operand::Reg(Register::R8)));

            std::string skipReallocLabel = "skip_realloc_" + std::to_string(this->getCurrentOffset());
            this->emit(X64Encoder::JL(skipReallocLabel)); // If Count < Capacity, skip

            // 2. Realloc
            // New Capacity = Capacity * 2
            this->emit(X64Encoder::SHL_(Register::R8, 1));

            // Save original pointer before calling realloc
            this->emit(Instruction(InstructionType::MOV,
                           Operand::Reg(Register::RDI),
                           Operand::Reg(Register::RCX))); // RDI = original array pointer

            this->emit(X64Encoder::MOV(Operand::Mem(Register::RDI, 0), Register::R8)); // Update Capacity in original buffer

            // New Total Bytes = (New Capacity * 8) + 16
            this->emit(X64Encoder::MOV(Register::RDX, Register::R8));
            this->emit(X64Encoder::SHL_(Register::RDX, 3)); // * 8
            this->emit(X64Encoder::ADD(Register::RDX, Operand::Imm(16))); // + 16

            // Use Win32 HeapReAlloc (instead of realloc)
            // Save new size in R10 while we get the heap handle
            this->emit(X64Encoder::MOV(Register::R10, Register::RDX));

            // GetProcessHeap()
            this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
            uint32_t gphOffset3 = this->getCurrentOffset() + 1;
            this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
            this->relocations.add(gphOffset3, "GetProcessHeap", RelocationType::REL32);
            this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));

            // HeapReAlloc(hHeap, dwFlags, lpMem, dwBytes)
            this->emit(X64Encoder::MOV(Register::RCX, Register::RAX)); // hHeap
            this->emit(X64Encoder::MOV(Register::RDX, 8));            // dwFlags (HEAP_ZERO_MEMORY)
            this->emit(X64Encoder::MOV(Register::R8, Register::RDI));  // lpMem (old pointer)
            this->emit(X64Encoder::MOV(Register::R9, Register::R10));  // dwBytes (new size)

            this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));
            uint32_t hraOffset = this->getCurrentOffset() + 1;
            this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
            this->relocations.add(hraOffset, "HeapReAlloc", RelocationType::REL32);
            this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(32)));

            // RAX has new pointer. Update local variable.
            this->emit(Instruction(InstructionType::MOV,
                           Operand::Mem(Register::RBP, -static_cast<int32_t>(stackOffset)),
                           Operand::Reg(Register::RAX)));

            // RCX = new base pointer from RAX
            this->emit(X64Encoder::MOV(Register::RCX, Register::RAX));

            // Jump to done (skip the non-realloc path that tries to pop)
            this->emit(X64Encoder::JMP(skipReallocLabel + "_done"));

            // skipReallocLabel: This is where we jump if no reallocation is needed
            this->symbols.addLabel(skipReallocLabel, this->getCurrentOffset());
            // No realloc needed - just restore RCX from stack (it was pushed first)
            // RCX already has the correct pointer, no need to reload
            this->emit(X64Encoder::MOV(Register::R9, Operand::Mem(Register::RCX, 8))); // Reload Count

            this->symbols.addLabel(skipReallocLabel + "_done", this->getCurrentOffset());

            // 3. Store: [RCX + 16 + Count * 8] = RDX (value)
            // Reload the value from stack (it was pushed second, after RCX)
            this->emit(X64Encoder::MOV(Register::RDX, Operand::Mem(Register::RSP, 0))); // RDX = value to add

            this->emit(X64Encoder::MOV(Register::R8, Register::R9));
            this->emit(X64Encoder::SHL_(Register::R8, 3)); // Count * 8
            this->emit(X64Encoder::ADD(Register::R8, Operand::Imm(16))); // + 16
            this->emit(X64Encoder::ADD(Register::R8, Register::RCX)); // Base + Absolute Offset

            this->emit(Instruction(InstructionType::MOV,
                           Operand::Mem(Register::R8, 0),
                           Operand::Reg(Register::RDX)));

            // 4. Increment Count
            this->emit(X64Encoder::INC(Register::R9));
            this->emit(X64Encoder::MOV(Operand::Mem(Register::RCX, 8), Register::R9));

            // Clean up stack (remove the 2 pushed values)
            this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(16)));

            this->emit(X64Encoder::MOV(Register::RAX, 0)); // Return 0
            return;
        }
    }

    // 4. Find class name
    std::string className;

    if (cmd.object_name == "هذا") {
        if (this->currentClassName.empty()) {
             throw std::runtime_error("'this' used outside of class context");
        }
        className = this->currentClassName;
    } else {
        if (!this->symbols.hasSymbol(cmd.object_name)) {
             throw std::runtime_error("Undefined object: " + cmd.object_name);
        }
        className = this->symbols.getSymbol(cmd.object_name).className;

        if (className.empty()) {
            // Fallback: Search all classes to find one that has this method
            for (const auto& clsPair : this->symbols.getAllClasses()) {
                 if (clsPair.second.methods.count(cmd.method_name)) {
                     className = clsPair.first;
                     break;
                 }
            }
        }
    }

    if (className.empty()) {
        throw std::runtime_error("Cannot determine class for method: " + cmd.method_name);
    }

    // 5. Call Function (Class_Method)
    std::string mangledName = className + "_" + cmd.method_name;

    // 6. Allocate Shadow Space
    this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(32)));

    // 7. CALL with relocation
    uint32_t callOffset = this->getCurrentOffset() + 1;
    this->emit(Instruction(InstructionType::CALL, Operand::Imm(0)));
    this->relocations.add(callOffset, mangledName, RelocationType::REL32);

    // 8. Deallocate Shadow Space + Stack Arguments
    int extraArgs = std::max(0, totalArgs - 4);
    int totalCleanup = 32 + (extraArgs * 8);
    this->emit(X64Encoder::ADD(Register::RSP, Operand::Imm(totalCleanup)));
}
std::map<std::string, int> CodeGenerator::getStatistics() const {
    std::map<std::string, int> stats;
    stats["Code Size (bytes)"] = static_cast<int>(this->code.size());
    stats["Data Size (bytes)"] = static_cast<int>(this->dataSection.size());
    stats["Variables"] = this->symbols.getVariablesCount();
    stats["Imports"] = static_cast<int>(this->symbols.getImportNames().size());
    return stats;
}

// ════════════════════════════════════════════════════════════
// 🔬 ABI Compliance Checks
// ════════════════════════════════════════════════════════════

void CodeGenerator::runShadowSpaceCheck() {
    std::cout << "🔍 جاري فحص Shadow Space للنداءات الخارجية..." << std::endl;
    auto importNames = this->symbols.getImportNames();
    std::map<std::string, uint32_t> importedFunctions;
    for (const auto& name : importNames) {
        importedFunctions[name] = 0; // القيمة غير مهمة للفحص
    }

/*
    ShadowSpaceChecker shadowChecker(this->code, importedFunctions);
    shadowChecker.checkAllCalls();
    shadowChecker.printReport();

    if (shadowChecker.getWarningCount() > 0) {
        std::cout << "⚠️ تم العثور على " << shadowChecker.getWarningCount() 
                  << " تحذيرات متعلقة بـ Shadow Space" << std::endl;
    } else {
        std::cout << "✅ جميع النداءات الخارجية تحتوي على Shadow Space الصحيح" << std::endl;
    }
*/
}

void CodeGenerator::generateLambdaDef(const Command& cmd) {
    // Generate code for lambda expression
    // For now, treat as a simple function (experimental feature)

    std::string lambdaName = "__lambda_" + std::to_string(this->getCurrentOffset());
    
    // 0. Jump over lambda body
    std::string skipLabel = "lambda_skip_" + lambdaName;
    uint32_t jmpOffset = this->getCurrentOffset();
    this->emit(X64Encoder::JMP(500)); 
    this->relocations.add(jmpOffset + 1, skipLabel, RelocationType::REL32);

    uint32_t lambdaOffset = this->getCurrentOffset();

    // Register lambda as a function
    if (this->symbols.hasSymbol(lambdaName)) {
        Symbol& sym = this->symbols.getSymbol(lambdaName);
        sym.offset = lambdaOffset;
        sym.resolved = true;
    } else {
        this->symbols.addLabel(lambdaName, lambdaOffset);
    }
    this->functionTable[lambdaName] = lambdaOffset;

    // For now, just generate a simple function that returns the parameter
    // TODO: Implement full lambda expression parsing and code generation

    std::cout << "🔄 مولد lambda تجريبي: " << lambdaName << " (معاملات: " << cmd.lambda_params << ")" << std::endl;

    // Simple lambda implementation (experimental)
    // PUSH RBP
    this->emit(X64Encoder::PUSH(Register::RBP));
    this->emit(X64Encoder::MOV(Register::RBP, Register::RSP));

    // Allocate space for parameters
    if (!cmd.lambda_params.empty()) {
        this->emit(X64Encoder::SUB(Register::RSP, Operand::Imm(16))); // Space for params
    }

    // For now, just return the first parameter (simplified)
    if (!cmd.lambda_params.empty()) {
        this->emit(X64Encoder::MOV(Register::RAX, Operand::Mem(Register::RBP, 16))); // First param
    } else {
        this->emit(X64Encoder::MOV(Register::RAX, Operand::Imm(0))); // Default return 0
    }

    // Cleanup and return
    this->emit(X64Encoder::MOV(Register::RSP, Register::RBP));
    this->emit(X64Encoder::POP(Register::RBP));
    this->emit(X64Encoder::RET());
    
    // Place skip label
    this->symbols.addLabel(skipLabel, this->getCurrentOffset());
}

void CodeGenerator::emit(const Instruction& instr) {
    std::vector<uint8_t> encoded = encoder.encode(instr);
    code.insert(code.end(), encoded.begin(), encoded.end());
}

} // namespace ArabicAssembler
