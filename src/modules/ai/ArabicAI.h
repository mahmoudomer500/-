#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <future>
#include <chrono>

namespace ArabicAI {

// ============================================================================
// التعريفات الأساسية والأنواع
// ============================================================================

enum class AIModelType {
    CODE_COMPLETION,      // إكمال الكود
    CODE_ANALYSIS,        // تحليل الكود
    ERROR_DETECTION,      // كشف الأخطاء
    CODE_GENERATION,      // توليد الكود
    CODE_REVIEW,          // مراجعة الكود
    DOCUMENTATION,        // التوثيق
    TESTING,              // الاختبارات
    TRANSLATION           // الترجمة
};

enum class AIConfidence {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
    VERY_HIGH = 3
};

enum class AIProvider {
    LOCAL_MODEL,          // نموذج محلي
    OPENAI,               // OpenAI API
    GOOGLE_AI,            // Google AI
    MICROSOFT_AI,         // Microsoft AI
    ARABIC_SPECIFIC       // نماذج متخصصة للعربية
};

struct AITask {
    std::string id;
    AIModelType type;
    std::string description;
    std::unordered_map<std::string, std::string> parameters;
    std::chrono::milliseconds timeout;
    bool requiresArabicContext;
};

struct AIResult {
    std::string taskId;
    bool success;
    std::string content;
    AIConfidence confidence;
    std::vector<std::string> suggestions;
    std::unordered_map<std::string, std::string> metadata;
    std::chrono::milliseconds processingTime;
    std::string errorMessage;
};

struct CodeContext {
    std::string filePath;
    std::string language;
    int cursorLine;
    int cursorColumn;
    std::string currentLine;
    std::vector<std::string> surroundingLines;
    std::vector<std::string> importedModules;
    std::string functionContext;
    std::string classContext;
    std::unordered_map<std::string, std::string> variables;
};

struct ArabicCodePattern {
    std::string pattern;
    std::string description;
    std::vector<std::string> examples;
    AIConfidence confidence;
    std::vector<std::string> relatedKeywords;
};

// ============================================================================
// الواجهة الرئيسية للذكاء الاصطناعي العربي
// ============================================================================

class ArabicAI {
public:
    ArabicAI();
    ~ArabicAI();

    // ============================================================================
    // إعدادات النظام
    // ============================================================================

    /**
     * @brief تهيئة نظام الذكاء الاصطناعي
     * @param config ملف التكوين
     * @return نجاح العملية
     */
    bool initialize(const std::string& configPath = "");

    /**
     * @brief تحديث إعدادات النظام
     * @param settings الإعدادات الجديدة
     */
    void updateSettings(const std::unordered_map<std::string, std::string>& settings);

    /**
     * @brief إيقاف نظام الذكاء الاصطناعي
     */
    void shutdown();

    // ============================================================================
    // معالجة المهام الأساسية
    // ============================================================================

    /**
     * @brief تنفيذ مهمة ذكاء اصطناعي
     * @param task المهمة المطلوب تنفيذها
     * @return نتيجة التنفيذ
     */
    std::future<AIResult> executeTask(const AITask& task);

    /**
     * @brief تنفيذ مهام متعددة بشكل متزامن
     * @param tasks قائمة المهام
     * @return قائمة النتائج
     */
    std::vector<std::future<AIResult>> executeTasks(const std::vector<AITask>& tasks);

    /**
     * @brief إلغاء مهمة قيد التنفيذ
     * @param taskId معرف المهمة
     */
    void cancelTask(const std::string& taskId);

    // ============================================================================
    // وظائف متخصصة للكود العربي
    // ============================================================================

    /**
     * @brief إكمال الكود التلقائي
     * @param context السياق الحالي للكود
     * @param prefix النص المكتوب حالياً
     * @return اقتراحات الإكمال
     */
    std::vector<std::string> completeCode(const CodeContext& context, const std::string& prefix);

    /**
     * @brief تحليل الكود وتقديم توصيات
     * @param code الكود المراد تحليله
     * @param context السياق
     * @return تقرير التحليل
     */
    AIResult analyzeCode(const std::string& code, const CodeContext& context);

    /**
     * @brief كشف وإصلاح الأخطاء في الكود
     * @param code الكود المراد فحصه
     * @return قائمة الأخطاء والإصلاحات المقترحة
     */
    std::vector<AIResult> detectAndFixErrors(const std::string& code);

    /**
     * @brief توليد كود من الوصف الطبيعي
     * @param description وصف الوظيفة المطلوبة
     * @param language لغة البرمجة
     * @return الكود المولد
     */
    std::string generateCode(const std::string& description, const std::string& language = "arabic");

    /**
     * @brief ترجمة الكود بين العربية والإنجليزية
     * @param code الكود المراد ترجمته
     * @param fromLanguage اللغة الأصلية
     * @param toLanguage اللغة المستهدفة
     * @return الكود المترجم
     */
    std::string translateCode(const std::string& code, const std::string& fromLanguage, const std::string& toLanguage);

    // ============================================================================
    // التعلم والتكيف
    // ============================================================================

    /**
     * @brief تعلم من ردود الفعل
     * @param taskId معرف المهمة
     * @param feedback رد الفعل (إيجابي/سلبي)
     * @param correction التصحيح إن وجد
     */
    void learnFromFeedback(const std::string& taskId, bool positive, const std::string& correction = "");

    /**
     * @brief تحديث نماذج الذكاء الاصطناعي
     * @param modelType نوع النموذج
     * @param newData البيانات الجديدة
     */
    void updateModel(AIModelType modelType, const std::vector<std::string>& newData);

    /**
     * @brief الحصول على إحصائيات الأداء
     * @return إحصائيات مفصلة
     */
    std::unordered_map<std::string, double> getPerformanceStats();

    // ============================================================================
    // إدارة النماذج والمزودين
    // ============================================================================

    /**
     * @brief إضافة مزود ذكاء اصطناعي جديد
     * @param provider المزود
     * @param config التكوين
     */
    void addProvider(AIProvider provider, const std::unordered_map<std::string, std::string>& config);

    /**
     * @brief تحديد المزود المفضل لنوع مهمة معين
     * @param taskType نوع المهمة
     * @param provider المزود المفضل
     */
    void setPreferredProvider(AIModelType taskType, AIProvider provider);

    /**
     * @brief اختبار اتصال المزود
     * @param provider المزود المراد اختباره
     * @return حالة الاتصال
     */
    bool testProviderConnection(AIProvider provider);

    // ============================================================================
    // وظائف مساعدة
    // ============================================================================

    /**
     * @brief التحقق من صحة الإعدادات
     * @return تقرير التحقق
     */
    std::string validateConfiguration() const;

    /**
     * @brief حفظ حالة النظام
     * @param filePath مسار الملف
     */
    void saveState(const std::string& filePath) const;

    /**
     * @brief تحميل حالة النظام
     * @param filePath مسار الملف
     */
    void loadState(const std::string& filePath);

    // ============================================================================
    // Events و Callbacks
    // ============================================================================

    using TaskCompletedCallback = std::function<void(const AIResult&)>;
    using ErrorCallback = std::function<void(const std::string& taskId, const std::string& error)>;
    using ProgressCallback = std::function<void(const std::string& taskId, double progress)>;

    void setTaskCompletedCallback(TaskCompletedCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setProgressCallback(ProgressCallback callback);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

    // منع النسخ
    ArabicAI(const ArabicAI&) = delete;
    ArabicAI& operator=(const ArabicAI&) = delete;
};

// ============================================================================
// وظائف مساعدة عامة
// ============================================================================

/**
 * @brief إنشاء مهمة ذكاء اصطناعي بسرعة
 * @param type نوع المهمة
 * @param description الوصف
 * @param params المعاملات
 * @return المهمة الجاهزة
 */
AITask createTask(AIModelType type, const std::string& description,
                  const std::unordered_map<std::string, std::string>& params = {});

/**
 * @brief تحويل مستوى الثقة إلى نص
 * @param confidence مستوى الثقة
 * @return النص المقابل
 */
std::string confidenceToString(AIConfidence confidence);

/**
 * @brief تحويل نوع المهمة إلى نص
 * @param type نوع المهمة
 * @return النص المقابل
 */
std::string modelTypeToString(AIModelType type);

/**
 * @brief التحقق من صحة نتيجة الذكاء الاصطناعي
 * @param result النتيجة المراد التحقق منها
 * @return صحة النتيجة
 */
bool validateAIResult(const AIResult& result);

} // namespace ArabicAI

// ============================================================================
// Macros مساعدة للاستخدام السريع
// ============================================================================

#define ARABIC_AI_TASK(type, desc) ArabicAI::createTask(type, desc)
#define ARABIC_AI_CHECK_RESULT(result) ArabicAI::validateAIResult(result)
