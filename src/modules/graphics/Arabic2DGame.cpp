#include "Arabic2DGame.h"
#include "../../core/ArabicDatabase.h"
#include "ArabicGraphics.h"
#include "ArabicUI.h"
#include "ArabicText.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace ArabicLanguage {

using UIColor = ArabicUI::UIColor;

// ────────────────────────────────────────────────────────
// تنفيذ لعبة المنصة 2D
// ────────────────────────────────────────────────────────

// Player Implementation
Arabic2DGame::Player::Player(Arabic2DGame* game)
    : GameEntity("Player", game), facingRight(true), jumpPower(400.0f),
      moveSpeed(200.0f), lives(3), score(0) {

    setPosition(glm::vec2(100.0f, 300.0f));
    setScale(glm::vec2(32.0f, 32.0f));

    // إنشاء Physics Body
    physicsBody = std::make_shared<PhysicsBody>();
    physicsBody->setMass(1.0f);
    physicsBody->setGravityEnabled(true);
    addComponent(physicsBody);

    // إنشاء Collider
    collider = std::make_shared<BoxCollider>(glm::vec2(32.0f, 32.0f));
    addComponent(collider);

    // إنشاء الرسوم المتحركة
    idleAnimation = std::make_shared<SpriteAnimation>(this);
    idleAnimation->setFrameTime(0.2f);
    // idleAnimation->addFrame(game->getTexture("player_idle")); 
    
    runAnimation = std::make_shared<SpriteAnimation>(this);
    runAnimation->setFrameTime(0.1f);
    // runAnimation->addFrame(game->getTexture("player_run"));
    
    jumpAnimation = std::make_shared<SpriteAnimation>(this);
    // jumpAnimation->addFrame(game->getTexture("player_jump"));
}

void Arabic2DGame::Player::handleInput(const ArabicInput& input) {
    float moveDir = 0.0f;

    if (input.isKeyPressed('D') || input.isKeyPressed(VK_RIGHT)) {
        moveDir = 1.0f;
        facingRight = true;
    }
    if (input.isKeyPressed('A') || input.isKeyPressed(VK_LEFT)) {
        moveDir = -1.0f;
        facingRight = false;
    }

    if (input.isKeyPressed(VK_SPACE) && onGround) {
        jump();
    }

    move(moveDir);
}

void Arabic2DGame::Player::jump() {
    if (onGround) {
        velocity.y = -jumpPower;
        onGround = false;
    }
}

void Arabic2DGame::Player::move(float direction) {
    velocity.x = direction * moveSpeed;
}

void Arabic2DGame::Player::takeDamage() {
    lives--;
    if (lives > 0) {
        // إعادة توضيع اللاعب
        setPosition(glm::vec2(100.0f, 300.0f));
        velocity = glm::vec2(0.0f);
    }
}

void Arabic2DGame::Player::update(double deltaTime) {
    GameEntity::update(deltaTime);

    // تحديث السرعة بناءً على الجاذبية
    if (!onGround) {
        velocity.y += static_cast<float>(980.0f * deltaTime); // جاذبية
    }

    // تحديث الموقع
    glm::vec2 newPos = getPosition2D() + velocity * static_cast<float>(deltaTime);
    setPosition(newPos);

    // إعادة تعيين سرعة X عندما لا يتحرك اللاعب
    if (std::abs(velocity.x) < 0.1f) {
        velocity.x = 0.0f;
    }
}

void Arabic2DGame::Player::updatePhysics(double deltaTime) {
    // التحقق من التصادم مع المنصات
    onGround = false;

    for (const auto& platform : game->platforms) {
        if (collider && platform->getComponent<Collider>()) {
            CollisionInfo info = collider->checkCollision(*platform->getComponent<Collider>());
            if (info.colliding) {
                // تصحيح الموقع
                setPosition(getPosition() + info.normal * info.depth);

                // التحقق من الاصطدام من الأعلى
                if (info.normal.y > 0.5f) {
                    onGround = true;
                    velocity.y = 0.0f;
                } else if (info.normal.y < -0.5f) {
                    velocity.y = 0.0f;
                }

                if (std::abs(info.normal.x) > 0.5f) {
                    velocity.x = 0.0f;
                }
            }
        }
    }
}

void Arabic2DGame::Player::render(ArabicGraphics& graphics) {
    ArabicUI::UIColor playerColor = isAlive() ? ArabicUI::UIColor(0, 255, 0) : ArabicUI::UIColor(255, 0, 0);
    graphics.drawRectangle(getPosition2D(), getScale2D(), playerColor.toVec4(), true);
}

// Platform Implementation
Arabic2DGame::Platform::Platform(const std::string& name, Arabic2DGame* game, float w, float h)
    : GameEntity(name, game), width(w), height(h) {
    setScale(glm::vec2(w, h));
}

void Arabic2DGame::Platform::render(ArabicGraphics& graphics) {
    graphics.drawRectangle(getPosition2D(), getScale2D(), ArabicUI::UIColor(139, 69, 19).toVec4(), true);
}

// Coin Implementation
Arabic2DGame::Coin::Coin(const std::string& name, Arabic2DGame* game, const glm::vec2& pos)
    : GameEntity(name, game), rotation(0.0f), collected(false) {
    setPosition(pos);
    setScale(glm::vec2(16.0f, 16.0f));

    collider = std::make_shared<CircleCollider>(8.0f);
    addComponent(collider);
}

void Arabic2DGame::Coin::update(double deltaTime) {
    if (!collected) {
        rotation += static_cast<float>(180.0f * deltaTime); // دوران 180 درجة في الثانية
    }
}

void Arabic2DGame::Coin::render(ArabicGraphics& graphics) {
    if (!collected) {
        // رسم عملة ذهبية مع دوران
        graphics.drawCircle(getPosition2D(), 8.0f, ArabicUI::UIColor(255, 215, 0).toVec4(), true);
        graphics.drawCircle(getPosition2D(), 6.0f, ArabicUI::UIColor(255, 255, 0).toVec4(), true);
    }
}

void Arabic2DGame::Coin::collect() {
    collected = true;
}

// Enemy Implementation
Arabic2DGame::Enemy::Enemy(const std::string& name, Arabic2DGame* game, const glm::vec2& pos, float patrolDist)
    : GameEntity(name, game), patrolDistance(patrolDist), startX(pos.x), movingRight(true) {
    setPosition(pos);
    setScale(glm::vec2(24.0f, 24.0f));

    physicsBody = std::make_shared<PhysicsBody>();
    physicsBody->setMass(1.0f);
    physicsBody->setGravityEnabled(true);
    addComponent(physicsBody);

    collider = std::make_shared<BoxCollider>(glm::vec2(24.0f, 24.0f));
    addComponent(collider);
}

void Arabic2DGame::Enemy::update(double deltaTime) {
    GameEntity::update(deltaTime);

    // حركة الدورية
    float currentX = getPosition().x;
    if (movingRight) {
        if (currentX >= startX + patrolDistance) {
            movingRight = false;
        } else {
            velocity.x = 50.0f;
        }
    } else {
        if (currentX <= startX) {
            movingRight = true;
        } else {
            velocity.x = -50.0f;
        }
    }

    // تحديث الموقع
    glm::vec2 newPos = getPosition2D() + velocity * (float)deltaTime;
    setPosition(newPos);
}

void Arabic2DGame::Enemy::updatePhysics(double deltaTime) {
    // نفس منطق اللاعب للمنصات
    bool onGround = false;
    velocity.y += static_cast<float>(980.0f * deltaTime);

    for (const auto& platform : game->platforms) {
        if (collider && platform->getComponent<Collider>()) {
            CollisionInfo info = collider->checkCollision(*platform->getComponent<Collider>());
            if (info.colliding) {
                setPosition(getPosition() + info.normal * info.depth);

                if (info.normal.y > 0.5f) {
                    onGround = true;
                    velocity.y = 0.0f;
                } else if (info.normal.y < -0.5f) {
                    velocity.y = 0.0f;
                }
            }
        }
    }
}

void Arabic2DGame::Enemy::render(ArabicGraphics& graphics) {
    graphics.drawRectangle(getPosition2D(), getScale2D(), ArabicUI::UIColor(255, 0, 0).toVec4(), true);
}

void Arabic2DGame::Enemy::onPlayerCollision() {
    // إعادة توضيع العدو
    setPosition(glm::vec2(startX, getPosition().y));
}

// GameCamera Implementation
Arabic2DGame::GameCamera::GameCamera(Arabic2DGame* g)
    : position(0.0f), target(0.0f), smoothing(5.0f), game(g) {}

void Arabic2DGame::GameCamera::update(double deltaTime) {
    // تتبع اللاعب مع تلطيف
    if (game->getPlayer()) {
        auto player = game->getPlayer();
        target = player->getPosition2D() + player->getScale2D() * 0.5f - glm::vec2(400.0f, 300.0f);
    }

    // تلطيف حركة الكاميرا
    float factor = static_cast<float>(smoothing * deltaTime);
    if (factor > 1.0f) factor = 1.0f;
    position = position + (target - position) * factor;
}

void Arabic2DGame::GameCamera::setTarget(const glm::vec2& targetPos) {
    target = targetPos;
}

glm::mat4 Arabic2DGame::GameCamera::getViewMatrix() const {
    return glm::translate(glm::mat4(1.0f), glm::vec3(-position.x, -position.y, 0.0f));
}

void Arabic2DGame::GameCamera::applyToGraphics(ArabicGraphics& graphics) {
    graphics.setViewMatrix(getViewMatrix());
}

// Arabic2DGame Implementation
Arabic2DGame::Arabic2DGame()
    : currentState(MENU), gravity(0.0f, 980.0f), gameSpeed(1.0f),
      levelWidth(2000), levelHeight(600), rng(std::random_device{}()), dist(0.0f, 1.0f) {

    engine = std::make_shared<ArabicGameEngine>();
    input = std::make_shared<ArabicInput>();
    audio = std::make_shared<ArabicAudio>();
    sceneManager = std::make_shared<SceneManager>();

    camera = std::make_unique<GameCamera>(this);
}

bool Arabic2DGame::initialize(HINSTANCE hInstance) {
    // تهيئة المحرك
    if (!engine->initialize(hInstance, "لعبة المنصة العربية 2D", 800, 600)) {
        return false;
    }

    // تهيئة الإدخال والصوت
    input->initialize();
    audio->initialize();

    // تحميل الموارد
    loadResources();

    // إنشاء UI
    createUI();

    // إنشاء مستوى اللعبة
    createLevel();

    return true;
}

void Arabic2DGame::loadResources() {
    // تحميل الأصوات
    sounds["jump"] = audio->loadSound("resources/sounds/jump.wav");
    sounds["coin"] = audio->loadSound("resources/sounds/coin.wav");
    sounds["damage"] = audio->loadSound("resources/sounds/damage.wav");

    // تحميل الخطوط والنصوص
    // (سنضيفها حسب الحاجة)
}

void Arabic2DGame::createLevel() {
    // إنشاء اللاعب
    player = std::make_unique<Player>(this);

    // إنشاء المنصات
    addPlatform(glm::vec2(0, 500), 400, 100);     // منصة البداية
    addPlatform(glm::vec2(500, 450), 200, 50);    // منصة متوسطة
    addPlatform(glm::vec2(800, 400), 300, 50);    // منصة علوية
    addPlatform(glm::vec2(1200, 350), 200, 50);   // منصة أعلى
    addPlatform(glm::vec2(1500, 300), 500, 50);   // منصة النهاية

    // إنشاء العملات
    addCoin(glm::vec2(550, 380));  // فوق المنصة المتوسطة
    addCoin(glm::vec2(850, 330));  // فوق المنصة العلوية
    addCoin(glm::vec2(1250, 280)); // فوق المنصة الأعلى
    addCoin(glm::vec2(1600, 230)); // فوق منصة النهاية

    // إنشاء الأعداء
    addEnemy(glm::vec2(700, 380), 100);  // عدو على المنصة المتوسطة
    addEnemy(glm::vec2(1300, 280), 150); // عدو على المنصة الأعلى
}

void Arabic2DGame::createUI() {
    // إنشاء أزرار القائمة
    restartButton = std::make_shared<ArabicUI::UIButton>(
        "إعادة اللعب", glm::vec2(350, 250), glm::vec2(100, 40),
        [this]() { restartGame(); }
    );

    menuButton = std::make_shared<ArabicUI::UIButton>(
        "القائمة", glm::vec2(350, 300), glm::vec2(100, 40),
        [this]() { goToMenu(); }
    );

    // إنشاء تسميات النقاط والأرواح
    scoreLabel = std::make_shared<ArabicUI::UILabel>(
        "النقاط: 0", glm::vec2(10, 10), ArabicUI::UIColor(255, 255, 255)
    );

    livesLabel = std::make_shared<ArabicUI::UILabel>(
        "الأرواح: 3", glm::vec2(10, 40), ArabicUI::UIColor(255, 100, 100)
    );

    gameOverLabel = std::make_shared<ArabicUI::UILabel>(
        "انتهت اللعبة!", glm::vec2(300, 200), ArabicUI::UIColor(255, 0, 0)
    );
}

void Arabic2DGame::addPlatform(const glm::vec2& position, float width, float height) {
    auto platform = std::make_shared<Platform>(
        "Platform_" + std::to_string(platforms.size()), this, width, height
    );
    platform->setPosition(position);
    platforms.push_back(platform);
}

void Arabic2DGame::addCoin(const glm::vec2& position) {
    auto coin = std::make_shared<Coin>(
        "Coin_" + std::to_string(coins.size()), this, position
    );
    coins.push_back(coin);
}

void Arabic2DGame::addEnemy(const glm::vec2& position, float patrolDistance) {
    auto enemy = std::make_shared<Enemy>(
        "Enemy_" + std::to_string(enemies.size()), this, position, patrolDistance
    );
    enemies.push_back(enemy);
}

void Arabic2DGame::checkCollisions() {
    if (!player) return;

    // التحقق من تصادم اللاعب مع العملات
    for (auto it = coins.begin(); it != coins.end(); ) {
        auto& coin = *it;
        if (!coin->isCollected() && coin->getComponent<Collider>() &&
            player->getComponent<Collider>()) {

            CollisionInfo info = player->getComponent<Collider>()->checkCollision(
                *coin->getComponent<Collider>()
            );

            if (info.colliding) {
                coin->collect();
                player->addScore(100);
                if (sounds["coin"]) {
                    audio->playSound(sounds["coin"]);
                }
                it = coins.erase(it);
                continue;
            }
        }
        ++it;
    }

    // التحقق من تصادم اللاعب مع الأعداء
    for (auto& enemy : enemies) {
        if (enemy->getComponent<Collider>() && player->getComponent<Collider>()) {
            CollisionInfo info = player->getComponent<Collider>()->checkCollision(
                *enemy->getComponent<Collider>()
            );

            if (info.colliding) {
                player->takeDamage();
                enemy->onPlayerCollision();
                if (sounds["damage"]) {
                    audio->playSound(sounds["damage"]);
                }
            }
        }
    }
}

void Arabic2DGame::updateGameState(double deltaTime) {
    if (currentState == PLAYING && player) {
        // تحديث الكاميرا
        camera->update(deltaTime);

        // تحديث اللاعب والأعداء
        player->update(deltaTime);
        for (auto& enemy : enemies) {
            enemy->update(deltaTime);
        }

        // تحديث الفيزياء
        player->updatePhysics(deltaTime);
        for (auto& enemy : enemies) {
            enemy->updatePhysics(deltaTime);
        }

        // التحقق من التصادمات
        checkCollisions();

        // تحديث UI
        scoreLabel->setText("النقاط: " + std::to_string(player->getScore()));
        livesLabel->setText("الأرواح: " + std::to_string(player->getLives()));

        // التحقق من النهاية
        if (!player->isAlive()) {
            currentState = GAME_OVER;
        } else if (checkVictory()) {
            currentState = VICTORY;
        }

        // التحقق من سقوط اللاعب
        if (player->getPosition().y > levelHeight) {
            player->takeDamage();
        }
    }
}

bool Arabic2DGame::checkVictory() const {
    if (!player) return false;

    // الفوز عند الوصول للنهاية مع جمع جميع العملات
    return player->getPosition().x >= levelWidth - 100 &&
           coins.empty(); // جميع العملات مجمعة
}

void Arabic2DGame::renderUI() {
    scoreLabel->render(*engine->getGraphics());
    livesLabel->render(*engine->getGraphics());

    if (currentState == MENU || currentState == GAME_OVER || currentState == VICTORY) {
        restartButton->render(*engine->getGraphics());
        menuButton->render(*engine->getGraphics());

        // رسم رسالة الحالة
        std::string message;
        ArabicUI::UIColor color(255, 255, 255);

        switch (currentState) {
            case MENU:
                message = "مرحباً بك في لعبة المنصة العربية!";
                break;
            case GAME_OVER:
                message = "انتهت اللعبة! حاول مرة أخرى";
                color = ArabicUI::UIColor(255, 0, 0);
                break;
            case VICTORY:
                message = "مبروك! لقد فزت!";
                color = ArabicUI::UIColor(0, 255, 0);
                break;
            default:
                break;
        }

        if (!message.empty()) {
            engine->getGraphics()->drawText(
                message, glm::vec2(250, 200), color.toVec4(), 24.0f, true
            );
        }
    }
}

void Arabic2DGame::restartGame() {
    currentState = PLAYING;

    // إعادة إنشاء المستوى
    platforms.clear();
    coins.clear();
    enemies.clear();
    createLevel();

    // إعادة توضيع الكاميرا
    camera->setTarget(glm::vec2(0.0f));
}

void Arabic2DGame::goToMenu() {
    currentState = MENU;
}

void Arabic2DGame::run() {
    double lastTime = engine->getCurrentTime();

    while (engine->isRunning()) {
        double currentTime = engine->getCurrentTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // معالجة الأحداث
        engine->processEvents();

        // تحديث الإدخال
        input->update();

        // معالجة إدخال اللاعب
        if (currentState == PLAYING && player) {
            player->handleInput(*input);
        }

        // معالجة أحداث UI
        if (restartButton->handleInput(*input, *engine->getGraphics())) {
            restartGame();
        }
        if (menuButton->handleInput(*input, *engine->getGraphics())) {
            goToMenu();
        }

        // تحديث اللعبة
        updateGameState(deltaTime);

        // رسم
        engine->getGraphics()->clear(ArabicUI::UIColor(135, 206, 235).toVec4()); // سماء زرقاء

        if (currentState == PLAYING) {
            // تطبيق الكاميرا
            camera->applyToGraphics(*engine->getGraphics());

            // رسم المنصات
            for (auto& platform : platforms) {
                platform->render(*engine->getGraphics());
            }

            // رسم العملات
            for (auto& coin : coins) {
                coin->render(*engine->getGraphics());
            }

            // رسم الأعداء
            for (auto& enemy : enemies) {
                enemy->render(*engine->getGraphics());
            }

            // رسم اللاعب
            if (player) {
                player->render(*engine->getGraphics());
            }
        }

        // رسم UI
        renderUI();

        // تبديل الإطارات
        engine->swapBuffers();
    }
}

void Arabic2DGame::stop() {
    if (engine) {
        engine->shutdown();
    }
}

// ────────────────────────────────────────────────────────
// تنفيذ محرر النصوص العربي
// ────────────────────────────────────────────────────────

ArabicTextEditorApp::ArabicTextEditorApp() {
    engine = std::make_shared<ArabicGameEngine>();
    ui = std::make_shared<ArabicUI>();
    hasUnsavedChanges = false;
}

bool ArabicTextEditorApp::initialize(HINSTANCE hInstance) {
    if (!engine->initialize(hInstance, "محرر النصوص العربي", 1000, 700)) {
        return false;
    }

    ui->initialize(engine->getGraphics(), engine->getWindowHandle());
    
    // تهيئة محرك النصوص
    auto textEngine = std::make_shared<ArabicText>();
    textEngine->initialize();
    textEditor = std::make_shared<ArabicTextEditor>(textEngine);

    createUI();

    return true;
}

void ArabicTextEditorApp::createUI() {
    // منطقة النص الرئيسية
    textArea = std::make_shared<ArabicUI::UITextBox>(
        "", glm::vec2(50, 100), glm::vec2(900, 500)
    );
    textArea->setMultiline(true);
    textArea->setPlaceholder("ابدأ الكتابة هنا...");

    // الأزرار
    saveButton = std::make_shared<ArabicUI::UIButton>(
        "حفظ", glm::vec2(50, 620), glm::vec2(80, 30),
        [this]() {
            if (!currentFilePath.empty()) {
                saveFile(currentFilePath);
            } else {
                // يمكن إضافة حوار حفظ كملف جديد
                saveFile("untitled.txt");
            }
        }
    );

    loadButton = std::make_shared<ArabicUI::UIButton>(
        "تحميل", glm::vec2(140, 620), glm::vec2(80, 30),
        [this]() {
            // يمكن إضافة حوار اختيار ملف
            loadFile("sample.txt");
        }
    );

    clearButton = std::make_shared<ArabicUI::UIButton>(
        "مسح", glm::vec2(230, 620), glm::vec2(80, 30),
        [this]() {
            textArea->setText("");
            hasUnsavedChanges = true;
            updateStatusBar();
        }
    );

    // شريط الحالة
    statusLabel = std::make_shared<ArabicUI::UILabel>(
        "جاهز", glm::vec2(50, 670), ArabicUI::UIColor(128, 128, 128)
    );
}

void ArabicTextEditorApp::handleEvents() {
    // التحقق من التغييرات
    static std::string lastText;
    if (textArea->getText() != lastText) {
        hasUnsavedChanges = true;
        lastText = textArea->getText();
        updateStatusBar();
    }
}

void ArabicTextEditorApp::updateStatusBar() {
    std::string status = "الأحرف: " + std::to_string(textArea->getText().length());

    if (hasUnsavedChanges) {
        status += " - غير محفوظ";
    }

    if (!currentFilePath.empty()) {
        status += " - " + currentFilePath;
    }

    statusLabel->setText(status);
}

bool ArabicTextEditorApp::saveFile(const std::string& filePath) {
    std::ofstream file(filePath, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        statusLabel->setText("خطأ في الحفظ: " + filePath);
        return false;
    }

    std::string content = textArea->getText();
    file.write(content.c_str(), content.length());
    file.close();

    currentFilePath = filePath;
    hasUnsavedChanges = false;
    updateStatusBar();

    return true;
}

bool ArabicTextEditorApp::loadFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        statusLabel->setText("خطأ في التحميل: " + filePath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    textArea->setText(content);
    file.close();

    currentFilePath = filePath;
    hasUnsavedChanges = false;
    updateStatusBar();

    return true;
}

void ArabicTextEditorApp::run() {
    while (engine->isRunning()) {
        engine->processEvents();

        // تحديث UI
        ui->update(engine->getDeltaTime());

        // معالجة الأحداث
        handleEvents();

        // معالجة إدخال UI
        auto input = engine->getInput();
        textArea->handleInput(*input, *engine->getGraphics());
        saveButton->handleInput(*input, *engine->getGraphics());
        loadButton->handleInput(*input, *engine->getGraphics());
        clearButton->handleInput(*input, *engine->getGraphics());

        // رسم
        engine->getGraphics()->clear(ArabicUI::UIColor(240, 240, 240).toVec4());

        // رسم العنوان
        engine->getGraphics()->drawText(
            "محرر النصوص العربي", glm::vec2(500, 20),
            ArabicUI::UIColor(0, 0, 0).toVec4(), static_cast<float>(28.0f), true
        );

        // رسم UI
        textArea->render(*engine->getGraphics());
        saveButton->render(*engine->getGraphics());
        loadButton->render(*engine->getGraphics());
        clearButton->render(*engine->getGraphics());
        statusLabel->render(*engine->getGraphics());

        engine->swapBuffers();
    }
}

void ArabicTextEditorApp::stop() {
    if (engine) {
        engine->shutdown();
    }
}

// ────────────────────────────────────────────────────────
// تنفيذ أداة الرسم البسيطة
// ────────────────────────────────────────────────────────

ArabicDrawingApp::ArabicDrawingApp()
    : currentTool(PEN), currentColor(0, 0, 0), currentSize(2.0f),
      isDrawing(false), canvasWidth(800), canvasHeight(600) {

    engine = std::make_shared<ArabicGameEngine>();
    ui = std::make_shared<ArabicUI>();
}

bool ArabicDrawingApp::initialize(HINSTANCE hInstance) {
    if (!engine->initialize(hInstance, "أداة الرسم العربية", 1000, 700)) {
        return false;
    }

    ui->initialize(engine->getGraphics(), engine->getWindowHandle());
    createCanvas();
    createUI();

    return true;
}

void ArabicDrawingApp::createUI() {
    // أزرار الأدوات
    std::vector<std::string> toolNames = {"قلم", "فرشاة", "ممحاة", "خط", "مستطيل", "دائرة", "تعبئة"};
    std::vector<DrawingTool> tools = {PEN, BRUSH, ERASER, LINE, RECTANGLE, CIRCLE, FILL};

    for (size_t i = 0; i < toolNames.size(); ++i) {
        auto button = std::make_shared<ArabicUI::UIButton>(
            toolNames[i], glm::vec2(820, static_cast<float>(50 + i * 40)), glm::vec2(120, 35),
            [this, tool = tools[i]]() { setCurrentTool(tool); }
        );
        toolButtons.push_back(button);
    }

    // أزرار أخرى
    clearButton = std::make_shared<ArabicUI::UIButton>(
        "مسح الكل", glm::vec2(820, 380), glm::vec2(120, 35),
        [this]() { clearCanvas(); }
    );

    saveButton = std::make_shared<ArabicUI::UIButton>(
        "حفظ الصورة", glm::vec2(820, 425), glm::vec2(120, 35),
        [this]() { saveImage("drawing.png"); }
    );

    // تسمية الأداة الحالية
    toolLabel = std::make_shared<ArabicUI::UILabel>(
        "الأداة: قلم", glm::vec2(820, 480), ArabicUI::UIColor(0, 0, 0)
    );
}

void ArabicDrawingApp::createCanvas() {
    // إنشاء نسيج فارغ للرسم
    glGenTextures(1, &canvasTexture);
    glBindTexture(GL_TEXTURE_2D, canvasTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // إنشاء بيانات فارغة للنسيج
    std::vector<unsigned char> emptyData(canvasWidth * canvasHeight * 4, 255); // RGBA
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvasWidth, canvasHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, emptyData.data());
}

void ArabicDrawingApp::setCurrentTool(DrawingTool tool) {
    currentTool = tool;

    std::string toolName;
    switch (tool) {
        case PEN: toolName = "قلم"; break;
        case BRUSH: toolName = "فرشاة"; break;
        case ERASER: toolName = "ممحاة"; break;
        case LINE: toolName = "خط"; break;
        case RECTANGLE: toolName = "مستطيل"; break;
        case CIRCLE: toolName = "دائرة"; break;
        case FILL: toolName = "تعبئة"; break;
    }

    toolLabel->setText("الأداة: " + toolName);
}

void ArabicDrawingApp::handleDrawing(const ArabicInput& input) {
    glm::vec2 mousePos = input.getMousePosition();
    bool mousePressed = input.isMouseButtonPressed(static_cast<ArabicInput::MouseButton>(0));

    // التأكد من أن الماوس داخل اللوحة
    if (mousePos.x >= 10 && mousePos.x <= 810 && mousePos.y >= 10 && mousePos.y <= 610) {
        if (mousePressed && !isDrawing) {
            // بداية رسم ضربة جديدة
            isDrawing = true;
            currentStroke = Stroke(currentTool, currentColor, currentSize);
            currentStroke.points.push_back(mousePos);
        } else if (mousePressed && isDrawing) {
            // إضافة نقطة للضربة الحالية
            currentStroke.points.push_back(mousePos);
        } else if (!mousePressed && isDrawing) {
            // انتهاء الضربة
            isDrawing = false;
            if (!currentStroke.points.empty()) {
                strokes.push_back(currentStroke);
                applyStrokeToCanvas(currentStroke);
            }
        }
    } else if (!mousePressed) {
        isDrawing = false;
    }
}

void ArabicDrawingApp::applyStrokeToCanvas(const Stroke& stroke) {
    // تطبيق الضربة على نسيج اللوحة
    // (تنفيذ بسيط - يمكن تحسينه باستخدام OpenGL)
    glBindTexture(GL_TEXTURE_2D, canvasTexture);

    // للتبسيط، سنرسم خطوط بين النقاط
    if (stroke.points.size() >= 2) {
        for (size_t i = 1; i < stroke.points.size(); ++i) {
            glm::vec2 start = stroke.points[i-1];
            glm::vec2 end = stroke.points[i];

            // رسم خط بسيط بين النقطتين
            // (يمكن تحسين هذا باستخدام خوارزميات رسم خطوط أفضل)
            glm::vec2 diff = end - start;
            float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            glm::vec2 direction = glm::normalize(end - start);

            for (float d = 0; d <= distance; d += 1.0f) {
                glm::vec2 point = start + direction * d;
                int x = static_cast<int>(point.x - 10); // offset for canvas position
                int y = static_cast<int>(point.y - 10);

                if (x >= 0 && x < canvasWidth && y >= 0 && y < canvasHeight) {
                    // تطبيق اللون على البكسل (تبسيط شديد)
                    // في التطبيق الحقيقي، استخدم Framebuffer Object
                }
            }
        }
    }
}

void ArabicDrawingApp::renderStrokes(ArabicGraphics& graphics) {
    // رسم الضربات المكتملة
    for (const auto& stroke : strokes) {
        if (stroke.points.size() >= 2) {
            for (size_t i = 1; i < stroke.points.size(); ++i) {
                graphics.drawLine(glm::vec3(stroke.points[i-1].x, stroke.points[i-1].y, 0.0f),
                               glm::vec3(stroke.points[i].x, stroke.points[i].y, 0.0f),
                               stroke.color.toVec4(), stroke.size);
            }
        }
    }

    // رسم الضربة الحالية
    if (isDrawing && currentStroke.points.size() >= 2) {
        for (size_t i = 1; i < currentStroke.points.size(); ++i) {
            graphics.drawLine(glm::vec3(currentStroke.points[i-1].x, currentStroke.points[i-1].y, 0.0f),
                           glm::vec3(currentStroke.points[i].x, currentStroke.points[i].y, 0.0f),
                           currentStroke.color.toVec4(), currentStroke.size);
        }
    }
}

bool ArabicDrawingApp::saveImage(const std::string& filePath) {
    // حفظ اللوحة كصورة
    // (تنفيذ بسيط - يمكن تحسينه باستخدام مكتبات مثل stb_image)
    std::ofstream file(filePath, std::ios::binary);
    if (!file) return false;

    // رأس ملف BMP بسيط
    // (للتبسيط، سننشئ صورة فارغة)

    file.close();
    return true;
}

void ArabicDrawingApp::clearCanvas() {
    strokes.clear();
    currentStroke.points.clear();

    // إعادة إنشاء اللوحة الفارغة
    glBindTexture(GL_TEXTURE_2D, canvasTexture);
    std::vector<unsigned char> emptyData(canvasWidth * canvasHeight * 4, 255);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, canvasWidth, canvasHeight,
                   GL_RGBA, GL_UNSIGNED_BYTE, emptyData.data());
}

void ArabicDrawingApp::run() {
    while (engine->isRunning()) {
        engine->processEvents();

        // تحديث UI
        ui->update(static_cast<float>(engine->getDeltaTime()));

        // معالجة الرسم
        auto input = engine->getInput();
        handleDrawing(*input);

        // معالجة أزرار UI
        for (auto& button : toolButtons) {
            button->handleInput(*input, *engine->getGraphics());
        }
        clearButton->handleInput(*input, *engine->getGraphics());
        saveButton->handleInput(*input, *engine->getGraphics());

        // رسم
        engine->getGraphics()->clear(ArabicUI::UIColor(200, 200, 200).toVec4());

        // رسم العنوان
        engine->getGraphics()->drawText(
            "أداة الرسم العربية", glm::vec2(400, 20),
            ArabicUI::UIColor(0, 0, 0).toVec4(), 28.0f, true
        );

        // رسم اللوحة (مستطيل أبيض للرسم)
        engine->getGraphics()->drawRectangle(
            glm::vec2(10, 10), glm::vec2(980, 640),
            ArabicUI::UIColor(255, 255, 255).toVec4(), true
        );

        // رسم الضربات
        renderStrokes(*engine->getGraphics());

        // رسم UI
        for (auto& button : toolButtons) {
            button->render(*engine->getGraphics());
        }
        clearButton->render(*engine->getGraphics());
        saveButton->render(*engine->getGraphics());
        toolLabel->render(*engine->getGraphics());

        engine->swapBuffers();
    }
}

void ArabicDrawingApp::stop() {
    if (canvasTexture != 0) {
        glDeleteTextures(1, &canvasTexture);
    }
    if (engine) {
        engine->shutdown();
    }
}

// ────────────────────────────────────────────────────────
// تنفيذ تطبيق إدارة المهام
// ────────────────────────────────────────────────────────

ArabicTaskManagerApp::ArabicTaskManagerApp() {
    engine = std::make_shared<ArabicGameEngine>();
    ui = std::make_shared<ArabicUI>();
    database = std::make_shared<ArabicDatabase>();
    nextTaskId = 1;
}

bool ArabicTaskManagerApp::initialize(HINSTANCE hInstance) {
    if (!engine->initialize(hInstance, "مدير المهام العربي", 800, 600)) {
        return false;
    }

    ui->initialize(engine->getGraphics(), engine->getWindowHandle());

    // تهيئة قاعدة البيانات
    if (!database->initialize("tasks.db")) {
        return false;
    }

    // إنشاء جدول المهام
    database->executeQuery(
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id INTEGER PRIMARY KEY,"
        "title TEXT NOT NULL,"
        "description TEXT,"
        "completed INTEGER DEFAULT 0,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "due_date DATETIME"
        ")"
    );

    loadTasks();
    createUI();

    return true;
}

void ArabicTaskManagerApp::createUI() {
    // حقول الإدخال
    titleInput = std::make_shared<ArabicUI::UITextBox>(
        "", glm::vec2(50, 50), glm::vec2(400, 30)
    );
    titleInput->setPlaceholder("عنوان المهمة...");

    descInput = std::make_shared<ArabicUI::UITextBox>(
        "", glm::vec2(50, 90), glm::vec2(400, 60)
    );
    descInput->setMultiline(true);
    descInput->setPlaceholder("وصف المهمة...");

    // الأزرار
    addButton = std::make_shared<ArabicUI::UIButton>(
        "إضافة مهمة", glm::vec2(470, 50), glm::vec2(100, 40),
        [this]() {
            std::string title = titleInput->getText();
            std::string desc = descInput->getText();
            if (!title.empty()) {
                addTask(title, desc);
                titleInput->setText("");
                descInput->setText("");
            }
        }
    );

    clearCompletedButton = std::make_shared<ArabicUI::UIButton>(
        "مسح المكتملة", glm::vec2(470, 100), glm::vec2(100, 40),
        [this]() { clearCompletedTasks(); }
    );

    // تحديث عرض المهام
    updateTaskDisplay();
}

void ArabicTaskManagerApp::loadTasks() {
    tasks.clear();

    auto results = database->executeQuery("SELECT * FROM tasks ORDER BY created_at DESC");

    for (const auto& row : results) {
        Task task;
        task.id = std::stoi(row.at("id"));
        task.title = row.at("title");
        task.description = row.at("description");
        task.completed = std::stoi(row.at("completed")) != 0;

        tasks.push_back(task);

        if (task.id >= nextTaskId) {
            nextTaskId = task.id + 1;
        }
    }
}

void ArabicTaskManagerApp::saveTasks() {
    // مسح الجدول وإعادة إدراج البيانات
    database->executeQuery("DELETE FROM tasks");

    for (const auto& task : tasks) {
        std::string query = "INSERT INTO tasks (id, title, description, completed) VALUES (" +
                           std::to_string(task.id) + ", '" +
                           task.title + "', '" +
                           task.description + "', " +
                           std::to_string(task.completed ? 1 : 0) + ")";
        database->executeQuery(query);
    }
}

void ArabicTaskManagerApp::addTask(const std::string& title, const std::string& description) {
    Task newTask(nextTaskId++, title, description);
    tasks.push_back(newTask);
    saveTasks();
    updateTaskDisplay();
}

void ArabicTaskManagerApp::removeTask(int taskId) {
    auto it = std::remove_if(tasks.begin(), tasks.end(),
                            [taskId](const Task& t) { return t.id == taskId; });
    if (it != tasks.end()) {
        tasks.erase(it, tasks.end());
        saveTasks();
        updateTaskDisplay();
    }
}

void ArabicTaskManagerApp::toggleTask(int taskId) {
    for (auto& task : tasks) {
        if (task.id == taskId) {
            task.completed = !task.completed;
            break;
        }
    }
    saveTasks();
    updateTaskDisplay();
}

void ArabicTaskManagerApp::updateTaskDisplay() {
    // مسح الأزرار والتسميات القديمة
    taskButtons.clear();
    taskLabels.clear();

    // إنشاء أزرار وتسميات للمهام
    float yOffset = 200.0f;
    for (const auto& task : tasks) {
        // تسمية المهمة
        UIColor textColor = task.completed ? UIColor(128, 128, 128) : UIColor(0, 0, 0);
        auto label = std::make_shared<ArabicUI::UILabel>(
            task.title + (task.completed ? " ✓" : ""), glm::vec2(50, yOffset), textColor
        );
        taskLabels.push_back(label);

        // زر تبديل الحالة
        auto toggleButton = std::make_shared<ArabicUI::UIButton>(
            task.completed ? "إلغاء" : "تم", glm::vec2(550, yOffset - 5), glm::vec2(60, 25),
            [this, taskId = task.id]() { toggleTask(taskId); }
        );
        taskButtons.push_back(toggleButton);

        // زر الحذف
        auto deleteButton = std::make_shared<ArabicUI::UIButton>(
            "حذف", glm::vec2(620, yOffset - 5), glm::vec2(60, 25),
            [this, taskId = task.id]() { removeTask(taskId); }
        );
        taskButtons.push_back(deleteButton);

        yOffset += 35.0f;
    }
}

void ArabicTaskManagerApp::clearCompletedTasks() {
    auto it = std::remove_if(tasks.begin(), tasks.end(),
                            [](const Task& t) { return t.completed; });
    if (it != tasks.end()) {
        tasks.erase(it, tasks.end());
        saveTasks();
        updateTaskDisplay();
    }
}

size_t ArabicTaskManagerApp::getCompletedTaskCount() const {
    return std::count_if(tasks.begin(), tasks.end(),
                        [](const Task& t) { return t.completed; });
}

void ArabicTaskManagerApp::run() {
    while (engine->isRunning()) {
        engine->processEvents();

        // تحديث UI
        ui->update(static_cast<float>(engine->getDeltaTime()));

        // معالجة إدخال UI
        auto input = engine->getInput();
        titleInput->handleInput(*input, *engine->getGraphics());
        descInput->handleInput(*input, *engine->getGraphics());
        addButton->handleInput(*input, *engine->getGraphics());
        clearCompletedButton->handleInput(*input, *engine->getGraphics());

        for (auto& button : taskButtons) {
            button->handleInput(*input, *engine->getGraphics());
        }

        // رسم
        engine->getGraphics()->clear(ArabicUI::UIColor(245, 245, 245).toVec4());

        // رسم العنوان
        engine->getGraphics()->drawText(
            "مدير المهام العربي", glm::vec2(300, 10),
            ArabicUI::UIColor(0, 0, 0).toVec4(), 28.0f, true
        );

        // رسم إحصائيات
        std::string stats = "المهام: " + std::to_string(tasks.size()) +
                           " | المكتملة: " + std::to_string(getCompletedTaskCount());
        engine->getGraphics()->drawText(
            stats, glm::vec2(50, 160), ArabicUI::UIColor(100, 100, 100).toVec4(), 16.0f, true
        );

        // رسم UI
        titleInput->render(*engine->getGraphics());
        descInput->render(*engine->getGraphics());
        addButton->render(*engine->getGraphics());
        clearCompletedButton->render(*engine->getGraphics());

        for (auto& label : taskLabels) {
            label->render(*engine->getGraphics());
        }
        for (auto& button : taskButtons) {
            button->render(*engine->getGraphics());
        }

        engine->swapBuffers();
    }
}

void ArabicTaskManagerApp::stop() {
    saveTasks();
    if (engine) {
        engine->shutdown();
    }
}

} // namespace ArabicLanguage
