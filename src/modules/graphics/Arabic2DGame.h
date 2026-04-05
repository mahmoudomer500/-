#pragma once

#include "ArabicGameEngine.h"
#include "ArabicUI.h"
#include "ArabicText.h"
#include "../../core/ArabicDatabase.h"
#include <vector>
#include <memory>
#include <random>

namespace ArabicLanguage {

/**
 * @brief لعبة منصة قفز عربية 2D - Arabic 2D Platformer Game
 *
 * لعبة بسيطة تعرض إمكانيات محرك الألعاب العربي
 * مع شخصية قابلة للتحكم وقفز على المنصات
 */
class Arabic2DGame {
public:
    /**
     * @brief حالة اللعبة
     */
    enum GameState {
        MENU,
        PLAYING,
        PAUSED,
        GAME_OVER,
        VICTORY
    };

    /**
     * @brief عناصر اللعبة
     */
    class GameEntity : public GameObject {
    protected:
        glm::vec2 velocity;
        bool onGround;
        Arabic2DGame* game;

    public:
        GameEntity(const std::string& name, Arabic2DGame* g)
            : GameObject(name), velocity(0.0f), onGround(false), game(g) {}

        void setVelocity(const glm::vec2& vel) { velocity = vel; }
        const glm::vec2& getVelocity() const { return velocity; }

        void setOnGround(bool ground) { onGround = ground; }
        bool isOnGround() const { return onGround; }

        virtual void handleInput(const ArabicInput& input) {}
        virtual void updatePhysics(double deltaTime) {}
    };

    /**
     * @brief اللاعب الرئيسي
     */
    class Player : public GameEntity {
    private:
        bool facingRight;
        float jumpPower;
        float moveSpeed;
        int lives;
        int score;

        std::shared_ptr<SpriteAnimation> idleAnimation;
        std::shared_ptr<SpriteAnimation> runAnimation;
        std::shared_ptr<SpriteAnimation> jumpAnimation;

        std::shared_ptr<PhysicsBody> physicsBody;
        std::shared_ptr<Collider> collider;

    public:
        Player(Arabic2DGame* game);

        void handleInput(const ArabicInput& input) override;
        void update(double deltaTime) override;
        void updatePhysics(double deltaTime) override;
        void render(ArabicGraphics& graphics) override;

        void jump();
        void move(float direction);
        void takeDamage();

        int getLives() const { return lives; }
        int getScore() const { return score; }
        void addScore(int points) { score += points; }

        bool isAlive() const { return lives > 0; }
    };

    /**
     * @brief منصة ثابتة
     */
    class Platform : public GameEntity {
    private:
        float width, height;
        GLuint textureId;

    public:
        Platform(const std::string& name, Arabic2DGame* game, float w, float h);

        void update(double deltaTime) override {}
        void render(ArabicGraphics& graphics) override;
        void updatePhysics(double deltaTime) override {}

        float getWidth() const { return width; }
        float getHeight() const { return height; }
    };

    /**
     * @brief عملة قابلة للتجميع
     */
    class Coin : public GameEntity {
    private:
        float rotation;
        bool collected;
        GLuint textureId;

        std::shared_ptr<SpriteAnimation> spinAnimation;
        std::shared_ptr<Collider> collider;

    public:
        Coin(const std::string& name, Arabic2DGame* game, const glm::vec2& pos);

        void update(double deltaTime) override;
        void render(ArabicGraphics& graphics) override;
        void updatePhysics(double deltaTime) override {}

        bool isCollected() const { return collected; }
        void collect();
    };

    /**
     * @brief عدو بسيط
     */
    class Enemy : public GameEntity {
    private:
        float patrolDistance;
        float startX;
        bool movingRight;
        GLuint textureId;

        std::shared_ptr<PhysicsBody> physicsBody;
        std::shared_ptr<Collider> collider;

    public:
        Enemy(const std::string& name, Arabic2DGame* game, const glm::vec2& pos, float patrolDist);

        void update(double deltaTime) override;
        void updatePhysics(double deltaTime) override;
        void render(ArabicGraphics& graphics) override;

        void onPlayerCollision();
    };

    /**
     * @brief كاميرا اللعبة
     */
    class GameCamera {
    private:
        glm::vec2 position;
        glm::vec2 target;
        float smoothing;
        Arabic2DGame* game;

    public:
        GameCamera(Arabic2DGame* g);

        void update(double deltaTime);
        void setTarget(const glm::vec2& targetPos);
        const glm::vec2& getPosition() const { return position; }

        glm::mat4 getViewMatrix() const;
        void applyToGraphics(ArabicGraphics& graphics);
    };

private:
    std::shared_ptr<ArabicGameEngine> engine;
    std::shared_ptr<ArabicInput> input;
    std::shared_ptr<ArabicAudio> audio;
    std::shared_ptr<SceneManager> sceneManager;

    GameState currentState;
    std::unique_ptr<Player> player;
    std::unique_ptr<GameCamera> camera;

    std::vector<std::shared_ptr<Platform>> platforms;
    std::vector<std::shared_ptr<Coin>> coins;
    std::vector<std::shared_ptr<Enemy>> enemies;

    // UI Elements
    std::shared_ptr<ArabicUI::UIButton> restartButton;
    std::shared_ptr<ArabicUI::UIButton> menuButton;
    std::shared_ptr<ArabicUI::UILabel> scoreLabel;
    std::shared_ptr<ArabicUI::UILabel> livesLabel;
    std::shared_ptr<ArabicUI::UILabel> gameOverLabel;

    // Game settings
    glm::vec2 gravity;
    float gameSpeed;
    int levelWidth;
    int levelHeight;

    // Resources
    std::unordered_map<std::string, GLuint> textures;
    std::unordered_map<std::string, std::shared_ptr<ArabicAudio::AudioSource>> sounds;

    // Random number generator
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;

    /**
     * @brief إنشاء مستوى اللعبة
     */
    void createLevel();

    /**
     * @brief تحميل الموارد
     */
    void loadResources();

    /**
     * @brief إنشاء UI
     */
    void createUI();

    /**
     * @brief التحقق من التصادمات
     */
    void checkCollisions();

    /**
     * @brief تحديث حالة اللعبة
     */
    void updateGameState(double deltaTime);

    /**
     * @brief رسم UI
     */
    void renderUI();

    /**
     * @brief إعادة تشغيل اللعبة
     */
    void restartGame();

    /**
     * @brief الانتقال للقائمة الرئيسية
     */
    void goToMenu();

public:
    Arabic2DGame();
    ~Arabic2DGame() = default;

    /**
     * @brief تهيئة اللعبة
     */
    bool initialize(HINSTANCE hInstance);

    /**
     * @brief تشغيل اللعبة
     */
    void run();

    /**
     * @brief إيقاف اللعبة
     */
    void stop();

    /**
     * @brief الحصول على حالة اللعبة
     */
    GameState getCurrentState() const { return currentState; }

    /**
     * @brief إضافة منصة
     */
    void addPlatform(const glm::vec2& position, float width, float height);

    /**
     * @brief إضافة عملة
     */
    void addCoin(const glm::vec2& position);

    /**
     * @brief إضافة عدو
     */
    void addEnemy(const glm::vec2& position, float patrolDistance);

    /**
     * @brief التحقق من الفوز
     */
    bool checkVictory() const;

    /**
     * @brief الحصول على اللاعب
     */
    Player* getPlayer() const { return player.get(); }

    /**
     * @brief الحصول على النقاط
     */
    int getScore() const { return player ? player->getScore() : 0; }

    /**
     * @brief الحصول على عدد الأرواح
     */
    int getLives() const { return player ? player->getLives() : 0; }
};

// ────────────────────────────────────────────────────────
// محرر النصوص العربي
// ────────────────────────────────────────────────────────

class ArabicTextEditorApp {
private:
    std::shared_ptr<ArabicGameEngine> engine;
    std::shared_ptr<ArabicUI> ui;
    std::shared_ptr<ArabicTextEditor> textEditor;

    // UI Elements
    std::shared_ptr<ArabicUI::UITextBox> textArea;
    std::shared_ptr<ArabicUI::UIButton> saveButton;
    std::shared_ptr<ArabicUI::UIButton> loadButton;
    std::shared_ptr<ArabicUI::UIButton> clearButton;
    std::shared_ptr<ArabicUI::UILabel> statusLabel;

    std::string currentFilePath;
    bool hasUnsavedChanges;

    /**
     * @brief إنشاء UI المحرر
     */
    void createUI();

    /**
     * @brief معالجة أحداث المحرر
     */
    void handleEvents();

    /**
     * @brief حفظ الملف
     */
    bool saveFile(const std::string& filePath);

    /**
     * @brief تحميل الملف
     */
    bool loadFile(const std::string& filePath);

    /**
     * @brief تحديث شريط الحالة
     */
    void updateStatusBar();

public:
    ArabicTextEditorApp();
    ~ArabicTextEditorApp() = default;

    /**
     * @brief تهيئة المحرر
     */
    bool initialize(HINSTANCE hInstance);

    /**
     * @brief تشغيل المحرر
     */
    void run();

    /**
     * @brief إيقاف المحرر
     */
    void stop();
};

// ────────────────────────────────────────────────────────
// أداة الرسم البسيطة
// ────────────────────────────────────────────────────────

class ArabicDrawingApp {
public:
    /**
     * @brief أداة الرسم
     */
    enum DrawingTool {
        PEN,
        BRUSH,
        ERASER,
        LINE,
        RECTANGLE,
        CIRCLE,
        FILL
    };

    /**
     * @brief ضربة رسم
     */
    struct Stroke {
        DrawingTool tool;
        ArabicLanguage::ArabicUI::UIColor color;
        float size;
        std::vector<glm::vec2> points;

        Stroke(DrawingTool t = PEN, const ArabicLanguage::ArabicUI::UIColor& c = ArabicLanguage::ArabicUI::UIColor(0, 0, 0),
               float s = 1.0f) : tool(t), color(c), size(s) {}
    };

private:
    std::shared_ptr<ArabicGameEngine> engine;
    std::shared_ptr<ArabicUI> ui;

    // Drawing state
    std::vector<Stroke> strokes;
    Stroke currentStroke;
    DrawingTool currentTool;
    ArabicLanguage::ArabicUI::UIColor currentColor;
    float currentSize;

    bool isDrawing;
    glm::vec2 lastMousePos;

    // Canvas
    GLuint canvasTexture;
    int canvasWidth, canvasHeight;

    // UI Elements
    std::vector<std::shared_ptr<ArabicUI::UIButton>> toolButtons;
    std::shared_ptr<ArabicUI::UIButton> clearButton;
    std::shared_ptr<ArabicUI::UIButton> saveButton;
    std::shared_ptr<ArabicUI::UILabel> toolLabel;

    /**
     * @brief إنشاء UI أداة الرسم
     */
    void createUI();

    /**
     * @brief إنشاء لوحة الرسم
     */
    void createCanvas();

    /**
     * @brief معالجة الرسم
     */
    void handleDrawing(const ArabicInput& input);

    /**
     * @brief رسم الضربات
     */
    void renderStrokes(ArabicGraphics& graphics);

    /**
     * @brief تطبيق الرسم على اللوحة
     */
    void applyStrokeToCanvas(const Stroke& stroke);

    /**
     * @brief تغيير الأداة
     */
    void setCurrentTool(DrawingTool tool);

    /**
     * @brief حفظ الصورة
     */
    bool saveImage(const std::string& filePath);

public:
    ArabicDrawingApp();
    ~ArabicDrawingApp() = default;

    /**
     * @brief تهيئة أداة الرسم
     */
    bool initialize(HINSTANCE hInstance);

    /**
     * @brief تشغيل أداة الرسم
     */
    void run();

    /**
     * @brief إيقاف أداة الرسم
     */
    void stop();

    /**
     * @brief مسح اللوحة
     */
    void clearCanvas();

    /**
     * @brief الحصول على عدد الضربات
     */
    size_t getStrokeCount() const { return strokes.size(); }
};

// ────────────────────────────────────────────────────────
// تطبيق إدارة المهام
// ────────────────────────────────────────────────────────

class ArabicTaskManagerApp {
public:
    /**
     * @brief مهمة
     */
    struct Task {
        int id;
        std::string title;
        std::string description;
        bool completed;
        std::chrono::system_clock::time_point createdAt;
        std::chrono::system_clock::time_point dueDate;

        Task(int i = 0, const std::string& t = "", const std::string& d = "")
            : id(i), title(t), description(d), completed(false) {
            createdAt = std::chrono::system_clock::now();
        }
    };

private:
    std::shared_ptr<ArabicGameEngine> engine;
    std::shared_ptr<ArabicUI> ui;
    std::shared_ptr<ArabicDatabase> database;

    std::vector<Task> tasks;
    int nextTaskId;

    // UI Elements
    std::shared_ptr<ArabicUI::UITextBox> titleInput;
    std::shared_ptr<ArabicUI::UITextBox> descInput;
    std::shared_ptr<ArabicUI::UIButton> addButton;
    std::shared_ptr<ArabicUI::UIButton> clearCompletedButton;
    std::vector<std::shared_ptr<ArabicUI::UIButton>> taskButtons;
    std::vector<std::shared_ptr<ArabicUI::UILabel>> taskLabels;

    /**
     * @brief إنشاء UI إدارة المهام
     */
    void createUI();

    /**
     * @brief تحميل المهام من قاعدة البيانات
     */
    void loadTasks();

    /**
     * @brief حفظ المهام في قاعدة البيانات
     */
    void saveTasks();

    /**
     * @brief إضافة مهمة جديدة
     */
    void addTask(const std::string& title, const std::string& description);

    /**
     * @brief حذف مهمة
     */
    void removeTask(int taskId);

    /**
     * @brief تحديث حالة المهمة
     */
    void toggleTask(int taskId);

    /**
     * @brief تحديث عرض المهام
     */
    void updateTaskDisplay();

    /**
     * @brief مسح المهام المكتملة
     */
    void clearCompletedTasks();

public:
    ArabicTaskManagerApp();
    ~ArabicTaskManagerApp() = default;

    /**
     * @brief تهيئة تطبيق إدارة المهام
     */
    bool initialize(HINSTANCE hInstance);

    /**
     * @brief تشغيل التطبيق
     */
    void run();

    /**
     * @brief إيقاف التطبيق
     */
    void stop();

    /**
     * @brief الحصول على عدد المهام
     */
    size_t getTaskCount() const { return tasks.size(); }

    /**
     * @brief الحصول على عدد المهام المكتملة
     */
    size_t getCompletedTaskCount() const;
};

} // namespace ArabicLanguage
