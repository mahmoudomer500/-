#include "ArabicJITCompiler.h"
#include "ArabicParser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <set>

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // ⚡ تنفيذ مترجم JIT المتقدم
    // ══════════════════════════════════════════════════════════════

    // Singleton instance
    ArabicJITCompiler& ArabicJITCompiler::getInstance() {
        static ArabicJITCompiler instance;
        return instance;
    }

    // ══════════════════════════════════════════════════════════════
    // 📝 تنفيذ تعليمات البايت كود
    // ══════════════════════════════════════════════════════════════

    std::string ArabicJITCompiler::BytecodeInstruction::toString() const {
        std::stringstream ss;
        ss << "Opcode: " << static_cast<int>(opcode)
           << " (" << operand1 << ", " << operand2 << ", " << operand3 << ")";
        return ss.str();
    }

    // ══════════════════════════════════════════════════════════════
    // 💾 تنفيذ نظام التخزين المؤقت
    // ══════════════════════════════════════════════════════════════

    void ArabicJITCompiler::BytecodeCache::evictLRU() {
        if (cache.empty()) return;

        auto oldest = cache.begin();
        for (auto it = cache.begin(); it != cache.end(); ++it) {
            if (it->second.lastAccess < oldest->second.lastAccess) {
                oldest = it;
            }
        }

        currentSize -= oldest->second.bytecode->getSize();
        cache.erase(oldest);
    }

    void ArabicJITCompiler::BytecodeCache::cleanupExpired() {
        auto now = std::chrono::steady_clock::now();
        auto expiry = std::chrono::hours(24); // انتهاء صلاحية بعد 24 ساعة

        for (auto it = cache.begin(); it != cache.end(); ) {
            auto age = now - it->second.lastAccess;
            if (age > expiry) {
                currentSize -= it->second.bytecode->getSize();
                it = cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::shared_ptr<ArabicJITCompiler::CompiledBytecode> ArabicJITCompiler::BytecodeCache::get(const std::string& hash) {
        std::lock_guard<std::mutex> lock(cacheMutex);

        auto it = cache.find(hash);
        if (it != cache.end()) {
            it->second.lastAccess = std::chrono::steady_clock::now();
            it->second.accessCount++;
            hits++;
            return it->second.bytecode;
        }

        misses++;
        return nullptr;
    }

    void ArabicJITCompiler::BytecodeCache::put(const std::string& hash, std::shared_ptr<CompiledBytecode> bytecode) {
        std::lock_guard<std::mutex> lock(cacheMutex);

        size_t bytecodeSize = bytecode->getSize();

        // تنظيف المنتهية الصلاحية
        cleanupExpired();

        // إزالة إذا تجاوز الحد الأقصى
        while (currentSize + bytecodeSize > maxSize && !cache.empty()) {
            evictLRU();
        }

        cache[hash] = CacheEntry(bytecode);
        currentSize += bytecodeSize;
    }

    void ArabicJITCompiler::BytecodeCache::clear() {
        std::lock_guard<std::mutex> lock(cacheMutex);
        cache.clear();
        currentSize = 0;
        hits = 0;
        misses = 0;
    }

    std::vector<std::string> ArabicJITCompiler::BytecodeCache::getStats() const {
        std::lock_guard<std::mutex> lock(cacheMutex);
        std::vector<std::string> stats;
        stats.push_back("=== إحصائيات التخزين المؤقت ===");
        stats.push_back("عدد المدخلات: " + std::to_string(cache.size()));
        stats.push_back("الحجم الحالي: " + std::to_string(currentSize) + " بايت");
        stats.push_back("الحد الأقصى: " + std::to_string(maxSize) + " بايت");
        stats.push_back("معدل الإصابة: " + std::to_string(getHitRate()) + "%");
        stats.push_back("الإصابات: " + std::to_string(hits.load()));
        stats.push_back("الإخفاقات: " + std::to_string(misses.load()));

        return stats;
    }

    // ══════════════════════════════════════════════════════════════
    // ⚡ تنفيذ مترجم JIT
    // ══════════════════════════════════════════════════════════════

    void* ArabicJITCompiler::JITCompiler::compile(const CompiledBytecode& bytecode) {
        // تنفيذ بسيط - في الإصدار الكامل سيتم إنشاء كود آلة حقيقي
        // هذا مجرد نموذج للدلالة

        std::lock_guard<std::mutex> lock(jitMutex);

        // تحسين البايت كود أولاً
        CompiledBytecode optimizedBytecode = bytecode;
        optimize(optimizedBytecode);

        // حفظ في التخزين المؤقت
        std::string key = optimizedBytecode.sourceHash + "_jit";
        
        // استخدام lambda لمحاكاة التنفيذ
        compiledFunctions[key] = [optimizedBytecode](const CompiledBytecode&) -> void* {
            return nullptr; // في الحقيقة سنعيد مؤشراً للدالة المولدة
        };

        return nullptr;
    }

    void ArabicJITCompiler::JITCompiler::optimize(CompiledBytecode& bytecode) {
        // تحسينات بسيطة للبايت كود

        // إزالة التعليمات غير المستخدمة
        std::vector<BytecodeInstruction> optimized;
        for (const auto& instr : bytecode.code) {
            if (instr.opcode != Opcode::NOP) {
                optimized.push_back(instr);
            }
        }
        bytecode.code = std::move(optimized);

        // دمج العمليات البسيطة
        for (size_t i = 0; i + 1 < bytecode.code.size(); ++i) {
            auto& current = bytecode.code[i];
            auto& next = bytecode.code[i + 1];

            // دمج ADD مع ثابت
            if (current.opcode == Opcode::LOAD_CONST && next.opcode == Opcode::ADD) {
                current.opcode = Opcode::OPTIMIZED_ADD;
                bytecode.code.erase(bytecode.code.begin() + i + 1);
                i--; // إعادة فحص الموضع الحالي
            }
        }
    }

    void ArabicJITCompiler::JITCompiler::clearCache() {
        std::lock_guard<std::mutex> lock(jitMutex);
        compiledFunctions.clear();
    }

    // ══════════════════════════════════════════════════════════════
    // 🎯 تنفيذ واجهة المترجم الرئيسية
    // ══════════════════════════════════════════════════════════════

    std::shared_ptr<ArabicJITCompiler::CompiledBytecode> ArabicJITCompiler::compileToBytecode(
        const std::string& sourceCode, const std::string& filename) {

        // حساب الهاش
        std::string hash = computeHash(sourceCode);

        // فحص التخزين المؤقت
        auto cached = cache.get(hash);
        if (cached) {
            return cached;
        }

        // تحليل الكود
        SymbolTable symbols;
        ArabicParser parser(&symbols);
        auto commands = parser.parse(sourceCode, symbols);

        // توليد البايت كود
        auto bytecode = std::make_shared<CompiledBytecode>();
        bytecode->sourceHash = hash;
        bytecode->code = generateBytecode(commands);
        bytecode->compiledAt = std::chrono::steady_clock::now();

        // استخراج الثوابت والمتغيرات
        extractConstantsAndVariables(commands, *bytecode);

        // تحسين البايت كود
        optimizeBytecode(bytecode);

        // حفظ في التخزين المؤقت
        cache.put(hash, bytecode);

        return bytecode;
    }

    Value ArabicJITCompiler::executeBytecode(const std::shared_ptr<CompiledBytecode>& bytecode) {
        if (!bytecode) return Value{ValueType::NONE};

        JITExecutionEngine engine(*this);
        auto result = engine.execute(bytecode);

        // تحديث إحصائيات الأداء
        bytecode->executionCount++;
        // (سيتم حساب متوسط وقت التنفيذ لاحقاً)

        return result;
    }

    void ArabicJITCompiler::optimizeBytecode(std::shared_ptr<CompiledBytecode>& bytecode) {
        if (!bytecode) return;

        // تحسينات JIT
        jitCompiler.optimize(*bytecode);

        // تحسينات إضافية
        optimizeInstructions(bytecode->code);
        inlineFunctions(bytecode->code);
    }

    bool ArabicJITCompiler::saveBytecode(const std::shared_ptr<CompiledBytecode>& bytecode, const std::string& filename) {
        if (!bytecode) return false;

        try {
            std::ofstream file(filename, std::ios::binary);
            if (!file.is_open()) return false;

            // كتابة الهاش
            size_t hashSize = bytecode->sourceHash.size();
            file.write(reinterpret_cast<const char*>(&hashSize), sizeof(hashSize));
            file.write(bytecode->sourceHash.c_str(), hashSize);

            // كتابة البايت كود
            size_t codeSize = bytecode->code.size();
            file.write(reinterpret_cast<const char*>(&codeSize), sizeof(codeSize));
            file.write(reinterpret_cast<const char*>(bytecode->code.data()),
                      codeSize * sizeof(BytecodeInstruction));

            // كتابة الثوابت
            size_t constSize = bytecode->constants.size();
            file.write(reinterpret_cast<const char*>(&constSize), sizeof(constSize));
            for (const auto& constant : bytecode->constants) {
                // (تبسيط للعرض التوضيحي - في الإصدار الكامل سيتم تسلسل كامل للـ Value)
                size_t strSize = constant.string_value.size();
                file.write(reinterpret_cast<const char*>(&strSize), sizeof(strSize));
                file.write(constant.string_value.c_str(), strSize);
            }

            file.close();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    std::shared_ptr<ArabicJITCompiler::CompiledBytecode> ArabicJITCompiler::loadBytecode(const std::string& filename) {
        try {
            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open()) return nullptr;

            auto bytecode = std::make_shared<CompiledBytecode>();

            // قراءة الهاش
            size_t hashSize;
            file.read(reinterpret_cast<char*>(&hashSize), sizeof(hashSize));
            bytecode->sourceHash.resize(hashSize);
            file.read(&bytecode->sourceHash[0], hashSize);

            // قراءة البايت كود
            size_t codeSize;
            file.read(reinterpret_cast<char*>(&codeSize), sizeof(codeSize));
            bytecode->code.resize(codeSize);
            file.read(reinterpret_cast<char*>(bytecode->code.data()),
                     codeSize * sizeof(BytecodeInstruction));

            // قراءة الثوابت
            size_t constSize;
            file.read(reinterpret_cast<char*>(&constSize), sizeof(constSize));
            bytecode->constants.resize(constSize);
            for (auto& constant : bytecode->constants) {
                size_t strSize;
                file.read(reinterpret_cast<char*>(&strSize), sizeof(strSize));
                constant.string_value.resize(strSize);
                file.read(&constant.string_value[0], strSize);
                constant.type = ValueType::STRING;
            }

            file.close();

            bytecode->compiledAt = std::chrono::steady_clock::now();
            return bytecode;
        } catch (const std::exception&) {
            return nullptr;
        }
    }

    std::vector<std::string> ArabicJITCompiler::getPerformanceStats() const {
        std::vector<std::string> stats;
        stats.push_back("=== إحصائيات أداء JIT ===");

        auto cacheStats = cache.getStats();
        stats.insert(stats.end(), cacheStats.begin(), cacheStats.end());

        stats.push_back("حجم تخزين JIT: " + std::to_string(jitCompiler.getCacheSize()));

        return stats;
    }

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة خاصة
    // ══════════════════════════════════════════════════════════════

    std::string ArabicJITCompiler::computeHash(const std::string& source) const {
        return std::to_string(hasher(source));
    }

    std::vector<ArabicJITCompiler::BytecodeInstruction> ArabicJITCompiler::generateBytecode(
        const std::vector<std::shared_ptr<Command>>& commands) {

        std::vector<BytecodeInstruction> bytecode;

        for (const auto& cmd : commands) {
            switch (cmd->type) {
                case CommandType::ASSIGNMENT: {
                    // LOAD_CONST value
                    bytecode.emplace_back(Opcode::LOAD_CONST, 0); // index in constants
                    // STORE_VAR var_name
                    bytecode.emplace_back(Opcode::STORE_VAR, 0); // index in variable names
                    break;
                }
                case CommandType::PRINT: {
                    // LOAD_VAR or LOAD_CONST
                    bytecode.emplace_back(Opcode::LOAD_VAR, 0);
                    // CALL_NATIVE print
                    bytecode.emplace_back(Opcode::CALL_NATIVE, 0); // print function index
                    break;
                }
                case CommandType::CONDITION: {
                    // condition evaluation and jump
                    bytecode.emplace_back(Opcode::LOAD_VAR, 0); // condition var
                    bytecode.emplace_back(Opcode::JMP_IF_FALSE, 0); // jump target
                    break;
                }
                // إضافة المزيد من الحالات حسب الحاجة
                default:
                    bytecode.emplace_back(Opcode::NOP);
                    break;
            }
        }

        return bytecode;
    }

    void ArabicJITCompiler::optimizeInstructions(std::vector<BytecodeInstruction>& instructions) {
        // إزالة NOPs المتتالية
        instructions.erase(
            std::remove_if(instructions.begin(), instructions.end(),
                          [](const BytecodeInstruction& instr) {
                              return instr.opcode == Opcode::NOP;
                          }),
            instructions.end());

        // تحسينات إضافية يمكن إضافتها هنا
    }

    void ArabicJITCompiler::inlineFunctions(std::vector<BytecodeInstruction>& instructions) {
        // تبسيط للعرض - في الإصدار الكامل سيتم تحليل ودمج الدوال الصغيرة
    }

    void ArabicJITCompiler::extractConstantsAndVariables(
        const std::vector<std::shared_ptr<Command>>& commands,
        ArabicJITCompiler::CompiledBytecode& bytecode) {

        std::set<std::string> vars;
        std::set<std::string> consts;

        // مسح الأوامر لاستخراج المتغيرات والثوابت
        for (const auto& cmd : commands) {
            if (cmd->type == CommandType::ASSIGNMENT) {
                // في التنفيذ الفعلي، سنقوم باستخراج اسم المتغير والقيمة المسندة
                // هنا نستخدم قيم افتراضية للتوضيح
                vars.insert("متغير_عام");
                consts.insert("قيمة_افتراضية");
            }
        }

        bytecode.variableNames.assign(vars.begin(), vars.end());
        for (const auto& c : consts) {
            Value v;
            v.type = ValueType::STRING;
            v.string_value = c;
            bytecode.constants.push_back(v);
        }
    }

    // ══════════════════════════════════════════════════════════════
    // 🏃‍♂️ تنفيذ محرك التنفيذ JIT
    // ══════════════════════════════════════════════════════════════

    Value JITExecutionEngine::execute(const std::shared_ptr<ArabicJITCompiler::CompiledBytecode>& bytecode) {
        if (!bytecode || bytecode->code.empty()) {
            return Value{ValueType::NONE};
        }

        valueStack.clear();
        callStack.clear();
        instructionPointer = 0;

        auto startTime = std::chrono::high_resolution_clock::now();

        try {
            while (instructionPointer < bytecode->code.size()) {
                const auto& instr = bytecode->code[instructionPointer];
                totalInstructions++;

                Value result = Value{ValueType::NONE};

                switch (instr.opcode) {
                    case ArabicJITCompiler::Opcode::LOAD_CONST: {
                        if (instr.operand1 < bytecode->constants.size()) {
                            result = bytecode->constants[instr.operand1];
                        }
                        break;
                    }
                    case ArabicJITCompiler::Opcode::LOAD_VAR: {
                        if (instr.operand1 < bytecode->variableNames.size()) {
                            const auto& varName = bytecode->variableNames[instr.operand1];
                            if (globalVariables.count(varName)) {
                                result = globalVariables[varName];
                            }
                        }
                        break;
                    }
                    case ArabicJITCompiler::Opcode::STORE_VAR: {
                        if (!valueStack.empty() && instr.operand1 < bytecode->variableNames.size()) {
                            const auto& varName = bytecode->variableNames[instr.operand1];
                            globalVariables[varName] = valueStack.back();
                            valueStack.pop_back();
                        }
                        break;
                    }
                    case ArabicJITCompiler::Opcode::ADD: {
                        if (valueStack.size() >= 2) {
                            Value b = valueStack.back(); valueStack.pop_back();
                            Value a = valueStack.back(); valueStack.pop_back();
                            // تبسيط - في الإصدار الكامل سيتم التعامل مع الأنواع المختلفة
                            Value sum;
                            sum.type = ValueType::NUMBER;
                            sum.string_value = "0"; // placeholder
                            result = sum;
                        }
                        break;
                    }
                    case ArabicJITCompiler::Opcode::JMP: {
                        instructionPointer = instr.operand1;
                        continue;
                    }
                    case ArabicJITCompiler::Opcode::JMP_IF_FALSE: {
                        if (!valueStack.empty()) {
                            Value condition = valueStack.back();
                            valueStack.pop_back();
                            // تبسيط للشرط
                            if (condition.string_value == "false") {
                                instructionPointer = instr.operand1;
                                continue;
                            }
                        }
                        break;
                    }
                    case ArabicJITCompiler::Opcode::OPTIMIZED_ADD: {
                        optimizedInstructions++;
                        // نفس منطق ADD لكن محسّن
                        if (valueStack.size() >= 2) {
                            Value b = valueStack.back(); valueStack.pop_back();
                            Value a = valueStack.back(); valueStack.pop_back();
                            Value sum;
                            sum.type = ValueType::NUMBER;
                            sum.string_value = "optimized_0"; // placeholder
                            result = sum;
                        }
                        break;
                    }
                    default:
                        // تعليمات أخرى
                        break;
                }

                if (result.type != ValueType::NONE) {
                    valueStack.push_back(result);
                }

                instructionPointer++;
            }
        } catch (const std::exception& e) {
            std::cerr << "خطأ في تنفيذ JIT: " << e.what() << std::endl;
            return Value{ValueType::NONE};
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

        // تحديث إحصائيات البايت كود
        double executionTime = duration.count() / 1000.0; // مللي ثانية
        bytecode->averageExecutionTime =
            (bytecode->averageExecutionTime * bytecode->executionCount + executionTime) /
            (bytecode->executionCount + 1);

        return valueStack.empty() ? Value{ValueType::NONE} : valueStack.back();
    }

    void JITExecutionEngine::reset() {
        valueStack.clear();
        globalVariables.clear();
        callStack.clear();
        instructionPointer = 0;
    }

    void JITExecutionEngine::enableProfiling(bool enable) {
        // (سيتم تنفيذ التوصيف التفصيلي لاحقاً)
    }

    std::vector<std::string> JITExecutionEngine::getExecutionStats() const {
        std::vector<std::string> stats;
        stats.push_back("=== إحصائيات تنفيذ JIT ===");
        stats.push_back("إجمالي التعليمات: " + std::to_string(totalInstructions.load()));
        stats.push_back("التعليمات المحسّنة: " + std::to_string(optimizedInstructions.load()));
        stats.push_back("نسبة التحسين: " + std::to_string(getOptimizationRatio()) + "%");

        return stats;
    }

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة عامة
    // ══════════════════════════════════════════════════════════════

    JITExecutionEngine createJITEngine() {
        return JITExecutionEngine(ArabicJITCompiler::getInstance());
    }

    Value compileAndExecuteJIT(const std::string& sourceCode, const std::string& filename) {
        auto& jit = ArabicJITCompiler::getInstance();
        auto bytecode = jit.compileToBytecode(sourceCode, filename);
        return jit.executeBytecode(bytecode);
    }

    void warmupJITCache(const std::vector<std::string>& commonCode) {
        auto& jit = ArabicJITCompiler::getInstance();

        std::cout << "🔥 تسخين تخزين JIT...\n";
        for (const auto& code : commonCode) {
            jit.compileToBytecode(code);
        }
        std::cout << "✅ تم تسخين التخزين المؤقت\n";
    }

    void cleanupJITResources() {
        auto& jit = ArabicJITCompiler::getInstance();
        jit.clearCache();
        jit.getJIT().clearCache();

        // تنظيف ذاكرة إضافية
        ArabicMemoryManager::getInstance().collectGarbage();
    }

} // namespace ArabicLanguage
