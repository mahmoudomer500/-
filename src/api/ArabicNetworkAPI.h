// ArabicNetworkAPI.h - واجهة برمجة التطبيقات الشبكية العربية
// Arabic Network API support for HTTP/HTTPS and WebSocket

#ifndef ARABIC_NETWORK_API_H
#define ARABIC_NETWORK_API_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <cstdint>

namespace ArabicCompiler {

// ════════════════════════════════════════════════════════════
// 🌐 دعم HTTP/HTTPS
// ════════════════════════════════════════════════════════════

struct ArabicHTTPResponse {
    int statusCode;
    std::string statusText;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::string error;
    bool success;

    ArabicHTTPResponse() : statusCode(0), success(false) {}
};

struct ArabicHTTPRequest {
    std::string method;
    std::string url;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    int timeout; // milliseconds
    bool followRedirects;
    bool verifySSL;

    ArabicHTTPRequest() : timeout(30000), followRedirects(true), verifySSL(true) {}
};

class ArabicHTTP {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;

public:
    ArabicHTTP();
    ~ArabicHTTP() = default;

    // تهيئة HTTP
    bool initialize();
    void cleanup();

    // طلبات HTTP أساسية
    ArabicHTTPResponse get(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {});
    ArabicHTTPResponse post(const std::string& url, const std::string& body = "", const std::unordered_map<std::string, std::string>& headers = {});
    ArabicHTTPResponse put(const std::string& url, const std::string& body = "", const std::unordered_map<std::string, std::string>& headers = {});
    ArabicHTTPResponse patch(const std::string& url, const std::string& body = "", const std::unordered_map<std::string, std::string>& headers = {});
    ArabicHTTPResponse delete_(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {});
    ArabicHTTPResponse head(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {});
    ArabicHTTPResponse options(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {});

    // طلبات HTTP متقدمة
    ArabicHTTPResponse request(const ArabicHTTPRequest& request);

    // دوال مساعدة
    std::string urlEncode(const std::string& str) const;
    std::string urlDecode(const std::string& str) const;
    std::string base64Encode(const std::string& str) const;
    std::string base64Decode(const std::string& str) const;

    // إعدادات متقدمة
    void setTimeout(int milliseconds);
    void setUserAgent(const std::string& userAgent);
    void setProxy(const std::string& proxyUrl);
    void setCACertificate(const std::string& caPath);
    void enableCookies(bool enable);
    void clearCookies();

    // دوال غير متزامنة (callbacks)
    using HTTPCompleteCallback = std::function<void(const ArabicHTTPResponse&)>;
    bool getAsync(const std::string& url, HTTPCompleteCallback callback, const std::unordered_map<std::string, std::string>& headers = {});
    bool postAsync(const std::string& url, const std::string& body, HTTPCompleteCallback callback, const std::unordered_map<std::string, std::string>& headers = {});

    // معلومات الاتصال
    std::string getLastError() const;
    bool isConnected() const;
    std::string getLocalIP() const;
    std::string getPublicIP() const;

    bool isLoaded() const { return initialized; }
    std::string getVersion() const;
};

// ════════════════════════════════════════════════════════════
// 🔌 دعم WebSocket
// ════════════════════════════════════════════════════════════

enum class ArabicWebSocketState {
    CONNECTING,
    OPEN,
    CLOSING,
    CLOSED
};

enum class ArabicWebSocketMessageType {
    TEXT,
    BINARY,
    PING,
    PONG,
    CLOSE
};

struct ArabicWebSocketMessage {
    ArabicWebSocketMessageType type;
    std::string data;
    std::vector<uint8_t> binaryData;
    bool isBinary;

    ArabicWebSocketMessage() : isBinary(false) {}
};

class ArabicWebSocket {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    ArabicWebSocketState state;
    uintptr_t socketHandle;

public:
    ArabicWebSocket();
    ~ArabicWebSocket();

    // تهيئة WebSocket
    bool initialize();
    void cleanup();

    // اتصال WebSocket
    bool connect(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {});
    bool connectSecure(const std::string& url, const std::string& caPath = "", const std::unordered_map<std::string, std::string>& headers = {});
    void disconnect();
    bool reconnect();

    // إرسال البيانات
    bool sendText(const std::string& message);
    bool sendBinary(const std::vector<uint8_t>& data);
    bool sendPing();
    bool sendPong();

    // استقبال البيانات
    bool receive(ArabicWebSocketMessage& message, int timeout = 1000);
    bool isMessageAvailable();

    // حالة الاتصال
    ArabicWebSocketState getState() const { return state; }
    bool isConnected() const { return state == ArabicWebSocketState::OPEN; }
    bool isConnecting() const { return state == ArabicWebSocketState::CONNECTING; }

    // إعدادات الاتصال
    void setTimeout(int milliseconds);
    void setMaxMessageSize(size_t maxSize);
    void setHeartbeatInterval(int milliseconds);
    void enableCompression(bool enable);

    // دوال غير متزامنة (callbacks)
    using ConnectCallback = std::function<void(bool success, const std::string& error)>;
    using MessageCallback = std::function<void(const ArabicWebSocketMessage&)>;
    using DisconnectCallback = std::function<void(int code, const std::string& reason)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    void setConnectCallback(ConnectCallback callback);
    void setMessageCallback(MessageCallback callback);
    void setDisconnectCallback(DisconnectCallback callback);
    void setErrorCallback(ErrorCallback callback);

    // معلومات الاتصال
    std::string getURL() const;
    std::string getProtocol() const;
    std::vector<std::string> getExtensions() const;
    std::string getLastError() const;

    bool isLoaded() const { return initialized; }
    std::string getVersion() const;
};

// ════════════════════════════════════════════════════════════
// 🌐 دعم WebSocket Server
// ════════════════════════════════════════════════════════════

class ArabicWebSocketServer {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    uintptr_t serverHandle;
    std::vector<uintptr_t> clientHandles;

public:
    ArabicWebSocketServer();
    ~ArabicWebSocketServer();

    // تهيئة الخادم
    bool initialize();
    void cleanup();

    // بدء الخادم
    bool start(int port, const std::string& host = "localhost");
    bool startSecure(int port, const std::string& certPath, const std::string& keyPath, const std::string& host = "localhost");
    void stop();

    // إدارة العملاء
    std::vector<uintptr_t> getConnectedClients() const;
    bool disconnectClient(uintptr_t clientHandle);
    void disconnectAllClients();

    // إرسال البيانات للعملاء
    bool sendToClient(uintptr_t clientHandle, const std::string& message);
    bool sendToClientBinary(uintptr_t clientHandle, const std::vector<uint8_t>& data);
    bool broadcastText(const std::string& message);
    bool broadcastBinary(const std::vector<uint8_t>& data);

    // استقبال البيانات من العملاء
    bool receiveFromClient(uintptr_t clientHandle, ArabicWebSocketMessage& message, int timeout = 1000);

    // إعدادات الخادم
    void setMaxConnections(int maxConnections);
    void setTimeout(int milliseconds);
    void enableCompression(bool enable);
    void setSubprotocol(const std::string& protocol);

    // دوال غير متزامنة (callbacks)
    using ClientConnectCallback = std::function<void(uintptr_t clientHandle, const std::string& clientIP)>;
    using ClientMessageCallback = std::function<void(uintptr_t clientHandle, const ArabicWebSocketMessage&)>;
    using ClientDisconnectCallback = std::function<void(uintptr_t clientHandle)>;

    void setClientConnectCallback(ClientConnectCallback callback);
    void setClientMessageCallback(ClientMessageCallback callback);
    void setClientDisconnectCallback(ClientDisconnectCallback callback);

    // معلومات الخادم
    bool isRunning() const;
    int getPort() const;
    std::string getHost() const;
    int getConnectedClientCount() const;
    std::string getLastError() const;

    bool isLoaded() const { return initialized; }
    std::string getVersion() const;
};

// ════════════════════════════════════════════════════════════
// 📡 دعم UDP/TCP متقدم
// ════════════════════════════════════════════════════════════

class ArabicAdvancedNetworking {
private:
    std::unique_ptr<ArabicFFI> ffi;
    std::unique_ptr<ArabicWs232> winsock;
    bool initialized;

public:
    ArabicAdvancedNetworking();
    ~ArabicAdvancedNetworking() = default;

    // تهيئة الشبكات المتقدمة
    bool initialize();
    void cleanup();

    // TCP Server
    uintptr_t createTCPServer(int port, const std::string& host = "0.0.0.0");
    uintptr_t acceptTCPConnection(uintptr_t serverSocket);
    bool closeTCPSocket(uintptr_t socket);

    // TCP Client
    uintptr_t createTCPClient();
    bool connectTCP(uintptr_t socket, const std::string& host, int port);
    int sendTCP(uintptr_t socket, const std::vector<uint8_t>& data);
    int receiveTCP(uintptr_t socket, std::vector<uint8_t>& buffer, int maxSize);

    // UDP Socket
    uintptr_t createUDPSocket();
    bool bindUDP(uintptr_t socket, int port, const std::string& host = "0.0.0.0");
    int sendUDP(uintptr_t socket, const std::vector<uint8_t>& data, const std::string& host, int port);
    int receiveUDP(uintptr_t socket, std::vector<uint8_t>& buffer, std::string& senderHost, int& senderPort);

    // دوال مساعدة للشبكات
    std::string resolveHostName(const std::string& hostname);
    std::vector<std::string> getLocalIPAddresses();
    bool isValidIPAddress(const std::string& ip);
    bool isValidPort(int port);

    // إعدادات متقدمة
    bool setSocketOption(uintptr_t socket, int level, int option, const void* value, int valueSize);
    bool getSocketOption(uintptr_t socket, int level, int option, void* value, int& valueSize);
    bool setNonBlocking(uintptr_t socket, bool nonBlocking);
    bool setTimeout(uintptr_t socket, int timeoutMs);

    bool isLoaded() const { return initialized; }
    std::string getLastError() const;
};

// ════════════════════════════════════════════════════════════
// 📦 مدير واجهات الشبكات الرئيسي
// ════════════════════════════════════════════════════════════

class ArabicNetworkAPIManager {
private:
    std::unique_ptr<ArabicHTTP> http;
    std::unique_ptr<ArabicWebSocket> websocket;
    std::unique_ptr<ArabicWebSocketServer> websocketServer;
    std::unique_ptr<ArabicAdvancedNetworking> advancedNetworking;

public:
    ArabicNetworkAPIManager();
    ~ArabicNetworkAPIManager() = default;

    // الوصول للواجهات المختلفة
    ArabicHTTP* getHTTP() const { return http.get(); }
    ArabicWebSocket* getWebSocket() const { return websocket.get(); }
    ArabicWebSocketServer* getWebSocketServer() const { return websocketServer.get(); }
    ArabicAdvancedNetworking* getAdvancedNetworking() const { return advancedNetworking.get(); }

    // دوال عامة
    bool initializeAllAPIs();
    bool isAllAPIsLoaded() const;
    std::vector<std::string> getLoadedAPIs() const;
    std::vector<std::string> getFailedAPIs() const;

    // دوال مساعدة للشبكات
    static std::string formatURL(const std::string& protocol, const std::string& host, int port = 0, const std::string& path = "");
    static bool parseURL(const std::string& url, std::string& protocol, std::string& host, int& port, std::string& path);
    static std::string createWebSocketURL(const std::string& host, int port, const std::string& path = "", bool secure = false);
    static std::string generateBoundary();
};

} // namespace ArabicCompiler

#endif // ARABIC_NETWORK_API_H