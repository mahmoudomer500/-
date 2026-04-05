// ArabicVisionBridge.h - Stub for CI builds
#pragma once

#include <memory>
#include <functional>
#include <string>

namespace ArabicLanguage {

class ArabicRuntime;

class ArabicVisionBridge {
public:
    static void registerFunctions(std::shared_ptr<ArabicRuntime> runtime,
                                  std::function<void(const std::string&)> outputCallback = nullptr);
};

} // namespace ArabicLanguage
