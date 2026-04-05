#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <GL/wglew.h>

#include "ArabicGraphics.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <chrono>

// stb_image for texture loading
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"

// Definition for dummy glew
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;

namespace ArabicLanguage {

ArabicGraphics::ArabicGraphics()
    : defaultVAO(0), defaultVBO(0), defaultEBO(0), defaultShaderProgram(0),
      initialized(false), rendering(false) {
}

ArabicGraphics::~ArabicGraphics() {
    shutdown();
}

bool ArabicGraphics::initialize(HDC hdc, const InitParams& params) {
    if (initialized) {
        return true;
    }

    initParams = params;

    bool success = false;

    if (params.api == OPENGL) {
        success = initOpenGL(hdc);
    } else if (params.api == VULKAN) {
        #ifdef USE_VULKAN
        success = initVulkan();
        #else
        std::cerr << "Vulkan not supported in this build" << std::endl;
        return false;
        #endif
    }

    if (!success) {
        return false;
    }

    // Load default shaders
    if (!loadDefaultShaders()) {
        std::cerr << "Failed to load default shaders" << std::endl;
        return false;
    }

    // Create default buffers
    if (!createDefaultBuffers()) {
        std::cerr << "Failed to create default buffers" << std::endl;
        return false;
    }

    initialized = true;
    lastFrameTime = std::chrono::steady_clock::now();

    return true;
}

void ArabicGraphics::shutdown() {
    if (!initialized) {
        return;
    }

    // Clean up OpenGL resources
    if (defaultVAO) glDeleteVertexArrays(1, &defaultVAO);
    if (defaultVBO) glDeleteBuffers(1, &defaultVBO);
    if (defaultEBO) glDeleteBuffers(1, &defaultEBO);
    if (defaultShaderProgram) glDeleteProgram(defaultShaderProgram);

    // Clean up shaders
    for (auto& pair : shaderPrograms) {
        glDeleteProgram(pair.second);
    }
    shaderPrograms.clear();

    // Clean up textures
    for (auto& pair : textures) {
        glDeleteTextures(1, &pair.second);
    }
    textures.clear();

    // Clean up vertex buffers
    for (auto& pair : vertexBuffers) {
        glDeleteBuffers(1, &pair.second);
    }
    vertexBuffers.clear();

    // Clean up Vulkan resources
    #ifdef USE_VULKAN
    if (initParams.api == VULKAN) {
        // Vulkan cleanup
    }
    #endif

    initialized = false;
}

bool ArabicGraphics::initOpenGL(HDC hdc) {
    // Set pixel format
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        24, 8, 0,
        PFD_MAIN_PLANE,
        0, 0, 0, 0
    };

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (!pixelFormat) {
        return false;
    }

    if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
        return false;
    }

    HGLRC tempContext = wglCreateContext(hdc);
    if (!tempContext) {
        return false;
    }

    if (!wglMakeCurrent(hdc, tempContext)) {
        wglDeleteContext(tempContext);
        return false;
    }

    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tempContext);
        return false;
    }

    // Check OpenGL version
    if (!GLEW_VERSION_3_3) {
        std::cerr << "OpenGL 3.3 not supported" << std::endl;
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tempContext);
        return false;
    }

    // Create modern OpenGL context
    HGLRC modernContext = NULL;
    if (glewIsSupported("WGL_ARB_create_context")) {
        const int contextAttribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };

        modernContext = wglCreateContextAttribsARB(hdc, 0, contextAttribs);
    }
    if (!modernContext) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tempContext);
        return false;
    }

    wglMakeCurrent(hdc, modernContext);
    wglDeleteContext(tempContext);

    // Enable debug output if requested
    if (initParams.debugMode) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback([](GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar* message, const void* userParam) {
            std::cerr << "OpenGL Debug: " << message << std::endl;
        }, nullptr);
    }

    // Set default OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    if (initParams.msaaSamples > 1) {
        glEnable(GL_MULTISAMPLE);
    }

    // Set viewport
    setViewport(0, 0, initParams.windowWidth, initParams.windowHeight);

    return true;
}

bool ArabicGraphics::initVulkan() {
    #ifdef USE_VULKAN
    // Vulkan initialization would go here
    // This is a placeholder for future Vulkan support
    return false;
    #else
    return false;
    #endif
}

bool ArabicGraphics::loadDefaultShaders() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;
        layout (location = 3) in vec4 aColor;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;
        out vec4 Color;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;
            Color = aColor;

            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoord;
        in vec4 Color;

        uniform sampler2D texture1;
        uniform bool useTexture;

        void main() {
            if (useTexture) {
                FragColor = texture(texture1, TexCoord) * Color;
            } else {
                FragColor = Color;
            }
        }
    )";

    defaultShaderProgram = loadShaderProgram("default", vertexShaderSource, fragmentShaderSource);
    return defaultShaderProgram != 0;
}

bool ArabicGraphics::createDefaultBuffers() {
    // Create VAO
    glGenVertexArrays(1, &defaultVAO);
    glBindVertexArray(defaultVAO);

    // Create VBO
    glGenBuffers(1, &defaultVBO);
    glBindBuffer(GL_ARRAY_BUFFER, defaultVBO);

    // Create EBO
    glGenBuffers(1, &defaultEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, defaultEBO);

    // Set up vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    return true;
}

void ArabicGraphics::beginFrame() {
    if (!initialized) return;

    rendering = true;
    stats.drawCalls = 0;
    stats.trianglesRendered = 0;
}

void ArabicGraphics::endFrame() {
    if (!initialized) return;

    // Render batched commands
    processRenderQueue();

    // Swap buffers (handled by window system)
    rendering = false;

    updateStats();
}

void ArabicGraphics::clear(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ArabicGraphics::setViewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
}

void ArabicGraphics::setProjection(const glm::mat4& projection) {
    glUseProgram(defaultShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(defaultShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void ArabicGraphics::setView(const glm::mat4& view) {
    glUseProgram(defaultShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(defaultShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
}

void ArabicGraphics::setModel(const glm::mat4& model) {
    glUseProgram(defaultShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(defaultShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
}

void ArabicGraphics::drawRectangle(const glm::vec2& position, const glm::vec2& size,
                                  const glm::vec4& color, bool filled) {
    if (!rendering) return;

    if (filled) {
        std::vector<ArabicGraphics::Vertex> vertices = {
            ArabicGraphics::Vertex(glm::vec3(position.x, position.y, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), color),
            ArabicGraphics::Vertex(glm::vec3(position.x + size.x, position.y, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f), color),
            ArabicGraphics::Vertex(glm::vec3(position.x + size.x, position.y + size.y, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f), color),
            ArabicGraphics::Vertex(glm::vec3(position.x, position.y + size.y, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f), color)
        };

        std::vector<GLuint> indices = { 0, 1, 2, 2, 3, 0 };

        RenderCommand cmd(RenderCommand::QUAD);
        cmd.vertices = vertices;
        cmd.indices = indices;
        addToBatch(cmd);
    } else {
        drawLine(glm::vec3(position.x, position.y, 0.0f), glm::vec3(position.x + size.x, position.y, 0.0f), color);
        drawLine(glm::vec3(position.x + size.x, position.y, 0.0f), glm::vec3(position.x + size.x, position.y + size.y, 0.0f), color);
        drawLine(glm::vec3(position.x + size.x, position.y + size.y, 0.0f), glm::vec3(position.x, position.y + size.y, 0.0f), color);
        drawLine(glm::vec3(position.x, position.y + size.y, 0.0f), glm::vec3(position.x, position.y, 0.0f), color);
    }
}

void ArabicGraphics::drawTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3,
                                 const glm::vec4& color) {
    if (!rendering) return;

    std::vector<ArabicGraphics::Vertex> vertices = {
        ArabicGraphics::Vertex(v1, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), color),
        ArabicGraphics::Vertex(v2, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.5f, 1.0f), color),
        ArabicGraphics::Vertex(v3, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f), color)
    };

    RenderCommand cmd(RenderCommand::TRIANGLE);
    cmd.vertices = vertices;

    addToBatch(cmd);
}

void ArabicGraphics::drawQuad(const glm::vec3& center, float width, float height,
                             const glm::vec4& color) {
    if (!rendering) return;

    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;

    glm::vec3 p1 = center; p1.x -= halfWidth; p1.y -= halfHeight;
    glm::vec3 p2 = center; p2.x += halfWidth; p2.y -= halfHeight;
    glm::vec3 p3 = center; p3.x += halfWidth; p3.y += halfHeight;
    glm::vec3 p4 = center; p4.x -= halfWidth; p4.y += halfHeight;

    glm::vec3 normal(0.0f, 0.0f, 1.0f);

    std::vector<Vertex> vertices = {
        Vertex(p1, normal, glm::vec2(0.0f, 0.0f), color),
        Vertex(p2, normal, glm::vec2(1.0f, 0.0f), color),
        Vertex(p3, normal, glm::vec2(1.0f, 1.0f), color),
        Vertex(p4, normal, glm::vec2(0.0f, 1.0f), color)
    };

    std::vector<GLuint> indices = { 0, 1, 2, 2, 3, 0 };

    RenderCommand cmd(RenderCommand::QUAD);
    cmd.vertices = vertices;
    cmd.indices = indices;
    addToBatch(cmd);
}

void ArabicGraphics::drawLine(const glm::vec3& start, const glm::vec3& end,
                             const glm::vec4& color, float width) {
    if (!rendering) return;

    glm::vec3 normal(0.0f, 0.0f, 1.0f);
    std::vector<Vertex> vertices = {
        Vertex(start, normal, glm::vec2(0.0f, 0.0f), color),
        Vertex(end, normal, glm::vec2(1.0f, 1.0f), color)
    };

    RenderCommand cmd(RenderCommand::LINE);
    cmd.vertices = vertices;
    cmd.lineWidth = width;
    addToBatch(cmd);
}

void ArabicGraphics::drawCircle(const glm::vec2& center, float radius, const glm::vec4& color, bool filled) {
    drawCircle(glm::vec3(center.x, center.y, 0.0f), radius, 32, color);
}

void ArabicGraphics::drawCircle(const glm::vec3& center, float radius, int segments,
                              const glm::vec4& color) {
    if (!rendering) return;

    std::vector<Vertex> vertices;
    glm::vec3 normal(0.0f, 0.0f, 1.0f);
    
    // المركز
    vertices.push_back(Vertex(center, normal, glm::vec2(0.5f, 0.5f), color));

    for (int i = 0; i <= segments; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(segments)) * 2.0f * 3.14159f;
        glm::vec3 pos = center;
        pos.x += cos(angle) * radius;
        pos.y += sin(angle) * radius;
        vertices.push_back(Vertex(pos, normal, glm::vec2(0.5f + cos(angle) * 0.5f, 0.5f + sin(angle) * 0.5f), color));
    }

    RenderCommand cmd(RenderCommand::MESH);
    cmd.vertices = vertices;
    // Note: Reusing indices logic if needed or adjusting to TRIANGLE_FAN if supported
    std::vector<GLuint> indices;
    for (int i = 1; i <= segments; ++i) {
        indices.push_back(0); indices.push_back(i); indices.push_back(i + 1);
    }
    cmd.indices = indices;
    
    addToBatch(cmd);
}

void ArabicGraphics::drawText(const std::string& text, const glm::vec2& position,
                             const glm::vec4& color, float scale, bool centered) {
    if (!rendering) return;

    RenderCommand cmd(RenderCommand::TEXT);
    cmd.text = text;
    cmd.textPosition = position;
    cmd.color = color;
    cmd.textScale = scale;
    cmd.textCentered = centered;
    
    addToBatch(cmd);
}

void ArabicGraphics::addToBatch(const RenderCommand& command) {
    std::lock_guard<std::mutex> lock(renderMutex);
    renderQueue.push_back(command);
}

void ArabicGraphics::processRenderQueue() {
    std::lock_guard<std::mutex> lock(renderMutex);

    glUseProgram(defaultShaderProgram);

    for (const auto& cmd : renderQueue) {
        switch (cmd.type) {
            case RenderCommand::TRIANGLE:
                renderTriangle(cmd);
                break;
            case RenderCommand::QUAD:
                renderQuad(cmd);
                break;
            case RenderCommand::LINE:
                renderLine(cmd);
                break;
            case RenderCommand::MESH:
                renderMesh(cmd);
                break;
            case RenderCommand::TEXT:
                renderText(cmd);
                break;
        }

        stats.drawCalls++;
    }

    renderQueue.clear();
}

void ArabicGraphics::renderTriangle(const RenderCommand& cmd) {
    glBindVertexArray(defaultVAO);
    glBindBuffer(GL_ARRAY_BUFFER, defaultVBO);
    glBufferData(GL_ARRAY_BUFFER, cmd.vertices.size() * sizeof(Vertex),
                 cmd.vertices.data(), GL_DYNAMIC_DRAW);

    glUniform1i(glGetUniformLocation(defaultShaderProgram, "useTexture"), 0);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    stats.trianglesRendered++;
}

void ArabicGraphics::renderQuad(const RenderCommand& cmd) {
    glBindVertexArray(defaultVAO);
    glBindBuffer(GL_ARRAY_BUFFER, defaultVBO);
    glBufferData(GL_ARRAY_BUFFER, cmd.vertices.size() * sizeof(Vertex),
                 cmd.vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, defaultEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cmd.indices.size() * sizeof(GLuint),
                 cmd.indices.data(), GL_DYNAMIC_DRAW);

    glUniform1i(glGetUniformLocation(defaultShaderProgram, "useTexture"), 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // Quad is 2 triangles
    stats.trianglesRendered += 2;
}

void ArabicGraphics::renderLine(const RenderCommand& cmd) {
    glLineWidth(cmd.lineWidth);

    glBindVertexArray(defaultVAO);
    glBindBuffer(GL_ARRAY_BUFFER, defaultVBO);
    glBufferData(GL_ARRAY_BUFFER, cmd.vertices.size() * sizeof(Vertex),
                 cmd.vertices.data(), GL_DYNAMIC_DRAW);

    glUniform1i(glGetUniformLocation(defaultShaderProgram, "useTexture"), 0);

    glDrawArrays(GL_LINES, 0, 2);
}

void ArabicGraphics::renderMesh(const RenderCommand& cmd) {
    glBindVertexArray(defaultVAO);
    glBindBuffer(GL_ARRAY_BUFFER, defaultVBO);
    glBufferData(GL_ARRAY_BUFFER, cmd.vertices.size() * sizeof(Vertex),
                 cmd.vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, defaultEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cmd.indices.size() * sizeof(GLuint),
                 cmd.indices.data(), GL_DYNAMIC_DRAW);

    glUniform1i(glGetUniformLocation(defaultShaderProgram, "useTexture"), 0);

    glDrawElements(GL_TRIANGLES, (GLsizei)cmd.indices.size(), GL_UNSIGNED_INT, 0);
    stats.trianglesRendered += (int)cmd.indices.size() / 3;
}

void ArabicGraphics::renderText(const RenderCommand& cmd) {
    // Basic text rendering
    float charWidth = 8.0f * cmd.textScale;
    float charHeight = 16.0f * cmd.textScale;
    
    float startX = cmd.textPosition.x;
    if (cmd.textCentered) {
        startX -= (cmd.text.length() * charWidth) / 2.0f;
    }

    for (size_t i = 0; i < cmd.text.length(); ++i) {
        glm::vec3 pos = glm::vec3(startX + i * charWidth, cmd.textPosition.y, 0.0f);
        
        // Render a simple quad for the character without adding to batch (to avoid deadlock)
        float halfWidth = charWidth / 2.0f;
        float halfHeight = charHeight / 2.0f;

        glm::vec3 p1 = pos; p1.x -= halfWidth; p1.y -= halfHeight;
        glm::vec3 p2 = pos; p2.x += halfWidth; p2.y -= halfHeight;
        glm::vec3 p3 = pos; p3.x += halfWidth; p3.y += halfHeight;
        glm::vec3 p4 = pos; p4.x -= halfWidth; p4.y += halfHeight;

        std::vector<Vertex> vertices = {
            Vertex(p1, glm::vec3(0,0,1), glm::vec2(0,0), cmd.color),
            Vertex(p2, glm::vec3(0,0,1), glm::vec2(1,0), cmd.color),
            Vertex(p3, glm::vec3(0,0,1), glm::vec2(1,1), cmd.color),
            Vertex(p4, glm::vec3(0,0,1), glm::vec2(0,1), cmd.color)
        };
        std::vector<GLuint> indices = { 0, 1, 2, 2, 3, 0 };

        glBindVertexArray(defaultVAO);
        glBindBuffer(GL_ARRAY_BUFFER, defaultVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, defaultEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_DYNAMIC_DRAW);
        glUniform1i(glGetUniformLocation(defaultShaderProgram, "useTexture"), 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
}

void ArabicGraphics::clearBatch() {
    std::lock_guard<std::mutex> lock(renderMutex);
    renderQueue.clear();
}

GLuint ArabicGraphics::loadShaderProgram(const std::string& name, const std::string& vertexSource,
                                       const std::string& fragmentSource) {
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vsrc = vertexSource.c_str();
    glShaderSource(vertexShader, 1, &vsrc, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        return 0;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fsrc = fragmentSource.c_str();
    glShaderSource(fragmentShader, 1, &fsrc, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return 0;
    }

    // Link program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Program linking failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    shaderPrograms[name] = program;
    return program;
}

GLuint ArabicGraphics::getShaderProgram(const std::string& name) const {
    auto it = shaderPrograms.find(name);
    return it != shaderPrograms.end() ? it->second : 0;
}

GLuint ArabicGraphics::loadTexture(const std::string& name, const std::string& filePath) {
    int width, height, channels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);

    if (!data) {
        std::cerr << "Failed to load texture: " << filePath << std::endl;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    textures[name] = texture;
    return texture;
}

GLuint ArabicGraphics::getTexture(const std::string& name) const {
    auto it = textures.find(name);
    return it != textures.end() ? it->second : 0;
}

GLuint ArabicGraphics::createVertexBuffer(const std::string& name, const std::vector<Vertex>& vertices) {
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    vertexBuffers[name] = VBO;
    return VBO;
}

GLuint ArabicGraphics::getVertexBuffer(const std::string& name) const {
    auto it = vertexBuffers.find(name);
    return it != vertexBuffers.end() ? it->second : 0;
}

void ArabicGraphics::enableInstancing(GLuint VAO, const std::vector<glm::mat4>& instanceMatrices) {
    glBindVertexArray(VAO);

    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4),
                 instanceMatrices.data(), GL_STATIC_DRAW);

    // Set up instance matrix attributes
    for (int i = 0; i < 4; ++i) {
        glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                            (void*)(sizeof(glm::vec4) * i));
        glEnableVertexAttribArray(4 + i);
        glVertexAttribDivisor(4 + i, 1);
    }
}

void ArabicGraphics::disableInstancing() {
    for (int i = 0; i < 4; ++i) {
        glDisableVertexAttribArray(4 + i);
        glVertexAttribDivisor(4 + i, 0);
    }
}

void ArabicGraphics::updateStats() {
    auto currentTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - lastFrameTime).count();

    if (duration > 0) {
        stats.fps = static_cast<int>(1000.0f / static_cast<float>(duration));
    }

    stats.frameTime = static_cast<float>(duration) / 1000.0f; // Convert to seconds
    lastFrameTime = currentTime;
}

std::string ArabicGraphics::getAPIVersion() const {
    if (initParams.api == OPENGL) {
        return reinterpret_cast<const char*>(glGetString(GL_VERSION));
    }
    return "Unknown";
}

std::vector<std::string> ArabicGraphics::getSupportedExtensions() const {
    std::vector<std::string> extensions;

    if (initParams.api == OPENGL) {
        GLint numExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

        for (GLint i = 0; i < numExtensions; ++i) {
            extensions.push_back(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)));
        }
    }

    return extensions;
}

bool ArabicGraphics::isFeatureSupported(const std::string& feature) const {
    auto extensions = getSupportedExtensions();
    return std::find(extensions.begin(), extensions.end(), feature) != extensions.end();
}

bool ArabicGraphics::takeScreenshot(const std::string& filePath) {
    // Implementation for taking screenshots
    // This would read pixels from framebuffer and save to file
    return false; // Placeholder
}

void ArabicGraphics::setDebugCallback(std::function<void(const std::string&)> callback) {
    // Set debug callback for OpenGL errors
}

// ────────────────────────────────────────────────────────
// GeometryBatch Implementation
// ────────────────────────────────────────────────────────

ArabicGraphics::GeometryBatch::GeometryBatch() : VAO(0), VBO(0), EBO(0), initialized(false) {
}

ArabicGraphics::GeometryBatch::~GeometryBatch() {
    if (initialized) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
}

void ArabicGraphics::GeometryBatch::addTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3) {
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);

    GLuint baseIndex = static_cast<GLuint>(vertices.size() - 3);
    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
}

void ArabicGraphics::GeometryBatch::addQuad(const Vertex& v1, const Vertex& v2,
                                           const Vertex& v3, const Vertex& v4) {
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    vertices.push_back(v4);

    GLuint baseIndex = static_cast<GLuint>(vertices.size() - 4);
    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
    indices.push_back(baseIndex);
}

void ArabicGraphics::GeometryBatch::render() {
    if (vertices.empty()) return;

    if (!initialized) {
        // Create OpenGL objects
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        initialized = true;
    }

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                 indices.data(), GL_DYNAMIC_DRAW);

    // Set up vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(3);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void ArabicGraphics::GeometryBatch::clear() {
    vertices.clear();
    indices.clear();
}

// ────────────────────────────────────────────────────────
// InstanceRenderer Implementation
// ────────────────────────────────────────────────────────

ArabicGraphics::InstanceRenderer::InstanceRenderer() : instanceVBO(0), initialized(false) {
}

ArabicGraphics::InstanceRenderer::~InstanceRenderer() {
    if (initialized) {
        glDeleteBuffers(1, &instanceVBO);
    }
}

void ArabicGraphics::InstanceRenderer::addInstance(const glm::mat4& transform) {
    instanceMatrices.push_back(transform);
}

void ArabicGraphics::InstanceRenderer::renderInstanced(GLuint VAO, GLsizei instanceCount) {
    if (instanceMatrices.empty()) return;

    if (!initialized) {
        glGenBuffers(1, &instanceVBO);
        initialized = true;
    }

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4),
                 instanceMatrices.data(), GL_DYNAMIC_DRAW);

    // Set up instance matrix attributes
    for (int i = 0; i < 4; ++i) {
        glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                            (void*)(sizeof(glm::vec4) * i));
        glEnableVertexAttribArray(4 + i);
        glVertexAttribDivisor(4 + i, 1);
    }

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, instanceCount);

    // Disable instance attributes
    for (int i = 0; i < 4; ++i) {
        glDisableVertexAttribArray(4 + i);
        glVertexAttribDivisor(4 + i, 0);
    }
}

void ArabicGraphics::InstanceRenderer::clear() {
    instanceMatrices.clear();
}

// ────────────────────────────────────────────────────────
// TextureAtlas Implementation
// ────────────────────────────────────────────────────────

TextureAtlas::TextureAtlas(int width, int height)
    : atlasWidth(width), atlasHeight(height), currentX(0), currentY(0),
      currentRowHeight(0), initialized(false) {

    glGenTextures(1, &atlasTexture);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    initialized = true;
}

TextureAtlas::~TextureAtlas() {
    if (initialized) {
        glDeleteTextures(1, &atlasTexture);
    }
}

TextureRegion TextureAtlas::addTexture(const std::string& name, unsigned char* data,
                                     int width, int height, int channels) {
    TextureRegion region = {0.0f, 0.0f, 0.0f, 0.0f, 0, 0, width, height};

    int x, y;
    if (!findPosition(width, height, x, y)) {
        // Atlas is full
        return region;
    }

    // Copy texture data to atlas
    glBindTexture(GL_TEXTURE_2D, atlasTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height,
                   (channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);

    // Calculate UV coordinates
    region.u1 = static_cast<float>(x) / atlasWidth;
    region.v1 = static_cast<float>(y) / atlasHeight;
    region.u2 = static_cast<float>(x + width) / atlasWidth;
    region.v2 = static_cast<float>(y + height) / atlasHeight;
    region.x = x;
    region.y = y;

    regions[name] = region;
    return region;
}

bool TextureAtlas::findPosition(int width, int height, int& x, int& y) {
    if (currentX + width > atlasWidth) {
        currentX = 0;
        currentY += currentRowHeight;
        currentRowHeight = 0;
    }

    if (currentY + height > atlasHeight) {
        return false; // Atlas is full
    }

    x = currentX;
    y = currentY;

    currentX += width;
    currentRowHeight = (std::max)(currentRowHeight, height);

    return true;
}

const TextureRegion* TextureAtlas::getRegion(const std::string& name) const {
    auto it = regions.find(name);
    return it != regions.end() ? &it->second : nullptr;
}

void TextureAtlas::bind() {
    glBindTexture(GL_TEXTURE_2D, atlasTexture);
}

float TextureAtlas::getUsageRatio() const {
    int usedPixels = 0;
    for (const auto& pair : regions) {
        usedPixels += pair.second.width * pair.second.height;
    }
    return static_cast<float>(usedPixels) / (atlasWidth * atlasHeight);
}

// ────────────────────────────────────────────────────────
// ShaderManager Implementation
// ────────────────────────────────────────────────────────

GLuint ShaderManager::loadShader(const std::string& name, GLenum type) {
    std::string filePath = shaderPath + name;
    std::string source = readShaderFile(filePath);

    if (source.empty()) {
        return 0;
    }

    return compileShader(source, type);
}

GLuint ShaderManager::createProgram(const std::string& name, const std::string& vertexShader,
                                  const std::string& fragmentShader) {
    GLuint vertex = loadShader(vertexShader, GL_VERTEX_SHADER);
    GLuint fragment = loadShader(fragmentShader, GL_FRAGMENT_SHADER);

    if (!vertex || !fragment) {
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    if (linkProgram(program)) {
        shaders[name] = program;
        return program;
    }

    glDeleteProgram(program);
    return 0;
}

GLuint ShaderManager::getProgram(const std::string& name) const {
    auto it = shaders.find(name);
    return it != shaders.end() ? it->second : 0;
}

void ShaderManager::setUniform(GLuint program, const std::string& name, float value) {
    glUniform1f(glGetUniformLocation(program, name.c_str()), value);
}

void ShaderManager::setUniform(GLuint program, const std::string& name, int value) {
    glUniform1i(glGetUniformLocation(program, name.c_str()), value);
}

void ShaderManager::setUniform(GLuint program, const std::string& name, const glm::vec2& value) {
    glUniform2fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value));
}

void ShaderManager::setUniform(GLuint program, const std::string& name, const glm::vec3& value) {
    glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value));
}

void ShaderManager::setUniform(GLuint program, const std::string& name, const glm::vec4& value) {
    glUniform4fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value));
}

void ShaderManager::setUniform(GLuint program, const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

std::string ShaderManager::readShaderFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint ShaderManager::compileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool ShaderManager::linkProgram(GLuint program) {
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Program linking failed: " << infoLog << std::endl;
        return false;
    }

    return true;
}

} // namespace ArabicLanguage
