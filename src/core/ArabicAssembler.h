// ArabicAssembler.h - المجمع العربي المحدث مع ImprovedPEBuilder
// ✅ استبدال IndependentPEBuilder بـ ImprovedPEBuilder المحسّن
// ✅ دعم كامل للـ Import Tables و Relocations

#ifndef ARABIC_ASSEMBLER_H
#define ARABIC_ASSEMBLER_H

#include "ImprovedPEBuilder.h"  // ✅ استخدام الباني المحسّن
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cstdint>
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <ctime>
#include <chrono>
#include <stack> // Added by user instruction

#include "ArabicTypes.h"

// Undefine macros that might conflict with STL and DataType enum
#undef min
#undef max
#undef small
#undef VOID
#undef INT8
#undef INT16
#undef INT32
#undef INT64
#undef UINT8
#undef UINT16
#undef UINT32
#undef UINT64
#undef FLOAT
#undef BOOLEAN
#undef STRING
#undef POINTER
#undef CHARACTER
#undef BOOL
#undef byte
#undef Value
#undef Command

static std::string mapArabicSectionDirective_global(const std::string&);
static std::string mapArabicDataDirective_global(const std::string&);
static std::string mapArabicOperand_global(const std::string&);
static std::string mapArabicRegister_global(const std::string&);

namespace ArabicAssembler {

    // ════════════════════════════════════════════════════════════
    // 🎯 نظام الأنواع المتقدم
    // ════════════════════════════════════════════════════════════
    enum class DataType {
        VOID, INT8, INT16, INT32, INT64, UINT8, UINT16, UINT32, UINT64,
        FLOAT32, FLOAT64, BOOLEAN, CHARACTER, STRING, ARRAY, STRUCT,
        POINTER, FUNCTION, CUSTOM
    };

    class TypeInfo {
    public:
        DataType baseType;
        size_t size;
        size_t alignment;
        bool isSigned;
        bool isConst;

        TypeInfo(DataType type) : baseType(type), size(0), alignment(0),
            isSigned(true), isConst(false) {
            calculateSize();
        }
        
        void calculateSize() {
            switch (baseType) {
            case DataType::VOID: size = 0; alignment = 1; break;
            case DataType::INT8: size = 1; alignment = 1; break;
            case DataType::INT16: size = 2; alignment = 2; break;
            case DataType::INT32: size = 4; alignment = 4; break;
            case DataType::INT64: size = 8; alignment = 8; break;
            case DataType::UINT8: size = 1; alignment = 1; break;
            case DataType::UINT16: size = 2; alignment = 2; break;
            case DataType::UINT32: size = 4; alignment = 4; break;
            case DataType::UINT64: size = 8; alignment = 8; break;
            case DataType::FLOAT32: size = 4; alignment = 4; break;
            case DataType::FLOAT64: size = 8; alignment = 8; break;
            case DataType::BOOLEAN: size = 1; alignment = 1; break;
            case DataType::CHARACTER: size = 1; alignment = 1; break;
            case DataType::STRING: size = 8; alignment = 8; break;
            case DataType::POINTER: size = 8; alignment = 8; break;
            default: size = 0; alignment = 1; break;
            }
        }

        std::string toString() const {
            switch (baseType) {
            case DataType::VOID: return "void";
            case DataType::INT32: return "int32";
            case DataType::INT64: return "int64";
            case DataType::FLOAT64: return "float64";
            case DataType::STRING: return "string";
            case DataType::POINTER: return "pointer";
            default: return "unknown";
            }
        }
    };

    // ════════════════════════════════════════════════════════════
    // 📖 محلل Assembly المتقدم
    // ════════════════════════════════════════════════════════════
    class AdvancedParser {
    public:
        struct Instruction {
            std::string mnemonic;
            std::vector<std::string> operands;
            int lineNumber = 0;
        };

        enum class Section { TEXT, DATA, IMPORTS, CONSTS };
        Section currentSection = Section::TEXT;
        std::map<std::string, size_t> dataLabels;
        std::vector<uint8_t> dataBytes;
        size_t currentDataOffset = 0;
        std::vector<ArabicAssembler::ImprovedPEBuilder::ImportDLL> parsedImports;
        std::map<std::string, size_t> constLabels;
        std::vector<uint8_t> constBytes;
        size_t currentConstOffset = 0;

        std::vector<Instruction> parse(const std::string& asmCode) {
            std::vector<Instruction> instructions;
            labels.clear();
            dataLabels.clear();
            dataBytes.clear();
            currentSection = Section::TEXT;
            currentDataOffset = 0;
            parsedImports.clear();

            std::istringstream stream(asmCode);
            std::string line;
            int lineNum = 0;
            size_t instrIndex = 0;

            while (std::getline(stream, line)) {
                lineNum++;
                line = trim(line);
                // Arabic-only import section triggers
                if (line.find("واردات") != std::string::npos || line.find("قسم .واردات") != std::string::npos) {
                    currentSection = Section::IMPORTS;
                    continue;
                }
                line = mapArabicSectionDirective_global(line);

                if (line.empty() || line[0] == '#' || line[0] == ';') continue;

                // Handle Section Directives
                if (line.find("section .data") != std::string::npos || line.find("قسم .بيانات") != std::string::npos) {
                    currentSection = Section::DATA;
                    continue;
                }
                if (line.find("section .text") != std::string::npos || line.find("قسم .نص") != std::string::npos) {
                    currentSection = Section::TEXT;
                    continue;
                }
                if (line.find("قسم .ثابتات") != std::string::npos || line.find("ثابتات:") != std::string::npos) {
                    currentSection = Section::CONSTS;
                    continue;
                }
                
                // ✅ Ignore global and extern directives
                if (line.find("global ") == 0 || line.find("extern ") == 0) {
                    continue;
                }

                if (currentSection == Section::DATA) {
                    std::string dl = trim(line);
                    dl = mapArabicDataDirective_global(dl);
                }
                else if (currentSection == Section::IMPORTS) {
                    std::string imp = trim(line);
                    if (imp.empty()) continue;
                    // Parse: مكتبة "kernel32": دالة "ExitProcess"
                    if (imp.find("مكتبة") != std::string::npos) {
                        size_t q1 = imp.find('"');
                        size_t q2 = std::string::npos;
                        if (q1 != std::string::npos) q2 = imp.find('"', q1 + 1);
                        std::string lib;
                        if (q1 != std::string::npos && q2 != std::string::npos) lib = imp.substr(q1 + 1, q2 - q1 - 1);
                        // function name after دالة
                        std::string func;
                        size_t dalah = imp.find("دالة", q2 != std::string::npos ? q2 : 0);
                        if (dalah != std::string::npos) {
                            size_t fq1 = imp.find('"', dalah);
                            size_t fq2 = std::string::npos;
                            if (fq1 != std::string::npos) fq2 = imp.find('"', fq1 + 1);
                            if (fq1 != std::string::npos && fq2 != std::string::npos) func = imp.substr(fq1 + 1, fq2 - fq1 - 1);
                        }
                        if (!lib.empty() && !func.empty()) {
                            addImport(lib, func);
                        }
                    }
                    else if (imp.rfind("قالب", 0) == 0) {
                        std::string t = trim(imp.substr(std::string("قالب").size()));
                        addImportTemplate(t);
                    }
                    continue;
                }
                else if (currentSection == Section::CONSTS) {
                    std::string dl = trim(line);
                    dl = mapArabicDataDirective_global(dl);
                    // reuse processLine for data constant handling
                    // Add label mapping if present
                    if (isLabel(dl)) {
                        size_t colonPos = dl.find(':');
                        std::string labelName = trim(dl.substr(0, colonPos));
                        constLabels[labelName] = currentConstOffset;
                        dl = trim(dl.substr(colonPos + 1));
                        if (dl.empty()) continue;
                    }
                    // only db/dq/dd supported
                    if (dl.rfind("db", 0) == 0) {
                        size_t quoteStart = dl.find('"');
                        if (quoteStart != std::string::npos) {
                            size_t quoteEnd = dl.find('"', quoteStart + 1);
                            if (quoteEnd != std::string::npos) {
                                std::string str = dl.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                                for (char c : str) { constBytes.push_back(static_cast<uint8_t>(c)); currentConstOffset++; }
                                if (dl.find(", 0", quoteEnd) != std::string::npos || dl.find(",0", quoteEnd) != std::string::npos) {
                                    constBytes.push_back(0); currentConstOffset++;
                                }
                            }
                        } else {
                            std::string values = dl.substr(2);
                            size_t pos = 0;
                            while ((pos = values.find(',')) != std::string::npos || !values.empty()) {
                                std::string val = (pos != std::string::npos) ? values.substr(0, pos) : values;
                                val = trim(val);
                                if (!val.empty() && std::isdigit(val[0])) { constBytes.push_back(static_cast<uint8_t>(std::stoi(val))); currentConstOffset++; }
                                if (pos == std::string::npos) break;
                                values = values.substr(pos + 1);
                            }
                        }
                    } else if (dl.rfind("dq", 0) == 0) {
                        std::string val = trim(dl.substr(2));
                        int64_t num = (val.empty() || val == "0") ? 0 : std::stoll(val);
                        for (int i = 0; i < 8; i++) { constBytes.push_back(static_cast<uint8_t>((num >> (i * 8)) & 0xFF)); }
                        currentConstOffset += 8;
                    } else if (dl.rfind("dd", 0) == 0) {
                        std::string val = trim(dl.substr(2));
                        int32_t num = (val.empty() || val == "0") ? 0 : std::stoi(val);
                        for (int i = 0; i < 4; i++) { constBytes.push_back(static_cast<uint8_t>((num >> (i * 8)) & 0xFF)); }
                        currentConstOffset += 4;
                    }
                }
                
                // Check for labels
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    std::string labelName = trim(line.substr(0, colonPos));
                    
                    if (currentSection == Section::TEXT) {
                        labels[labelName] = instrIndex;
                    } else {
                        dataLabels[labelName] = currentDataOffset;
                    }

                    std::string afterLabel = trim(line.substr(colonPos + 1));
                    
                    if (!afterLabel.empty()) {
                        processLine(afterLabel, lineNum, instructions, instrIndex);
                    }
                }
                else {
                    processLine(line, lineNum, instructions, instrIndex);
                }
            }

            return instructions;
        }

        void emitShadowSpaceWarnings(const std::vector<Instruction>& instructions,
                                     const std::vector<int>& imports) {
            std::set<std::string> importNames;
            /*
            for (const auto& dll : imports) {
                for (size_t k = 0; k < dll.functions.size(); k++) {
                     importNames.insert(dll.functions[k].name);
                }
            }
            */
            for (size_t i = 0; i < instructions.size(); ++i) {
                const auto& ins = instructions[i];
                if (ins.mnemonic == "call" && !ins.operands.empty()) {
                    std::string target = ins.operands[0];
                    if (importNames.count(target)) {
                        bool hasPrologue = false;
                        bool hasEpilogue = false;
                        if (i > 0) {
                            const auto& prev = instructions[i-1];
                            if (prev.mnemonic == "sub" && prev.operands.size() == 2 && prev.operands[0] == "rsp") {
                                try { int v = std::stoi(prev.operands[1]); if (v >= 32) hasPrologue = true; } catch (...) {}
                            }
                        }
                        if (i + 1 < instructions.size()) {
                            const auto& next = instructions[i+1];
                            if (next.mnemonic == "add" && next.operands.size() == 2 && next.operands[0] == "rsp") {
                                try { int v = std::stoi(next.operands[1]); if (v >= 32) hasEpilogue = true; } catch (...) {}
                            }
                        }
                        if (!(hasPrologue && hasEpilogue)) {
                            std::cout << "⚠️ تحذير ABI: ينبغي حجز 32-بايت حول نداء خارجي '" << target
                                      << "' عند السطر " << ins.lineNumber << std::endl;
                        }
                    }
                }
            }
        }

        void processLine(const std::string& line, int lineNum, std::vector<Instruction>& instructions, size_t& instrIndex) {
            if (currentSection == Section::TEXT) {
                Instruction instr = parseLine(line);
                instr.lineNumber = lineNum;
                instructions.push_back(instr);
                instrIndex++;
            }
            else {
                // Parse data directives and generate bytes
                std::string trimmedLine = trim(line);
                
                if (trimmedLine.rfind("db", 0) == 0) { // Starts with db
                    // Parse: db "string", 0  or  db 10, 0
                    size_t quoteStart = trimmedLine.find('"');
                    if (quoteStart != std::string::npos) {
                        size_t quoteEnd = trimmedLine.find('"', quoteStart + 1);
                        if (quoteEnd != std::string::npos) {
                            std::string str = trimmedLine.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                            for (char c : str) {
                                dataBytes.push_back(static_cast<uint8_t>(c));
                                currentDataOffset++;
                            }
                            // Check for trailing ", 0" (null terminator)
                            if (trimmedLine.find(", 0", quoteEnd) != std::string::npos || 
                                trimmedLine.find(",0", quoteEnd) != std::string::npos) {
                                dataBytes.push_back(0);
                                currentDataOffset++;
                            }
                        }
                    }
                    else {
                        // db 10, 0 (comma-separated bytes)
                        std::string values = trimmedLine.substr(2); // Skip "db"
                        // Simple parse: split by comma
                        size_t pos = 0;
                        while ((pos = values.find(',')) != std::string::npos || !values.empty()) {
                            std::string val = (pos != std::string::npos) ? values.substr(0, pos) : values;
                            val = trim(val);
                            if (!val.empty() && std::isdigit(val[0])) {
                                dataBytes.push_back(static_cast<uint8_t>(std::stoi(val)));
                                currentDataOffset++;
                            }
                            if (pos == std::string::npos) break;
                            values = values.substr(pos + 1);
                        }
                    }
                }
                else if (trimmedLine.rfind("dq", 0) == 0) {
                    // dq 0  -> 8 bytes (little-endian)
                    std::string val = trim(trimmedLine.substr(2));
                    int64_t num = (val.empty() || val == "0") ? 0 : std::stoll(val);
                    for (int i = 0; i < 8; i++) {
                        dataBytes.push_back(static_cast<uint8_t>((num >> (i * 8)) & 0xFF));
                    }
                    currentDataOffset += 8;
                }
                else if (trimmedLine.rfind("dd", 0) == 0) {
                    std::string val = trim(trimmedLine.substr(2));
                    int32_t num = (val.empty() || val == "0") ? 0 : std::stoi(val);
                    for (int i = 0; i < 4; i++) {
                        dataBytes.push_back(static_cast<uint8_t>((num >> (i * 8)) & 0xFF));
                    }
                    currentDataOffset += 4;
                }
                else if (trimmedLine.rfind("resq", 0) == 0) {
                    // resq 1 -> 8 zero bytes
                    for (int i = 0; i < 8; i++) {
                        dataBytes.push_back(0);
                    }
                    currentDataOffset += 8;
                }
            }
        }

        std::map<std::string, size_t> getLabels() const { return labels; }
        std::map<std::string, size_t> getDataLabels() const { return dataLabels; }
        std::vector<uint8_t> getDataBytes() const { return dataBytes; }
        std::map<std::string, size_t> getConstLabels() const { return constLabels; }
        std::vector<uint8_t> getConstBytes() const { return constBytes; }

        void addImport(const std::string& dll, const std::string& func) {
            for (size_t i = 0; i < parsedImports.size(); ++i) {
                if (parsedImports[i].name == dll) {
                    bool exists = false;
                    for (size_t k = 0; k < parsedImports[i].functions.size(); k++) {
                        if (parsedImports[i].functions[k].name == func) { exists = true; break; }
                    }
                    if (!exists) { parsedImports[i].functions.push_back({func, 0}); }
                    return;
                }
            }
            ArabicAssembler::ImprovedPEBuilder::ImportDLL nd;
            nd.name = dll;
            nd.functions.push_back({func, 0});
            parsedImports.push_back(std::move(nd));
        }

        std::vector<ArabicAssembler::ImprovedPEBuilder::ImportDLL> getParsedImports() const { return parsedImports; }

        void addImportTemplate(const std::string& t) {
            std::string s = trim(t);
            std::string normalized;
            normalized.reserve(s.size());
            for (char c : s) {
                if (c == '+' || c == ',') normalized.push_back(' ');
                else normalized.push_back(c);
            }
                std::istringstream iss(normalized);
                std::string token;
                auto applyTemplate = [this](const std::string& key) {
                    if (key == "طباعة" || key == "سلاسل") {
                        addImport("msvcrt.dll", "printf");
                        addImport("msvcrt.dll", "strlen");
                        addImport("msvcrt.dll", "strcpy");
                        addImport("msvcrt.dll", "strcat");
                    } else if (key == "نظام" || key == "انهاء") {
                        addImport("KERNEL32.DLL", "ExitProcess");
                        addImport("KERNEL32.DLL", "GetStdHandle");
                        addImport("KERNEL32.DLL", "WriteFile");
                    } else if (key == "ملفات") {
                        addImport("KERNEL32.DLL", "GetStdHandle");
                        addImport("KERNEL32.DLL", "WriteFile");
                        addImport("KERNEL32.DLL", "ReadFile");
                    } else if (key == "ذاكرة") {
                        addImport("msvcrt.dll", "malloc");
                        addImport("msvcrt.dll", "free");
                    } else if (key == "وقت") {
                        addImport("KERNEL32.DLL", "Sleep");
                        addImport("KERNEL32.DLL", "GetTickCount");
                    } else if (key == "مسارات") {
                        addImport("KERNEL32.DLL", "GetModuleFileNameA");
                    } else if (key == "قياس") {
                        addImport("KERNEL32.DLL", "QueryPerformanceCounter");
                        addImport("KERNEL32.DLL", "QueryPerformanceFrequency");
                        addImport("KERNEL32.DLL", "GetTickCount");
                    } else if (key == "تصحيح") {
                        addImport("KERNEL32.DLL", "OutputDebugStringA");
                    }
                };
            while (iss >> token) {
                token = trim(token);
                if (!token.empty()) applyTemplate(token);
            }
        }

    private:
        std::map<std::string, size_t> labels;

        std::string trim(const std::string& str) {
            size_t start = str.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return "";
            size_t end = str.find_last_not_of(" \t\n\r");
            return str.substr(start, end - start + 1);
        }

        bool isLabel(const std::string& line) {
            return line.find(':') != std::string::npos;
        }

        Instruction parseLine(const std::string& line) {
            Instruction instr;
            std::istringstream iss(line);
            iss >> instr.mnemonic;

            instr.mnemonic = mapArabicMnemonic(instr.mnemonic);

            std::string operand;
            while (std::getline(iss, operand, ',')) {
                operand = trim(operand);
                if (!operand.empty()) {
                    operand = mapArabicRegister_global(operand);
                    instr.operands.push_back(mapArabicOperand_global(operand));
                }
            }

            return instr;
        }

        std::string mapArabicMnemonic(const std::string& m) {
            static const std::map<std::string, std::string> mapp = {
                {"حرك", "mov"},
                {"جمع", "add"},
                {"اطرح", "sub"},
                {"جمع_مع_حمل", "adc"},
                {"اطرح_مع_حمل", "sbb"},
                {"ادفع", "push"},
                {"اسحب", "pop"},
                {"ارجع", "ret"},
                {"ناد", "call"},
                {"استدعي", "call"},
                {"اتصل", "call"},
                {"حمّل_عنوان", "lea"},
                {"حمّل", "lea"},
                {"حصريا", "xor"},
                {"و", "and"},
                {"أو", "or"},
                {"اختبر_بت", "bt"},
                {"مسح_بت_أمامي", "bsf"},
                {"مسح_بت_خلفي", "bsr"},
                {"حرك_تمديد_صفر", "movzx"},
                {"حرك_تمديد_إشارة", "movsx"},
                {"ازح_يسارا", "shl"},
                {"ازح_يمينا", "shr"},
                {"اعكس", "not"},
                {"سلب", "neg"},
                {"اختبر", "test"},
                {"قارن", "cmp"},
                {"اقفز", "jmp"},
                {"اقفز_اذا_يساوي", "jz"},
                {"اقفز_اذا_لا_يساوي", "jnz"},
                {"اقفز_اذا_أكبر", "jg"},
                {"اقفز_اذا_أصغر", "jl"},
                {"اقفز_اذا_أكبر_أو_يساوي", "jge"},
                {"اقفز_اذا_أصغر_أو_يساوي", "jle"}
            };
            auto s = m;
            std::string l;
            l.reserve(s.size());
            for (unsigned char c : s) l.push_back(static_cast<char>(std::tolower(c)));
            auto it = mapp.find(l);
            return it != mapp.end() ? it->second : l;
        }
    };

    // ════════════════════════════════════════════════════════════
    // 🔧 مولد كود الآلة المحسّن
    // ════════════════════════════════════════════════════════════
    // ✅ Forward declaration for CodeGenerator compatibility
    class CodeGenerator;

    class AdvancedCodeGenerator {
    public:
        // ✅ Dummy methods for backwards compatibility with CodeGenerator interface
        std::vector<uint8_t> generate(const std::vector<std::shared_ptr<ArabicLanguage::Command>>& commands) {
            return {};  // Return empty vector
        }

        bool buildExecutable(const std::string& outputFile) {
            return true;  // Dummy success
        }

        std::map<std::string, int> getStatistics() const {
            return {};  // Return empty map
        }

        // ✅ Main generate method for AdvancedCodeGenerator
        std::vector<uint8_t> generate(
            const std::vector<AdvancedParser::Instruction>& instructions,
            const std::map<std::string, size_t>& labelMap,
            const std::map<std::string, size_t>& dataLabelMap,
            const std::map<std::string, uint32_t>& imports = {}) {

            this->labels = labelMap;
            this->dataLabels = dataLabelMap;
            this->importMap = imports;
            currentAddress = 0;

            std::vector<uint8_t> machineCode;

            // ✅ PASS 1: Calculate RVA for each instruction by generating temp code
            std::map<size_t, uint32_t> instrToRVA;
            uint32_t currentRVA = 0x1000; // Start of .text section
            
            for (size_t i = 0; i < instructions.size(); i++) {
                instrToRVA[i] = currentRVA;
                
                // Generate actual instruction to get precise size
                const auto& instr = instructions[i];
                std::string mnemonic = instr.mnemonic;
                std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                size_t instrSize = 0;
                if (mnemonic == "mov" && instr.operands.size() == 2) {
                    instrSize = generateMOV(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "add" && instr.operands.size() == 2) {
                    instrSize = generateADD(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "sub" && instr.operands.size() == 2) {
                    instrSize = generateSUB(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "adc" && instr.operands.size() == 2) {
                    instrSize = generateADC(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "sbb" && instr.operands.size() == 2) {
                    instrSize = generateSBB(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "push" && instr.operands.size() == 1) {
                    instrSize = generatePUSH(instr.operands[0]).size();
                }
                else if (mnemonic == "pop" && instr.operands.size() == 1) {
                    instrSize = generatePOP(instr.operands[0]).size();
                }
                else if (mnemonic == "ret") {
                    instrSize = 1;
                }
                else if (mnemonic == "call" && instr.operands.size() == 1) {
                    instrSize = generateCALL(instr.operands[0]).size();
                }
                else if (mnemonic == "lea" && instr.operands.size() == 2) {
                    instrSize = 7; // LEA is always 7 bytes
                }
                else if (mnemonic == "xor" && instr.operands.size() == 2) {
                    instrSize = generateXOR(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "and" && instr.operands.size() == 2) {
                    instrSize = generateAND(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "or" && instr.operands.size() == 2) {
                    instrSize = generateOR(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "shl" && instr.operands.size() == 2) {
                    instrSize = generateSHL(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "shr" && instr.operands.size() == 2) {
                    instrSize = generateSHR(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "not" && instr.operands.size() == 1) {
                    instrSize = generateNOT(instr.operands[0]).size();
                }
                else if (mnemonic == "neg" && instr.operands.size() == 1) {
                    instrSize = generateNEG(instr.operands[0]).size();
                }
                else if (mnemonic == "imul" && instr.operands.size() == 2) {
                    instrSize = 4; // REX + 0F AF + ModRM
                }
                else if (mnemonic == "test" && instr.operands.size() == 2) {
                    instrSize = 3; // REX + 85 + ModRM
                }
                else if (mnemonic == "dec" && instr.operands.size() == 1) {
                    instrSize = 3; // REX + FF + ModRM
                }
                else if (mnemonic == "inc" && instr.operands.size() == 1) {
                    instrSize = 3; // REX + FF + ModRM
                }
                else if (mnemonic == "nop") {
                    instrSize = 1;
                }
                else if (mnemonic == "cmp" && instr.operands.size() == 2) {
                    instrSize = generateCMP(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "bt" && instr.operands.size() == 2) {
                    instrSize = generateBT(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "bsf" && instr.operands.size() == 2) {
                    instrSize = generateBSF(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "bsr" && instr.operands.size() == 2) {
                    instrSize = generateBSR(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "movzx" && instr.operands.size() == 2) {
                    instrSize = generateMOVZX(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "movsx" && instr.operands.size() == 2) {
                    instrSize = generateMOVSX(instr.operands[0], instr.operands[1]).size();
                }
                else if (mnemonic == "jz" || mnemonic == "jnz") {
                    instrSize = 6; // 0F 8x + rel32
                }
                else if (mnemonic == "jg" || mnemonic == "jl" || mnemonic == "jge" || mnemonic == "jle") {
                    instrSize = 6; // 0F 8x + rel32
                }
                else if (mnemonic == "jmp") {
                    instrSize = 5; // E9 + rel32
                }
                else {
                    instrSize = 3; // Default for unknown instructions
                }
                
                currentRVA += instrSize;
            }
            
            // Store for use in generateLEA/CALL
            this->instrToRVA = instrToRVA;
            this->currentInstrIndex = 0;

            // ✅ PASS 2: Generate actual machine code
            currentAddress = 0;
            for (const auto& instr : instructions) {
                std::vector<uint8_t> instrCode;

                std::string mnemonic = instr.mnemonic;
                std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (mnemonic == "mov" && instr.operands.size() == 2) {
                    instrCode = generateMOV(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "add" && instr.operands.size() == 2) {
                    instrCode = generateADD(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "sub" && instr.operands.size() == 2) {
                    instrCode = generateSUB(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "adc" && instr.operands.size() == 2) {
                    instrCode = generateADC(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "sbb" && instr.operands.size() == 2) {
                    instrCode = generateSBB(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "push" && instr.operands.size() == 1) {
                    instrCode = generatePUSH(instr.operands[0]);
                }
                else if (mnemonic == "pop" && instr.operands.size() == 1) {
                    instrCode = generatePOP(instr.operands[0]);
                }
                else if (mnemonic == "ret") {
                    instrCode = generateRET();
                }
                else if (mnemonic == "call" && instr.operands.size() == 1) {
                    instrCode = generateCALL(instr.operands[0]);
                }
                else if (mnemonic == "lea" && instr.operands.size() == 2) {
                    instrCode = generateLEA(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "xor" && instr.operands.size() == 2) {
                    instrCode = generateXOR(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "and" && instr.operands.size() == 2) {
                    instrCode = generateAND(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "or" && instr.operands.size() == 2) {
                    instrCode = generateOR(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "shl" && instr.operands.size() == 2) {
                    instrCode = generateSHL(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "shr" && instr.operands.size() == 2) {
                    instrCode = generateSHR(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "not" && instr.operands.size() == 1) {
                    instrCode = generateNOT(instr.operands[0]);
                }
                else if (mnemonic == "neg" && instr.operands.size() == 1) {
                    instrCode = generateNEG(instr.operands[0]);
                }
                else if (mnemonic == "imul" && instr.operands.size() == 2) {
                    instrCode = generateIMUL(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "test" && instr.operands.size() == 2) {
                    instrCode = generateTEST(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "dec" && instr.operands.size() == 1) {
                    instrCode = generateDEC(instr.operands[0]);
                }
                else if (mnemonic == "inc" && instr.operands.size() == 1) {
                    instrCode = generateINC(instr.operands[0]);
                }
                else if (mnemonic == "nop") {
                    instrCode = generateNOP();
                }
                else if (mnemonic == "cmp" && instr.operands.size() == 2) {
                    instrCode = generateCMP(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "bt" && instr.operands.size() == 2) {
                    instrCode = generateBT(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "bsf" && instr.operands.size() == 2) {
                    instrCode = generateBSF(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "bsr" && instr.operands.size() == 2) {
                    instrCode = generateBSR(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "movzx" && instr.operands.size() == 2) {
                    instrCode = generateMOVZX(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "movsx" && instr.operands.size() == 2) {
                    instrCode = generateMOVSX(instr.operands[0], instr.operands[1]);
                }
                else if (mnemonic == "jz" && instr.operands.size() == 1) {
                    instrCode = generateJZ(instr.operands[0]);
                }
                else if (mnemonic == "jnz" && instr.operands.size() == 1) {
                    instrCode = generateJNZ(instr.operands[0]);
                }
                else if (mnemonic == "jg" && instr.operands.size() == 1) {
                    instrCode = generateJG(instr.operands[0]);
                }
                else if (mnemonic == "jl" && instr.operands.size() == 1) {
                    instrCode = generateJL(instr.operands[0]);
                }
                else if (mnemonic == "jge" && instr.operands.size() == 1) {
                    instrCode = generateJGE(instr.operands[0]);
                }
                else if (mnemonic == "jle" && instr.operands.size() == 1) {
                    instrCode = generateJLE(instr.operands[0]);
                }
                else if (mnemonic == "jmp" && instr.operands.size() == 1) {
                    instrCode = generateJMP(instr.operands[0]);
                }

                machineCode.insert(machineCode.end(), instrCode.begin(), instrCode.end());
                currentAddress += instrCode.size();
                currentInstrIndex++;
            }

            return machineCode;
        }

        std::vector<uint8_t> generateMOV(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;

            // MOV reg, immediate
            if (isRegister(dest) && isImmediate(src)) {
                int64_t imm = std::stoll(src); // ✅ Use 64-bit immediate
                uint8_t reg = getRegisterCode(dest);

                // Check if 64-bit register (rax, rbx, etc.) or 32-bit (eax, ecx, etc.)
                if (dest[0] == 'r') {
                    // 64-bit register: needs REX.W prefix + imm64 (8 bytes)
                    code.push_back(getREXPrefix(true, false, false, reg >= 8));
                    code.push_back(static_cast<uint8_t>(0xB8 + (reg & 7)));
                    
                    // ✅ MOV r64, imm64 uses full 8 bytes
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 32) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 40) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 48) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 56) & 0xFF));
                }
                else if (dest[0] == 'e') {
                    // 32-bit register: no REX prefix needed, imm32 (4 bytes)
                    code.push_back(static_cast<uint8_t>(0xB8 + (reg & 7)));
                    
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
                }
                else {
                    // Legacy or other register
                    code.push_back(static_cast<uint8_t>(0xB8 + reg));
                    
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
                }
            }
            // MOV reg, reg
            else if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);

                if (dest[0] == 'r' || src[0] == 'r') {
                    code.push_back(getREXPrefix(true, srcReg >= 8, false, destReg >= 8));
                }

                code.push_back(0x89);
                code.push_back(getModRM(3, srcReg & 7, destReg & 7));
            }
            // MOV reg, [rel label] (load from memory)
            else if (isRegister(dest) && (src.find("[rel") != std::string::npos || src.find("qword [rel") != std::string::npos)) {
                // Extract label
                std::string label = src;
                size_t relPos = label.find("rel ");
                if (relPos != std::string::npos) {
                    label = label.substr(relPos + 4);
                    if (label.back() == ']') label.pop_back();
                }
                label = ArabicLanguage::trim(label);
                
                uint8_t reg = getRegisterCode(dest);
                
                // Calculate offset
                int32_t offset = 0;
                if (dataLabels.find(label) != dataLabels.end()) {
                    size_t dataOffset = dataLabels[label];
                    uint32_t targetRVA = 0x2000 + static_cast<uint32_t>(dataOffset);
                    uint32_t currentRVA = (instrToRVA.find(currentInstrIndex) != instrToRVA.end()) 
                        ? instrToRVA[currentInstrIndex] 
                        : (0x1000 + static_cast<uint32_t>(currentAddress));
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 7);
                }
                
                // REX.W + 8B /r (MOV r64, r/m64)
                code.push_back(getREXPrefix(true, reg >= 8, false, false));
                code.push_back(0x8B);
                code.push_back(getModRM(0, reg & 7, 5)); // ModRM for RIP-relative
                
                code.push_back(static_cast<uint8_t>(offset & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            }
            // MOV [rel label], reg (store to memory)
            else if (isRegister(src) && (dest.find("[rel") != std::string::npos || dest.find("qword [rel") != std::string::npos)) {
                // Extract label
                std::string label = dest;
                size_t relPos = label.find("rel ");
                if (relPos != std::string::npos) {
                    label = label.substr(relPos + 4);
                    if (label.back() == ']') label.pop_back();
                }
                label = ArabicLanguage::trim(label);
                
                uint8_t reg = getRegisterCode(src);
                
                // Calculate offset
                int32_t offset = 0;
                if (dataLabels.find(label) != dataLabels.end()) {
                    size_t dataOffset = dataLabels[label];
                    uint32_t targetRVA = 0x2000 + static_cast<uint32_t>(dataOffset);
                    uint32_t currentRVA = (instrToRVA.find(currentInstrIndex) != instrToRVA.end()) 
                        ? instrToRVA[currentInstrIndex] 
                        : (0x1000 + static_cast<uint32_t>(currentAddress));
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 7);
                }
                
                // REX.W + 89 /r (MOV r/m64, r64)
                code.push_back(getREXPrefix(true, reg >= 8, false, false));
                code.push_back(0x89);
                code.push_back(getModRM(0, reg & 7, 5)); // ModRM for RIP-relative
                
                code.push_back(static_cast<uint8_t>(offset & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            }
            // MOV [rel label], immediate
            else if (isImmediate(src) && (dest.find("[rel") != std::string::npos || dest.find("qword [rel") != std::string::npos)) {
                // Extract label
                std::string label = dest;
                size_t relPos = label.find("rel ");
                if (relPos != std::string::npos) {
                    label = label.substr(relPos + 4);
                    if (label.back() == ']') label.pop_back();
                }
                label = ArabicLanguage::trim(label);
                
                int64_t imm = std::stoll(src);
                
                // Calculate offset
                int32_t offset = 0;
                if (dataLabels.find(label) != dataLabels.end()) {
                    size_t dataOffset = dataLabels[label];
                    uint32_t targetRVA = 0x2000 + static_cast<uint32_t>(dataOffset);
                    uint32_t currentRVA = (instrToRVA.find(currentInstrIndex) != instrToRVA.end()) 
                        ? instrToRVA[currentInstrIndex] 
                        : (0x1000 + static_cast<uint32_t>(currentAddress));
                    // Instruction size is 11 bytes (REX + C7 + ModRM + Offset + Imm32)
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 11);
                }
                
                // REX.W + C7 /0 (MOV r/m64, imm32)
                code.push_back(getREXPrefix(true, false, false, false));
                code.push_back(0xC7);
                code.push_back(0x05); // ModRM for RIP-relative (Mod=00, RM=101)
                
                // Offset
                code.push_back(static_cast<uint8_t>(offset & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
                
                // Immediate (32-bit only for this opcode)
                code.push_back(static_cast<uint8_t>(imm & 0xFF));
                code.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
                code.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
                code.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
            }

            return code;
        }

        std::vector<uint8_t> generatePUSH(const std::string& reg) {
            std::vector<uint8_t> code;
            uint8_t regCode = getRegisterCode(reg);

            if (reg[0] == 'r' && regCode >= 8) {
                code.push_back(0x41);
            }

            code.push_back(static_cast<uint8_t>(0x50 + (regCode & 7)));
            return code;
        }

        std::vector<uint8_t> generatePOP(const std::string& reg) {
            std::vector<uint8_t> code;
            uint8_t regCode = getRegisterCode(reg);

            if (reg[0] == 'r' && regCode >= 8) {
                code.push_back(0x41);
            }

            code.push_back(static_cast<uint8_t>(0x58 + (regCode & 7)));
            return code;
        }

        std::vector<uint8_t> generateRET() {
            return { 0xC3 };
        }

        std::vector<uint8_t> generateCALL(const std::string& target) {
            // Check if it's an external import
            if (importMap.find(target) != importMap.end()) {
                // Indirect call via IAT: FF 15 <offset>
                // Offset = IAT_RVA - (Current_RVA + Instruction_Length)
                // Current_RVA = 0x1000 + currentAddress
                // Instruction_Length = 6 bytes
                
                uint32_t iatRVA = importMap[target];
                uint32_t currentRVA = 0x1000 + static_cast<uint32_t>(currentAddress);
                int32_t offset = static_cast<int32_t>(iatRVA) - static_cast<int32_t>(currentRVA + 6);
                
                std::vector<uint8_t> code;
                // Reserve 32-byte shadow space (Windows x64 ABI)
                code.push_back(0x48); code.push_back(0x83); code.push_back(0xEC); code.push_back(0x28);
                // Indirect call via IAT
                code.push_back(0xFF);
                code.push_back(0x15);
                code.push_back(static_cast<uint8_t>(offset & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
                // Restore shadow space
                code.push_back(0x48); code.push_back(0x83); code.push_back(0xC4); code.push_back(0x28);
                return code;
            }
            
            // Standard relative call (E8)
            int32_t offset = 0;
            
            if (labels.find(target) != labels.end()) {
                size_t targetInstrIndex = labels[target];
                
                // ✅ Use instrToRVA for accurate offset calculation
                if (instrToRVA.find(currentInstrIndex) != instrToRVA.end() &&
                    instrToRVA.find(targetInstrIndex) != instrToRVA.end()) {
                    uint32_t currentRVA = instrToRVA[currentInstrIndex];
                    uint32_t targetRVA = instrToRVA[targetInstrIndex];
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 5);
                } else {
                    // Fallback (should not happen in Pass 2)
                    offset = static_cast<int32_t>(targetInstrIndex) - static_cast<int32_t>(currentAddress + 5);
                }
                
                std::vector<uint8_t> code;
                code.push_back(0xE8);
                code.push_back(static_cast<uint8_t>(offset & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
                return code;
            }

            // Size pass fallback: choose placeholder length based on target type
            if (importMap.find(target) != importMap.end()) {
                return { 0xFF, 0x15, 0x00, 0x00, 0x00, 0x00 }; // IAT call (6 bytes)
            }
            return { 0xE8, 0x00, 0x00, 0x00, 0x00 }; // relative call (5 bytes)
        }

        uint8_t getRegisterCode(const std::string& reg) {
            static std::map<std::string, uint8_t> regCodes = {
                {"rax", uint8_t{0}}, {"eax", uint8_t{0}}, {"ax", uint8_t{0}}, {"al", uint8_t{0}},
                {"rcx", uint8_t{1}}, {"ecx", uint8_t{1}}, {"cx", uint8_t{1}}, {"cl", uint8_t{1}},
                {"rdx", uint8_t{2}}, {"edx", uint8_t{2}}, {"dx", uint8_t{2}}, {"dl", uint8_t{2}},
                {"rbx", uint8_t{3}}, {"ebx", uint8_t{3}}, {"bx", uint8_t{3}}, {"bl", uint8_t{3}},
                {"rsp", uint8_t{4}}, {"esp", uint8_t{4}}, {"sp", uint8_t{4}}, {"spl", uint8_t{4}},
                {"rbp", uint8_t{5}}, {"ebp", uint8_t{5}}, {"bp", uint8_t{5}}, {"bpl", uint8_t{5}},
                {"rsi", uint8_t{6}}, {"esi", uint8_t{6}}, {"si", uint8_t{6}}, {"sil", uint8_t{6}},
                {"rdi", uint8_t{7}}, {"edi", uint8_t{7}}, {"di", uint8_t{7}}, {"dil", uint8_t{7}},
                {"r8", uint8_t{8}}, {"r8d", uint8_t{8}}, {"r8w", uint8_t{8}}, {"r8b", uint8_t{8}},
                {"r9", uint8_t{9}}, {"r9d", uint8_t{9}}, {"r9w", uint8_t{9}}, {"r9b", uint8_t{9}},
                {"r10", uint8_t{10}}, {"r10d", uint8_t{10}}, {"r10w", uint8_t{10}}, {"r10b", uint8_t{10}},
                {"r11", uint8_t{11}}, {"r11d", uint8_t{11}}, {"r11w", uint8_t{11}}, {"r11b", uint8_t{11}},
                {"r12", uint8_t{12}}, {"r12d", uint8_t{12}}, {"r12w", uint8_t{12}}, {"r12b", uint8_t{12}},
                {"r13", uint8_t{13}}, {"r13d", uint8_t{13}}, {"r13w", uint8_t{13}}, {"r13b", uint8_t{13}},
                {"r14", uint8_t{14}}, {"r14d", uint8_t{14}}, {"r14w", uint8_t{14}}, {"r14b", uint8_t{14}},
                {"r15", uint8_t{15}}, {"r15d", uint8_t{15}}, {"r15w", uint8_t{15}}, {"r15b", uint8_t{15}}
            };
            auto it = regCodes.find(reg);
            return (it != regCodes.end()) ? it->second : uint8_t{ 0 };
        }

        bool isRegister(const std::string& operand) {
            static std::set<std::string> registers = {
                "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
                "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
                "ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
                "al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl",
                "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
                "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
                "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
                "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"
            };
            return registers.find(operand) != registers.end();
        }

        bool isImmediate(const std::string& operand) {
            if (operand.empty()) return false;
            return std::isdigit(static_cast<unsigned char>(operand[0])) ||
                operand[0] == '-';
        }

        int32_t parseImmediate(const std::string& operand) {
            try { return std::stoi(operand); }
            catch (...) { return 0; }
        }

    private:
        std::map<std::string, size_t> labels;
        std::map<std::string, size_t> dataLabels;
        std::map<std::string, uint32_t> importMap;
        std::map<size_t, uint32_t> instrToRVA; // ✅ NEW: Instruction index to RVA mapping
        size_t currentInstrIndex = 0; // ✅ NEW: Current instruction being generated
        size_t currentAddress = 0;

        uint8_t getREXPrefix(bool w, bool r, bool x, bool b) {
            uint8_t rex = 0x40;
            if (w) rex |= 0x08;
            if (r) rex |= 0x04;
            if (x) rex |= 0x02;
            if (b) rex |= 0x01;
            return rex;
        }

        uint8_t getModRM(uint8_t mod, uint8_t reg, uint8_t rm) {
            return (mod << 6) | (reg << 3) | rm;
        }

        std::vector<uint8_t> generateLEA(const std::string& dest, const std::string& src) {
            // src format: [rel label]
            std::string label = src;
            size_t relPos = label.find("rel ");
            if (relPos != std::string::npos) {
                label = label.substr(relPos + 4);
                if (label.back() == ']') label.pop_back();
            }
            
            // Default to 0 offset if label not found
            int32_t offset = 0;
            
            // Check Data Labels first (Absolute offsets in .data)
            if (dataLabels.find(label) != dataLabels.end()) {
                size_t dataOffset = dataLabels[label];
                // Target RVA = .data RVA (0x2000) + offset
                uint32_t targetRVA = 0x2000 + static_cast<uint32_t>(dataOffset);
                
                // ✅ Use instrToRVA for current instruction RVA
                uint32_t currentRVA = (instrToRVA.find(currentInstrIndex) != instrToRVA.end()) 
                    ? instrToRVA[currentInstrIndex] 
                    : (0x1000 + static_cast<uint32_t>(currentAddress)); // Fallback
                
                offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 7);
            }
            // Check Text Labels
            else if (labels.find(label) != labels.end()) {
                size_t targetInstrIndex = labels[label];
                
                // ✅ Use instrToRVA for both current and target
                if (instrToRVA.find(currentInstrIndex) != instrToRVA.end() &&
                    instrToRVA.find(targetInstrIndex) != instrToRVA.end()) {
                    uint32_t currentRVA = instrToRVA[currentInstrIndex];
                    uint32_t targetRVA = instrToRVA[targetInstrIndex];
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 7);
                }
            }
            
            uint8_t reg = getRegisterCode(dest);
            std::vector<uint8_t> code;
            
            // REX.W + 8D /r
            // ✅ REX.R must be based on whether reg >= 8, not hardcoded to true
            code.push_back(getREXPrefix(true, reg >= 8, false, false)); // REX.W=1, R=(reg>=8), X=0, B=0
            code.push_back(0x8D);
            
            // ModRM: Mod=00, Reg=dest, RM=101 (RIP-relative)
            code.push_back(getModRM(0, reg & 7, 5));
            
            code.push_back(static_cast<uint8_t>(offset & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            
            return code;
        }

        std::vector<uint8_t> generateXOR(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                
                // REX.W + 31 /r for 64-bit XOR
                code.push_back(getREXPrefix(true, srcReg >= 8, false, destReg >= 8));
                code.push_back(0x31);  // XOR opcode
                code.push_back(getModRM(3, srcReg & 7, destReg & 7));
            }
            
            return code;
        }

        std::vector<uint8_t> generateAND(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                code.push_back(getREXPrefix(true, srcReg >= 8, false, destReg >= 8));
                code.push_back(0x21); // AND r/m64, r64
                code.push_back(getModRM(3, srcReg & 7, destReg & 7));
            } else if (isRegister(dest) && isImmediate(src)) {
                uint8_t destReg = getRegisterCode(dest);
                int64_t imm = std::stoll(src);
                code.push_back(getREXPrefix(true, false, false, destReg >= 8));
                if (imm >= -128 && imm <= 127) {
                    code.push_back(0x83);
                    code.push_back(getModRM(3, 4, destReg & 7)); // /4 for AND
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                } else {
                    code.push_back(0x81);
                    code.push_back(getModRM(3, 4, destReg & 7)); // /4 for AND
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
                }
            }
            return code;
        }

        std::vector<uint8_t> generateOR(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                code.push_back(getREXPrefix(true, srcReg >= 8, false, destReg >= 8));
                code.push_back(0x09); // OR r/m64, r64
                code.push_back(getModRM(3, srcReg & 7, destReg & 7));
            } else if (isRegister(dest) && isImmediate(src)) {
                uint8_t destReg = getRegisterCode(dest);
                int64_t imm = std::stoll(src);
                code.push_back(getREXPrefix(true, false, false, destReg >= 8));
                if (imm >= -128 && imm <= 127) {
                    code.push_back(0x83);
                    code.push_back(getModRM(3, 1, destReg & 7)); // /1 for OR
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                } else {
                    code.push_back(0x81);
                    code.push_back(getModRM(3, 1, destReg & 7)); // /1 for OR
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
                }
            }
            return code;
        }

        std::vector<uint8_t> generateSHL(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            if (isRegister(dest) && isImmediate(src)) {
                uint8_t reg = getRegisterCode(dest);
                int64_t imm = std::stoll(src);
                code.push_back(getREXPrefix(true, false, false, reg >= 8));
                if (imm == 1) {
                    code.push_back(0xD1);
                    code.push_back(getModRM(3, 4, reg & 7)); // /4 for SHL
                } else {
                    code.push_back(0xC1);
                    code.push_back(getModRM(3, 4, reg & 7));
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                }
            }
            return code;
        }

        std::vector<uint8_t> generateSHR(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            if (isRegister(dest) && isImmediate(src)) {
                uint8_t reg = getRegisterCode(dest);
                int64_t imm = std::stoll(src);
                code.push_back(getREXPrefix(true, false, false, reg >= 8));
                if (imm == 1) {
                    code.push_back(0xD1);
                    code.push_back(getModRM(3, 5, reg & 7)); // /5 for SHR
                } else {
                    code.push_back(0xC1);
                    code.push_back(getModRM(3, 5, reg & 7));
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                }
            }
            return code;
        }

        std::vector<uint8_t> generateNOT(const std::string& reg) {
            std::vector<uint8_t> code;
            if (isRegister(reg)) {
                uint8_t r = getRegisterCode(reg);
                code.push_back(getREXPrefix(true, false, false, r >= 8));
                code.push_back(0xF7);
                code.push_back(getModRM(3, 2, r & 7)); // /2 for NOT
            }
            return code;
        }

        std::vector<uint8_t> generateNEG(const std::string& reg) {
            std::vector<uint8_t> code;
            if (isRegister(reg)) {
                uint8_t r = getRegisterCode(reg);
                code.push_back(getREXPrefix(true, false, false, r >= 8));
                code.push_back(0xF7);
                code.push_back(getModRM(3, 3, r & 7)); // /3 for NEG
            }
            return code;
        }

        std::vector<uint8_t> generateIMUL(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                
                // REX.W + 0F AF /r for 64-bit IMUL
                code.push_back(getREXPrefix(true, destReg >= 8, false, srcReg >= 8));
                code.push_back(0x0F);
                code.push_back(0xAF);
                code.push_back(getModRM(3, destReg & 7, srcReg & 7));
            }
            
            return code;
        }

        std::vector<uint8_t> generateTEST(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                
                // REX.W + 85 /r for 64-bit TEST
                code.push_back(getREXPrefix(true, srcReg >= 8, false, destReg >= 8));
                code.push_back(0x85);
                code.push_back(getModRM(3, srcReg & 7, destReg & 7));
            }
            
            return code;
        }

        std::vector<uint8_t> generateDEC(const std::string& reg) {
            std::vector<uint8_t> code;
            
            if (isRegister(reg)) {
                uint8_t regCode = getRegisterCode(reg);
                
                // REX.W + FF /1 for 64-bit DEC
                code.push_back(getREXPrefix(true, false, false, regCode >= 8));
                code.push_back(0xFF);
                code.push_back(getModRM(3, 1, regCode & 7));
            }
            
            return code;
        }

        std::vector<uint8_t> generateINC(const std::string& reg) {
            std::vector<uint8_t> code;
            
            if (isRegister(reg)) {
                uint8_t regCode = getRegisterCode(reg);
                
                // REX.W + FF /0 for 64-bit INC
                code.push_back(getREXPrefix(true, false, false, regCode >= 8));
                code.push_back(0xFF);
                code.push_back(getModRM(3, 0, regCode & 7));
            }
            
            return code;
        }

        std::vector<uint8_t> generateNOP() {
            return { 0x90 };
        }

        std::vector<uint8_t> generateCMP(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                
                // REX.W + 39 /r for 64-bit CMP
                code.push_back(getREXPrefix(true, srcReg >= 8, false, destReg >= 8));
                code.push_back(0x39);
                code.push_back(getModRM(3, srcReg & 7, destReg & 7));
            }
            else if (isRegister(dest) && isImmediate(src)) {
                uint8_t destReg = getRegisterCode(dest);
                int64_t imm = std::stoll(src);
                
                // CMP r/m64, imm8 (sign-extended) if fits in 8 bits
                if (imm >= -128 && imm <= 127) {
                    code.push_back(getREXPrefix(true, false, false, destReg >= 8));
                    code.push_back(0x83);
                    code.push_back(getModRM(3, 7, destReg & 7)); // /7 for CMP
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                }
                // CMP r/m64, imm32 (sign-extended)
                else {
                    code.push_back(getREXPrefix(true, false, false, destReg >= 8));
                    code.push_back(0x81);
                    code.push_back(getModRM(3, 7, destReg & 7)); // /7 for CMP
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
                }
            }
            
            return code;
        }

        std::vector<uint8_t> generateADD(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                
                // REX.W + 01 /r
                code.push_back(getREXPrefix(true, srcReg >= 8, false, destReg >= 8));
                code.push_back(0x01);
                code.push_back(getModRM(3, srcReg & 7, destReg & 7));
            }
            else if (isRegister(dest) && isImmediate(src)) {
                uint8_t destReg = getRegisterCode(dest);
                int64_t imm = std::stoll(src);
                
                // ADD r/m64, imm8
                if (imm >= -128 && imm <= 127) {
                    code.push_back(getREXPrefix(true, false, false, destReg >= 8));
                    code.push_back(0x83);
                    code.push_back(getModRM(3, 0, destReg & 7)); // /0 for ADD
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                }
                // ADD r/m64, imm32
                else {
                    code.push_back(getREXPrefix(true, false, false, destReg >= 8));
                    code.push_back(0x81);
                    code.push_back(getModRM(3, 0, destReg & 7)); // /0 for ADD
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
                }
            }
            
            return code;
        }

        std::vector<uint8_t> generateADC(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                code.push_back(getREXPrefix(true, srcReg >= 8, false, destReg >= 8));
                code.push_back(0x11);
                code.push_back(getModRM(3, srcReg & 7, destReg & 7));
            } else if (isRegister(dest) && isImmediate(src)) {
                uint8_t destReg = getRegisterCode(dest);
                int64_t imm = std::stoll(src);
                code.push_back(getREXPrefix(true, false, false, destReg >= 8));
                if (imm >= -128 && imm <= 127) {
                    code.push_back(0x83);
                    code.push_back(getModRM(3, 2, destReg & 7));
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                } else {
                    code.push_back(0x81);
                    code.push_back(getModRM(3, 2, destReg & 7));
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
                }
            }
            return code;
        }

        std::vector<uint8_t> generateSBB(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                code.push_back(getREXPrefix(true, srcReg >= 8, false, destReg >= 8));
                code.push_back(0x19);
                code.push_back(getModRM(3, srcReg & 7, destReg & 7));
            } else if (isRegister(dest) && isImmediate(src)) {
                uint8_t destReg = getRegisterCode(dest);
                int64_t imm = std::stoll(src);
                code.push_back(getREXPrefix(true, false, false, destReg >= 8));
                if (imm >= -128 && imm <= 127) {
                    code.push_back(0x83);
                    code.push_back(getModRM(3, 3, destReg & 7));
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                } else {
                    code.push_back(0x81);
                    code.push_back(getModRM(3, 3, destReg & 7));
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
                }
            }
            return code;
        }

        std::vector<uint8_t> generateBT(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                code.push_back(getREXPrefix(true, srcReg >= 8, false, destReg >= 8));
                code.push_back(0x0F);
                code.push_back(0xA3);
                code.push_back(getModRM(3, srcReg & 7, destReg & 7));
            }
            return code;
        }

        std::vector<uint8_t> generateBSF(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                code.push_back(getREXPrefix(true, destReg >= 8, false, srcReg >= 8));
                code.push_back(0x0F);
                code.push_back(0xBC);
                code.push_back(getModRM(3, destReg & 7, srcReg & 7));
            }
            return code;
        }

        std::vector<uint8_t> generateBSR(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                code.push_back(getREXPrefix(true, destReg >= 8, false, srcReg >= 8));
                code.push_back(0x0F);
                code.push_back(0xBD);
                code.push_back(getModRM(3, destReg & 7, srcReg & 7));
            }
            return code;
        }

        std::vector<uint8_t> generateMOVZX(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                bool isByte = false;
                bool isWord = false;
                static const std::set<std::string> r8set = {"al","bl","cl","dl","sil","dil","spl","bpl","r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b"};
                static const std::set<std::string> r16set = {"ax","bx","cx","dx","si","di","sp","bp","r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"};
                isByte = r8set.find(src) != r8set.end();
                isWord = r16set.find(src) != r16set.end();
                if (isByte || isWord) {
                    bool w = true;
                    if (!dest.empty() && (dest[0] == 'e' || dest.back() == 'd')) w = false;
                    code.push_back(getREXPrefix(w, destReg >= 8, false, srcReg >= 8));
                    code.push_back(0x0F);
                    code.push_back(isByte ? 0xB6 : 0xB7);
                    code.push_back(getModRM(3, destReg & 7, srcReg & 7));
                    return code;
                }
            }
            if (isRegister(dest) && (src.find("[rel") != std::string::npos) && (src.find("byte") != std::string::npos || src.find("word") != std::string::npos)) {
                std::string label = src;
                bool isByte = (label.find("byte [rel") != std::string::npos) || (label.find("byte ptr [rel") != std::string::npos);
                size_t relPos = label.find("rel ");
                if (relPos != std::string::npos) {
                    label = label.substr(relPos + 4);
                    if (!label.empty() && label.back() == ']') label.pop_back();
                }
                label = ArabicLanguage::trim(label);
                uint8_t reg = getRegisterCode(dest);
                int32_t offset = 0;
                if (dataLabels.find(label) != dataLabels.end()) {
                    size_t dataOffset = dataLabels[label];
                    uint32_t targetRVA = 0x2000 + static_cast<uint32_t>(dataOffset);
                    uint32_t currentRVA = (instrToRVA.find(currentInstrIndex) != instrToRVA.end()) ? instrToRVA[currentInstrIndex] : (0x1000 + static_cast<uint32_t>(currentAddress));
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 7);
                }
                bool w = true;
                if (!dest.empty() && (dest[0] == 'e' || dest.back() == 'd')) w = false;
                code.push_back(getREXPrefix(w, reg >= 8, false, false));
                code.push_back(0x0F);
                code.push_back(isByte ? 0xB6 : 0xB7);
                code.push_back(getModRM(0, reg & 7, 5));
                code.push_back(static_cast<uint8_t>(offset & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            }
            return code;
        }

        std::vector<uint8_t> generateMOVSX(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                bool isByte = false;
                bool isWord = false;
                static const std::set<std::string> r8set = {"al","bl","cl","dl","sil","dil","spl","bpl","r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b"};
                static const std::set<std::string> r16set = {"ax","bx","cx","dx","si","di","sp","bp","r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"};
                isByte = r8set.find(src) != r8set.end();
                isWord = r16set.find(src) != r16set.end();
                if (isByte || isWord) {
                    bool w = true;
                    if (!dest.empty() && (dest[0] == 'e' || dest.back() == 'd')) w = false;
                    code.push_back(getREXPrefix(w, destReg >= 8, false, srcReg >= 8));
                    code.push_back(0x0F);
                    code.push_back(isByte ? 0xBE : 0xBF);
                    code.push_back(getModRM(3, destReg & 7, srcReg & 7));
                    return code;
                }
            }
            if (isRegister(dest) && (src.find("[rel") != std::string::npos) && (src.find("byte") != std::string::npos || src.find("word") != std::string::npos)) {
                std::string label = src;
                bool isByte = (label.find("byte [rel") != std::string::npos) || (label.find("byte ptr [rel") != std::string::npos);
                size_t relPos = label.find("rel ");
                if (relPos != std::string::npos) {
                    label = label.substr(relPos + 4);
                    if (!label.empty() && label.back() == ']') label.pop_back();
                }
                label = ArabicLanguage::trim(label);
                uint8_t reg = getRegisterCode(dest);
                int32_t offset = 0;
                if (dataLabels.find(label) != dataLabels.end()) {
                    size_t dataOffset = dataLabels[label];
                    uint32_t targetRVA = 0x2000 + static_cast<uint32_t>(dataOffset);
                    uint32_t currentRVA = (instrToRVA.find(currentInstrIndex) != instrToRVA.end()) ? instrToRVA[currentInstrIndex] : (0x1000 + static_cast<uint32_t>(currentAddress));
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 7);
                }
                bool w = true;
                if (!dest.empty() && (dest[0] == 'e' || dest.back() == 'd')) w = false;
                code.push_back(getREXPrefix(w, reg >= 8, false, false));
                code.push_back(0x0F);
                code.push_back(isByte ? 0xBE : 0xBF);
                code.push_back(getModRM(0, reg & 7, 5));
                code.push_back(static_cast<uint8_t>(offset & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
                code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            }
            return code;
        }

        std::vector<uint8_t> generateSUB(const std::string& dest, const std::string& src) {
            std::vector<uint8_t> code;
            
            if (isRegister(dest) && isRegister(src)) {
                uint8_t destReg = getRegisterCode(dest);
                uint8_t srcReg = getRegisterCode(src);
                
                // REX.W + 29 /r
                code.push_back(getREXPrefix(true, srcReg >= 8, false, destReg >= 8));
                code.push_back(0x29);
                code.push_back(getModRM(3, srcReg & 7, destReg & 7));
            }
            else if (isRegister(dest) && isImmediate(src)) {
                uint8_t destReg = getRegisterCode(dest);
                int64_t imm = std::stoll(src);
                
                // SUB r/m64, imm8
                if (imm >= -128 && imm <= 127) {
                    code.push_back(getREXPrefix(true, false, false, destReg >= 8));
                    code.push_back(0x83);
                    code.push_back(getModRM(3, 5, destReg & 7)); // /5 for SUB
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                }
                // SUB r/m64, imm32
                else {
                    code.push_back(getREXPrefix(true, false, false, destReg >= 8));
                    code.push_back(0x81);
                    code.push_back(getModRM(3, 5, destReg & 7)); // /5 for SUB
                    code.push_back(static_cast<uint8_t>(imm & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
                    code.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
                }
            }
            
            return code;
        }

        std::vector<uint8_t> generateJZ(const std::string& target) {
            // JZ/JE uses 0F 84 for near jump (rel32)
            std::vector<uint8_t> code;
            code.push_back(0x0F);
            code.push_back(0x84);
            
            // Calculate offset if label exists
            int32_t offset = 0;
            if (labels.find(target) != labels.end()) {
                size_t targetInstrIndex = labels[target];
                if (instrToRVA.find(currentInstrIndex) != instrToRVA.end() &&
                    instrToRVA.find(targetInstrIndex) != instrToRVA.end()) {
                    uint32_t currentRVA = instrToRVA[currentInstrIndex];
                    uint32_t targetRVA = instrToRVA[targetInstrIndex];
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 6);
                }
            }
            
            code.push_back(static_cast<uint8_t>(offset & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            
            return code;
        }

        std::vector<uint8_t> generateJNZ(const std::string& target) {
            // JNZ/JNE uses 0F 85 for near jump (rel32)
            std::vector<uint8_t> code;
            code.push_back(0x0F);
            code.push_back(0x85);
            
            // Calculate offset if label exists
            int32_t offset = 0;
            if (labels.find(target) != labels.end()) {
                size_t targetInstrIndex = labels[target];
                if (instrToRVA.find(currentInstrIndex) != instrToRVA.end() &&
                    instrToRVA.find(targetInstrIndex) != instrToRVA.end()) {
                    uint32_t currentRVA = instrToRVA[currentInstrIndex];
                    uint32_t targetRVA = instrToRVA[targetInstrIndex];
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 6);
                }
            }
            
            code.push_back(static_cast<uint8_t>(offset & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            
            return code;
        }
        std::vector<uint8_t> generateJG(const std::string& target) {
            // JG/JNLE uses 0F 8F for near jump (rel32)
            std::vector<uint8_t> code;
            code.push_back(0x0F);
            code.push_back(0x8F);
            
            // Calculate offset if label exists
            int32_t offset = 0;
            if (labels.find(target) != labels.end()) {
                size_t targetInstrIndex = labels[target];
                if (instrToRVA.find(currentInstrIndex) != instrToRVA.end() &&
                    instrToRVA.find(targetInstrIndex) != instrToRVA.end()) {
                    uint32_t currentRVA = instrToRVA[currentInstrIndex];
                    uint32_t targetRVA = instrToRVA[targetInstrIndex];
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 6);
                }
            }
            
            code.push_back(static_cast<uint8_t>(offset & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            
            return code;
        }

        std::vector<uint8_t> generateJL(const std::string& target) {
            // JL/JNGE uses 0F 8C for near jump (rel32)
            std::vector<uint8_t> code;
            code.push_back(0x0F);
            code.push_back(0x8C);
            
            // Calculate offset if label exists
            int32_t offset = 0;
            if (labels.find(target) != labels.end()) {
                size_t targetInstrIndex = labels[target];
                if (instrToRVA.find(currentInstrIndex) != instrToRVA.end() &&
                    instrToRVA.find(targetInstrIndex) != instrToRVA.end()) {
                    uint32_t currentRVA = instrToRVA[currentInstrIndex];
                    uint32_t targetRVA = instrToRVA[targetInstrIndex];
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 6);
                }
            }
            
            code.push_back(static_cast<uint8_t>(offset & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            
            return code;
        }

        std::vector<uint8_t> generateJGE(const std::string& target) {
            // JGE/JNL uses 0F 8D for near jump (rel32)
            std::vector<uint8_t> code;
            code.push_back(0x0F);
            code.push_back(0x8D);
            
            // Calculate offset if label exists
            int32_t offset = 0;
            if (labels.find(target) != labels.end()) {
                size_t targetInstrIndex = labels[target];
                if (instrToRVA.find(currentInstrIndex) != instrToRVA.end() &&
                    instrToRVA.find(targetInstrIndex) != instrToRVA.end()) {
                    uint32_t currentRVA = instrToRVA[currentInstrIndex];
                    uint32_t targetRVA = instrToRVA[targetInstrIndex];
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 6);
                }
            }
            
            code.push_back(static_cast<uint8_t>(offset & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            
            return code;
        }

        std::vector<uint8_t> generateJLE(const std::string& target) {
            // JLE/JNG uses 0F 8E for near jump (rel32)
            std::vector<uint8_t> code;
            code.push_back(0x0F);
            code.push_back(0x8E);
            
            // Calculate offset if label exists
            int32_t offset = 0;
            if (labels.find(target) != labels.end()) {
                size_t targetInstrIndex = labels[target];
                if (instrToRVA.find(currentInstrIndex) != instrToRVA.end() &&
                    instrToRVA.find(targetInstrIndex) != instrToRVA.end()) {
                    uint32_t currentRVA = instrToRVA[currentInstrIndex];
                    uint32_t targetRVA = instrToRVA[targetInstrIndex];
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 6);
                }
            }
            
            code.push_back(static_cast<uint8_t>(offset & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            
            return code;
        }

        std::vector<uint8_t> generateJMP(const std::string& target) {
            // JMP uses E9 for near jump (rel32)
            std::vector<uint8_t> code;
            code.push_back(0xE9);
            
            // Calculate offset if label exists
            int32_t offset = 0;
            if (labels.find(target) != labels.end()) {
                size_t targetInstrIndex = labels[target];
                if (instrToRVA.find(currentInstrIndex) != instrToRVA.end() &&
                    instrToRVA.find(targetInstrIndex) != instrToRVA.end()) {
                    uint32_t currentRVA = instrToRVA[currentInstrIndex];
                    uint32_t targetRVA = instrToRVA[targetInstrIndex];
                    offset = static_cast<int32_t>(targetRVA) - static_cast<int32_t>(currentRVA + 5); // 5 bytes for JMP rel32
                }
            }
            
            code.push_back(static_cast<uint8_t>(offset & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 16) & 0xFF));
            code.push_back(static_cast<uint8_t>((offset >> 24) & 0xFF));
            
            return code;
        }
    };

    // ════════════════════════════════════════════════════════════
    // ✅ المجمع العربي الموحد - مع ImprovedPEBuilder
    // ════════════════════════════════════════════════════════════
    class UnifiedAssembler {
    public:
        enum class Architecture { x86_32, x86_64 };

        struct Statistics {
            size_t instructionsProcessed = 0;
            size_t codeSize = 0;
            size_t exeSize = 0;
            double compilationTime = 0.0;
        };

        UnifiedAssembler(Architecture arch = Architecture::x86_64)
            : currentArch(arch), optimizationsEnabled(true), verboseOutput(false) {
            stats = {};
        }

        // ✅ دالة generate معرّفة هنا
        std::vector<uint8_t> generate(const std::vector<uint8_t>& machineCode) {
            return machineCode;  // مرور مباشر
        }

        // ✅ دالة buildExecutable معرّفة هنا
        bool buildExecutable(const std::string& outputFile) {
            return saveFile({0xC3}, outputFile);  // RET instruction كملف اختبار
        }

        // ✅ دالة getStatistics معرّفة هنا
        std::map<std::string, int> getStatistics() const {
            std::map<std::string, int> statsMap;
            statsMap["instructions"] = static_cast<int>(stats.instructionsProcessed);
            statsMap["codeSize"] = static_cast<int>(stats.codeSize);
            statsMap["exeSize"] = static_cast<int>(stats.exeSize);
            statsMap["time"] = static_cast<int>(stats.compilationTime);
            return statsMap;
        }

        bool assemble(const std::string& assemblyCode, const std::string& outputFile) {
            auto startTime = std::chrono::high_resolution_clock::now();

            if (verboseOutput) {
                std::cout << "🔧 بدء التجميع المحسّن..." << std::endl;
                std::cout << "   المعمارية: " <<
                    (currentArch == Architecture::x86_64 ? "x86-64" : "x86-32") << std::endl;
            }

            try {
                // ✅ Startup code with main call
                std::string startupCode = 
                    "_start:\n"
                    "sub rsp, 40\n"
                    "call main\n"
                    "add rsp, 40\n"
                    "mov rcx, rax\n"
                    "call ExitProcess\n";
                std::string fullAssembly = startupCode + "\n" + assemblyCode;

                // تحليل الكود
                if (verboseOutput) std::cout << "📖 المرحلة 1: تحليل الكود..." << std::endl;
                auto instructions = parser.parse(fullAssembly);
                auto labelMap = parser.getLabels();
                auto dataLabelMap = parser.getDataLabels();
                auto dataBytes = parser.getDataBytes();
                stats.instructionsProcessed = instructions.size();

                if (verboseOutput) {
                    std::cout << "   ✅ تم تحليل " << instructions.size() << " تعليمة" << std::endl;
                    std::cout << "   📍 تم العثور على " << labelMap.size() << " علامة" << std::endl;
                    std::cout << "   📊 تم العثور على " << dataLabelMap.size() << " علامة بيانات" << std::endl;
                    std::cout << "   💾 حجم البيانات: " << dataBytes.size() << " بايت" << std::endl;
                }

                // توليد كود الآلة
                if (verboseOutput) std::cout << "⚙️  المرحلة 2: توليد كود الآلة..." << std::endl;
                // auto imports = parser.getParsedImports();
                auto imports = ImprovedPEBuilder::createMinimalImports();
                // parser.emitShadowSpaceWarnings(instructions, imports);
                auto importMap = ImprovedPEBuilder::getImportMap(imports, 4); // .text, .data, .idata, .reloc
                auto machineCode = codeGen.generate(instructions, labelMap, dataLabelMap, importMap);
                stats.codeSize = machineCode.size();

                if (verboseOutput) {
                    std::cout << "   ✅ تم توليد " << machineCode.size() << " بايت" << std::endl;
                }

                // بناء ملف PE
                if (verboseOutput) std::cout << "🏗️  المرحلة 3: بناء ملف PE المحسّن..." << std::endl;
                ImprovedPEBuilder peBuilder;
                peBuilder.setVerbose(verboseOutput);
                auto exeData = peBuilder.buildExecutable(machineCode, dataBytes, imports);
                stats.exeSize = exeData.size();

                if (verboseOutput) {
                    std::cout << "   ✅ حجم الملف النهائي: " << exeData.size() << " بايت" << std::endl;
                }

                // حفظ الملف
                if (verboseOutput) std::cout << "💾 المرحلة 4: حفظ الملف..." << std::endl;
                bool saved = saveFile(exeData, outputFile);

                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    endTime - startTime);
                stats.compilationTime = static_cast<double>(duration.count());

                if (saved && verboseOutput) {
                    std::cout << "✅ تم التجميع بنجاح!" << std::endl;
                    std::cout << "   ⏱️  الوقت: " << stats.compilationTime
                        << " مللي ثانية" << std::endl;
                }

                return saved;

            }
            catch (const std::exception& e) {
                std::cerr << "❌ خطأ في التجميع: " << e.what() << std::endl;
                return false;
            }
        }

        void setArchitecture(Architecture arch) { currentArch = arch; }
        void enableOptimizations(bool enable) { optimizationsEnabled = enable; }
        void setVerbose(bool verbose) { verboseOutput = verbose; }
        Statistics getStats() const { return stats; }

    private:
        Architecture currentArch;
        bool optimizationsEnabled;
        bool verboseOutput;
        Statistics stats;

        AdvancedParser parser;
        AdvancedCodeGenerator codeGen;

        bool saveFile(const std::vector<uint8_t>& data, const std::string& filename) {
            std::ofstream file(filename, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "❌ فشل فتح الملف: " << filename << std::endl;
                return false;
            }

            file.write(reinterpret_cast<const char*>(data.data()), data.size());
            file.close();

            return file.good();
        }
    };

    using Architecture = UnifiedAssembler::Architecture;

    // ════════════════════════════════════════════════════════════
    // ✅ AssemblerCore للتوافق مع الكود القديم
    // ════════════════════════════════════════════════════════════
    class AssemblerCore {
    private:
        UnifiedAssembler assembler;

    public:
        AssemblerCore(Architecture arch = Architecture::x86_64) : assembler(arch) {}

        bool assemble(const std::string& assemblyCode, const std::string& outputFile) {
            assembler.setVerbose(true);
            return assembler.assemble(assemblyCode, outputFile);
        }

        void setVerbose(bool verbose) {
            assembler.setVerbose(verbose);
        }

        void enableOptimizations(bool enable) {
            assembler.enableOptimizations(enable);
        }

        UnifiedAssembler::Statistics getStats() const {
            return assembler.getStats();
        }
    };

} // namespace ArabicAssembler

#endif // ARABIC_ASSEMBLER_H
#ifndef ARABIC_ASSEMBLER_TRAILING_FIX
#define ARABIC_ASSEMBLER_TRAILING_FIX
        static std::string mapArabicSectionDirective_global(const std::string& line) {
            std::string l = line;
            if (l.find("قسم .بيانات") != std::string::npos || l.find("القسم بيانات") != std::string::npos) {
                return "section .data";
            }
            if (l.find("قسم .نص") != std::string::npos || l.find("القسم نص") != std::string::npos) {
                return "section .text";
            }
            return l;
        }

        static std::string mapArabicDataDirective_global(const std::string& line) {
            if (line.rfind("ضع_بايت", 0) == 0) {
                return std::string("db") + line.substr(std::string("ضع_بايت").size());
            }
            if (line.rfind("ضع_رباعية", 0) == 0) {
                return std::string("dd") + line.substr(std::string("ضع_رباعية").size());
            }
            if (line.rfind("ضع_ثمانية", 0) == 0) {
                return std::string("dq") + line.substr(std::string("ضع_ثمانية").size());
            }
            if (line.rfind("احجز_ثمانية", 0) == 0) {
                return std::string("resq") + line.substr(std::string("احجز_ثمانية").size());
            }
            return line;
        }

        static std::string mapArabicOperand_global(const std::string& op) {
            static const std::map<std::string, std::string> m = {
                {"اطبع", "printf"},
                {"اطبع_سطر", "printf"},
                {"انهاء_العملية", "ExitProcess"},
                {"انهاء", "ExitProcess"},
                {"بايت", "byte"},
                {"كلمة", "word"}
            };
            auto s = op;
            std::string l;
            l.reserve(s.size());
            for (unsigned char c : s) l.push_back(static_cast<char>(std::tolower(c)));
            auto it = m.find(l);
            return it != m.end() ? it->second : op;
        }
        
        static std::string mapArabicRegister_global(const std::string& op) {
            std::string t = op;
            t.erase(0, t.find_first_not_of(" \t"));
            size_t e = t.find_last_not_of(" \t");
            if (e != std::string::npos) t = t.substr(0, e + 1);
            std::string l;
            l.reserve(t.size());
            for (unsigned char c : t) l.push_back(static_cast<char>(std::tolower(c)));
            static const std::map<std::string, std::string> baseMap = {
                {"راكس", "rax"},
                {"مراكس", "rax"},
                {"ربكس", "rbx"},
                {"ركس", "rcx"},
                {"ردكس", "rdx"},
                {"مصدر", "rsi"},
                {"هدف", "rdi"},
                {"قاعدة", "rbp"},
                {"مكدس", "rsp"},
                {"سجل_أ", "rax"},
                {"سجل_ب", "rbx"},
                {"سجل_ج", "rcx"},
                {"سجل_د", "rdx"}
            };
            auto startsWith = [](const std::string& s, const std::string& p){ return s.rfind(p,0) == 0; };
            auto itBase = baseMap.find(l);
            size_t us = l.rfind('_');
            int sizeSel = 0;
            if (us != std::string::npos) {
                std::string suf = l.substr(us + 1);
                std::string latinSz;
                latinSz.reserve(suf.size());
                for (size_t i = 0; i < suf.size(); ++i) {
                    unsigned char c = static_cast<unsigned char>(suf[i]);
                    if (c >= '0' && c <= '9') { latinSz.push_back(static_cast<char>(c)); continue; }
                    if (i + 1 < suf.size() && c == 0xD9) {
                        unsigned char c2 = static_cast<unsigned char>(suf[i+1]);
                        int d = -1;
                        switch (c2) {
                            case 0xA0: d = 0; break;
                            case 0xA1: d = 1; break;
                            case 0xA2: d = 2; break;
                            case 0xA3: d = 3; break;
                            case 0xA4: d = 4; break;
                            case 0xA5: d = 5; break;
                            case 0xA6: d = 6; break;
                            case 0xA7: d = 7; break;
                            case 0xA8: d = 8; break;
                            case 0xA9: d = 9; break;
                        }
                        if (d >= 0) { latinSz.push_back(static_cast<char>('0' + d)); i++; continue; }
                    }
                    if (i + 1 < suf.size() && c == 0xDB) {
                        unsigned char c2 = static_cast<unsigned char>(suf[i+1]);
                        int d = -1;
                        switch (c2) {
                            case 0xB0: d = 0; break;
                            case 0xB1: d = 1; break;
                            case 0xB2: d = 2; break;
                            case 0xB3: d = 3; break;
                            case 0xB4: d = 4; break;
                            case 0xB5: d = 5; break;
                            case 0xB6: d = 6; break;
                            case 0xB7: d = 7; break;
                            case 0xB8: d = 8; break;
                            case 0xB9: d = 9; break;
                        }
                        if (d >= 0) { latinSz.push_back(static_cast<char>('0' + d)); i++; continue; }
                    }
                }
                if (!latinSz.empty()) {
                    if (latinSz == "8") sizeSel = 8;
                    else if (latinSz == "16") sizeSel = 16;
                }
            }
            if (sizeSel != 0) {
                std::string baseName = l.substr(0, us);
                auto itBase2 = baseMap.find(baseName);
                if (itBase2 != baseMap.end()) {
                    std::string b = itBase2->second;
                    if (b == "rax") return sizeSel == 8 ? "al" : "ax";
                    if (b == "rbx") return sizeSel == 8 ? "bl" : "bx";
                    if (b == "rcx") return sizeSel == 8 ? "cl" : "cx";
                    if (b == "rdx") return sizeSel == 8 ? "dl" : "dx";
                    if (b == "rsi") return sizeSel == 8 ? "sil" : "si";
                    if (b == "rdi") return sizeSel == 8 ? "dil" : "di";
                    if (b == "rbp") return sizeSel == 8 ? "bpl" : "bp";
                    if (b == "rsp") return sizeSel == 8 ? "spl" : "sp";
                }
                if (startsWith(baseName, "سجل")) {
                    std::string num = baseName.substr(4);
                    std::string latin;
                    latin.reserve(num.size());
                    for (size_t i = 0; i < num.size(); ++i) {
                        unsigned char c = static_cast<unsigned char>(num[i]);
                        if (c >= '0' && c <= '9') { latin.push_back(static_cast<char>(c)); continue; }
                        if (i + 1 < num.size() && c == 0xD9) {
                            unsigned char c2 = static_cast<unsigned char>(num[i+1]);
                            int d = -1;
                            switch (c2) {
                                case 0xA0: d = 0; break;
                                case 0xA1: d = 1; break;
                                case 0xA2: d = 2; break;
                                case 0xA3: d = 3; break;
                                case 0xA4: d = 4; break;
                                case 0xA5: d = 5; break;
                                case 0xA6: d = 6; break;
                                case 0xA7: d = 7; break;
                                case 0xA8: d = 8; break;
                                case 0xA9: d = 9; break;
                            }
                            if (d >= 0) { latin.push_back(static_cast<char>('0' + d)); i++; continue; }
                        }
                        if (i + 1 < num.size() && c == 0xDB) {
                            unsigned char c2 = static_cast<unsigned char>(num[i+1]);
                            int d = -1;
                            switch (c2) {
                                case 0xB0: d = 0; break;
                                case 0xB1: d = 1; break;
                                case 0xB2: d = 2; break;
                                case 0xB3: d = 3; break;
                                case 0xB4: d = 4; break;
                                case 0xB5: d = 5; break;
                                case 0xB6: d = 6; break;
                                case 0xB7: d = 7; break;
                                case 0xB8: d = 8; break;
                                case 0xB9: d = 9; break;
                            }
                            if (d >= 0) { latin.push_back(static_cast<char>('0' + d)); i++; continue; }
                        }
                        if (c == '_' || c == ' ') continue;
                    }
                    if (!latin.empty()) {
                        int value = 0;
                        for (char dc : latin) { value = value * 10 + (dc - '0'); }
                        if (value >= 8 && value <= 15) {
                            return std::string("r") + latin + (sizeSel == 8 ? "b" : "w");
                        }
                    }
                }
            }
            if (itBase != baseMap.end()) return itBase->second;
            if (startsWith(l, "سجل")) {
                std::string num = l.substr(4);
                std::string latin;
                latin.reserve(num.size());
                for (size_t i = 0; i < num.size(); ++i) {
                    unsigned char c = static_cast<unsigned char>(num[i]);
                    if (c >= '0' && c <= '9') { latin.push_back(static_cast<char>(c)); continue; }
                    if (i + 1 < num.size() && c == 0xD9) {
                        unsigned char c2 = static_cast<unsigned char>(num[i+1]);
                        int d = -1;
                        switch (c2) {
                            case 0xA0: d = 0; break;
                            case 0xA1: d = 1; break;
                            case 0xA2: d = 2; break;
                            case 0xA3: d = 3; break;
                            case 0xA4: d = 4; break;
                            case 0xA5: d = 5; break;
                            case 0xA6: d = 6; break;
                            case 0xA7: d = 7; break;
                            case 0xA8: d = 8; break;
                            case 0xA9: d = 9; break;
                        }
                        if (d >= 0) { latin.push_back(static_cast<char>('0' + d)); i++; continue; }
                    }
                    if (i + 1 < num.size() && c == 0xDB) {
                        unsigned char c2 = static_cast<unsigned char>(num[i+1]);
                        int d = -1;
                        switch (c2) {
                            case 0xB0: d = 0; break;
                            case 0xB1: d = 1; break;
                            case 0xB2: d = 2; break;
                            case 0xB3: d = 3; break;
                            case 0xB4: d = 4; break;
                            case 0xB5: d = 5; break;
                            case 0xB6: d = 6; break;
                            case 0xB7: d = 7; break;
                            case 0xB8: d = 8; break;
                            case 0xB9: d = 9; break;
                        }
                        if (d >= 0) { latin.push_back(static_cast<char>('0' + d)); i++; continue; }
                    }
                    if (c == '_' || c == ' ') continue;
                }
                if (!latin.empty()) {
                    int value = 0;
                    for (char dc : latin) { value = value * 10 + (dc - '0'); }
                    if (value >= 8 && value <= 15) {
                        return std::string("r") + latin;
                    }
                }
            }
            return op;
        }
        
#endif // ARABIC_ASSEMBLER_TRAILING_FIX
