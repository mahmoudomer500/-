// PackageManager.cpp - تطبيق مدير الحزم الشامل
// الأسبوع الرابع من الشهر الرابع: نظام إدارة الحزم
#include "PackageManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

namespace ArabicLanguage {

// ==================== PackageVersion Implementation ====================

std::string PackageVersion::toString() const {
    std::stringstream ss;
    ss << major << "." << minor << "." << patch;
    if (!prerelease.empty()) {
        ss << "-" << prerelease;
    }
    if (!build.empty()) {
        ss << "+" << build;
    }
    return ss.str();
}

PackageVersion PackageVersion::fromString(const std::string& versionStr) {
    PackageVersion version;
    
    // تحليل بسيط للإصدار (مثل "1.2.3" أو "1.2.3-beta")
    size_t dashPos = versionStr.find('-');
    size_t plusPos = versionStr.find('+');
    
    std::string mainVersion = versionStr;
    if (dashPos != std::string::npos) {
        mainVersion = versionStr.substr(0, dashPos);
        version.prerelease = versionStr.substr(dashPos + 1);
        if (plusPos != std::string::npos && plusPos > dashPos) {
            version.prerelease = versionStr.substr(dashPos + 1, plusPos - dashPos - 1);
            version.build = versionStr.substr(plusPos + 1);
        }
    } else if (plusPos != std::string::npos) {
        mainVersion = versionStr.substr(0, plusPos);
        version.build = versionStr.substr(plusPos + 1);
    }
    
    // تحليل الأرقام
    std::stringstream ss(mainVersion);
    char dot;
    ss >> version.major >> dot >> version.minor >> dot >> version.patch;
    
    return version;
}

bool PackageVersion::operator==(const PackageVersion& other) const {
    return major == other.major && minor == other.minor && patch == other.patch;
}

bool PackageVersion::operator<(const PackageVersion& other) const {
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
    return patch < other.patch;
}

bool PackageVersion::operator>(const PackageVersion& other) const {
    return other < *this;
}

bool PackageVersion::operator<=(const PackageVersion& other) const {
    return !(*this > other);
}

bool PackageVersion::operator>=(const PackageVersion& other) const {
    return !(*this < other);
}

bool PackageDependency::isSatisfiedBy(const PackageVersion& version) const {
    if (version < minVersion) return false;
    if (maxVersion.major > 0 && version > maxVersion) return false;
    return true;
}

// ==================== PackageManager Implementation ====================

PackageManager::PackageManager() : verbose(false) {
    packagesDirectory = "./packages";
    cacheDirectory = "./cache";
    repositoryUrls.push_back("https://packages.arabic-lang.org");
}

void PackageManager::initialize() {
    std::cout << "[INFO] Package Manager initialized" << std::endl;
    
    // إنشاء المجلدات المطلوبة
    if (!fs::exists(packagesDirectory)) {
        fs::create_directories(packagesDirectory);
    }
    if (!fs::exists(cacheDirectory)) {
        fs::create_directories(cacheDirectory);
    }
    
    // تحميل الحزم المثبتة
    loadInstalledPackages();
    
    std::cout << "[INFO] Loaded " << installedPackages.size() << " installed packages" << std::endl;
}

void PackageManager::setPackagesDirectory(const std::string& dir) {
    packagesDirectory = dir;
    if (!fs::exists(packagesDirectory)) {
        fs::create_directories(packagesDirectory);
    }
}

void PackageManager::setCacheDirectory(const std::string& dir) {
    cacheDirectory = dir;
    if (!fs::exists(cacheDirectory)) {
        fs::create_directories(cacheDirectory);
    }
}

void PackageManager::addRepository(const std::string& url) {
    repositoryUrls.push_back(url);
    if (verbose) {
        std::cout << "[INFO] Added repository: " << url << std::endl;
    }
}

InstallResult PackageManager::installPackage(const std::string& packageName, const std::string& version) {
    InstallResult result;
    
    std::cout << "\n[INSTALL] Installing package: " << packageName;
    if (version != "latest") {
        std::cout << " @ " << version;
    }
    std::cout << std::endl;
    
    // التحقق من التثبيت المسبق
    if (isInstalled(packageName)) {
        auto installed = getPackageInfo(packageName);
        std::cout << "[WARNING] Package already installed: " << installed.version.toString() << std::endl;
        result.status = InstallStatus::ALREADY_INSTALLED;
        result.message = "Package already installed";
        return result;
    }
    
    // جلب معلومات الحزمة
    PackageInfo packageInfo = fetchPackageInfo(packageName, version);
    if (packageInfo.name.empty()) {
        result.status = InstallStatus::INVALID_PACKAGE;
        result.message = "Package not found in repository";
        std::cout << "[ERROR] Package not found: " << packageName << std::endl;
        return result;
    }
    
    // التحقق من التعارضات
    auto conflicts = detectConflicts(packageInfo);
    if (!conflicts.empty()) {
        result.status = InstallStatus::CONFLICT_DETECTED;
        result.message = "Package conflicts detected";
        result.warnings = conflicts;
        std::cout << "[ERROR] Conflicts detected:" << std::endl;
        for (const auto& conflict : conflicts) {
            std::cout << "  - " << conflict << std::endl;
        }
        return result;
    }
    
    // حل التبعيات
    std::cout << "[INFO] Resolving dependencies..." << std::endl;
    auto dependencies = resolveDependencies(packageName);
    for (const auto& dep : dependencies) {
        if (!isInstalled(dep)) {
            std::cout << "[INFO] Installing dependency: " << dep << std::endl;
            auto depResult = installPackage(dep, "latest");
            if (depResult.status != InstallStatus::SUCCESS) {
                result.status = InstallStatus::DEPENDENCY_FAILED;
                result.message = "Failed to install dependency: " + dep;
                result.failedPackages.push_back(dep);
                return result;
            }
            result.installedPackages.insert(result.installedPackages.end(),
                                          depResult.installedPackages.begin(),
                                          depResult.installedPackages.end());
        }
    }
    
    // تثبيت الحزمة
    std::string packagePath = getPackagePath(packageName);
    if (!fs::exists(packagePath)) {
        fs::create_directories(packagePath);
    }
    
    // محاكاة التثبيت (في الإصدار الكامل سيتم تنزيل واستخراج الملفات)
    packageInfo.isInstalled = true;
    packageInfo.installPath = packagePath;
    packageInfo.installDate = std::chrono::system_clock::now();
    packageInfo.updateDate = packageInfo.installDate;
    
    if (registerPackage(packageInfo)) {
        result.status = InstallStatus::SUCCESS;
        result.message = "Package installed successfully";
        result.installedPackages.push_back(packageName);
        std::cout << "[SUCCESS] Package installed: " << packageName << " @ " 
                 << packageInfo.version.toString() << std::endl;
    } else {
        result.status = InstallStatus::FAILED;
        result.message = "Failed to register package";
        std::cout << "[ERROR] Failed to register package" << std::endl;
    }
    
    return result;
}

InstallResult PackageManager::installPackageFromFile(const std::string& packageFile) {
    InstallResult result;
    
    std::cout << "[INSTALL] Installing from file: " << packageFile << std::endl;
    
    if (!fs::exists(packageFile)) {
        result.status = InstallStatus::INVALID_PACKAGE;
        result.message = "Package file not found";
        return result;
    }
    
    // في الإصدار الكامل سيتم قراءة معلومات الحزمة من الملف
    result.status = InstallStatus::FAILED;
    result.message = "File installation not yet implemented";
    
    return result;
}

InstallResult PackageManager::installPackages(const std::vector<std::string>& packageNames) {
    InstallResult result;
    
    for (const auto& packageName : packageNames) {
        auto packageResult = installPackage(packageName);
        if (packageResult.status == InstallStatus::SUCCESS) {
            result.installedPackages.insert(result.installedPackages.end(),
                                          packageResult.installedPackages.begin(),
                                          packageResult.installedPackages.end());
        } else {
            result.failedPackages.push_back(packageName);
            result.warnings.push_back(packageName + ": " + packageResult.message);
        }
    }
    
    result.status = result.failedPackages.empty() ? InstallStatus::SUCCESS : InstallStatus::FAILED;
    result.message = "Installed " + std::to_string(result.installedPackages.size()) + 
                    " packages, " + std::to_string(result.failedPackages.size()) + " failed";
    
    return result;
}

bool PackageManager::uninstallPackage(const std::string& packageName, bool removeDependencies) {
    std::cout << "\n[UNINSTALL] Uninstalling package: " << packageName << std::endl;
    
    if (!isInstalled(packageName)) {
        std::cout << "[WARNING] Package not installed: " << packageName << std::endl;
        return false;
    }
    
    // التحقق من التبعيات
    auto dependents = getDependents(packageName);
    if (!dependents.empty() && !removeDependencies) {
        std::cout << "[ERROR] Package is required by:" << std::endl;
        for (const auto& dep : dependents) {
            std::cout << "  - " << dep << std::endl;
        }
        return false;
    }
    
    // حذف الملفات
    std::string packagePath = getPackagePath(packageName);
    if (fs::exists(packagePath)) {
        fs::remove_all(packagePath);
    }
    
    // إلغاء التسجيل
    if (unregisterPackage(packageName)) {
        std::cout << "[SUCCESS] Package uninstalled: " << packageName << std::endl;
        return true;
    }
    
    return false;
}

bool PackageManager::uninstallPackages(const std::vector<std::string>& packageNames) {
    bool allSuccess = true;
    for (const auto& packageName : packageNames) {
        if (!uninstallPackage(packageName)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool PackageManager::updatePackage(const std::string& packageName) {
    std::cout << "\n[UPDATE] Updating package: " << packageName << std::endl;
    
    if (!isInstalled(packageName)) {
        std::cout << "[ERROR] Package not installed: " << packageName << std::endl;
        return false;
    }
    
    auto currentInfo = getPackageInfo(packageName);
    auto latestInfo = fetchPackageInfo(packageName, "latest");
    
    if (latestInfo.version <= currentInfo.version) {
        std::cout << "[INFO] Package is already up to date" << std::endl;
        return true;
    }
    
    std::cout << "[INFO] Updating from " << currentInfo.version.toString() 
             << " to " << latestInfo.version.toString() << std::endl;
    
    // إزالة القديم وتثبيت الجديد
    if (uninstallPackage(packageName, false)) {
        auto result = installPackage(packageName, latestInfo.version.toString());
        return result.status == InstallStatus::SUCCESS;
    }
    
    return false;
}

bool PackageManager::updateAllPackages() {
    auto updatable = getUpdatablePackages();
    std::cout << "[UPDATE] Found " << updatable.size() << " updatable packages" << std::endl;
    
    bool allSuccess = true;
    for (const auto& packageName : updatable) {
        if (!updatePackage(packageName)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

std::vector<std::string> PackageManager::getUpdatablePackages() const {
    std::vector<std::string> updatable;
    
    for (const auto& pair : installedPackages) {
        auto latest = fetchPackageInfo(pair.first, "latest");
        if (latest.version > pair.second.version) {
            updatable.push_back(pair.first);
        }
    }
    
    return updatable;
}

bool PackageManager::isInstalled(const std::string& packageName) const {
    return installedPackages.find(packageName) != installedPackages.end();
}

PackageInfo PackageManager::getPackageInfo(const std::string& packageName) const {
    auto it = installedPackages.find(packageName);
    if (it != installedPackages.end()) {
        return it->second;
    }
    return PackageInfo();
}

std::vector<PackageInfo> PackageManager::listInstalledPackages() const {
    std::vector<PackageInfo> packages;
    for (const auto& pair : installedPackages) {
        packages.push_back(pair.second);
    }
    return packages;
}

std::vector<PackageInfo> PackageManager::searchPackages(const std::string& query) const {
    std::vector<PackageInfo> results;
    
    // بحث بسيط في الحزم المثبتة
    for (const auto& pair : installedPackages) {
        const auto& info = pair.second;
        if (info.name.find(query) != std::string::npos ||
            info.description.find(query) != std::string::npos) {
            results.push_back(info);
        }
    }
    
    return results;
}

std::vector<PackageInfo> PackageManager::getPackageVersions(const std::string& packageName) const {
    std::vector<PackageInfo> versions;
    
    // في الإصدار الكامل سيتم جلب جميع الإصدارات من المستودع
    PackageInfo info = fetchPackageInfo(packageName, "latest");
    if (!info.name.empty()) {
        versions.push_back(info);
    }
    
    return versions;
}

std::vector<std::string> PackageManager::resolveDependencies(const std::string& packageName) const {
    std::vector<std::string> dependencies;
    
    PackageInfo info = fetchPackageInfo(packageName, "latest");
    for (const auto& dep : info.dependencies) {
        if (dep.required) {
            dependencies.push_back(dep.packageName);
        }
    }
    
    return dependencies;
}

std::vector<std::string> PackageManager::getDependents(const std::string& packageName) const {
    std::vector<std::string> dependents;
    
    for (const auto& pair : installedPackages) {
        auto deps = resolveDependencies(pair.first);
        if (std::find(deps.begin(), deps.end(), packageName) != deps.end()) {
            dependents.push_back(pair.first);
        }
    }
    
    return dependents;
}

bool PackageManager::checkDependencies(const std::string& packageName) const {
    auto dependencies = resolveDependencies(packageName);
    
    for (const auto& dep : dependencies) {
        if (!isInstalled(dep)) {
            std::cout << "[WARNING] Missing dependency: " << dep << std::endl;
            return false;
        }
    }
    
    return true;
}

size_t PackageManager::getTotalInstalledSize() const {
    size_t total = 0;
    for (const auto& pair : installedPackages) {
        total += pair.second.size;
    }
    return total;
}

size_t PackageManager::getPackageCount() const {
    return installedPackages.size();
}

std::map<std::string, size_t> PackageManager::getStatistics() const {
    std::map<std::string, size_t> stats;
    stats["installed_packages"] = installedPackages.size();
    stats["total_size"] = getTotalInstalledSize();
    stats["repositories"] = repositoryUrls.size();
    return stats;
}

void PackageManager::cleanCache() {
    if (fs::exists(cacheDirectory)) {
        fs::remove_all(cacheDirectory);
        fs::create_directories(cacheDirectory);
        std::cout << "[INFO] Cache cleaned" << std::endl;
    }
}

void PackageManager::cleanOrphanedPackages() {
    std::vector<std::string> orphaned;
    
    for (const auto& pair : installedPackages) {
        auto dependents = getDependents(pair.first);
        if (dependents.empty()) {
            orphaned.push_back(pair.first);
        }
    }
    
    std::cout << "[INFO] Found " << orphaned.size() << " orphaned packages" << std::endl;
    // في الإصدار الكامل سيتم حذفها
}

PackageInfo PackageManager::fetchPackageInfo(const std::string& packageName, const std::string& version) const {
    PackageInfo info;
    
    // محاكاة جلب معلومات الحزمة من المستودع
    // في الإصدار الكامل سيتم جلبها من المستودع الفعلي
    
    info.name = packageName;
    if (version == "latest") {
        info.version = PackageVersion(1, 0, 0);
    } else {
        info.version = PackageVersion::fromString(version);
    }
    info.description = "Package: " + packageName;
    info.author = "Unknown";
    info.license = "MIT";
    info.size = 1024 * 100; // 100 KB
    
    return info;
}

bool PackageManager::downloadPackage(const PackageInfo& packageInfo, const std::string& destination) const {
    // محاكاة التنزيل
    // في الإصدار الكامل سيتم تنزيل الملف من المستودع
    std::cout << "[INFO] Downloading package: " << packageInfo.name << std::endl;
    return true;
}

bool PackageManager::extractPackage(const std::string& packageFile, const std::string& destination) const {
    // محاكاة الاستخراج
    // في الإصدار الكامل سيتم استخراج الأرشيف
    std::cout << "[INFO] Extracting package to: " << destination << std::endl;
    return true;
}

bool PackageManager::installPackageFiles(const PackageInfo& packageInfo) const {
    // محاكاة تثبيت الملفات
    // في الإصدار الكامل سيتم نسخ الملفات إلى المجلدات المناسبة
    std::cout << "[INFO] Installing package files..." << std::endl;
    return true;
}

bool PackageManager::registerPackage(const PackageInfo& packageInfo) {
    installedPackages[packageInfo.name] = packageInfo;
    saveInstalledPackages();
    return true;
}

bool PackageManager::unregisterPackage(const std::string& packageName) {
    auto it = installedPackages.find(packageName);
    if (it != installedPackages.end()) {
        installedPackages.erase(it);
        saveInstalledPackages();
        return true;
    }
    return false;
}

void PackageManager::loadInstalledPackages() {
    // محاكاة تحميل الحزم المثبتة
    // في الإصدار الكامل سيتم قراءة ملفات metadata
}

void PackageManager::saveInstalledPackages() const {
    // محاكاة حفظ الحزم المثبتة
    // في الإصدار الكامل سيتم حفظ metadata
}

std::string PackageManager::getPackagePath(const std::string& packageName) const {
    return packagesDirectory + "/" + packageName;
}

std::string PackageManager::getPackageMetadataPath(const std::string& packageName) const {
    return getPackagePath(packageName) + "/package.meta";
}

bool PackageManager::validatePackage(const PackageInfo& packageInfo) const {
    if (packageInfo.name.empty()) return false;
    if (packageInfo.version.major < 0) return false;
    return true;
}

std::vector<std::string> PackageManager::detectConflicts(const PackageInfo& packageInfo) const {
    std::vector<std::string> conflicts;
    
    for (const auto& conflict : packageInfo.conflicts) {
        if (isInstalled(conflict)) {
            conflicts.push_back("Conflicts with installed package: " + conflict);
        }
    }
    
    return conflicts;
}

// Factory function
std::unique_ptr<PackageManager> createPackageManager() {
    return std::make_unique<PackageManager>();
}

} // namespace ArabicLanguage

