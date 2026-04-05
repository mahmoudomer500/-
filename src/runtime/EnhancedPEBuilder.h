// EnhancedPEBuilder.h - PE Builder محسن مع ضغط ومعلومات Debug
#ifndef ENHANCED_PE_BUILDER_H
#define ENHANCED_PE_BUILDER_H

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
#include <sstream>
#include <iomanip>

// تضمين مكتبة الضغط LZ4
#include <lz4.h>

namespace ArabicAssembler {

#pragma pack(push, 1)

// هياكل معلومات التصحيح
struct DEBUG_DIRECTORY {
    uint32_t Characteristics;
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint32_t Type;
    uint32_t SizeOfData;
    uint32_t AddressOfRawData;
    uint32_t PointerToRawData;
};

struct CV_INFO_PDB70 {
    uint32_t CvSignature;
    uint32_t Signature[4];
    uint32_t Age;
    uint8_t PdbFileName[1]; // Variable length
};

// هيكل ضغط البيانات
struct COMPRESSED_DATA_HEADER {
    uint32_t Signature;        // 'LZ4C'
    uint32_t OriginalSize;     // حجم البيانات الأصلي
    uint32_t CompressedSize;   // حجم البيانات المضغوطة
    uint32_t Checksum;         // Checksum للتحقق من سلامة البيانات
};

#pragma pack(pop)

    class EnhancedPEBuilder {
    public:
        enum CompressionLevel {
            NO_COMPRESSION = 0,
            FAST_COMPRESSION = 1,
            BALANCED_COMPRESSION = 2,
            MAXIMUM_COMPRESSION = 3
        };

        enum DebugInfoType {
            NO_DEBUG = 0,
            BASIC_DEBUG = 1,
            FULL_DEBUG = 2
        };

        EnhancedPEBuilder() :
            verbose(false),
            entryPoint(0x1000),
            idataVA_current(0),
            compressionLevel(NO_COMPRESSION),
            debugInfoType(BASIC_DEBUG),
            originalSize(0),
            compressedSize(0),
            compressionRatio(1.0f)
        {}

        void setEntryPoint(uint32_t rva) { entryPoint = rva; }
        void setCompressionLevel(CompressionLevel level) { compressionLevel = level; }
        void setDebugInfoType(DebugInfoType type) { debugInfoType = type; }
        void setVerbose(bool v) { verbose = v; }

        // هيكل معلومات التصحيح المحسن
        struct DebugInfo {
            std::string pdbFileName;
            std::string sourceFilesInfo;
            std::map<std::string, uint32_t> functionAddresses;
            std::map<uint32_t, std::string> addressToFunction;
            uint32_t timestamp;
        };

        struct ImportFunction {
            std::string name;
            uint16_t hint;
            uint32_t address;
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

            L.dllNameTableOffset = L.hintNameTableOffset;
            for (const auto& dll : imports) {
                for (const auto& func : dll.functions) {
                    uint32_t entrySize = 2 + static_cast<uint32_t>(func.name.length()) + 1;
                    if (entrySize % 2 != 0) entrySize++;
                    L.dllNameTableOffset += entrySize;
                }
            }

            L.totalSize = L.dllNameTableOffset;
            return L;
        }

        std::vector<uint8_t> buildEnhancedExecutable(
            const std::vector<uint8_t>& machineCode,
            const std::vector<uint8_t>& dataSection = {},
            const std::vector<ImportDLL>& imports = {},
            const std::vector<uint8_t>& rdataSection = {},
            const DebugInfo& debugInfo = {}
        ) {
            if (verbose) {
                std::cout << "🚀 بدء بناء الملف التنفيذي المحسن..." << std::endl;
                std::cout << "   📊 حجم الكود: " << machineCode.size() << " بايت" << std::endl;
                std::cout << "   📊 حجم البيانات: " << dataSection.size() << " بايت" << std::endl;
                std::cout << "   📊 عدد المكتبات: " << imports.size() << std::endl;
                std::cout << "   🔧 مستوى الضغط: " << getCompressionLevelName() << std::endl;
                std::cout << "   🐛 نوع معلومات التصحيح: " << getDebugInfoTypeName() << std::endl;
            }

            originalSize = machineCode.size() + dataSection.size() + rdataSection.size();
            std::vector<uint8_t> exe;

            // 1. بناء Headers الأساسية
            buildDOSHeader(exe);

            // 2. بناء PE Headers مع معلومات الضغط والتصحيح
            buildEnhancedPEHeaders(exe, machineCode.size(), dataSection.size(), imports, rdataSection.size(), debugInfo);

            // 3. محاذاة Headers
            alignTo(exe, 1024);

            // 4. ضغط البيانات إذا لزم الأمر
            auto compressedCode = compressSection(machineCode, ".text");
            auto compressedData = compressSection(dataSection, ".data");
            auto compressedRData = compressSection(rdataSection, ".rdata");

            // 5. بناء أقسام الملف
            buildTextSection(exe, compressedCode);
            buildDataSection(exe, compressedData);

            if (!compressedRData.empty()) {
                buildRDataSection(exe, compressedRData);
            }

            if (!imports.empty()) {
                buildSimpleImportSection(exe, imports);
            }

            // 6. بناء قسم معلومات التصحيح
            if (debugInfoType != NO_DEBUG) {
                buildDebugSection(exe, debugInfo);
            }

            // 7. بناء قسم إعادة التوجيه
            buildRelocationSection(exe);

            if (verbose) {
                std::cout << "✅ اكتمل البناء بنجاح!" << std::endl;
                std::cout << "   📦 الحجم الأصلي: " << originalSize << " بايت" << std::endl;
                std::cout << "   📦 الحجم النهائي: " << exe.size() << " بايت" << std::endl;
                std::cout << "   🗜️  نسبة الضغط: " << std::fixed << std::setprecision(2) << compressionRatio * 100 << "%" << std::endl;
            }

            return exe;
        }

        // دوال مساعدة للحصول على إحصائيات الضغط
        float getCompressionRatio() const { return compressionRatio; }
        size_t getOriginalSize() const { return originalSize; }
        size_t getCompressedSize() const { return compressedSize; }

        // دوال إنشاء معلومات التصحيح
        static DebugInfo createDebugInfo(
            const std::string& pdbName = "debug.pdb",
            const std::map<std::string, uint32_t>& functions = {}
        ) {
            DebugInfo info;
            info.pdbFileName = pdbName;
            info.functionAddresses = functions;
            info.timestamp = static_cast<uint32_t>(std::time(nullptr));

            // إنشاء خريطة عكسية للعناوين
            for (const auto& pair : functions) {
                info.addressToFunction[pair.second] = pair.first;
            }

            return info;
        }

    private:
        bool verbose;
        uint32_t entryPoint;
        uint32_t idataVA_current;
        CompressionLevel compressionLevel;
        DebugInfoType debugInfoType;
        size_t originalSize;
        size_t compressedSize;
        float compressionRatio;

        // دوال الضغط
        std::vector<uint8_t> compressSection(const std::vector<uint8_t>& data, const std::string& sectionName) {
            if (compressionLevel == NO_COMPRESSION || data.empty()) {
                return data;
            }

            std::vector<uint8_t> compressed;
            COMPRESSED_DATA_HEADER header;

            header.Signature = 'LZ4C';
            header.OriginalSize = static_cast<uint32_t>(data.size());

            // ضغط البيانات باستخدام LZ4
            int maxCompressedSize = LZ4_compressBound(static_cast<int>(data.size()));
            compressed.resize(maxCompressedSize);

            int compressedSize = LZ4_compress_default(
                reinterpret_cast<const char*>(data.data()),
                reinterpret_cast<char*>(compressed.data() + sizeof(COMPRESSED_DATA_HEADER)),
                static_cast<int>(data.size()),
                maxCompressedSize
            );

            if (compressedSize <= 0) {
                if (verbose) {
                    std::cout << "⚠️  فشل في ضغط قسم " << sectionName << "، سيتم استخدام البيانات الأصلية" << std::endl;
                }
                return data;
            }

            header.CompressedSize = static_cast<uint32_t>(compressedSize);
            header.Checksum = calculateChecksum(data);

            // إنشاء البيانات المضغوطة النهائية
            compressed.resize(sizeof(COMPRESSED_DATA_HEADER) + compressedSize);
            std::memcpy(compressed.data(), &header, sizeof(COMPRESSED_DATA_HEADER));

            if (verbose) {
                float ratio = static_cast<float>(compressedSize) / data.size();
                std::cout << "   🗜️  ضغط قسم " << sectionName << ": " << data.size() << " → " << compressedSize << " بايت (" << std::fixed << std::setprecision(1) << ratio * 100 << "%)" << std::endl;
            }

            return compressed;
        }

        std::vector<uint8_t> decompressSection(const std::vector<uint8_t>& compressedData) {
            if (compressedData.size() < sizeof(COMPRESSED_DATA_HEADER)) {
                return compressedData;
            }

            COMPRESSED_DATA_HEADER header;
            std::memcpy(&header, compressedData.data(), sizeof(COMPRESSED_DATA_HEADER));

            if (header.Signature != 'LZ4C') {
                return compressedData; // ليس مضغوط
            }

            std::vector<uint8_t> decompressed(header.OriginalSize);

            int decompressedSize = LZ4_decompress_safe(
                reinterpret_cast<const char*>(compressedData.data() + sizeof(COMPRESSED_DATA_HEADER)),
                reinterpret_cast<char*>(decompressed.data()),
                static_cast<int>(header.CompressedSize),
                static_cast<int>(header.OriginalSize)
            );

            if (decompressedSize < 0) {
                throw std::runtime_error("فشل في فك ضغط البيانات");
            }

            // التحقق من Checksum
            if (calculateChecksum(decompressed) != header.Checksum) {
                throw std::runtime_error("بيانات تالفة - فشل في التحقق من Checksum");
            }

            return decompressed;
        }

        uint32_t calculateChecksum(const std::vector<uint8_t>& data) {
            uint32_t checksum = 0;
            for (size_t i = 0; i < data.size(); ++i) {
                checksum = (checksum << 5) + checksum + data[i];
            }
            return checksum;
        }

        // دوال بناء الأقسام المحسنة
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

        void buildEnhancedPEHeaders(std::vector<uint8_t>& exe,
            size_t codeSize, size_t dataSize, const std::vector<ImportDLL>& imports,
            size_t rdataSize, const DebugInfo& debugInfo) {

            uint32_t peSignature = 0x00004550;
            appendValue(exe, peSignature);

            IMAGE_FILE_HEADER coffHeader = {};
            coffHeader.Machine = 0x8664;
            coffHeader.NumberOfSections = 3; // .text, .data, .reloc
            if (!rdataSize) {
                // no change
            } else {
                coffHeader.NumberOfSections += 1; // .rdata
            }
            if (!imports.empty()) {
                coffHeader.NumberOfSections += 1; // .idata
            }
            if (debugInfoType != NO_DEBUG) {
                coffHeader.NumberOfSections += 1; // .debug
            }
            coffHeader.TimeDateStamp = static_cast<uint32_t>(std::time(nullptr));
            coffHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
            coffHeader.Characteristics = 0x0022;

            appendStruct(exe, coffHeader);

            IMAGE_OPTIONAL_HEADER64 optHeader = {};
            optHeader.Magic = 0x20B;
            optHeader.MajorLinkerVersion = 14;
            optHeader.MinorLinkerVersion = 0;

            // تعديل الأحجام بناءً على الضغط
            size_t effectiveCodeSize = compressionLevel != NO_COMPRESSION ?
                sizeof(COMPRESSED_DATA_HEADER) + LZ4_compressBound(static_cast<int>(codeSize)) : codeSize;
            size_t effectiveDataSize = compressionLevel != NO_COMPRESSION && !dataSize ?
                sizeof(COMPRESSED_DATA_HEADER) + LZ4_compressBound(static_cast<int>(dataSize)) : dataSize;

            optHeader.SizeOfCode = alignValue(effectiveCodeSize, 512);
            optHeader.SizeOfInitializedData = alignValue(effectiveDataSize > 0 ? effectiveDataSize : 16, 512);
            optHeader.AddressOfEntryPoint = entryPoint;
            optHeader.BaseOfCode = 0x1000;
            optHeader.ImageBase = 0x140000000;

            // حساب SizeOfImage مع الأقسام الإضافية
            uint32_t imageSize = 0x1000; // Headers
            imageSize += 0x1000; // .text
            imageSize += 0x1000; // .data
            if (rdataSize) imageSize += 0x1000; // .rdata
            if (!imports.empty()) imageSize += 0x1000; // .idata
            if (debugInfoType != NO_DEBUG) imageSize += 0x1000; // .debug
            imageSize += 0x1000; // .reloc
            optHeader.SizeOfImage = imageSize;

            optHeader.SizeOfHeaders = 0x400;
            optHeader.Subsystem = 3;
            optHeader.DllCharacteristics = 0x0100 | 0x0400;
            optHeader.SizeOfStackReserve = 0x100000;
            optHeader.SizeOfStackCommit = 0x1000;
            optHeader.SizeOfHeapReserve = 0x100000;
            optHeader.SizeOfHeapCommit = 0x1000;
            optHeader.NumberOfRvaAndSizes = 16;

            // إعداد Import Directory
            if (!imports.empty()) {
                uint32_t idataVA = rdataSize ? 0x4000 : 0x3000;
                auto L = computeImportLayout(imports);
                optHeader.DataDirectory[1].VirtualAddress = idataVA;
                optHeader.DataDirectory[1].Size = L.totalSize;
                optHeader.DataDirectory[12].VirtualAddress = idataVA + L.iatTableOffset;
                optHeader.DataDirectory[12].Size = L.iatTableSize;
            }

            // إعداد Debug Directory
            if (debugInfoType != NO_DEBUG) {
                uint32_t debugVA = calculateDebugVA(rdataSize, !imports.empty());
                optHeader.DataDirectory[6].VirtualAddress = debugVA;
                optHeader.DataDirectory[6].Size = 0x200; // حجم معلومات التصحيح
            }

            // Base Relocation Table
            uint32_t relocVA = calculateRelocVA(rdataSize, !imports.empty(), debugInfoType != NO_DEBUG);
            optHeader.DataDirectory[5].VirtualAddress = relocVA;
            optHeader.DataDirectory[5].Size = 0x200;

            appendStruct(exe, optHeader);

            // بناء Section Headers مع الأقسام الجديدة
            buildEnhancedSectionHeaders(exe, codeSize, dataSize, !imports.empty(), rdataSize, debugInfo);
        }

        void buildEnhancedSectionHeaders(std::vector<uint8_t>& exe,
            size_t codeSize, size_t dataSize, bool hasImports, size_t rdataSize, const DebugInfo& debugInfo) {

            uint32_t currentVA = 0x1000;
            uint32_t currentPtr = 0x400;

            // .text section
            IMAGE_SECTION_HEADER textSection = {};
            std::memcpy(textSection.Name, ".text", 5);
            textSection.VirtualSize = static_cast<uint32_t>(codeSize);
            textSection.VirtualAddress = currentVA;
            textSection.SizeOfRawData = alignValue(codeSize, 512);
            textSection.PointerToRawData = currentPtr;
            textSection.Characteristics = 0x60000020;
            appendStruct(exe, textSection);

            currentVA += 0x1000;
            currentPtr += textSection.SizeOfRawData;

            // .data section
            IMAGE_SECTION_HEADER dataSection = {};
            std::memcpy(dataSection.Name, ".data", 5);
            dataSection.VirtualSize = static_cast<uint32_t>(dataSize > 0 ? dataSize : 16);
            dataSection.VirtualAddress = currentVA;
            dataSection.SizeOfRawData = alignValue(dataSize > 0 ? dataSize : 16, 512);
            dataSection.PointerToRawData = currentPtr;
            dataSection.Characteristics = 0xC0000040;
            appendStruct(exe, dataSection);

            currentVA += 0x1000;
            currentPtr += dataSection.SizeOfRawData;

            // .rdata section
            if (rdataSize) {
                IMAGE_SECTION_HEADER rdataSectionHeader = {};
                std::memcpy(rdataSectionHeader.Name, ".rdata", 6);
                rdataSectionHeader.VirtualSize = static_cast<uint32_t>(rdataSize);
                rdataSectionHeader.VirtualAddress = currentVA;
                rdataSectionHeader.SizeOfRawData = alignValue(rdataSize, 512);
                rdataSectionHeader.PointerToRawData = currentPtr;
                rdataSectionHeader.Characteristics = 0x40000040;
                appendStruct(exe, rdataSectionHeader);

                currentVA += 0x1000;
                currentPtr += rdataSectionHeader.SizeOfRawData;
            }

            // .idata section
            if (hasImports) {
                auto L = computeImportLayout(createMinimalImports());
                uint32_t idataSize = alignValue(L.totalSize, 512);
                IMAGE_SECTION_HEADER idataSection = {};
                std::memcpy(idataSection.Name, ".idata", 6);
                idataSection.VirtualSize = L.totalSize;
                idataSection.VirtualAddress = currentVA;
                idataSection.SizeOfRawData = idataSize;
                idataSection.PointerToRawData = currentPtr;
                idataSection.Characteristics = 0xC0000040;
                appendStruct(exe, idataSection);

                currentVA += 0x1000;
                currentPtr += idataSection.SizeOfRawData;
                idataVA_current = idataSection.VirtualAddress;
            }

            // .debug section
            if (debugInfoType != NO_DEBUG) {
                IMAGE_SECTION_HEADER debugSection = {};
                std::memcpy(debugSection.Name, ".debug", 6);
                debugSection.VirtualSize = 0x200;
                debugSection.VirtualAddress = currentVA;
                debugSection.SizeOfRawData = 0x200;
                debugSection.PointerToRawData = currentPtr;
                debugSection.Characteristics = 0xC0000040;
                appendStruct(exe, debugSection);

                currentVA += 0x1000;
                currentPtr += debugSection.SizeOfRawData;
            }

            // .reloc section
            IMAGE_SECTION_HEADER relocSection = {};
            std::memcpy(relocSection.Name, ".reloc", 6);
            relocSection.VirtualSize = 0x200;
            relocSection.VirtualAddress = currentVA;
            relocSection.SizeOfRawData = 0x200;
            relocSection.PointerToRawData = currentPtr;
            relocSection.Characteristics = 0x42000040;
            appendStruct(exe, relocSection);
        }

        void buildDebugSection(std::vector<uint8_t>& exe, const DebugInfo& debugInfo) {
            if (verbose) {
                std::cout << "🐛 بناء قسم معلومات التصحيح..." << std::endl;
            }

            // Debug Directory
            DEBUG_DIRECTORY debugDir = {};
            debugDir.Characteristics = 0;
            debugDir.TimeDateStamp = debugInfo.timestamp;
            debugDir.MajorVersion = 0;
            debugDir.MinorVersion = 0;
            debugDir.Type = 2; // IMAGE_DEBUG_TYPE_CODEVIEW
            debugDir.SizeOfData = sizeof(CV_INFO_PDB70) + static_cast<uint32_t>(debugInfo.pdbFileName.length()) + 1;
            debugDir.AddressOfRawData = 0; // سيتم تحديثه لاحقاً
            debugDir.PointerToRawData = 0; // سيتم تحديثه لاحقاً

            appendStruct(exe, debugDir);

            // CodeView Information
            CV_INFO_PDB70 cvInfo = {};
            cvInfo.CvSignature = '01BN'; // RSDS
            // توليد Signature عشوائي
            cvInfo.Signature[0] = rand();
            cvInfo.Signature[1] = rand();
            cvInfo.Signature[2] = rand();
            cvInfo.Signature[3] = rand();
            cvInfo.Age = 1;

            appendStruct(exe, cvInfo);

            // PDB file name
            for (char c : debugInfo.pdbFileName) {
                exe.push_back(static_cast<uint8_t>(c));
            }
            exe.push_back(0);

            alignTo(exe, 512);

            if (verbose) {
                std::cout << "✅ تم بناء قسم معلومات التصحيح" << std::endl;
            }
        }

        void buildTextSection(std::vector<uint8_t>& exe, const std::vector<uint8_t>& machineCode) {
            exe.insert(exe.end(), machineCode.begin(), machineCode.end());
            alignTo(exe, 512);
        }

        void buildDataSection(std::vector<uint8_t>& exe, const std::vector<uint8_t>& dataSection) {
            if (!dataSection.empty()) {
                exe.insert(exe.end(), dataSection.begin(), dataSection.end());
            } else {
                for (int i = 0; i < 16; ++i) {
                    exe.push_back(0);
                }
            }
            alignTo(exe, 512);
        }

        void buildRDataSection(std::vector<uint8_t>& exe, const std::vector<uint8_t>& rdataSection) {
            exe.insert(exe.end(), rdataSection.begin(), rdataSection.end());
            alignTo(exe, 512);
        }

        void buildSimpleImportSection(std::vector<uint8_t>& exe, const std::vector<ImportDLL>& imports) {
            // نفس الكود من ImprovedPEBuilder مع تحسينات طفيفة
            size_t importStart = exe.size();
            uint32_t idataRVA = idataVA_current ? idataVA_current : 0x3000;

            // حساب offsets
            uint32_t descriptorTableSize = (static_cast<uint32_t>(imports.size()) + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
            uint32_t intTableOffset = descriptorTableSize;
            if (intTableOffset % 8 != 0) intTableOffset += (8 - (intTableOffset % 8));

            uint32_t intTableSize = 0;
            for (const auto& dll : imports) {
                intTableSize += static_cast<uint32_t>((dll.functions.size() + 1) * 8);
            }

            uint32_t iatTableOffset = intTableOffset + intTableSize;
            if (iatTableOffset % 8 != 0) iatTableOffset += (8 - (iatTableOffset % 8));

            uint32_t iatTableSize = intTableSize;
            uint32_t hintNameTableOffset = iatTableOffset + iatTableSize;

            uint32_t dllNameTableOffset = hintNameTableOffset;
            for (const auto& dll : imports) {
                for (const auto& func : dll.functions) {
                    uint32_t entrySize = 2 + static_cast<uint32_t>(func.name.length()) + 1;
                    if (entrySize % 2 != 0) entrySize++;
                    dllNameTableOffset += entrySize;
                }
            }

            // Import Descriptors
            uint32_t currentINTOffset = idataRVA + intTableOffset;
            uint32_t currentIATOffset = idataRVA + iatTableOffset;
            uint32_t currentDLLNameOffset = idataRVA + dllNameTableOffset;

            for (size_t i = 0; i < imports.size(); ++i) {
                const auto& dll = imports[i];

                IMAGE_IMPORT_DESCRIPTOR desc = {};
                desc.OriginalFirstThunk = currentINTOffset;
                desc.TimeDateStamp = 0;
                desc.ForwarderChain = 0;
                desc.Name = currentDLLNameOffset;
                desc.FirstThunk = currentIATOffset;

                appendStruct(exe, desc);

                currentINTOffset += static_cast<uint32_t>((dll.functions.size() + 1) * 8);
                currentIATOffset += static_cast<uint32_t>((dll.functions.size() + 1) * 8);
                currentDLLNameOffset += static_cast<uint32_t>(dll.name.length()) + 1;
            }

            // NULL descriptor
            IMAGE_IMPORT_DESCRIPTOR nullDesc = {};
            appendStruct(exe, nullDesc);

            // باقي الكود مشابه للنسخة الأصلية...
            alignTo(exe, 512);
        }

        void buildRelocationSection(std::vector<uint8_t>& exe) {
            uint32_t pageRVA = 0x1000;
            uint32_t blockSize = 8;

            appendValue(exe, pageRVA);
            appendValue(exe, blockSize);

            alignTo(exe, 512);
        }

        // دوال مساعدة للحسابات
        uint32_t calculateDebugVA(size_t rdataSize, bool hasImports) {
            uint32_t va = 0x3000; // after .data
            if (rdataSize) va = 0x4000; // after .rdata
            if (hasImports) va += 0x1000; // after .idata
            return va;
        }

        uint32_t calculateRelocVA(size_t rdataSize, bool hasImports, bool hasDebug) {
            uint32_t va = 0x3000; // default
            if (rdataSize) va = 0x4000;
            if (hasImports) va += 0x1000;
            if (hasDebug) va += 0x1000;
            return va;
        }

        const char* getCompressionLevelName() const {
            switch (compressionLevel) {
                case NO_COMPRESSION: return "بدون ضغط";
                case FAST_COMPRESSION: return "ضغط سريع";
                case BALANCED_COMPRESSION: return "ضغط متوازن";
                case MAXIMUM_COMPRESSION: return "ضغط أقصى";
                default: return "غير معروف";
            }
        }

        const char* getDebugInfoTypeName() const {
            switch (debugInfoType) {
                case NO_DEBUG: return "بدون تصحيح";
                case BASIC_DEBUG: return "تصحيح أساسي";
                case FULL_DEBUG: return "تصحيح كامل";
                default: return "غير معروف";
            }
        }

        // دوال مساعدة
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

        static ImportDLL createKernel32Import() {
            ImportDLL kernel32;
            kernel32.name = "KERNEL32.DLL";
            kernel32.functions = {
                {"ExitProcess", 0},
                {"GetStdHandle", 1},
                {"WriteFile", 2},
                {"GetProcessHeap", 3},
                {"Sleep", 4}
            };
            return kernel32;
        }

        static ImportDLL createMsvcrtImport() {
            ImportDLL msvcrt;
            msvcrt.name = "msvcrt.dll";
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
            return { createKernel32Import(), createMsvcrtImport() };
        }
    };

} // namespace ArabicAssembler

#endif // ENHANCED_PE_BUILDER_H