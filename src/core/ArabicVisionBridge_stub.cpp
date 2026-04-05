// ArabicVisionBridge_stub.cpp - Stub for CI builds
#include "ArabicVisionBridge.h"
#include "ArabicRuntime.h"
#include <functional>
#include <memory>
#include <string>

namespace ArabicLanguage {

void ArabicVisionBridge::registerFunctions(std::shared_ptr<ArabicRuntime> runtime,
    std::function<void(const std::string&)> outputCallback) {
    // Stub - vision not available in this build
    (void)runtime;
    (void)outputCallback;
}

} // namespace ArabicLanguage
