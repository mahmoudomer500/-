#ifndef PE_COMPILER_H
#define PE_COMPILER_H

#include "ImprovedPEBuilder.h"
#include "ImportThunkGenerator.h"
#include "SymbolTable.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>

namespace ArabicAssembler {

    class PECompiler {
    public:
        static bool compile(
            const std::string& outputPath,
            std::vector<uint8_t>& mainCode,
            const std::vector<uint8_t>& dataSection,
            const std::vector<ImprovedPEBuilder::ImportDLL>& imports,
            SymbolTable& symbols,
            RelocationTable& relocations,
            bool verbose = true
        ) {
            if (verbose) std::cout << "🚀 Starting PE Compilation..." << std::endl;

            // 1. Filter imports to only include those actually used in the code
            // This prevents "Entry Point Not Found" errors for unused functions.
            std::set<std::string> usedSymbols;
            for (const auto& reloc : relocations.getRelocations()) {
                usedSymbols.insert(reloc.symbol);
            }

            std::vector<ImprovedPEBuilder::ImportDLL> filteredImports;
            for (const auto& dll : imports) {
                ImprovedPEBuilder::ImportDLL filteredDll;
                filteredDll.name = dll.name;
                for (const auto& func : dll.functions) {
                    if (usedSymbols.count(func.name)) {
                        filteredDll.functions.push_back(func);
                    }
                }
                if (!filteredDll.functions.empty()) {
                    filteredImports.push_back(filteredDll);
                }
            }

            // 1. Calculate IAT Addresses using ImprovedPEBuilder's layout logic
            int numSections = 1; // .text
            if (!dataSection.empty()) numSections++; // .data
            auto iatMap = ImprovedPEBuilder::getImportMap(filteredImports, numSections);

            // 2. Generate Thunks (only for actually used external functions)
            // Convert to ArabicAssembler::ImportDLL for thunk generator
            std::vector<ArabicAssembler::ImportDLL> thunkImports;
            for (const auto& fDll : filteredImports) {
                ArabicAssembler::ImportDLL tDll;
                tDll.name = fDll.name;
                for (const auto& fFunc : fDll.functions) {
                    tDll.functions.push_back(ArabicAssembler::ImportFunction(fFunc.name, fFunc.hint));
                }
                thunkImports.push_back(tDll);
            }

            ImportThunkGenerator thunkGen;
            auto thunks = thunkGen.generateWithMap(thunkImports, 0x1000, iatMap);

            // 3. Flatten Thunks to bytes
            std::vector<uint8_t> thunksCode;
            thunksCode.reserve(1024);
            for(const auto& thunk : thunks) {
                thunksCode.insert(thunksCode.end(), thunk.code.begin(), thunk.code.end());
            }
            size_t thunkSize = thunksCode.size();
            
            if (verbose) std::cout << "   📦 Thunk Size: " << thunkSize << " bytes" << std::endl;

            // 4. Create Unified Text Section (Thunks + MainCode)
            std::vector<uint8_t> textSection;
            textSection.reserve(thunkSize + mainCode.size());
            textSection.insert(textSection.end(), thunksCode.begin(), thunksCode.end());
            textSection.insert(textSection.end(), mainCode.begin(), mainCode.end());

            // #region agent log
            std::ofstream logFile2("d:\\73 و نهاية المرحلة e\\.cursor\\debug.log", std::ios::app);
            if (logFile2.is_open()) {
                auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();
                std::string logEntry = R"({"id":"log_)" + std::to_string(timestamp) + R"(_text_section","timestamp":)" + std::to_string(timestamp) + R"(,"location":"PECompiler.h:56","message":"Text section creation","data":{"thunkSize":)" + std::to_string(thunkSize) + R"(,"mainCodeSize":)" + std::to_string(mainCode.size()) + R"(,"textSectionSize":)" + std::to_string(textSection.size()) + R"(,"thunksCodeSize":)" + std::to_string(thunksCode.size()) + R"(},"sessionId":"debug-session","runId":"initial-run","hypothesisId":"B"})" + "\n";
                logFile2 << logEntry;
                logFile2.close();
            }
            // #endregion

            // 5. Update Symbol Table with correct RVAs
            // Base RVA for .text section is 0x1000
            uint32_t textBaseRVA = 0x1000;
            uint32_t mainCodeStartRVA = textBaseRVA + static_cast<uint32_t>(thunkSize);
            
            // Update import symbols with their thunk RVAs
            uint32_t currentThunkRVA = textBaseRVA;
            for(const auto& dll : filteredImports) { // Changed from thunkImports to filteredImports
                for(const auto& func : dll.functions) {
                    symbols.resolve(func.name, currentThunkRVA);
                    currentThunkRVA += 6;  // Each thunk is 6 bytes
                }
            }
            
            // Update all labels (non-imports) to reflect their RVAs in finalCode
            // Labels are currently offsets in mainCode (0-based)
            // We need to add mainCodeStartRVA to convert them to absolute RVAs
            symbols.adjustNonImportSymbols(mainCodeStartRVA);
            
            if (verbose) std::cout << "   🔧 Resolving relocations..." << std::endl;
            
            // 6. Update Relocation Offsets
            // Relocations from CodeGenerator are relative to mainCode start (0)
            // In textSection, mainCode starts at thunkSize
            for (auto& reloc : relocations.getRelocations()) {
                reloc.offset += static_cast<uint32_t>(thunkSize);
            }
            
            // 7. Resolve Relocations on the Unified Section
            // This modifies textSection in-place
            relocations.resolve(textSection, symbols);

            // 8. Determine Entry Point
            // Entry Point is start of mainCode (after thunks)
            uint32_t entryPoint = 0x1000 + static_cast<uint32_t>(thunkSize);

            // #region agent log
            std::ofstream logFile("d:\\73 و نهاية المرحلة e\\.cursor\\debug.log", std::ios::app);
            if (logFile.is_open()) {
                auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();
                std::string logEntry = R"({"id":"log_)" + std::to_string(timestamp) + R"(_pe_compile","timestamp":)" + std::to_string(timestamp) + R"(,"location":"PECompiler.h:94","message":"Entry point calculation","data":{"entryPoint":)" + std::to_string(entryPoint) + R"(,"textBaseRVA":)" + std::to_string(textBaseRVA) + R"(,"thunkSize":)" + std::to_string(thunkSize) + R"(,"mainCodeSize":)" + std::to_string(mainCode.size()) + R"(,"textSectionSize":)" + std::to_string(textSection.size()) + R"(},"sessionId":"debug-session","runId":"initial-run","hypothesisId":"A"})" + "\n";
                logFile << logEntry;
                logFile.close();
            }
            // #endregion

            if (verbose) std::cout << "   📍 Entry Point: 0x" << std::hex << entryPoint << std::dec << std::endl;

            // 9. Build PE using the Resolved Text Section
            ImprovedPEBuilder builder;
            builder.setVerbose(verbose);
            builder.setEntryPoint(entryPoint);

            // Pass the fully resolved textSection and only the used imports
            auto peData = builder.buildExecutable(textSection, dataSection, filteredImports);

            // 10. Write to Disk
            std::ofstream out(outputPath, std::ios::binary);
            if (!out) {
                std::cerr << "❌ Failed to open output file: " << outputPath << std::endl;
                return false;
            }

            out.write((const char*)peData.data(), peData.size());
            out.close();

            // #region agent log
            std::ofstream logFile3("d:\\73 و نهاية المرحلة e\\.cursor\\debug.log", std::ios::app);
            if (logFile3.is_open()) {
                auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();
                std::string logEntry = R"({"id":"log_)" + std::to_string(timestamp) + R"(_pe_written","timestamp":)" + std::to_string(timestamp) + R"(,"location":"PECompiler.h:113","message":"PE file written to disk","data":{"outputPath":")" + outputPath + R"(","peDataSize":)" + std::to_string(peData.size()) + R"(,"entryPoint":)" + std::to_string(entryPoint) + R"(},"sessionId":"debug-session","runId":"initial-run","hypothesisId":"D"})" + "\n";
                logFile3 << logEntry;
                logFile3.close();
            }
            // #endregion

            if (verbose) std::cout << "✅ PE File Saved: " << outputPath << std::endl;
            return true;
        }
    };

}

#endif // PE_COMPILER_H
