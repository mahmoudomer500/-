// ArabicMobileUI.h - مكتبة واجهات المستخدم للهواتف والأجهزة المحمولة
// الأسبوع الأول من الشهر السابع: مكتبة Android

#ifndef ARABIC_MOBILE_UI_H
#define ARABIC_MOBILE_UI_H

#include "ArabicUI.h"

namespace ArabicLanguage {

/**
 * @brief أنواع أحداث اللمس
 */
enum class TouchType {
    DOWN,
    UP,
    MOVE,
    CANCEL,
    TAP,
    DOUBLE_TAP,
    LONG_PRESS,
    FLING,
    SCROLL,
    PINCH
};

/**
 * @brief حدث لمس متعدد
 */
struct TouchEvent {
    TouchType type;
    int pointerId;
    float x, y;
    float pressure;
    long timestamp;
};

/**
 * @brief عنصر واجهة مستخدم محسن للهواتف
 */
class MobileElement : public ArabicUI::UIElement {
public:
    virtual void onTouch(const TouchEvent& event) = 0;
    virtual void onGesture(TouchType gestureType, float velocityX, float velocityY) {}
};

/**
 * @brief زر محسن للمس (كبير المساحة وسهل النقر)
 */
class MobileButton : public MobileElement {
public:
    MobileButton(const std::string& id, const std::string& text, float x, float y, float w, float h);
    
    void render(ArabicGraphics& graphics) override;
    bool handleEvent(const ArabicUI::UIEvent& event) override;
    void onTouch(const TouchEvent& event) override;
    
    ArabicUI::UIElementType getType() const override { return ArabicUI::UIElementType::BUTTON; }

    void setOnClick(std::function<void()> callback) { _onClick = callback; }

private:
    std::string _text;
    std::function<void()> _onClick;
    bool _isPressed = false;
};

/**
 * @brief حاوية تخطيط مرنة (Flex Layout) للهواتف
 */
class MobileLayout : public MobileElement {
public:
    enum class Orientation { VERTICAL, HORIZONTAL };
    
    MobileLayout(Orientation orientation = Orientation::VERTICAL);
    
    void addElement(std::shared_ptr<MobileElement> element);
    void render(ArabicGraphics& graphics) override;
    bool handleEvent(const ArabicUI::UIEvent& event) override;
    void onTouch(const TouchEvent& event) override;
    
    ArabicUI::UIElementType getType() const override { return ArabicUI::UIElementType::PANEL; }

private:
    Orientation _orientation;
    std::vector<std::shared_ptr<MobileElement>> _elements;
};

} // namespace ArabicLanguage

#endif // ARABIC_MOBILE_UI_H
