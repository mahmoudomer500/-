#include "ArabicNetwork.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <wincrypt.h>
#include <shlwapi.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shlwapi.lib")

namespace ArabicLanguage {

ArabicNetwork::ArabicNetwork()
    : hSession(nullptr), initialized(false), stopWorkers(false) {
}

ArabicNetwork::~ArabicNetwork() {
    shutdown();
}

bool ArabicNetwork::initialize() {
    if (initialized) {
        return true;
    }

    std::lock_guard<std::mutex> lock(networkMutex);

    if (!initializeSession()) {
        return false;
    }

    // Initialize thread pool
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;

    stopWorkers = false;
    for (unsigned int i = 0; i < numThreads; ++i) {
        workerThreads.emplace_back(&ArabicNetwork::workerThreadFunction, this);
    }

    initialized = true;
    return true;
}

void ArabicNetwork::shutdown() {
    if (!initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(networkMutex);

    // Stop thread pool
    stopWorkers = true;
    queueCondition.notify_all();

    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workerThreads.clear();

    cleanup();
    initialized = false;
}

bool ArabicNetwork::initializeSession() {
    // Initialize WinHTTP session
    hSession = WinHttpOpen(L"ArabicLanguage/1.0",
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME,
                          WINHTTP_NO_PROXY_BYPASS,
                          0);

    if (!hSession) {
        return false;
    }

    // Set timeouts
    DWORD timeout = 30000; // 30 seconds
    WinHttpSetTimeouts(hSession, timeout, timeout, timeout, timeout);

    return true;
}

void ArabicNetwork::cleanup() {
    if (hSession) {
        WinHttpCloseHandle(hSession);
        hSession = nullptr;
    }
}

bool ArabicNetwork::parseUrl(const std::string& url, std::string& host, std::string& path,
                             int& port, bool& isHttps) {
    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(URL_COMPONENTS);

    WCHAR hostBuffer[256];
    WCHAR pathBuffer[1024];

    urlComp.lpszHostName = hostBuffer;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = pathBuffer;
    urlComp.dwUrlPathLength = 1024;

    std::wstring wUrl(url.begin(), url.end());

    if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp)) {
        return false;
    }

    host = std::string(hostBuffer, hostBuffer + urlComp.dwHostNameLength);
    path = std::string(pathBuffer, pathBuffer + urlComp.dwUrlPathLength);
    port = urlComp.nPort;
    isHttps = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);

    // Default ports
    if (port == 0) {
        port = isHttps ? 443 : 80;
    }

    return true;
}

ArabicNetwork::NetworkResult ArabicNetwork::performHttpRequest(const RequestData& request) {
    NetworkResult result;
    auto start_time = std::chrono::steady_clock::now();

    if (!hSession) {
        result.errorMessage = "Network session not initialized";
        stats.failedRequests++;
        return result;
    }

    std::string host, path;
    int port;
    bool isHttps;

    if (!parseUrl(request.url, host, path, port, isHttps)) {
        result.errorMessage = "Invalid URL format";
        stats.failedRequests++;
        return result;
    }

    // Convert to wide strings
    std::wstring wHost(host.begin(), host.end());
    std::wstring wPath(path.begin(), path.end());

    // Connect to host
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), port, 0);
    if (!hConnect) {
        result.errorMessage = "Failed to connect to host";
        stats.failedRequests++;
        return result;
    }

    // Create request
    const WCHAR* wMethod;
    if (request.method == "GET") wMethod = L"GET";
    else if (request.method == "POST") wMethod = L"POST";
    else if (request.method == "PUT") wMethod = L"PUT";
    else if (request.method == "DELETE") wMethod = L"DELETE";
    else wMethod = L"GET";

    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, wMethod, wPath.c_str(),
                                          nullptr, WINHTTP_NO_REFERER,
                                          WINHTTP_DEFAULT_ACCEPT_TYPES, flags);

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        result.errorMessage = "Failed to create HTTP request";
        stats.failedRequests++;
        return result;
    }

    // Set headers
    for (const auto& header : request.headers) {
        std::wstring wHeader = std::wstring(header.first.begin(), header.first.end()) +
                              L": " +
                              std::wstring(header.second.begin(), header.second.end());
        WinHttpAddRequestHeaders(hRequest, wHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
    }

    // Add User-Agent
    std::wstring wUserAgent = L"User-Agent: " + std::wstring(request.userAgent.begin(), request.userAgent.end());
    WinHttpAddRequestHeaders(hRequest, wUserAgent.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW);

    // Send request
    BOOL bSendResult = WinHttpSendRequest(hRequest,
                                        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                        request.body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)request.body.c_str(),
                                        request.body.length(), request.body.length(), 0);

    if (!bSendResult) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        result.errorMessage = "Failed to send HTTP request";
        stats.failedRequests++;
        return result;
    }

    // Receive response
    bSendResult = WinHttpReceiveResponse(hRequest, nullptr);
    if (!bSendResult) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        result.errorMessage = "Failed to receive HTTP response";
        stats.failedRequests++;
        return result;
    }

    // Get status code
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(DWORD);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                       WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
    result.statusCode = statusCode;

    // Read response data
    DWORD bytesAvailable = 0;
    std::string responseBody;

    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        std::vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;

        if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
            responseBody.append(buffer.data(), bytesRead);
        }
    }

    result.responseBody = responseBody;
    result.success = (statusCode >= 200 && statusCode < 300);

    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);

    auto end_time = std::chrono::steady_clock::now();
    result.responseTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

    stats.totalRequests++;
    if (result.success) {
        stats.successfulRequests++;
    } else {
        stats.failedRequests++;
    }
    stats.totalResponseTime += result.responseTime;
    stats.totalBytesSent += request.body.length();
    stats.totalBytesReceived += responseBody.length();
    stats.avgResponseTime = static_cast<double>(stats.totalResponseTime.count()) / stats.totalRequests;

    return result;
}

void ArabicNetwork::workerThreadFunction() {
    while (!stopWorkers) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this]() {
                return stopWorkers || !taskQueue.empty();
            });

            if (stopWorkers && taskQueue.empty()) {
                break;
            }

            if (!taskQueue.empty()) {
                task = std::move(taskQueue.front());
                taskQueue.pop();
            }
        }

        if (task) {
            task();
        }
    }
}

void ArabicNetwork::addAsyncTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(std::move(task));
    }
    queueCondition.notify_one();
}

ArabicNetwork::NetworkResult ArabicNetwork::httpRequest(const RequestData& request) {
    return performHttpRequest(request);
}

void ArabicNetwork::httpRequestAsync(const RequestData& request,
                                    std::function<void(NetworkResult)> callback) {
    addAsyncTask([this, request, callback]() {
        NetworkResult result = performHttpRequest(request);
        if (callback) {
            callback(result);
        }
    });
}

ArabicNetwork::NetworkResult ArabicNetwork::httpGet(const std::string& url,
                                                   const std::unordered_map<std::string, std::string>& headers) {
    RequestData request;
    request.url = url;
    request.method = "GET";
    request.headers = headers;

    return httpRequest(request);
}

ArabicNetwork::NetworkResult ArabicNetwork::httpPost(const std::string& url, const std::string& data,
                                                    const std::unordered_map<std::string, std::string>& headers,
                                                    const std::string& contentType) {
    RequestData request;
    request.url = url;
    request.method = "POST";
    request.body = data;
    request.headers = headers;
    request.headers["Content-Type"] = contentType;

    return httpRequest(request);
}

ArabicNetwork::NetworkResult ArabicNetwork::httpPut(const std::string& url, const std::string& data,
                                                   const std::unordered_map<std::string, std::string>& headers,
                                                   const std::string& contentType) {
    RequestData request;
    request.url = url;
    request.method = "PUT";
    request.body = data;
    request.headers = headers;
    request.headers["Content-Type"] = contentType;

    return httpRequest(request);
}

ArabicNetwork::NetworkResult ArabicNetwork::httpDelete(const std::string& url,
                                                      const std::unordered_map<std::string, std::string>& headers) {
    RequestData request;
    request.url = url;
    request.method = "DELETE";
    request.headers = headers;

    return httpRequest(request);
}

ArabicNetwork::NetworkResult ArabicNetwork::downloadFile(const std::string& url, const std::string& localPath,
                                                        std::function<void(size_t, size_t)> progressCallback) {
    NetworkResult result = httpGet(url);

    if (!result.success) {
        return result;
    }

    std::ofstream file(localPath, std::ios::binary);
    if (!file.is_open()) {
        result.success = false;
        result.errorMessage = "Cannot open local file for writing";
        return result;
    }

    file.write(result.responseBody.c_str(), result.responseBody.length());
    file.close();

    return result;
}

ArabicNetwork::NetworkResult ArabicNetwork::uploadFile(const std::string& url, const std::string& filePath,
                                                      const std::string& fieldName,
                                                      const std::unordered_map<std::string, std::string>& headers) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        NetworkResult result;
        result.success = false;
        result.errorMessage = "Cannot open file for reading";
        return result;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string fileContent;
    fileContent.resize(fileSize);
    file.read(&fileContent[0], fileSize);
    file.close();

    // Create multipart form data
    std::string boundary = "----ArabicLanguageBoundary";
    std::string contentType = "multipart/form-data; boundary=" + boundary;

    std::string body = "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"" + fieldName + "\"; filename=\"" +
            filePath.substr(filePath.find_last_of("/\\") + 1) + "\"\r\n";
    body += "Content-Type: application/octet-stream\r\n\r\n";
    body += fileContent;
    body += "\r\n--" + boundary + "--\r\n";

    auto requestHeaders = headers;
    requestHeaders["Content-Type"] = contentType;

    return httpPost(url, body, requestHeaders);
}

bool ArabicNetwork::testConnectivity(const std::string& testUrl) {
    NetworkResult result = httpGet(testUrl);
    return result.success;
}

std::vector<std::string> ArabicNetwork::resolveDNS(const std::string& hostname) {
    std::vector<std::string> addresses;

    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = nullptr;
    int rc = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);

    if (rc != 0) {
        return addresses;
    }

    for (struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        char ipStr[INET6_ADDRSTRLEN];

        if (ptr->ai_family == AF_INET) {
            struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
            inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ipStr, sizeof(ipStr));
            addresses.push_back(ipStr);
        } else if (ptr->ai_family == AF_INET6) {
            struct sockaddr_in6* sockaddr_ipv6 = (struct sockaddr_in6*)ptr->ai_addr;
            inet_ntop(AF_INET6, &sockaddr_ipv6->sin6_addr, ipStr, sizeof(ipStr));
            addresses.push_back(ipStr);
        }
    }

    freeaddrinfo(result);
    return addresses;
}

std::vector<std::string> ArabicNetwork::diagnoseNetwork() {
    std::vector<std::string> diagnosis;

    diagnosis.push_back("=== تشخيص الشبكة ===");

    if (!initialized) {
        diagnosis.push_back("❌ المكتبة غير مهيأة");
        return diagnosis;
    }

    diagnosis.push_back("✅ المكتبة مهيأة");

    // Test connectivity
    bool connected = testConnectivity();
    diagnosis.push_back(connected ? "✅ الاتصال بالإنترنت متاح" : "❌ لا يوجد اتصال بالإنترنت");

    // DNS resolution test
    auto dnsResults = resolveDNS("google.com");
    diagnosis.push_back("DNS resolution لـ google.com: " + std::to_string(dnsResults.size()) + " عنوان");

    // Statistics
    diagnosis.push_back("📊 إحصائيات:");
    diagnosis.push_back("   - إجمالي الطلبات: " + std::to_string(stats.totalRequests));
    diagnosis.push_back("   - الطلبات الناجحة: " + std::to_string(stats.successfulRequests));
    diagnosis.push_back("   - الطلبات الفاشلة: " + std::to_string(stats.failedRequests));
    diagnosis.push_back("   - متوسط وقت الاستجابة: " + std::to_string(stats.avgResponseTime / 1e6) + " ms");
    diagnosis.push_back("   - إجمالي البيانات المرسلة: " + std::to_string(stats.totalBytesSent) + " bytes");
    diagnosis.push_back("   - إجمالي البيانات المستلمة: " + std::to_string(stats.totalBytesReceived) + " bytes");

    return diagnosis;
}

// ────────────────────────────────────────────────────────
// تطبيق دوال المساعدة
// ────────────────────────────────────────────────────────

std::string ArabicNetwork::buildUrl(const std::string& baseUrl,
                                   const std::unordered_map<std::string, std::string>& params) {
    std::string url = baseUrl;
    if (!params.empty()) {
        url += "?";
        for (auto it = params.begin(); it != params.end(); ++it) {
            if (it != params.begin()) url += "&";
            url += urlEncode(it->first) + "=" + urlEncode(it->second);
        }
    }
    return url;
}

std::unordered_map<std::string, std::string> ArabicNetwork::parseUrlParams(const std::string& url) {
    std::unordered_map<std::string, std::string> params;

    size_t queryPos = url.find('?');
    if (queryPos == std::string::npos) {
        return params;
    }

    std::string query = url.substr(queryPos + 1);
    std::stringstream ss(query);
    std::string pair;

    while (std::getline(ss, pair, '&')) {
        size_t equalPos = pair.find('=');
        if (equalPos != std::string::npos) {
            std::string key = urlDecode(pair.substr(0, equalPos));
            std::string value = urlDecode(pair.substr(equalPos + 1));
            params[key] = value;
        }
    }

    return params;
}

std::string ArabicNetwork::urlEncode(const std::string& str) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            ss << c;
        } else {
            ss << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }

    return ss.str();
}

std::string ArabicNetwork::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%') {
            if (i + 2 < str.length()) {
                std::string hex = str.substr(i + 1, 2);
                char decoded = static_cast<char>(std::stoi(hex, nullptr, 16));
                result += decoded;
                i += 2;
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

bool ArabicNetwork::isValidUrl(const std::string& url) {
    std::string host, path;
    int port;
    bool isHttps;
    return parseUrl(url, host, path, port, isHttps);
}

// ────────────────────────────────────────────────────────
// تطبيق ArabicJSON
// ────────────────────────────────────────────────────────

namespace ArabicJSON {

std::unordered_map<std::string, std::string> parseSimpleJSON(const std::string& json) {
    std::unordered_map<std::string, std::string> result;

    // Simple JSON parser - supports only string values
    size_t pos = 0;
    while (pos < json.length()) {
        // Find key
        size_t keyStart = json.find('"', pos);
        if (keyStart == std::string::npos) break;
        size_t keyEnd = json.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) break;

        std::string key = json.substr(keyStart + 1, keyEnd - keyStart - 1);

        // Find value
        size_t valueStart = json.find('"', keyEnd + 1);
        if (valueStart == std::string::npos) break;
        size_t valueEnd = json.find('"', valueStart + 1);
        if (valueEnd == std::string::npos) break;

        std::string value = json.substr(valueStart + 1, valueEnd - valueStart - 1);

        result[key] = value;
        pos = valueEnd + 1;
    }

    return result;
}

std::string createSimpleJSON(const std::unordered_map<std::string, std::string>& data) {
    std::string json = "{";
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it != data.begin()) json += ",";
        json += "\"" + it->first + "\":\"" + it->second + "\"";
    }
    json += "}";
    return json;
}

bool isValidJSON(const std::string& json) {
    // Basic validation
    if (json.empty()) return false;
    if (json[0] != '{' || json.back() != '}') return false;

    int braceCount = 0;
    bool inString = false;

    for (char c : json) {
        if (c == '"' && (braceCount == 0 || inString)) {
            inString = !inString;
        } else if (!inString) {
            if (c == '{') braceCount++;
            else if (c == '}') braceCount--;
        }
    }

    return braceCount == 0 && !inString;
}

std::string extractJSONValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";

    size_t valueStart = keyPos + searchKey.length();
    size_t valueEnd = json.find('"', valueStart);
    if (valueEnd == std::string::npos) return "";

    return json.substr(valueStart, valueEnd - valueStart);
}

}

// ────────────────────────────────────────────────────────
// تطبيق ArabicXML
// ────────────────────────────────────────────────────────

namespace ArabicXML {

std::unordered_map<std::string, std::string> parseSimpleXML(const std::string& xml) {
    std::unordered_map<std::string, std::string> result;

    // Simple XML parser - supports only simple tags
    size_t pos = 0;
    while (pos < xml.length()) {
        size_t tagStart = xml.find('<', pos);
        if (tagStart == std::string::npos) break;

        size_t tagEnd = xml.find('>', tagStart);
        if (tagEnd == std::string::npos) break;

        std::string tag = xml.substr(tagStart + 1, tagEnd - tagStart - 1);

        // Skip closing tags
        if (!tag.empty() && tag[0] == '/') {
            pos = tagEnd + 1;
            continue;
        }

        // Find closing tag
        std::string closingTag = "</" + tag + ">";
        size_t contentStart = tagEnd + 1;
        size_t contentEnd = xml.find(closingTag, contentStart);
        if (contentEnd == std::string::npos) {
            pos = tagEnd + 1;
            continue;
        }

        std::string content = xml.substr(contentStart, contentEnd - contentStart);
        result[tag] = content;

        pos = contentEnd + closingTag.length();
    }

    return result;
}

std::string createSimpleXML(const std::string& rootTag,
                           const std::unordered_map<std::string, std::string>& data) {
    std::string xml = "<" + rootTag + ">";

    for (const auto& pair : data) {
        xml += "<" + pair.first + ">" + pair.second + "</" + pair.first + ">";
    }

    xml += "</" + rootTag + ">";
    return xml;
}

bool isValidXML(const std::string& xml) {
    // Basic validation
    if (xml.empty()) return false;
    if (xml.find('<') == std::string::npos) return false;

    // Count opening and closing tags (very basic)
    size_t openCount = 0;
    size_t closeCount = 0;

    size_t pos = 0;
    while ((pos = xml.find('<', pos)) != std::string::npos) {
        if (pos + 1 < xml.length() && xml[pos + 1] == '/') {
            closeCount++;
        } else {
            openCount++;
        }
        pos++;
    }

    return openCount == closeCount && openCount > 0;
}

std::string extractXMLValue(const std::string& xml, const std::string& tagName) {
    std::string openTag = "<" + tagName + ">";
    std::string closeTag = "</" + tagName + ">";

    size_t startPos = xml.find(openTag);
    if (startPos == std::string::npos) return "";

    startPos += openTag.length();
    size_t endPos = xml.find(closeTag, startPos);
    if (endPos == std::string::npos) return "";

    return xml.substr(startPos, endPos - startPos);
}

}

// ────────────────────────────────────────────────────────
// تطبيق ArabicWebSocket
// ────────────────────────────────────────────────────────

ArabicWebSocket::ArabicWebSocket()
    : state(ConnectionState::DISCONNECTED), running(false), hSession(nullptr),
      hConnection(nullptr), hRequest(nullptr) {
}

ArabicWebSocket::~ArabicWebSocket() {
    disconnect();
}

bool ArabicWebSocket::connect(const std::string& wsUrl) {
    if (state == ConnectionState::CONNECTED) {
        return true;
    }

    state = ConnectionState::CONNECTING;

    // Parse WebSocket URL
    std::string protocol, host, path;
    int port = 80;

    if (wsUrl.substr(0, 5) == "ws://") {
        protocol = "ws";
        std::string urlPart = wsUrl.substr(5);
        size_t colonPos = urlPart.find(':');
        size_t slashPos = urlPart.find('/');

        if (colonPos != std::string::npos && colonPos < slashPos) {
            host = urlPart.substr(0, colonPos);
            std::string portStr = urlPart.substr(colonPos + 1, slashPos - colonPos - 1);
            port = std::stoi(portStr);
            path = urlPart.substr(slashPos);
        } else {
            host = urlPart.substr(0, slashPos);
            path = urlPart.substr(slashPos);
        }
    } else if (wsUrl.substr(0, 6) == "wss://") {
        protocol = "wss";
        port = 443;
        std::string urlPart = wsUrl.substr(6);
        size_t slashPos = urlPart.find('/');
        host = urlPart.substr(0, slashPos);
        path = urlPart.substr(slashPos);
    } else {
        state = ConnectionState::FAILURE;
        if (errorCallback) errorCallback("Invalid WebSocket URL");
        return false;
    }

    if (path.empty()) path = "/";

    // Note: Full WebSocket implementation requires WebSocket API
    // This is a placeholder for basic WebSocket support
    // In production, you would use WinHTTP WebSocket functions or a dedicated library

    state = ConnectionState::CONNECTED;
    if (stateCallback) stateCallback(state);

    running = true;
    wsThread = std::thread(&ArabicWebSocket::wsThreadFunction, this);

    return true;
}

void ArabicWebSocket::disconnect() {
    if (state == ConnectionState::DISCONNECTED) {
        return;
    }

    running = false;
    if (wsThread.joinable()) {
        wsThread.join();
    }

    state = ConnectionState::DISCONNECTED;
    if (stateCallback) stateCallback(state);
}

bool ArabicWebSocket::send(const std::string& message, bool isText) {
    if (state != ConnectionState::CONNECTED) {
        return false;
    }

    // Placeholder for WebSocket send implementation
    WSMessage msg(message, isText);
    // In real implementation, send via WebSocket connection

    return true;
}

bool ArabicWebSocket::sendBinary(const std::vector<uint8_t>& data) {
    if (state != ConnectionState::CONNECTED) {
        return false;
    }

    // Placeholder for binary WebSocket send
    return true;
}

void ArabicWebSocket::wsThreadFunction() {
    // Placeholder for WebSocket message handling thread
    while (running) {
        // In real implementation, this would handle incoming messages
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ArabicWebSocket::enableAutoReconnect(bool enable, int maxRetries, int retryDelayMs) {
    // Placeholder for auto-reconnect functionality
}

// ────────────────────────────────────────────────────────
// تطبيق ArabicTCPClient
// ────────────────────────────────────────────────────────

ArabicTCPClient::ArabicTCPClient()
    : state(ConnectionState::DISCONNECTED), socketFd(INVALID_SOCKET), running(false) {
    NetworkHelpers::initializeWinsock();
}

ArabicTCPClient::~ArabicTCPClient() {
    disconnect();
    NetworkHelpers::cleanupWinsock();
}

bool ArabicTCPClient::connect(const std::string& host, int port) {
    if (state == ConnectionState::CONNECTED) {
        disconnect();
    }

    state = ConnectionState::CONNECTING;
    this->host = host;
    this->port = port;

    // Create socket
    socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFd == INVALID_SOCKET) {
        state = ConnectionState::FAILURE;
        if (stateCallback) stateCallback(state);
        return false;
    }

    // Resolve hostname
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = nullptr;
    int rc = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);

    if (rc != 0) {
        closesocket(socketFd);
        socketFd = INVALID_SOCKET;
        state = ConnectionState::FAILURE;
        if (stateCallback) stateCallback(state);
        return false;
    }

    // Connect
    rc = ::connect(socketFd, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    if (rc == SOCKET_ERROR) {
        closesocket(socketFd);
        socketFd = INVALID_SOCKET;
        state = ConnectionState::FAILURE;
        if (stateCallback) stateCallback(state);
        return false;
    }

    state = ConnectionState::CONNECTED;
    if (stateCallback) stateCallback(state);

    // Start receive thread
    running = true;
    receiveThread = std::thread(&ArabicTCPClient::receiveThreadFunction, this);

    return true;
}

void ArabicTCPClient::disconnect() {
    if (state == ConnectionState::DISCONNECTED) {
        return;
    }

    running = false;

    if (receiveThread.joinable()) {
        receiveThread.join();
    }

    if (socketFd != INVALID_SOCKET) {
        closesocket(socketFd);
        socketFd = INVALID_SOCKET;
    }

    state = ConnectionState::DISCONNECTED;
    if (stateCallback) stateCallback(state);
}

bool ArabicTCPClient::send(const std::string& data) {
    if (state != ConnectionState::CONNECTED || socketFd == INVALID_SOCKET) {
        return false;
    }

    int sent = ::send(socketFd, data.c_str(), data.length(), 0);
    return sent != SOCKET_ERROR;
}

bool ArabicTCPClient::send(const std::vector<uint8_t>& data) {
    if (state != ConnectionState::CONNECTED || socketFd == INVALID_SOCKET) {
        return false;
    }

    int sent = ::send(socketFd, reinterpret_cast<const char*>(data.data()), data.size(), 0);
    return sent != SOCKET_ERROR;
}

void ArabicTCPClient::receiveThreadFunction() {
    const int bufferSize = 4096;
    char buffer[bufferSize];

    while (running && state == ConnectionState::CONNECTED) {
        int received = recv(socketFd, buffer, bufferSize - 1, 0);

        if (received > 0) {
            buffer[received] = '\0';
            if (dataCallback) {
                dataCallback(std::string(buffer, received));
            }
        } else if (received == 0) {
            // Connection closed
            state = ConnectionState::DISCONNECTED;
            if (stateCallback) stateCallback(state);
            break;
        } else {
            // Error
            state = ConnectionState::FAILURE;
            if (stateCallback) stateCallback(state);
            break;
        }
    }
}

// ────────────────────────────────────────────────────────
// تطبيق NetworkHelpers
// ────────────────────────────────────────────────────────

namespace NetworkHelpers {

bool initializeWinsock() {
    static bool initialized = false;
    if (initialized) return true;

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        return false;
    }

    initialized = true;
    return true;
}

void cleanupWinsock() {
    WSACleanup();
}

bool createSecureSocket(SOCKET& socket, const std::string& host, int port) {
    // Placeholder for SSL/TLS socket creation
    // In production, this would use Schannel or OpenSSL
    return false;
}

bool verifySSLCertificate(const std::string& host, const std::string& certificate) {
    // Placeholder for SSL certificate verification
    return true;
}

std::string compressData(const std::string& data) {
    // Placeholder for data compression
    return data;
}

std::string decompressData(const std::string& compressedData) {
    // Placeholder for data decompression
    return compressedData;
}

std::string encryptData(const std::string& data, const std::string& key) {
    // Placeholder for data encryption
    return data;
}

std::string decryptData(const std::string& encryptedData, const std::string& key) {
    // Placeholder for data decryption
    return encryptedData;
}

}

} // namespace ArabicLanguage
