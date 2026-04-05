// ArabicMobileUI.cpp - تطبيق واجهات المستخدم للهواتف
// الأسبوع الأول من الشهر السابع: مكتبة Android

#include "ArabicMobileUI.h"
#include <iostream>

namespace ArabicLanguage {

// implementation of MobileButton
MobileButton::MobileButton(const std::string& id, const std::string& text, float x, float y, float w, float h)
    : _text(text) {
    this->id = id;
    this->bounds = ArabicUI::UIRect(x, y, w, h);
}

void MobileButton::render(ArabicGraphics& graphics) {
    // محاكاة الرسم - في الواقع سيستخدم ArabicGraphics لرسم زر مستدير الحواف
    uint32_t color = _isPressed ? 0xFFAAAAAA : 0xFFDDDDDD;
    // graphics.drawRoundedRect(bounds.x, bounds.y, bounds.width, bounds.height, 10, color);
    // graphics.drawText(_text, bounds.x + 10, bounds.y + bounds.height/2, 0xFF000000);
    
    std::cout << "🎨 رسم زر محمول: [" << _text << "] في الموقع (" 
              << bounds.x << ", " << bounds.y << ") الحالة: " 
              << (_isPressed ? "مضغوط" : "عادي") << std::endl;
}

bool MobileButton::handleEvent(const ArabicUI::UIEvent& event) {
    // تحويل أحداث الفأرة إلى أحداث لمس بسيطة للاختبار
    if (event.type == ArabicUI::UIEvent::MOUSE_CLICK) {
        if (containsPoint(static_cast<float>(event.mouseX), static_cast<float>(event.mouseY))) {
            if (_onClick) _onClick();
            return true;
        }
    }
    return false;
}

void MobileButton::onTouch(const TouchEvent& event) {
    if (event.type == TouchType::DOWN) {
        _isPressed = true;
    } else if (event.type == TouchType::UP) {
        if (_isPressed && _onClick) {
            _onClick();
        }
        _isPressed = false;
    }
}

// implementation of MobileLayout
MobileLayout::MobileLayout(Orientation orientation) : _orientation(orientation) {}

void MobileLayout::addElement(std::shared_ptr<MobileElement> element) {
    _elements.push_back(element);
}

void MobileLayout::render(ArabicGraphics& graphics) {
    for (auto& element : _elements) {
        element->render(graphics);
    }
}

bool MobileLayout::handleEvent(const ArabicUI::UIEvent& event) {
    for (auto& element : _elements) {
        if (element->handleEvent(event)) return true;
    }
    return false;
}

void MobileLayout::onTouch(const TouchEvent& event) {
    for (auto& element : _elements) {
        if (element->containsPoint(event.x, event.y)) {
            element->onTouch(event);
        }
    }
}

} // namespace ArabicLanguage
