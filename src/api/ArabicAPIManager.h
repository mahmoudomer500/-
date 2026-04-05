// ArabicAPIManager.h - مدير واجهات برمجة التطبيقات العربية
// Arabic API Manager - Central hub for all Arabic programming interfaces

#ifndef ARABIC_API_MANAGER_H
#define ARABIC_API_MANAGER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace ArabicCompiler {

// ════════════════════════════════════════════════════════════
// 🏢 مدير واجهات برمجة التطبيقات الرئيسي
// ════════════════════════════════════════════════════════════

class ArabicAPIManager {
private:
    // مديرو الواجهات المختلفة
    std::unique_ptr<class ArabicGraphicsAPIManager> graphicsManager;
    std::unique_ptr<class ArabicNetworkAPIManager> networkManager;
    std::unique_ptr<class ArabicDatabaseAPIManager> databaseManager;

    // حالة التهيئة
    bool initialized;
    std::vector<std::string> loadedAPIs;
    std::vector<std::string> failedAPIs;

public:
    ArabicAPIManager();
    ~ArabicAPIManager() = default;

    // تهيئة جميع الواجهات
    bool initializeAllAPIs();

    // الوصول للواجهات المختلفة
    class ArabicGraphicsAPIManager* getGraphicsManager() const { return graphicsManager.get(); }
    class ArabicNetworkAPIManager* getNetworkManager() const { return networkManager.get(); }
    class ArabicDatabaseAPIManager* getDatabaseManager() const { return databaseManager.get(); }

    // معلومات الحالة
    bool isInitialized() const { return initialized; }
    const std::vector<std::string>& getLoadedAPIs() const { return loadedAPIs; }
    const std::vector<std::string>& getFailedAPIs() const { return failedAPIs; }

    // دوال مساعدة
    std::string getVersion() const;
    std::vector<std::string> getAvailableAPIs() const;
};

// ════════════════════════════════════════════════════════════
// 🎨 مدير واجهات الرسومات
// ════════════════════════════════════════════════════════════

class ArabicGraphicsAPIManager {
private:
    std::unique_ptr<class ArabicOpenGL> opengl;
    std::unique_ptr<class ArabicDirectX> directx;
    std::unique_ptr<class ArabicWebGL> webgl;

    bool openglLoaded;
    bool directxLoaded;
    bool webglLoaded;

public:
    ArabicGraphicsAPIManager();
    ~ArabicGraphicsAPIManager() = default;

    // تهيئة واجهات الرسومات
    bool initializeAllAPIs();

    // الوصول للواجهات
    class ArabicOpenGL* getOpenGL() const { return opengl.get(); }
    class ArabicDirectX* getDirectX() const { return directx.get(); }
    class ArabicWebGL* getWebGL() const { return webgl.get(); }

    // معلومات الحالة
    bool isAllAPIsLoaded() const { return openglLoaded && directxLoaded && webglLoaded; }
    std::vector<std::string> getLoadedAPIs() const;
    std::vector<std::string> getFailedAPIs() const;

    // دوال مساعدة للرسومات
    static uint32_t rgbToColor(float r, float g, float b, float a = 1.0f);
    static void colorToRGB(uint32_t color, float& r, float& g, float& b, float& a);
    static void createIdentityMatrix(float* matrix);
    static void createPerspectiveMatrix(float* matrix, float fov, float aspect, float nearVal, float farVal);
    static void createOrthoMatrix(float* matrix, float left, float right, float bottom, float top, float nearVal, float farVal);
    static void multiplyMatrices(float* result, const float* a, const float* b);
};

// ════════════════════════════════════════════════════════════
// 🌐 مدير واجهات الشبكات
// ════════════════════════════════════════════════════════════

class ArabicNetworkAPIManager {
private:
    std::unique_ptr<class ArabicHTTP> http;
    std::unique_ptr<class ArabicWebSocket> websocket;
    std::unique_ptr<class ArabicWebSocketServer> websocketServer;
    std::unique_ptr<class ArabicAdvancedNetworking> advancedNetworking;

    bool httpLoaded;
    bool websocketLoaded;
    bool websocketServerLoaded;
    bool advancedNetworkingLoaded;

public:
    ArabicNetworkAPIManager();
    ~ArabicNetworkAPIManager() = default;

    // تهيئة واجهات الشبكات
    bool initializeAllAPIs();

    // الوصول للواجهات
    class ArabicHTTP* getHTTP() const { return http.get(); }
    class ArabicWebSocket* getWebSocket() const { return websocket.get(); }
    class ArabicWebSocketServer* getWebSocketServer() const { return websocketServer.get(); }
    class ArabicAdvancedNetworking* getAdvancedNetworking() const { return advancedNetworking.get(); }

    // معلومات الحالة
    bool isAllAPIsLoaded() const { return httpLoaded && websocketLoaded && websocketServerLoaded && advancedNetworkingLoaded; }
    std::vector<std::string> getLoadedAPIs() const;
    std::vector<std::string> getFailedAPIs() const;
};

// ════════════════════════════════════════════════════════════
// 🗄️ مدير واجهات قواعد البيانات
// ════════════════════════════════════════════════════════════

class ArabicDatabaseAPIManager {
private:
    std::unique_ptr<class ArabicSQLite> sqlite;
    std::unique_ptr<class ArabicODBC> odbc;
    std::unique_ptr<class ArabicMySQL> mysql;
    std::unique_ptr<class ArabicPostgreSQL> postgresql;
    std::unique_ptr<class ArabicORM> orm;

    bool sqliteLoaded;
    bool odbcLoaded;
    bool mysqlLoaded;
    bool postgresqlLoaded;
    bool ormLoaded;

public:
    ArabicDatabaseAPIManager();
    ~ArabicDatabaseAPIManager() = default;

    // تهيئة واجهات قواعد البيانات
    bool initializeAllAPIs();

    // الوصول للواجهات
    class ArabicSQLite* getSQLite() const { return sqlite.get(); }
    class ArabicODBC* getODBC() const { return odbc.get(); }
    class ArabicMySQL* getMySQL() const { return mysql.get(); }
    class ArabicPostgreSQL* getPostgreSQL() const { return postgresql.get(); }
    class ArabicORM* getORM() const { return orm.get(); }

    // معلومات الحالة
    bool isAllAPIsLoaded() const { return sqliteLoaded && odbcLoaded && mysqlLoaded && postgresqlLoaded && ormLoaded; }
    std::vector<std::string> getLoadedAPIs() const;
    std::vector<std::string> getFailedAPIs() const;
};

} // namespace ArabicCompiler

#endif // ARABIC_API_MANAGER_H
