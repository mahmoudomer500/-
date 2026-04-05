#ifndef ARABIC_VISION_BRIDGE_H
#define ARABIC_VISION_BRIDGE_H

#include "ArabicRuntime.h"
#include "ArabicComputerVision.h"

namespace ArabicLanguage {

/**
 * @brief فئة مسؤولة عن ربط مكتبة الرؤية الحاسوبية بلغة البرمجة العربية
 */
class ArabicVisionBridge {
public:
    /**
     * @brief تسجيل جميع دوال الرؤية الحاسوبية في بيئة التشغيل
     * @param runtime بيئة التشغيل المراد التسجيل فيها
     * @param outputCallback دالة لاستقبال المخرجات النصية (اختياري)
     */
    static void registerFunctions(std::shared_ptr<ArabicRuntime> runtime, 
                                  std::function<void(const std::string&)> outputCallback = nullptr);

private:
    // دوال مساعدة لتحويل القيم بين لغة البرمجة والمكتبة
    static Image valueToImage(const Value& val);
    static Value imageToValue(const Image& img);
};

} // namespace ArabicLanguage

#endif // ARABIC_VISION_BRIDGE_H
