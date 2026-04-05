#include "ArabicUI.h"
#include "../graphics/ArabicGameEngine.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>

namespace ArabicLanguage {

// ────────────────────────────────────────────────────────
// UIButton Implementation
// ────────────────────────────────────────────────────────

bool ArabicUI::UIButton::handleInput(const ArabicInput& input, ArabicGraphics& graphics) {
    if (!enabled || !visible) return false;

    float mx = static_cast<float>(input.getMouseX());
    float my = static_cast<float>(input.getMouseY());

    if (containsPoint(mx, my)) {
        if (input.isMouseButtonPressed(ArabicInput::BUTTON_LEFT)) {
            state = PRESSED;
            isPressed = true;
            
            UIEvent event(UIEvent::MOUSE_CLICK);
            event.mouseX = static_cast<int>(mx);
            event.mouseY = static_cast<int>(my);
            event.elementId = id;
            if (onClick) onClick(event);
            return true;
        } else {
            if (state != PRESSED) {
                state = HOVER;
            }
        }
    } else {
        state = NORMAL;
        isPressed = false;
    }
    return false;
}

// ────────────────────────────────────────────────────────
// UITextBox Implementation
// ────────────────────────────────────────────────────────

bool ArabicUI::UITextBox::handleInput(const ArabicInput& input, ArabicGraphics& graphics) {
    if (!enabled || !visible) return false;

    float mx = static_cast<float>(input.getMouseX());
    float my = static_cast<float>(input.getMouseY());

    bool handled = false;

    if (input.isMouseButtonPressed(ArabicInput::BUTTON_LEFT)) {
        if (containsPoint(mx, my)) {
            isFocused = true;
            state = FOCUSED;
            
            UIEvent event(UIEvent::FOCUS_GAIN);
            event.elementId = id;
            if (onFocus) onFocus(event);
            handled = true;
        } else {
            if (isFocused) {
                isFocused = false;
                state = NORMAL;
                UIEvent event(UIEvent::FOCUS_LOSE);
                event.elementId = id;
                if (onBlur) onBlur(event);
            }
        }
    }

    if (isFocused) {
        const auto& textInput = input.getTextInputBuffer();
        if (!textInput.empty()) {
            for (char c : textInput) {
                if (c == '\b') {
                    if (!text.empty()) text.pop_back();
                } else if (c >= 32) {
                    text += c;
                }
            }
            cursorPosition = text.length();
            
            UIEvent event(UIEvent::VALUE_CHANGED);
            event.elementId = id;
            event.textInput = text;
            if (onValueChange) onValueChange(event);
            handled = true;
        }
    }
    return handled;
}

ArabicUI::ArabicUI()
    : layoutDirection(RIGHT_TO_LEFT), designWidth(800.0f), designHeight(600.0f),
      windowHandle(nullptr) {
}

bool ArabicUI::initialize(std::shared_ptr<ArabicGraphics> graphicsPtr, HWND hwnd) {
    graphics = graphicsPtr;
    windowHandle = hwnd;

    if (!graphics) {
        std::cerr << "ArabicUI: Graphics system not provided" << std::endl;
        return false;
    }

    // Set default theme
    setDefaultTheme();

    return true;
}

void ArabicUI::shutdown() {
    elements.clear();
    elementMap.clear();
    focusedElement.reset();
    hoveredElement.reset();

    while (!eventQueue.empty()) {
        eventQueue.pop();
    }
}

void ArabicUI::addElement(std::shared_ptr<UIElement> element) {
    if (!element) return;

    // Convert to RTL if needed
    if (layoutDirection == RIGHT_TO_LEFT) {
        element->setBounds(convertToRTL(element->getBounds()));
    }

    elements.push_back(element);
    elementMap[element->getId()] = element;
    stats.elementCount++;
}

void ArabicUI::removeElement(const std::string& elementId) {
    auto it = elementMap.find(elementId);
    if (it != elementMap.end()) {
        // Remove from vector
        elements.erase(std::remove(elements.begin(), elements.end(), it->second), elements.end());
        elementMap.erase(it);
        stats.elementCount--;

        // Clear references if needed
        if (focusedElement == it->second) {
            focusedElement.reset();
        }
        if (hoveredElement == it->second) {
            hoveredElement.reset();
        }
    }
}

std::shared_ptr<ArabicUI::UIElement> ArabicUI::getElement(const std::string& elementId) const {
    auto it = elementMap.find(elementId);
    return it != elementMap.end() ? it->second : nullptr;
}

void ArabicUI::update(float deltaTime) {
    for (auto& element : elements) {
        if (element->isVisible() && element->isEnabled()) {
            element->update(deltaTime);
        }
    }
}

void ArabicUI::render(std::shared_ptr<ArabicGraphics> graphicsPtr) {
    if (graphicsPtr) {
        graphics = graphicsPtr;
    }
    
    if (!graphics) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    graphics->beginFrame();

    for (auto& element : elements) {
        if (element->isVisible()) {
            element->render(*graphics);
        }
    }

    graphics->endFrame();

    auto endTime = std::chrono::high_resolution_clock::now();
    stats.renderTime = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count());
}

void ArabicUI::processEvent(const UIEvent& event) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Add to queue for processing
    eventQueue.push(event);

    // Process events in reverse order (top elements first)
    for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
        auto& element = *it;

        if (!element->isVisible() || !element->isEnabled()) {
            continue;
        }

        if (element->handleEvent(event)) {
            // Event was handled, stop propagation
            break;
        }
    }

    // Update focused/hovered elements
    if (event.type == UIEvent::MOUSE_MOVE) {
        std::shared_ptr<UIElement> newHovered = nullptr;

        for (auto& element : elements) {
            if (element->isVisible() && element->isEnabled() &&
                element->containsPoint(static_cast<float>(event.mouseX), static_cast<float>(event.mouseY))) {
                newHovered = element;
                break;
            }
        }

        if (newHovered != hoveredElement) {
            if (hoveredElement) {
                hoveredElement->setState(NORMAL);
            }
            hoveredElement = newHovered;
            if (hoveredElement) {
                hoveredElement->setState(HOVER);
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    stats.eventProcessTime = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count());
}

void ArabicUI::processWindowsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    UIEvent uiEvent = UIHelpers::windowsMessageToUIEvent(hwnd, msg, wParam, lParam);
    if (uiEvent.type != UIEvent::MOUSE_CLICK) { // Don't process empty events
        processEvent(uiEvent);
    }
}

std::shared_ptr<ArabicUI::UIButton> ArabicUI::createButton(const std::string& id,
                                                         const std::string& text,
                                                         const UIRect& bounds) {
    auto button = std::make_shared<UIButton>(id);
    button->setText(text);
    button->setBounds(bounds);
    button->setFont(UIFont{"Arial", 12.0f, UIColor(0.0f, 0.0f, 0.0f), false, false, RIGHT_TO_LEFT});

    addElement(button);
    return button;
}

std::shared_ptr<ArabicUI::UITextBox> ArabicUI::createTextBox(const std::string& id,
                                                           const std::string& placeholder,
                                                           const UIRect& bounds) {
    auto textBox = std::make_shared<UITextBox>(id);
    textBox->setPlaceholder(placeholder);
    textBox->setBounds(bounds);
    textBox->setFont(UIFont{"Arial", 12.0f, UIColor(0.0f, 0.0f, 0.0f), false, false, RIGHT_TO_LEFT});

    addElement(textBox);
    return textBox;
}

std::shared_ptr<ArabicUI::UIDropdown> ArabicUI::createDropdown(const std::string& id,
                                                             const UIRect& bounds) {
    auto dropdown = std::make_shared<UIDropdown>(id);
    dropdown->setBounds(bounds);
    dropdown->setFont(UIFont{"Arial", 12.0f, UIColor(0.0f, 0.0f, 0.0f), false, false, RIGHT_TO_LEFT});

    addElement(dropdown);
    return dropdown;
}

std::shared_ptr<ArabicUI::UILabel> ArabicUI::createLabel(const std::string& id,
                                                       const std::string& text,
                                                       const UIRect& bounds) {
    auto label = std::make_shared<UILabel>(id);
    label->setText(text);
    label->setBounds(bounds);
    label->setFont(UIFont{"Arial", 12.0f, UIColor(0.0f, 0.0f, 0.0f), false, false, RIGHT_TO_LEFT});

    addElement(label);
    return label;
}

std::shared_ptr<ArabicUI::UITextArea> ArabicUI::createTextArea(const std::string& id,
                                                             const UIRect& bounds) {
    auto textArea = std::make_shared<UITextArea>(id);
    textArea->setBounds(bounds);
    textArea->setFont(UIFont{"Arial", 12.0f, UIColor(0.0f, 0.0f, 0.0f), false, false, RIGHT_TO_LEFT});

    addElement(textArea);
    return textArea;
}

void ArabicUI::setLayoutDirection(TextDirection direction) {
    layoutDirection = direction;

    // Re-layout all elements if switching to RTL
    if (direction == RIGHT_TO_LEFT) {
        for (auto& element : elements) {
            element->setBounds(convertToRTL(element->getBounds()));
        }
    }
}

ArabicUI::UIRect ArabicUI::convertToRTL(const UIRect& rect) const {
    if (layoutDirection != RIGHT_TO_LEFT) {
        return rect;
    }

    // Assume designWidth is the screen width
    float rtlX = designWidth - rect.x - rect.width;
    return UIRect(rtlX, rect.y, rect.width, rect.height);
}

bool ArabicUI::loadArabicFont(const std::string& fontName, float size) {
    // Implementation for loading Arabic fonts
    // This would integrate with a font system
    return true; // Placeholder
}

void ArabicUI::setDefaultTheme() {
    // Set up default colors and fonts for Arabic UI
    // This would define a complete theme system
}

bool ArabicUI::saveLayout(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    file << "ArabicUI Layout v1.0\n";
    file << "Direction: " << (layoutDirection == RIGHT_TO_LEFT ? "RTL" : "LTR") << "\n";
    file << "Elements: " << elements.size() << "\n";

    for (const auto& element : elements) {
        const auto& bounds = element->getBounds();
        file << "Element: " << element->getId() << "\n";
        file << "  Type: " << element->getType() << "\n";
        file << "  Bounds: " << bounds.x << "," << bounds.y << ","
             << bounds.width << "," << bounds.height << "\n";
        file << "  Text: " << element->getText() << "\n";
        file << "  Visible: " << (element->isVisible() ? "1" : "0") << "\n";
        file << "  Enabled: " << (element->isEnabled() ? "1" : "0") << "\n";
    }

    file.close();
    return true;
}

bool ArabicUI::loadLayout(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::getline(file, line); // Header

    if (line != "ArabicUI Layout v1.0") {
        return false;
    }

    // Clear existing elements
    elements.clear();
    elementMap.clear();

    // Parse layout
    while (std::getline(file, line)) {
        if (line.find("Direction:") == 0) {
            std::string dir = line.substr(10);
            layoutDirection = (dir == "RTL") ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
        } else if (line.find("Element:") == 0) {
            std::string elementId = line.substr(9);

            // Read element properties
            UIRect bounds;
            std::string text;
            bool visible = true;
            bool enabled = true;
            UIElementType type = BUTTON;

            while (std::getline(file, line) && !line.empty()) {
                if (line.find("  Type:") == 0) {
                    type = static_cast<UIElementType>(std::stoi(line.substr(8)));
                } else if (line.find("  Bounds:") == 0) {
                    std::string boundsStr = line.substr(10);
                    std::stringstream ss(boundsStr);
                    std::string token;
                    std::getline(ss, token, ','); bounds.x = std::stof(token);
                    std::getline(ss, token, ','); bounds.y = std::stof(token);
                    std::getline(ss, token, ','); bounds.width = std::stof(token);
                    std::getline(ss, token, ','); bounds.height = std::stof(token);
                } else if (line.find("  Text:") == 0) {
                    text = line.substr(8);
                } else if (line.find("  Visible:") == 0) {
                    visible = (line.substr(11) == "1");
                } else if (line.find("  Enabled:") == 0) {
                    enabled = (line.substr(11) == "1");
                }
            }

            // Create element based on type
            std::shared_ptr<UIElement> element;
            switch (type) {
                case BUTTON:
                    element = createButton(elementId, text, bounds);
                    break;
                case TEXTBOX:
                    element = createTextBox(elementId, text, bounds);
                    break;
                case DROPDOWN:
                    element = createDropdown(elementId, bounds);
                    break;
                case LABEL:
                    element = createLabel(elementId, text, bounds);
                    break;
                case TEXTAREA:
                    element = createTextArea(elementId, bounds);
                    break;
                default:
                    continue;
            }

            if (element) {
                element->setVisible(visible);
                element->setEnabled(enabled);
            }
        }
    }

    file.close();
    return true;
}

void ArabicUI::makeResponsive(float screenWidth, float screenHeight) {
    float scaleX = screenWidth / designWidth;
    float scaleY = screenHeight / designHeight;

    for (auto& element : elements) {
        const auto& bounds = element->getBounds();
        UIRect newBounds(bounds.x * scaleX, bounds.y * scaleY,
                        bounds.width * scaleX, bounds.height * scaleY);
        element->setBounds(newBounds);
    }
}

// ────────────────────────────────────────────────────────
// تطبيق UIHelpers
// ────────────────────────────────────────────────────────

namespace UIHelpers {

ArabicUI::UIRect createRTLLayout(float screenWidth, const ArabicUI::UIRect& original) {
    float rtlX = screenWidth - original.x - original.width;
    return ArabicUI::UIRect(rtlX, original.y, original.width, original.height);
}

glm::vec2 measureArabicText(const std::string& text, const ArabicUI::UIFont& font) {
    // Basic text measurement - would need proper font metrics
    float width = text.length() * font.size * 0.6f; // Rough estimate
    float height = font.size * 1.2f;
    return glm::vec2(width, height);
}

std::vector<std::string> splitArabicText(const std::string& text) {
    std::vector<std::string> words;
    std::stringstream ss(text);
    std::string word;

    while (ss >> word) {
        if (!word.empty()) {
            words.push_back(word);
        }
    }

    return words;
}

ArabicUI::TextDirection detectTextDirection(const std::string& text) {
    // Simple detection based on Arabic characters
    for (char c : text) {
        if ((c >= 0x0600 && c <= 0x06FF) || (c >= 0x0750 && c <= 0x077F)) {
            return ArabicUI::RIGHT_TO_LEFT;
        }
    }
    return ArabicUI::LEFT_TO_RIGHT;
}

ArabicUI::UIRect mirrorRTL(const ArabicUI::UIRect& rect, float screenWidth) {
    return createRTLLayout(screenWidth, rect);
}

ArabicUI::UIColor getStandardColor(const std::string& colorName) {
    static std::unordered_map<std::string, ArabicUI::UIColor> colors = {
        {"white", ArabicUI::UIColor(1.0f, 1.0f, 1.0f)},
        {"black", ArabicUI::UIColor(0.0f, 0.0f, 0.0f)},
        {"gray", ArabicUI::UIColor(0.5f, 0.5f, 0.5f)},
        {"red", ArabicUI::UIColor(1.0f, 0.0f, 0.0f)},
        {"green", ArabicUI::UIColor(0.0f, 1.0f, 0.0f)},
        {"blue", ArabicUI::UIColor(0.0f, 0.0f, 1.0f)},
        {"yellow", ArabicUI::UIColor(1.0f, 1.0f, 0.0f)},
        {"purple", ArabicUI::UIColor(0.5f, 0.0f, 0.5f)},
        {"orange", ArabicUI::UIColor(1.0f, 0.5f, 0.0f)}
    };

    auto it = colors.find(colorName);
    return it != colors.end() ? it->second : ArabicUI::UIColor(1.0f, 1.0f, 1.0f);
}

ArabicUI::UIEvent windowsMessageToUIEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ArabicUI::UIEvent event;

    switch (msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP: {
            event.type = (msg == WM_LBUTTONDOWN) ? ArabicUI::UIEvent::MOUSE_CLICK :
                                                   ArabicUI::UIEvent::MOUSE_CLICK;
            event.mouseX = LOWORD(lParam);
            event.mouseY = HIWORD(lParam);
            break;
        }

        case WM_MOUSEMOVE: {
            event.type = ArabicUI::UIEvent::MOUSE_MOVE;
            event.mouseX = LOWORD(lParam);
            event.mouseY = HIWORD(lParam);
            break;
        }

        case WM_KEYDOWN: {
            event.type = ArabicUI::UIEvent::KEY_PRESS;
            event.keyCode = static_cast<int>(wParam);
            break;
        }

        case WM_KEYUP: {
            event.type = ArabicUI::UIEvent::KEY_RELEASE;
            event.keyCode = static_cast<int>(wParam);
            break;
        }

        case WM_CHAR: {
            if (wParam >= 32) { // Printable characters
                event.type = ArabicUI::UIEvent::TEXT_INPUT;
                event.textInput = std::string(1, static_cast<char>(wParam));
            }
            break;
        }

        default:
            event.type = ArabicUI::UIEvent::MOUSE_CLICK; // Invalid event
            break;
    }

    return event;
}

void createUIEffects(ArabicGraphics& graphics, const ArabicUI::UIElement& element,
                    ArabicUI::UIState state) {
    const auto& bounds = element.getBounds();

    switch (state) {
        case ArabicUI::HOVER: {
            // Add glow effect
            graphics.drawQuad(
                glm::vec3(bounds.x - 2, bounds.y - 2, 0.0f),
                bounds.width + 4, bounds.height + 4,
                glm::vec4(1.0f, 1.0f, 0.0f, 0.3f)
            );
            break;
        }

        case ArabicUI::FOCUSED: {
            // Add focus ring
            graphics.drawLine(
                glm::vec3(bounds.x - 1, bounds.y - 1, 0.0f),
                glm::vec3(bounds.x + bounds.width + 1, bounds.y - 1, 0.0f),
                glm::vec4(0.0f, 0.5f, 1.0f, 1.0f), 2.0f
            );
            graphics.drawLine(
                glm::vec3(bounds.x + bounds.width + 1, bounds.y - 1, 0.0f),
                glm::vec3(bounds.x + bounds.width + 1, bounds.y + bounds.height + 1, 0.0f),
                glm::vec4(0.0f, 0.5f, 1.0f, 1.0f), 2.0f
            );
            graphics.drawLine(
                glm::vec3(bounds.x + bounds.width + 1, bounds.y + bounds.height + 1, 0.0f),
                glm::vec3(bounds.x - 1, bounds.y + bounds.height + 1, 0.0f),
                glm::vec4(0.0f, 0.5f, 1.0f, 1.0f), 2.0f
            );
            graphics.drawLine(
                glm::vec3(bounds.x - 1, bounds.y + bounds.height + 1, 0.0f),
                glm::vec3(bounds.x - 1, bounds.y - 1, 0.0f),
                glm::vec4(0.0f, 0.5f, 1.0f, 1.0f), 2.0f
            );
            break;
        }

        default:
            break;
    }
}

}

} // namespace ArabicLanguage
