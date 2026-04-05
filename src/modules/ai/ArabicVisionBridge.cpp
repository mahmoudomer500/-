#include <iostream>
#include <map>
#include <string>
#include <algorithm>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "ArabicVisionBridge.h"
#include "../../core/ArabicExecutor.h"

namespace ArabicLanguage {

// Global image storage - bypasses the broken assignment mechanism
static std::map<std::string, Image> globalImages;

static int safeStoi(const std::string& s) {
    if (s.empty()) return 0;
    try { return std::stoi(s); } catch (...) { return 0; }
}

// هيكل لتخزين معلومات استدعاء الفأرة
struct MouseCallbackInfo {
    std::shared_ptr<ArabicRuntime> runtime;
    std::string functionName;
    Value userData; // إضافة لتخزين البيانات الممررة
    std::function<void(const std::string&)> outputCallback;
    bool inCallback = false; // لمنع استدعاء متكرر
};

// قاموس لتخزين معلومات الاستدعاء لكل نافذة
static std::map<std::string, MouseCallbackInfo> globalMouseCallbacks;

// دوال تحويل آمنة لتجنب انهيار التطبيق عند استلام قيم فارغة أو نصوص غير قابلة للتحويل
static double safeStod(const std::string& s) {
    if (s.empty()) return 0.0;
    try { return std::stod(s); } catch (...) { return 0.0; }
}

// تغليف لدالة استدعاء الفأرة لربطها بلغة البرمجة العربية
void arabicMouseCallbackWrapper(MouseEvent event, int x, int y, int flags, void* userdata) {
    auto info = static_cast<MouseCallbackInfo*>(userdata);
    if (!info || !info->runtime) return;
    
    // تحقق من عدم وجود استدعاء قيد التنفيذ
    if (info->inCallback) {
        char buf[128];
        sprintf(buf, "[MOUSE] Skipping - already in callback\n");
        OutputDebugStringA(buf);
        return;
    }
    
    info->inCallback = true;
    
    // إضافة debug
    char buf[128];
    sprintf(buf, "[MOUSE Callback] event=%d x=%d y=%d\n", static_cast<int>(event), x, y);
    OutputDebugStringA(buf);
    
    // إذا كانت الدالة غير موجودة، لا نقوم بتنفيذ شيء
    if (!info->runtime->hasFunction(info->functionName)) {
        info->inCallback = false;
        return;
    }
    
    try {
        // استخدام ArabicExecutor لتنفيذ استدعاء الدالة
        // مع ضبط resetRuntime = false للحفاظ على المتغيرات العالمية (Global Variables)
        ArabicExecutor executor(info->runtime);
        if (info->outputCallback) {
            executor.setOutputCallback(info->outputCallback);
        }
        
        // إنشاء أمر استدعاء دالة
        auto callCmd = std::make_shared<Command>(CommandType::FUNCTION_CALL);
        callCmd->function_name = info->functionName;
        
        // تحويل المعاملات إلى تعبيرات
        auto eventVal = std::make_shared<ArabicValue>();
        eventVal->value = std::to_string(static_cast<int>(event));
        eventVal->type = ValueType::NUMBER;
        callCmd->arguments.push_back(eventVal);
        
        auto xVal = std::make_shared<ArabicValue>();
        xVal->value = std::to_string(x);
        xVal->type = ValueType::NUMBER;
        callCmd->arguments.push_back(xVal);
        
        auto yVal = std::make_shared<ArabicValue>();
        yVal->value = std::to_string(y);
        yVal->type = ValueType::NUMBER;
        callCmd->arguments.push_back(yVal);
        
        auto flagsVal = std::make_shared<ArabicValue>();
        flagsVal->value = std::to_string(flags);
        flagsVal->type = ValueType::NUMBER;
        callCmd->arguments.push_back(flagsVal);
        
        auto userDataVal = std::make_shared<ArabicValue>();
        userDataVal->value = info->userData.value;
        userDataVal->type = info->userData.type;
        callCmd->arguments.push_back(userDataVal); 
        
        // تنفيذ الاستدعاء بدون مسح المتغيرات (resetRuntime = false)
        // هذا يضمن أن الدالة يمكنها الوصول وتعديل المتغير 'صورة' المعرف في البرنامج الرئيسي
        std::vector<std::shared_ptr<Command>> commands = { callCmd };
        executor.execute(commands, false);
    } catch (const std::exception& e) {
        // تجنب الطباعة المستمرة في أحداث الحركة لتقليل التشويش
        if (event != MouseEvent::MOVE) {
            std::string errMsg = "❌ خطأ في استدعاء حدث الماوس '" + info->functionName + "': " + e.what() + "\n";
            if (info->outputCallback) info->outputCallback(errMsg);
            else std::cerr << errMsg;
        }
    }
    
    info->inCallback = false;
}

void ArabicVisionBridge::registerFunctions(std::shared_ptr<ArabicRuntime> runtime, 
                                           std::function<void(const std::string&)> outputCallback) {
    if (!runtime) return;
    
    OutputDebugStringA("[VISION] registerFunctions CALLED\n");
    std::cerr << "[VISION] registerFunctions CALLED" << std::endl;
    if (outputCallback) outputCallback("[VISION] Registering vision functions...\n");

    ImageProcessor processor;

    // --- 1. تحميل وحفظ وإنشاء الصور ---

    runtime->defineNativeFunction("حمل_صورة", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) {
            std::cerr << "❌ خطأ: حمل_picture يتطلب مسار الصورة" << std::endl;
            return Value(ValueType::NONE);
        }
        std::string path = args[0].string_value;
        if (path.empty()) {
            std::cerr << "❌ خطأ: مسار الصورة فارغ" << std::endl;
            return Value(ValueType::NONE);
        }
        ImageProcessor ip;
        Image img = ip.loadImage(path);
        return imageToValue(img);
    });

    runtime->defineNativeFunction("انشئ_صورة", [](const std::vector<Value>& args) -> Value {
        int w = 512, h = 512, c = 3;
        
        auto getInt = [](const Value& v) -> int {
            if (v.type == ValueType::NUMBER) return (int)std::stod(v.value);
            return (int)safeStod(v.string_value);
        };
        
        if (!args.empty()) {
            w = getInt(args[0]);
        }
        if (args.size() > 1) {
            h = getInt(args[1]);
        }
        if (args.size() > 2) {
            c = getInt(args[2]);
        }
        
        if (w <= 0 || h <= 0 || c <= 0) {
            std::cerr << "❌ خطأ: أبعاد غير صالحة" << std::endl;
            return Value(ValueType::NONE);
        }
        
        Image img(w, h, c);
        std::fill(img.data.begin(), img.data.end(), 0);
        
        return imageToValue(img);
    });

    runtime->defineNativeFunction("احفظ_صورة", [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) {
            std::cerr << "❌ خطأ: احفظ_صورة يتطلب معاملين (صورة، مسار)" << std::endl;
            return Value(ValueType::NUMBER, "0");
        }
        
        Image img = valueToImage(args[0]);
        
        // التحقق من صحة الصورة
        if (img.isEmpty()) {
            std::cerr << "❌ خطأ: الصورة فارغة أو غير صالحة" << std::endl;
            return Value(ValueType::NUMBER, "0");
        }
        
        std::string path = args[1].string_value;
        
        // التحقق من أن المسار ليس فارغاً
        if (path.empty()) {
            std::cerr << "❌ خطأ: مسار الملف فارغ" << std::endl;
            return Value(ValueType::NUMBER, "0");
        }
        
        ImageProcessor ip;
        // استنتاج التنسيق من المسار
        ImageFormat format = ImageFormat::PNG;
        if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos) {
            format = ImageFormat::JPEG;
        } else if (path.find(".bmp") != std::string::npos) {
            format = ImageFormat::BMP;
        } else if (path.find(".png") == std::string::npos) {
            // إذا لم يكن هناك امتداد معروف، أضف .png
            path += ".png";
        }
        
        try {
            bool success = ip.saveImage(img, path, format);
            if (success) {
                std::cout << "✅ تم حفظ الصورة بنجاح: " << path << std::endl;
            } else {
                std::cerr << "❌ فشل حفظ الصورة في: " << path << std::endl;
            }
            return Value(ValueType::NUMBER, success ? "1" : "0");
        } catch (const std::exception& e) {
            std::cerr << "❌ استثناء أثناء حفظ الصورة: " << e.what() << std::endl;
            return Value(ValueType::NUMBER, "0");
        }
    });

    // --- 2. المعالجة الأساسية ---

    runtime->defineNativeFunction("حول_لرمادي", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(ValueType::NONE);
        Image img = valueToImage(args[0]);
        ImageProcessor ip;
        return imageToValue(ip.toGrayscale(img));
    });

    runtime->defineNativeFunction("عتم_غاوسي", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(ValueType::NONE);
        Image img = valueToImage(args[0]);
        double sigma = args.size() > 1 ? safeStod(args[1].string_value) : 1.0;
        ImageProcessor ip;
        return imageToValue(ip.applyGaussianBlur(img, sigma));
    });

    // --- 3. اكتشاف الحواف ---

    runtime->defineNativeFunction("كاشف_كاني", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(ValueType::NONE);
        Image img = valueToImage(args[0]);
        int low = args.size() > 1 ? (int)safeStod(args[1].string_value) : 50;
        int high = args.size() > 2 ? (int)safeStod(args[2].string_value) : 150;
        ImageProcessor ip;
        return imageToValue(ip.detectEdgesCanny(img, low, high));
    });

    // --- 4. التعرف على الوجوه ---

    runtime->defineNativeFunction("اكتشف_وجوه", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(ValueType::NONE);
        Image img = valueToImage(args[0]);
        FaceDetector fd;
        std::vector<Face> faces = fd.detectFaces(img);
        
        Value result(ValueType::ARRAY);
        for (const auto& f : faces) {
            Value face(ValueType::ARRAY);
            face.elements.push_back(Value(ValueType::NUMBER, std::to_string(f.x)));
            face.elements.push_back(Value(ValueType::NUMBER, std::to_string(f.y)));
            face.elements.push_back(Value(ValueType::NUMBER, std::to_string(f.width)));
            face.elements.push_back(Value(ValueType::NUMBER, std::to_string(f.height)));
            result.elements.push_back(face);
        }
        return result;
    });

    // --- 5. التحويلات الهندسية ---

    runtime->defineNativeFunction("غير_الحجم", [](const std::vector<Value>& args) -> Value {
        if (args.size() < 3) return Value(ValueType::NONE);
        Image img = valueToImage(args[0]);
        if (img.isEmpty()) return Value(ValueType::NONE);
        
        try {
            int w = (int)safeStod(args[1].type == ValueType::NUMBER ? args[1].value : args[1].string_value);
            int h = (int)safeStod(args[2].type == ValueType::NUMBER ? args[2].value : args[2].string_value);
            
            if (w <= 0 || h <= 0 || w > 10000 || h > 10000) {
                std::cerr << "❌ خطأ في الدالة غير_الحجم: أبعاد غير صالحة (" << w << "x" << h << ")\n";
                return Value(ValueType::NONE);
            }
            
            ImageProcessor ip;
            return imageToValue(ip.resize(img, w, h));
        } catch (const std::exception& e) {
            std::cerr << "❌ خطأ في الدالة غير_الحجم: معاملات غير صحيحة لتغيير الحجم - " << e.what() << "\n";
            return Value(ValueType::NONE);
        }
    });

    runtime->defineNativeFunction("دور_الصورة", [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) return Value(ValueType::NONE);
        Image img = valueToImage(args[0]);
        double angle = 0.0;
        if (args[1].type == ValueType::NUMBER) {
            angle = args[1].number_value != 0 ? args[1].number_value : safeStod(args[1].value);
        } else {
            angle = safeStod(args[1].string_value);
        }
        ImageProcessor ip;
        return imageToValue(ip.rotate(img, angle));
    });

    // --- 6. اكتشاف الميزات المتقدم ---

    runtime->defineNativeFunction("اكتشف_زوايا_هيريس", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(ValueType::NONE);
        Image img = valueToImage(args[0]);
        ShapeDetector sd;
        std::vector<Keypoint> corners = sd.detectHarrisCorners(img);
        
        Value result(ValueType::ARRAY);
        for (const auto& kp : corners) {
            Value point(ValueType::ARRAY);
            point.elements.push_back(Value(ValueType::NUMBER, std::to_string(kp.x)));
            point.elements.push_back(Value(ValueType::NUMBER, std::to_string(kp.y)));
            result.elements.push_back(point);
        }
        return result;
    });

    runtime->defineNativeFunction("اكتشف_ميزات_ORB", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(ValueType::NONE);
        Image img = valueToImage(args[0]);
        ShapeDetector sd;
        std::vector<Keypoint> kps = sd.detectORB(img);
        
        Value result(ValueType::ARRAY);
        for (const auto& kp : kps) {
            Value point(ValueType::ARRAY);
            point.elements.push_back(Value(ValueType::NUMBER, std::to_string(kp.x)));
            point.elements.push_back(Value(ValueType::NUMBER, std::to_string(kp.y)));
            result.elements.push_back(point);
        }
        return result;
    });

    // --- 7. الرؤية المجسمة والمعايرة ---

    runtime->defineNativeFunction("احسب_العمق", [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) return Value(ValueType::NONE);
        Image left = valueToImage(args[0]);
        Image right = valueToImage(args[1]);
        ImageProcessor ip;
        return imageToValue(ip.computeStereoDisparity(left, right));
    });

    // --- 8. التعرف الضوئي (OCR) ---

    runtime->defineNativeFunction("اقرأ_النص_العربي", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(ValueType::NONE);
        Image img = valueToImage(args[0]);
        ArabicOCR ocr;
        std::string text = ocr.recognizeText(img);
        return Value(ValueType::STRING, text);
    });

    // --- 9. التجزئة المتقدمة ---

    runtime->defineNativeFunction("تجزئة_واترشد", [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) return Value(ValueType::NONE);
        Image img = valueToImage(args[0]);
        Image markers = valueToImage(args[1]);
        ImageProcessor ip;
        ip.watershed(img, markers);
        return imageToValue(markers);
    });

    // --- 10. اكتشاف اليد والأصابع ---

    runtime->defineNativeFunction("اكتشف_أصابع", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(ValueType::NONE);
        Image img = valueToImage(args[0]);
        HandDetector hd;
        std::vector<std::pair<int, int>> fingers = hd.detectFingers(img);
        
        Value result(ValueType::ARRAY);
        for (const auto& f : fingers) {
            Value point(ValueType::ARRAY);
            point.elements.push_back(Value(ValueType::NUMBER, std::to_string(f.first)));
            point.elements.push_back(Value(ValueType::NUMBER, std::to_string(f.second)));
            result.elements.push_back(point);
        }
        return result;
    });

    // --- 11. التحكم في الكاميرا ---

    static Camera globalCamera;

    runtime->defineNativeFunction("افتح_كاميرا", [](const std::vector<Value>& args) -> Value {
        try {
            int index = args.empty() ? 0 : (int)safeStod(args[0].type == ValueType::NUMBER ? args[0].value : args[0].string_value);
            bool success = globalCamera.open(index);
            return Value(ValueType::NUMBER, success ? "1" : "0");
        } catch (...) {
            bool success = globalCamera.open(0);
            return Value(ValueType::NUMBER, success ? "1" : "0");
        }
    });

    runtime->defineNativeFunction("التقط_صورة", [](const std::vector<Value>& args) -> Value {
        if (!globalCamera.isOpened()) {
            return Value(ValueType::NONE);
        }
        Image img = globalCamera.capture();
        return imageToValue(img);
    });

    runtime->defineNativeFunction("عرض_صورة", [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) return Value(ValueType::NONE);
        std::string windowName = args[0].string_value;
        
        char buf[512];
        sprintf(buf, "[DEBUG] عرض_صورة: window='%s', args[1].type=%d, args[1].object_name='%s'\n", 
                windowName.c_str(), (int)args[1].type, args[1].object_name.c_str());
        OutputDebugStringA(buf);
        
        Image img = valueToImage(args[1]);
        
        sprintf(buf, "[DEBUG] عرض_صورة after valueToImage: %dx%dx%d\n", img.width, img.height, img.channels);
        OutputDebugStringA(buf);
        
        WindowManager::showImage(windowName, img);
        return Value(ValueType::NONE);
    });

    runtime->defineNativeFunction("انشئ_نافذة", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(ValueType::NONE);
        std::string windowName = args[0].string_value;
        WindowManager::createWindow(windowName);
        return Value(ValueType::NONE);
    });

    runtime->defineNativeFunction("انتظر_مفتاح", [](const std::vector<Value>& args) -> Value {
        try {
            int delay = args.empty() ? 0 : (int)safeStod(args[0].type == ValueType::NUMBER ? args[0].value : args[0].string_value);
            
            char buf[128];
            sprintf(buf, "[DEBUG] انتظر_مفتاح delay=%d\n", delay);
            OutputDebugStringA(buf);
            
            int key = WindowManager::waitKey(delay);
            
            sprintf(buf, "[DEBUG] انتظر_مفتاح returned key=%d\n", key);
            OutputDebugStringA(buf);
            
            return Value(ValueType::NUMBER, std::to_string(key));
        } catch (...) {
            int key = WindowManager::waitKey(0);
            return Value(ValueType::NUMBER, std::to_string(key));
        }
    });

    runtime->defineNativeFunction("أغلق_كاميرا", [](const std::vector<Value>& args) -> Value {
        globalCamera.close();
        return Value(ValueType::NUMBER, "1");
    });

    runtime->defineNativeFunction("أغلق_كل_النوافذ", [](const std::vector<Value>& args) -> Value {
        WindowManager::destroyAllWindows();
        return Value(ValueType::NONE);
    });

    // --- 12. وظائف الرسم والتفاعل الجديدة ---

    auto getInt = [](const Value& v) -> int {
        if (v.type == ValueType::NUMBER) {
            if (!v.value.empty()) return (int)std::stod(v.value);
            if (v.number_value != 0) return (int)v.number_value;
            return 0;
        }
        return (int)safeStod(v.string_value);
    };

    runtime->defineNativeFunction("ارسم_دائرة", [getInt](const std::vector<Value>& args) -> Value {
        if (args.size() < 5) {
            std::cerr << "❌ خطأ: ارسم_دائرة يتطلب 5 معاملات على الأقل" << std::endl;
            return Value(ValueType::NONE);
        }
        Image img = valueToImage(args[0]);
        if (img.isEmpty()) {
            std::cerr << "❌ خطأ: الصورة فارغة" << std::endl;
            return Value(ValueType::NONE);
        }
        int x = getInt(args[1]);
        int y = getInt(args[2]);
        int r = getInt(args[3]);

        Color color(255, 255, 255);
        if (args[4].type == ValueType::ARRAY && args[4].elements.size() >= 3) {
            color.r = (uint8_t)getInt(args[4].elements[0]);
            color.g = (uint8_t)getInt(args[4].elements[1]);
            color.b = (uint8_t)getInt(args[4].elements[2]);
        }

        int thickness = 1;
        bool filled = false;
        if (args.size() > 5) {
            thickness = getInt(args[5]);
            if (thickness < 0) {
                filled = true;
                thickness = 1;
            }
        }

        char dbg[256];
        sprintf(dbg, "[VISION] drawCircle: center=(%d,%d) r=%d color=(%d,%d,%d) thickness=%d filled=%d args.size=%zu args[5].type=%d\n",
                x, y, r, color.r, color.g, color.b, thickness, filled, args.size(),
                args.size() > 5 ? (int)args[5].type : -1);
        OutputDebugStringA(dbg);
        std::cerr << dbg;

        ImageProcessor ip;
        return imageToValue(ip.drawCircle(img, x, y, r, color, thickness, filled));
    });

    runtime->defineNativeFunction("ارسم_مستطيل", [getInt](const std::vector<Value>& args) -> Value {
        if (args.size() < 5) {
            std::cerr << "❌ خطأ: ارسم_مستطيل يتطلب 5 معاملات على الأقل" << std::endl;
            return Value(ValueType::NONE);
        }
        Image img = valueToImage(args[0]);
        if (img.isEmpty()) {
            std::cerr << "❌ خطأ: الصورة فارغة" << std::endl;
            return Value(ValueType::NONE);
        }
        int x = getInt(args[1]);
        int y = getInt(args[2]);
        int w = getInt(args[3]);
        int h = getInt(args[4]);

        Color color(255, 255, 255);
        if (args.size() > 5 && args[5].type == ValueType::ARRAY && args[5].elements.size() >= 3) {
            color.r = (uint8_t)getInt(args[5].elements[0]);
            color.g = (uint8_t)getInt(args[5].elements[1]);
            color.b = (uint8_t)getInt(args[5].elements[2]);
        }

        int thickness = args.size() > 6 ? getInt(args[6]) : 1;
        bool filled = false;
        if (thickness < 0) {
            filled = true;
            thickness = 1;
        }

        ImageProcessor ip;
        return imageToValue(ip.drawRectangle(img, x, y, w, h, color, thickness, filled));
    });

    runtime->defineNativeFunction("لون", [getInt](const std::vector<Value>& args) -> Value {
        Value colorArray(ValueType::ARRAY);
        int r = 0, g = 0, b = 0;
        if (args.size() >= 1) r = getInt(args[0]);
        if (args.size() >= 2) g = getInt(args[1]);
        if (args.size() >= 3) b = getInt(args[2]);
        
        colorArray.elements.push_back(Value(ValueType::NUMBER, std::to_string(r)));
        colorArray.elements.push_back(Value(ValueType::NUMBER, std::to_string(g)));
        colorArray.elements.push_back(Value(ValueType::NUMBER, std::to_string(b)));
        return colorArray;
    });

    runtime->defineNativeFunction("حدد_حدث_الماوس", [runtime, outputCallback](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) {
            std::cerr << "❌ خطأ: حدد_حدث_الماوس يتطلب معاملين (اسم النافذة، اسم الدالة)" << std::endl;
            return Value(ValueType::NONE);
        }
        std::string windowName = args[0].string_value;
        std::string functionName = args[1].string_value;
        
        if (windowName.empty() || functionName.empty()) {
            std::cerr << "❌ خطأ: اسم النافذة أو الدالة فارغ" << std::endl;
            return Value(ValueType::NONE);
        }
        
        // دعم تمرير بيانات اختيارية كمعامل ثالث
        Value userData = (args.size() >= 3) ? args[2] : Value(ValueType::NONE);

        // تخزين معلومات الاستدعاء مع ضمان بقاء الـ runtime حياً
        globalMouseCallbacks[windowName] = { runtime, functionName, userData, outputCallback };
        
        // إضافة debug
        std::cout << "🔧 حدد_حدث_الماوس: window=" << windowName << ", function=" << functionName << std::endl;
        
        // إعداد استدعاء الماوس في مدير النوافذ
        WindowManager::setMouseCallback(windowName, arabicMouseCallbackWrapper, &globalMouseCallbacks[windowName]);
        
        return Value(ValueType::NONE);
    });

    // تسجيل ثوابت الأحداث لسهولة الاستخدام
    runtime->defineVariable("حدث_ماوس_حركة", Value(ValueType::NUMBER, std::to_string(static_cast<int>(MouseEvent::MOVE))));
    runtime->defineVariable("حدث_ماوس_يسار_سفل", Value(ValueType::NUMBER, std::to_string(static_cast<int>(MouseEvent::LEFT_DOWN))));
    runtime->defineVariable("حدث_ماوس_يمين_سفل", Value(ValueType::NUMBER, std::to_string(static_cast<int>(MouseEvent::RIGHT_DOWN))));
    runtime->defineVariable("حدث_ماوس_يسار_نقر_مزدوج", Value(ValueType::NUMBER, std::to_string(static_cast<int>(MouseEvent::LEFT_DBLCLK))));
    runtime->defineVariable("حدث_ماوس_يمين_نقر_مزدوج", Value(ValueType::NUMBER, std::to_string(static_cast<int>(MouseEvent::RIGHT_DBLCLK))));

    std::cout << "✅ تم ربط مكتبة الرؤية الحاسوبية (ArabicComputerVision) بنجاح\n";
}

Image ArabicVisionBridge::valueToImage(const Value& val) {
    // القيم الفارغة أو غير OBJECT ترجع صورة فارغة
    if (val.type == ValueType::NONE) return Image();
    if (val.type != ValueType::OBJECT) {
        std::cerr << "⚠️ valueToImage: نوع غير متوقع (" << (int)val.type << "), object_name='" << val.object_name << "'" << std::endl;
        return Image();
    }

    // تحذير فقط إذا لم يكن اسم الكائن صورة — لكن نكمل المحاولة
    if (val.object_name != "صورة") {
        std::cerr << "⚠️ valueToImage: object_name='" << val.object_name << "' (متوقع 'صورة')" << std::endl;
    }

    try {
        // دالة مساعدة: تقرأ الرقم من value أو string_value أو number_value
        auto readInt = [](const Value& v) -> int {
            if (!v.string_value.empty()) return safeStoi(v.string_value);
            if (!v.value.empty())        return safeStoi(v.value);
            if (v.number_value != 0)     return (int)v.number_value;
            return 0;
        };

        // البحث عن الأبعاد
        auto wIt = val.map_elements.find("عرض");
        auto hIt = val.map_elements.find("ارتفاع");
        auto cIt = val.map_elements.find("قنوات");

        if (wIt == val.map_elements.end() || hIt == val.map_elements.end() || cIt == val.map_elements.end()) {
            std::cerr << "⚠️ valueToImage: مفاتيح الأبعاد مفقودة، map_elements.size=" << val.map_elements.size() << std::endl;
            return Image();
        }

        int w = readInt(wIt->second);
        int h = readInt(hIt->second);
        int c = readInt(cIt->second);

        if (w <= 0 || h <= 0 || c <= 0 || w > 10000 || h > 10000 || c > 4) {
            std::cerr << "⚠️ valueToImage: أبعاد غير صالحة (" << w << "x" << h << "x" << c << ")" << std::endl;
            return Image();
        }

        Image img(w, h, c);
        size_t expected = img.data.size();

        // بيانات الصورة: blob_value أولاً ثم string_value
        const std::vector<uint8_t>* srcData = nullptr;
        std::vector<uint8_t> fromString;

        if (!val.blob_value.empty()) {
            srcData = &val.blob_value;
        } else if (!val.string_value.empty()) {
            fromString.assign(val.string_value.begin(), val.string_value.end());
            srcData = &fromString;
        }

        if (srcData && !srcData->empty()) {
            size_t copySize = std::min(srcData->size(), expected);
            if (copySize != expected) {
                std::cerr << "⚠️ valueToImage: حجم بيانات غير متطابق (متوقع " << expected << "، فعلي " << srcData->size() << ")" << std::endl;
            }
            std::copy(srcData->begin(), srcData->begin() + copySize, img.data.begin());
        }
        // إذا لم توجد بيانات، الصورة ستبقى سوداء (مُهيأة بأصفار) — لا تُرجع فارغة

        return img;
    } catch (const std::exception& e) {
        std::cerr << "❌ valueToImage exception: " << e.what() << std::endl;
        return Image();
    }
}

Value ArabicVisionBridge::imageToValue(const Image& img) {
    Value val(ValueType::OBJECT);
    val.object_name = "صورة";
    
    // استخدام string_value لتخزين القيم الرقمية لضمان قراءتها بشكل صحيح في valueToImage
    val.map_elements["عرض"] = Value(ValueType::NUMBER, std::to_string(img.width));
    val.map_elements["عرض"].string_value = std::to_string(img.width);
    
    val.map_elements["ارتفاع"] = Value(ValueType::NUMBER, std::to_string(img.height));
    val.map_elements["ارتفاع"].string_value = std::to_string(img.height);
    
    val.map_elements["قنوات"] = Value(ValueType::NUMBER, std::to_string(img.channels));
    val.map_elements["قنوات"].string_value = std::to_string(img.channels);
    
    // تخزين بيانات الصورة في blob_value لضمان سلامة البيانات الثنائية
    val.blob_value = img.data;
    // تخزين نسخة احتياطية في string_value للتوافق مع النسخ القديمة
    val.string_value = std::string(img.data.begin(), img.data.end());

    return val;
}

} // namespace ArabicLanguage
