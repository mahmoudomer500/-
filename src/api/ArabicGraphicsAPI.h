// ArabicGraphicsAPI.h - واجهة برمجة التطبيقات الرسومية العربية
// Arabic Graphics API support for OpenGL, DirectX, and WebGL

#ifndef ARABIC_GRAPHICS_API_H
#define ARABIC_GRAPHICS_API_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <functional>

namespace ArabicCompiler {

// ════════════════════════════════════════════════════════════
// 🎨 دعم OpenGL
// ════════════════════════════════════════════════════════════

class ArabicOpenGL {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;

public:
    ArabicOpenGL();
    ~ArabicOpenGL() = default;

    // تهيئة OpenGL
    bool initialize();
    bool createContext(uintptr_t hdc);
    bool makeCurrent(uintptr_t hdc, uintptr_t context);
    void swapBuffers(uintptr_t hdc);

    // دوال الرسم الأساسية
    void clear(uint32_t mask);
    void clearColor(float red, float green, float blue, float alpha);
    void viewport(int x, int y, int width, int height);
    void flush();
    void finish();

    // دوال الإحداثيات والتحويلات
    void matrixMode(uint32_t mode);
    void loadIdentity();
    void pushMatrix();
    void popMatrix();
    void translate(float x, float y, float z);
    void rotate(float angle, float x, float y, float z);
    void scale(float x, float y, float z);
    void ortho(double left, double right, double bottom, double top, double nearVal, double farVal);
    void perspective(double fovy, double aspect, double nearVal, double farVal);

    // دوال الرسم
    void begin(uint32_t mode);
    void end();
    void vertex2f(float x, float y);
    void vertex3f(float x, float y, float z);
    void color3f(float red, float green, float blue);
    void color4f(float red, float green, float blue, float alpha);
    void texCoord2f(float s, float t);

    // دوال النصوص والخطوط
    void rasterPos2f(float x, float y);
    void rasterPos3f(float x, float y, float z);
    void bitmap(int width, int height, float xorig, float yorig, float xmove, float ymove, const uint8_t* bitmap);

    // دوال الإضاءة
    void enable(uint32_t cap);
    void disable(uint32_t cap);
    void light(uint32_t light, uint32_t pname, const float* params);
    void lightModel(uint32_t pname, const float* params);
    void material(uint32_t face, uint32_t pname, const float* params);

    // دوال النصوص
    void genTextures(int n, uint32_t* textures);
    void deleteTextures(int n, const uint32_t* textures);
    void bindTexture(uint32_t target, uint32_t texture);
    void texImage2D(uint32_t target, int level, int internalformat, int width, int height, int border, uint32_t format, uint32_t type, const void* data);
    void texParameter(uint32_t target, uint32_t pname, int param);
    void texEnv(uint32_t target, uint32_t pname, int param);

    // دوال الألوان والمزج
    void blendFunc(uint32_t sfactor, uint32_t dfactor);
    void alphaFunc(uint32_t func, float ref);

    bool isLoaded() const { return initialized; }
    std::string getVersion() const;
};

// ════════════════════════════════════════════════════════════
// 🎮 دعم DirectX
// ════════════════════════════════════════════════════════════

class ArabicDirectX {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;

public:
    ArabicDirectX();
    ~ArabicDirectX() = default;

    // تهيئة DirectX
    bool initialize();
    bool createDevice(uintptr_t hwnd);
    void cleanup();

    // Direct3D دوال أساسية
    bool clear(uint32_t count, const void* rects, uint32_t flags, uint32_t color, float z, uint32_t stencil);
    bool beginScene();
    bool endScene();
    bool present(const void* sourceRect, const void* destRect, uintptr_t destWindowOverride, const void* dirtyRegion);

    // دوال الرسم
    bool drawPrimitive(uint32_t primitiveType, uint32_t startVertex, uint32_t primitiveCount);
    bool drawIndexedPrimitive(uint32_t primitiveType, int baseVertexIndex, uint32_t minIndex, uint32_t numVertices, uint32_t startIndex, uint32_t primitiveCount);
    bool drawPrimitiveUP(uint32_t primitiveType, uint32_t primitiveCount, const void* vertexStreamZeroData, uint32_t vertexStreamZeroStride);

    // دوال الإحداثيات والتحويلات
    bool setTransform(uint32_t state, const void* matrix);
    bool setViewport(const void* viewport);
    bool setProjectionMatrix(const void* matrix);
    bool setViewMatrix(const void* matrix);
    bool setWorldMatrix(const void* matrix);

    // دوال المواد والإضاءة
    bool setMaterial(const void* material);
    bool setLight(uint32_t index, const void* light);
    bool lightEnable(uint32_t index, bool enable);
    bool setRenderState(uint32_t state, uint32_t value);

    // دوال النصوص
    bool createTexture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, uintptr_t* texture, uintptr_t* handle);
    bool setTexture(uint32_t stage, uintptr_t texture);
    bool setTextureStageState(uint32_t stage, uint32_t type, uint32_t value);
    bool setSamplerState(uint32_t sampler, uint32_t type, uint32_t value);

    // دوال الإطارات
    bool createVertexBuffer(uint32_t length, uint32_t usage, uint32_t fvf, uint32_t pool, uintptr_t* buffer, uintptr_t* handle);
    bool setStreamSource(uint32_t streamNumber, uintptr_t buffer, uint32_t offset, uint32_t stride);
    bool setFVF(uint32_t fvf);
    bool setVertexShader(uintptr_t shader);
    bool setPixelShader(uintptr_t shader);

    // DirectInput دوال
    bool createInputDevice(uint32_t deviceType);
    bool getDeviceState(uint32_t size, void* data);
    bool acquireDevice();
    void unacquireDevice();

    bool isLoaded() const { return initialized; }
    std::string getVersion() const;
};

// ════════════════════════════════════════════════════════════
// 🌐 دعم WebGL
// ════════════════════════════════════════════════════════════

class ArabicWebGL {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;

public:
    ArabicWebGL();
    ~ArabicWebGL() = default;

    // تهيئة WebGL
    bool initialize();
    bool createContext(const std::string& canvasId);
    void cleanup();

    // WebGL دوال أساسية
    void clear(uint32_t mask);
    void clearColor(float red, float green, float blue, float alpha);
    void clearDepth(float depth);
    void clearStencil(int s);
    void viewport(int x, int y, int width, int height);
    void flush();
    void finish();

    // دوال البرامج والتظليل
    uintptr_t createShader(uint32_t type);
    void shaderSource(uintptr_t shader, const std::string& source);
    void compileShader(uintptr_t shader);
    uintptr_t createProgram();
    void attachShader(uintptr_t program, uintptr_t shader);
    void linkProgram(uintptr_t program);
    void useProgram(uintptr_t program);
    void deleteShader(uintptr_t shader);
    void deleteProgram(uintptr_t program);

    // دوال المتغيرات الموحدة
    int getUniformLocation(uintptr_t program, const std::string& name);
    void uniform1f(int location, float x);
    void uniform2f(int location, float x, float y);
    void uniform3f(int location, float x, float y, float z);
    void uniform4f(int location, float x, float y, float z, float w);
    void uniform1i(int location, int x);
    void uniformMatrix4fv(int location, bool transpose, const float* value);

    // دوال الخصائص
    int getAttribLocation(uintptr_t program, const std::string& name);
    void vertexAttribPointer(uint32_t index, int size, uint32_t type, bool normalized, int stride, uintptr_t offset);
    void enableVertexAttribArray(uint32_t index);
    void disableVertexAttribArray(uint32_t index);

    // دوال الإطارات
    uintptr_t createBuffer();
    void bindBuffer(uint32_t target, uintptr_t buffer);
    void bufferData(uint32_t target, uintptr_t size, const void* data, uint32_t usage);
    void deleteBuffer(uintptr_t buffer);

    // دوال النصوص
    uintptr_t createTexture();
    void bindTexture(uint32_t target, uintptr_t texture);
    void texImage2D(uint32_t target, int level, int internalformat, int width, int height, int border, uint32_t format, uint32_t type, const void* data);
    void texParameter(uint32_t target, uint32_t pname, int param);
    void generateMipmap(uint32_t target);
    void deleteTexture(uintptr_t texture);

    // دوال الإطارات
    uintptr_t createFramebuffer();
    void bindFramebuffer(uint32_t target, uintptr_t framebuffer);
    void framebufferTexture2D(uint32_t target, uint32_t attachment, uint32_t textarget, uintptr_t texture, int level);
    void deleteFramebuffer(uintptr_t framebuffer);

    // دوال الرسم
    void drawArrays(uint32_t mode, int first, int count);
    void drawElements(uint32_t mode, int count, uint32_t type, uintptr_t offset);

    // دوال المزج والاختبار
    void enable(uint32_t cap);
    void disable(uint32_t cap);
    void blendFunc(uint32_t sfactor, uint32_t dfactor);
    void depthFunc(uint32_t func);
    void stencilFunc(uint32_t func, int ref, uint32_t mask);
    void stencilOp(uint32_t fail, uint32_t zfail, uint32_t zpass);

    bool isLoaded() const { return initialized; }
    std::string getVersion() const;
};

// ════════════════════════════════════════════════════════════
// 📦 مدير واجهات الرسومات الرئيسي
// ════════════════════════════════════════════════════════════

class ArabicGraphicsAPIManager {
private:
    std::unique_ptr<ArabicOpenGL> opengl;
    std::unique_ptr<ArabicDirectX> directx;
    std::unique_ptr<ArabicWebGL> webgl;

public:
    ArabicGraphicsAPIManager();
    ~ArabicGraphicsAPIManager() = default;

    // الوصول للواجهات المختلفة
    ArabicOpenGL* getOpenGL() const { return opengl.get(); }
    ArabicDirectX* getDirectX() const { return directx.get(); }
    ArabicWebGL* getWebGL() const { return webgl.get(); }

    // دوال عامة
    bool initializeAllAPIs();
    bool isAllAPIsLoaded() const;
    std::vector<std::string> getLoadedAPIs() const;
    std::vector<std::string> getFailedAPIs() const;

    // دوال مساعدة للرسومات
    static uint32_t rgbToColor(float r, float g, float b, float a = 1.0f);
    static void colorToRGB(uint32_t color, float& r, float& g, float& b, float& a);

    // دوال مساعدة للمصفوفات
    static void createIdentityMatrix(float* matrix);
    static void createPerspectiveMatrix(float* matrix, float fov, float aspect, float nearVal, float farVal);
    static void createOrthoMatrix(float* matrix, float left, float right, float bottom, float top, float nearVal, float farVal);
    static void multiplyMatrices(float* result, const float* a, const float* b);
};

} // namespace ArabicCompiler

#endif // ARABIC_GRAPHICS_API_H