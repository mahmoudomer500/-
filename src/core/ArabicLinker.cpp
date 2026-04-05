#include "ArabicLinker.h"
#include "SafeWindows.h"
#include <fstream>
#include <vector>
#include <cstdint>

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
