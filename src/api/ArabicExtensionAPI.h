#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

// Forward declarations
class ArabicExtensionManager;
class ArabicMonacoEditor;
class ArabicProjectManager;

/**
 * @brief Arabic Extension API - واجهة برمجة الإضافات العربية
 *
 * توفر هذه الواجهة إمكانية تطوير إضافات للبيئة التفاعلية العربية
 * مع دعم كامل للغة العربية والتكامل مع جميع مكونات النظام
 */
class ArabicExtensionAPI {
public:
    // ============================================================================
    // أنواع البيانات الأساسية
    // ============================================================================

    /**
     * @brief معلومات الإضافة
     */
    struct ExtensionInfo {
        std::string id;                    // معرف فريد للإضافة
        std::string name;                  // اسم الإضافة بالعربية
        std::string version;               // رقم الإصدار
        std::string description;           // وصف الإضافة
        std::string author;                // اسم المطور
        std::vector<std::string> keywords; // كلمات مفتاحية للبحث
        std::vector<std::string> categories; // فئات الإضافة
        std::string homepage;             // موقع الإضافة
        std::string repository;           // مستودع الكود
        std::string license;              // نوع الترخيص
        std::vector<std::string> engines; // محركات التشغيل المدعومة
        std::unordered_map<std::string, std::string> dependencies; // التبعيات
    };

    /**
     * @brief حالة الإضافة
     */
    enum class ExtensionState {
        NOT_LOADED,     // غير محملة
        LOADING,        // جاري التحميل
        LOADED,         // محملة
        ACTIVATED,      // مفعلة
        DEACTIVATED,    // معطلة
        ERROR,          // خطأ في التحميل
        DISABLED        // معطلة من قبل المستخدم
    };

    /**
     * @brief نوع الإضافة
     */
    enum class ExtensionType {
        LANGUAGE_SUPPORT,    // دعم لغة برمجة
        THEME,              // مظهر
        DEBUGGER,           // مصحح أخطاء
        FORMATTER,          // تهيئة الكود
        LINTER,             // فاحص الكود
        SNIPPET,            // مقتطفات الكود
        TOOL,               // أداة
        INTEGRATION,        // تكامل مع خدمة خارجية
        OTHER               // أخرى
    };

    // ============================================================================
    // واجهة الإضافة الأساسية
    // ============================================================================

    /**
     * @brief واجهة الإضافة الأساسية
     *
     * يجب على جميع الإضافات تنفيذ هذه الواجهة
     */
    class IArabicExtension {
    public:
        virtual ~IArabicExtension() = default;

        /**
         * @brief الحصول على معلومات الإضافة
         */
        virtual ExtensionInfo getInfo() const = 0;

        /**
         * @brief الحصول على نوع الإضافة
         */
        virtual ExtensionType getType() const = 0;

        /**
         * @brief تهيئة الإضافة
         * @param api مؤشر لواجهة API النظام
         * @return true في حالة النجاح
         */
        virtual bool initialize(ArabicExtensionAPI* api) = 0;

        /**
         * @brief تفعيل الإضافة
         * @return true في حالة النجاح
         */
        virtual bool activate() = 0;

        /**
         * @brief إلغاء تفعيل الإضافة
         * @return true في حالة النجاح
         */
        virtual bool deactivate() = 0;

        /**
         * @brief الحصول على حالة الإضافة الحالية
         */
        virtual ExtensionState getState() const = 0;

        /**
         * @brief الحصول على رسالة الخطأ الأخيرة (إن وجدت)
         */
        virtual std::string getLastError() const = 0;
    };

    // ============================================================================
    // خدمات النظام المتاحة للإضافات
    // ============================================================================

    /**
     * @brief خدمة المحرر
     */
    class EditorService {
    public:
        /**
         * @brief تسجيل أمر محرر جديد
         */
        virtual bool registerCommand(const std::string& commandId,
                                   std::function<void()> handler) = 0;

        /**
         * @brief إلغاء تسجيل أمر محرر
         */
        virtual bool unregisterCommand(const std::string& commandId) = 0;

        /**
         * @brief الحصول على النص المحدد حالياً
         */
        virtual std::string getSelectedText() const = 0;

        /**
         * @brief استبدال النص المحدد
         */
        virtual bool replaceSelectedText(const std::string& newText) = 0;

        /**
         * @brief الحصول على محتوى الملف الحالي
         */
        virtual std::string getCurrentFileContent() const = 0;

        /**
         * @brief الحصول على مسار الملف الحالي
         */
        virtual std::string getCurrentFilePath() const = 0;

        /**
         * @brief فتح ملف في المحرر
         */
        virtual bool openFile(const std::string& filePath) = 0;

        /**
         * @brief حفظ الملف الحالي
         */
        virtual bool saveCurrentFile() = 0;

        /**
         * @brief إضافة علامة مرجعية
         */
        virtual bool addBookmark(int lineNumber, const std::string& description = "") = 0;

        /**
         * @brief إزالة علامة مرجعية
         */
        virtual bool removeBookmark(int lineNumber) = 0;
    };

    /**
     * @brief خدمة إدارة المشاريع
     */
    class ProjectService {
    public:
        /**
         * @brief الحصول على مسار المشروع الحالي
         */
        virtual std::string getCurrentProjectPath() const = 0;

        /**
         * @brief الحصول على قائمة ملفات المشروع
         */
        virtual std::vector<std::string> getProjectFiles() const = 0;

        /**
         * @brief إنشاء ملف جديد في المشروع
         */
        virtual bool createFile(const std::string& relativePath, const std::string& content = "") = 0;

        /**
         * @brief حذف ملف من المشروع
         */
        virtual bool deleteFile(const std::string& relativePath) = 0;

        /**
         * @brief قراءة ملف من المشروع
         */
        virtual std::string readFile(const std::string& relativePath) const = 0;

        /**
         * @brief كتابة ملف في المشروع
         */
        virtual bool writeFile(const std::string& relativePath, const std::string& content) = 0;

        /**
         * @brief بناء المشروع
         */
        virtual bool buildProject() = 0;

        /**
         * @brief تشغيل المشروع
         */
        virtual bool runProject() = 0;
    };

    /**
     * @brief خدمة الرسائل والإشعارات
     */
    class NotificationService {
    public:
        /**
         * @brief عرض رسالة معلومات
         */
        virtual void showInfo(const std::string& message) = 0;

        /**
         * @brief عرض رسالة تحذير
         */
        virtual void showWarning(const std::string& message) = 0;

        /**
         * @brief عرض رسالة خطأ
         */
        virtual void showError(const std::string& message) = 0;

        /**
         * @brief طلب تأكيد من المستخدم
         */
        virtual bool showConfirmation(const std::string& message) = 0;

        /**
         * @brief عرض شريط تقدم
         */
        virtual void showProgress(const std::string& title, int percentage) = 0;

        /**
         * @brief إخفاء شريط التقدم
         */
        virtual void hideProgress() = 0;
    };

    /**
     * @brief خدمة التكوين
     */
    class ConfigurationService {
    public:
        /**
         * @brief الحصول على إعداد
         */
        virtual std::string getSetting(const std::string& key) const = 0;

        /**
         * @brief تعيين إعداد
         */
        virtual bool setSetting(const std::string& key, const std::string& value) = 0;

        /**
         * @brief حذف إعداد
         */
        virtual bool removeSetting(const std::string& key) = 0;

        /**
         * @brief التحقق من وجود إعداد
         */
        virtual bool hasSetting(const std::string& key) const = 0;

        /**
         * @brief الحصول على جميع الإعدادات
         */
        virtual std::unordered_map<std::string, std::string> getAllSettings() const = 0;
    };

    /**
     * @brief خدمة السجلات
     */
    class LoggingService {
    public:
        /**
         * @brief تسجيل رسالة معلومات
         */
        virtual void logInfo(const std::string& message) = 0;

        /**
         * @brief تسجيل رسالة تحذير
         */
        virtual void logWarning(const std::string& message) = 0;

        /**
         * @brief تسجيل رسالة خطأ
         */
        virtual void logError(const std::string& message) = 0;

        /**
         * @brief تسجيل رسالة تصحيح
         */
        virtual void logDebug(const std::string& message) = 0;
    };

    // ============================================================================
    // وظائف API الأساسية
    // ============================================================================

    /**
     * @brief تسجيل إضافة جديدة
     */
    virtual bool registerExtension(std::unique_ptr<IArabicExtension> extension) = 0;

    /**
     * @brief إلغاء تسجيل إضافة
     */
    virtual bool unregisterExtension(const std::string& extensionId) = 0;

    /**
     * @brief الحصول على إضافة مسجلة
     */
    virtual IArabicExtension* getExtension(const std::string& extensionId) const = 0;

    /**
     * @brief الحصول على جميع الإضافات المسجلة
     */
    virtual std::vector<IArabicExtension*> getAllExtensions() const = 0;

    /**
     * @brief الحصول على خدمة المحرر
     */
    virtual EditorService* getEditorService() = 0;

    /**
     * @brief الحصول على خدمة إدارة المشاريع
     */
    virtual ProjectService* getProjectService() = 0;

    /**
     * @brief الحصول على خدمة الرسائل
     */
    virtual NotificationService* getNotificationService() = 0;

    /**
     * @brief الحصول على خدمة التكوين
     */
    virtual ConfigurationService* getConfigurationService() = 0;

    /**
     * @brief الحصول على خدمة السجلات
     */
    virtual LoggingService* getLoggingService() = 0;

    /**
     * @brief إرسال حدث لجميع الإضافات
     */
    virtual void broadcastEvent(const std::string& eventType, const std::string& eventData) = 0;

    /**
     * @brief تسجيل مستمع حدث
     */
    virtual bool registerEventListener(const std::string& eventType,
                                     std::function<void(const std::string&)> listener) = 0;

    /**
     * @brief إلغاء تسجيل مستمع حدث
     */
    virtual bool unregisterEventListener(const std::string& eventType,
                                       std::function<void(const std::string&)> listener) = 0;

protected:
    ArabicExtensionAPI() = default;
    virtual ~ArabicExtensionAPI() = default;
};

// ============================================================================
// أنواع الأحداث الشائعة
// ============================================================================

namespace ArabicExtensionEvents {
    static const std::string EDITOR_OPENED = "editor.opened";
    static const std::string EDITOR_CLOSED = "editor.closed";
    static const std::string FILE_SAVED = "file.saved";
    static const std::string FILE_OPENED = "file.opened";
    static const std::string PROJECT_OPENED = "project.opened";
    static const std::string PROJECT_CLOSED = "project.closed";
    static const std::string BUILD_STARTED = "build.started";
    static const std::string BUILD_FINISHED = "build.finished";
    static const std::string DEBUG_STARTED = "debug.started";
    static const std::string DEBUG_STOPPED = "debug.stopped";
    static const std::string EXTENSION_ACTIVATED = "extension.activated";
    static const std::string EXTENSION_DEACTIVATED = "extension.deactivated";
}