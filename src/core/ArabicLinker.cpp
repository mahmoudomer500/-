#include "ArabicLinker.h"
#include "SafeWindows.h"
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

#ifndef _WIN32
// Manual definitions for Windows PE structures for non-Windows platforms
typedef struct _IMAGE_DOS_HEADER {
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
    int32_t  e_lfanew;
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

#define IMAGE_DOS_SIGNATURE 0x5A4D

typedef struct _IMAGE_FILE_HEADER {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
    uint32_t VirtualAddress;
    uint32_t Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _IMAGE_OPTIONAL_HEADER64 {
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
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

typedef struct _IMAGE_NT_HEADERS64 {
    uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

#define IMAGE_NT_SIGNATURE 0x00004550
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_FILE_EXECUTABLE_IMAGE 0x0002
#define IMAGE_FILE_LARGE_ADDRESS_AWARE 0x0020
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b
#define IMAGE_SUBSYSTEM_WINDOWS_CUI 3
#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE 0x0040
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT 0x0100

typedef struct _IMAGE_SECTION_HEADER {
    uint8_t  Name[8];
    union {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SCN_CNT_CODE 0x00000020
#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#define IMAGE_SCN_MEM_READ 0x40000000
#define IMAGE_SCN_MEM_WRITE 0x80000000

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        uint32_t Characteristics;
        uint32_t OriginalFirstThunk;
    } DUMMYUNIONNAME;
    uint32_t TimeDateStamp;
    uint32_t ForwarderChain;
    uint32_t Name;
    uint32_t FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR, *PIMAGE_IMPORT_DESCRIPTOR;

#endif

bool ArabicLinker::link(const std::vector<uint8_t>& code, const std::string& outputFile) {
    // Create a minimal PE file with import table

    // DOS Header
    IMAGE_DOS_HEADER dosHeader = {};
    dosHeader.e_magic = IMAGE_DOS_SIGNATURE;
    dosHeader.e_lfanew = sizeof(IMAGE_DOS_HEADER);

    // NT Headers
    IMAGE_NT_HEADERS64 ntHeaders = {};
    ntHeaders.Signature = IMAGE_NT_SIGNATURE;
    ntHeaders.FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
    ntHeaders.FileHeader.NumberOfSections = 1;
    ntHeaders.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
    ntHeaders.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE;

    ntHeaders.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    ntHeaders.OptionalHeader.AddressOfEntryPoint = 0x1000; // RVA of entry point
    ntHeaders.OptionalHeader.ImageBase = 0x140000000;
    ntHeaders.OptionalHeader.SectionAlignment = 0x1000;
    ntHeaders.OptionalHeader.FileAlignment = 0x200;
    ntHeaders.OptionalHeader.MajorSubsystemVersion = 6;
    ntHeaders.OptionalHeader.MinorSubsystemVersion = 0;
    ntHeaders.OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    ntHeaders.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE | IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
    ntHeaders.OptionalHeader.SizeOfStackReserve = 0x100000;
    ntHeaders.OptionalHeader.SizeOfStackCommit = 0x1000;
    ntHeaders.OptionalHeader.SizeOfHeapReserve = 0x100000;
    ntHeaders.OptionalHeader.SizeOfHeapCommit = 0x1000;
    ntHeaders.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;

    // Section
    IMAGE_SECTION_HEADER section = {};
    strcpy((char*)section.Name, ".text");
    section.VirtualAddress = 0x1000;
    section.PointerToRawData = sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS64) + sizeof(IMAGE_SECTION_HEADER);
    section.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

    // Create import data
    uint32_t code_size = code.size();
    uint32_t import_data_offset = code_size;

    // RVAs relative to section base (0x1000)
    uint32_t ilt_rva = 0x1000 + import_data_offset;
    uint32_t iat_rva = ilt_rva + 32;  // 4 entries * 8 bytes
    uint32_t dll_name_rva = iat_rva + 32;  // 4 entries * 8 bytes
    uint32_t dll_name2_rva = dll_name_rva + 10;  // "user32.dll" + null
    uint32_t hint_name1_rva = dll_name2_rva + 13;  // "kernel32.dll" + null
    uint32_t hint_name2_rva = hint_name1_rva + 13;  // hint(2) + "MessageBoxA" + null
    uint32_t hint_name3_rva = hint_name2_rva + 11;  // hint(2) + "ExitProcess" + null
    uint32_t import_desc_rva = hint_name3_rva + 20;  // 2 * sizeof(IMAGE_IMPORT_DESCRIPTOR)

    // Calculate total import data size
    uint32_t import_data_size = (import_desc_rva - 0x1000) + 2 * sizeof(IMAGE_IMPORT_DESCRIPTOR);

    std::vector<uint8_t> import_data(import_data_size, 0);

    // ILT (Import Lookup Table) - user32: 2 entries, kernel32: 2 entries
    uint64_t* ilt = reinterpret_cast<uint64_t*>(&import_data[0]);
    ilt[0] = hint_name2_rva;  // MessageBoxA
    ilt[1] = 0;               // Null terminator
    ilt[2] = hint_name3_rva;  // ExitProcess
    ilt[3] = 0;               // Null terminator

    // IAT (Import Address Table) - 4 entries total
    uint64_t* iat = reinterpret_cast<uint64_t*>(&import_data[16]);
    iat[0] = hint_name2_rva;  // MessageBoxA
    iat[1] = 0;               // Null terminator
    iat[2] = hint_name3_rva;  // ExitProcess
    iat[3] = 0;               // Null terminator

    // DLL names
    const char* dll1_name = "user32.dll";
    memcpy(&import_data[32], dll1_name, 10);

    const char* dll2_name = "kernel32.dll";
    memcpy(&import_data[72], dll2_name, 13);

    // Hint/Name tables
    uint16_t hint = 0;

    // MessageBoxA
    memcpy(&import_data[45], &hint, 2);
    const char* func1_name = "MessageBoxA";
    memcpy(&import_data[47], func1_name, 10);
    import_data[57] = 0;

    // ExitProcess
    memcpy(&import_data[87], &hint, 2);
    const char* func2_name = "ExitProcess";
    memcpy(&import_data[89], func2_name, 11);
    import_data[100] = 0;

    // Import descriptors - 2 descriptors
    IMAGE_IMPORT_DESCRIPTOR* desc1 = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(&import_data[101]);
    desc1->OriginalFirstThunk = ilt_rva;
    desc1->FirstThunk = iat_rva;
    desc1->Name = dll_name_rva;

    IMAGE_IMPORT_DESCRIPTOR* desc2 = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(&import_data[101 + sizeof(IMAGE_IMPORT_DESCRIPTOR)]);
    desc2->OriginalFirstThunk = ilt_rva + 16;  // Second ILT
    desc2->FirstThunk = iat_rva + 16;  // Second IAT
    desc2->Name = dll_name2_rva;

    // Null import descriptor
    IMAGE_IMPORT_DESCRIPTOR* null_desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(&import_data[101 + 2 * sizeof(IMAGE_IMPORT_DESCRIPTOR)]);
    memset(null_desc, 0, sizeof(IMAGE_IMPORT_DESCRIPTOR));

    ntHeaders.OptionalHeader.DataDirectory[1].VirtualAddress = import_desc_rva;
    ntHeaders.OptionalHeader.DataDirectory[1].Size = 2 * sizeof(IMAGE_IMPORT_DESCRIPTOR);

    std::vector<uint8_t> section_data = code;
    section_data.insert(section_data.end(), import_data.begin(), import_data.end());

    section.Misc.VirtualSize = section_data.size();
    section.SizeOfRawData = (section_data.size() + 0x1FF) & ~0x1FF;

    // Calculate sizes
    ntHeaders.OptionalHeader.SizeOfHeaders = section.PointerToRawData;
    ntHeaders.OptionalHeader.SizeOfImage = ((section.VirtualAddress + section.Misc.VirtualSize + ntHeaders.OptionalHeader.SectionAlignment - 1) / ntHeaders.OptionalHeader.SectionAlignment) * ntHeaders.OptionalHeader.SectionAlignment;

    // Write to file
    std::ofstream out(outputFile, std::ios::binary);
    if (!out) return false;

    out.write((char*)&dosHeader, sizeof(dosHeader));
    out.write((char*)&ntHeaders, sizeof(ntHeaders));
    out.write((char*)&section, sizeof(section));

    // Pad to file alignment
    size_t headerSize = sizeof(dosHeader) + sizeof(ntHeaders) + sizeof(section);
    size_t padding = section.PointerToRawData - headerSize;
    std::vector<char> pad(padding, 0);
    out.write(pad.data(), padding);

    // Write section data
    out.write((char*)section_data.data(), section_data.size());

    // Pad to file alignment
    size_t dataPadding = section.SizeOfRawData - section_data.size();
    std::vector<char> dataPad(dataPadding, 0);
    out.write(dataPad.data(), dataPadding);

    return true;
}
