// ArabicAppsAPI.h - واجهة برمجة التطبيقات الخاصة بالتطبيقات العربية
// Arabic Apps API - Complete application framework for Arabic software development

#ifndef ARABIC_APPS_API_H
#define ARABIC_APPS_API_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <cstdint>

#include "ArabicDeepLearning.h"

namespace ArabicApps {

class ArabicFFI;

// ════════════════════════════════════════════════════════════
// 🖥️ التطبيق العربي الرئيسي
// ════════════════════════════════════════════════════════════

class ArabicApplication {
private:
    std::string appName;
    std::string version;
    std::string author;
    bool initialized;

    // الأنظمة الفرعية
    std::unique_ptr<class ArabicGUI> gui;
    std::unique_ptr<class ArabicDatabaseManager> database;
    std::unique_ptr<class ArabicNetworking> networking;
    std::unique_ptr<class ArabicSecurity> security;
    std::unique_ptr<class ArabicAI> ai;
    std::unique_ptr<class ArabicFileSystem> filesystem;
    std::unique_ptr<class ArabicReports> reports;
    std::unique_ptr<class ArabicUpdater> updater;
    std::unique_ptr<class ArabicCloudStorage> cloud;
    std::unique_ptr<class ArabicNotifications> notifications;
    std::unique_ptr<class ArabicBackup> backup;

public:
    ArabicApplication(const std::string& name, const std::string& ver, const std::string& auth);
    ~ArabicApplication() = default;

    // تهيئة التطبيق
    bool initializeAllSystems();

    // تشغيل التطبيق
    int run();

    // إيقاف التطبيق
    void stop();

    // معلومات التطبيق
    const std::string& getName() const { return appName; }
    const std::string& getVersion() const { return version; }
    const std::string& getAuthor() const { return author; }

    // الوصول للأنظمة
    class ArabicGUI* getGUI() const { return gui.get(); }
    class ArabicDatabaseManager* getDatabase() const { return database.get(); }
    class ArabicNetworking* getNetworking() const { return networking.get(); }
    class ArabicSecurity* getSecurity() const { return security.get(); }
    class ArabicAI* getAI() const { return ai.get(); }
    class ArabicFileSystem* getFileSystem() const { return filesystem.get(); }
    class ArabicReports* getReports() const { return reports.get(); }
    class ArabicUpdater* getUpdater() const { return updater.get(); }
    class ArabicCloudStorage* getCloud() const { return cloud.get(); }
    class ArabicNotifications* getNotifications() const { return notifications.get(); }
    class ArabicBackup* getBackup() const { return backup.get(); }

    bool isInitialized() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 🖼️ نظام واجهات المستخدم الرسومية العربية
// ════════════════════════════════════════════════════════════

enum class ArabicWindowType {
    MAIN,
    DIALOG,
    MODAL,
    TOOL,
    SPLASH
};

enum class ArabicTextAlignment {
    LEFT,
    CENTER,
    RIGHT,
    JUSTIFY
};

struct ArabicPosition {
    int x, y;
    ArabicPosition() : x(0), y(0) {}
    ArabicPosition(int x, int y) : x(x), y(y) {}
};

struct ArabicSize {
    int width, height;
    ArabicSize() : width(0), height(0) {}
    ArabicSize(int w, int h) : width(w), height(h) {}
};

struct ArabicColor {
    float r, g, b, a;
    ArabicColor() : r(0), g(0), b(0), a(1.0f) {}
    ArabicColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
};

class ArabicWindow {
private:
    std::string title;
    ArabicPosition position;
    ArabicSize size;
    bool visible;
    std::vector<std::unique_ptr<class ArabicWidget>> widgets;

public:
    ArabicWindow(const std::string& title, int width, int height);
    virtual ~ArabicWindow() = default;

    // إدارة النافذة
    void setTitle(const std::string& title);
    void setPosition(int x, int y);
    void setSize(int width, int height);
    void show();
    void hide();
    void close();

    // إضافة عناصر
    void addButton(const std::string& text, std::function<void()> callback,
                  int x, int y, int width, int height);
    void addTextBox(const std::string& initialText, int x, int y, int width, int height,
                   bool multiline = false);
    void addLabel(const std::string& text, int x, int y, int width, int height);
    void addComboBox(const std::vector<std::string>& items, int x, int y, int width);
    void addTable(const std::vector<std::string>& headers,
                 const std::vector<std::vector<std::string>>& data,
                 int x, int y, int width, int height);
    void addCanvas(int x, int y, int width, int height);

    // معلومات النافذة
    const std::string& getTitle() const { return title; }
    ArabicPosition getPosition() const { return position; }
    ArabicSize getSize() const { return size; }
    bool isVisible() const { return visible; }
};

class ArabicGUI {
private:
    std::vector<std::unique_ptr<ArabicWindow>> windows;
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;

public:
    ArabicGUI();
    ~ArabicGUI() = default;

    // تهيئة نظام الواجهة
    bool initialize();

    // إنشاء النوافذ
    ArabicWindow* createWindow(const std::string& title, int width, int height,
                              ArabicWindowType type = ArabicWindowType::MAIN);
    ArabicWindow* createDialog(const std::string& title, const std::string& message,
                              const std::vector<std::string>& buttons);

    // إدارة النوافذ
    void showWindow(ArabicWindow* window);
    void hideWindow(ArabicWindow* window);
    void closeWindow(ArabicWindow* window);

    // إعدادات الواجهة العربية
    void setRTL(bool rtl);
    void setFont(const std::string& fontName, int size);
    void setTheme(const std::string& themeName);

    bool isInitialized() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 🗄️ نظام قواعد البيانات العربية
// ════════════════════════════════════════════════════════════

enum class ArabicDatabaseType {
    SQLITE,
    MYSQL,
    POSTGRESQL,
    MONGODB
};

struct ArabicDatabaseConfig {
    ArabicDatabaseType type;
    std::string host;
    int port;
    std::string database;
    std::string username;
    std::string password;

    ArabicDatabaseConfig() : port(0) {}
};

struct ArabicTableColumn {
    std::string name;
    std::string type;
    bool primaryKey;
    bool autoIncrement;
    bool nullable;

    ArabicTableColumn() : primaryKey(false), autoIncrement(false), nullable(true) {}
};

class ArabicDatabaseConnection {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool connected;
    ArabicDatabaseConfig config;

public:
    ArabicDatabaseConnection(const ArabicDatabaseConfig& config);
    ~ArabicDatabaseConnection();

    // الاتصال وإغلاقه
    bool connect();
    void disconnect();
    bool isConnected() const { return connected; }

    // إنشاء الجداول
    bool createTable(const std::string& tableName,
                    const std::vector<ArabicTableColumn>& columns);

    // العمليات الأساسية
    bool insert(const std::string& tableName,
               const std::unordered_map<std::string, std::string>& data);
    std::vector<std::unordered_map<std::string, std::string>> select(
        const std::string& tableName,
        const std::vector<std::string>& columns = {},
        const std::string& whereClause = "");
    bool update(const std::string& tableName,
               const std::unordered_map<std::string, std::string>& data,
               const std::string& whereClause);
    bool delete_(const std::string& tableName, const std::string& whereClause);

    // المعاملات
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
};

class ArabicDatabaseManager {
private:
    std::vector<std::unique_ptr<ArabicDatabaseConnection>> connections;

public:
    ArabicDatabaseManager();
    ~ArabicDatabaseManager() = default;

    // إنشاء اتصال
    ArabicDatabaseConnection* createConnection(const ArabicDatabaseConnection& config);

    // إدارة الاتصالات
    void closeConnection(ArabicDatabaseConnection* connection);
    std::vector<ArabicDatabaseConnection*> getConnections() const;
};

// ════════════════════════════════════════════════════════════
// 🌐 نظام الشبكات العربي
// ════════════════════════════════════════════════════════════

enum class ArabicHTTPMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS
};

struct ArabicHTTPResponse {
    int statusCode;
    std::string statusText;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::string error;

    ArabicHTTPResponse() : statusCode(0) {}
};

struct ArabicHTTPRequest {
    ArabicHTTPMethod method;
    std::string url;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    int timeout;

    ArabicHTTPRequest() : timeout(30000) {}
};

class ArabicHTTPServer {
private:
    std::unique_ptr<ArabicFFI> ffi;
    int port;
    bool running;

public:
    ArabicHTTPServer(int port);
    ~ArabicHTTPServer();

    bool start();
    void stop();
    bool isRunning() const { return running; }

    // إعداد المسارات
    void addRoute(const std::string& path, ArabicHTTPMethod method,
                 std::function<ArabicHTTPResponse(const ArabicHTTPRequest&)> handler);
};

class ArabicWebSocketServer {
private:
    std::unique_ptr<ArabicFFI> ffi;
    int port;
    bool running;

public:
    ArabicWebSocketServer(int port);
    ~ArabicWebSocketServer();

    bool start();
    void stop();
    bool isRunning() const { return running; }

    // إدارة العملاء
    void setMessageHandler(std::function<void(uintptr_t clientId, const std::string& message)> handler);
    void broadcastMessage(const std::string& message);
};

class ArabicNetworking {
private:
    std::unique_ptr<ArabicHTTPServer> httpServer;
    std::unique_ptr<ArabicWebSocketServer> wsServer;

public:
    ArabicNetworking();
    ~ArabicNetworking() = default;

    // طلبات HTTP
    ArabicHTTPResponse httpRequest(const ArabicHTTPRequest& request);

    // خوادم
    ArabicHTTPServer* createHTTPServer(int port);
    ArabicWebSocketServer* createWebSocketServer(int port);

    // عملاء WebSocket
    uintptr_t createWebSocketClient();
    bool connectWebSocket(uintptr_t client, const std::string& url);
    bool sendWebSocketMessage(uintptr_t client, const std::string& message);
    std::string receiveWebSocketMessage(uintptr_t client);
    void closeWebSocket(uintptr_t client);
};

// ════════════════════════════════════════════════════════════
// 🔐 نظام الأمان التطبيقي
// ════════════════════════════════════════════════════════════

enum class ArabicEncryptionAlgorithm {
    AES,
    RSA,
    SHA256,
    MD5
};

class ArabicSecurity {
private:
    std::unique_ptr<ArabicFFI> ffi;

public:
    ArabicSecurity();
    ~ArabicSecurity() = default;

    // التشفير والفك
    std::string encrypt(const std::string& data, const std::string& key,
                       ArabicEncryptionAlgorithm algorithm = ArabicEncryptionAlgorithm::AES);
    std::string decrypt(const std::string& encryptedData, const std::string& key,
                       ArabicEncryptionAlgorithm algorithm = ArabicEncryptionAlgorithm::AES);

    // التجزئة
    std::string hash(const std::string& data,
                    ArabicEncryptionAlgorithm algorithm = ArabicEncryptionAlgorithm::SHA256);

    // التوقيع الرقمي
    std::string sign(const std::string& data, const std::string& privateKey);
    bool verifySignature(const std::string& data, const std::string& signature,
                        const std::string& publicKey);

    // توليد المفاتيح
    std::pair<std::string, std::string> generateKeyPair(ArabicEncryptionAlgorithm algorithm = ArabicEncryptionAlgorithm::RSA);

    // شهادات الأمان
    std::string createCertificate(const std::string& data, const std::string& issuer,
                                const std::string& subject, time_t validUntil);
};

// ════════════════════════════════════════════════════════════
// 🤖 محرك الذكاء الاصطناعي التطبيقي
// ════════════════════════════════════════════════════════════

enum class ArabicAITask {
    TEXT_ANALYSIS,
    CONTENT_GENERATION,
    IMAGE_RECOGNITION,
    TRANSLATION,
    SENTIMENT_ANALYSIS,
    CODE_GENERATION,
    CHAT_CONVERSATION,
    DEEP_LEARNING
};

struct ArabicAIResult {
    bool success;
    std::string content;
    std::string taskId;
    std::string errorMessage;
    float confidence;
    std::unordered_map<std::string, std::string> metadata;
    std::vector<double> raw_output; // مخرجات خام للتعلم العميق
    std::vector<std::string> suggestions; // اقتراحات إضافية

    ArabicAIResult() : success(false), confidence(0.0f) {}
};

class ArabicAI {
private:
    std::unique_ptr<ArabicFFI> ffi;
    std::unique_ptr<ArabicLanguage::NeuralNetwork> nn; // نموذج تعلم عميق محلي
    std::string current_config; // تكوين الطبقات الحالي

public:
    ArabicAI();
    ~ArabicAI();

    // تهيئة نظام الذكاء الاصطناعي
    bool initialize(const std::string& config = "");

    // تحليل النصوص
    ArabicAIResult analyzeText(const std::string& text, ArabicAITask task);

    // توليد المحتوى
    ArabicAIResult generateContent(const std::string& prompt, ArabicAITask task);

    // التعرف على الصور
    ArabicAIResult recognizeImage(const std::string& imagePath, const std::string& task);

    // الترجمة
    ArabicAIResult translate(const std::string& text,
                           const std::string& sourceLang,
                           const std::string& targetLang);

    // المحادثة
    ArabicAIResult chat(const std::string& message,
                       const std::vector<std::string>& context = {});

    // --- ميزات التعلم العميق الجديدة ---

    /**
     * @brief إنشاء نموذج تعلم عميق
     * @param layers_config تكوين الطبقات (مثلاً: "dense:10,relu;dense:2,softmax")
     */
    bool createDeepModel(const std::string& layers_config);

    /**
     * @brief تدريب النموذج
     * @param input البيانات المدخلة
     * @param target الأهداف المنشودة
     * @return قيمة الخسارة (Loss)
     */
    double trainStep(const std::vector<double>& input, const std::vector<double>& target);

    /**
     * @brief التنبؤ باستخدام نموذج التعلم العميق
     * @param input البيانات المدخلة
     * @return نتيجة التنبؤ
     */
    ArabicAIResult predictDeep(const std::vector<double>& input);

    /**
     * @brief حفظ النموذج
     * @param path مسار الملف
     */
    bool saveModel(const std::string& path);

    /**
     * @brief تحميل نموذج سابق
     * @param path مسار الملف
     */
    bool loadModel(const std::string& path);
};

// ════════════════════════════════════════════════════════════
// 📁 نظام الملفات العربية
// ════════════════════════════════════════════════════════════

class ArabicFileSystem {
private:
    std::unique_ptr<ArabicFFI> ffi;

public:
    ArabicFileSystem();
    ~ArabicFileSystem() = default;

    // قراءة وكتابة الملفات
    std::string readFile(const std::string& path);
    bool writeFile(const std::string& path, const std::string& content);

    // إدارة المجلدات
    bool createDirectory(const std::string& path);
    bool deleteDirectory(const std::string& path);
    std::vector<std::string> listDirectory(const std::string& path);

    // العمليات على الملفات
    bool copyFile(const std::string& source, const std::string& destination);
    bool moveFile(const std::string& source, const std::string& destination);
    bool deleteFile(const std::string& path);
    uint64_t getFileSize(const std::string& path);

    // البحث في الملفات
    std::vector<std::string> searchFiles(const std::string& directory,
                                       const std::string& pattern);

    // ضغط وفك الضغط
    bool compressFile(const std::string& source, const std::string& destination);
    bool decompressFile(const std::string& source, const std::string& destination);
};

// ════════════════════════════════════════════════════════════
// 📊 نظام التقارير والإحصائيات
// ════════════════════════════════════════════════════════════

enum class ArabicReportType {
    TABLE,
    CHART,
    DASHBOARD,
    PDF,
    EXCEL
};

enum class ArabicChartType {
    BAR,
    LINE,
    PIE,
    SCATTER,
    HISTOGRAM
};

class ArabicReport {
private:
    ArabicReportType type;
    std::string title;
    std::unordered_map<std::string, std::string> data;

public:
    ArabicReport(ArabicReportType type, const std::string& title);

    // إضافة البيانات
    void addData(const std::string& key, const std::string& value);
    void addTableData(const std::vector<std::string>& headers,
                     const std::vector<std::vector<std::string>>& rows);
    void addChartData(ArabicChartType chartType,
                     const std::vector<std::string>& labels,
                     const std::vector<double>& values);

    // تصدير التقرير
    bool exportToFile(const std::string& path);

    const std::string& getTitle() const { return title; }
    ArabicReportType getType() const { return type; }
};

class ArabicReports {
private:
    std::vector<std::unique_ptr<ArabicReport>> reports;

public:
    ArabicReports();
    ~ArabicReports() = default;

    // إنشاء التقارير
    ArabicReport* createReport(ArabicReportType type, const std::string& title);
    ArabicReport* createStatisticsReport(const std::string& title,
                                       const std::vector<double>& data);

    // إدارة التقارير
    void deleteReport(ArabicReport* report);
    std::vector<ArabicReport*> getReports() const;
};

// ════════════════════════════════════════════════════════════
// 🔄 نظام التحديثات التلقائي
// ════════════════════════════════════════════════════════════

class ArabicUpdater {
private:
    std::string currentVersion;
    std::string updateUrl;

public:
    ArabicUpdater(const std::string& currentVer, const std::string& url);

    // فحص التحديثات
    bool checkForUpdates();

    // تحميل وتثبيت التحديث
    bool downloadAndInstallUpdate();

    // معلومات التحديث
    std::string getLatestVersion() const;
    std::string getUpdateDescription() const;
    uint64_t getUpdateSize() const;

    // إنشاء نسخة احتياطية قبل التحديث
    bool createBackup(const std::string& backupPath);
    bool restoreFromBackup(const std::string& backupPath);
};

// ════════════════════════════════════════════════════════════
// ☁️ نظام التخزين السحابي العربي
// ════════════════════════════════════════════════════════════

enum class ArabicCloudProvider {
    AWS,
    GOOGLE_CLOUD,
    AZURE,
    ARABIC_CLOUD
};

class ArabicCloudStorage {
private:
    ArabicCloudProvider provider;
    std::string accessKey;
    std::string secretKey;
    std::string bucketName;

public:
    ArabicCloudStorage(ArabicCloudProvider provider,
                      const std::string& accessKey,
                      const std::string& secretKey,
                      const std::string& bucket);

    // رفع وتحميل الملفات
    bool uploadFile(const std::string& localPath, const std::string& cloudPath);
    bool downloadFile(const std::string& cloudPath, const std::string& localPath);

    // إدارة الملفات السحابية
    std::vector<std::string> listFiles(const std::string& prefix = "");
    bool deleteFile(const std::string& cloudPath);
    uint64_t getFileSize(const std::string& cloudPath);

    // مشاركة الملفات
    std::string generateShareableLink(const std::string& cloudPath, int expiryHours = 24);
};

// ════════════════════════════════════════════════════════════
// 🔔 نظام الإشعارات والتنبيهات
// ════════════════════════════════════════════════════════════

enum class ArabicNotificationType {
    INFO,
    WARNING,
    ERROR,
    SUCCESS
};

class ArabicNotifications {
private:
    std::vector<std::unique_ptr<class ArabicNotification>> notifications;

public:
    ArabicNotifications();
    ~ArabicNotifications() = default;

    // إنشاء الإشعارات
    void showNotification(const std::string& title, const std::string& message,
                         ArabicNotificationType type = ArabicNotificationType::INFO);

    // إشعارات مجدولة
    void scheduleNotification(const std::string& title, const std::string& message,
                             time_t scheduledTime,
                             ArabicNotificationType type = ArabicNotificationType::INFO);

    // إشعارات البريد الإلكتروني
    bool sendEmailNotification(const std::string& to, const std::string& subject,
                              const std::string& body);

    // إشعارات SMS
    bool sendSMSNotification(const std::string& phoneNumber, const std::string& message);
};

// ════════════════════════════════════════════════════════════
// 💾 نظام النسخ الاحتياطي والاستعادة
// ════════════════════════════════════════════════════════════

class ArabicBackup {
private:
    std::string backupDirectory;

public:
    explicit ArabicBackup(const std::string& backupDir);

    // إنشاء النسخ الاحتياطية
    bool createBackup(const std::string& sourcePath, const std::string& backupName);
    bool createDatabaseBackup(ArabicDatabaseConnection* db, const std::string& backupName);

    // استعادة النسخ الاحتياطية
    bool restoreBackup(const std::string& backupName, const std::string& destinationPath);
    bool restoreDatabaseBackup(ArabicDatabaseConnection* db, const std::string& backupName);

    // إدارة النسخ الاحتياطية
    std::vector<std::string> listBackups() const;
    bool deleteBackup(const std::string& backupName);
    uint64_t getBackupSize(const std::string& backupName) const;

    // ضغط النسخ الاحتياطية
    bool compressBackup(const std::string& backupName);
    bool decompressBackup(const std::string& backupName);
};

} // namespace ArabicApps

#endif // ARABIC_APPS_API_H
