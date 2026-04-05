#pragma once

#include "ArabicDocGenerator.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <functional>

// Forward declarations
class ArabicCodeGen;

/**
 * @brief موثق واجهات برمجة التطبيقات العربية - Arabic API Documenter
 *
 * يقدم هذا الفئة نظام توثيق متخصص لواجهات برمجة التطبيقات
 * مع استخراج تلقائي للمعلومات وتوليد توثيق تفاعلي
 */
class ArabicAPIDocumenter {
public:
    // ============================================================================
    // أنواع البيانات والتعدادات
    // ============================================================================

    /**
     * @brief معلومات API
     */
    struct APIInfo {
        std::string name;                   // اسم API
        std::string version;                // رقم الإصدار
        std::string description;            // الوصف
        std::string baseUrl;                // الرابط الأساسي
        std::string contactEmail;           // البريد الإلكتروني للتواصل
        std::vector<std::string> tags;      // العلامات
        std::unordered_map<std::string, std::string> metadata; // بيانات إضافية
    };

    /**
     * @brief معلومات نقطة نهاية API
     */
    struct EndpointInfo {
        std::string path;                   // المسار
        std::string method;                 // طريقة HTTP (GET, POST, etc.)
        std::string summary;                // ملخص
        std::string description;            // الوصف
        std::vector<std::string> tags;      // العلامات
        std::vector<ParameterInfo> parameters; // المعاملات
        ResponseInfo response;              // الاستجابة
        std::vector<std::string> examples;  // الأمثلة
        bool deprecated;                    // مهجور
        std::string operationId;            // معرف العملية
    };

    /**
     * @brief معلومات معامل API
     */
    struct ParameterInfo {
        std::string name;                   // اسم المعامل
        std::string in;                     // الموقع (query, path, header, body)
        std::string description;            // الوصف
        bool required;                      // مطلوب
        std::string type;                   // النوع
        std::string format;                 // التنسيق
        std::unordered_map<std::string, std::string> schema; // المخطط
        std::string example;                // مثال
    };

    /**
     * @brief معلومات استجابة API
     */
    struct ResponseInfo {
        int statusCode;                     // رمز الحالة
        std::string description;            // الوصف
        std::unordered_map<std::string, std::string> headers; // الترويسات
        std::unordered_map<std::string, std::string> content; // المحتوى
        std::string example;                // مثال
    };

    /**
     * @brief معلومات مخطط البيانات
     */
    struct SchemaInfo {
        std::string name;                   // اسم المخطط
        std::string type;                   // النوع الأساسي
        std::string description;            // الوصف
        std::unordered_map<std::string, PropertyInfo> properties; // الخصائص
        std::vector<std::string> required;  // الخصائص المطلوبة
        std::string example;                // مثال
    };

    /**
     * @brief معلومات خاصية
     */
    struct PropertyInfo {
        std::string type;                   // النوع
        std::string description;            // الوصف
        bool required;                      // مطلوب
        std::string format;                 // التنسيق
        std::string example;                // مثال
        std::unordered_map<std::string, std::string> items; // للمصفوفات
    };

    /**
     * @brief تنسيق توثيق API
     */
    enum class APIFormat {
        OPENAPI_3_0,    // OpenAPI 3.0
        SWAGGER_2_0,    // Swagger 2.0
        POSTMAN,        // Postman Collection
        RAML,           // RAML
        API_BLUEPRINT,  // API Blueprint
        CUSTOM          // مخصص
    };

    /**
     * @brief خيارات توثيق API
     */
    struct APIDocOptions {
        APIFormat format = APIFormat::OPENAPI_3_0;
        std::string outputDirectory = "api_docs";
        bool includeExamples = true;
        bool generateInteractiveDocs = true;
        bool includeAuthentication = true;
        bool generateSDK = false;
        std::string language = "ar";
        std::string theme = "default";
        bool compressOutput = false;
        std::vector<std::string> excludePatterns;
        std::unordered_map<std::string, std::string> customHeaders;
    };

    /**
     * @brief نتيجة توثيق API
     */
    struct APIDocResult {
        bool success;
        std::string outputPath;
        std::string specFile;               // ملف المواصفات
        std::string interactiveDocsUrl;     // رابط التوثيق التفاعلي
        int endpointsDocumented;
        int schemasDefined;
        std::chrono::milliseconds generationTime;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };

    /**
     * @brief إحصائيات توثيق API
     */
    struct APIDocStats {
        int totalEndpoints;
        int documentedEndpoints;
        int totalSchemas;
        int documentedSchemas;
        std::unordered_map<std::string, int> methodCount; // GET, POST, etc.
        std::unordered_map<std::string, int> responseCodeCount; // 200, 404, etc.
        std::chrono::milliseconds totalProcessingTime;
        double documentationCoverage;
        size_t specSize;
    };

    // ============================================================================
    // المنشئ والمدمر
    // ============================================================================

    /**
     * @brief المنشئ
     * @param docGenerator مؤشر لمولد التوثيق العام
     */
    ArabicAPIDocumenter(ArabicDocGenerator* docGenerator = nullptr);

    /**
     * @brief المدمر
     */
    ~ArabicAPIDocumenter();

    // ============================================================================
    // استخراج معلومات API
    // ============================================================================

    /**
     * @brief استخراج API من ملفات الكود
     * @param sourceFiles قائمة ملفات المصدر
     * @return معلومات API المستخرجة
     */
    std::unique_ptr<APIInfo> extractAPIFromCode(const std::vector<std::string>& sourceFiles);

    /**
     * @brief استخراج نقطة نهاية من دالة
     * @param functionCode كود الدالة
     * @param functionName اسم الدالة
     * @return معلومات نقطة النهاية
     */
    std::unique_ptr<EndpointInfo> extractEndpointFromFunction(const std::string& functionCode,
                                                            const std::string& functionName);

    /**
     * @brief استخراج مخطط من صنف
     * @param classCode كود الصنف
     * @param className اسم الصنف
     * @return معلومات المخطط
     */
    std::unique_ptr<SchemaInfo> extractSchemaFromClass(const std::string& classCode,
                                                     const std::string& className);

    /**
     * @brief تحليل تعليقات API
     * @param comments تعليقات التوثيق
     * @return معلومات API المستخرجة
     */
    std::unordered_map<std::string, std::string> parseAPIComments(
        const std::vector<std::string>& comments);

    // ============================================================================
    // توليد توثيق API
    // ============================================================================

    /**
     * @brief توليد مواصفات API
     * @param apiInfo معلومات API
     * @param endpoints قائمة نقاط النهاية
     * @param schemas قائمة المخططات
     * @param options خيارات التوليد
     * @return نتيجة التوليد
     */
    APIDocResult generateAPISpecification(const APIInfo& apiInfo,
                                        const std::vector<EndpointInfo>& endpoints,
                                        const std::vector<SchemaInfo>& schemas,
                                        const APIDocOptions& options);

    /**
     * @brief توليد توثيق تفاعلي
     * @param specPath مسار ملف المواصفات
     * @param outputDir مجلد الإخراج
     * @return رابط التوثيق التفاعلي
     */
    std::string generateInteractiveDocs(const std::string& specPath,
                                      const std::string& outputDir);

    /**
     * @brief توليد SDK
     * @param apiInfo معلومات API
     * @param language لغة البرمجة المستهدفة
     * @param outputDir مجلد الإخراج
     * @return مسار SDK المولد
     */
    std::string generateSDK(const APIInfo& apiInfo,
                          const std::string& language,
                          const std::string& outputDir);

    /**
     * @brief التحقق من صحة مواصفات API
     * @param specPath مسار ملف المواصفات
     * @return قائمة الأخطاء المكتشفة
     */
    std::vector<std::string> validateAPISpecification(const std::string& specPath);

    // ============================================================================
    // إدارة المخططات والنماذج
    // ============================================================================

    /**
     * @brief إضافة مخطط مخصص
     * @param schema معلومات المخطط
     * @return true في حالة النجاح
     */
    bool addCustomSchema(const SchemaInfo& schema);

    /**
     * @brief الحصول على مخطط
     * @param schemaName اسم المخطط
     * @return معلومات المخطط أو nullptr إذا لم يوجد
     */
    const SchemaInfo* getSchema(const std::string& schemaName) const;

    /**
     * @brief إنشاء مخطط من JSON Schema
     * @param jsonSchema النص JSON
     * @param name اسم المخطط
     * @return معلومات المخطط
     */
    std::unique_ptr<SchemaInfo> createSchemaFromJSON(const std::string& jsonSchema,
                                                   const std::string& name);

    /**
     * @brief تحويل مخطط إلى JSON Schema
     * @param schema معلومات المخطط
     * @return النص JSON
     */
    std::string schemaToJSON(const SchemaInfo& schema);

    /**
     * @brief دمج مخططين
     * @param base المخطط الأساسي
     * @param extension المخطط الامتداد
     * @return المخطط المدمج
     */
    std::unique_ptr<SchemaInfo> mergeSchemas(const SchemaInfo& base, const SchemaInfo& extension);

    // ============================================================================
    // الأمثلة والاختبارات
    // ============================================================================

    /**
     * @brief توليد أمثلة لطلبات API
     * @param endpoint نقطة النهاية
     * @param format تنسيق المثال (curl, javascript, python, etc.)
     * @return كود المثال
     */
    std::string generateAPIExample(const EndpointInfo& endpoint, const std::string& format);

    /**
     * @brief إنشاء اختبار API تلقائي
     * @param endpoint نقطة النهاية
     * @param testType نوع الاختبار
     * @return كود الاختبار
     */
    std::string generateAPITest(const EndpointInfo& endpoint, const std::string& testType = "unit");

    /**
     * @brief التحقق من تناسق API
     * @param endpoints قائمة نقاط النهاية
     * @return تقرير التناسق
     */
    std::string checkAPIConsistency(const std::vector<EndpointInfo>& endpoints);

    // ============================================================================
    // التصدير والاستيراد
    // ============================================================================

    /**
     * @brief تصدير API إلى تنسيق آخر
     * @param sourceSpec مسار المواصفات المصدر
     * @param targetFormat التنسيق المستهدف
     * @param outputPath مسار الإخراج
     * @return true في حالة النجاح
     */
    bool exportAPIToFormat(const std::string& sourceSpec,
                         APIFormat targetFormat,
                         const std::string& outputPath);

    /**
     * @brief استيراد API من ملف خارجي
     * @param specPath مسار ملف المواصفات
     * @param format تنسيق الملف
     * @return معلومات API المستوردة
     */
    std::unique_ptr<APIInfo> importAPIFromFile(const std::string& specPath, APIFormat format);

    /**
     * @brief دمج APIs متعددة
     * @param apis قائمة APIs المراد دمجها
     * @return API المدمج
     */
    std::unique_ptr<APIInfo> mergeAPIs(const std::vector<APIInfo>& apis);

    // ============================================================================
    // البحث والاستعلام
    // ============================================================================

    /**
     * @brief البحث في نقاط النهاية
     * @param query استعلام البحث
     * @param endpoints قائمة نقاط النهاية
     * @return قائمة النتائج
     */
    std::vector<const EndpointInfo*> searchEndpoints(const std::string& query,
                                                   const std::vector<EndpointInfo>& endpoints);

    /**
     * @brief الحصول على نقاط النهاية حسب العلامة
     * @param tag العلامة
     * @param endpoints قائمة نقاط النهاية
     * @return قائمة نقاط النهاية
     */
    std::vector<const EndpointInfo*> getEndpointsByTag(const std::string& tag,
                                                     const std::vector<EndpointInfo>& endpoints);

    /**
     * @brief إنشاء فهرس للبحث في API
     * @param apiInfo معلومات API
     * @param endpoints نقاط النهاية
     * @return فهرس البحث
     */
    std::unordered_map<std::string, std::vector<std::string>> createAPISearchIndex(
        const APIInfo& apiInfo, const std::vector<EndpointInfo>& endpoints);

    // ============================================================================
    // الإحصائيات والمراقبة
    // ============================================================================

    /**
     * @brief الحصول على إحصائيات توثيق API
     * @return الإحصائيات
     */
    APIDocStats getStatistics() const;

    /**
     * @brief تصدير الإحصائيات
     * @param filePath مسار ملف التصدير
     * @return true في حالة النجاح
     */
    bool exportStatistics(const std::string& filePath) const;

    /**
     * @brief إعادة تعيين الإحصائيات
     */
    void resetStatistics();

    // ============================================================================
    // الأحداث والإشعارات
    // ============================================================================

    /**
     * @brief تسجيل مستمع أحداث API
     * @param listener دالة المستمع
     */
    void registerAPIEventListener(std::function<void(const std::string& eventType,
                                                   const std::string& message,
                                                   const std::string& details)> listener);

    /**
     * @brief إلغاء تسجيل مستمع الأحداث
     * @param listener دالة المستمع
     */
    void unregisterAPIEventListener(std::function<void(const std::string& eventType,
                                                     const std::string& message,
                                                     const std::string& details)> listener);

    // ============================================================================
    // الوظائف المساعدة
    // ============================================================================

    /**
     * @brief التحقق من صحة مسار API
     * @param path المسار
     * @return true إذا كان صحيحاً
     */
    static bool isValidAPIPath(const std::string& path);

    /**
     * @brief التحقق من صحة طريقة HTTP
     * @param method الطريقة
     * @return true إذا كانت صحيحة
     */
    static bool isValidHTTPMethod(const std::string& method);

    /**
     * @brief تحويل تنسيق API إلى نص
     * @param format التنسيق
     * @return الاسم النصي
     */
    static std::string apiFormatToString(APIFormat format);

    /**
     * @brief حساب تغطية توثيق API
     * @param documented عدد نقاط النهاية الموثقة
     * @param total العدد الإجمالي
     * @return نسبة التغطية
     */
    static double calculateAPIDocumentationCoverage(int documented, int total);

private:
    // ============================================================================
    // البيانات الخاصة
    // ============================================================================

    ArabicDocGenerator* docGenerator_;
    std::unordered_map<std::string, SchemaInfo> customSchemas_;
    std::vector<std::function<void(const std::string&, const std::string&, const std::string&)>> eventListeners_;

    APIDocStats stats_;

    // ============================================================================
    // الوظائف الخاصة
    // ============================================================================

    /**
     * @brief تحليل نقطة نهاية من تعليق توثيق
     */
    std::unique_ptr<EndpointInfo> parseEndpointFromComment(const std::string& comment,
                                                         const std::string& functionName);

    /**
     * @brief تحليل معامل من تعليق توثيق
     */
    std::unique_ptr<ParameterInfo> parseParameterFromComment(const std::string& comment);

    /**
     * @brief تحليل استجابة من تعليق توثيق
     */
    std::unique_ptr<ResponseInfo> parseResponseFromComment(const std::string& comment);

    /**
     * @brief توليد OpenAPI JSON
     */
    std::string generateOpenAPIJSON(const APIInfo& apiInfo,
                                   const std::vector<EndpointInfo>& endpoints,
                                   const std::vector<SchemaInfo>& schemas);

    /**
     * @brief توليد Swagger JSON
     */
    std::string generateSwaggerJSON(const APIInfo& apiInfo,
                                  const std::vector<EndpointInfo>& endpoints,
                                  const std::vector<SchemaInfo>& schemas);

    /**
     * @brief توليد Postman Collection
     */
    std::string generatePostmanCollection(const APIInfo& apiInfo,
                                        const std::vector<EndpointInfo>& endpoints);

    /**
     * @brief إنشاء خادم توثيق تفاعلي
     */
    bool createInteractiveDocsServer(const std::string& specPath, const std::string& outputDir);

    /**
     * @brief توليد عميل API للغة محددة
     */
    std::string generateAPIClient(const APIInfo& apiInfo,
                                const std::vector<EndpointInfo>& endpoints,
                                const std::string& language);

    /**
     * @brief إرسال حدث للمستمعين
     */
    void notifyEventListeners(const std::string& eventType,
                            const std::string& message,
                            const std::string& details = "");

    /**
     * @brief تحديث الإحصائيات
     */
    void updateStats(const EndpointInfo& endpoint, bool documented);

    /**
     * @brief استخراج نوع البيانات من كود C++
     */
    std::string extractTypeFromCode(const std::string& code);

    /**
     * @brief تنظيف مسار API
     */
    std::string normalizeAPIPath(const std::string& path);

    /**
     * @brief التحقق من تناسق API
     */
    std::vector<std::string> validateAPIConsistency(const std::vector<EndpointInfo>& endpoints);
};

// ============================================================================
// دوال مساعدة
// ============================================================================

/**
 * @brief إنشاء موثق API عربي جديد
 */
std::unique_ptr<ArabicAPIDocumenter> createArabicAPIDocumenter(ArabicDocGenerator* docGenerator = nullptr);

/**
 * @brief استخراج نقاط النهاية من ملف كود
 */
std::vector<ArabicAPIDocumenter::EndpointInfo> extractEndpointsFromFile(const std::string& filePath);

/**
 * @brief التحقق من صحة مواصفات OpenAPI
 */
bool validateOpenAPISpec(const std::string& specPath);

/**
 * @brief تحويل OpenAPI إلى Postman
 */
bool convertOpenAPIToPostman(const std::string& openApiPath, const std::string& postmanPath);