// ArabicWindowsAPI.h - دعم واجهات برمجة التطبيقات للويندوز العربية
// Windows API support for Arabic programming language

#ifndef ARABIC_WINDOWS_API_H
#define ARABIC_WINDOWS_API_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include "ArabicFFI.h"

namespace ArabicCompiler {

// ════════════════════════════════════════════════════════════
// 🪟 دعم User32.dll - واجهة المستخدم والنوافذ
// ════════════════════════════════════════════════════════════

class ArabicUser32 {
private:
    std::unique_ptr<ArabicFFI> ffi;

public:
    ArabicUser32();
    ~ArabicUser32() = default;

    // دوال النوافذ الأساسية
    bool createWindow(const std::string& className, const std::string& windowName,
                     int x, int y, int width, int height);
    bool showWindow(int cmdShow);
    bool updateWindow();
    bool destroyWindow(uintptr_t hWnd);

    // دوال الرسائل
    bool getMessage(void* msg, uintptr_t hWnd, uint32_t msgFilterMin, uint32_t msgFilterMax);
    bool translateMessage(const void* msg);
    bool dispatchMessage(const void* msg);

    // دوال التحكم
    bool createButton(const std::string& text, int x, int y, int width, int height, uintptr_t parentWnd);
    bool createEdit(const std::string& text, int x, int y, int width, int height, uintptr_t parentWnd);
    bool createStatic(const std::string& text, int x, int y, int width, int height, uintptr_t parentWnd);

    // دوال الحوار
    bool messageBox(const std::string& text, const std::string& caption, uint32_t type);
    bool inputBox(std::string& result, const std::string& prompt, const std::string& title);

    // دوال المناطق والإحداثيات
    bool getWindowRect(uintptr_t hWnd, void* rect);
    bool getClientRect(uintptr_t hWnd, void* rect);
    bool screenToClient(uintptr_t hWnd, void* point);
    bool clientToScreen(uintptr_t hWnd, void* point);

    // دوال الرسم الأساسية
    uintptr_t beginPaint(uintptr_t hWnd, void* paintStruct);
    bool endPaint(uintptr_t hWnd, const void* paintStruct);

    bool isLoaded() const;
};

// ════════════════════════════════════════════════════════════
// 🎨 دعم Gdi32.dll - واجهة الرسومات
// ════════════════════════════════════════════════════════════

class ArabicGdi32 {
private:
    std::unique_ptr<ArabicFFI> ffi;

public:
    ArabicGdi32();
    ~ArabicGdi32() = default;

    // دوال السياق الرسومي (Device Context)
    uintptr_t createCompatibleDC(uintptr_t hdc);
    uintptr_t getDC(uintptr_t hWnd);
    int releaseDC(uintptr_t hWnd, uintptr_t hdc);
    bool deleteDC(uintptr_t hdc);

    // دوال الخطوط والنصوص
    uintptr_t createFont(int height, int width, int escapement, int orientation,
                        int weight, bool italic, bool underline, bool strikeOut,
                        uint32_t charSet, uint32_t outputPrecision, uint32_t clipPrecision,
                        uint32_t quality, uint32_t pitchAndFamily, const std::string& faceName);
    uintptr_t selectObject(uintptr_t hdc, uintptr_t hObject);
    bool textOut(uintptr_t hdc, int x, int y, const std::string& text);
    bool drawText(uintptr_t hdc, const std::string& text, void* rect, uint32_t format);

    // دوال الأشكال
    bool rectangle(uintptr_t hdc, int left, int top, int right, int bottom);
    bool ellipse(uintptr_t hdc, int left, int top, int right, int bottom);
    bool lineTo(uintptr_t hdc, int x, int y);
    bool moveToEx(uintptr_t hdc, int x, int y, void* point);

    // دوال الألوان والفرش
    uintptr_t createSolidBrush(uint32_t color);
    uintptr_t createPen(int style, int width, uint32_t color);
    uint32_t setBkColor(uintptr_t hdc, uint32_t color);
    uint32_t setTextColor(uintptr_t hdc, uint32_t color);

    // دوال الصور النقطية
    uintptr_t createCompatibleBitmap(uintptr_t hdc, int width, int height);
    uintptr_t loadBitmap(uintptr_t hInstance, const std::string& name);

    bool isLoaded() const;
};

// ════════════════════════════════════════════════════════════
// 🔐 دعم Advapi32.dll - الأمان والسجلات
// ════════════════════════════════════════════════════════════

class ArabicAdvapi32 {
private:
    std::unique_ptr<ArabicFFI> ffi;

public:
    ArabicAdvapi32();
    ~ArabicAdvapi32() = default;

    // دوال السجلات (Registry)
    uintptr_t regOpenKeyEx(uintptr_t hKey, const std::string& subKey, uint32_t options, uint32_t samDesired);
    int regCloseKey(uintptr_t hKey);
    int regQueryValueEx(uintptr_t hKey, const std::string& valueName, uint32_t* type, uint8_t* data, uint32_t* dataSize);
    int regSetValueEx(uintptr_t hKey, const std::string& valueName, uint32_t type, const uint8_t* data, uint32_t dataSize);
    int regCreateKeyEx(uintptr_t hKey, const std::string& subKey, uint32_t reserved, std::string& className,
                      uint32_t options, uint32_t samDesired, void* securityAttributes, uintptr_t* result, uint32_t* disposition);

    // دوال الأمان الأساسية
    bool openProcessToken(uintptr_t processHandle, uint32_t desiredAccess, uintptr_t* tokenHandle);
    bool getTokenInformation(uintptr_t tokenHandle, int tokenInformationClass, void* tokenInformation, uint32_t tokenInformationLength, uint32_t* returnLength);
    bool lookupPrivilegeValue(const std::string& systemName, const std::string& name, void* luid);
    bool adjustTokenPrivileges(uintptr_t tokenHandle, bool disableAllPrivileges, void* newState, uint32_t bufferLength, void* previousState, uint32_t* returnLength);

    // دوال الخدمات (Services)
    uintptr_t openSCManager(const std::string& machineName, const std::string& databaseName, uint32_t desiredAccess);
    uintptr_t openService(uintptr_t scManager, const std::string& serviceName, uint32_t desiredAccess);
    bool closeServiceHandle(uintptr_t serviceHandle);
    bool startService(uintptr_t serviceHandle, uint32_t numArgs, const char** args);
    bool controlService(uintptr_t serviceHandle, uint32_t control, void* serviceStatus);

    bool isLoaded() const;
};

// ════════════════════════════════════════════════════════════
// 🗂️ دعم Shell32.dll - الواجهة والملفات
// ════════════════════════════════════════════════════════════

class ArabicShell32 {
private:
    std::unique_ptr<ArabicFFI> ffi;

public:
    ArabicShell32();
    ~ArabicShell32() = default;

    // دوال مستعرض الملفات
    bool shellExecute(uintptr_t hwnd, const std::string& operation, const std::string& file,
                     const std::string& parameters, const std::string& directory, int showCmd);
    uintptr_t findExecutable(const std::string& file, const std::string& directory, std::string& result);

    // دوال المجلدات الخاصة
    uintptr_t getSpecialFolderLocation(int folder, uintptr_t* pidl);
    bool getSpecialFolderPath(uintptr_t hwnd, std::string& path, int folder, bool create);

    // دوال مربع الحوار
    bool browseForFolder(uintptr_t hwnd, const std::string& title, uint32_t flags, uintptr_t* pidl, std::string& displayName);
    bool fileOpenDialog(uintptr_t hwnd, const std::string& title, const std::string& initialDir,
                       const std::string& filter, std::string& fileName);
    bool fileSaveDialog(uintptr_t hwnd, const std::string& title, const std::string& initialDir,
                       const std::string& filter, std::string& fileName);

    // دوال الإشعارات
    bool notifyIcon(uintptr_t hwnd, uint32_t message, uint32_t id, uint32_t flags,
                   uintptr_t callbackMessage, const std::string& tip);

    // دوال الروابط
    bool createShortcut(const std::string& targetPath, const std::string& shortcutPath,
                       const std::string& description, const std::string& workingDir,
                       const std::string& arguments, const std::string& iconPath, int iconIndex);

    bool isLoaded() const;
};

// ════════════════════════════════════════════════════════════
// 🔗 دعم Ole32.dll - OLE/COM الأساسي
// ════════════════════════════════════════════════════════════

class ArabicOle32 {
private:
    std::unique_ptr<ArabicFFI> ffi;

public:
    ArabicOle32();
    ~ArabicOle32() = default;

    // دوال تهيئة OLE
    int oleInitialize(void* pvReserved);
    void oleUninitialize();

    // دوال تخصيص الذاكرة
    uintptr_t coTaskMemAlloc(uintptr_t cb);
    void coTaskMemFree(uintptr_t pv);

    // دوال إنشاء الكائنات
    int coCreateInstance(const void* rclsid, uintptr_t pUnkOuter, uint32_t dwClsContext,
                        const void* riid, uintptr_t* ppv);
    int coGetClassObject(const void* rclsid, uint32_t dwClsContext, void* pvReserved,
                        const void* riid, uintptr_t* ppv);

    // دوال التحويل
    int progIDFromCLSID(const void* clsid, uintptr_t* lplpszProgID);
    int clsidFromProgID(const std::string& lpszProgID, void* lpclsid);
    int stringFromCLSID(const void* rclsid, uintptr_t* lplpsz);
    int clsidFromString(const std::string& lpsz, void* pclsid);

    // دوال المكتبة
    int coFreeLibrary(uintptr_t hInst);
    int coLoadLibrary(const std::string& lpszLibName, bool bAutoFree);

    bool isLoaded() const;
};

// ════════════════════════════════════════════════════════════
// 🌐 دعم Ws2_32.dll - شبكات Winsock
// ════════════════════════════════════════════════════════════

class ArabicWs232 {
private:
    std::unique_ptr<ArabicFFI> ffi;

public:
    ArabicWs232();
    ~ArabicWs232() = default;

    // دوال تهيئة Winsock
    int wsaStartup(uint16_t version, void* wsaData);
    int wsaCleanup();

    // دوال إنشاء السوكيت
    uintptr_t socket(int af, int type, int protocol);
    int closesocket(uintptr_t s);

    // دوال الاتصال
    int connect(uintptr_t s, const void* name, int namelen);
    int bind(uintptr_t s, const void* name, int namelen);
    int listen(uintptr_t s, int backlog);
    uintptr_t accept(uintptr_t s, void* addr, int* addrlen);

    // دوال الإرسال والاستقبال
    int send(uintptr_t s, const char* buf, int len, int flags);
    int recv(uintptr_t s, char* buf, int len, int flags);
    int sendto(uintptr_t s, const char* buf, int len, int flags, const void* to, int tolen);
    int recvfrom(uintptr_t s, char* buf, int len, int flags, void* from, int* fromlen);

    // دوال العناوين
    uintptr_t inet_addr(const std::string& cp);
    int inet_ntoa(uint32_t in, std::string& result);
    uintptr_t gethostbyname(const std::string& name);
    uintptr_t getaddrinfo(const std::string& node, const std::string& service, const void* hints, uintptr_t* res);
    void freeaddrinfo(uintptr_t ai);

    // دوال التحكم في السوكيت
    int ioctlsocket(uintptr_t s, long cmd, unsigned long* argp);
    int setsockopt(uintptr_t s, int level, int optname, const char* optval, int optlen);
    int getsockopt(uintptr_t s, int level, int optname, char* optval, int* optlen);

    bool isLoaded() const;
};

// ════════════════════════════════════════════════════════════
// 🔐 دعم Crypt32.dll - التشفير والشهادات
// ════════════════════════════════════════════════════════════

class ArabicCrypt32 {
private:
    std::unique_ptr<ArabicFFI> ffi;

public:
    ArabicCrypt32();
    ~ArabicCrypt32() = default;

    // دوال التشفير الأساسية
    bool cryptAcquireContext(uintptr_t* phProv, const std::string& pszContainer, const std::string& pszProvider, uint32_t dwProvType, uint32_t dwFlags);
    bool cryptReleaseContext(uintptr_t hProv, uint32_t dwFlags);
    bool cryptGenKey(uintptr_t hProv, uint32_t algId, uint32_t dwFlags, uintptr_t* phKey);
    bool cryptDestroyKey(uintptr_t hKey);
    bool cryptEncrypt(uintptr_t hKey, uintptr_t hHash, bool final, uint32_t dwFlags, uint8_t* pbData, uint32_t* pdwDataLen, uint32_t dwBufLen);
    bool cryptDecrypt(uintptr_t hKey, uintptr_t hHash, bool final, uint32_t dwFlags, uint8_t* pbData, uint32_t* pdwDataLen);

    // دوال التجزئة (Hash)
    bool cryptCreateHash(uintptr_t hProv, uint32_t algId, uintptr_t hKey, uint32_t dwFlags, uintptr_t* phHash);
    bool cryptHashData(uintptr_t hHash, const uint8_t* pbData, uint32_t dwDataLen, uint32_t dwFlags);
    bool cryptGetHashParam(uintptr_t hHash, uint32_t dwParam, uint8_t* pbData, uint32_t* pdwDataLen, uint32_t dwFlags);
    bool cryptDestroyHash(uintptr_t hHash);

    // دوال الشهادات
    uintptr_t certOpenStore(const std::string& lpszStoreProvider, uint32_t dwEncodingType, uintptr_t hCryptProv, uint32_t dwFlags, const void* pvPara);
    bool certCloseStore(uintptr_t hCertStore, uint32_t dwFlags);
    uintptr_t certFindCertificateInStore(uintptr_t hCertStore, uint32_t dwCertEncodingType, uint32_t dwFindFlags, uint32_t dwFindType, const void* pvFindPara, uintptr_t pPrevCertContext);
    bool certFreeCertificateContext(uintptr_t pCertContext);

    // دوال التوقيع والتحقق
    bool cryptSignHash(uintptr_t hHash, uint32_t dwKeySpec, const std::string& sDescription, uint32_t dwFlags, uint8_t* pbSignature, uint32_t* pdwSigLen);
    bool cryptVerifySignature(uintptr_t hHash, const uint8_t* pbSignature, uint32_t dwSigLen, uintptr_t hPubKey, const std::string& sDescription, uint32_t dwFlags);

    bool isLoaded() const;
};

// ════════════════════════════════════════════════════════════
// 📦 مدير Windows APIs الرئيسي
// ════════════════════════════════════════════════════════════

class ArabicWindowsAPIManager {
private:
    std::unique_ptr<ArabicUser32> user32;
    std::unique_ptr<ArabicGdi32> gdi32;
    std::unique_ptr<ArabicAdvapi32> advapi32;
    std::unique_ptr<ArabicShell32> shell32;
    std::unique_ptr<ArabicOle32> ole32;
    std::unique_ptr<ArabicWs232> ws232;
    std::unique_ptr<ArabicCrypt32> crypt32;

public:
    ArabicWindowsAPIManager();
    ~ArabicWindowsAPIManager() = default;

    // الوصول للواجهات المختلفة
    ArabicUser32* getUser32() const { return user32.get(); }
    ArabicGdi32* getGdi32() const { return gdi32.get(); }
    ArabicAdvapi32* getAdvapi32() const { return advapi32.get(); }
    ArabicShell32* getShell32() const { return shell32.get(); }
    ArabicOle32* getOle32() const { return ole32.get(); }
    ArabicWs232* getWs232() const { return ws232.get(); }
    ArabicCrypt32* getCrypt32() const { return crypt32.get(); }

    // دوال عامة
    bool initializeAllAPIs();
    bool isAllAPIsLoaded() const;
    std::vector<std::string> getLoadedAPIs() const;
    std::vector<std::string> getFailedAPIs() const;

    // دوال مساعدة للتحويل
    static uint32_t rgb(int red, int green, int blue);
    static uintptr_t makeLong(int low, int high);
    static int lowWord(uintptr_t value);
    static int highWord(uintptr_t value);
};

} // namespace ArabicCompiler

#endif // ARABIC_WINDOWS_API_H