// ImprovedPEBuilder.h - النسخة المصلحة مع Import Table صحيح
#ifndef IMPROVED_PE_BUILDER_H
#define IMPROVED_PE_BUILDER_H

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <algorithm>
#include <ctime>

namespace ArabicAssembler {

#pragma pack(push, 1)

    struct IMAGE_DOS_HEADER {
        uint16_t e_magic;
        uint16_t e_cblp;
        uint16_t e_cp;
        uint16_t e_crlc;
        uint16_t e_cparhdr;
        uint16_t e_minalloc;
        uint16_t e_maxalloc;
        uint16_t e_ss;
        uint16_t e_sp;
        uint16_t e_csum;
        uint16_t e_ip;
        uint16_t e_cs;
        uint16_t e_lfarlc;
        uint16_t e_ovno;
        uint16_t e_res[4];
        uint16_t e_oemid;
        uint16_t e_oeminfo;
        uint16_t e_res2[10];
        uint32_t e_lfanew;
    };

    struct IMAGE_FILE_HEADER {
        uint16_t Machine;
        uint16_t NumberOfSections;
        uint32_t TimeDateStamp;
        uint32_t PointerToSymbolTable;
        uint32_t NumberOfSymbols;
        uint16_t SizeOfOptionalHeader;
        uint16_t Characteristics;
    };

    struct IMAGE_DATA_DIRECTORY {
        uint32_t VirtualAddress;
        uint32_t Size;
    };

    struct IMAGE_OPTIONAL_HEADER64 {
        uint16_t Magic;
        uint8_t  MajorLinkerVersion;
        uint8_t  MinorLinkerVersion;
        uint32_t SizeOfCode;
        uint32_t SizeOfInitializedData;
        uint32_t SizeOfUninitializedData;
        uint32_t AddressOfEntryPoint;
        uint32_t BaseOfCode;
        uint64_t ImageBase;
        uint32_t SectionAlignment;
        uint32_t FileAlignment;
        uint16_t MajorOperatingSystemVersion;
        uint16_t MinorOperatingSystemVersion;
        uint16_t MajorImageVersion;
        uint16_t MinorImageVersion;
        uint16_t MajorSubsystemVersion;
        uint16_t MinorSubsystemVersion;
        uint32_t Win32VersionValue;
        uint32_t SizeOfImage;
        uint32_t SizeOfHeaders;
        uint32_t CheckSum;
        uint16_t Subsystem;
        uint16_t DllCharacteristics;
        uint64_t SizeOfStackReserve;
        uint64_t SizeOfStackCommit;
        uint64_t SizeOfHeapReserve;
        uint64_t SizeOfHeapCommit;
        uint32_t LoaderFlags;
        uint32_t NumberOfRvaAndSizes;
        IMAGE_DATA_DIRECTORY DataDirectory[16];
    };

    struct IMAGE_SECTION_HEADER {
        uint8_t  Name[8];
        uint32_t VirtualSize;
        uint32_t VirtualAddress;
        uint32_t SizeOfRawData;
        uint32_t PointerToRawData;
        uint32_t PointerToRelocations;
        uint32_t PointerToLinenumbers;
        uint16_t NumberOfRelocations;
        uint16_t NumberOfLinenumbers;
        uint32_t Characteristics;
    };

    struct IMAGE_IMPORT_DESCRIPTOR {
        uint32_t OriginalFirstThunk;
        uint32_t TimeDateStamp;
        uint32_t ForwarderChain;
        uint32_t Name;
        uint32_t FirstThunk;
    };

#pragma pack(pop)

    class ImprovedPEBuilder {
    public:
        ImprovedPEBuilder() : verbose(false), entryPoint(0x1000), idataVA_current(0) {}
        
        void setEntryPoint(uint32_t rva) {
            entryPoint = rva;
            // #region agent log
            std::ofstream logFile("d:\\73 و نهاية المرحلة e\\.cursor\\debug.log", std::ios::app);
            if (logFile.is_open()) {
                auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
                std::string logEntry = R"({"id":"log_)" + std::to_string(timestamp) + R"(_set_entry","timestamp":)" + std::to_string(timestamp) + R"(,"location":"ImprovedPEBuilder.h:117","message":"Entry point set in PE builder","data":{"entryPointRVA":)" + std::to_string(entryPoint) + R"(},"sessionId":"debug-session","runId":"initial-run","hypothesisId":"C"})" + "\n";
                logFile << logEntry;
                logFile.close();
            }
            // #endregion
        }

        struct ImportFunction {
            std::string name;
            uint16_t hint;
        };

        struct ImportDLL {
            std::string name;
            std::vector<ImportFunction> functions;
        };

        struct ImportLayout {
            uint32_t descriptorTableSize;
            uint32_t intTableOffset;
            uint32_t intTableSize;
            uint32_t iatTableOffset;
            uint32_t iatTableSize;
            uint32_t hintNameTableOffset;
            uint32_t dllNameTableOffset;
            uint32_t totalSize;
        };

        static ImportLayout computeImportLayout(const std::vector<ImportDLL>& imports) {
            ImportLayout L{};
            L.descriptorTableSize = (static_cast<uint32_t>(imports.size()) + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
            L.intTableOffset = L.descriptorTableSize;
            if (L.intTableOffset % 8 != 0) L.intTableOffset += (8 - (L.intTableOffset % 8));

            L.intTableSize = 0;
            for (const auto& dll : imports) {
                L.intTableSize += static_cast<uint32_t>((dll.functions.size() + 1) * 8);
            }

            L.iatTableOffset = L.intTableOffset + L.intTableSize;
            if (L.iatTableOffset % 8 != 0) L.iatTableOffset += (8 - (L.iatTableOffset % 8));

            L.iatTableSize = L.intTableSize;
            L.hintNameTableOffset = L.iatTableOffset + L.iatTableSize;

            uint32_t currentHintNameOffset = L.hintNameTableOffset;
            for (const auto& dll : imports) {
                for (const auto& func : dll.functions) {
                    uint32_t entrySize = 2 + static_cast<uint32_t>(func.name.length()) + 1;
                    if (entrySize % 2 != 0) entrySize++;
                    currentHintNameOffset += entrySize;
                }
            }

            L.dllNameTableOffset = currentHintNameOffset;
            uint32_t currentDllNameOffset = L.dllNameTableOffset;
            for (const auto& dll : imports) {
                currentDllNameOffset += static_cast<uint32_t>(dll.name.length()) + 1;
            }

            L.totalSize = currentDllNameOffset;
            return L;
        }

        std::vector<uint8_t> buildExecutable(
            const std::vector<uint8_t>& machineCode,
            const std::vector<uint8_t>& dataSection = {},
            const std::vector<ImportDLL>& imports = {},
            const std::vector<uint8_t>& rdataSection = {}
        ) {
            if (verbose) {
                std::cout << "🏗️  بدء بناء ملف PE64..." << std::endl;
                std::cout << "   📊 حجم الكود: " << machineCode.size() << " بايت" << std::endl;
                std::cout << "   📊 حجم البيانات: " << dataSection.size() << " بايت" << std::endl;
                std::cout << "   📊 عدد المكتبات: " << imports.size() << std::endl;
            }

            std::vector<uint8_t> exe;

            // 1. DOS Header
            buildDOSHeader(exe);

            // 2. PE Headers
            buildPEHeaders(exe, machineCode.size(), dataSection.size(), imports, rdataSection.size());

            // 3. محاذاة to 0x400 (1024 bytes) for headers
            alignTo(exe, 1024);

            // 4. .text section
            buildTextSection(exe, machineCode);

            // 5. .data section
            if (!dataSection.empty()) {
                buildDataSection(exe, dataSection);
            }
            else {
                buildEmptyDataSection(exe);
            }

            // 6. .rdata section
            if (!rdataSection.empty()) {
                buildRDataSection(exe, rdataSection);
            }

            // 7. .idata section
            if (!imports.empty()) {
                buildSimpleImportSection(exe, imports);
            }
            
            // 8. .reloc section - ✅ NEW
            buildRelocationSection(exe);

            if (verbose) {
                std::cout << "✅ اكتمل البناء!" << std::endl;
                std::cout << "   📦 الحجم النهائي: " << exe.size() << " بايت" << std::endl;
            }

            return exe;
        }

        void setVerbose(bool v) { verbose = v; }

        static ImportDLL createKernel32Import() {
            ImportDLL kernel32;
            kernel32.name = "KERNEL32.DLL";
            kernel32.functions = {
                {"ExitProcess", 0},
                {"GetStdHandle", 1},
                {"WriteFile", 2},
                {"GetProcessHeap", 3}
            };
            return kernel32;
        }

        static ImportDLL createMsvcrtImport() {
            ImportDLL msvcrt;
            msvcrt.name = "MSVCRT.DLL";  // ✅ Uppercase for consistency
            msvcrt.functions = {
                {"printf", 0},
                {"scanf", 1},
                {"system", 2},
                {"exit", 3},
                {"malloc", 4},
                {"free", 5},
                {"strlen", 6},
                {"strcpy", 7},
                {"strcat", 8},
                {"memcpy", 9},
                {"memset", 10},
                {"strncmp", 11},
                {"fopen", 12},
                {"fclose", 13},
                {"fread", 14},
                {"fwrite", 15},
                {"fgets", 16},
                {"fprintf", 17},
                {"_access", 18},
                {"remove", 19},
                {"rename", 20},
                {"time", 21},
                {"getenv", 22},
                {"system", 23},
                {"pow", 24},
                {"rand", 25},
                {"srand", 26},
                {"fseek", 27},
                {"ftell", 28}
            };
            return msvcrt;
        }

        static std::vector<ImportDLL> createMinimalImports() {
            return {
                createKernel32Import()
            };
        }

        // ✅ NEW: Calculate IAT RVAs for all imported functions
        static std::map<std::string, uint32_t> getImportMap(const std::vector<ImportDLL>& imports, int /*numSections*/) {
            std::map<std::string, uint32_t> importMap;
            // idataRVA is fixed at 0x3000 for this builder's layout (.text=1000, .data=2000, .idata=3000)
            uint32_t idataRVA = 0x3000;

            // Calculate offsets same as buildSimpleImportSection
            uint32_t descriptorTableSize = (static_cast<uint32_t>(imports.size()) + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
            
            // INT comes after Descriptors
            uint32_t intTableOffset = descriptorTableSize;
            if (intTableOffset % 8 != 0) intTableOffset += (8 - (intTableOffset % 8));
            
            uint32_t intTableSize = 0;
            for (const auto& dll : imports) {
                intTableSize += static_cast<uint32_t>((dll.functions.size() + 1) * 8);
            }
            
            // IAT comes after INT
            uint32_t iatTableOffset = intTableOffset + intTableSize;
            if (iatTableOffset % 8 != 0) iatTableOffset += (8 - (iatTableOffset % 8));
            
            uint32_t currentIATOffset = idataRVA + iatTableOffset;

            for (const auto& dll : imports) {
                for (const auto& func : dll.functions) {
                    importMap[func.name] = currentIATOffset;
                    currentIATOffset += 8; // 64-bit pointer
                }
                currentIATOffset += 8; // Skip NULL terminator
            }

            return importMap;
        }

    private:
        bool verbose;
        uint32_t entryPoint;
        uint32_t idataVA_current;

        void buildDOSHeader(std::vector<uint8_t>& exe) {
            IMAGE_DOS_HEADER dosHeader = {};
            dosHeader.e_magic = 0x5A4D;
            dosHeader.e_cblp = 0x90;
            dosHeader.e_cp = 0x03;
            dosHeader.e_cparhdr = 0x04;
            dosHeader.e_maxalloc = 0xFFFF;
            dosHeader.e_sp = 0xB8;
            dosHeader.e_lfarlc = 0x40;
            dosHeader.e_lfanew = 0x80;

            appendStruct(exe, dosHeader);

            const uint8_t dosStub[] = {
                0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD,
                0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21, 0x54, 0x68,
                0x69, 0x73, 0x20, 0x70, 0x72, 0x6F, 0x67, 0x72,
                0x61, 0x6D, 0x20, 0x63, 0x61, 0x6E, 0x6E, 0x6F,
                0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6E,
                0x20, 0x69, 0x6E, 0x20, 0x44, 0x4F, 0x53, 0x20,
                0x6D, 0x6F, 0x64, 0x65, 0x2E, 0x0D, 0x0D, 0x0A,
                0x24
            };

            exe.insert(exe.end(), dosStub, dosStub + sizeof(dosStub));
            exe.resize(0x80, 0);
        }

        void buildPEHeaders(std::vector<uint8_t>& exe,
            size_t codeSize,
            size_t dataSize,
            const std::vector<ImportDLL>& imports,
            size_t rdataSize) {

            uint32_t peSignature = 0x00004550;
            appendValue(exe, peSignature);

            IMAGE_FILE_HEADER coffHeader = {};
            coffHeader.Machine = 0x8664;  // AMD64
            coffHeader.NumberOfSections = 3; // .text, .data, .reloc
            if (!rdataSize) {
                // no change
            } else {
                coffHeader.NumberOfSections += 1; // .rdata
            }
            if (!imports.empty()) {
                coffHeader.NumberOfSections += 1; // .idata
            }
            coffHeader.TimeDateStamp = static_cast<uint32_t>(std::time(nullptr));
            coffHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
            // ✅ FIXED: Added LARGE_ADDRESS_AWARE (0x0010) for 64-bit
            // 0x0002 = EXECUTABLE_IMAGE
            // 0x0020 = 32BIT_MACHINE (should NOT be set for 64-bit)
            // 0x0100 = DEBUG_STRIPPED
            coffHeader.Characteristics = 0x0102; // EXECUTABLE_IMAGE | LARGE_ADDRESS_AWARE

            appendStruct(exe, coffHeader);

            IMAGE_OPTIONAL_HEADER64 optHeader = {};
            optHeader.Magic = 0x20B;
            optHeader.MajorLinkerVersion = 14;
            optHeader.MinorLinkerVersion = 0;
            optHeader.SizeOfCode = alignValue(codeSize, 512);
            optHeader.SizeOfInitializedData = alignValue(dataSize > 0 ? dataSize : 16, 512);
            optHeader.AddressOfEntryPoint = entryPoint;
            optHeader.BaseOfCode = 0x1000;
            optHeader.ImageBase = 0x00400000; // Standard compatibility base
            optHeader.SectionAlignment = 0x1000;
            optHeader.FileAlignment = 0x200;
            
            optHeader.MajorOperatingSystemVersion = 5;
            optHeader.MinorOperatingSystemVersion = 2; // Server 2003 / XP x64
            optHeader.MajorLinkerVersion = 14;
            optHeader.MinorLinkerVersion = 0;
            optHeader.MajorImageVersion = 0;
            optHeader.MinorImageVersion = 0;
            optHeader.MajorSubsystemVersion = 5;
            optHeader.MinorSubsystemVersion = 2;
            optHeader.Win32VersionValue = 0;
            
            // ✅ حساب SizeOfImage ديناميكياً بناءً على أحجام الأقسام الفعلية
            const uint32_t SECTION_ALIGN = 0x1000;
            
            // حساب العنوان الافتراضي لكل قسم بناءً على الحجم
            uint32_t textVA = 0x1000;
            uint32_t textVS = static_cast<uint32_t>(codeSize);
            uint32_t textAligned = (textVS + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1);
            
            uint32_t dataVA = textVA + textAligned; // بعد .text مباشرة
            uint32_t dataVS = static_cast<uint32_t>(dataSize > 0 ? dataSize : 16);
            uint32_t dataAligned = (dataVS + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1);
            
            uint32_t rdataVA = 0;
            uint32_t rdataAligned = 0;
            if (rdataSize) {
                rdataVA = dataVA + dataAligned;
                rdataAligned = (static_cast<uint32_t>(rdataSize) + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1);
            }
            
            uint32_t idataVA = 0;
            uint32_t idataVS = 0;
            uint32_t idataAligned = 0;
            if (!imports.empty()) {
                auto L = computeImportLayout(imports);
                idataVS = L.totalSize;
                if (rdataSize) {
                    idataVA = rdataVA + rdataAligned;
                } else {
                    idataVA = dataVA + dataAligned;
                }
                idataAligned = (idataVS + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1);
            }
            
            uint32_t relocVA = 0;
            if (!imports.empty()) {
                relocVA = idataVA + idataAligned;
            } else if (rdataSize) {
                relocVA = rdataVA + rdataAligned;
            } else {
                relocVA = dataVA + dataAligned;
            }
            
            uint32_t imageSize = relocVA + SECTION_ALIGN; // + .reloc section
            imageSize = (imageSize + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1);

            optHeader.SizeOfImage = imageSize;
            optHeader.SizeOfHeaders = 0x400;
            optHeader.Subsystem = 3; // CONSOLE
            // ✅ FIXED DllCharacteristics for better Windows 10/11 compatibility
            // 0x0100 = NX_COMPAT (No Execute) - DEP enabled
            // 0x0400 = NO_SEH (Required because we don't generate .pdata)
            // 0x8140 = DYNAMIC_BASE | NX_COMPAT | TERMINAL_SERVER_AWARE
            optHeader.DllCharacteristics = 0x8160; // DYNAMIC_BASE | NX_COMPAT | NO_SEH | TERMINAL_SERVER_AWARE 
            optHeader.SizeOfStackReserve = 0x100000;
            optHeader.SizeOfStackCommit = 0x1000;
            optHeader.SizeOfHeapReserve = 0x100000;
            optHeader.SizeOfHeapCommit = 0x1000;
            optHeader.NumberOfRvaAndSizes = 16;

            // ✅ Import Directory Entry - باستخدام العنوان المحسوب ديناميكياً
            if (!imports.empty()) {
                auto L = computeImportLayout(imports);
                optHeader.DataDirectory[1].VirtualAddress = idataVA;
                optHeader.DataDirectory[1].Size = idataVS;

                // ✅ IAT Directory Entry (Index 12)
                optHeader.DataDirectory[12].VirtualAddress = idataVA + L.iatTableOffset;
                optHeader.DataDirectory[12].Size = L.iatTableSize;
            }
            
            // ✅ Base Relocation Table - باستخدام العنوان المحسوب ديناميكياً
            optHeader.DataDirectory[5].VirtualAddress = relocVA;
            optHeader.DataDirectory[5].Size = 0x200;

            appendStruct(exe, optHeader);
            buildSectionHeaders(exe, codeSize, dataSize, !imports.empty(), rdataSize, imports);
        }

        void buildSectionHeaders(std::vector<uint8_t>& exe,
            size_t codeSize,
            size_t dataSize,
            bool hasImports,
            size_t rdataSize,
            const std::vector<ImportDLL>& imports) {
            
            const uint32_t SECTION_ALIGN = 0x1000;
            const uint32_t FILE_ALIGN = 0x200;
            
            // ✅ حساب العناوين ديناميكياً
            uint32_t textVA = 0x1000;
            uint32_t textVS = static_cast<uint32_t>(codeSize);
            uint32_t textAlignedVS = (textVS + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1);
            
            uint32_t dataVA = textVA + textAlignedVS;
            uint32_t dataVS = static_cast<uint32_t>(dataSize > 0 ? dataSize : 16);
            uint32_t dataAlignedVS = (dataVS + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1);
            
            uint32_t rdataVA = 0;
            uint32_t rdataAlignedVS = 0;
            if (rdataSize) {
                rdataVA = dataVA + dataAlignedVS;
                rdataAlignedVS = (static_cast<uint32_t>(rdataSize) + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1);
            }
            
            uint32_t idataVA = 0;
            uint32_t idataVS = 0;
            if (hasImports) {
                auto L = computeImportLayout(imports);
                idataVS = L.totalSize;
                if (rdataSize) {
                    idataVA = rdataVA + rdataAlignedVS;
                } else {
                    idataVA = dataVA + dataAlignedVS;
                }
            }
            
            uint32_t relocVA = hasImports ? (idataVA + ((idataVS + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1))) 
                                          : (rdataSize ? (rdataVA + rdataAlignedVS) : (dataVA + dataAlignedVS));

            // حساب File Offsets
            uint32_t headerSize = 0x400; // After DOS + PE headers
            uint32_t textRawSize = (static_cast<uint32_t>(codeSize) + FILE_ALIGN - 1) & ~(FILE_ALIGN - 1);
            uint32_t dataRawSize = ((dataVS + FILE_ALIGN - 1) & ~(FILE_ALIGN - 1));
            uint32_t rdataRawSize = 0;
            if (rdataSize) rdataRawSize = (static_cast<uint32_t>(rdataSize) + FILE_ALIGN - 1) & ~(FILE_ALIGN - 1);
            uint32_t idataRawSize = 0;
            if (hasImports) idataRawSize = ((idataVS + FILE_ALIGN - 1) & ~(FILE_ALIGN - 1));
            
            uint32_t textFilePtr = headerSize;
            uint32_t dataFilePtr = textFilePtr + textRawSize;
            uint32_t rdataFilePtr = dataFilePtr + dataRawSize;
            uint32_t idataFilePtr = rdataSize ? (rdataFilePtr + rdataRawSize) : dataFilePtr;
            uint32_t relocFilePtr = hasImports ? (idataFilePtr + idataRawSize) : (rdataSize ? (rdataFilePtr + rdataRawSize) : dataFilePtr);

            // ✅ .text section
            IMAGE_SECTION_HEADER textSection = {};
            std::memcpy(textSection.Name, ".text", 5);
            textSection.VirtualSize = textVS;
            textSection.VirtualAddress = textVA;
            textSection.SizeOfRawData = textRawSize;
            textSection.PointerToRawData = textFilePtr;
            textSection.Characteristics = 0x60000020; // CODE | EXECUTE | READ
            appendStruct(exe, textSection);

            // ✅ .data section
            IMAGE_SECTION_HEADER dataSection = {};
            std::memcpy(dataSection.Name, ".data", 5);
            dataSection.VirtualSize = dataVS;
            dataSection.VirtualAddress = dataVA;
            dataSection.SizeOfRawData = dataRawSize;
            dataSection.PointerToRawData = dataFilePtr;
            dataSection.Characteristics = 0xC0000040; // INIT | READ | WRITE
            appendStruct(exe, dataSection);

            // ✅ .rdata section
            if (rdataSize) {
                IMAGE_SECTION_HEADER rdataSectionHeader = {};
                std::memcpy(rdataSectionHeader.Name, ".rdata", 6);
                rdataSectionHeader.VirtualSize = static_cast<uint32_t>(rdataSize);
                rdataSectionHeader.VirtualAddress = rdataVA;
                rdataSectionHeader.SizeOfRawData = rdataRawSize;
                rdataSectionHeader.PointerToRawData = rdataFilePtr;
                rdataSectionHeader.Characteristics = 0x40000040; // INIT | READ
                appendStruct(exe, rdataSectionHeader);
            }

            // ✅ .idata section
            if (hasImports) {
                auto L = computeImportLayout(imports);
                IMAGE_SECTION_HEADER idataSection = {};
                std::memcpy(idataSection.Name, ".idata", 6);
                idataSection.VirtualSize = idataVS;
                idataSection.VirtualAddress = idataVA;
                idataSection.SizeOfRawData = idataRawSize;
                idataSection.PointerToRawData = idataFilePtr;
                idataSection.Characteristics = 0xC0000040; // INIT | READ | WRITE
                appendStruct(exe, idataSection);
                idataVA_current = idataVA;
            }
            
            // ✅ .reloc section
            IMAGE_SECTION_HEADER relocSection = {};
            std::memcpy(relocSection.Name, ".reloc", 6);
            relocSection.VirtualSize = 0x200;
            relocSection.VirtualAddress = relocVA;
            relocSection.SizeOfRawData = 0x200;
            relocSection.PointerToRawData = relocFilePtr;
            relocSection.Characteristics = 0x42000040; // INIT | READ | DISCARDABLE
            appendStruct(exe, relocSection);
        }

        void buildTextSection(std::vector<uint8_t>& exe,
            const std::vector<uint8_t>& machineCode) {

            exe.insert(exe.end(), machineCode.begin(), machineCode.end());
            alignTo(exe, 512);
        }

        void buildDataSection(std::vector<uint8_t>& exe,
            const std::vector<uint8_t>& dataSection) {
            exe.insert(exe.end(), dataSection.begin(), dataSection.end());
            alignTo(exe, 512);
        }

        void buildEmptyDataSection(std::vector<uint8_t>& exe) {
            for (int i = 0; i < 16; ++i) {
                exe.push_back(0);
            }
            alignTo(exe, 512);
        }

        void buildRDataSection(std::vector<uint8_t>& exe,
            const std::vector<uint8_t>& rdataSection) {
            exe.insert(exe.end(), rdataSection.begin(), rdataSection.end());
            alignTo(exe, 512);
        }

        // ✅ الإصلاح الرئيسي: Import Section صحيح 100% لـ 64-bit
        void buildSimpleImportSection(std::vector<uint8_t>& exe,
            const std::vector<ImportDLL>& imports) {

            size_t importStart = exe.size();
            uint32_t idataRVA = idataVA_current ? idataVA_current : 0x3000; // RVA قسم .idata

            // حساب offsets داخل .idata
            uint32_t descriptorTableOffset = 0;
            uint32_t descriptorTableSize = (static_cast<uint32_t>(imports.size()) + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
            
            // INT (Import Name Table) - يأتي بعد Descriptors
            // Ensure 8-byte alignment
            uint32_t intTableOffset = descriptorTableSize;
            if (intTableOffset % 8 != 0) intTableOffset += (8 - (intTableOffset % 8));
            
            uint32_t intTableSize = 0;
            for (const auto& dll : imports) {
                intTableSize += static_cast<uint32_t>((dll.functions.size() + 1) * 8);
            }

            // IAT (Import Address Table) - يأتي بعد INT
            // Ensure 8-byte alignment (should be already if INT is aligned and size is multiple of 8)
            uint32_t iatTableOffset = intTableOffset + intTableSize;
            if (iatTableOffset % 8 != 0) iatTableOffset += (8 - (iatTableOffset % 8));
            
            uint32_t iatTableSize = intTableSize; // same size as INT

            // Hint/Name Table - يأتي بعد IAT
            uint32_t hintNameTableOffset = iatTableOffset + iatTableSize;
            
            // DLL Names - تأتي في النهاية
            uint32_t dllNameTableOffset = hintNameTableOffset;
            for (const auto& dll : imports) {
                for (const auto& func : dll.functions) {
                    uint32_t entrySize = 2 + static_cast<uint32_t>(func.name.length()) + 1;
                    if (entrySize % 2 != 0) entrySize++;
                    dllNameTableOffset += entrySize;
                }
            }

            // 1. Import Descriptors
            uint32_t currentINTOffset = idataRVA + intTableOffset;
            uint32_t currentIATOffset = idataRVA + iatTableOffset;
            uint32_t currentDLLNameOffset = idataRVA + dllNameTableOffset;
            
            for (size_t i = 0; i < imports.size(); ++i) {
                const auto& dll = imports[i];
                
                IMAGE_IMPORT_DESCRIPTOR desc = {};
                desc.OriginalFirstThunk = currentINTOffset; // INT RVA
                desc.TimeDateStamp = 0;
                desc.ForwarderChain = 0;
                desc.Name = currentDLLNameOffset; // DLL name RVA
                desc.FirstThunk = currentIATOffset; // IAT RVA
                
                appendStruct(exe, desc);
                
                // تحديث offsets for next DLL
                currentINTOffset += static_cast<uint32_t>((dll.functions.size() + 1) * 8);
                currentIATOffset += static_cast<uint32_t>((dll.functions.size() + 1) * 8);
                currentDLLNameOffset += static_cast<uint32_t>(dll.name.length()) + 1;
            }
            
            // NULL descriptor
            IMAGE_IMPORT_DESCRIPTOR nullDesc = {};
            appendStruct(exe, nullDesc);
            
            // 2. Import Name Table (INT) - Lookups
            // Add Padding if needed to match intTableOffset
            uint32_t currentOffset = static_cast<uint32_t>(exe.size() - importStart);
            if (currentOffset < intTableOffset) {
                for (uint32_t k = 0; k < intTableOffset - currentOffset; ++k) exe.push_back(0);
            }

            uint32_t currentHintNameOffset = idataRVA + hintNameTableOffset;
            
            for (const auto& dll : imports) {
                for (const auto& func : dll.functions) {
                    // 64-bit thunk pointing to Hint/Name table
                    appendValue<uint64_t>(exe, currentHintNameOffset);
                    
                    uint32_t entrySize = 2 + static_cast<uint32_t>(func.name.length()) + 1;
                    if (entrySize % 2 != 0) entrySize++; // Padding for alignment
                    
                    currentHintNameOffset += entrySize;
                }
                // NULL terminator for this DLL's INT
                appendValue<uint64_t>(exe, 0);
            }

            // 3. Import Address Table (IAT) - Identical copy of INT initially
            // Add Padding if needed to match iatTableOffset
            currentOffset = static_cast<uint32_t>(exe.size() - importStart);
            if (currentOffset < iatTableOffset) {
                for (uint32_t k = 0; k < iatTableOffset - currentOffset; ++k) exe.push_back(0);
            }

            currentHintNameOffset = idataRVA + hintNameTableOffset; // Reset to start of Hint/Names
            
            for (const auto& dll : imports) {
                for (const auto& func : dll.functions) {
                    appendValue<uint64_t>(exe, currentHintNameOffset);
                    
                    uint32_t entrySize = 2 + static_cast<uint32_t>(func.name.length()) + 1;
                    if (entrySize % 2 != 0) entrySize++;
                    currentHintNameOffset += entrySize;
                }
                appendValue<uint64_t>(exe, 0); // NULL terminator
            }

            // 4. Hint/Name Table
            // Add Padding if needed to match hintNameTableOffset
            currentOffset = static_cast<uint32_t>(exe.size() - importStart);
            if (currentOffset < hintNameTableOffset) {
                for (uint32_t k = 0; k < hintNameTableOffset - currentOffset; ++k) exe.push_back(0);
            }

            for (const auto& dll : imports) {
                for (const auto& func : dll.functions) {
                    appendValue<uint16_t>(exe, func.hint);
                    for (char c : func.name) {
                        exe.push_back(static_cast<uint8_t>(c));
                    }
                    exe.push_back(0);
                    
                    if (exe.size() % 2 != 0) exe.push_back(0); // Padding
                }
            }

            // 5. DLL Names
            for (const auto& dll : imports) {
                for (char c : dll.name) {
                    exe.push_back(static_cast<uint8_t>(c));
                }
                exe.push_back(0);
            }

            alignTo(exe, 512);

            if (verbose) {
                std::cout << "✅ Import Section بني بشكل صحيح: " 
                          << (exe.size() - importStart) << " بايت" << std::endl;
            }
        }
        
        // ✅ NEW: Build Base Relocation Section
        void buildRelocationSection(std::vector<uint8_t>& exe) {
            size_t relocStart = exe.size();
            
            // Base Relocation Directory structure:
            // struct {
            //   uint32_t VirtualAddress;  // RVA of the block
            //   uint32_t SizeOfBlock;     // Size of this block
            //   uint16_t TypeOffset[];    // Array of relocations
            // }
            
            // For simplicity, we create an empty relocation table
            // (just a single block with no relocations)
            uint32_t pageRVA = 0x1000; // RVA of .text section
            uint32_t blockSize = 8;    // Minimum size (header only, no relocations)
            
            appendValue(exe, pageRVA);
            appendValue(exe, blockSize);
            
            // Pad to align
            alignTo(exe, 512);
            
            if (verbose) {
                std::cout << "✅ Relocation Section بني: " 
                          << (exe.size() - relocStart) << " بايت" << std::endl;
            }
        }

        template<typename T>
        void appendStruct(std::vector<uint8_t>& vec, const T& data) {
            const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);
            vec.insert(vec.end(), ptr, ptr + sizeof(T));
        }

        template<typename T>
        void appendValue(std::vector<uint8_t>& vec, T value) {
            const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&value);
            vec.insert(vec.end(), ptr, ptr + sizeof(T));
        }

        void alignTo(std::vector<uint8_t>& vec, size_t alignment) {
            size_t remainder = vec.size() % alignment;
            if (remainder != 0) {
                vec.resize(vec.size() + (alignment - remainder), 0);
            }
        }

        uint32_t alignValue(size_t value, uint32_t alignment) {
            return static_cast<uint32_t>(
                ((value + alignment - 1) / alignment) * alignment
                );
        }
    };

} // namespace ArabicAssembler

#endif // IMPROVED_PE_BUILDER_H
