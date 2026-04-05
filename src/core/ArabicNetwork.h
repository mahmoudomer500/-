#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <condition_variable>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include "SafeWindows.h"
#include <winhttp.h>

namespace ArabicLanguage {

/**
 * @brief مكتبة الشبكات العربية - Arabic Network Library
 *
 * توفر واجهة شاملة للتواصل عبر الشبكات مع دعم HTTP, HTTPS, WebSocket, TCP
 * مع التركيز على الأمان والأداء والاستخدام السهل.
 */
class ArabicNetwork {
public:
    /**
     * @brief نتيجة عملية الشبكة
     */
    struct NetworkResult {
        bool success;
        std::string errorMessage;
        std::chrono::nanoseconds responseTime;
        int statusCode;
        std::string responseBody;
        std::unordered_map<std::string, std::string> responseHeaders;

        NetworkResult() : success(false), statusCode(0) {}
        NetworkResult(bool s, const std::string& err = "", int code = 0,
                     const std::string& body = "")
            : success(s), errorMessage(err), statusCode(code), responseBody(body) {}
    };

    /**
     * @brief بيانات الطلب
     */
    struct RequestData {
        std::string url;
        std::string method;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
        int timeoutMs;
        bool followRedirects;
        std::string proxyUrl;
        std::string userAgent;

        RequestData() : timeoutMs(30000), followRedirects(true),
                       userAgent("ArabicLanguage/1.0") {}
    };

    /**
     * @brief معلومات الاتصال
     */
    struct ConnectionInfo {
        std::string host;
        int port;
        bool useSSL;
        std::string username;
        std::string password;
        int timeoutMs;
        bool keepAlive;

        ConnectionInfo() : port(80), useSSL(false), timeoutMs(30000), keepAlive(true) {}
    };

    /**
     * @brief إحصائيات الشبكة
     */
    struct NetworkStats {
        int totalRequests;
        int successfulRequests;
        int failedRequests;
        std::chrono::nanoseconds totalResponseTime;
        size_t totalBytesSent;
        size_t totalBytesReceived;
        int activeConnections;
        double avgResponseTime;

        NetworkStats() : totalRequests(0), successfulRequests(0), failedRequests(0),
                        totalBytesSent(0), totalBytesReceived(0), activeConnections(0),
                        avgResponseTime(0.0) {}
    };

private:
    NetworkStats stats;
    std::mutex networkMutex;
    HINTERNET hSession;
    std::atomic<bool> initialized;

    // Thread pool for async operations
    std::vector<std::thread> workerThreads;
    std::queue<std::function<void()>> taskQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::atomic<bool> stopWorkers;

    /**
     * @brief تهيئة WinHTTP session
     */
    bool initializeSession();

    /**
     * @brief تنظيف الموارد
     */
    void cleanup();

    /**
     * @brief تنفيذ طلب HTTP
     */
    NetworkResult performHttpRequest(const RequestData& request);

    /**
     * @brief تحليل URL
     */
    static bool parseUrl(const std::string& url, std::string& host, std::string& path,
                  int& port, bool& isHttps);

    /**
     * @brief thread pool worker function
     */
    void workerThreadFunction();

    /**
     * @brief add task to thread pool
     */
    void addAsyncTask(std::function<void()> task);

public:
    ArabicNetwork();
    ~ArabicNetwork();

    // منع النسخ والتعيين
    ArabicNetwork(const ArabicNetwork&) = delete;
    ArabicNetwork& operator=(const ArabicNetwork&) = delete;

    /**
     * @brief تهيئة المكتبة
     */
    bool initialize();

    /**
     * @brief إغلاق المكتبة
     */
    void shutdown();

    /**
     * @brief طلب HTTP متزامن
     */
    NetworkResult httpRequest(const RequestData& request);

    /**
     * @brief طلب HTTP غير متزامن
     */
    void httpRequestAsync(const RequestData& request,
                         std::function<void(NetworkResult)> callback);

    /**
     * @brief GET request
     */
    NetworkResult httpGet(const std::string& url,
                         const std::unordered_map<std::string, std::string>& headers = {});

    /**
     * @brief POST request
     */
    NetworkResult httpPost(const std::string& url, const std::string& data,
                          const std::unordered_map<std::string, std::string>& headers = {},
                          const std::string& contentType = "application/json");

    /**
     * @brief PUT request
     */
    NetworkResult httpPut(const std::string& url, const std::string& data,
                         const std::unordered_map<std::string, std::string>& headers = {},
                         const std::string& contentType = "application/json");

    /**
     * @brief DELETE request
     */
    NetworkResult httpDelete(const std::string& url,
                            const std::unordered_map<std::string, std::string>& headers = {});

    /**
     * @brief تحميل ملف
     */
    NetworkResult downloadFile(const std::string& url, const std::string& localPath,
                              std::function<void(size_t, size_t)> progressCallback = nullptr);

    /**
     * @brief رفع ملف
     */
    NetworkResult uploadFile(const std::string& url, const std::string& filePath,
                            const std::string& fieldName = "file",
                            const std::unordered_map<std::string, std::string>& headers = {});

    /**
     * @brief الحصول على إحصائيات الشبكة
     */
    const NetworkStats& getNetworkStats() const { return stats; }

    /**
     * @brief إعادة تعيين الإحصائيات
     */
    void resetStats() { stats = NetworkStats(); }

    /**
     * @brief اختبار الاتصال بالإنترنت
     */
    bool testConnectivity(const std::string& testUrl = "https://www.google.com");

    /**
     * @brief الحصول على معلومات DNS
     */
    std::vector<std::string> resolveDNS(const std::string& hostname);

    /**
     * @brief تشخيص مشاكل الشبكة
     */
    std::vector<std::string> diagnoseNetwork();

    /**
     * @brief إنشاء URL مع parameters
     */
    static std::string buildUrl(const std::string& baseUrl,
                               const std::unordered_map<std::string, std::string>& params);

    /**
     * @brief تحليل parameters من URL
     */
    static std::unordered_map<std::string, std::string> parseUrlParams(const std::string& url);

    /**
     * @brief ترميز URL
     */
    static std::string urlEncode(const std::string& str);

    /**
     * @brief فك ترميز URL
     */
    static std::string urlDecode(const std::string& str);

    /**
     * @brief التحقق من صحة URL
     */
    static bool isValidUrl(const std::string& url);
};

/**
 * @brief دعم JSON بسيط
 */
namespace ArabicJSON {

    /**
     * @brief تحليل JSON بسيط
     */
    std::unordered_map<std::string, std::string> parseSimpleJSON(const std::string& json);

    /**
     * @brief إنشاء JSON بسيط
     */
    std::string createSimpleJSON(const std::unordered_map<std::string, std::string>& data);

    /**
     * @brief التحقق من صحة JSON
     */
    bool isValidJSON(const std::string& json);

    /**
     * @brief استخراج قيمة من JSON
     */
    std::string extractJSONValue(const std::string& json, const std::string& key);

}

/**
 * @brief دعم XML بسيط
 */
namespace ArabicXML {

    /**
     * @brief تحليل XML بسيط
     */
    std::unordered_map<std::string, std::string> parseSimpleXML(const std::string& xml);

    /**
     * @brief إنشاء XML بسيط
     */
    std::string createSimpleXML(const std::string& rootTag,
                               const std::unordered_map<std::string, std::string>& data);

    /**
     * @brief التحقق من صحة XML
     */
    bool isValidXML(const std::string& xml);

    /**
     * @brief استخراج قيمة من XML
     */
    std::string extractXMLValue(const std::string& xml, const std::string& tagName);

}

/**
 * @brief WebSocket Client
 */
class ArabicWebSocket {
public:
    /**
     * @brief حالة الاتصال
     */
    enum class ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        RECONNECTING,
        FAILURE
    };

    /**
     * @brief رسالة WebSocket
     */
    struct WSMessage {
        std::string data;
        bool isText;
        std::chrono::system_clock::time_point timestamp;

        WSMessage() : isText(true) {}
        WSMessage(const std::string& d, bool text = true)
            : data(d), isText(text), timestamp(std::chrono::system_clock::now()) {}
    };

private:
    ConnectionState state;
    std::string url;
    std::function<void(WSMessage)> messageCallback;
    std::function<void(ConnectionState)> stateCallback;
    std::function<void(const std::string&)> errorCallback;

    std::thread wsThread;
    std::atomic<bool> running;
    std::mutex wsMutex;

    // WebSocket implementation using WinHTTP/WebSocket API
    HINTERNET hSession;
    HINTERNET hConnection;
    HINTERNET hRequest;

    /**
     * @brief WebSocket thread function
     */
    void wsThreadFunction();

public:
    ArabicWebSocket();
    ~ArabicWebSocket();

    /**
     * @brief الاتصال بـ WebSocket server
     */
    bool connect(const std::string& wsUrl);

    /**
     * @brief قطع الاتصال
     */
    void disconnect();

    /**
     * @brief إرسال رسالة
     */
    bool send(const std::string& message, bool isText = true);

    /**
     * @brief إرسال بيانات ثنائية
     */
    bool sendBinary(const std::vector<uint8_t>& data);

    /**
     * @brief callback للرسائل الواردة
     */
    void onMessage(std::function<void(WSMessage)> callback) {
        messageCallback = callback;
    }

    /**
     * @brief callback لتغيير الحالة
     */
    void onStateChange(std::function<void(ConnectionState)> callback) {
        stateCallback = callback;
    }

    /**
     * @brief callback للأخطاء
     */
    void onError(std::function<void(const std::string&)> callback) {
        errorCallback = callback;
    }

    /**
     * @brief الحصول على الحالة الحالية
     */
    ConnectionState getState() const { return state; }

    /**
     * @brief إعادة الاتصال تلقائياً
     */
    void enableAutoReconnect(bool enable, int maxRetries = 5, int retryDelayMs = 1000);
};

/**
 * @brief TCP Client
 */
class ArabicTCPClient {
public:
    /**
     * @brief حالة الاتصال
     */
    enum class ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        FAILURE
    };

private:
    ConnectionState state;
    SOCKET socketFd;
    std::string host;
    int port;
    std::function<void(const std::string&)> dataCallback;
    std::function<void(ConnectionState)> stateCallback;
    std::thread receiveThread;
    std::atomic<bool> running;

    /**
     * @brief thread لاستقبال البيانات
     */
    void receiveThreadFunction();

public:
    ArabicTCPClient();
    ~ArabicTCPClient();

    /**
     * @brief الاتصال بـ TCP server
     */
    bool connect(const std::string& host, int port);

    /**
     * @brief قطع الاتصال
     */
    void disconnect();

    /**
     * @brief إرسال بيانات
     */
    bool send(const std::string& data);

    /**
     * @brief إرسال بيانات ثنائية
     */
    bool send(const std::vector<uint8_t>& data);

    /**
     * @brief callback للبيانات الواردة
     */
    void onData(std::function<void(const std::string&)> callback) {
        dataCallback = callback;
    }

    /**
     * @brief callback لتغيير الحالة
     */
    void onStateChange(std::function<void(ConnectionState)> callback) {
        stateCallback = callback;
    }

    /**
     * @brief الحصول على الحالة
     */
    ConnectionState getState() const { return state; }
};

/**
 * @brief دوال مساعدة للشبكات
 */
namespace NetworkHelpers {

    /**
     * @brief تهيئة Windows Sockets
     */
    bool initializeWinsock();

    /**
     * @brief تنظيف Windows Sockets
     */
    void cleanupWinsock();

    /**
     * @brief إنشاء socket آمن
     */
    bool createSecureSocket(SOCKET& socket, const std::string& host, int port);

    /**
     * @brief التحقق من شهادة SSL (للمستقبل)
     */
    bool verifySSLCertificate(const std::string& host, const std::string& certificate);

    /**
     * @brief ضغط البيانات (GZIP)
     */
    std::string compressData(const std::string& data);

    /**
     * @brief فك ضغط البيانات
     */
    std::string decompressData(const std::string& compressedData);

    /**
     * @brief تشفير البيانات (للمستقبل)
     */
    std::string encryptData(const std::string& data, const std::string& key);

    /**
     * @brief فك تشفير البيانات
     */
    std::string decryptData(const std::string& encryptedData, const std::string& key);

}

} // namespace ArabicLanguage
