// BackupManager.h - مدير نظام النسخ الاحتياطي
#ifndef BACKUP_MANAGER_H
#define BACKUP_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <filesystem>

namespace ArabicLanguage {

enum class BackupType {
    FULL,       // نسخ احتياطي كامل
    INCREMENTAL,// نسخ احتياطي تزايدي
    DIFFERENTIAL,// نسخ احتياطي تفاضلي
    SNAPSHOT    // لقطة فورية
};

enum class BackupStatus {
    PENDING,
    IN_PROGRESS,
    COMPLETED,
    FAILED,
    VERIFIED
};

struct BackupConfig {
    std::string backupName;
    std::string sourcePath;
    std::string destinationPath;
    BackupType type;
    std::chrono::hours retentionPeriod;
    bool enableCompression;
    bool enableEncryption;
    std::string encryptionKey;
    std::vector<std::string> excludePatterns;
    bool autoCleanup;
};

struct BackupInfo {
    std::string backupId;
    std::string backupName;
    BackupType type;
    BackupStatus status;
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point completionTime;
    size_t originalSize;
    size_t compressedSize;
    size_t fileCount;
    std::string sourcePath;
    std::string destinationPath;
    std::vector<std::string> includedFiles;
    std::vector<std::string> errors;
};

// مدير نظام النسخ الاحتياطي
class BackupManager {
private:
    std::map<std::string, BackupConfig> backupConfigs;
    std::map<std::string, BackupInfo> backupHistory;
    std::string baseBackupDirectory;
    bool autoBackupEnabled;
    std::chrono::minutes backupInterval;

public:
    BackupManager();
    ~BackupManager() = default;

    // الواجهة العامة
    bool createBackup(const BackupConfig& config);
    bool restoreBackup(const std::string& backupId, const std::string& restorePath);
    bool verifyBackup(const std::string& backupId);
    bool deleteBackup(const std::string& backupId);

    // دوال مجدولة
    void enableAutoBackup(std::chrono::minutes interval);
    void disableAutoBackup();
    void performScheduledBackup();

    // دوال الإدارة
    std::vector<BackupInfo> listBackups() const;
    BackupInfo getBackupInfo(const std::string& backupId) const;
    bool cleanupOldBackups();
    std::map<std::string, size_t> getBackupStatistics() const;

    // دوال التشخيص
    bool validateBackupIntegrity(const std::string& backupId);
    std::vector<std::string> getBackupIssues(const std::string& backupId);
    size_t estimateBackupSize(const BackupConfig& config) const;

private:
    std::string generateBackupId() const;
    bool performFullBackup(const BackupConfig& config, BackupInfo& info);
    bool performIncrementalBackup(const BackupConfig& config, BackupInfo& info);
    bool compressBackup(const std::string& sourcePath, const std::string& destPath);
    bool encryptBackup(const std::string& filePath, const std::string& key);
    bool decryptBackup(const std::string& filePath, const std::string& key);
    bool shouldIncludeFile(const std::string& filePath, const std::vector<std::string>& excludePatterns) const;
    void updateBackupHistory(const BackupInfo& info);
    void logBackupOperation(const std::string& operation, const std::string& details);
};

// Factory function لإنشاء مدير النسخ الاحتياطي
std::unique_ptr<BackupManager> createBackupManager();

} // namespace ArabicLanguage

#endif // BACKUP_MANAGER_H