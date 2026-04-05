#include "ImportSystemHelper.h"
#include "ArabicRuntime.h"

namespace ArabicLanguage {

bool ImportSystemHelper::loadLibrary(ArabicRuntime& runtime, const std::string& libName) {
    // Stub: currently just mark library as loaded via runtime symbol
    runtime.defineVariable(std::string("__imported_") + libName, Value(ValueType::STRING, "1"));
    return true;
}

std::vector<std::string> ImportSystemHelper::getImportedSymbols(const std::string& libName) const {
    // Stub: no symbol information available
    return {};
}

} // namespace ArabicLanguage
