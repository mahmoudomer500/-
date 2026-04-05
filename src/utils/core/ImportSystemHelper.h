#pragma once
#include <string>
#include <vector>
#include <memory>

namespace ArabicLanguage {

class ArabicRuntime;

class ImportSystemHelper {
public:
    ImportSystemHelper() = default;
    // Load library/file by name, return true if loaded
    bool loadLibrary(ArabicRuntime& runtime, const std::string& libName);
    // Retrieve list of symbols imported
    std::vector<std::string> getImportedSymbols(const std::string& libName) const;
};

} // namespace ArabicLanguage
