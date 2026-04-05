#ifndef ARABIC_LINKER_H
#define ARABIC_LINKER_H

#include <vector>
#include <string>
#include <cstdint>

class ArabicLinker {
public:
    static bool link(const std::vector<uint8_t>& code, const std::string& outputFile);
};

#endif
