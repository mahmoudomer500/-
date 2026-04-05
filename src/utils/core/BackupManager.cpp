// BackupManager.cpp - تطبيق مدير نظام النسخ الاحتياطي
#include "BackupManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

namespace ArabicLanguage {

BackupManager::BackupManager()
    : baseBackupDirectory("./backups"),
      autoBackupEnabled(false),
      backupInterval(std::chrono::minutes(60)) { // ساعة واحدة

    std::cout << "[INFO] Backup Manager initialized" << std::endl;

    // إنشاء مجلد النسخ الاحتياطي إذا لم يكن موجوداً
    if (!fs::exists(baseBackupDirectory)) {
        try {
            fs::create_directories(baseBackupDirectory);
            std::cout << "[INFO] Created backup directory: " << baseBackupDirectory << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to create backup directory: " << e.what() << std::endl;
        }
    }
}

bool BackupManager::createBackup(const BackupConfig& config) {
    std::cout << "[INFO] Creating backup: " << config.backupName << std::endl;

    try {
        // إنشاء معلومات النسخ الاحتياطي
        BackupInfo info;
        info.backupId = generateBackupId();
        info.backupName = config.backupName;
        info.type = config.type;
        info.status = BackupStatus::IN_PROGRESS;
        info.creationTime = std::chrono::system_clock::now();
        info.sourcePath = config.sourcePath;
        info.destinationPath = config.destinationPath + "/" + info.backupId;

        // إنشاء مجلد النسخ الاحتياطي
        fs::create_directories(info.destinationPath);

        // تنفيذ النسخ الاحتياطي حسب النوع
        bool success = false;
        switch (config.type) {
            case BackupType::FULL:
                success = performFullBackup(config, info);
                break;
            case BackupType::INCREMENTAL:
                success = performIncrementalBackup(config, info);
                break;
            case BackupType::DIFFERENTIAL:
                // تنفيذ نسخ احتياطي تفاضلي (مبسط)
                success = performFullBackup(config, info);
                break;
            case BackupType::SNAPSHOT:
                // لقطة فورية (نسخ كامل سريع)
                success = performFullBackup(config, info);
                break;
        }

        // تحديث حالة النسخ الاحتياطي
        info.completionTime = std::chrono::system_clock::now();
        info.status = success ? BackupStatus::COMPLETED : BackupStatus::FAILED;

        // حفظ معلومات النسخ الاحتياطي
        updateBackupHistory(info);

        if (success) {
            std::cout << "[SUCCESS] Backup created: " << info.backupId << std::endl;
            std::cout << "[INFO] Files: " << info.fileCount << ", Size: " << info.originalSize << " bytes" << std::endl;
        } else {
            std::cout << "[ERROR] Backup failed: " << config.backupName << std::endl;
        }

        return success;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Backup creation failed: " << e.what() << std::endl;
        return false;
    }
}

bool BackupManager::restoreBackup(const std::string& backupId, const std::string& restorePath) {
    std::cout << "[INFO] Restoring backup: " << backupId << " to " << restorePath << std::endl;

    auto it = backupHistory.find(backupId);
    if (it == backupHistory.end()) {
        std::cerr << "[ERROR] Backup not found: " << backupId << std::endl;
        return false;
    }

    const BackupInfo& info = it->second;

    try {
        // إنشاء مجلد الاستعادة
        fs::create_directories(restorePath);

        // نسخ الملفات من النسخ الاحتياطي
        std::string backupPath = info.destinationPath;

        if (fs::exists(backupPath)) {
            // نسخ جميع الملفات
            for (const auto& entry : fs::recursive_directory_iterator(backupPath)) {
                if (fs::is_regular_file(entry)) {
                    std::string relativePath = fs::relative(entry.path(), backupPath).string();
                    fs::path restoreFile = fs::path(restorePath) / relativePath;

                    // إنشاء المجلدات الفرعية حسب الحاجة
                    fs::create_directories(restoreFile.parent_path());

                    // نسخ الملف
                    fs::copy_file(entry.path(), restoreFile, fs::copy_options::overwrite_existing);
                }
            }

            std::cout << "[SUCCESS] Backup restored: " << backupId << std::endl;
            return true;
        } else {
            std::cerr << "[ERROR] Backup files not found: " << backupPath << std::endl;
            return false;
        }

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Backup restoration failed: " << e.what() << std::endl;
        return false;
    }
}

bool BackupManager::verifyBackup(const std::string& backupId) {
    std::cout << "[INFO] Verifying backup: " << backupId << std::endl;

    auto it = backupHistory.find(backupId);
    if (it == backupHistory.end()) {
        std::cerr << "[ERROR] Backup not found: " << backupId << std::endl;
        return false;
    }

    const BackupInfo& info = it->second;

    // فحص وجود مجلد النسخ الاحتياطي
    if (!fs::exists(info.destinationPath)) {
        std::cerr << "[ERROR] Backup directory does not exist" << std::endl;
        return false;
    }

    // فحص عدد الملفات
    size_t fileCount = 0;
    for (const auto& entry : fs::recursive_directory_iterator(info.destinationPath)) {
        if (fs::is_regular_file(entry)) {
            fileCount++;
        }
    }

    if (fileCount != info.fileCount) {
        std::cerr << "[ERROR] File count mismatch: expected " << info.fileCount
                  << ", found " << fileCount << std::endl;
        return false;
    }

    // تحديث حالة النسخ الاحتياطي
    BackupInfo updatedInfo = info;
    updatedInfo.status = BackupStatus::VERIFIED;
    updateBackupHistory(updatedInfo);

    std::cout << "[SUCCESS] Backup verified: " << backupId << std::endl;
    return true;
}

bool BackupManager::deleteBackup(const std::string& backupId) {
    std::cout << "[INFO] Deleting backup: " << backupId << std::endl;

    auto it = backupHistory.find(backupId);
    if (it == backupHistory.end()) {
        std::cerr << "[ERROR] Backup not found: " << backupId << std::endl;
        return false;
    }

    const BackupInfo& info = it->second;

    try {
        // حذف مجلد النسخ الاحتياطي
        if (fs::exists(info.destinationPath)) {
            fs::remove_all(info.destinationPath);
            std::cout << "[INFO] Backup directory removed: " << info.destinationPath << std::endl;
        }

        // إزالة من التاريخ
        backupHistory.erase(backupId);

        std::cout << "[SUCCESS] Backup deleted: " << backupId << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Backup deletion failed: " << e.what() << std::endl;
        return false;
    }
}

void BackupManager::enableAutoBackup(std::chrono::minutes interval) {
    autoBackupEnabled = true;
    backupInterval = interval;
    std::cout << "[INFO] Auto backup enabled with interval: " << interval.count() << " minutes" << std::endl;
}

void BackupManager::disableAutoBackup() {
    autoBackupEnabled = false;
    std::cout << "[INFO] Auto backup disabled" << std::endl;
}

void BackupManager::performScheduledBackup() {
    if (!autoBackupEnabled) return;

    static auto lastBackup = std::chrono::system_clock::now() - backupInterval;

    auto now = std::chrono::system_clock::now();
    if (now - lastBackup >= backupInterval) {
        std::cout << "[INFO] Performing scheduled backup..." << std::endl;

        // إنشاء نسخ احتياطي مجدول للمشروع الحالي
        BackupConfig config;
        config.backupName = "scheduled_backup_" + std::to_string(std::chrono::system_clock::to_time_t(now));
        config.sourcePath = "."; // المجلد الحالي
        config.destinationPath = baseBackupDirectory;
        config.type = BackupType::INCREMENTAL;
        config.retentionPeriod = std::chrono::hours(24 * 7); // أسبوع
        config.enableCompression = true;
        config.enableEncryption = false;

        if (createBackup(config)) {
            lastBackup = now;
            std::cout << "[SUCCESS] Scheduled backup completed" << std::endl;
        } else {
            std::cout << "[WARNING] Scheduled backup failed" << std::endl;
        }
    }
}

std::vector<BackupInfo> BackupManager::listBackups() const {
    std::vector<BackupInfo> backups;
    for (const auto& pair : backupHistory) {
        backups.push_back(pair.second);
    }

    // ترتيب حسب وقت الإنشاء (الأحدث أولاً)
    std::sort(backups.begin(), backups.end(),
              [](const BackupInfo& a, const BackupInfo& b) {
                  return a.creationTime > b.creationTime;
              });

    return backups;
}

BackupInfo BackupManager::getBackupInfo(const std::string& backupId) const {
    auto it = backupHistory.find(backupId);
    if (it != backupHistory.end()) {
        return it->second;
    }

    return BackupInfo{}; // إرجاع نسخة فارغة
}

bool BackupManager::cleanupOldBackups() {
    std::cout << "[INFO] Cleaning up old backups..." << std::endl;

    auto now = std::chrono::system_clock::now();
    std::vector<std::string> backupsToDelete;

    for (const auto& pair : backupHistory) {
        const BackupInfo& info = pair.second;

        // فحص الفترة المحتفظ بها (مبسط)
        auto age = now - info.creationTime;
        if (age > std::chrono::hours(24 * 30)) { // أكثر من 30 يوماً
            backupsToDelete.push_back(pair.first);
        }
    }

    bool allDeleted = true;
    for (const auto& backupId : backupsToDelete) {
        if (!deleteBackup(backupId)) {
            allDeleted = false;
        }
    }

    std::cout << "[INFO] Cleaned up " << backupsToDelete.size() << " old backups" << std::endl;
    return allDeleted;
}

std::map<std::string, size_t> BackupManager::getBackupStatistics() const {
    std::map<std::string, size_t> stats;

    stats["total_backups"] = backupHistory.size();

    size_t totalSize = 0;
    size_t totalFiles = 0;
    size_t failedBackups = 0;
    size_t verifiedBackups = 0;

    for (const auto& pair : backupHistory) {
        const BackupInfo& info = pair.second;
        totalSize += info.originalSize;
        totalFiles += info.fileCount;

        if (info.status == BackupStatus::FAILED) failedBackups++;
        if (info.status == BackupStatus::VERIFIED) verifiedBackups++;
    }

    stats["total_size"] = totalSize;
    stats["total_files"] = totalFiles;
    stats["failed_backups"] = failedBackups;
    stats["verified_backups"] = verifiedBackups;

    return stats;
}

bool BackupManager::validateBackupIntegrity(const std::string& backupId) {
    return verifyBackup(backupId);
}

std::vector<std::string> BackupManager::getBackupIssues(const std::string& backupId) {
    std::vector<std::string> issues;

    auto it = backupHistory.find(backupId);
    if (it == backupHistory.end()) {
        issues.push_back("Backup not found");
        return issues;
    }

    const BackupInfo& info = it->second;

    // فحص المشاكل الأساسية
    if (info.status == BackupStatus::FAILED) {
        issues.push_back("Backup creation failed");
    }

    if (info.fileCount == 0) {
        issues.push_back("No files in backup");
    }

    if (!fs::exists(info.destinationPath)) {
        issues.push_back("Backup directory missing");
    }

    // إضافة أخطاء محددة
    issues.insert(issues.end(), info.errors.begin(), info.errors.end());

    return issues;
}

size_t BackupManager::estimateBackupSize(const BackupConfig& config) const {
    size_t totalSize = 0;

    try {
        if (fs::exists(config.sourcePath)) {
            for (const auto& entry : fs::recursive_directory_iterator(config.sourcePath)) {
                if (fs::is_regular_file(entry) &&
                    shouldIncludeFile(entry.path().string(), config.excludePatterns)) {
                    totalSize += fs::file_size(entry);
                }
            }
        }
    } catch (const std::exception&) {
        // تجاهل الأخطاء في التقدير
    }

    return totalSize;
}

std::string BackupManager::generateBackupId() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << "backup_" << time << "_" << dis(gen);
    return ss.str();
}

bool BackupManager::performFullBackup(const BackupConfig& config, BackupInfo& info) {
    try {
        std::cout << "[INFO] Performing full backup from " << config.sourcePath << std::endl;

        if (!fs::exists(config.sourcePath)) {
            info.errors.push_back("Source path does not exist: " + config.sourcePath);
            return false;
        }

        // نسخ جميع الملفات
        for (const auto& entry : fs::recursive_directory_iterator(config.sourcePath)) {
            if (fs::is_regular_file(entry)) {
                std::string relativePath = fs::relative(entry.path(), config.sourcePath).string();

                if (shouldIncludeFile(relativePath, config.excludePatterns)) {
                    fs::path destPath = fs::path(info.destinationPath) / relativePath;

                    // إنشاء المجلدات الفرعية
                    fs::create_directories(destPath.parent_path());

                    // نسخ الملف
                    fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);

                    info.fileCount++;
                    info.originalSize += fs::file_size(entry.path());
                    info.includedFiles.push_back(relativePath);

                    std::cout << "[BACKUP] " << relativePath << std::endl;
                }
            }
        }

        // ضغط النسخ الاحتياطي إذا طُلب
        if (config.enableCompression) {
            if (compressBackup(info.destinationPath, info.destinationPath + ".compressed")) {
                info.compressedSize = fs::file_size(info.destinationPath + ".compressed");
            }
        }

        // تشفير النسخ الاحتياطي إذا طُلب
        if (config.enableEncryption && !config.encryptionKey.empty()) {
            encryptBackup(info.destinationPath, config.encryptionKey);
        }

        return true;

    } catch (const std::exception& e) {
        info.errors.push_back("Backup failed: " + std::string(e.what()));
        return false;
    }
}

bool BackupManager::performIncrementalBackup(const BackupConfig& config, BackupInfo& info) {
    // نسخ احتياطي تزايدي مبسط (نفس النسخ الكامل للوقت الحالي)
    return performFullBackup(config, info);
}

bool BackupManager::compressBackup(const std::string& sourcePath, const std::string& destPath) {
    // ضغط بسيط (يمكن تحسينه لاحقاً)
    try {
        // للتبسيط، فقط نسخ الملفات كما هي
        // يمكن إضافة خوارزمية ضغط حقيقية هنا
        std::cout << "[INFO] Compression not implemented yet, skipping" << std::endl;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool BackupManager::encryptBackup(const std::string& filePath, const std::string& key) {
    // تشفير بسيط (يمكن تحسينه لاحقاً)
    try {
        std::cout << "[INFO] Encryption not implemented yet, skipping" << std::endl;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool BackupManager::decryptBackup(const std::string& filePath, const std::string& key) {
    // فك تشفير بسيط
    try {
        std::cout << "[INFO] Decryption not implemented yet, skipping" << std::endl;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool BackupManager::shouldIncludeFile(const std::string& filePath,
                                      const std::vector<std::string>& excludePatterns) const {
    // فحص ما إذا كان يجب تضمين الملف
    for (const auto& pattern : excludePatterns) {
        // فحص بسيط للنمط (يمكن تحسينه باستخدام regex)
        if (filePath.find(pattern) != std::string::npos) {
            return false;
        }
    }

    return true;
}

void BackupManager::updateBackupHistory(const BackupInfo& info) {
    backupHistory[info.backupId] = info;

    // حفظ معلومات النسخ الاحتياطي في ملف
    try {
        std::string metadataFile = info.destinationPath + "/backup_metadata.txt";
        std::ofstream metadata(metadataFile);

        if (metadata.is_open()) {
            metadata << "Backup ID: " << info.backupId << "\n";
            metadata << "Name: " << info.backupName << "\n";
            metadata << "Type: " << static_cast<int>(info.type) << "\n";
            metadata << "Status: " << static_cast<int>(info.status) << "\n";
            metadata << "Creation Time: " << std::chrono::system_clock::to_time_t(info.creationTime) << "\n";
            metadata << "File Count: " << info.fileCount << "\n";
            metadata << "Original Size: " << info.originalSize << "\n";
            metadata << "Compressed Size: " << info.compressedSize << "\n";
            metadata << "Source Path: " << info.sourcePath << "\n";
            metadata << "Destination Path: " << info.destinationPath << "\n";

            metadata.close();
        }
    } catch (const std::exception&) {
        // تجاهل أخطاء حفظ المعلومات
    }
}

void BackupManager::logBackupOperation(const std::string& operation, const std::string& details) {
    std::cout << "[BACKUP " << operation << "] " << details << std::endl;
}

// Factory function
std::unique_ptr<BackupManager> createBackupManager() {
    return std::make_unique<BackupManager>();
}

} // namespace ArabicLanguage