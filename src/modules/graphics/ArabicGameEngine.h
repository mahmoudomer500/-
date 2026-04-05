#pragma once

#include "ArabicGraphics.h"
#include "ArabicUI.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../../core/SafeWindows.h"
#include <mmsystem.h>
#include <dsound.h>
#include <Xinput.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "Xinput.lib")

namespace ArabicLanguage {
    class ArabicGraphics;
    class ArabicUI;

// ────────────────────────────────────────────────────────
// نظام الإدخال
// ────────────────────────────────────────────────────────

/**
 * @brief نظام الإدخال - Input System
 */
class ArabicInput {
public:
    /**
     * @brief حالة المفتاح
     */
    enum KeyState {
        KEY_RELEASED,
        KEY_PRESSED,
        KEY_HELD
    };

    /**
     * @brief أزرار الماوس
     */
    enum MouseButton {
        BUTTON_LEFT,
        BUTTON_RIGHT,
        BUTTON_MIDDLE
    };

    /**
     * @brief حالة زر الماوس
     */
    enum MouseButtonState {
        MOUSE_STATE_RELEASED,
        MOUSE_STATE_PRESSED,
        MOUSE_STATE_CLICKED,
        MOUSE_STATE_DOUBLE_CLICKED
    };

    /**
     * @brief إعدادات Gamepad
     */
    struct GamepadState {
        bool connected;
        float leftStickX, leftStickY;
        float rightStickX, rightStickY;
        float leftTrigger, rightTrigger;
        bool buttons[16]; // Standard Xbox controller buttons

        GamepadState() : connected(false), leftStickX(0.0f), leftStickY(0.0f),
                        rightStickX(0.0f), rightStickY(0.0f),
                        leftTrigger(0.0f), rightTrigger(0.0f) {
            memset(buttons, 0, sizeof(buttons));
        }
    };

private:
    // Keyboard state
    KeyState keyStates[256];
    bool keyPressed[256];
    bool keyReleased[256];

    // Mouse state
    int mouseX, mouseY;
    int mouseDeltaX, mouseDeltaY;
    int mouseWheelDelta;
    MouseButtonState mouseButtonStates[3];
    bool mouseButtonPressed[3];
    bool mouseButtonReleased[3];
    bool mouseDoubleClicked[3];

    // Gamepad state
    GamepadState gamepadState;

    // Input buffers
    std::vector<char> textInputBuffer;

    // Window handle for input capture
    HWND windowHandle;

    /**
     * @brief Update key states
     */
    void updateKeyStates();

    /**
     * @brief Update mouse states
     */
    void updateMouseStates();

    /**
     * @brief Update gamepad states
     */
    void updateGamepadStates();

public:
    ArabicInput();
    ~ArabicInput() = default;

    /**
     * @brief تهيئة نظام الإدخال
     */
    bool initialize(HWND hwnd);
    bool initialize() { return windowHandle != NULL; }

    /**
     * @brief تحديث حالة الإدخال
     */
    void update();

    /**
     * @brief معالجة رسائل Windows
     */
    void processWindowsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Keyboard functions
    bool isKeyPressed(int keyCode) const;
    bool isKeyHeld(int keyCode) const;
    bool isKeyReleased(int keyCode) const;
    bool isKeyDown(int keyCode) const;

    // Mouse functions
    int getMouseX() const { return mouseX; }
    int getMouseY() const { return mouseY; }
    glm::vec2 getMousePosition() const { return glm::vec2(static_cast<float>(mouseX), static_cast<float>(mouseY)); }
    int getMouseDeltaX() const { return mouseDeltaX; }
    int getMouseDeltaY() const { return mouseDeltaY; }
    int getMouseWheelDelta() const { return mouseWheelDelta; }

    bool isMouseButtonPressed(MouseButton button) const;
    bool isMouseButtonHeld(MouseButton button) const;
    bool isMouseButtonReleased(MouseButton button) const;
    bool isMouseButtonClicked(MouseButton button) const;
    bool isMouseButtonDoubleClicked(MouseButton button) const;

    // Gamepad functions
    const GamepadState& getGamepadState() const { return gamepadState; }
    bool isGamepadConnected() const { return gamepadState.connected; }
    bool isGamepadButtonPressed(int button) const;
    float getGamepadAxis(int axis) const;

    // Text input
    const std::vector<char>& getTextInputBuffer() const { return textInputBuffer; }
    void clearTextInputBuffer();

    /**
     * @brief Show/hide cursor
     */
    void showCursor(bool show);

    /**
     * @brief Set cursor position
     */
    void setCursorPosition(int x, int y);

    /**
     * @brief Clip cursor to window
     */
    void clipCursorToWindow(bool clip);
};

/**
 * @brief محرك الألعاب العربي - Arabic Game Engine
 *
 * محرك ألعاب شامل مع دعم 2D/3D ونظام إدخال وصوت
 * مصمم لتطوير الألعاب باللغة العربية
 */
class ArabicGameEngine {
public:
    /**
     * @brief حالة المحرك
     */
    enum class EngineState {
        STATE_UNINITIALIZED,
        STATE_INITIALIZING,
        STATE_RUNNING,
        STATE_SUSPENDED,
        STATE_STOPPED,
        STATE_ERROR
    };

    /**
     * @brief إعدادات المحرك
     */
    struct EngineConfig {
        int windowWidth;
        int windowHeight;
        bool fullscreen;
        bool vsync;
        int targetFPS;
        bool enableAudio;
        int audioChannels;
        int audioSampleRate;
        std::string windowTitle;
        std::string gameTitle;

        EngineConfig() : windowWidth(800), windowHeight(600), fullscreen(false),
                        vsync(true), targetFPS(60), enableAudio(true),
                        audioChannels(2), audioSampleRate(44100),
                        windowTitle("Arabic Game Engine"), gameTitle("Arabic Game") {}
    };

    /**
     * @brief إحصائيات المحرك
     */
    struct EngineStats {
        double fps;
        double frameTime;
        int drawCalls;
        int trianglesRendered;
        size_t memoryUsed;
        int activeEntities;
        int activeSounds;
        double deltaTime;

        EngineStats() : fps(0.0), frameTime(0.0), drawCalls(0),
                       trianglesRendered(0), memoryUsed(0),
                       activeEntities(0), activeSounds(0), deltaTime(0.0) {}
    };

private:
    EngineState state;
    EngineConfig config;
    EngineStats stats;

    // Window and graphics
    HWND windowHandle;
    std::shared_ptr<ArabicGraphics> graphics;
    std::shared_ptr<ArabicUI> ui;
    std::shared_ptr<ArabicInput> input;

    // Timing
    std::chrono::steady_clock::time_point lastFrameTime;
    std::chrono::steady_clock::time_point startTime;
    double accumulator;
    double fixedTimeStep;

    // Game loop
    std::atomic<bool> running;
    std::thread gameThread;

    // Callbacks
    std::function<void()> onInit;
    std::function<void(double)> onUpdate;
    std::function<void()> onRender;
    std::function<void()> onShutdown;

    /**
     * @brief Game loop function
     */
    void gameLoop();

    /**
     * @brief Fixed update function
     */
    void fixedUpdate(double deltaTime);

    /**
     * @brief Render function
     */
    void render();

    /**
     * @brief Update statistics
     */
    void updateStats();

public:
    ArabicGameEngine();
    ~ArabicGameEngine();

    // منع النسخ والتعيين
    ArabicGameEngine(const ArabicGameEngine&) = delete;
    ArabicGameEngine& operator=(const ArabicGameEngine&) = delete;

    /**
     * @brief تهيئة المحرك
     */
    bool initialize(HINSTANCE hInstance, const EngineConfig& cfg = EngineConfig());
    bool initialize(HINSTANCE hInstance, const std::string& title, int width, int height) {
        EngineConfig cfg;
        cfg.windowTitle = title;
        cfg.windowWidth = width;
        cfg.windowHeight = height;
        return initialize(hInstance, cfg);
    }

    /**
     * @brief الحصول على الوقت الحالي بالثواني
     */
    double getCurrentTime() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - startTime).count();
    }

    /**
     * @brief بدء تشغيل المحرك
     */
    bool start();

    /**
     * @brief إيقاف المحرك
     */
    void stop();

    /**
     * @brief إغلاق المحرك نهائياً
     */
    void shutdown() { stop(); }

    /**
     * @brief إيقاف مؤقت
     */
    void pause();

    /**
     * @brief استئناف
     */
    void resume();

    /**
     * @brief الحصول على حالة المحرك
     */
    EngineState getState() const { return state; }

    /**
     * @brief الحصول على الإعدادات
     */
    const EngineConfig& getConfig() const { return config; }

    /**
     * @brief الحصول على الإحصائيات
     */
    const EngineStats& getStats() const { return stats; }

    /**
     * @brief تعيين callbacks
     */
    void setOnInit(std::function<void()> callback) { onInit = callback; }
    void setOnUpdate(std::function<void(double)> callback) { onUpdate = callback; }
    void setOnRender(std::function<void()> callback) { onRender = callback; }
    void setOnShutdown(std::function<void()> callback) { onShutdown = callback; }

    /**
     * @brief الحصول على نافذة Windows
     */
    HWND getWindowHandle() const { return windowHandle; }

    /**
     * @brief الحصول على نظام الرسومات
     */
    std::shared_ptr<ArabicGraphics> getGraphics() const { return graphics; }

    /**
     * @brief الحصول على نظام واجهة المستخدم
     */
    std::shared_ptr<ArabicUI> getUI() const { return ui; }

    /**
     * @brief الحصول على نظام الإدخال
     */
    std::shared_ptr<ArabicInput> getInput() const { return input; }

    /**
     * @brief الحصول على الوقت المنقضي بين الإطارات
     */
    double getDeltaTime() const { return stats.deltaTime; }

    /**
     * @brief معالجة الأحداث
     */
    void processEvents();

    /**
     * @brief بدء إطار جديد
     */
    void beginFrame();

    /**
     * @brief إنهاء الإطار الحالي
     */
    void endFrame();

    /**
     * @brief تبديل الصواني (Buffer Swap)
     */
    void swapBuffers();

    /**
     * @brief معالجة رسائل Windows
     */
    void processWindowsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief التحقق من تشغيل المحرك
     */
    bool isRunning() const { return running; }
};

// ────────────────────────────────────────────────────────
// نظام الصوت
// ────────────────────────────────────────────────────────

/**
 * @brief نظام الصوت - Audio System
 */
class ArabicAudio {
public:
    /**
     * @brief تنسيق الصوت
     */
    struct AudioFormat {
        int channels;
        int sampleRate;
        int bitsPerSample;

        AudioFormat() : channels(2), sampleRate(44100), bitsPerSample(16) {}
    };

    /**
     * @brief مصدر صوت
     */
    class AudioSource {
    private:
        LPDIRECTSOUNDBUFFER buffer;
        AudioFormat format;
        bool isPlaying;
        bool isLooping;
        float volume;
        float pan;

    public:
        AudioSource();
        ~AudioSource();

        bool loadFromFile(const std::string& filePath);
        bool loadFromMemory(const void* data, size_t size, const AudioFormat& fmt);

        void play();
        void pause();
        void stop();
        void resume();
        void setVolume(float v);
        void setPan(float panVal);
        void setLooping(bool loop);
        void update();

        bool getIsPlaying() const { return isPlaying; }
        float getVolume() const { return volume; }
        float getPan() const { return pan; }
    };

    /**
     * @brief إعدادات الصوت
     */
    struct AudioConfig {
        int masterVolume;
        int sfxVolume;
        int musicVolume;
        int voiceVolume;
        bool enable3DAudio;

        AudioConfig() : masterVolume(100), sfxVolume(100), musicVolume(100),
                       voiceVolume(100), enable3DAudio(false) {}
    };

private:
    LPDIRECTSOUND directSound;
    LPDIRECTSOUNDBUFFER primaryBuffer;
    AudioConfig config;
    std::vector<std::shared_ptr<AudioSource>> sources;
    bool initialized;

    /**
     * @brief Initialize DirectSound
     */
    bool initDirectSound(HWND hwnd);

    /**
     * @brief Create primary buffer
     */
    bool createPrimaryBuffer();

public:
    ArabicAudio();
    ~ArabicAudio();

    // منع النسخ والتعيين
    ArabicAudio(const ArabicAudio&) = delete;
    ArabicAudio& operator=(const ArabicAudio&) = delete;

    /**
     * @brief تهيئة نظام الصوت
     */
    bool initialize(HWND hwnd);
    bool initialize() { return initialized; }

    /**
     * @brief إنهاء نظام الصوت
     */
    void shutdown();

    /**
     * @brief إنشاء مصدر صوت جديد
     */
    std::shared_ptr<AudioSource> createSource();

    /**
     * @brief تحميل صوت من ملف
     */
    std::shared_ptr<AudioSource> loadAudio(const std::string& filePath);
    std::shared_ptr<AudioSource> loadSound(const std::string& filePath) { return loadAudio(filePath); }

    /**
     * @brief تشغيل صوت
     */
    void playSound(std::shared_ptr<AudioSource> source) { if (source) source->play(); }

    /**
     * @brief تحديث نظام الصوت
     */
    void update();

    /**
     * @brief تعيين إعدادات الصوت
     */
    void setConfig(const AudioConfig& cfg) { config = cfg; }
    const AudioConfig& getConfig() const { return config; }

    /**
     * @brief التحكم في مستوى الصوت الرئيسي
     */
    void setMasterVolume(int volume);
    int getMasterVolume() const { return config.masterVolume; }

    /**
     * @brief التحكم في مستوى تأثيرات الصوت
     */
    void setSFXVolume(int volume);
    int getSFXVolume() const { return config.sfxVolume; }

    /**
     * @brief التحكم في مستوى الموسيقى
     */
    void setMusicVolume(int volume);
    int getMusicVolume() const { return config.musicVolume; }

    /**
     * @brief التحكم في مستوى الصوت المنطوق
     */
    void setVoiceVolume(int volume);
    int getVoiceVolume() const { return config.voiceVolume; }

    /**
     * @brief إيقاف جميع الأصوات
     */
    void stopAll();

    /**
     * @brief إيقاف مؤقت لجميع الأصوات
     */
    void pauseAll();

    /**
     * @brief استئناف جميع الأصوات
     */
    void resumeAll();

    /**
     * @brief الحصول على عدد المصادر النشطة
     */
    size_t getActiveSourceCount() const;
};

// ────────────────────────────────────────────────────────
// مكونات الألعاب الأساسية
// ────────────────────────────────────────────────────────

class GameObject;

/**
 * @brief مكون (Component) للكائن
 */
class Component {
protected:
    GameObject* owner;
    bool enabled;

public:
    Component(GameObject* obj = nullptr) : owner(obj), enabled(true) {}
    virtual ~Component() = default;

    void setOwner(GameObject* obj) { owner = obj; }
    GameObject* getOwner() const { return owner; }

    void setEnabled(bool en) { enabled = en; }
    bool isEnabled() const { return enabled; }

    virtual void update(double deltaTime) {}
    virtual void render(ArabicGraphics& graphics) {}
};

/**
 * @brief كائن اللعبة الأساسي - Game Object
 */
class GameObject {
protected:
    std::string name;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    bool active;
    int layer;

public:
    GameObject(const std::string& objName = "GameObject");
    virtual ~GameObject() = default;

    // Transform functions
    void setPosition(const glm::vec3& pos) { position = pos; }
    void setPosition(const glm::vec2& pos) { position = glm::vec3(pos.x, pos.y, position.z); }
    void setRotation(const glm::vec3& rot) { rotation = rot; }
    void setRotation(float rotZ) { rotation.z = rotZ; }
    void setScale(const glm::vec3& scl) { scale = scl; }
    void setScale(const glm::vec2& scl) { scale = glm::vec3(scl.x, scl.y, 1.0f); }

    const glm::vec3& getPosition() const { return position; }
    glm::vec2 getPosition2D() const { return glm::vec2(position.x, position.y); }
    const glm::vec3& getRotation() const { return rotation; }
    const glm::vec3& getScale() const { return scale; }
    glm::vec2 getScale2D() const { return glm::vec2(scale.x, scale.y); }

    void translate(const glm::vec3& delta) { position += delta; }
    void translate(const glm::vec2& delta) { position.x += delta.x; position.y += delta.y; }
    void rotate(const glm::vec3& delta) { rotation += delta; }
    void rotate(float deltaZ) { rotation.z += deltaZ; }
    void scaleBy(const glm::vec3& factor) { scale.x *= factor.x; scale.y *= factor.y; scale.z *= factor.z; }
    void scaleBy(const glm::vec2& factor) { scale.x *= factor.x; scale.y *= factor.y; }

    // State functions
    void setActive(bool act) { active = act; }
    bool isActive() const { return active; }

    void setLayer(int lyr) { layer = lyr; }
    int getLayer() const { return layer; }

    const std::string& getName() const { return name; }
    void setName(const std::string& n) { name = n; }

    // Virtual functions
    virtual void update(double deltaTime);
    virtual void render(ArabicGraphics& graphics);
    virtual void onCollision(GameObject* other) {}

    // Component management
    void addComponent(std::shared_ptr<Component> component) {
        component->setOwner(this);
        m_components.push_back(component);
    }

    template<typename T>
    std::shared_ptr<T> getComponent() const {
        for (auto& component : m_components) {
            auto ptr = std::dynamic_pointer_cast<T>(component);
            if (ptr) return ptr;
        }
        return nullptr;
    }

    // Utility functions
    glm::mat4 getTransformMatrix() const;
    bool checkCollision(const GameObject* other) const;

protected:
    std::vector<std::shared_ptr<Component>> m_components;
};

/**
 * @brief مدير المشاهد - Scene Manager
 */
class SceneManager {
private:
    std::vector<std::shared_ptr<GameObject>> objects;
    std::string currentSceneName;
    bool sceneLoaded;

public:
    SceneManager();
    ~SceneManager() = default;

    /**
     * @brief إضافة كائن إلى المشهد
     */
    void addObject(std::shared_ptr<GameObject> object);

    /**
     * @brief إزالة كائن من المشهد
     */
    void removeObject(const std::string& name);

    /**
     * @brief العثور على كائن بالاسم
     */
    std::shared_ptr<GameObject> findObject(const std::string& name) const;

    /**
     * @brief الحصول على جميع الكائنات
     */
    const std::vector<std::shared_ptr<GameObject>>& getAllObjects() const { return objects; }

    /**
     * @brief تحديث جميع الكائنات
     */
    void updateAll(double deltaTime);

    /**
     * @brief رسم جميع الكائنات
     */
    void renderAll(ArabicGraphics& graphics);

    /**
     * @brief تحميل مشهد
     */
    bool loadScene(const std::string& sceneName);

    /**
     * @brief حفظ مشهد
     */
    bool saveScene(const std::string& sceneName) const;

    /**
     * @brief مسح المشهد الحالي
     */
    void clearScene();

    /**
     * @brief الحصول على اسم المشهد الحالي
     */
    const std::string& getCurrentSceneName() const { return currentSceneName; }

    /**
     * @brief عدد الكائنات في المشهد
     */
    size_t getObjectCount() const { return objects.size(); }
};

// ────────────────────────────────────────────────────────
// دعم 2D الأساسي
// ────────────────────────────────────────────────────────

/**
 * @brief Sprite 2D
 */
class Sprite2D : public GameObject {
private:
    GLuint textureId;
    glm::vec2 size;
    glm::vec4 color;
    glm::vec4 uvRect; // UV coordinates for texture atlas

public:
    Sprite2D(const std::string& spriteName = "Sprite2D");

    void setTexture(GLuint texId) { textureId = texId; }
    void setSize(const glm::vec2& sz) { size = sz; }
    void setColor(const glm::vec4& col) { color = col; }
    void setUVRect(const glm::vec4& uv) { uvRect = uv; }

    GLuint getTexture() const { return textureId; }
    const glm::vec2& getSize() const { return size; }
    const glm::vec4& getColor() const { return color; }

    void render(ArabicGraphics& graphics) override;
};

/**
 * @brief نظام الرسوم المتحركة لـ 2D
 */
class SpriteAnimation : public Component {
private:
    std::vector<GLuint> frames;
    float frameTime;
    float currentTime;
    int currentFrame;
    bool looping;
    bool playing;

public:
    SpriteAnimation(GameObject* obj = nullptr);

    void addFrame(GLuint textureId);
    void setFrameTime(float time) { frameTime = time; }
    void setLooping(bool loop) { looping = loop; }

    void play();
    void pause();
    void stop();
    void reset();

    bool isPlaying() const { return playing; }
    int getCurrentFrame() const { return currentFrame; }

    void update(double deltaTime) override;
};

// ────────────────────────────────────────────────────────
// دعم 3D الأساسي
// ────────────────────────────────────────────────────────

/**
 * @brief نموذج 3D بسيط
 */
class Model3D : public GameObject {
private:
    GLuint vao, vbo, ebo;
    size_t vertexCount, indexCount;
    GLuint textureId;
    glm::vec4 color;

public:
    Model3D(const std::string& modelName = "Model3D");
    ~Model3D();

    bool loadFromFile(const std::string& filePath);
    void createCube(float size = 1.0f);
    void createSphere(float radius = 1.0f, int segments = 16);
    void createPlane(float width = 1.0f, float height = 1.0f);

    void setTexture(GLuint texId) { textureId = texId; }
    void setColor(const glm::vec4& col) { color = col; }

    void render(ArabicGraphics& graphics) override;
};

/**
 * @brief كاميرا 3D
 */
class Camera3D : public GameObject {
private:
    float fov;
    float nearPlane, farPlane;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

public:
    Camera3D(const std::string& cameraName = "Camera3D");

    void setFOV(float fovDegrees) { fov = glm::radians(fovDegrees); updateProjection(); }
    void setClippingPlanes(float nearVal, float farVal) { nearPlane = nearVal; farPlane = farVal; updateProjection(); }

    const glm::mat4& getViewMatrix() const { return viewMatrix; }
    const glm::mat4& getProjectionMatrix() const { return projectionMatrix; }

    void update(double deltaTime) override;
    void lookAt(const glm::vec3& target);

private:
    void updateViewMatrix();
    void updateProjection();
};

// ────────────────────────────────────────────────────────
// نظام فيزياء أساسي
// ────────────────────────────────────────────────────────

/**
 * @brief جسم فيزيائي
 */
class PhysicsBody : public Component {
public:
    enum BodyType {
        STATIC,
        DYNAMIC,
        KINEMATIC
    };

private:
    BodyType type;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float mass;
    float friction;
    float restitution; // Bounciness
    bool gravityEnabled;

public:
    PhysicsBody(GameObject* obj = nullptr);

    void setType(BodyType t) { type = t; }
    void setVelocity(const glm::vec3& vel) { velocity = vel; }
    void setVelocity(const glm::vec2& vel) { velocity = glm::vec3(vel.x, vel.y, velocity.z); }
    void setAcceleration(const glm::vec3& acc) { acceleration = acc; }
    void setMass(float m) { mass = m; }
    void setFriction(float f) { friction = f; }
    void setRestitution(float r) { restitution = r; }
    void setGravityEnabled(bool enabled) { gravityEnabled = enabled; }

    BodyType getType() const { return type; }
    const glm::vec3& getVelocity() const { return velocity; }
    glm::vec2 getVelocity2D() const { return glm::vec2(velocity.x, velocity.y); }
    const glm::vec3& getAcceleration() const { return acceleration; }
    float getMass() const { return mass; }

    void applyForce(const glm::vec3& force);
    void applyImpulse(const glm::vec3& impulse);

    void update(double deltaTime) override;
};

/**
 * @brief معلومات التصادم
 */
struct CollisionInfo {
    bool colliding;
    glm::vec3 normal;
    float depth;
    void* other;

    CollisionInfo() : colliding(false), normal(0.0f), depth(0.0f), other(nullptr) {}
};

/**
 * @brief كاشف التصادم
 */
class Collider : public Component {
public:
    enum Shape {
        BOX,
        SPHERE,
        CAPSULE,
        CIRCLE
    };

private:
    Shape shape;
    glm::vec3 size; // For box: width, height, depth; For sphere: radius
    bool m_isTrigger; // Sensor collider

public:
    Collider(GameObject* obj = nullptr);

    void setShape(Shape s) { shape = s; }
    void setSize(const glm::vec3& sz) { size = sz; }
    void setTrigger(bool trigger) { m_isTrigger = trigger; }

    Shape getShape() const { return shape; }
    const glm::vec3& getSize() const { return size; }
    bool isTrigger() const { return m_isTrigger; }

    virtual CollisionInfo checkCollision(const Collider& other) const;
};

/**
 * @brief كاشف تصادم مستطيل
 */
class BoxCollider : public Collider {
public:
    BoxCollider(const glm::vec2& size, GameObject* obj = nullptr);
    BoxCollider(const glm::vec3& size, GameObject* obj = nullptr);
};

/**
 * @brief كاشف تصادم دائري
 */
class CircleCollider : public Collider {
public:
    CircleCollider(float radius, GameObject* obj = nullptr);
};

} // namespace ArabicLanguage
