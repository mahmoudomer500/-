#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <vector>
#include <string>
#include <map>

// Mock SQLite types if header is missing
#ifndef SQLITE3_H
struct sqlite3;
struct sqlite3_stmt;
#endif

namespace ArabicLanguage {

/**
 * @brief مكتبة قواعد البيانات العربية - Arabic Database Library
 *
 * توفر واجهة عربية كاملة للتعامل مع قواعد البيانات
 * مع دعم SQLite كمحرك أساسي وإمكانية التوسع لقواعد أخرى.
 */
class ArabicDatabase {
public:
    /**
     * @brief نتيجة عملية قاعدة البيانات
     */
    struct DatabaseResult {
        bool success;
        std::string errorMessage;
        int affectedRows;
        long long lastInsertId;
        std::chrono::nanoseconds executionTime;

        DatabaseResult() : success(false), affectedRows(0), lastInsertId(0) {}
        DatabaseResult(bool s, const std::string& err = "", int rows = 0, long long id = 0)
            : success(s), errorMessage(err), affectedRows(rows), lastInsertId(id) {}
    };

    /**
     * @brief صف من البيانات
     */
    struct DatabaseRow {
        std::unordered_map<std::string, std::string> columns;

        std::string getString(const std::string& column) const {
            auto it = columns.find(column);
            return it != columns.end() ? it->second : "";
        }

        int getInt(const std::string& column) const {
            auto it = columns.find(column);
            return it != columns.end() ? std::stoi(it->second) : 0;
        }

        double getDouble(const std::string& column) const {
            auto it = columns.find(column);
            return it != columns.end() ? std::stod(it->second) : 0.0;
        }

        long long getLongLong(const std::string& column) const {
            auto it = columns.find(column);
            return it != columns.end() ? std::stoll(it->second) : 0;
        }

        // Alias for compatibility
        const std::string& at(const std::string& column) const {
            return columns.at(column);
        }
    };

    /**
     * @brief نتيجة استعلام
     */
    struct QueryResult {
        bool success;
        std::string errorMessage;
        std::vector<std::string> columnNames;
        std::vector<DatabaseRow> rows;
        int affectedRows;
        std::chrono::nanoseconds executionTime;

        QueryResult() : success(false), affectedRows(0) {}

        size_t rowCount() const { return rows.size(); }
        bool empty() const { return rows.empty(); }

        // Iterators for range-based for loops
        auto begin() { return rows.begin(); }
        auto end() { return rows.end(); }
        auto begin() const { return rows.begin(); }
        auto end() const { return rows.end(); }
    };

    /**
     * @brief معلومات الاتصال
     */
    struct ConnectionInfo {
        std::string databasePath;
        bool readOnly;
        int timeoutMs;
        std::string encryptionKey;

        ConnectionInfo() : readOnly(false), timeoutMs(5000) {}
        ConnectionInfo(const std::string& path, bool ro = false, int timeout = 5000)
            : databasePath(path), readOnly(ro), timeoutMs(timeout) {}
    };

    /**
     * @brief معلومات الجدول
     */
    struct TableInfo {
        std::string name;
        std::vector<std::string> columns;
        std::unordered_map<std::string, std::string> columnTypes;
        bool exists;

        TableInfo() : exists(false) {}
    };

    /**
     * @brief إحصائيات الأداء
     */
    struct PerformanceStats {
        int totalQueries;
        int successfulQueries;
        int failedQueries;
        std::chrono::nanoseconds totalExecutionTime;
        size_t cacheHits;
        size_t cacheMisses;

        PerformanceStats() : totalQueries(0), successfulQueries(0), failedQueries(0),
                           cacheHits(0), cacheMisses(0) {}
    };

private:
    sqlite3* dbConnection;
    ConnectionInfo connectionInfo;
    PerformanceStats stats;
    std::mutex dbMutex;
    bool connected;

    // Cache for prepared statements
    std::unordered_map<std::string, sqlite3_stmt*> preparedStatements;

public:
    ArabicDatabase();
    ~ArabicDatabase();

    ArabicDatabase(const ArabicDatabase&) = delete;
    ArabicDatabase& operator=(const ArabicDatabase&) = delete;

    /**
     * @brief الاتصال بقاعدة البيانات
     */
    DatabaseResult connect(const ConnectionInfo& info);
    
    // Alias for compatibility
    bool initialize(const std::string& databasePath) {
        return connect(ConnectionInfo(databasePath)).success;
    }

    /**
     * @brief قطع الاتصال
     */
    DatabaseResult disconnect();

    /**
     * @brief التحقق من الاتصال
     */
    bool isConnected() const { return connected; }

    /**
     * @brief تنفيذ استعلام عام
     */
    QueryResult query(const std::string& sql);
    
    // Alias for compatibility
    QueryResult executeQuery(const std::string& sql) {
        return query(sql);
    }

    /**
     * @brief تنفيذ استعلام SELECT
     */
    QueryResult select(const std::string& table, const std::vector<std::string>& columns = {"*"},
                      const std::string& where = "", const std::string& orderBy = "",
                      int limit = -1, int offset = 0);

    /**
     * @brief إدراج بيانات
     */
    DatabaseResult insert(const std::string& table, const std::unordered_map<std::string, std::string>& data);

    /**
     * @brief تحديث بيانات
     */
    DatabaseResult update(const std::string& table, const std::unordered_map<std::string, std::string>& data,
                         const std::string& where);

    /**
     * @brief حذف بيانات
     */
    DatabaseResult remove(const std::string& table, const std::string& where);

    /**
     * @brief إنشاء جدول
     */
    DatabaseResult createTable(const std::string& tableName,
                              const std::unordered_map<std::string, std::string>& columns);

    /**
     * @brief حذف جدول
     */
    DatabaseResult dropTable(const std::string& tableName);

    /**
     * @brief فحص وجود جدول
     */
    bool tableExists(const std::string& tableName);

    /**
     * @brief الحصول على معلومات الجدول
     */
    TableInfo getTableInfo(const std::string& tableName);

    /**
     * @brief بدء معاملة
     */
    DatabaseResult beginTransaction();

    /**
     * @brief تأكيد المعاملة
     */
    DatabaseResult commitTransaction();

    /**
     * @brief التراجع عن المعاملة
     */
    DatabaseResult rollbackTransaction();

    /**
     * @brief تنفيذ استعلام مع معاملات آمنة (prepared statement)
     */
    QueryResult executePrepared(const std::string& sql, const std::vector<std::string>& parameters);

    /**
     * @brief إنشاء نسخة احتياطية
     */
    DatabaseResult createBackup(const std::string& backupPath);

    /**
     * @brief استعادة من نسخة احتياطية
     */
    DatabaseResult restoreFromBackup(const std::string& backupPath);

    /**
     * @brief الحصول على إحصائيات الأداء
     */
    const PerformanceStats& getPerformanceStats() const { return stats; }

    /**
     * @brief إعادة تعيين الإحصائيات
     */
    void resetStats() { stats = PerformanceStats(); }

    /**
     * @brief الحصول على قائمة الجداول
     */
    std::vector<std::string> getTableList();

    /**
     * @brief تنظيف قاعدة البيانات (VACUUM)
     */
    DatabaseResult vacuum();

    /**
     * @brief تحليل قاعدة البيانات (ANALYZE)
     */
    DatabaseResult analyze();

    /**
     * @brief تشخيص مشاكل قاعدة البيانات
     */
    std::vector<std::string> diagnoseDatabase();

    /**
     * @brief تصدير بيانات الجدول إلى CSV
     */
    DatabaseResult exportToCSV(const std::string& tableName, const std::string& filePath,
                              const std::string& delimiter = ",");

    /**
     * @brief استيراد بيانات من CSV
     */
    DatabaseResult importFromCSV(const std::string& tableName, const std::string& filePath,
                               const std::vector<std::string>& columns, const std::string& delimiter = ",");

    /**
     * @brief تنفيذ دفعة من الاستعلامات
     */
    std::vector<DatabaseResult> executeBatch(const std::vector<std::string>& queries);

    /**
     * @brief إنشاء فهرس
     */
    DatabaseResult createIndex(const std::string& tableName, const std::string& columnName,
                             const std::string& indexName = "");

    /**
     * @brief حذف فهرس
     */
    DatabaseResult dropIndex(const std::string& indexName);

    /**
     * @brief إعادة بناء الفهارس
     */
    DatabaseResult reindex();

    /**
     * @brief الحصول على حجم قاعدة البيانات
     */
    long long getDatabaseSize() const;

    /**
     * @brief الحصول على معلومات SQLite
     */
    std::unordered_map<std::string, std::string> getSQLiteInfo();
};

/**
 * @brief ORM بسيط للعربية - Arabic Simple ORM
 */
class ArabicORM {
private:
    std::shared_ptr<ArabicDatabase> database;

public:
    ArabicORM(std::shared_ptr<ArabicDatabase> db) : database(db) {}

    template<typename T>
    std::vector<T> query(const std::string& sql) {
        auto result = database->query(sql);
        return convertResults<T>(result);
    }

    template<typename T>
    T findOne(const std::string& tableName, const std::string& where) {
        auto result = database->select(tableName, {"*"}, where, "", 1);
        if (result.rows.empty()) {
            return T{};
        }
        return convertRow<T>(result.rows[0]);
    }

    template<typename T>
    std::vector<T> findMany(const std::string& tableName, const std::string& where = "",
                           const std::string& orderBy = "", int limit = -1) {
        auto result = database->select(tableName, {"*"}, where, orderBy, limit);
        return convertResults<T>(result);
    }

    template<typename T>
    ArabicDatabase::DatabaseResult save(const std::string& tableName, const T& object) {
        auto data = convertObjectToMap(object);
        return database->insert(tableName, data);
    }

    template<typename T>
    ArabicDatabase::DatabaseResult update(const std::string& tableName, const T& object,
                                         const std::string& where) {
        auto data = convertObjectToMap(object);
        return database->update(tableName, data, where);
    }

    ArabicDatabase::DatabaseResult remove(const std::string& tableName, const std::string& where) {
        return database->remove(tableName, where);
    }

private:
    template<typename T>
    std::vector<T> convertResults(const ArabicDatabase::QueryResult& result) {
        std::vector<T> objects;
        for (const auto& row : result.rows) {
            objects.push_back(convertRow<T>(row));
        }
        return objects;
    }

    template<typename T>
    T convertRow(const ArabicDatabase::DatabaseRow& row) {
        return T{};
    }

    template<typename T>
    std::unordered_map<std::string, std::string> convertObjectToMap(const T& object) {
        return {};
    }
};

/**
 * @brief Query Builder للاستعلامات المعقدة
 */
class ArabicQueryBuilder {
private:
    std::string tableName;
    std::vector<std::string> selectColumns;
    std::vector<std::string> whereConditions;
    std::vector<std::string> orderByColumns;
    std::vector<std::string> joinConditions;
    int limitValue;
    int offsetValue;
    std::unordered_map<std::string, std::string> parameters;

public:
    ArabicQueryBuilder(const std::string& table) : tableName(table), limitValue(-1), offsetValue(0) {}

    ArabicQueryBuilder& select(const std::vector<std::string>& columns) {
        selectColumns = columns;
        return *this;
    }

    ArabicQueryBuilder& where(const std::string& condition) {
        whereConditions.push_back(condition);
        return *this;
    }

    ArabicQueryBuilder& whereEqual(const std::string& column, const std::string& value) {
        whereConditions.push_back(column + " = ?");
        parameters[column] = value;
        return *this;
    }

    ArabicQueryBuilder& whereLike(const std::string& column, const std::string& pattern) {
        whereConditions.push_back(column + " LIKE ?");
        parameters[column] = pattern;
        return *this;
    }

    ArabicQueryBuilder& orderBy(const std::string& column, bool ascending = true) {
        orderByColumns.push_back(column + (ascending ? " ASC" : " DESC"));
        return *this;
    }

    ArabicQueryBuilder& limit(int limit) {
        limitValue = limit;
        return *this;
    }

    ArabicQueryBuilder& offset(int offset) {
        offsetValue = offset;
        return *this;
    }

    ArabicQueryBuilder& join(const std::string& table, const std::string& onCondition) {
        joinConditions.push_back("JOIN " + table + " ON " + onCondition);
        return *this;
    }

    std::string buildSelectSQL() const {
        std::string sql = "SELECT ";

        if (selectColumns.empty()) {
            sql += "*";
        } else {
            for (size_t i = 0; i < selectColumns.size(); ++i) {
                if (i > 0) sql += ", ";
                sql += selectColumns[i];
            }
        }

        sql += " FROM " + tableName;

        for (const auto& join : joinConditions) {
            sql += " " + join;
        }

        if (!whereConditions.empty()) {
            sql += " WHERE ";
            for (size_t i = 0; i < whereConditions.size(); ++i) {
                if (i > 0) sql += " AND ";
                sql += whereConditions[i];
            }
        }

        if (!orderByColumns.empty()) {
            sql += " ORDER BY ";
            for (size_t i = 0; i < orderByColumns.size(); ++i) {
                if (i > 0) sql += ", ";
                sql += orderByColumns[i];
            }
        }

        if (limitValue > 0) {
            sql += " LIMIT " + std::to_string(limitValue);
            if (offsetValue > 0) {
                sql += " OFFSET " + std::to_string(offsetValue);
            }
        }

        return sql;
    }

    const std::unordered_map<std::string, std::string>& getParameters() const {
        return parameters;
    }
};

} // namespace ArabicLanguage
