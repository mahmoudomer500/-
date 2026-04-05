// BridgeManager.h - مدير الجسور الموحد
// يقوم بتجميع كافة التفاعلات بين C++ والمكونات المكتوبة بالعربية

#ifndef BRIDGE_MANAGER_H
#define BRIDGE_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include "../runtime/ArabicTypes.h"
#include "ArabicParser.h"
#include "SymbolTable.h"

namespace ArabicLanguage {

class ArabicCompiler;
class ArabicRuntime;

class BridgeManager {
private:
    std::shared_ptr<ArabicCompiler> selfCompiler;
    std::shared_ptr<ArabicRuntime> selfRuntime;
    bool isInitialized;

    // Singleton Instance
    BridgeManager();

public:
    static BridgeManager& getInstance();
    
    ~BridgeManager();

    // منع النسخ
    BridgeManager(const BridgeManager&) = delete;
    BridgeManager& operator=(const BridgeManager&) = delete;

    // تهيئة النظام الأساسي
    bool initialize();

    // ══════════════════════════════════════════════════════════════
    // 🌉 جسور المكونات الأساسية
    // ══════════════════════════════════════════════════════════════

    // جسر المحلل اللغوي
    std::vector<ArabicParser::Token> tokenize(const std::string& code);

    // جسر المحلل القواعدي
    std::vector<std::shared_ptr<Command>> callParser(const std::string& code, SymbolTable& symbols);
    Value parseToValue(const std::vector<ArabicParser::Token>& tokens);

    // جسر مولد الكود
    std::string callCodeGen(const std::vector<std::shared_ptr<Command>>& ast);
    std::string generateFromValue(const Value& astNodes);

    // جسر معالج الأخطاء
    void reportError(const std::string& message, int line, int column);

    // جسر المحسن
    void optimizeAST(std::vector<std::shared_ptr<Command>>& ast);

    // ══════════════════════════════════════════════════════════════
    // 🛠️ وظائف إضافية
    // ══════════════════════════════════════════════════════════════
    bool isReady() const { return isInitialized; }
    void reset();
};

} // namespace ArabicLanguage

#endif // BRIDGE_MANAGER_H
