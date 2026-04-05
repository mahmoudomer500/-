// CloudServices.h - خدمات السحابة الشاملة
// الأسبوع الثالث من الشهر الخامس: خدمات السحابة
#ifndef CLOUD_SERVICES_H
#define CLOUD_SERVICES_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <fstream>

namespace ArabicLanguage {

// ==================== Cloud Storage ====================

// معلومات الملف في السحابة
struct CloudFile {
    std::string id;
    std::string name;
    std::string path;
    std::string contentType;
    size_t size;
    std::chrono::system_clock::time_point uploadDate;
    std::chrono::system_clock::time_point lastModified;
    std::map<std::string, std::string> metadata;
    std::string url;
    bool isPublic;
    
    CloudFile() : size(0), isPublic(false) {}
};

// نتيجة عملية التخزين
enum class StorageResult {
    SUCCESS,
    FAILED,
    FILE_NOT_FOUND,
    PERMISSION_DENIED,
    QUOTA_EXCEEDED,
    NETWORK_ERROR
};

// نظام تخزين الملفات في السحابة
class CloudStorage {
private:
    std::string storageUrl;
    std::string accessKey;
    std::string secretKey;
    std::string bucketName;
    size_t maxFileSize;
    size_t totalQuota;
    size_t usedQuota;
    std::map<std::string, CloudFile> files;

public:
    CloudStorage();
    ~CloudStorage() = default;
    
    // الإعدادات
    void initialize(const std::string& url, const std::string& accessKey, 
                   const std::string& secretKey, const std::string& bucket);
    void setMaxFileSize(size_t maxSize) { maxFileSize = maxSize; }
    void setQuota(size_t quota) { totalQuota = quota; }
    
    // رفع الملفات
    StorageResult uploadFile(const std::string& localPath, const std::string& cloudPath);
    StorageResult uploadFile(const std::vector<uint8_t>& data, const std::string& cloudPath, 
                            const std::string& contentType);
    StorageResult uploadFile(const std::string& content, const std::string& cloudPath, 
                            const std::string& contentType);
    
    // تنزيل الملفات
    StorageResult downloadFile(const std::string& cloudPath, const std::string& localPath);
    std::vector<uint8_t> downloadFileData(const std::string& cloudPath);
    std::string downloadFileString(const std::string& cloudPath);
    
    // إدارة الملفات
    StorageResult deleteFile(const std::string& cloudPath);
    StorageResult renameFile(const std::string& oldPath, const std::string& newPath);
    StorageResult copyFile(const std::string& sourcePath, const std::string& destPath);
    StorageResult moveFile(const std::string& sourcePath, const std::string& destPath);
    
    // معلومات الملفات
    CloudFile getFileInfo(const std::string& cloudPath) const;
    std::vector<CloudFile> listFiles(const std::string& prefix = "") const;
    bool fileExists(const std::string& cloudPath) const;
    
    // الصلاحيات
    StorageResult setFilePublic(const std::string& cloudPath, bool isPublic);
    StorageResult setFileMetadata(const std::string& cloudPath, 
                                  const std::map<std::string, std::string>& metadata);
    
    // الإحصائيات
    size_t getUsedQuota() const { return usedQuota; }
    size_t getAvailableQuota() const { return totalQuota - usedQuota; }
    size_t getFileCount() const { return files.size(); }
    
    static std::unique_ptr<CloudStorage> create();
};

// ==================== Cloud Database ====================

// نوع البيانات
enum class DataType {
    STRING,
    INTEGER,
    DOUBLE,
    BOOLEAN,
    DATETIME,
    BLOB
};

// قيمة البيانات
struct DataValue {
    DataType type;
    std::string stringValue;
    int intValue;
    double doubleValue;
    bool boolValue;
    std::chrono::system_clock::time_point dateValue;
    std::vector<uint8_t> blobValue;
    
    DataValue() : type(DataType::STRING), intValue(0), doubleValue(0.0), boolValue(false) {}
    
    static DataValue fromString(const std::string& value);
    static DataValue fromInt(int value);
    static DataValue fromDouble(double value);
    static DataValue fromBool(bool value);
    
    std::string toString() const;
};

// سجل قاعدة البيانات
struct DatabaseRecord {
    std::string id;
    std::map<std::string, DataValue> fields;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point updatedAt;
    
    DatabaseRecord() {}
};

// نتيجة عملية قاعدة البيانات
enum class DatabaseResult {
    SUCCESS,
    FAILED,
    NOT_FOUND,
    DUPLICATE_KEY,
    INVALID_DATA,
    CONNECTION_ERROR
};

// قاعدة بيانات سحابية
class CloudDatabase {
private:
    std::string databaseUrl;
    std::string databaseName;
    std::string apiKey;
    std::map<std::string, std::vector<DatabaseRecord>> collections;
    std::map<std::string, std::map<std::string, DataType>> schemas;

public:
    CloudDatabase();
    ~CloudDatabase() = default;
    
    // الإعدادات
    void initialize(const std::string& url, const std::string& dbName, const std::string& key);
    void createCollection(const std::string& collectionName, 
                         const std::map<std::string, DataType>& schema);
    
    // العمليات CRUD
    DatabaseResult insert(const std::string& collection, const DatabaseRecord& record);
    DatabaseResult update(const std::string& collection, const std::string& id, 
                         const std::map<std::string, DataValue>& updates);
    DatabaseResult remove(const std::string& collection, const std::string& id);
    DatabaseRecord findById(const std::string& collection, const std::string& id) const;
    std::vector<DatabaseRecord> findAll(const std::string& collection) const;
    
    // الاستعلامات
    std::vector<DatabaseRecord> query(const std::string& collection, 
                                      const std::map<std::string, DataValue>& filters) const;
    std::vector<DatabaseRecord> queryRange(const std::string& collection, 
                                          const std::string& field, 
                                          const DataValue& minValue, 
                                          const DataValue& maxValue) const;
    
    // الفهرسة
    void createIndex(const std::string& collection, const std::string& field);
    
    // الإحصائيات
    size_t getRecordCount(const std::string& collection) const;
    std::vector<std::string> getCollections() const;
    
    static std::unique_ptr<CloudDatabase> create();
};

// ==================== Messaging Service ====================

// نوع الرسالة
enum class MessageType {
    TEXT,
    JSON,
    BINARY,
    NOTIFICATION
};

// رسالة
struct Message {
    std::string id;
    std::string from;
    std::string to;
    MessageType type;
    std::string content;
    std::vector<uint8_t> binaryData;
    std::map<std::string, std::string> metadata;
    std::chrono::system_clock::time_point timestamp;
    int priority;
    bool isDelivered;
    
    Message() : type(MessageType::TEXT), priority(0), isDelivered(false) {}
};

// قناة الرسائل
struct MessageChannel {
    std::string id;
    std::string name;
    std::vector<std::string> subscribers;
    std::vector<Message> messages;
    int maxMessages;
    bool isPersistent;
    
    MessageChannel() : maxMessages(1000), isPersistent(true) {}
};

// معالج الرسائل
using MessageHandler = std::function<void(const Message&)>;

// خدمة الرسائل
class MessagingService {
private:
    std::string serviceUrl;
    std::string apiKey;
    std::map<std::string, MessageChannel> channels;
    std::map<std::string, MessageHandler> handlers;
    std::map<std::string, std::vector<Message>> queues;
    bool isConnected;

public:
    MessagingService();
    ~MessagingService() = default;
    
    // الإعدادات
    void initialize(const std::string& url, const std::string& key);
    bool connect();
    void disconnect();
    bool isServiceConnected() const { return isConnected; }
    
    // إدارة القنوات
    void createChannel(const std::string& channelId, const std::string& name);
    void subscribe(const std::string& channelId, const std::string& subscriberId);
    void unsubscribe(const std::string& channelId, const std::string& subscriberId);
    
    // إرسال الرسائل
    bool sendMessage(const std::string& channelId, const Message& message);
    bool sendMessage(const std::string& channelId, const std::string& to, 
                    const std::string& content, MessageType type = MessageType::TEXT);
    bool broadcastMessage(const std::string& channelId, const std::string& content);
    
    // استقبال الرسائل
    void setMessageHandler(const std::string& channelId, MessageHandler handler);
    std::vector<Message> receiveMessages(const std::string& channelId, int count = 10);
    Message receiveMessage(const std::string& channelId);
    
    // إدارة الطوابير
    void createQueue(const std::string& queueId);
    bool enqueue(const std::string& queueId, const Message& message);
    Message dequeue(const std::string& queueId);
    size_t getQueueSize(const std::string& queueId) const;
    
    // الإحصائيات
    size_t getChannelCount() const { return channels.size(); }
    size_t getTotalMessages() const;
    
    static std::unique_ptr<MessagingService> create();
};

// ==================== Cloud Services Manager ====================

// مدير خدمات السحابة الشامل
class CloudServicesManager {
private:
    std::unique_ptr<CloudStorage> storage;
    std::unique_ptr<CloudDatabase> database;
    std::unique_ptr<MessagingService> messaging;
    
public:
    CloudServicesManager();
    ~CloudServicesManager() = default;
    
    // الوصول للخدمات
    CloudStorage& getStorage();
    CloudDatabase& getDatabase();
    MessagingService& getMessaging();
    
    // التهيئة الشاملة
    void initialize(const std::string& storageUrl, const std::string& dbUrl, 
                   const std::string& messagingUrl, const std::string& apiKey);
    
    // الحالة
    bool isAllServicesReady() const;
    
    static std::unique_ptr<CloudServicesManager> create();
};

} // namespace ArabicLanguage

#endif // CLOUD_SERVICES_H

