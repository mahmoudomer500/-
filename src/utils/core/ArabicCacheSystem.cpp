#include "ArabicCacheSystem.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <algorithm>
#include <functional>

namespace ArabicLanguage {

    // ══════════════════════════════════════════════════════════════
    // 💾 تنفيذ نظام التخزين المؤقت الشامل
    // ══════════════════════════════════════════════════════════════

    // Singleton instance
    ArabicCacheSystem& ArabicCacheSystem::getInstance() {
        static ArabicCacheSystem instance;
        return instance;
    }

    ArabicCacheSystem::ArabicCacheSystem()
        : cacheCleaner([this]() { this->performCleanup(); }) {
    }

    ArabicCacheSystem::~ArabicCacheSystem() {
        stopAutoCleanup();
        clearAll();
    }

    // ══════════════════════════════════════════════════════════════
    // 🌳 تنفيذ مخزن AST
    // ══════════════════════════════════════════════════════════════

    bool ArabicCacheSystem::ASTCache::store(const std::string& key, const std::vector<std::shared_ptr<Command>>& ast) {
        std::unique_lock<std::shared_mutex> lock(astMutex);

        if (astMap.size() >= maxEntries) {
            // إزالة أقدم إدخال
            auto oldest = astMap.begin();
            for (auto it = astMap.begin(); it != astMap.end(); ++it) {
                // تبسيط - في الإصدار الكامل سيتم اختيار الأقل استخداماً
                oldest = it;
                break;
            }
            astMap.erase(oldest);
        }

        auto astCopy = std::make_shared<std::vector<std::shared_ptr<Command>>>(ast);
        astMap[key] = astCopy;

        return true;
    }

    std::shared_ptr<std::vector<std::shared_ptr<Command>>> ArabicCacheSystem::ASTCache::retrieve(const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(astMutex);
        auto it = astMap.find(key);
        return (it != astMap.end()) ? it->second : nullptr;
    }

    bool ArabicCacheSystem::ASTCache::contains(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(astMutex);
        return astMap.count(key) > 0;
    }

    void ArabicCacheSystem::ASTCache::clear() {
        std::unique_lock<std::shared_mutex> lock(astMutex);
        astMap.clear();
    }

    void ArabicCacheSystem::ASTCache::optimize() {
        std::unique_lock<std::shared_mutex> lock(astMutex);
        // تحسين بإزالة الأشجار غير المستخدمة (تبسيط)
        // في الإصدار الكامل سيتم تحليل استخدام الأشجار
    }

    std::vector<std::string> ArabicCacheSystem::ASTCache::getFrequentKeys() const {
        std::shared_lock<std::shared_mutex> lock(astMutex);
        std::vector<std::string> keys;
        for (const auto& pair : astMap) {
            keys.push_back(pair.first);
        }
        return keys;
    }

    // ══════════════════════════════════════════════════════════════
    // 🔤 تنفيذ مخزن الرموز
    // ══════════════════════════════════════════════════════════════

    bool ArabicCacheSystem::SymbolCache::store(const std::string& key, const SymbolTable& symbols) {
        std::unique_lock<std::shared_mutex> lock(symbolMutex);

        if (symbolMap.size() >= maxEntries) {
            // إزالة أقدم إدخال
            auto oldest = symbolMap.begin();
            symbolMap.erase(oldest);
        }

        auto symbolsCopy = std::make_shared<SymbolTable>(symbols);
        symbolMap[key] = symbolsCopy;

        return true;
    }

    std::shared_ptr<SymbolTable> ArabicCacheSystem::SymbolCache::retrieve(const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(symbolMutex);
        auto it = symbolMap.find(key);
        return (it != symbolMap.end()) ? it->second : nullptr;
    }

    bool ArabicCacheSystem::SymbolCache::contains(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(symbolMutex);
        return symbolMap.count(key) > 0;
    }

    void ArabicCacheSystem::SymbolCache::clear() {
        std::unique_lock<std::shared_mutex> lock(symbolMutex);
        symbolMap.clear();
    }

    void ArabicCacheSystem::SymbolCache::mergeSymbols(const std::string& key1, const std::string& key2) {
        std::unique_lock<std::shared_mutex> lock(symbolMutex);
        // دمج جداول الرموز (تبسيط)
        // في الإصدار الكامل سيتم دمج فعلي للرموز
    }

    std::vector<std::string> ArabicCacheSystem::SymbolCache::getUndefinedSymbols(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(symbolMutex);
        std::vector<std::string> undefined;
        // فحص الرموز غير المعرفة (تبسيط)
        return undefined;
    }

    // ══════════════════════════════════════════════════════════════
    // 🎯 تنفيذ مخزن الأنواع
    // ══════════════════════════════════════════════════════════════

    bool ArabicCacheSystem::TypeCache::store(const std::string& key,
                                            const std::unordered_map<std::string, ArabicTypeInference::TypeInfo>& types) {
        std::unique_lock<std::shared_mutex> lock(typeMutex);

        if (typeMap.size() >= maxEntries) {
            // إزالة أقدم إدخال
            auto oldest = typeMap.begin();
            typeMap.erase(oldest);
        }

        auto typesCopy = std::make_shared<std::unordered_map<std::string, ArabicTypeInference::TypeInfo>>(types);
        typeMap[key] = typesCopy;

        return true;
    }

    std::shared_ptr<std::unordered_map<std::string, ArabicTypeInference::TypeInfo>>
    ArabicCacheSystem::TypeCache::retrieve(const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(typeMutex);
        auto it = typeMap.find(key);
        return (it != typeMap.end()) ? it->second : nullptr;
    }

    bool ArabicCacheSystem::TypeCache::contains(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(typeMutex);
        return typeMap.count(key) > 0;
    }

    void ArabicCacheSystem::TypeCache::clear() {
        std::unique_lock<std::shared_mutex> lock(typeMutex);
        typeMap.clear();
    }

    void ArabicCacheSystem::TypeCache::updateType(const std::string& key,
                                                 const std::string& varName,
                                                 const ArabicTypeInference::TypeInfo& type) {
        std::unique_lock<std::shared_mutex> lock(typeMutex);
        auto it = typeMap.find(key);
        if (it != typeMap.end()) {
            (*it->second)[varName] = type;
        }
    }

    std::vector<std::string> ArabicCacheSystem::TypeCache::getTypeDependencies(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(typeMutex);
        std::vector<std::string> deps;
        auto it = typeMap.find(key);
        if (it != typeMap.end()) {
            for (const auto& typePair : *it->second) {
                deps.push_back(typePair.second.name);
            }
        }
        return deps;
    }

    // ══════════════════════════════════════════════════════════════
    // 💾 تنفيذ مخزن البايت كود
    // ══════════════════════════════════════════════════════════════

    bool ArabicCacheSystem::BytecodeCache::store(const std::string& key, const std::vector<uint8_t>& bytecode) {
        std::unique_lock<std::shared_mutex> lock(bytecodeMutex);

        if (bytecodeMap.size() >= maxEntries) {
            // إزالة أقدم إدخال
            auto oldest = bytecodeMap.begin();
            bytecodeMap.erase(oldest);
        }

        auto bytecodeCopy = std::make_shared<std::vector<uint8_t>>(bytecode);
        bytecodeMap[key] = bytecodeCopy;

        return true;
    }

    std::shared_ptr<std::vector<uint8_t>> ArabicCacheSystem::BytecodeCache::retrieve(const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(bytecodeMutex);
        auto it = bytecodeMap.find(key);
        return (it != bytecodeMap.end()) ? it->second : nullptr;
    }

    bool ArabicCacheSystem::BytecodeCache::contains(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(bytecodeMutex);
        return bytecodeMap.count(key) > 0;
    }

    void ArabicCacheSystem::BytecodeCache::clear() {
        std::unique_lock<std::shared_mutex> lock(bytecodeMutex);
        bytecodeMap.clear();
    }

    void ArabicCacheSystem::BytecodeCache::optimizeBytecode(const std::string& key) {
        std::unique_lock<std::shared_mutex> lock(bytecodeMutex);
        // تحسين البايت كود (تبسيط)
        // في الإصدار الكامل سيتم تحسين فعلي للبايت كود
    }

    size_t ArabicCacheSystem::BytecodeCache::getBytecodeSize(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(bytecodeMutex);
        auto it = bytecodeMap.find(key);
        return (it != bytecodeMap.end()) ? it->second->size() : 0;
    }

    // ══════════════════════════════════════════════════════════════
    // 💾 تنفيذ التخزين المستمر
    // ══════════════════════════════════════════════════════════════

    ArabicCacheSystem::PersistentStorage::PersistentStorage(const fs::path& dir)
        : cacheDir(dir) {
        // إنشاء مجلد التخزين إذا لم يكن موجوداً
        if (!fs::exists(cacheDir)) {
            try {
                fs::create_directories(cacheDir);
            } catch (...) {}
        }
    }

    void ArabicCacheSystem::PersistentStorage::setDirectory(const fs::path& dir) {
        std::lock_guard<std::mutex> lock(fileMutex);
        cacheDir = dir;
        if (!fs::exists(cacheDir)) {
            try {
                fs::create_directories(cacheDir);
            } catch (...) {}
        }
    }

    bool ArabicCacheSystem::PersistentStorage::saveToDisk(const std::string& key,
                                                         const std::string& dataType,
                                                         const std::string& data) {
        try {
            fs::path filePath = cacheDir / (key + "_" + dataType + ".cache");
            std::ofstream file(filePath, std::ios::binary);
            if (!file.is_open()) return false;

            // كتابة البيانات
            size_t dataSize = data.size();
            file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
            file.write(data.c_str(), dataSize);

            // تحديث الحجم الإجمالي
            totalStoredSize += dataSize + sizeof(dataSize);

            file.close();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    std::string ArabicCacheSystem::PersistentStorage::loadFromDisk(const std::string& key,
                                                                  const std::string& dataType) {
        try {
            fs::path filePath = cacheDir / (key + "_" + dataType + ".cache");
            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open()) return "";

            // قراءة البيانات
            size_t dataSize;
            file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

            std::string data(dataSize, '\0');
            file.read(&data[0], dataSize);

            file.close();
            return data;
        } catch (const std::exception&) {
            return "";
        }
    }

    bool ArabicCacheSystem::PersistentStorage::existsOnDisk(const std::string& key,
                                                           const std::string& dataType) const {
        fs::path filePath = cacheDir / (key + "_" + dataType + ".cache");
        return fs::exists(filePath);
    }

    void ArabicCacheSystem::PersistentStorage::cleanupOldEntries(std::chrono::hours maxAge) {
        try {
            auto now = std::chrono::system_clock::now();
            for (const auto& entry : fs::directory_iterator(cacheDir)) {
                if (fs::is_regular_file(entry)) {
                    auto ftime = fs::last_write_time(entry);
                    
                    // تحويل وقت الملف إلى system_clock
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                    );

                    if (now - sctp > maxAge) {
                        fs::remove(entry);
                    }
                }
            }
        } catch (const std::exception&) {
            // تجاهل الأخطاء في التنظيف
        }
    }

    void ArabicCacheSystem::PersistentStorage::clearDiskCache() {
        try {
            for (const auto& entry : fs::directory_iterator(cacheDir)) {
                if (fs::is_regular_file(entry)) {
                    fs::remove(entry);
                }
            }
            totalStoredSize = 0;
        } catch (const std::exception&) {
            // تجاهل الأخطاء
        }
    }

    std::vector<std::string> ArabicCacheSystem::PersistentStorage::getStoredKeys(const std::string& dataType) const {
        std::vector<std::string> keys;
        try {
            for (const auto& entry : fs::directory_iterator(cacheDir)) {
                if (fs::is_regular_file(entry)) {
                    std::string filename = entry.path().filename().string();
                    size_t underscorePos = filename.find_last_of('_');
                    if (underscorePos != std::string::npos) {
                        std::string key = filename.substr(0, underscorePos);
                        std::string type = filename.substr(underscorePos + 1);
                        type = type.substr(0, type.find_last_of('.'));

                        if (dataType.empty() || type == dataType) {
                            keys.push_back(key);
                        }
                    }
                }
            }
        } catch (const std::exception&) {
            // تجاهل الأخطاء
        }
        return keys;
    }

    // ══════════════════════════════════════════════════════════════
    // 🧹 تنفيذ مدير التنظيف التلقائي
    // ══════════════════════════════════════════════════════════════

    ArabicCacheSystem::CacheCleaner::CacheCleaner(std::function<void()> callback)
        : cleanupCallback(std::move(callback)) {
    }

    ArabicCacheSystem::CacheCleaner::~CacheCleaner() {
        stop();
    }

    void ArabicCacheSystem::CacheCleaner::start() {
        if (running) return;

        running = true;
        cleanerThread = std::thread([this]() {
            while (running) {
                std::this_thread::sleep_for(cleanupInterval);
                if (running && cleanupCallback) {
                    cleanupCallback();
                }
            }
        });
    }

    void ArabicCacheSystem::CacheCleaner::stop() {
        if (!running) return;

        running = false;
        if (cleanerThread.joinable()) {
            cleanerThread.join();
        }
    }

    void ArabicCacheSystem::CacheCleaner::setCleanupInterval(std::chrono::minutes interval) {
        cleanupInterval = interval;
    }

    // ══════════════════════════════════════════════════════════════
    // 🎯 تنفيذ واجهة الاستخدام العامة
    // ══════════════════════════════════════════════════════════════

    bool ArabicCacheSystem::storeAST(const std::string& key, const std::vector<std::shared_ptr<Command>>& ast) {
        bool success = astCache.store(key, ast);
        if (success && persistentEnabled) {
            // حفظ على القرص (تبسيط - في الإصدار الكامل سيتم تسلسل فعلي)
            std::string data = "AST:" + key; // placeholder
            persistentStorage.saveToDisk(key, "ast", data);
        }
        updateStats("store_ast", false, estimateDataSize(ast));
        return success;
    }

    std::shared_ptr<std::vector<std::shared_ptr<Command>>> ArabicCacheSystem::getAST(const std::string& key) {
        auto result = astCache.retrieve(key);
        bool hit = (result.get() != nullptr);
        updateStats("get_ast", hit);
        return result;
    }

    bool ArabicCacheSystem::storeSymbols(const std::string& key, const SymbolTable& symbols) {
        bool success = symbolCache.store(key, symbols);
        if (success && persistentEnabled) {
            // حفظ على القرص (تبسيط)
            std::string data = "SYMBOLS:" + key; // placeholder
            persistentStorage.saveToDisk(key, "symbols", data);
        }
        updateStats("store_symbols", false, sizeof(SymbolTable));
        return success;
    }

    std::shared_ptr<SymbolTable> ArabicCacheSystem::getSymbols(const std::string& key) {
        auto result = symbolCache.retrieve(key);
        bool hit = (result.get() != nullptr);
        updateStats("get_symbols", hit);
        return result;
    }

    bool ArabicCacheSystem::storeTypes(const std::string& key,
                                     const std::unordered_map<std::string, ArabicTypeInference::TypeInfo>& types) {
        bool success = typeCache.store(key, types);
        if (success && persistentEnabled) {
            // حفظ على القرص (تبسيط)
            std::string data = "TYPES:" + key; // placeholder
            persistentStorage.saveToDisk(key, "types", data);
        }
        updateStats("store_types", false, types.size() * sizeof(ArabicTypeInference::TypeInfo));
        return success;
    }

    std::shared_ptr<std::unordered_map<std::string, ArabicTypeInference::TypeInfo>>
    ArabicCacheSystem::getTypes(const std::string& key) {
        auto result = typeCache.retrieve(key);
        bool hit = (result.get() != nullptr);
        updateStats("get_types", hit);
        return result;
    }

    bool ArabicCacheSystem::storeBytecode(const std::string& key, const std::vector<uint8_t>& bytecode) {
        bool success = bytecodeCache.store(key, bytecode);
        if (success && persistentEnabled) {
            // حفظ على القرص (تبسيط)
            std::string data = "BYTECODE:" + key; // placeholder
            persistentStorage.saveToDisk(key, "bytecode", data);
        }
        updateStats("store_bytecode", false, bytecode.size());
        return success;
    }

    std::shared_ptr<std::vector<uint8_t>> ArabicCacheSystem::getBytecode(const std::string& key) {
        auto result = bytecodeCache.retrieve(key);
        bool hit = (result.get() != nullptr);
        updateStats("get_bytecode", hit);
        return result;
    }

    void ArabicCacheSystem::clearAll() {
        astCache.clear();
        symbolCache.clear();
        typeCache.clear();
        bytecodeCache.clear();
        persistentStorage.clearDiskCache();

        std::unique_lock<std::shared_mutex> lock(statsMutex);
        stats.totalEntries = 0;
        stats.totalSize = 0;
        stats.totalHits = 0;
        stats.totalMisses = 0;
    }

    void ArabicCacheSystem::optimizeAll() {
        astCache.optimize();
        // تحسين باقي المخازن (تبسيط)
        enforceMemoryLimits();
    }

    ArabicCacheSystem::CacheStats ArabicCacheSystem::getStats() const {
        std::shared_lock<std::shared_mutex> lock(statsMutex);
        CacheStats currentStats = stats;

        // تحديث الإحصائيات الحية
        currentStats.totalEntries = astCache.size() + symbolCache.size() +
                                   typeCache.size() + bytecodeCache.size();

        size_t totalRequests = currentStats.totalHits + currentStats.totalMisses;
        currentStats.hitRate = totalRequests > 0 ?
            (currentStats.totalHits * 100.0 / totalRequests) : 0.0;

        return currentStats;
    }

    void ArabicCacheSystem::setMaxMemorySize(size_t bytes) {
        maxMemorySize = bytes;
        enforceMemoryLimits();
    }

    void ArabicCacheSystem::setCacheDirectory(const fs::path& dir) {
        // إعادة إنشاء التخزين المستمر مع المجلد الجديد
        persistentStorage.setDirectory(dir);
    }

    void ArabicCacheSystem::enablePersistentStorage(bool enable) {
        persistentEnabled = enable;
    }

    void ArabicCacheSystem::startAutoCleanup() {
        cacheCleaner.start();
    }

    void ArabicCacheSystem::stopAutoCleanup() {
        cacheCleaner.stop();
    }

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة خاصة
    // ══════════════════════════════════════════════════════════════

    void ArabicCacheSystem::updateStats(const std::string& operation, bool hit, size_t dataSize) {
        std::unique_lock<std::shared_mutex> lock(statsMutex);
        if (hit) {
            stats.totalHits++;
        } else {
            stats.totalMisses++;
        }
        stats.totalSize += dataSize;
    }

    void ArabicCacheSystem::enforceMemoryLimits() {
        // فرض حدود الذاكرة (تبسيط)
        // في الإصدار الكامل سيتم تطبيق حدود دقيقة
    }

    std::string ArabicCacheSystem::generateCacheKey(const std::string& baseKey, const std::string& dataType) const {
        return baseKey + "_" + dataType;
    }

    void ArabicCacheSystem::performCleanup() {
        // تنظيف الإدخالات القديمة
        persistentStorage.cleanupOldEntries();

        // تنظيف الذاكرة إذا لزم الأمر
        enforceMemoryLimits();

        // تحسين المخازن
        optimizeAll();
    }

    void ArabicCacheSystem::cleanupTask() {
        // مهمة التنظيف الثابتة
        getInstance().performCleanup();
    }

    // ══════════════════════════════════════════════════════════════
    // 🔧 دوال مساعدة عامة
    // ══════════════════════════════════════════════════════════════

    std::string createCacheKey(const std::string& source, const std::string& context) {
        std::hash<std::string> hasher;
        std::string combined = source + (context.empty() ? "" : "|" + context);
        return std::to_string(hasher(combined));
    }

    bool isValidCacheKey(const std::string& key) {
        // فحص بسيط لصحة المفتاح
        return !key.empty() && key.length() >= 8;
    }

    size_t estimateDataSize(const std::vector<std::shared_ptr<Command>>& ast) {
        // تقدير حجم AST (تبسيط)
        return ast.size() * sizeof(std::shared_ptr<Command>);
    }

    std::string compressData(const std::string& data) {
        // ضغط بسيط (في الإصدار الكامل سيتم ضغط فعلي)
        return data; // لا ضغط حالياً
    }

    std::string decompressData(const std::string& compressedData) {
        // فك ضغط بسيط
        return compressedData; // لا فك ضغط حالياً
    }

    void cleanupCacheDirectory(const fs::path& cacheDir, std::chrono::hours maxAge) {
        ArabicCacheSystem::PersistentStorage storage(cacheDir);
        storage.cleanupOldEntries(maxAge);
    }

    void exportCacheStats(const ArabicCacheSystem::CacheStats& stats, const fs::path& outputFile) {
        try {
            std::ofstream file(outputFile);
            if (file.is_open()) {
                file << "=== إحصائيات التخزين المؤقت ===\n";
                file << "إجمالي الإدخالات: " << stats.totalEntries << "\n";
                file << "إجمالي الحجم: " << stats.totalSize << " بايت\n";
                file << "معدل الإصابة: " << stats.hitRate << "%\n";
                file << "إجمالي الإصابات: " << stats.totalHits << "\n";
                file << "إجمالي الإخفاقات: " << stats.totalMisses << "\n";
                file.close();
            }
        } catch (const std::exception&) {
            // تجاهل الأخطاء
        }
    }

} // namespace ArabicLanguage
