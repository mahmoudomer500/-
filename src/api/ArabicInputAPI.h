// ArabicInputAPI.h - واجهة برمجة التطبيقات الخاصة بالإدخال العربية
// Arabic Input API - Complete input system for keyboard, mouse, and advanced devices

#ifndef ARABIC_INPUT_API_H
#define ARABIC_INPUT_API_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <cstdint>

namespace ArabicInput {

// ════════════════════════════════════════════════════════════
// 🎮 مدير الإدخال الرئيسي
// ════════════════════════════════════════════════════════════

class ArabicInputManager {
private:
    std::unique_ptr<class ArabicKeyboard> keyboard;
    std::unique_ptr<class ArabicMouse> mouse;
    std::unique_ptr<class ArabicGamepad> gamepad;
    std::unique_ptr<class ArabicTouch> touch;
    std::unique_ptr<class ArabicVoice> voice;
    std::unique_ptr<class ArabicGestures> gestures;
    std::unique_ptr<class ArabicVR> vr;
    std::unique_ptr<class ArabicInputRecorder> recorder;
    std::unique_ptr<class ArabicInputAI> ai;

    bool initialized;

public:
    ArabicInputManager();
    ~ArabicInputManager() = default;

    // تهيئة جميع أنظمة الإدخال
    bool initializeAllSystems();

    // الوصول للأنظمة المختلفة
    class ArabicKeyboard* getKeyboard() const { return keyboard.get(); }
    class ArabicMouse* getMouse() const { return mouse.get(); }
    class ArabicGamepad* getGamepad() const { return gamepad.get(); }
    class ArabicTouch* getTouch() const { return touch.get(); }
    class ArabicVoice* getVoice() const { return voice.get(); }
    class ArabicGestures* getGestures() const { return gestures.get(); }
    class ArabicVR* getVR() const { return vr.get(); }
    class ArabicInputRecorder* getRecorder() const { return recorder.get(); }
    class ArabicInputAI* getAI() const { return ai.get(); }

    // معلومات الحالة
    bool isInitialized() const { return initialized; }
    std::vector<std::string> getLoadedSystems() const;
    std::vector<std::string> getFailedSystems() const;
};

// ════════════════════════════════════════════════════════════
// ⌨️ نظام الكيبورد المتقدم
// ════════════════════════════════════════════════════════════

enum class ArabicKeyCode {
    // أحرف إنجليزية
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72, I = 73, J = 74, K = 75, L = 76,
    M = 77, N = 78, O = 79, P = 80, Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
    Y = 89, Z = 90,

    // أرقام
    NUM_0 = 48, NUM_1 = 49, NUM_2 = 50, NUM_3 = 51, NUM_4 = 52, NUM_5 = 53, NUM_6 = 54, NUM_7 = 55,
    NUM_8 = 56, NUM_9 = 57,

    // مفاتيح وظيفية
    F1 = 112, F2 = 113, F3 = 114, F4 = 115, F5 = 116, F6 = 117, F7 = 118, F8 = 119, F9 = 120,
    F10 = 121, F11 = 122, F12 = 123,

    // مفاتيح تحكم
    SPACE = 32, ENTER = 13, ESCAPE = 27, BACKSPACE = 8, TAB = 9,
    SHIFT = 16, CTRL = 17, ALT = 18, CAPS_LOCK = 20,

    // أسهم
    ARROW_UP = 38, ARROW_DOWN = 40, ARROW_LEFT = 37, ARROW_RIGHT = 39
};

class ArabicKeyboard {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    std::unordered_map<ArabicKeyCode, bool> keyStates;
    std::string currentLayout;

public:
    ArabicKeyboard();
    ~ArabicKeyboard() = default;

    // تهيئة الكيبورد
    bool initialize();

    // فحص حالة المفاتيح
    bool isKeyPressed(ArabicKeyCode key) const;
    std::unordered_map<ArabicKeyCode, bool> getAllKeyStates() const;

    // قراءة النصوص
    std::string readText(int maxLength = 1024);
    bool isKeySequencePressed(const std::vector<ArabicKeyCode>& sequence, int timeoutMs = 1000);

    // تحويل ومعالجة
    std::string keyCodeToString(ArabicKeyCode key) const;
    ArabicKeyCode stringToKeyCode(const std::string& str) const;

    // تخطيطات الكيبورد
    std::string getCurrentLayout() const;
    bool setLayout(const std::string& layout);

    // اختصارات
    bool registerShortcut(const std::string& name, const std::vector<ArabicKeyCode>& keys,
                         std::function<void()> callback);
    bool unregisterShortcut(const std::string& name);

    // إحصائيات
    double getTypingSpeed() const; // كلمات في الدقيقة

    bool isLoaded() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 🖱️ نظام الماوس المتقدم
// ════════════════════════════════════════════════════════════

enum class ArabicMouseButton {
    LEFT = 0,
    MIDDLE = 1,
    RIGHT = 2,
    X1 = 3,
    X2 = 4
};

enum class ArabicCursorShape {
    ARROW = 0,
    HAND = 1,
    TEXT = 2,
    WAIT = 3,
    CROSS = 4,
    MOVE = 5,
    RESIZE = 6
};

struct ArabicMousePosition {
    int x, y;
    ArabicMousePosition() : x(0), y(0) {}
    ArabicMousePosition(int x, int y) : x(x), y(y) {}
};

struct ArabicMouseVelocity {
    double x, y;
    ArabicMouseVelocity() : x(0), y(0) {}
    ArabicMouseVelocity(double x, double y) : x(x), y(y) {}
};

class ArabicMouse {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    ArabicMousePosition currentPosition;
    ArabicMouseVelocity velocity;

public:
    ArabicMouse();
    ~ArabicMouse() = default;

    // تهيئة الماوس
    bool initialize();

    // موقع الماوس
    ArabicMousePosition getPosition() const;
    int getX() const { return currentPosition.x; }
    int getY() const { return currentPosition.y; }
    bool setPosition(int x, int y);

    // أزرار الماوس
    bool isButtonPressed(ArabicMouseButton button) const;
    std::unordered_map<ArabicMouseButton, bool> getAllButtonStates() const;

    // حركة الماوس
    ArabicMouseVelocity getVelocity() const;
    double getWheelDelta() const;

    // مؤشر الماوس
    bool showCursor();
    bool hideCursor();
    bool setCursorShape(ArabicCursorShape shape);

    // سحب وإفلات
    bool startDrag(ArabicMouseButton button, int startX, int startY);
    bool isDragging() const;
    ArabicMousePosition getDragStartPosition() const;
    ArabicMousePosition getDragCurrentPosition() const;
    void endDrag();

    bool isLoaded() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 🎮 نظام ألعاب الإدخال
// ════════════════════════════════════════════════════════════

enum class ArabicGamepadButton {
    A = 0, B = 1, X = 2, Y = 3,
    LEFT_SHOULDER = 4, RIGHT_SHOULDER = 5,
    SELECT = 6, START = 7,
    LEFT_THUMB = 8, RIGHT_THUMB = 9,
    DPAD_UP = 10, DPAD_DOWN = 11, DPAD_LEFT = 12, DPAD_RIGHT = 13
};

enum class ArabicGamepadTrigger {
    LEFT = 0, RIGHT = 1
};

enum class ArabicGamepadStick {
    LEFT = 0, RIGHT = 1
};

struct ArabicGamepadState {
    std::unordered_map<ArabicGamepadButton, bool> buttons;
    std::unordered_map<ArabicGamepadTrigger, float> triggers;
    std::unordered_map<ArabicGamepadStick, std::pair<float, float>> sticks;
};

class ArabicGamepad {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    int connectedDevices;

public:
    ArabicGamepad();
    ~ArabicGamepad() = default;

    // تهيئة نظام الألعاب
    bool initialize();

    // معلومات الأجهزة
    int getConnectedDeviceCount() const;
    bool isDeviceConnected(int deviceIndex) const;
    std::string getDeviceName(int deviceIndex) const;
    std::string getDeviceType(int deviceIndex) const;

    // حالة الأزرار والعصي
    bool isButtonPressed(int deviceIndex, ArabicGamepadButton button) const;
    float getTriggerValue(int deviceIndex, ArabicGamepadTrigger trigger) const;
    std::pair<float, float> getStickPosition(int deviceIndex, ArabicGamepadStick stick) const;
    ArabicGamepadState getDeviceState(int deviceIndex) const;

    // الاهتزاز
    bool setVibration(int deviceIndex, float leftMotor, float rightMotor, int durationMs);

    // معلومات البطارية (إذا كانت متوفرة)
    float getBatteryLevel(int deviceIndex) const;
    bool isBatteryCharging(int deviceIndex) const;

    bool isLoaded() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 👆 نظام اللمس المتقدم
// ════════════════════════════════════════════════════════════

struct ArabicTouchPoint {
    int id;
    int x, y;
    float pressure;
    bool isActive;

    ArabicTouchPoint() : id(0), x(0), y(0), pressure(0.0f), isActive(false) {}
};

enum class ArabicGestureType {
    TAP,
    DOUBLE_TAP,
    LONG_PRESS,
    SWIPE_LEFT, SWIPE_RIGHT, SWIPE_UP, SWIPE_DOWN,
    PINCH_IN, PINCH_OUT,
    ROTATE
};

struct ArabicGestureInfo {
    ArabicGestureType type;
    std::vector<ArabicTouchPoint> points;
    float scale;    // للتصغير/التكبير
    float rotation; // للدوران (بالدرجات)
    float velocity; // سرعة الإيماءة

    ArabicGestureInfo() : scale(1.0f), rotation(0.0f), velocity(0.0f) {}
};

class ArabicTouch {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    std::vector<ArabicTouchPoint> activePoints;

public:
    ArabicTouch();
    ~ArabicTouch() = default;

    // تهيئة نظام اللمس
    bool initialize();

    // دعم اللمس
    bool isTouchSupported() const;
    int getMaxTouchPoints() const;

    // نقاط اللمس
    int getActiveTouchPointCount() const;
    ArabicTouchPoint getTouchPoint(int index) const;
    std::vector<ArabicTouchPoint> getAllTouchPoints() const;

    // إيماءات اللمس
    bool registerGestureCallback(ArabicGestureType gesture,
                                std::function<void(const ArabicGestureInfo&)> callback);
    bool unregisterGestureCallback(ArabicGestureType gesture);

    // تحليل الإيماءات
    ArabicGestureInfo recognizeGesture(const std::vector<ArabicTouchPoint>& points);
    std::vector<ArabicGestureType> getSupportedGestures() const;

    bool isLoaded() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 🎤 نظام الصوت المتقدم
// ════════════════════════════════════════════════════════════

enum class ArabicVoiceCommand {
    START_LISTENING,
    STOP_LISTENING,
    TRAIN_MODEL,
    CLEAR_MODEL
};

struct ArabicVoiceResult {
    std::string recognizedText;
    float confidence;
    bool isFinal;
    std::string language;

    ArabicVoiceResult() : confidence(0.0f), isFinal(false) {}
};

class ArabicVoice {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    bool isListening;

public:
    ArabicVoice();
    ~ArabicVoice() = default;

    // تهيئة نظام الصوت
    bool initialize();

    // التعرف على الصوت
    bool startListening();
    bool stopListening();
    bool isCurrentlyListening() const;

    // الحصول على النتائج
    ArabicVoiceResult getRecognitionResult() const;
    bool hasNewResult() const;

    // البحث عن كلمات مفتاحية
    bool registerKeyword(const std::string& keyword,
                        std::function<void()> callback);
    bool unregisterKeyword(const std::string& keyword);

    // تدريب النموذج
    bool startModelTraining();
    bool isModelTraining() const;
    float getTrainingProgress() const;

    // إعدادات التعرف
    bool setLanguage(const std::string& language);
    bool setContinuousMode(bool continuous);
    bool setSensitivity(float sensitivity);

    bool isLoaded() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 👋 نظام الإيماءات
// ════════════════════════════════════════════════════════════

struct ArabicGesturePoint {
    int x, y;
    int timestamp;

    ArabicGesturePoint() : x(0), y(0), timestamp(0) {}
    ArabicGesturePoint(int x, int y, int timestamp) : x(x), y(y), timestamp(timestamp) {}
};

class ArabicGestures {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;
    std::unordered_map<std::string, std::vector<ArabicGesturePoint>> customGestures;

public:
    ArabicGestures();
    ~ArabicGestures() = default;

    // تهيئة نظام الإيماءات
    bool initialize();

    // إيماءات مخصصة
    bool registerCustomGesture(const std::string& name,
                              const std::vector<ArabicGesturePoint>& pattern);
    bool unregisterCustomGesture(const std::string& name);

    // التعرف على الإيماءات
    std::string recognizeGesture(const std::vector<ArabicGesturePoint>& input);
    float getRecognitionAccuracy() const;

    // إيماءات مدمجة
    bool isBuiltinGestureSupported(const std::string& gestureName) const;
    std::vector<std::string> getSupportedBuiltinGestures() const;

    bool isLoaded() const { return initialized; }
};

// ════════════════════════════════════════════════════════════
// 🥽 نظام الواقع الافتراضي
// ════════════════════════════════════════════════════════════

struct ArabicVRPosition {
    float x, y, z;
    ArabicVRPosition() : x(0), y(0), z(0) {}
    ArabicVRPosition(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct ArabicVRRotation {
    float pitch, yaw, roll; // درجات
    ArabicVRRotation() : pitch(0), yaw(0), roll(0) {}
    ArabicVRRotation(float p, float y, float r) : pitch(p), yaw(y), roll(r) {}
};

struct ArabicVRHandState {
    ArabicVRPosition position;
    ArabicVRRotation rotation;
    bool isTracked;
    std::unordered_map<std::string, bool> fingers; // إصبع: مضغوط أم لا

    ArabicVRHandState() : isTracked(false) {}
};

class ArabicVR {
private:
    std::unique_ptr<ArabicFFI> ffi;
    bool initialized;

public:
    ArabicVR();
    ~ArabicVR() = default;

    // تهيئة نظام VR
    bool initialize();

    // دعم VR
    bool isVRSupported() const;
    bool isVRActive() const;

    // موقع واتجاه الرأس
    ArabicVRPosition getHeadPosition() const;
    ArabicVRRotation getHeadRotation() const;

    // حالة اليدين
    ArabicVRHandState getLeftHandState() const;
    ArabicVRHandState getRightHandState() const;

    // أدوات VR
    bool vibrateController(int handIndex, float intensity, int durationMs);
    float getBatteryLevel(int handIndex) const;

    bool isLoaded() const { return initialized; }
};

} // namespace ArabicInput

#endif // ARABIC_INPUT_API_H
