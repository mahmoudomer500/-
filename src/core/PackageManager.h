// PackageManager.h - مدير الحزم الشامل
// الأسبوع الرابع من الشهر الرابع: نظام إدارة الحزم
#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <chrono>

namespace ArabicLanguage {

// إصدار الحزمة
struct PackageVersion {
    int major;
    int minor;
    int patch;
    std::string prerelease;
    std::string build;
    
    PackageVersion() : major(0), minor(0), patch(0) {}
    PackageVersion(int mj, int mn, int pt) : major(mj), minor(mn), patch(pt) {}
    
    std::string toString() const;
    static PackageVersion fromString(const std::string& versionStr);
    
    bool operator==(const PackageVersion& other) const;
    bool operator<(const PackageVersion& other) const;
    bool operator>(const PackageVersion& other) const;
    bool operator<=(const PackageVersion& other) const;
    bool operator>=(const PackageVersion& other) const;
};

// تبعية الحزمة
struct PackageDependency {
    std::string packageName;
    PackageVersion minVersion;
    PackageVersion maxVersion;
    bool required;
    std::string description;
    
    PackageDependency() : required(true) {}
    PackageDependency(const std::string& name, const PackageVersion& minVer)
        : packageName(name), minVersion(minVer), required(true) {}
    
    bool isSatisfiedBy(const PackageVersion& version) const;
};

// معلومات الحزمة
struct PackageInfo {
    std::string name;
    PackageVersion version;
    std::string author;
    std::string description;
    std::string license;
    std::string homepage;
    std::vector<std::string> keywords;
    std::vector<PackageDependency> dependencies;
    std::vector<std::string> conflicts;
    std::vector<std::string> files;
    size_t size;
    std::chrono::system_clock::time_point installDate;
    std::chrono::system_clock::time_point updateDate;
    bool isInstalled;
    std::string installPath;
    
    PackageInfo() : size(0), isInstalled(false) {}
    
    std::string getVersionString() const { return version.toString(); }
};

// حالة التثبيت
enum class InstallStatus {
    SUCCESS,
    FAILED,
    ALREADY_INSTALLED,
    DEPENDENCY_FAILED,
    CONFLICT_DETECTED,
    INVALID_PACKAGE,
    NETWORK_ERROR
};

// نتيجة التثبيت
struct InstallResult {
    InstallStatus status;
    std::string message;
    std::vector<std::string> installedPackages;
    std::vector<std::string> failedPackages;
    std::vector<std::string> warnings;
    
    InstallResult() : status(InstallStatus::FAILED) {}
};

// مدير الحزم الشامل
class PackageManager {
private:
    std::map<std::string, PackageInfo> installedPackages;
    std::vector<std::string> repositoryUrls;
    std::string packagesDirectory;
    std::string cacheDirectory;
    bool verbose;
    
public:
    PackageManager();
    ~PackageManager() = default;
    
    // الواجهة الرئيسية
    void initialize();
    void setPackagesDirectory(const std::string& dir);
    void setCacheDirectory(const std::string& dir);
    void addRepository(const std::string& url);
    void setVerbose(bool v) { verbose = v; }
    
    // تثبيت الحزم
    InstallResult installPackage(const std::string& packageName, const std::string& version = "latest");
    InstallResult installPackageFromFile(const std::string& packageFile);
    InstallResult installPackages(const std::vector<std::string>& packageNames);
    
    // إزالة الحزم
    bool uninstallPackage(const std::string& packageName, bool removeDependencies = false);
    bool uninstallPackages(const std::vector<std::string>& packageNames);
    
    // تحديث الحزم
    bool updatePackage(const std::string& packageName);
    bool updateAllPackages();
    std::vector<std::string> getUpdatablePackages() const;
    
    // استعلام الحزم
    bool isInstalled(const std::string& packageName) const;
    PackageInfo getPackageInfo(const std::string& packageName) const;
    std::vector<PackageInfo> listInstalledPackages() const;
    std::vector<PackageInfo> searchPackages(const std::string& query) const;
    std::vector<PackageInfo> getPackageVersions(const std::string& packageName) const;
    
    // إدارة التبعيات
    std::vector<std::string> resolveDependencies(const std::string& packageName) const;
    std::vector<std::string> getDependents(const std::string& packageName) const;
    bool checkDependencies(const std::string& packageName) const;
    
    // معلومات النظام
    size_t getTotalInstalledSize() const;
    size_t getPackageCount() const;
    std::map<std::string, size_t> getStatistics() const;
    
    // تنظيف
    void cleanCache();
    void cleanOrphanedPackages();
    
private:
    PackageInfo fetchPackageInfo(const std::string& packageName, const std::string& version) const;
    bool downloadPackage(const PackageInfo& packageInfo, const std::string& destination) const;
    bool extractPackage(const std::string& packageFile, const std::string& destination) const;
    bool installPackageFiles(const PackageInfo& packageInfo) const;
    bool registerPackage(const PackageInfo& packageInfo);
    bool unregisterPackage(const std::string& packageName);
    void loadInstalledPackages();
    void saveInstalledPackages() const;
    std::string getPackagePath(const std::string& packageName) const;
    std::string getPackageMetadataPath(const std::string& packageName) const;
    bool validatePackage(const PackageInfo& packageInfo) const;
    std::vector<std::string> detectConflicts(const PackageInfo& packageInfo) const;
};

// Factory function
std::unique_ptr<PackageManager> createPackageManager();

} // namespace ArabicLanguage

#endif // PACKAGE_MANAGER_H

