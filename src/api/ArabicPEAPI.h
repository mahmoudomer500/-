// ArabicPEAPI.h - واجهة برمجة التطبيقات العربية لملفات PE
#ifndef ARABIC_PE_API_H
#define ARABIC_PE_API_H

#include "ArabicPEBuilder.h"
#include "ArabicSectionBuilder.h"
#include "ArabicTableManager.h"
#include <string>
#include <vector>
#include <memory>

namespace ArabicAssembler {

class ArabicPEAPI {
private:
    std::unique_ptr<ArabicPEBuilder> peBuilder;
    std::unique_ptr<ArabicSectionBuilder> sectionBuilder;
    std::unique_ptr<ArabicTableManager> tableManager;

public:
    ArabicPEAPI();
    ~ArabicPEAPI() = default;

    // تهيئة ملف PE
    bool initializePEFile(uint16_t machineType = 0x8664, uint32_t imageBase = 0x400000, uint32_t subsystem = 3);

    // إضافة محتوى
    bool addCode(const std::vector<uint8_t>& code);
    bool addData(const std::string& name, const std::vector<uint8_t>& data);
    bool addString(const std::string& str);
    bool addSymbol(const std::string& name, uint32_t value);

    // بناء وحفظ
    std::vector<uint8_t> buildExecutable();
    bool saveToFile(const std::string& filename);

    // فحص ومعلومات
    bool validateExecutable() const;
    void printFileInfo() const;
    
    ArabicPEBuilder::BuildStats getBuildStats() const;
    
    // دوال مساعدة للمطورين
    uint32_t getCodeSize() const;
    uint32_t getDataSize() const;
    uint32_t getTotalSize() const;
};

} // namespace ArabicAssembler

#endif // ARABIC_PE_API_H
