// ArabicDatabaseAPI.h - واجهة برمجة التطبيقات قواعد البيانات العربية
// Arabic Database API support for SQLite, ODBC, MySQL, and PostgreSQL

#ifndef ARABIC_DATABASE_API_H
#define ARABIC_DATABASE_API_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>
#include <cstdint>

namespace ArabicCompiler {

// ════════════════════════════════════════════════════════════
// 🗄️ أنواع البيانات المشتركة
// ════════════════════════════════════════════════════════════

enum class ArabicDatabaseType {
    SQLITE,
    ODBC,
    MYSQL,
    POSTGRESQL
};

enum class ArabicDataType {
    INTEGER,
    REAL,
    TEXT,
    BLOB,
    NULL_TYPE,
    DATETIME,
    BOOLEAN
};

struct ArabicDatabaseValue {
    ArabicDataType type;
    std::variant<int64_t, double, std::string, std::vector<uint8_t>, bool> value;

    ArabicDatabaseValue() : type(ArabicDataType::NULL_TYPE) {}
    ArabicDatabaseValue(int64_t val) : type(ArabicDataType::INTEGER), value(val) {}
    ArabicDatabaseValue(double val) : type(ArabicDataType::REAL), value(val) {}
    ArabicDatabaseValue(const std::string& val) : type(ArabicDataType::TEXT), value(val) {}
    ArabicDatabaseValue(const std::vector<uint8_t>& val) : type(ArabicDataType::BLOB), value(val) {}
    ArabicDatabaseValue(bool val) : type(ArabicDataType::BOOLEAN), value(val) {}

    std::string toString() const;
    int64_t toInteger() const;
    double toReal() const;
    std::vector<uint8_t> toBlob() const;
    bool toBoolean() const;
    bool isNull() const { return type == ArabicDataType::NULL_TYPE; }
};

struct ArabicDatabaseRow {
    std::unordered_map<std::string, ArabicDatabaseValue> columns;
    std::vector<std::string> columnNames;
    std::vector<ArabicDatabaseValue> values;

    ArabicDatabaseValue operator[](const std::string& columnName) const;
    ArabicDatabaseValue operator[](size_t index) const;
    size_t size() const { return values.size(); }
    bool empty() const { return values.empty(); }
};

struct ArabicDatabaseResult {
    std::vector<ArabicDatabaseRow> rows;
    std::vector<std::string> columnNames;
    int affectedRows;
    int64_t lastInsertId;
    std::string error;
    bool success;

    ArabicDatabaseResult() : affectedRows(0), lastInsertId(0), success(false) {}
    size_t size() const { return rows.size(); }
    bool empty() const { return rows.empty(); }
};

// ════════════════════════════════════════════════════════════
// 📊 دعم SQLite
// ════════════════════════════════════════════════════════════

class ArabicSQLite {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    uintptr_t databaseHandle;

public:
    ArabicSQLite();
    ~ArabicSQLite();

    // تهيئة SQLite
    bool initialize();
    void cleanup();

    // فتح وإغلاق قاعدة البيانات
    bool open(const std::string& databasePath);
    bool openMemory();
    bool close();
    bool isOpen() const;

    // تنفيذ الاستعلامات
    ArabicDatabaseResult execute(const std::string& sql);
    ArabicDatabaseResult query(const std::string& sql);
    bool executeNonQuery(const std::string& sql, int& affectedRows);

    // استعلامات محضرة (Prepared Statements)
    uintptr_t prepareStatement(const std::string& sql);
    bool bindParameter(uintptr_t statement, int index, const ArabicDatabaseValue& value);
    bool bindParameter(uintptr_t statement, const std::string& name, const ArabicDatabaseValue& value);
    ArabicDatabaseResult executePrepared(uintptr_t statement);
    void finalizeStatement(uintptr_t statement);

    // معاملات قاعدة البيانات
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // دوال مساعدة
    int64_t getLastInsertRowId() const;
    int getChanges() const;
    int getTotalChanges() const;
    std::string getVersion() const;

    // إنشاء الجداول والفهارس
    bool createTable(const std::string& tableName, const std::vector<std::pair<std::string, std::string>>& columns);
    bool createIndex(const std::string& indexName, const std::string& tableName, const std::vector<std::string>& columns);
    bool dropTable(const std::string& tableName);
    bool dropIndex(const std::string& indexName);

    // دوال النسخ الاحتياطي
    bool backup(const std::string& destinationPath);
    bool restore(const std::string& sourcePath);

    // إعدادات متقدمة
    void setBusyTimeout(int milliseconds);
    void enableForeignKeys(bool enable);
    void enableWALMode(bool enable);
    void setCacheSize(int pages);
    void setJournalMode(const std::string& mode);

    std::string getLastError() const;
    bool isLoaded() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 🔗 دعم ODBC
// ════════════════════════════════════════════════════════════

class ArabicODBC {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    uintptr_t environmentHandle;
    uintptr_t connectionHandle;

public:
    ArabicODBC();
    ~ArabicODBC();

    // تهيئة ODBC
    bool initialize();
    void cleanup();

    // الاتصال بقاعدة البيانات
    bool connect(const std::string& connectionString);
    bool connect(const std::string& dsn, const std::string& user = "", const std::string& password = "");
    bool disconnect();
    bool isConnected() const;

    // تنفيذ الاستعلامات
    ArabicDatabaseResult execute(const std::string& sql);
    ArabicDatabaseResult query(const std::string& sql);
    bool executeNonQuery(const std::string& sql, int& affectedRows);

    // استعلامات محضرة
    uintptr_t prepareStatement(const std::string& sql);
    bool bindParameter(uintptr_t statement, int index, const ArabicDatabaseValue& value);
    ArabicDatabaseResult executePrepared(uintptr_t statement);
    void finalizeStatement(uintptr_t statement);

    // معاملات قاعدة البيانات
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // معلومات قاعدة البيانات
    std::string getDriverName() const;
    std::string getDatabaseName() const;
    std::string getServerName() const;
    std::vector<std::string> getTableNames();
    std::unordered_map<std::string, std::string> getTableSchema(const std::string& tableName);

    // إعدادات الاتصال
    void setConnectionTimeout(int seconds);
    void setCommandTimeout(int seconds);
    void setAutoCommit(bool autoCommit);

    std::string getLastError() const;
    bool isLoaded() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 🐬 دعم MySQL
// ════════════════════════════════════════════════════════════

class ArabicMySQL {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    uintptr_t connectionHandle;

public:
    ArabicMySQL();
    ~ArabicMySQL();

    // تهيئة MySQL
    bool initialize();
    void cleanup();

    // الاتصال بقاعدة البيانات
    bool connect(const std::string& host, int port, const std::string& database,
                const std::string& user, const std::string& password);
    bool connect(const std::string& connectionString);
    bool disconnect();
    bool isConnected() const;
    bool reconnect();

    // تنفيذ الاستعلامات
    ArabicDatabaseResult execute(const std::string& sql);
    ArabicDatabaseResult query(const std::string& sql);
    bool executeNonQuery(const std::string& sql, int& affectedRows);

    // استعلامات محضرة
    uintptr_t prepareStatement(const std::string& sql);
    bool bindParameter(uintptr_t statement, int index, const ArabicDatabaseValue& value);
    ArabicDatabaseResult executePrepared(uintptr_t statement);
    void finalizeStatement(uintptr_t statement);

    // معاملات قاعدة البيانات
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    bool setAutoCommit(bool autoCommit);

    // دوال MySQL الخاصة
    int64_t getLastInsertId() const;
    int getAffectedRows() const;
    std::string getServerVersion() const;
    std::string getClientVersion() const;
    bool ping();

    // إدارة قاعدة البيانات
    bool createDatabase(const std::string& databaseName);
    bool dropDatabase(const std::string& databaseName);
    bool useDatabase(const std::string& databaseName);
    std::vector<std::string> getDatabaseNames();
    std::vector<std::string> getTableNames(const std::string& databaseName = "");

    // معلومات الجداول
    std::unordered_map<std::string, std::string> getTableSchema(const std::string& tableName);
    bool tableExists(const std::string& tableName);

    // إعدادات متقدمة
    void setCharset(const std::string& charset);
    void setTimezone(const std::string& timezone);
    void setConnectTimeout(int seconds);
    void setReadTimeout(int seconds);
    void setWriteTimeout(int seconds);
    void enableCompression(bool enable);
    void enableSSL(const std::string& key = "", const std::string& cert = "", const std::string& ca = "");

    std::string getLastError() const;
    bool isLoaded() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 🐘 دعم PostgreSQL
// ════════════════════════════════════════════════════════════

class ArabicPostgreSQL {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    uintptr_t connectionHandle;

public:
    ArabicPostgreSQL();
    ~ArabicPostgreSQL();

    // تهيئة PostgreSQL
    bool initialize();
    void cleanup();

    // الاتصال بقاعدة البيانات
    bool connect(const std::string& connectionString);
    bool connect(const std::string& host, int port, const std::string& database,
                const std::string& user, const std::string& password);
    bool disconnect();
    bool isConnected() const;
    bool resetConnection();

    // تنفيذ الاستعلامات
    ArabicDatabaseResult execute(const std::string& sql);
    ArabicDatabaseResult query(const std::string& sql);
    bool executeNonQuery(const std::string& sql, int& affectedRows);

    // استعلامات محضرة
    uintptr_t prepareStatement(const std::string& name, const std::string& sql);
    bool bindParameter(uintptr_t statement, int index, const ArabicDatabaseValue& value);
    ArabicDatabaseResult executePrepared(uintptr_t statement);
    void deallocatePrepared(uintptr_t statement);

    // معاملات قاعدة البيانات
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    bool setAutoCommit(bool autoCommit);
    bool savepoint(const std::string& name);
    bool rollbackToSavepoint(const std::string& name);
    bool releaseSavepoint(const std::string& name);

    // دوال PostgreSQL الخاصة
    int64_t getLastInsertId(const std::string& sequenceName = "");
    int getAffectedRows() const;
    std::string getServerVersion() const;
    std::string getClientVersion() const;
    bool ping();

    // إدارة قاعدة البيانات والمخططات
    bool createDatabase(const std::string& databaseName, const std::string& owner = "");
    bool dropDatabase(const std::string& databaseName);
    std::vector<std::string> getDatabaseNames();
    std::vector<std::string> getSchemaNames();
    std::vector<std::string> getTableNames(const std::string& schemaName = "public");

    // معلومات الجداول والأعمدة
    std::unordered_map<std::string, std::string> getTableSchema(const std::string& tableName, const std::string& schemaName = "public");
    bool tableExists(const std::string& tableName, const std::string& schemaName = "public");
    std::vector<std::string> getColumnNames(const std::string& tableName, const std::string& schemaName = "public");

    // دوال JSON وأنواع البيانات المتقدمة
    ArabicDatabaseResult executeJSONQuery(const std::string& jsonQuery);
    bool createJSONIndex(const std::string& tableName, const std::string& columnName, const std::string& indexName = "");

    // إعدادات متقدمة
    void setClientEncoding(const std::string& encoding);
    void setTimezone(const std::string& timezone);
    void setConnectTimeout(int seconds);
    void setApplicationName(const std::string& appName);
    void enableSSL(const std::string& sslMode = "require", const std::string& cert = "", const std::string& key = "");

    // إشعارات واستماع
    bool listen(const std::string& channel);
    bool unlisten(const std::string& channel);
    bool notify(const std::string& channel, const std::string& payload = "");
    std::string getNotification();

    std::string getLastError() const;
    bool isLoaded() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 🔄 دعم ORM بسيط
// ════════════════════════════════════════════════════════════

class ArabicORM {
private:
    ArabicDatabaseType dbType;
    void* dbConnection; // Pointer to the actual database connection

public:
    ArabicORM(ArabicDatabaseType type, void* connection);

    // عمليات CRUD أساسية
    bool insert(const std::string& tableName, const std::unordered_map<std::string, ArabicDatabaseValue>& data, int64_t& newId);
    bool update(const std::string& tableName, const std::unordered_map<std::string, ArabicDatabaseValue>& data,
               const std::string& whereClause, int& affectedRows);
    bool delete_(const std::string& tableName, const std::string& whereClause, int& affectedRows);
    ArabicDatabaseResult select(const std::string& tableName, const std::vector<std::string>& columns = {},
                               const std::string& whereClause = "", const std::string& orderBy = "",
                               int limit = -1, int offset = 0);

    // استعلامات متقدمة
    ArabicDatabaseResult query(const std::string& sql, const std::vector<ArabicDatabaseValue>& params = {});
    bool executeScript(const std::string& sqlScript);

    // إدارة الجداول
    bool createTable(const std::string& tableName, const std::unordered_map<std::string, std::string>& columns,
                    const std::string& primaryKey = "", const std::vector<std::string>& indexes = {});
    bool alterTable(const std::string& tableName, const std::string& alterCommand);
    bool dropTable(const std::string& tableName);

    // دوال مساعدة
    std::string escapeString(const std::string& str) const;
    std::string quoteIdentifier(const std::string& identifier) const;
    std::string buildWhereClause(const std::unordered_map<std::string, ArabicDatabaseValue>& conditions) const;
};

// ════════════════════════════════════════════════════════════
// 📦 مدير قواعد البيانات الرئيسي
// ════════════════════════════════════════════════════════════

class ArabicDatabaseAPIManager {
private:
    std::unique_ptr<ArabicSQLite> sqlite;
    std::unique_ptr<ArabicODBC> odbc;
    std::unique_ptr<ArabicMySQL> mysql;
    std::unique_ptr<ArabicPostgreSQL> postgresql;

public:
    ArabicDatabaseAPIManager();
    ~ArabicDatabaseAPIManager() = default;

    // الوصول للواجهات المختلفة
    ArabicSQLite* getSQLite() const { return sqlite.get(); }
    ArabicODBC* getODBC() const { return odbc.get(); }
    ArabicMySQL* getMySQL() const { return mysql.get(); }
    ArabicPostgreSQL* getPostgreSQL() const { return postgresql.get(); }

    // دوال عامة
    bool initializeAllAPIs();
    bool isAllAPIsLoaded() const;
    std::vector<std::string> getLoadedAPIs() const;
    std::vector<std::string> getFailedAPIs() const;

    // دوال مساعدة لقواعد البيانات
    static ArabicDatabaseType detectDatabaseType(const std::string& connectionString);
    static std::string createConnectionString(ArabicDatabaseType type, const std::unordered_map<std::string, std::string>& params);
    static std::string escapeSQLString(const std::string& str, ArabicDatabaseType type);
    static std::string quoteSQLIdentifier(const std::string& identifier, ArabicDatabaseType type);

    // أدوات البيانات
    static ArabicDatabaseResult mergeResults(const std::vector<ArabicDatabaseResult>& results);
    static bool exportToCSV(const ArabicDatabaseResult& result, const std::string& filePath, char delimiter = ',');
    static bool exportToJSON(const ArabicDatabaseResult& result, const std::string& filePath);
    static ArabicDatabaseResult importFromCSV(const std::string& filePath, char delimiter = ',');
};

} // namespace ArabicCompiler

#endif // ARABIC_DATABASE_API_H