#include "ArabicGameEngine.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace ArabicLanguage {

// Window procedure for handling Windows messages
LRESULT CALLBACK GameEngineWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ArabicGameEngine* engine = reinterpret_cast<ArabicGameEngine*>(
        GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (engine) {
        engine->processWindowsMessage(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

ArabicGameEngine::ArabicGameEngine()
    : state(EngineState::STATE_UNINITIALIZED), running(false), accumulator(0.0), fixedTimeStep(1.0 / 60.0) {
}

ArabicGameEngine::~ArabicGameEngine() {
    stop();
}

bool ArabicGameEngine::initialize(HINSTANCE hInstance, const EngineConfig& cfg) {
    if (state != EngineState::STATE_UNINITIALIZED) {
        return false;
    }

    state = EngineState::STATE_INITIALIZING;
    config = cfg;

    // Register window class
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = GameEngineWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = "ArabicGameEngineWindow";
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        std::cerr << "Failed to register window class" << std::endl;
        state = EngineState::STATE_ERROR;
        return false;
    }

    // Create window
    DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    if (config.fullscreen) {
        windowStyle = WS_POPUP;
    }

    RECT windowRect = {0, 0, config.windowWidth, config.windowHeight};
    AdjustWindowRect(&windowRect, windowStyle, FALSE);

    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;

    windowHandle = CreateWindowExA(
        0,
        "ArabicGameEngineWindow",
        config.windowTitle.c_str(),
        windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!windowHandle) {
        std::cerr << "Failed to create window" << std::endl;
        state = EngineState::STATE_ERROR;
        return false;
    }

    // Store engine pointer in window
    SetWindowLongPtr(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Initialize graphics
    graphics = std::make_shared<ArabicGraphics>();
    ArabicGraphics::InitParams graphicsParams;
    graphicsParams.windowWidth = config.windowWidth;
    graphicsParams.windowHeight = config.windowHeight;
    graphicsParams.vsync = config.vsync;

    HDC hdc = GetDC(windowHandle);
    if (!graphics->initialize(hdc, graphicsParams)) {
        std::cerr << "Failed to initialize graphics" << std::endl;
        ReleaseDC(windowHandle, hdc);
        state = EngineState::STATE_ERROR;
        return false;
    }
    ReleaseDC(windowHandle, hdc);

    // Initialize UI
    ui = std::make_shared<ArabicUI>();
    if (!ui->initialize(graphics, windowHandle)) {
        std::cerr << "Failed to initialize UI" << std::endl;
        state = EngineState::STATE_ERROR;
        return false;
    }

    // Initialize Input
    input = std::make_shared<ArabicInput>();
    input->initialize(windowHandle);

    // Initialize timing
    startTime = std::chrono::steady_clock::now();
    lastFrameTime = startTime;

    state = EngineState::STATE_STOPPED;
    return true;
}

bool ArabicGameEngine::start() {
    if (state != EngineState::STATE_STOPPED && state != EngineState::STATE_INITIALIZING) {
        return false;
    }

    running = true;
    state = EngineState::STATE_RUNNING;

    // Show window
    ShowWindow(windowHandle, SW_SHOW);
    UpdateWindow(windowHandle);

    // Call initialization callback
    if (onInit) {
        onInit();
    }

    // Start game loop thread
    gameThread = std::thread(&ArabicGameEngine::gameLoop, this);

    return true;
}

void ArabicGameEngine::stop() {
    if (state == EngineState::STATE_STOPPED || state == EngineState::STATE_UNINITIALIZED) {
        return;
    }

    running = false;
    state = EngineState::STATE_STOPPED;

    if (gameThread.joinable()) {
        gameThread.join();
    }

    // Call shutdown callback
    if (onShutdown) {
        onShutdown();
    }

    // Destroy window
    if (windowHandle) {
        DestroyWindow(windowHandle);
        windowHandle = nullptr;
    }
}

void ArabicGameEngine::pause() {
    if (state == EngineState::STATE_RUNNING) {
        state = EngineState::STATE_SUSPENDED;
    }
}

void ArabicGameEngine::resume() {
    if (state == EngineState::STATE_SUSPENDED) {
        state = EngineState::STATE_RUNNING;
    }
}

void ArabicGameEngine::beginFrame() {
    if (graphics) {
        graphics->beginFrame();
    }
}

void ArabicGameEngine::endFrame() {
    if (graphics) {
        graphics->endFrame();
    }
}

void ArabicGameEngine::gameLoop() {
    while (running) {
        if (state == EngineState::STATE_SUSPENDED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            processEvents();
            continue;
        }

        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = currentTime - lastFrameTime;
        double deltaTime = elapsed.count();

        lastFrameTime = currentTime;
        stats.deltaTime = deltaTime;

        // Cap delta time to prevent spiral of death
        if (deltaTime > 1.0 / 30.0) {
            deltaTime = 1.0 / 30.0;
        }

        accumulator += deltaTime;

        // Fixed timestep updates
        while (accumulator >= fixedTimeStep) {
            fixedUpdate(fixedTimeStep);
            accumulator -= fixedTimeStep;
        }

        // Variable timestep update
        if (onUpdate) {
            onUpdate(deltaTime);
        }

        // Render
        render();

        // Frame rate limiting
        if (config.targetFPS > 0) {
            double targetFrameTime = 1.0 / config.targetFPS;
            if (deltaTime < targetFrameTime) {
                std::this_thread::sleep_for(
                    std::chrono::duration<double>(targetFrameTime - deltaTime));
            }
        }

        updateStats();
    }
}

void ArabicGameEngine::fixedUpdate(double deltaTime) {
    // Fixed timestep logic here
    // Physics, AI updates, etc.
}

void ArabicGameEngine::render() {
    if (!graphics) return;

    graphics->beginFrame();

    // Clear screen
    graphics->clear();

    // Call render callback
    if (onRender) {
        onRender();
    }

    // Render UI on top
    if (ui) {
        ui->render();
    }

    graphics->endFrame();
}

void ArabicGameEngine::updateStats() {
    static int frameCount = 0;
    static double timeAccumulator = 0.0;
    static auto lastStatsUpdate = std::chrono::steady_clock::now();

    frameCount++;
    timeAccumulator += stats.deltaTime;

    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> statsElapsed = currentTime - lastStatsUpdate;

    if (statsElapsed.count() >= 1.0) { // Update stats every second
        stats.fps = frameCount / timeAccumulator;
        stats.frameTime = timeAccumulator / frameCount;

        // Reset counters
        frameCount = 0;
        timeAccumulator = 0.0;
        lastStatsUpdate = currentTime;
    }
}

void ArabicGameEngine::processEvents() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (input) {
        input->update();
    }
}

void ArabicGameEngine::swapBuffers() {
    HDC hdc = GetDC(windowHandle);
    if (hdc) {
        SwapBuffers(hdc);
        ReleaseDC(windowHandle, hdc);
    }
}

void ArabicGameEngine::processWindowsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Forward to UI system
    if (ui) {
        ui->processWindowsMessage(hwnd, msg, wParam, lParam);
    }

    // Forward to Input system
    if (input) {
        input->processWindowsMessage(hwnd, msg, wParam, lParam);
    }

    // Handle engine-specific messages
    switch (msg) {
        case WM_CLOSE:
            stop();
            break;

        case WM_SIZE:
            if (graphics) {
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);
                graphics->setViewport(0, 0, width, height);
            }
            break;

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                stop();
            }
            break;
    }
}

// ────────────────────────────────────────────────────────
// تطبيق ArabicInput
// ────────────────────────────────────────────────────────

ArabicInput::ArabicInput()
    : mouseX(0), mouseY(0), mouseDeltaX(0), mouseDeltaY(0), mouseWheelDelta(0),
      windowHandle(nullptr) {

    memset(keyStates, 0, sizeof(keyStates));
    memset(keyPressed, 0, sizeof(keyPressed));
    memset(keyReleased, 0, sizeof(keyReleased));

    memset(mouseButtonStates, 0, sizeof(mouseButtonStates));
    memset(mouseButtonPressed, 0, sizeof(mouseButtonPressed));
    memset(mouseButtonReleased, 0, sizeof(mouseButtonReleased));
    memset(mouseDoubleClicked, 0, sizeof(mouseDoubleClicked));
}

bool ArabicInput::initialize(HWND hwnd) {
    windowHandle = hwnd;
    return true;
}

void ArabicInput::update() {
    updateKeyStates();
    updateMouseStates();
    updateGamepadStates();

    // Clear one-frame states
    memset(keyPressed, 0, sizeof(keyPressed));
    memset(keyReleased, 0, sizeof(keyReleased));
    memset(mouseButtonPressed, 0, sizeof(mouseButtonPressed));
    memset(mouseButtonReleased, 0, sizeof(mouseButtonReleased));
    memset(mouseDoubleClicked, 0, sizeof(mouseDoubleClicked));

    mouseDeltaX = 0;
    mouseDeltaY = 0;
    mouseWheelDelta = 0;
}

void ArabicInput::updateKeyStates() {
    for (int i = 0; i < 256; ++i) {
        bool currentlyPressed = (GetAsyncKeyState(i) & 0x8000) != 0;

        if (currentlyPressed && keyStates[i] != KEY_PRESSED) {
            keyStates[i] = KEY_PRESSED;
            keyPressed[i] = true;
        } else if (!currentlyPressed && keyStates[i] == KEY_PRESSED) {
            keyStates[i] = KEY_RELEASED;
            keyReleased[i] = true;
        } else if (currentlyPressed) {
            keyStates[i] = KEY_HELD;
        }
    }
}

void ArabicInput::updateMouseStates() {
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    if (windowHandle) {
        ScreenToClient(windowHandle, &cursorPos);
    }

    int newMouseX = cursorPos.x;
    int newMouseY = cursorPos.y;

    mouseDeltaX = newMouseX - mouseX;
    mouseDeltaY = newMouseY - mouseY;

    mouseX = newMouseX;
    mouseY = newMouseY;

    // Update mouse buttons
    for (int i = 0; i < 3; ++i) {
        int buttonCode;
        switch (i) {
            case BUTTON_LEFT: buttonCode = VK_LBUTTON; break;
            case BUTTON_RIGHT: buttonCode = VK_RBUTTON; break;
            case BUTTON_MIDDLE: buttonCode = VK_MBUTTON; break;
        }

        bool currentlyPressed = (GetAsyncKeyState(buttonCode) & 0x8000) != 0;

        if (currentlyPressed && mouseButtonStates[i] != MOUSE_STATE_PRESSED) {
            mouseButtonStates[i] = MOUSE_STATE_PRESSED;
            mouseButtonPressed[i] = true;
        } else if (!currentlyPressed && mouseButtonStates[i] == MOUSE_STATE_PRESSED) {
            mouseButtonStates[i] = MOUSE_STATE_RELEASED;
            mouseButtonReleased[i] = true;
        }
    }
}

void ArabicInput::updateGamepadStates() {
    // XInput implementation for Xbox controllers
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    DWORD result = XInputGetState(0, &state); // First controller

    if (result == ERROR_SUCCESS) {
        gamepadState.connected = true;

        // Left stick
        gamepadState.leftStickX = state.Gamepad.sThumbLX / 32767.0f;
        gamepadState.leftStickY = state.Gamepad.sThumbLY / 32767.0f;

        // Right stick
        gamepadState.rightStickX = state.Gamepad.sThumbRX / 32767.0f;
        gamepadState.rightStickY = state.Gamepad.sThumbRY / 32767.0f;

        // Triggers
        gamepadState.leftTrigger = state.Gamepad.bLeftTrigger / 255.0f;
        gamepadState.rightTrigger = state.Gamepad.bRightTrigger / 255.0f;

        // Buttons
        gamepadState.buttons[0] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
        gamepadState.buttons[1] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
        gamepadState.buttons[2] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
        gamepadState.buttons[3] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
        gamepadState.buttons[4] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
        gamepadState.buttons[5] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
        gamepadState.buttons[6] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
        gamepadState.buttons[7] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
        gamepadState.buttons[8] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
        gamepadState.buttons[9] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
        gamepadState.buttons[10] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
        gamepadState.buttons[11] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
        gamepadState.buttons[12] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
        gamepadState.buttons[13] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
    } else {
        gamepadState.connected = false;
    }
}

void ArabicInput::processWindowsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_KEYDOWN:
            // Already handled in updateKeyStates()
            break;

        case WM_KEYUP:
            // Already handled in updateKeyStates()
            break;

        case WM_LBUTTONDOWN:
            mouseButtonPressed[BUTTON_LEFT] = true;
            mouseButtonStates[BUTTON_LEFT] = MOUSE_STATE_PRESSED;
            break;

        case WM_LBUTTONUP:
            mouseButtonReleased[BUTTON_LEFT] = true;
            mouseButtonStates[BUTTON_LEFT] = MOUSE_STATE_RELEASED;
            break;

        case WM_LBUTTONDBLCLK:
            mouseDoubleClicked[BUTTON_LEFT] = true;
            break;

        case WM_RBUTTONDOWN:
            mouseButtonPressed[BUTTON_RIGHT] = true;
            mouseButtonStates[BUTTON_RIGHT] = MOUSE_STATE_PRESSED;
            break;

        case WM_RBUTTONUP:
            mouseButtonReleased[BUTTON_RIGHT] = true;
            mouseButtonStates[BUTTON_RIGHT] = MOUSE_STATE_RELEASED;
            break;

        case WM_RBUTTONDBLCLK:
            mouseDoubleClicked[BUTTON_RIGHT] = true;
            break;

        case WM_MBUTTONDOWN:
            mouseButtonPressed[BUTTON_MIDDLE] = true;
            mouseButtonStates[BUTTON_MIDDLE] = MOUSE_STATE_PRESSED;
            break;

        case WM_MBUTTONUP:
            mouseButtonReleased[BUTTON_MIDDLE] = true;
            mouseButtonStates[BUTTON_MIDDLE] = MOUSE_STATE_RELEASED;
            break;

        case WM_MOUSEWHEEL:
            mouseWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            break;

        case WM_CHAR:
            if (wParam >= 32) {
                textInputBuffer.push_back(static_cast<char>(wParam));
            }
            break;
    }
}

bool ArabicInput::isKeyPressed(int keyCode) const {
    return keyPressed[keyCode];
}

bool ArabicInput::isKeyHeld(int keyCode) const {
    return keyStates[keyCode] == KEY_HELD;
}

bool ArabicInput::isKeyReleased(int keyCode) const {
    return keyReleased[keyCode];
}

bool ArabicInput::isKeyDown(int keyCode) const {
    return keyStates[keyCode] == KEY_PRESSED || keyStates[keyCode] == KEY_HELD;
}

bool ArabicInput::isMouseButtonPressed(MouseButton button) const {
    return mouseButtonPressed[button];
}

bool ArabicInput::isMouseButtonHeld(MouseButton button) const {
    return mouseButtonStates[button] == MOUSE_STATE_PRESSED;
}

bool ArabicInput::isMouseButtonReleased(MouseButton button) const {
    return mouseButtonReleased[button];
}

bool ArabicInput::isMouseButtonClicked(MouseButton button) const {
    return mouseButtonStates[button] == MOUSE_STATE_CLICKED;
}

bool ArabicInput::isMouseButtonDoubleClicked(MouseButton button) const {
    return mouseDoubleClicked[button];
}

bool ArabicInput::isGamepadButtonPressed(int button) const {
    if (!gamepadState.connected || button < 0 || button >= 16) {
        return false;
    }
    return gamepadState.buttons[button];
}

float ArabicInput::getGamepadAxis(int axis) const {
    if (!gamepadState.connected) {
        return 0.0f;
    }

    switch (axis) {
        case 0: return gamepadState.leftStickX;
        case 1: return gamepadState.leftStickY;
        case 2: return gamepadState.rightStickX;
        case 3: return gamepadState.rightStickY;
        case 4: return gamepadState.leftTrigger;
        case 5: return gamepadState.rightTrigger;
        default: return 0.0f;
    }
}

void ArabicInput::clearTextInputBuffer() {
    textInputBuffer.clear();
}

void ArabicInput::showCursor(bool show) {
    ShowCursor(show ? TRUE : FALSE);
}

void ArabicInput::setCursorPosition(int x, int y) {
    if (windowHandle) {
        POINT point = {x, y};
        ClientToScreen(windowHandle, &point);
        SetCursorPos(point.x, point.y);
    }
}

void ArabicInput::clipCursorToWindow(bool clip) {
    if (!windowHandle) return;

    if (clip) {
        RECT rect;
        GetClientRect(windowHandle, &rect);
        ClientToScreen(windowHandle, reinterpret_cast<POINT*>(&rect.left));
        ClientToScreen(windowHandle, reinterpret_cast<POINT*>(&rect.right));
        ClipCursor(&rect);
    } else {
        ClipCursor(nullptr);
    }
}

// ────────────────────────────────────────────────────────
// تطبيق ArabicAudio
// ────────────────────────────────────────────────────────

ArabicAudio::AudioSource::AudioSource()
    : buffer(nullptr), isPlaying(false), isLooping(false), volume(1.0f), pan(0.0f) {
}

ArabicAudio::AudioSource::~AudioSource() {
    if (buffer) {
        buffer->Release();
        buffer = nullptr;
    }
}

bool ArabicAudio::AudioSource::loadFromFile(const std::string& filePath) {
    // Basic implementation - would need a WAV loader
    return false;
}

bool ArabicAudio::AudioSource::loadFromMemory(const void* data, size_t size, const AudioFormat& fmt) {
    // Basic implementation
    return false;
}

void ArabicAudio::AudioSource::play() {
    if (buffer) {
        if (SUCCEEDED(buffer->Play(0, 0, isLooping ? DSBPLAY_LOOPING : 0))) {
            isPlaying = true;
        }
    }
}

void ArabicAudio::AudioSource::pause() {
    if (buffer) {
        buffer->Stop();
        isPlaying = false;
    }
}

void ArabicAudio::AudioSource::stop() {
    if (buffer) {
        buffer->Stop();
        buffer->SetCurrentPosition(0);
        isPlaying = false;
    }
}

void ArabicAudio::AudioSource::resume() {
    if (buffer && !isPlaying) {
        play();
    }
}

void ArabicAudio::AudioSource::setVolume(float v) {
    volume = v;
    if (buffer) {
        // Convert 0.0-1.0 to DirectSound decibels
        LONG dsVol = (LONG)((1.0f - volume) * -10000.0f);
        buffer->SetVolume(dsVol);
    }
}

void ArabicAudio::AudioSource::setPan(float panVal) {
    pan = panVal;
    if (buffer) {
        LONG dsPan = (LONG)(pan * 10000.0f);
        buffer->SetPan(dsPan);
    }
}

void ArabicAudio::AudioSource::setLooping(bool loop) {
    isLooping = loop;
}

void ArabicAudio::AudioSource::update() {
    if (buffer) {
        DWORD status;
        if (SUCCEEDED(buffer->GetStatus(&status))) {
            if (!(status & DSBSTATUS_PLAYING)) {
                isPlaying = false;
            }
        }
    }
}

ArabicAudio::ArabicAudio() : directSound(nullptr), primaryBuffer(nullptr), initialized(false) {
}

ArabicAudio::~ArabicAudio() {
    shutdown();
}

bool ArabicAudio::initialize(HWND hwnd) {
    if (initialized) return true;

    if (!initDirectSound(hwnd)) {
        return false;
    }

    if (!createPrimaryBuffer()) {
        return false;
    }

    initialized = true;
    return true;
}

void ArabicAudio::shutdown() {
    if (!initialized) return;

    stopAll();

    sources.clear();

    if (primaryBuffer) {
        primaryBuffer->Release();
        primaryBuffer = nullptr;
    }

    if (directSound) {
        directSound->Release();
        directSound = nullptr;
    }

    initialized = false;
}

bool ArabicAudio::initDirectSound(HWND hwnd) {
    if (FAILED(DirectSoundCreate(nullptr, &directSound, nullptr))) {
        return false;
    }

    if (FAILED(directSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY))) {
        return false;
    }

    return true;
}

bool ArabicAudio::createPrimaryBuffer() {
    DSBUFFERDESC bufferDesc = {0};
    bufferDesc.dwSize = sizeof(DSBUFFERDESC);
    bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
    bufferDesc.dwBufferBytes = 0;
    bufferDesc.dwReserved = 0;
    bufferDesc.lpwfxFormat = nullptr;
    bufferDesc.guid3DAlgorithm = GUID_NULL;

    if (FAILED(directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, nullptr))) {
        return false;
    }

    // Set primary buffer format
    WAVEFORMATEX waveFormat = {0};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 2;
    waveFormat.nSamplesPerSec = 44100;
    waveFormat.nAvgBytesPerSec = 44100 * 4;
    waveFormat.nBlockAlign = 4;
    waveFormat.wBitsPerSample = 16;
    waveFormat.cbSize = 0;

    primaryBuffer->SetFormat(&waveFormat);

    return true;
}

std::shared_ptr<ArabicAudio::AudioSource> ArabicAudio::createSource() {
    auto source = std::make_shared<AudioSource>();
    sources.push_back(source);
    return source;
}

std::shared_ptr<ArabicAudio::AudioSource> ArabicAudio::loadAudio(const std::string& filePath) {
    auto source = createSource();
    if (source->loadFromFile(filePath)) {
        return source;
    }
    return nullptr;
}

void ArabicAudio::update() {
    // Update audio sources
    for (auto& source : sources) {
        source->update();
    }
}

void ArabicAudio::setMasterVolume(int volume) {
    config.masterVolume = (std::max)(0, (std::min)(100, volume));

    // Apply to primary buffer
    if (primaryBuffer) {
        LONG dsVolume = static_cast<LONG>((config.masterVolume / 100.0f - 1.0f) * 10000.0f);
        primaryBuffer->SetVolume(dsVolume);
    }
}

void ArabicAudio::setSFXVolume(int volume) {
    config.sfxVolume = (std::max)(0, (std::min)(100, volume));
}

void ArabicAudio::setMusicVolume(int volume) {
    config.musicVolume = (std::max)(0, (std::min)(100, volume));
}

void ArabicAudio::setVoiceVolume(int volume) {
    config.voiceVolume = (std::max)(0, (std::min)(100, volume));
}

void ArabicAudio::stopAll() {
    for (auto& source : sources) {
        source->stop();
    }
}

void ArabicAudio::pauseAll() {
    for (auto& source : sources) {
        source->pause();
    }
}

void ArabicAudio::resumeAll() {
    for (auto& source : sources) {
        if (source->getIsPlaying()) {
            source->play();
        }
    }
}

size_t ArabicAudio::getActiveSourceCount() const {
    return std::count_if(sources.begin(), sources.end(),
                        [](const std::shared_ptr<AudioSource>& source) {
                            return source->getIsPlaying();
                        });
}

// ────────────────────────────────────────────────────────
// تطبيق GameObject و Component
// ────────────────────────────────────────────────────────

GameObject::GameObject(const std::string& objName)
    : name(objName), position(0.0f), rotation(0.0f), scale(1.0f), active(true), layer(0) {
}

void GameObject::update(double deltaTime) {
    if (!active) return;
    for (auto& component : m_components) {
        if (component->isEnabled()) {
            component->update(deltaTime);
        }
    }
}

void GameObject::render(ArabicGraphics& graphics) {
    if (!active) return;
    for (auto& component : m_components) {
        if (component->isEnabled()) {
            component->render(graphics);
        }
    }
}

glm::mat4 GameObject::getTransformMatrix() const {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);

    transform = glm::rotate(transform, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

    transform = glm::scale(transform, scale);

    return transform;
}

bool GameObject::checkCollision(const GameObject* other) const {
    if (!other) return false;

    // Simple AABB collision detection
    glm::vec3 min1 = position - scale * 0.5f;
    glm::vec3 max1 = position + scale * 0.5f;
    glm::vec3 min2 = other->position - other->scale * 0.5f;
    glm::vec3 max2 = other->position + other->scale * 0.5f;

    return (min1.x <= max2.x && max1.x >= min2.x) &&
           (min1.y <= max2.y && max1.y >= min2.y) &&
           (min1.z <= max2.z && max1.z >= min2.z);
}

// ────────────────────────────────────────────────────────
// تطبيق SceneManager
// ────────────────────────────────────────────────────────

SceneManager::SceneManager() : sceneLoaded(false) {
}

void SceneManager::addObject(std::shared_ptr<GameObject> object) {
    if (object) {
        objects.push_back(object);
    }
}

void SceneManager::removeObject(const std::string& name) {
    objects.erase(std::remove_if(objects.begin(), objects.end(),
                                [&](const std::shared_ptr<GameObject>& obj) {
                                    return obj->getName() == name;
                                }), objects.end());
}

std::shared_ptr<GameObject> SceneManager::findObject(const std::string& name) const {
    auto it = std::find_if(objects.begin(), objects.end(),
                          [&](const std::shared_ptr<GameObject>& obj) {
                              return obj->getName() == name;
                          });
    return it != objects.end() ? *it : nullptr;
}

void SceneManager::updateAll(double deltaTime) {
    for (auto& object : objects) {
        if (object->isActive()) {
            object->update(deltaTime);
        }
    }
}

void SceneManager::renderAll(ArabicGraphics& graphics) {
    for (auto& object : objects) {
        if (object->isActive()) {
            object->render(graphics);
        }
    }
}

bool SceneManager::loadScene(const std::string& sceneName) {
    // Placeholder for scene loading
    // In production, this would load from file
    currentSceneName = sceneName;
    sceneLoaded = true;
    return true;
}

bool SceneManager::saveScene(const std::string& sceneName) const {
    // Placeholder for scene saving
    return true;
}

void SceneManager::clearScene() {
    objects.clear();
    currentSceneName.clear();
    sceneLoaded = false;
}

// ────────────────────────────────────────────────────────
// تطبيق Sprite2D و SpriteAnimation
// ────────────────────────────────────────────────────────

Sprite2D::Sprite2D(const std::string& spriteName)
    : GameObject(spriteName), textureId(0), size(1.0f), color(1.0f), uvRect(0.0f, 0.0f, 1.0f, 1.0f) {
}

void Sprite2D::render(ArabicGraphics& graphics) {
    if (!isActive()) return;

    glm::mat4 transform = getTransformMatrix();
    graphics.setModel(transform);

    if (textureId != 0) {
        // Render textured quad
        graphics.drawQuad(
            glm::vec3(position.x - size.x/2, position.y - size.y/2, position.z),
            size.x, size.y, color
        );
    } else {
        // Render colored quad
        graphics.drawQuad(
            glm::vec3(position.x - size.x/2, position.y - size.y/2, position.z),
            size.x, size.y, color
        );
    }
}

SpriteAnimation::SpriteAnimation(GameObject* obj)
    : Component(obj), frameTime(0.1f), currentTime(0.0f),
      currentFrame(0), looping(true), playing(false) {
}

void SpriteAnimation::addFrame(GLuint textureId) {
    frames.push_back(textureId);
}

void SpriteAnimation::play() {
    playing = true;
    currentFrame = 0;
    currentTime = 0.0f;
}

void SpriteAnimation::pause() {
    playing = false;
}

void SpriteAnimation::stop() {
    playing = false;
    currentFrame = 0;
    currentTime = 0.0f;
}

void SpriteAnimation::reset() {
    currentFrame = 0;
    currentTime = 0.0f;
}

void SpriteAnimation::update(double deltaTime) {
    if (!playing || frames.empty()) return;

    currentTime += static_cast<float>(deltaTime);

    if (currentTime >= frameTime) {
        currentTime = 0.0f;
        currentFrame++;

        if (currentFrame >= static_cast<int>(frames.size())) {
            if (looping) {
                currentFrame = 0;
            } else {
                stop();
                return;
            }
        }

        // Update sprite texture
        if (owner) {
            Sprite2D* sprite = dynamic_cast<Sprite2D*>(owner);
            if (sprite) {
                sprite->setTexture(frames[currentFrame]);
            }
        }
    }
}

// ────────────────────────────────────────────────────────
// تطبيق Model3D و Camera3D
// ────────────────────────────────────────────────────────

Model3D::Model3D(const std::string& modelName)
    : GameObject(modelName), vao(0), vbo(0), ebo(0),
      vertexCount(0), indexCount(0), textureId(0), color(1.0f) {
}

Model3D::~Model3D() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
}

bool Model3D::loadFromFile(const std::string& filePath) {
    // Placeholder for model loading
    // In production, implement OBJ/FBX loader
    return false;
}

void Model3D::createCube(float size) {
    float halfSize = size / 2.0f;

    std::vector<ArabicGraphics::Vertex> vertices = {
        // Front face
        {glm::vec3(-halfSize, -halfSize, halfSize), glm::vec3(0, 0, 1), glm::vec2(0, 0), color},
        {glm::vec3( halfSize, -halfSize, halfSize), glm::vec3(0, 0, 1), glm::vec2(1, 0), color},
        {glm::vec3( halfSize,  halfSize, halfSize), glm::vec3(0, 0, 1), glm::vec2(1, 1), color},
        {glm::vec3(-halfSize,  halfSize, halfSize), glm::vec3(0, 0, 1), glm::vec2(0, 1), color},

        // Back face
        {glm::vec3(-halfSize, -halfSize, -halfSize), glm::vec3(0, 0, -1), glm::vec2(0, 0), color},
        {glm::vec3(-halfSize,  halfSize, -halfSize), glm::vec3(0, 0, -1), glm::vec2(0, 1), color},
        {glm::vec3( halfSize,  halfSize, -halfSize), glm::vec3(0, 0, -1), glm::vec2(1, 1), color},
        {glm::vec3( halfSize, -halfSize, -halfSize), glm::vec3(0, 0, -1), glm::vec2(1, 0), color},

        // Left face
        {glm::vec3(-halfSize,  halfSize,  halfSize), glm::vec3(-1, 0, 0), glm::vec2(0, 1), color},
        {glm::vec3(-halfSize,  halfSize, -halfSize), glm::vec3(-1, 0, 0), glm::vec2(0, 0), color},
        {glm::vec3(-halfSize, -halfSize, -halfSize), glm::vec3(-1, 0, 0), glm::vec2(1, 0), color},
        {glm::vec3(-halfSize, -halfSize,  halfSize), glm::vec3(-1, 0, 0), glm::vec2(1, 1), color},

        // Right face
        {glm::vec3( halfSize,  halfSize,  halfSize), glm::vec3(1, 0, 0), glm::vec2(0, 1), color},
        {glm::vec3( halfSize, -halfSize,  halfSize), glm::vec3(1, 0, 0), glm::vec2(1, 1), color},
        {glm::vec3( halfSize, -halfSize, -halfSize), glm::vec3(1, 0, 0), glm::vec2(1, 0), color},
        {glm::vec3( halfSize,  halfSize, -halfSize), glm::vec3(1, 0, 0), glm::vec2(0, 0), color},

        // Top face
        {glm::vec3(-halfSize,  halfSize, -halfSize), glm::vec3(0, 1, 0), glm::vec2(0, 1), color},
        {glm::vec3(-halfSize,  halfSize,  halfSize), glm::vec3(0, 1, 0), glm::vec2(0, 0), color},
        {glm::vec3( halfSize,  halfSize,  halfSize), glm::vec3(0, 1, 0), glm::vec2(1, 0), color},
        {glm::vec3( halfSize,  halfSize, -halfSize), glm::vec3(0, 1, 0), glm::vec2(1, 1), color},

        // Bottom face
        {glm::vec3(-halfSize, -halfSize, -halfSize), glm::vec3(0, -1, 0), glm::vec2(0, 1), color},
        {glm::vec3( halfSize, -halfSize, -halfSize), glm::vec3(0, -1, 0), glm::vec2(1, 1), color},
        {glm::vec3( halfSize, -halfSize,  halfSize), glm::vec3(0, -1, 0), glm::vec2(1, 0), color},
        {glm::vec3(-halfSize, -halfSize,  halfSize), glm::vec3(0, -1, 0), glm::vec2(0, 0), color},
    };

    std::vector<GLuint> indices = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Left face
        8, 9, 10, 10, 11, 8,
        // Right face
        12, 13, 14, 14, 15, 12,
        // Top face
        16, 17, 18, 18, 19, 16,
        // Bottom face
        20, 21, 22, 22, 23, 20
    };

    vertexCount = vertices.size();
    indexCount = indices.size();

    // Create OpenGL buffers
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ArabicGraphics::Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                 indices.data(), GL_STATIC_DRAW);

    // Set vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ArabicGraphics::Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ArabicGraphics::Vertex),
                         (void*)offsetof(ArabicGraphics::Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ArabicGraphics::Vertex),
                         (void*)offsetof(ArabicGraphics::Vertex, texCoord));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(ArabicGraphics::Vertex),
                         (void*)offsetof(ArabicGraphics::Vertex, color));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

void Model3D::createSphere(float radius, int segments) {
    // Placeholder for sphere creation
    createCube(radius * 2.0f); // Temporary fallback
}

void Model3D::createPlane(float width, float height) {
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;

    std::vector<ArabicGraphics::Vertex> vertices = {
        {glm::vec3(-halfWidth, 0.0f, -halfHeight), glm::vec3(0, 1, 0), glm::vec2(0, 0), color},
        {glm::vec3( halfWidth, 0.0f, -halfHeight), glm::vec3(0, 1, 0), glm::vec2(1, 0), color},
        {glm::vec3( halfWidth, 0.0f,  halfHeight), glm::vec3(0, 1, 0), glm::vec2(1, 1), color},
        {glm::vec3(-halfWidth, 0.0f,  halfHeight), glm::vec3(0, 1, 0), glm::vec2(0, 1), color},
    };

    std::vector<GLuint> indices = { 0, 1, 2, 2, 3, 0 };

    vertexCount = vertices.size();
    indexCount = indices.size();

    // Create OpenGL buffers (same as cube)
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ArabicGraphics::Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                 indices.data(), GL_STATIC_DRAW);

    // Set vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ArabicGraphics::Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ArabicGraphics::Vertex),
                         (void*)offsetof(ArabicGraphics::Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ArabicGraphics::Vertex),
                         (void*)offsetof(ArabicGraphics::Vertex, texCoord));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(ArabicGraphics::Vertex),
                         (void*)offsetof(ArabicGraphics::Vertex, color));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

void Model3D::render(ArabicGraphics& graphics) {
    if (!isActive() || !vao) return;

    glm::mat4 transform = getTransformMatrix();
    graphics.setModel(transform);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

Camera3D::Camera3D(const std::string& cameraName)
    : GameObject(cameraName), fov(45.0f), nearPlane(0.1f), farPlane(1000.0f) {
    updateViewMatrix();
    updateProjection();
}

void Camera3D::update(double deltaTime) {
    updateViewMatrix();
}

void Camera3D::lookAt(const glm::vec3& target) {
    glm::vec3 diff = target - position;
    glm::vec3 direction = (glm::length(diff) > 0.0001f) ? glm::normalize(diff) : glm::vec3(0.0f, 0.0f, 1.0f);
    rotation.y = atan2(direction.x, direction.z);
    rotation.x = -asin(direction.y);

    updateViewMatrix();
}

void Camera3D::updateViewMatrix() {
    glm::vec3 front = glm::vec3(
        cos(rotation.y) * cos(rotation.x),
        sin(rotation.x),
        sin(rotation.y) * cos(rotation.x)
    );

    viewMatrix = glm::lookAt(position, position + front, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera3D::updateProjection() {
    projectionMatrix = glm::perspective(glm::radians(fov), 16.0f / 9.0f, nearPlane, farPlane);
}

// ────────────────────────────────────────────────────────
// تطبيق PhysicsBody و Collider
// ────────────────────────────────────────────────────────

PhysicsBody::PhysicsBody(GameObject* obj)
    : Component(obj), type(DYNAMIC), velocity(0.0f), acceleration(0.0f),
      mass(1.0f), friction(0.1f), restitution(0.5f), gravityEnabled(true) {
}

void PhysicsBody::applyForce(const glm::vec3& force) {
    if (type == STATIC) return;

    acceleration += force * (1.0f / mass);
}

void PhysicsBody::applyImpulse(const glm::vec3& impulse) {
    if (type == STATIC) return;

    velocity += impulse * (1.0f / mass);
}

void PhysicsBody::update(double deltaTime) {
    if (!owner || type == STATIC) return;

    // Apply gravity
    if (gravityEnabled && type == DYNAMIC) {
        acceleration.y -= 9.81f; // Gravity
    }

    // Update velocity
    velocity += acceleration * static_cast<float>(deltaTime);

    // Apply friction
    velocity *= (1.0f - friction);

    // Update position
    owner->translate(velocity * static_cast<float>(deltaTime));

    // Reset acceleration
    acceleration = glm::vec3(0.0f);
}

Collider::Collider(GameObject* obj)
    : Component(obj), shape(BOX), size(1.0f), m_isTrigger(false) {
}

BoxCollider::BoxCollider(const glm::vec2& sz, GameObject* obj)
    : Collider(obj) {
    setShape(BOX);
    setSize(glm::vec3(sz.x, sz.y, 1.0f));
}

BoxCollider::BoxCollider(const glm::vec3& sz, GameObject* obj)
    : Collider(obj) {
    setShape(BOX);
    setSize(sz);
}

CircleCollider::CircleCollider(float radius, GameObject* obj)
    : Collider(obj) {
    setShape(CIRCLE);
    setSize(glm::vec3(radius, radius, 0.0f));
}

CollisionInfo Collider::checkCollision(const Collider& other) const {
    CollisionInfo info;
    if (!owner || !other.getOwner()) return info;

    if (shape == BOX && other.getShape() == BOX) {
        // AABB collision
        glm::vec3 pos1 = owner->getPosition();
        glm::vec3 pos2 = other.getOwner()->getPosition();

        glm::vec3 halfSize1 = size * owner->getScale() * 0.5f;
        glm::vec3 halfSize2 = other.getSize() * other.getOwner()->getScale() * 0.5f;

        bool colliding = (std::abs(pos1.x - pos2.x) < (halfSize1.x + halfSize2.x)) &&
                         (std::abs(pos1.y - pos2.y) < (halfSize1.y + halfSize2.y)) &&
                         (std::abs(pos1.z - pos2.z) < (halfSize1.z + halfSize2.z));
        
        if (colliding) {
            info.colliding = true;
            info.other = other.getOwner();
            // Simple normal for now
            glm::vec3 diff = pos1 - pos2;
            info.normal = (glm::length(diff) > 0.0001f) ? glm::normalize(diff) : glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    return info;
}

} // namespace ArabicLanguage
