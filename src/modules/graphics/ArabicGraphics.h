#pragma once

#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <GL/wglew.h>

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../../core/SafeWindows.h"

// GLM for math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>
#include <atomic>
#include <queue>

// Vulkan Headers (optional)
#ifdef USE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#endif

namespace ArabicLanguage {

/**
 * @brief مكتبة الرسومات العربية المحسّنة - Arabic Graphics Library Enhanced
 *
 * مكتبة رسومات متقدمة مع دعم OpenGL/Vulkan محسّن
 * تركز على الأداء العالي وإدارة الذاكرة الفعالة
 */
class ArabicGraphics {
public:
    /**
     * @brief Vertex structure for rendering
     */
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec4 color;

        Vertex() : position(0.0f), normal(0.0f, 0.0f, 1.0f),
                  texCoord(0.0f), color(1.0f) {}

        Vertex(const glm::vec3& pos, const glm::vec3& norm = glm::vec3(0.0f, 0.0f, 1.0f),
               const glm::vec2& tex = glm::vec2(0.0f), const glm::vec4& col = glm::vec4(1.0f))
            : position(pos), normal(norm), texCoord(tex), color(col) {}
    };

    /**
     * @brief Render command for batching
     */
    struct RenderCommand {
        enum Type {
            TRIANGLE,
            QUAD,
            LINE,
            POINT,
            MESH,
            TEXT,
            SPRITE
        };

        Type type;
        glm::mat4 transform;
        GLuint textureId;
        glm::vec4 color;
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::string text;
        glm::vec2 textPosition;
        float textScale;
        bool textCentered;
        float lineWidth;

        RenderCommand(Type t = TRIANGLE) : type(t), textureId(0), color(1.0f), 
                                          textScale(1.0f), textCentered(false), lineWidth(1.0f) {}
    };

    /**
     * @brief Graphics API types
     */
    enum GraphicsAPI {
        OPENGL,
        VULKAN,
        DIRECTX
    };

    /**
     * @brief Initialization parameters
     */
    struct InitParams {
        GraphicsAPI api;
        int windowWidth;
        int windowHeight;
        bool vsync;
        bool debugMode;
        int msaaSamples;
        std::string shaderPath;

        InitParams() : api(OPENGL), windowWidth(800), windowHeight(600),
                      vsync(true), debugMode(false), msaaSamples(4) {}
    };

    /**
     * @brief Performance statistics
     */
    struct PerformanceStats {
        int fps;
        float frameTime;
        int drawCalls;
        int trianglesRendered;
        size_t memoryUsed;
        size_t textureMemory;
        size_t bufferMemory;

        PerformanceStats() : fps(0), frameTime(0.0f), drawCalls(0),
                           trianglesRendered(0), memoryUsed(0),
                           textureMemory(0), bufferMemory(0) {}
    };

private:
    InitParams initParams;
    PerformanceStats stats;
    std::chrono::steady_clock::time_point lastFrameTime;

    // OpenGL resources
    GLuint defaultVAO;
    GLuint defaultVBO;
    GLuint defaultEBO;
    GLuint defaultShaderProgram;

    // Vulkan resources (optional)
    #ifdef USE_VULKAN
    VkInstance vkInstance;
    VkPhysicalDevice vkPhysicalDevice;
    VkDevice vkDevice;
    VkQueue vkGraphicsQueue;
    VkCommandPool vkCommandPool;
    #endif

    // Resource management
    std::unordered_map<std::string, GLuint> shaderPrograms;
    std::unordered_map<std::string, GLuint> textures;
    std::unordered_map<std::string, GLuint> vertexBuffers;

    // Batching system
    std::vector<RenderCommand> renderQueue;
    std::mutex renderMutex;

    // Threading
    std::atomic<bool> initialized;
    std::atomic<bool> rendering;

    /**
     * @brief Initialize OpenGL context
     */
    bool initOpenGL(HDC hdc);

    /**
     * @brief Initialize Vulkan context
     */
    bool initVulkan();

    /**
     * @brief Load default shaders
     */
    bool loadDefaultShaders();

    /**
     * @brief Create default vertex buffers
     */
    bool createDefaultBuffers();

    /**
     * @brief Process render queue
     */
    void processRenderQueue();

    /**
     * @brief Update performance statistics
     */
    void updateStats();

    // Internal rendering methods
    void renderTriangle(const RenderCommand& cmd);
    void renderQuad(const RenderCommand& cmd);
    void renderLine(const RenderCommand& cmd);
    void renderMesh(const RenderCommand& cmd);
    void renderText(const RenderCommand& cmd);

public:
    ArabicGraphics();
    ~ArabicGraphics();

    // منع النسخ والتعيين
    ArabicGraphics(const ArabicGraphics&) = delete;
    ArabicGraphics& operator=(const ArabicGraphics&) = delete;

    /**
     * @brief Initialize graphics system
     */
    bool initialize(HDC hdc, const InitParams& params = InitParams());

    /**
     * @brief Shutdown graphics system
     */
    void shutdown();

    /**
     * @brief Check if initialized
     */
    bool isInitialized() const { return initialized; }

    /**
     * @brief Begin frame rendering
     */
    void beginFrame();

    /**
     * @brief End frame rendering
     */
    void endFrame();

    /**
     * @brief Clear screen
     */
    void clear(const glm::vec4& color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    /**
     * @brief Set viewport
     */
    void setViewport(int x, int y, int width, int height);

    /**
     * @brief Set projection matrix
     */
    void setProjection(const glm::mat4& projection);

    /**
     * @brief Set view matrix
     */
    void setView(const glm::mat4& view);
    void setViewMatrix(const glm::mat4& view) { setView(view); }

    /**
     * @brief Set model matrix
     */
    void setModel(const glm::mat4& model);

    // ────────────────────────────────────────────────────────
    // Drawing Functions
    // ────────────────────────────────────────────────────────

    /**
     * @brief Draw rectangle
     */
    void drawRectangle(const glm::vec2& position, const glm::vec2& size,
                      const glm::vec4& color = glm::vec4(1.0f), bool filled = true);

    /**
     * @brief Draw triangle
     */
    void drawTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3,
                     const glm::vec4& color = glm::vec4(1.0f));

    /**
     * @brief Draw quad
     */
    void drawQuad(const glm::vec3& center, float width, float height,
                 const glm::vec4& color = glm::vec4(1.0f));

    /**
     * @brief Draw line
     */
    void drawLine(const glm::vec3& start, const glm::vec3& end,
                const glm::vec4& color = glm::vec4(1.0f), float width = 1.0f);

    /**
     * @brief Draw circle
     */
    void drawCircle(const glm::vec3& center, float radius, int segments = 32,
                   const glm::vec4& color = glm::vec4(1.0f));
    void drawCircle(const glm::vec2& center, float radius, const glm::vec4& color = glm::vec4(1.0f), bool filled = true);

    /**
     * @brief Draw text (basic implementation)
     */
    void drawText(const std::string& text, const glm::vec2& position,
                 const glm::vec4& color = glm::vec4(1.0f), float scale = 1.0f, bool centered = false);

    // ────────────────────────────────────────────────────────
    // Batching System
    // ────────────────────────────────────────────────────────

    /**
     * @brief Add render command to batch
     */
    void addToBatch(const RenderCommand& command);

    /**
     * @brief Render all batched commands
     */
    void renderBatch();

    /**
     * @brief Clear render batch
     */
    void clearBatch();

    // ────────────────────────────────────────────────────────
    // Resource Management
    // ────────────────────────────────────────────────────────

    /**
     * @brief Load shader program
     */
    GLuint loadShaderProgram(const std::string& name, const std::string& vertexSource,
                           const std::string& fragmentSource);

    /**
     * @brief Get shader program
     */
    GLuint getShaderProgram(const std::string& name) const;

    /**
     * @brief Load texture
     */
    GLuint loadTexture(const std::string& name, const std::string& filePath);

    /**
     * @brief Get texture
     */
    GLuint getTexture(const std::string& name) const;

    /**
     * @brief Create vertex buffer
     */
    GLuint createVertexBuffer(const std::string& name, const std::vector<Vertex>& vertices);

    /**
     * @brief Get vertex buffer
     */
    GLuint getVertexBuffer(const std::string& name) const;

    // ────────────────────────────────────────────────────────
    // Advanced Features
    // ────────────────────────────────────────────────────────

    /**
     * @brief Enable instancing
     */
    void enableInstancing(GLuint VAO, const std::vector<glm::mat4>& instanceMatrices);

    /**
     * @brief Disable instancing
     */
    void disableInstancing();

    /**
     * @brief Create geometry batch
     */
    class GeometryBatch {
    private:
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        GLuint VAO, VBO, EBO;
        bool initialized;

    public:
        GeometryBatch();
        ~GeometryBatch();

        void addTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3);
        void addQuad(const Vertex& v1, const Vertex& v2, const Vertex& v3, const Vertex& v4);
        void render();
        void clear();
        size_t getVertexCount() const { return vertices.size(); }
    };

    /**
     * @brief Create instance renderer
     */
    class InstanceRenderer {
    private:
        std::vector<glm::mat4> instanceMatrices;
        GLuint instanceVBO;
        bool initialized;

    public:
        InstanceRenderer();
        ~InstanceRenderer();

        void addInstance(const glm::mat4& transform);
        void renderInstanced(GLuint VAO, GLsizei instanceCount);
        void clear();
        size_t getInstanceCount() const { return instanceMatrices.size(); }
    };

    // ────────────────────────────────────────────────────────
    // Utility Functions
    // ────────────────────────────────────────────────────────

    /**
     * @brief Get performance statistics
     */
    const PerformanceStats& getPerformanceStats() const { return stats; }

    /**
     * @brief Get graphics API version
     */
    std::string getAPIVersion() const;

    /**
     * @brief Get supported extensions
     */
    std::vector<std::string> getSupportedExtensions() const;

    /**
     * @brief Check if feature is supported
     */
    bool isFeatureSupported(const std::string& feature) const;

    /**
     * @brief Take screenshot
     */
    bool takeScreenshot(const std::string& filePath);

    /**
     * @brief Set debug callback
     */
    void setDebugCallback(std::function<void(const std::string&)> callback);

    // ────────────────────────────────────────────────────────
    // Vulkan-specific functions (when available)
    // ────────────────────────────────────────────────────────
    #ifdef USE_VULKAN

    /**
     * @brief Initialize Vulkan renderer
     */
    bool initVulkanRenderer();

    /**
     * @brief Create Vulkan buffer
     */
    VkBuffer createVulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags properties, VkDeviceMemory& bufferMemory);

    /**
     * @brief Create Vulkan image
     */
    VkImage createVulkanImage(uint32_t width, uint32_t height, VkFormat format,
                             VkImageTiling tiling, VkImageUsageFlags usage,
                             VkMemoryPropertyFlags properties, VkDeviceMemory& imageMemory);

    /**
     * @brief Vulkan command buffer management
     */
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    #endif
};

// ────────────────────────────────────────────────────────
// Resource Pool System
// ────────────────────────────────────────────────────────

/**
 * @brief Generic resource pool for memory management
 */
template<typename T>
class ResourcePool {
private:
    std::vector<std::unique_ptr<T>> available;
    std::vector<std::unique_ptr<T>> inUse;
    std::function<T*()> factory;

public:
    ResourcePool(std::function<T*()> f) : factory(f) {}

    std::unique_ptr<T> acquire() {
        if (available.empty()) {
            return std::unique_ptr<T>(factory());
        }

        auto resource = std::move(available.back());
        available.pop_back();
        inUse.push_back(std::move(resource));
        return std::move(inUse.back());
    }

    void release(std::unique_ptr<T> resource) {
        // Find and move to available
        auto it = std::find_if(inUse.begin(), inUse.end(),
                              [&](const std::unique_ptr<T>& r) { return r.get() == resource.get(); });

        if (it != inUse.end()) {
            resource.reset();
            available.push_back(std::move(*it));
            inUse.erase(it);
        }
    }

    size_t getAvailableCount() const { return available.size(); }
    size_t getInUseCount() const { return inUse.size(); }
};

// ────────────────────────────────────────────────────────
// Texture Atlas System
// ────────────────────────────────────────────────────────

/**
 * @brief Texture region in atlas
 */
struct TextureRegion {
    float u1, v1, u2, v2; // UV coordinates
    int x, y, width, height; // Pixel coordinates
};

/**
 * @brief Texture atlas for efficient texture management
 */
class TextureAtlas {
private:
    GLuint atlasTexture;
    int atlasWidth, atlasHeight;
    int currentX, currentY, currentRowHeight;
    std::unordered_map<std::string, TextureRegion> regions;
    bool initialized;

    /**
     * @brief Find suitable position for new texture
     */
    bool findPosition(int width, int height, int& x, int& y);

public:
    TextureAtlas(int width = 2048, int height = 2048);
    ~TextureAtlas();

    /**
     * @brief Add texture to atlas
     */
    TextureRegion addTexture(const std::string& name, unsigned char* data,
                           int width, int height, int channels);

    /**
     * @brief Get texture region
     */
    const TextureRegion* getRegion(const std::string& name) const;

    /**
     * @brief Bind atlas texture
     */
    void bind();

    /**
     * @brief Get atlas texture ID
     */
    GLuint getTextureID() const { return atlasTexture; }

    /**
     * @brief Get usage statistics
     */
    float getUsageRatio() const;
};

// ────────────────────────────────────────────────────────
// Shader Management
// ────────────────────────────────────────────────────────

/**
 * @brief Shader program manager
 */
class ShaderManager {
private:
    std::unordered_map<std::string, GLuint> shaders;
    std::string shaderPath;

public:
    ShaderManager(const std::string& path = "shaders/") : shaderPath(path) {}

    /**
     * @brief Load shader from file
     */
    GLuint loadShader(const std::string& name, GLenum type);

    /**
     * @brief Create shader program
     */
    GLuint createProgram(const std::string& name, const std::string& vertexShader,
                        const std::string& fragmentShader);

    /**
     * @brief Get shader program
     */
    GLuint getProgram(const std::string& name) const;

    /**
     * @brief Set uniform value
     */
    void setUniform(GLuint program, const std::string& name, float value);
    void setUniform(GLuint program, const std::string& name, int value);
    void setUniform(GLuint program, const std::string& name, const glm::vec2& value);
    void setUniform(GLuint program, const std::string& name, const glm::vec3& value);
    void setUniform(GLuint program, const std::string& name, const glm::vec4& value);
    void setUniform(GLuint program, const std::string& name, const glm::mat4& value);

private:
    /**
     * @brief Read shader file
     */
    std::string readShaderFile(const std::string& filePath);

    /**
     * @brief Compile shader
     */
    GLuint compileShader(const std::string& source, GLenum type);

    /**
     * @brief Link program
     */
    bool linkProgram(GLuint program);
};

} // namespace ArabicLanguage
