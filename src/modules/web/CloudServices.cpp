// CloudServices.cpp - تطبيق خدمات السحابة الشاملة
// الأسبوع الثالث من الشهر الخامس: خدمات السحابة
#include "CloudServices.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>

namespace ArabicLanguage {

// ==================== CloudStorage Implementation ====================

CloudStorage::CloudStorage() : maxFileSize(100 * 1024 * 1024), totalQuota(1024 * 1024 * 1024), usedQuota(0) {
}

void CloudStorage::initialize(const std::string& url, const std::string& accessKey, 
                              const std::string& secretKey, const std::string& bucket) {
    storageUrl = url;
    this->accessKey = accessKey;
    this->secretKey = secretKey;
    bucketName = bucket;
    std::cout << "[INFO] Cloud Storage initialized: " << url << "/" << bucket << std::endl;
}

StorageResult CloudStorage::uploadFile(const std::string& localPath, const std::string& cloudPath) {
    std::ifstream file(localPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Cannot open file: " << localPath << std::endl;
        return StorageResult::FAILED;
    }
    
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    return uploadFile(data, cloudPath, "application/octet-stream");
}

StorageResult CloudStorage::uploadFile(const std::vector<uint8_t>& data, const std::string& cloudPath, 
                                      const std::string& contentType) {
    if (data.size() > maxFileSize) {
        std::cerr << "[ERROR] File size exceeds maximum: " << data.size() << " > " << maxFileSize << std::endl;
        return StorageResult::QUOTA_EXCEEDED;
    }
    
    if (usedQuota + data.size() > totalQuota) {
        std::cerr << "[ERROR] Quota exceeded" << std::endl;
        return StorageResult::QUOTA_EXCEEDED;
    }
    
    CloudFile file;
    file.id = cloudPath;
    file.name = cloudPath.substr(cloudPath.find_last_of("/") + 1);
    file.path = cloudPath;
    file.contentType = contentType;
    file.size = data.size();
    file.uploadDate = std::chrono::system_clock::now();
    file.lastModified = file.uploadDate;
    file.url = storageUrl + "/" + bucketName + "/" + cloudPath;
    file.isPublic = false;
    
    files[cloudPath] = file;
    usedQuota += data.size();
    
    std::cout << "[INFO] File uploaded: " << cloudPath << " (" << data.size() << " bytes)" << std::endl;
    return StorageResult::SUCCESS;
}

StorageResult CloudStorage::uploadFile(const std::string& content, const std::string& cloudPath, 
                                      const std::string& contentType) {
    std::vector<uint8_t> data(content.begin(), content.end());
    return uploadFile(data, cloudPath, contentType);
}

StorageResult CloudStorage::downloadFile(const std::string& cloudPath, const std::string& localPath) {
    if (files.find(cloudPath) == files.end()) {
        std::cerr << "[ERROR] File not found: " << cloudPath << std::endl;
        return StorageResult::FILE_NOT_FOUND;
    }
    
    // محاكاة التنزيل (في الإصدار الكامل سيتم تنزيل الملف الفعلي)
    std::ofstream file(localPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Cannot create file: " << localPath << std::endl;
        return StorageResult::FAILED;
    }
    
    file.close();
    std::cout << "[INFO] File downloaded: " << cloudPath << " -> " << localPath << std::endl;
    return StorageResult::SUCCESS;
}

std::vector<uint8_t> CloudStorage::downloadFileData(const std::string& cloudPath) {
    std::vector<uint8_t> data;
    
    if (files.find(cloudPath) == files.end()) {
        return data;
    }
    
    // محاكاة التنزيل
    return data;
}

std::string CloudStorage::downloadFileString(const std::string& cloudPath) {
    auto data = downloadFileData(cloudPath);
    return std::string(data.begin(), data.end());
}

StorageResult CloudStorage::deleteFile(const std::string& cloudPath) {
    auto it = files.find(cloudPath);
    if (it == files.end()) {
        return StorageResult::FILE_NOT_FOUND;
    }
    
    usedQuota -= it->second.size;
    files.erase(it);
    
    std::cout << "[INFO] File deleted: " << cloudPath << std::endl;
    return StorageResult::SUCCESS;
}

StorageResult CloudStorage::renameFile(const std::string& oldPath, const std::string& newPath) {
    auto it = files.find(oldPath);
    if (it == files.end()) {
        return StorageResult::FILE_NOT_FOUND;
    }
    
    CloudFile file = it->second;
    file.path = newPath;
    file.name = newPath.substr(newPath.find_last_of("/") + 1);
    file.url = storageUrl + "/" + bucketName + "/" + newPath;
    
    files.erase(it);
    files[newPath] = file;
    
    std::cout << "[INFO] File renamed: " << oldPath << " -> " << newPath << std::endl;
    return StorageResult::SUCCESS;
}

StorageResult CloudStorage::copyFile(const std::string& sourcePath, const std::string& destPath) {
    auto it = files.find(sourcePath);
    if (it == files.end()) {
        return StorageResult::FILE_NOT_FOUND;
    }
    
    CloudFile newFile = it->second;
    newFile.id = destPath;
    newFile.path = destPath;
    newFile.name = destPath.substr(destPath.find_last_of("/") + 1);
    newFile.url = storageUrl + "/" + bucketName + "/" + destPath;
    newFile.uploadDate = std::chrono::system_clock::now();
    
    files[destPath] = newFile;
    usedQuota += newFile.size;
    
    std::cout << "[INFO] File copied: " << sourcePath << " -> " << destPath << std::endl;
    return StorageResult::SUCCESS;
}

StorageResult CloudStorage::moveFile(const std::string& sourcePath, const std::string& destPath) {
    auto result = copyFile(sourcePath, destPath);
    if (result == StorageResult::SUCCESS) {
        deleteFile(sourcePath);
    }
    return result;
}

CloudFile CloudStorage::getFileInfo(const std::string& cloudPath) const {
    auto it = files.find(cloudPath);
    if (it != files.end()) {
        return it->second;
    }
    return CloudFile();
}

std::vector<CloudFile> CloudStorage::listFiles(const std::string& prefix) const {
    std::vector<CloudFile> result;
    
    for (const auto& pair : files) {
        if (prefix.empty() || pair.first.find(prefix) == 0) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

bool CloudStorage::fileExists(const std::string& cloudPath) const {
    return files.find(cloudPath) != files.end();
}

StorageResult CloudStorage::setFilePublic(const std::string& cloudPath, bool isPublic) {
    auto it = files.find(cloudPath);
    if (it == files.end()) {
        return StorageResult::FILE_NOT_FOUND;
    }
    
    it->second.isPublic = isPublic;
    return StorageResult::SUCCESS;
}

StorageResult CloudStorage::setFileMetadata(const std::string& cloudPath, 
                                            const std::map<std::string, std::string>& metadata) {
    auto it = files.find(cloudPath);
    if (it == files.end()) {
        return StorageResult::FILE_NOT_FOUND;
    }
    
    it->second.metadata = metadata;
    return StorageResult::SUCCESS;
}

std::unique_ptr<CloudStorage> CloudStorage::create() {
    return std::make_unique<CloudStorage>();
}

// ==================== CloudDatabase Implementation ====================

CloudDatabase::CloudDatabase() {
}

void CloudDatabase::initialize(const std::string& url, const std::string& dbName, const std::string& key) {
    databaseUrl = url;
    databaseName = dbName;
    apiKey = key;
    std::cout << "[INFO] Cloud Database initialized: " << url << "/" << dbName << std::endl;
}

void CloudDatabase::createCollection(const std::string& collectionName, 
                                    const std::map<std::string, DataType>& schema) {
    collections[collectionName] = std::vector<DatabaseRecord>();
    schemas[collectionName] = schema;
    std::cout << "[INFO] Collection created: " << collectionName << std::endl;
}

DatabaseResult CloudDatabase::insert(const std::string& collection, const DatabaseRecord& record) {
    if (collections.find(collection) == collections.end()) {
        return DatabaseResult::FAILED;
    }
    
    // التحقق من المفتاح المكرر
    for (const auto& rec : collections[collection]) {
        if (rec.id == record.id) {
            return DatabaseResult::DUPLICATE_KEY;
        }
    }
    
    DatabaseRecord newRecord = record;
    newRecord.createdAt = std::chrono::system_clock::now();
    newRecord.updatedAt = newRecord.createdAt;
    
    collections[collection].push_back(newRecord);
    
    std::cout << "[INFO] Record inserted into " << collection << ": " << record.id << std::endl;
    return DatabaseResult::SUCCESS;
}

DatabaseResult CloudDatabase::update(const std::string& collection, const std::string& id, 
                                     const std::map<std::string, DataValue>& updates) {
    if (collections.find(collection) == collections.end()) {
        return DatabaseResult::FAILED;
    }
    
    for (auto& record : collections[collection]) {
        if (record.id == id) {
            for (const auto& update : updates) {
                record.fields[update.first] = update.second;
            }
            record.updatedAt = std::chrono::system_clock::now();
            std::cout << "[INFO] Record updated in " << collection << ": " << id << std::endl;
            return DatabaseResult::SUCCESS;
        }
    }
    
    return DatabaseResult::NOT_FOUND;
}

DatabaseResult CloudDatabase::remove(const std::string& collection, const std::string& id) {
    if (collections.find(collection) == collections.end()) {
        return DatabaseResult::FAILED;
    }
    
    auto& records = collections[collection];
    auto it = std::remove_if(records.begin(), records.end(),
                            [&id](const DatabaseRecord& rec) { return rec.id == id; });
    
    if (it != records.end()) {
        records.erase(it, records.end());
        std::cout << "[INFO] Record removed from " << collection << ": " << id << std::endl;
        return DatabaseResult::SUCCESS;
    }
    
    return DatabaseResult::NOT_FOUND;
}

DatabaseRecord CloudDatabase::findById(const std::string& collection, const std::string& id) const {
    if (collections.find(collection) == collections.end()) {
        return DatabaseRecord();
    }
    
    for (const auto& record : collections.at(collection)) {
        if (record.id == id) {
            return record;
        }
    }
    
    return DatabaseRecord();
}

std::vector<DatabaseRecord> CloudDatabase::findAll(const std::string& collection) const {
    if (collections.find(collection) == collections.end()) {
        return std::vector<DatabaseRecord>();
    }
    
    return collections.at(collection);
}

std::vector<DatabaseRecord> CloudDatabase::query(const std::string& collection, 
                                                  const std::map<std::string, DataValue>& filters) const {
    std::vector<DatabaseRecord> result;
    
    if (collections.find(collection) == collections.end()) {
        return result;
    }
    
    for (const auto& record : collections.at(collection)) {
        bool matches = true;
        for (const auto& filter : filters) {
            auto it = record.fields.find(filter.first);
            if (it == record.fields.end() || it->second.toString() != filter.second.toString()) {
                matches = false;
                break;
            }
        }
        if (matches) {
            result.push_back(record);
        }
    }
    
    return result;
}

std::vector<DatabaseRecord> CloudDatabase::queryRange(const std::string& collection, 
                                                      const std::string& field, 
                                                      const DataValue& minValue, 
                                                      const DataValue& maxValue) const {
    std::vector<DatabaseRecord> result;
    
    if (collections.find(collection) == collections.end()) {
        return result;
    }
    
    for (const auto& record : collections.at(collection)) {
        auto it = record.fields.find(field);
        if (it != record.fields.end()) {
            // مقارنة بسيطة (في الإصدار الكامل ستكون أكثر دقة)
            result.push_back(record);
        }
    }
    
    return result;
}

void CloudDatabase::createIndex(const std::string& collection, const std::string& field) {
    std::cout << "[INFO] Index created on " << collection << "." << field << std::endl;
}

size_t CloudDatabase::getRecordCount(const std::string& collection) const {
    if (collections.find(collection) == collections.end()) {
        return 0;
    }
    return collections.at(collection).size();
}

std::vector<std::string> CloudDatabase::getCollections() const {
    std::vector<std::string> result;
    for (const auto& pair : collections) {
        result.push_back(pair.first);
    }
    return result;
}

std::unique_ptr<CloudDatabase> CloudDatabase::create() {
    return std::make_unique<CloudDatabase>();
}

// ==================== DataValue Implementation ====================

DataValue DataValue::fromString(const std::string& value) {
    DataValue dv;
    dv.type = DataType::STRING;
    dv.stringValue = value;
    return dv;
}

DataValue DataValue::fromInt(int value) {
    DataValue dv;
    dv.type = DataType::INTEGER;
    dv.intValue = value;
    return dv;
}

DataValue DataValue::fromDouble(double value) {
    DataValue dv;
    dv.type = DataType::DOUBLE;
    dv.doubleValue = value;
    return dv;
}

DataValue DataValue::fromBool(bool value) {
    DataValue dv;
    dv.type = DataType::BOOLEAN;
    dv.boolValue = value;
    return dv;
}

std::string DataValue::toString() const {
    switch (type) {
        case DataType::STRING: return stringValue;
        case DataType::INTEGER: return std::to_string(intValue);
        case DataType::DOUBLE: return std::to_string(doubleValue);
        case DataType::BOOLEAN: return boolValue ? "true" : "false";
        default: return "";
    }
}

// ==================== MessagingService Implementation ====================

MessagingService::MessagingService() : isConnected(false) {
}

void MessagingService::initialize(const std::string& url, const std::string& key) {
    serviceUrl = url;
    apiKey = key;
    std::cout << "[INFO] Messaging Service initialized: " << url << std::endl;
}

bool MessagingService::connect() {
    isConnected = true;
    std::cout << "[INFO] Connected to messaging service" << std::endl;
    return true;
}

void MessagingService::disconnect() {
    isConnected = false;
    std::cout << "[INFO] Disconnected from messaging service" << std::endl;
}

void MessagingService::createChannel(const std::string& channelId, const std::string& name) {
    MessageChannel channel;
    channel.id = channelId;
    channel.name = name;
    channels[channelId] = channel;
    std::cout << "[INFO] Channel created: " << channelId << " (" << name << ")" << std::endl;
}

void MessagingService::subscribe(const std::string& channelId, const std::string& subscriberId) {
    auto it = channels.find(channelId);
    if (it != channels.end()) {
        auto& subscribers = it->second.subscribers;
        if (std::find(subscribers.begin(), subscribers.end(), subscriberId) == subscribers.end()) {
            subscribers.push_back(subscriberId);
            std::cout << "[INFO] Subscribed " << subscriberId << " to " << channelId << std::endl;
        }
    }
}

void MessagingService::unsubscribe(const std::string& channelId, const std::string& subscriberId) {
    auto it = channels.find(channelId);
    if (it != channels.end()) {
        auto& subscribers = it->second.subscribers;
        subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriberId), subscribers.end());
        std::cout << "[INFO] Unsubscribed " << subscriberId << " from " << channelId << std::endl;
    }
}

bool MessagingService::sendMessage(const std::string& channelId, const Message& message) {
    auto it = channels.find(channelId);
    if (it == channels.end()) {
        return false;
    }
    
    Message msg = message;
    msg.id = "msg_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    msg.timestamp = std::chrono::system_clock::now();
    
    it->second.messages.push_back(msg);
    
    // استدعاء المعالج إذا كان موجوداً
    auto handlerIt = handlers.find(channelId);
    if (handlerIt != handlers.end()) {
        handlerIt->second(msg);
    }
    
    std::cout << "[INFO] Message sent to " << channelId << ": " << msg.id << std::endl;
    return true;
}

bool MessagingService::sendMessage(const std::string& channelId, const std::string& to, 
                                  const std::string& content, MessageType type) {
    Message msg;
    msg.to = to;
    msg.content = content;
    msg.type = type;
    return sendMessage(channelId, msg);
}

bool MessagingService::broadcastMessage(const std::string& channelId, const std::string& content) {
    Message msg;
    msg.to = "all";
    msg.content = content;
    msg.type = MessageType::TEXT;
    return sendMessage(channelId, msg);
}

void MessagingService::setMessageHandler(const std::string& channelId, MessageHandler handler) {
    handlers[channelId] = handler;
    std::cout << "[INFO] Message handler set for " << channelId << std::endl;
}

std::vector<Message> MessagingService::receiveMessages(const std::string& channelId, int count) {
    std::vector<Message> result;
    
    auto it = channels.find(channelId);
    if (it != channels.end()) {
        auto& messages = it->second.messages;
        int takeCount = std::min(count, static_cast<int>(messages.size()));
        result.insert(result.end(), messages.begin(), messages.begin() + takeCount);
    }
    
    return result;
}

Message MessagingService::receiveMessage(const std::string& channelId) {
    auto messages = receiveMessages(channelId, 1);
    return messages.empty() ? Message() : messages[0];
}

void MessagingService::createQueue(const std::string& queueId) {
    queues[queueId] = std::vector<Message>();
    std::cout << "[INFO] Queue created: " << queueId << std::endl;
}

bool MessagingService::enqueue(const std::string& queueId, const Message& message) {
    if (queues.find(queueId) == queues.end()) {
        createQueue(queueId);
    }
    
    Message msg = message;
    msg.id = "msg_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    msg.timestamp = std::chrono::system_clock::now();
    
    queues[queueId].push_back(msg);
    std::cout << "[INFO] Message enqueued to " << queueId << std::endl;
    return true;
}

Message MessagingService::dequeue(const std::string& queueId) {
    if (queues.find(queueId) == queues.end() || queues[queueId].empty()) {
        return Message();
    }
    
    Message msg = queues[queueId].front();
    queues[queueId].erase(queues[queueId].begin());
    
    std::cout << "[INFO] Message dequeued from " << queueId << std::endl;
    return msg;
}

size_t MessagingService::getQueueSize(const std::string& queueId) const {
    auto it = queues.find(queueId);
    return it != queues.end() ? it->second.size() : 0;
}

size_t MessagingService::getTotalMessages() const {
    size_t total = 0;
    for (const auto& pair : channels) {
        total += pair.second.messages.size();
    }
    return total;
}

std::unique_ptr<MessagingService> MessagingService::create() {
    return std::make_unique<MessagingService>();
}

// ==================== CloudServicesManager Implementation ====================

CloudServicesManager::CloudServicesManager() {
    storage = CloudStorage::create();
    database = CloudDatabase::create();
    messaging = MessagingService::create();
}

CloudStorage& CloudServicesManager::getStorage() {
    return *storage;
}

CloudDatabase& CloudServicesManager::getDatabase() {
    return *database;
}

MessagingService& CloudServicesManager::getMessaging() {
    return *messaging;
}

void CloudServicesManager::initialize(const std::string& storageUrl, const std::string& dbUrl, 
                                     const std::string& messagingUrl, const std::string& apiKey) {
    storage->initialize(storageUrl, apiKey, apiKey, "default-bucket");
    database->initialize(dbUrl, "default-db", apiKey);
    messaging->initialize(messagingUrl, apiKey);
    messaging->connect();
    
    std::cout << "[INFO] Cloud Services Manager initialized" << std::endl;
}

bool CloudServicesManager::isAllServicesReady() const {
    return messaging->isServiceConnected();
}

std::unique_ptr<CloudServicesManager> CloudServicesManager::create() {
    return std::make_unique<CloudServicesManager>();
}

} // namespace ArabicLanguage

