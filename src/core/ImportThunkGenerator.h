// ImportThunkGenerator.h - مولد Import Thunks للمجمع العربي
// يولد JMP thunks لكل دالة مستوردة

#ifndef IMPORT_THUNK_GENERATOR_H
#define IMPORT_THUNK_GENERATOR_H

#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <iostream>
#include <iomanip>

namespace ArabicAssembler {

// ════════════════════════════════════════════════════════════
// 🔧 Import Structures
// ════════════════════════════════════════════════════════════

struct ImportFunction {
    std::string name;
    uint16_t hint;
    
    ImportFunction() : hint(0) {}
    ImportFunction(const std::string& n, uint16_t h = 0) : name(n), hint(h) {}
};

struct ImportDLL {
    std::string name;
    std::vector<ImportFunction> functions;
};


// ════════════════════════════════════════════════════════════
// 🎯 Import Thunk Structure
// ════════════════════════════════════════════════════════════

struct ImportThunk {
    std::string funcName;        // اسم الدالة
    uint32_t thunkRVA;          // RVA للـ thunk في .text
    uint32_t iatEntryRVA;       // RVA للـ IAT entry
    std::vector<uint8_t> code;  // كود الـ thunk (6 bytes)
    
    ImportThunk() : thunkRVA(0), iatEntryRVA(0) {}
    
    ImportThunk(const std::string& name, uint32_t tRVA, uint32_t iatRVA)
        : funcName(name), thunkRVA(tRVA), iatEntryRVA(iatRVA) {}
};

// ════════════════════════════════════════════════════════════
// ⚙️ Import Thunk Generator
// ════════════════════════════════════════════════════════════

class ImportThunkGenerator {
private:
    std::vector<ImportThunk> thunks;
    std::map<std::string, uint32_t> thunkMap;  // funcName → thunkRVA
    
    // ═══ Helper: Append 32-bit value ═══
    void appendInt32(std::vector<uint8_t>& vec, int32_t value) {
        vec.push_back(value & 0xFF);
        vec.push_back((value >> 8) & 0xFF);
        vec.push_back((value >> 16) & 0xFF);
        vec.push_back((value >> 24) & 0xFF);
    }
    
public:
    // ═══ Generate single thunk ═══
    // Format: FF 25 [disp32]  (jmp qword [rip + disp32])
    ImportThunk generateThunk(const std::string& funcName,
                             uint32_t thunkRVA,
                             uint32_t iatEntryRVA) {
        ImportThunk thunk(funcName, thunkRVA, iatEntryRVA);
        
        // JMP [rip + disp32]
        // Opcode: FF 25
        thunk.code.push_back(0xFF);
        thunk.code.push_back(0x25);
        
        // Calculate displacement
        // disp = iatEntryRVA - (thunkRVA + 6)
        // 6 = size of this instruction (2 bytes opcode + 4 bytes disp)
        int32_t disp = static_cast<int32_t>(iatEntryRVA) - 
                      static_cast<int32_t>(thunkRVA + 6);
        
        appendInt32(thunk.code, disp);
        
        return thunk;
    }
    
    // ═══ Generate all thunks for imports (Linear) ═══
    std::vector<ImportThunk> generateAll(
        const std::vector<ImportDLL>& imports,
        uint32_t thunkBaseRVA,
        uint32_t iatBaseRVA) {
        
        thunks.clear();
        thunkMap.clear();
        
        uint32_t currentThunkRVA = thunkBaseRVA;
        uint32_t currentIatRVA = iatBaseRVA;
        
        for (const auto& dll : imports) {
            for (const auto& func : dll.functions) {
                // Generate thunk
                ImportThunk thunk = generateThunk(
                    func.name,
                    currentThunkRVA,
                    currentIatRVA
                );
                
                thunks.push_back(thunk);
                thunkMap[func.name] = currentThunkRVA;
                
                // Move to next thunk (6 bytes each)
                currentThunkRVA += 6;
                
                // Move to next IAT entry (8 bytes each for 64-bit)
                currentIatRVA += 8;
            }
        }
        
        return thunks;
    }

    // ═══ Generate thunks using a pre-calculated IAT map (from ImprovedPEBuilder) ═══
    std::vector<ImportThunk> generateWithMap(
        const std::vector<ImportDLL>& imports,
        uint32_t baseThunkRVA,
        const std::map<std::string, uint32_t>& iatMap
    ) {
        thunks.clear();
        thunkMap.clear();
        uint32_t currentThunkRVA = baseThunkRVA;

        for (const auto& dll : imports) {
            for (const auto& func : dll.functions) {
                if (iatMap.find(func.name) == iatMap.end()) {
                    std::cerr << "Error: IAT address not found for " << func.name << std::endl;
                    continue;
                }

                uint32_t iatRVA = iatMap.at(func.name);
                
                ImportThunk thunk = generateThunk(
                    func.name,
                    currentThunkRVA,
                    iatRVA
                );

                thunks.push_back(thunk);
                thunkMap[func.name] = currentThunkRVA;
                currentThunkRVA += 6;
            }
        }
        return thunks;
    }
    
    // ═══ Get all generated thunks ═══
    const std::vector<ImportThunk>& getThunks() const {
        return thunks;
    }
    
    // ═══ Look up thunk RVA by function name ═══
    uint32_t getThunkRVA(const std::string& funcName) const {
        auto it = thunkMap.find(funcName);
        if (it != thunkMap.end()) {
            return it->second;
        }
        return 0;  // Not found
    }
    
    // ═══ Get total thunk section size ═══
    uint32_t getTotalSize() const {
        return static_cast<uint32_t>(thunks.size() * 6);  // 6 bytes per thunk
    }
    
    // ═══ Get all thunk code as one blob ═══
    std::vector<uint8_t> getAllThunkCode() const {
        std::vector<uint8_t> allCode;
        for (const auto& thunk : thunks) {
            allCode.insert(allCode.end(), thunk.code.begin(), thunk.code.end());
        }
        return allCode;
    }
    
    // ═══ Debug: Print thunk info ═══
    void printThunks() const {
        std::cout << "═══ Import Thunks ═══" << std::endl;
        for (const auto& thunk : thunks) {
            std::cout << "  " << thunk.funcName << ":" << std::endl;
            std::cout << "    Thunk RVA: 0x" << std::hex << thunk.thunkRVA << std::endl;
            std::cout << "    IAT RVA:   0x" << thunk.iatEntryRVA << std::endl;
            std::cout << "    Code: ";
            for (uint8_t byte : thunk.code) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') 
                         << static_cast<int>(byte) << " ";
            }
            std::cout << std::dec << std::endl;
        }
    }
};

} // namespace ArabicAssembler

#endif // IMPORT_THUNK_GENERATOR_H
