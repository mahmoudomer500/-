#pragma once

#include "ArabicGraphics.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <queue>

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../../core/SafeWindows.h"

namespace ArabicLanguage {

    class ArabicInput;
    class ArabicGraphics;

/**
 * @brief مكتبة واجهات المستخدم العربية - Arabic UI Library
 *
 * مكتبة شاملة لواجهات المستخدم مع دعم كامل للنصوص العربية
 * وتخطيط من اليمين لليسار (RTL)
 */
class ArabicUI {
public:
    /**
     * @brief حالة عنصر UI
     */
    enum UIState {
        NORMAL,
        HOVER,
        PRESSED,
        DISABLED,
        FOCUSED
    };

    /**
     * @brief اتجاه النص والتخطيط
     */
    enum TextDirection {
        LEFT_TO_RIGHT,
        RIGHT_TO_LEFT,
        AUTO_DETECT
    };

    /**
     * @brief محاذاة النص
     */
    enum TextAlignment {
        ALIGN_LEFT,
        ALIGN_CENTER,
        ALIGN_RIGHT,
        ALIGN_JUSTIFY
    };

    /**
     * @brief نوع عنصر UI
     */
    enum UIElementType {
        BUTTON,
        TEXTBOX,
        LABEL,
        DROPDOWN,
        CHECKBOX,
        RADIOBUTTON,
        SLIDER,
        PROGRESSBAR,
        PANEL,
        WINDOW,
        TEXTAREA
    };

    /**
     * @brief حدث UI
     */
    struct UIEvent {
        enum Type {
            MOUSE_CLICK,
            MOUSE_DOUBLE_CLICK,
            MOUSE_MOVE,
            MOUSE_ENTER,
            MOUSE_LEAVE,
            KEY_PRESS,
            KEY_RELEASE,
            TEXT_INPUT,
            FOCUS_GAIN,
            FOCUS_LOSE,
            VALUE_CHANGED
        };

        Type type;
        int mouseX, mouseY;
        int keyCode;
        std::string textInput;
        std::string elementId;

        UIEvent(Type t = MOUSE_CLICK) : type(t), mouseX(0), mouseY(0), keyCode(0) {}
    };

    /**
     * @brief كائن مستطيل للتخطيط
     */
    struct UIRect {
        float x, y, width, height;

        UIRect() : x(0), y(0), width(0), height(0) {}
        UIRect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}

        bool contains(float px, float py) const {
            return px >= x && px <= x + width && py >= y && py <= y + height;
        }

        UIRect intersect(const UIRect& other) const {
            float ix = (std::max)(x, other.x);
            float iy = (std::max)(y, other.y);
            float iw = (std::min)(x + width, other.x + other.width) - ix;
            float ih = (std::min)(y + height, other.y + other.height) - iy;

            if (iw > 0 && ih > 0) {
                return UIRect(ix, iy, iw, ih);
            }
            return UIRect();
        }
    };

    /**
     * @brief ألوان UI
     */
    struct UIColor {
        float r, g, b, a;

        UIColor() : r(0), g(0), b(0), a(1) {}
        UIColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
        UIColor(int r, int g, int b, int a = 255) : r(r / 255.0f), g(g / 255.0f), b(b / 255.0f), a(a / 255.0f) {}

        static UIColor fromHex(const std::string& hex) {
            UIColor color;
            // Parse hex color (implementation needed)
            return color;
        }

        glm::vec4 toVec4() const { return glm::vec4(r, g, b, a); }
        operator glm::vec4() const { return toVec4(); }
    };

    /**
     * @brief خصائص الخط
     */
    struct UIFont {
        std::string fontName;
        float size;
        UIColor color;
        bool bold;
        bool italic;
        TextDirection direction;

        UIFont() : fontName("Arial"), size(12.0f), bold(false), italic(false), direction(RIGHT_TO_LEFT) {}
        UIFont(const std::string& name, float s, UIColor c, bool b, bool i, TextDirection d)
            : fontName(name), size(s), color(c), bold(b), italic(i), direction(d) {}
    };

    /**
     * @brief عنصر UI أساسي
     */
    class UIElement {
    protected:
        std::string id;
        UIRect bounds;
        UIState state;
        bool visible;
        bool enabled;
        std::string text;
        UIFont font;
        UIColor backgroundColor;
        UIColor borderColor;
        float borderWidth;
        std::unordered_map<UIState, UIColor> stateColors;

        std::function<void(UIEvent)> onClick;
        std::function<void(UIEvent)> onHover;
        std::function<void(UIEvent)> onFocus;
        std::function<void(UIEvent)> onBlur;
        std::function<void(UIEvent)> onValueChange;

    public:
        UIElement(const std::string& elementId = "")
            : id(elementId), state(NORMAL), visible(true), enabled(true),
              borderWidth(1.0f) {}

        virtual ~UIElement() = default;

        // Getters
        const std::string& getId() const { return id; }
        const UIRect& getBounds() const { return bounds; }
        UIState getState() const { return state; }
        bool isVisible() const { return visible; }
        bool isEnabled() const { return enabled; }
        const std::string& getText() const { return text; }
        const UIFont& getFont() const { return font; }

        // Setters
        void setId(const std::string& elementId) { id = elementId; }
        void setBounds(const UIRect& rect) { bounds = rect; }
        void setBounds(float x, float y, float w, float h) { bounds = UIRect(x, y, w, h); }
        void setState(UIState s) { state = s; }
        void setVisible(bool v) { visible = v; }
        void setEnabled(bool e) { enabled = e; }
        void setText(const std::string& t) { text = t; }
        void setFont(const UIFont& f) { font = f; }
        void setBackgroundColor(const UIColor& color) { backgroundColor = color; }
        void setBorderColor(const UIColor& color) { borderColor = color; }
        void setBorderWidth(float width) { borderWidth = width; }

        // State colors
        void setStateColor(UIState state, const UIColor& color) {
            stateColors[state] = color;
        }

        UIColor getCurrentColor() const {
            auto it = stateColors.find(state);
            if (it != stateColors.end()) {
                return it->second;
            }
            return backgroundColor;
        }

        // Event handlers
        void setOnClick(std::function<void(UIEvent)> handler) { onClick = handler; }
        void setOnHover(std::function<void(UIEvent)> handler) { onHover = handler; }
        void setOnFocus(std::function<void(UIEvent)> handler) { onFocus = handler; }
        void setOnBlur(std::function<void(UIEvent)> handler) { onBlur = handler; }
        void setOnValueChange(std::function<void(UIEvent)> handler) { onValueChange = handler; }

        // Virtual methods
        virtual UIElementType getType() const = 0;
        virtual void render(ArabicGraphics& graphics) = 0;
        virtual bool handleEvent(const UIEvent& event) = 0;

        // Utility methods
        virtual bool containsPoint(float x, float y) const {
            return bounds.contains(x, y);
        }

        virtual void update(float deltaTime) {}
        virtual bool handleInput(const ArabicInput& input, ArabicGraphics& graphics) { return false; }
    };

    /**
     * @brief زر UI
     */
    class UIButton : public UIElement {
    private:
        bool isPressed;

    public:
        UIButton(const std::string& buttonId = "") : UIElement(buttonId), isPressed(false) {
            setBackgroundColor(UIColor(0.8f, 0.8f, 0.8f));
            setBorderColor(UIColor(0.5f, 0.5f, 0.5f));

            setStateColor(HOVER, UIColor(0.9f, 0.9f, 0.9f));
            setStateColor(PRESSED, UIColor(0.6f, 0.6f, 0.6f));
            setStateColor(DISABLED, UIColor(0.7f, 0.7f, 0.7f, 0.5f));
        }

        UIButton(const std::string& buttonText, const glm::vec2& pos, const glm::vec2& size, std::function<void()> callback = nullptr)
            : UIElement(buttonText), isPressed(false) {
            setText(buttonText);
            setBounds(pos.x, pos.y, size.x, size.y);
            if (callback) {
                setOnClick([callback](const UIEvent&) { callback(); });
            }
            setBackgroundColor(UIColor(0.8f, 0.8f, 0.8f));
            setBorderColor(UIColor(0.5f, 0.5f, 0.5f));
            setStateColor(HOVER, UIColor(0.9f, 0.9f, 0.9f));
            setStateColor(PRESSED, UIColor(0.6f, 0.6f, 0.6f));
            setStateColor(DISABLED, UIColor(0.7f, 0.7f, 0.7f, 0.5f));
        }

        UIElementType getType() const override { return BUTTON; }

        void setFontSize(float size) {
            font.size = size;
        }

        float getFontSize() const {
            return font.size;
        }

        void render(ArabicGraphics& graphics) override {
            if (!visible) return;

            UIColor currentColor = getCurrentColor();

            // Draw background
            graphics.drawQuad(
                glm::vec3(bounds.x, bounds.y, 0.0f),
                bounds.width, bounds.height,
                currentColor.toVec4()
            );

            // Draw border
            if (borderWidth > 0) {
                graphics.drawLine(
                    glm::vec3(bounds.x, bounds.y, 0.0f),
                    glm::vec3(bounds.x + bounds.width, bounds.y, 0.0f),
                    borderColor.toVec4(), borderWidth
                );
                graphics.drawLine(
                    glm::vec3(bounds.x + bounds.width, bounds.y, 0.0f),
                    glm::vec3(bounds.x + bounds.width, bounds.y + bounds.height, 0.0f),
                    borderColor.toVec4(), borderWidth
                );
                graphics.drawLine(
                    glm::vec3(bounds.x + bounds.width, bounds.y + bounds.height, 0.0f),
                    glm::vec3(bounds.x, bounds.y + bounds.height, 0.0f),
                    borderColor.toVec4(), borderWidth
                );
                graphics.drawLine(
                    glm::vec3(bounds.x, bounds.y + bounds.height, 0.0f),
                    glm::vec3(bounds.x, bounds.y, 0.0f),
                    borderColor.toVec4(), borderWidth
                );
            }

            // Draw text
            if (!text.empty()) {
                float textX = bounds.x + bounds.width / 2.0f;
                float textY = bounds.y + bounds.height / 2.0f;

                // Center text (basic implementation)
                graphics.drawText(text, glm::vec2(textX, textY),
                                font.color.toVec4(), font.size);
            }
        }

        bool handleEvent(const UIEvent& event) override {
            if (!enabled || !visible) return false;

            switch (event.type) {
                case UIEvent::MOUSE_MOVE:
                    if (containsPoint(static_cast<float>(event.mouseX), static_cast<float>(event.mouseY))) {
                        if (state != HOVER && state != PRESSED) {
                            state = HOVER;
                            if (onHover) onHover(event);
                            return true;
                        }
                    } else {
                        if (state == HOVER) {
                            state = NORMAL;
                            return true;
                        }
                    }
                    break;

                case UIEvent::MOUSE_CLICK:
                    if (containsPoint(static_cast<float>(event.mouseX), static_cast<float>(event.mouseY))) {
                        state = PRESSED;
                        isPressed = true;
                        if (onClick) onClick(event);
                        return true;
                    }
                    break;

                default:
                    break;
            }

            return false;
        }

        bool handleInput(const ArabicInput& input, ArabicGraphics& graphics) override;

        void update(float deltaTime) override {
            if (isPressed && state != PRESSED) {
                isPressed = false;
            }
        }
    };

    /**
     * @brief مربع نص UI
     */
    class UITextBox : public UIElement {
    private:
        std::string placeholder;
        bool isFocused;
        size_t cursorPosition;
        float blinkTimer;
        bool multiline;

    public:
        UITextBox(const std::string& textBoxId = "")
            : UIElement(textBoxId), isFocused(false), cursorPosition(0), blinkTimer(0.0f), multiline(false) {
            setBackgroundColor(UIColor(1.0f, 1.0f, 1.0f));
            setBorderColor(UIColor(0.5f, 0.5f, 0.5f));

            setStateColor(FOCUSED, UIColor(0.95f, 0.95f, 1.0f));
        }

        UITextBox(const std::string& textBoxId, const glm::vec2& pos, const glm::vec2& size)
            : UIElement(textBoxId), isFocused(false), cursorPosition(0), blinkTimer(0.0f), multiline(false) {
            setBounds(pos.x, pos.y, size.x, size.y);
            setBackgroundColor(UIColor(1.0f, 1.0f, 1.0f));
            setBorderColor(UIColor(0.5f, 0.5f, 0.5f));
            setStateColor(FOCUSED, UIColor(0.95f, 0.95f, 1.0f));
        }

        UIElementType getType() const override { return TEXTBOX; }

        void setPlaceholder(const std::string& ph) { placeholder = ph; }
        const std::string& getPlaceholder() const { return placeholder; }

        void setMultiline(bool m) { multiline = m; }
        bool isMultiline() const { return multiline; }

        void render(ArabicGraphics& graphics) override {
            if (!visible) return;

            UIColor currentColor = getCurrentColor();

            // Draw background
            graphics.drawQuad(
                glm::vec3(bounds.x, bounds.y, 0.0f),
                bounds.width, bounds.height,
                currentColor.toVec4()
            );

            // Draw border
            graphics.drawLine(
                glm::vec3(bounds.x, bounds.y, 0.0f),
                glm::vec3(bounds.x + bounds.width, bounds.y, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(bounds.x + bounds.width, bounds.y, 0.0f),
                glm::vec3(bounds.x + bounds.width, bounds.y + bounds.height, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(bounds.x + bounds.width, bounds.y + bounds.height, 0.0f),
                glm::vec3(bounds.x, bounds.y + bounds.height, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(bounds.x, bounds.y + bounds.height, 0.0f),
                glm::vec3(bounds.x, bounds.y, 0.0f),
                borderColor.toVec4(), borderWidth
            );

            // Draw text or placeholder
            std::string displayText = text.empty() ? placeholder : text;
            UIColor textColor = text.empty() ? UIColor(0.7f, 0.7f, 0.7f) : font.color;

            if (!displayText.empty()) {
                float textX = bounds.x + 5.0f; // Left padding
                float textY = bounds.y + bounds.height / 2.0f;

                graphics.drawText(displayText, glm::vec2(textX, textY),
                                textColor.toVec4(), font.size);
            }

            // Draw cursor if focused
            if (isFocused && fmod(blinkTimer, 1.0f) < 0.5f) {
                float cursorX = bounds.x + 5.0f;
                if (!text.empty()) {
                    // Calculate cursor position based on text width (simplified)
                    cursorX += text.length() * font.size * 0.6f;
                }

                graphics.drawLine(
                    glm::vec3(cursorX, bounds.y + 5.0f, 0.0f),
                    glm::vec3(cursorX, bounds.y + bounds.height - 5.0f, 0.0f),
                    UIColor(0.0f, 0.0f, 0.0f).toVec4(), 2.0f
                );
            }
        }

        bool handleEvent(const UIEvent& event) override {
            if (!enabled || !visible) return false;

            switch (event.type) {
                case UIEvent::MOUSE_CLICK:
                    if (containsPoint(static_cast<float>(event.mouseX), static_cast<float>(event.mouseY))) {
                        isFocused = true;
                        state = FOCUSED;
                        if (onFocus) onFocus(event);
                        return true;
                    } else {
                        isFocused = false;
                        state = NORMAL;
                        return true;
                    }
                    break;

                case UIEvent::TEXT_INPUT:
                    if (isFocused) {
                        text += event.textInput;
                        cursorPosition = text.length();
                        if (onValueChange) onValueChange(event);
                        return true;
                    }
                    break;

                case UIEvent::KEY_PRESS:
                    if (isFocused) {
                        if (event.keyCode == VK_BACK && !text.empty()) {
                            text.pop_back();
                            cursorPosition = text.length();
                            if (onValueChange) onValueChange(event);
                            return true;
                        }
                    }
                    break;

                default:
                    break;
            }

            return false;
        }

        bool handleInput(const ArabicInput& input, ArabicGraphics& graphics) override;

        void update(float deltaTime) override {
            blinkTimer += deltaTime;
        }
    };

    /**
     * @brief قائمة منسدلة UI
     */
    class UIDropdown : public UIElement {
    private:
        std::vector<std::string> options;
        int selectedIndex;
        bool isExpanded;
        UIRect expandedBounds;

    public:
        UIDropdown(const std::string& dropdownId = "")
            : UIElement(dropdownId), selectedIndex(-1), isExpanded(false) {
            setBackgroundColor(UIColor(1.0f, 1.0f, 1.0f));
            setBorderColor(UIColor(0.5f, 0.5f, 0.5f));
        }

        UIElementType getType() const override { return DROPDOWN; }

        void addOption(const std::string& option) {
            options.push_back(option);
        }

        void setSelectedIndex(int index) {
            if (index >= -1 && index < static_cast<int>(options.size())) {
                selectedIndex = index;
                if (index >= 0) {
                    text = options[index];
                } else {
                    text.clear();
                }
            }
        }

        int getSelectedIndex() const { return selectedIndex; }
        const std::string& getSelectedOption() const {
            if (selectedIndex >= 0 && selectedIndex < static_cast<int>(options.size())) {
                return options[selectedIndex];
            }
            static const std::string empty = "";
            return empty;
        }

        void render(ArabicGraphics& graphics) override {
            if (!visible) return;

            UIColor currentColor = getCurrentColor();

            // Draw main button
            graphics.drawQuad(
                glm::vec3(bounds.x, bounds.y, 0.0f),
                bounds.width, bounds.height,
                currentColor.toVec4()
            );

            // Draw border
            graphics.drawLine(
                glm::vec3(bounds.x, bounds.y, 0.0f),
                glm::vec3(bounds.x + bounds.width, bounds.y, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(bounds.x + bounds.width, bounds.y, 0.0f),
                glm::vec3(bounds.x + bounds.width, bounds.y + bounds.height, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(bounds.x + bounds.width, bounds.y + bounds.height, 0.0f),
                glm::vec3(bounds.x, bounds.y + bounds.height, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(bounds.x, bounds.y + bounds.height, 0.0f),
                glm::vec3(bounds.x, bounds.y, 0.0f),
                borderColor.toVec4(), borderWidth
            );

            // Draw dropdown arrow
            float arrowSize = 8.0f;
            float arrowX = bounds.x + bounds.width - arrowSize - 5.0f;
            float arrowY = bounds.y + bounds.height / 2.0f;

            graphics.drawLine(
                glm::vec3(arrowX, arrowY - arrowSize / 2.0f, 0.0f),
                glm::vec3(arrowX + arrowSize / 2.0f, arrowY + arrowSize / 2.0f, 0.0f),
                UIColor(0.3f, 0.3f, 0.3f).toVec4(), 2.0f
            );
            graphics.drawLine(
                glm::vec3(arrowX + arrowSize / 2.0f, arrowY + arrowSize / 2.0f, 0.0f),
                glm::vec3(arrowX + arrowSize, arrowY - arrowSize / 2.0f, 0.0f),
                UIColor(0.3f, 0.3f, 0.3f).toVec4(), 2.0f
            );

            // Draw text
            std::string displayText = text.empty() ? "اختر..." : text;
            float textX = bounds.x + 5.0f;
            float textY = bounds.y + bounds.height / 2.0f;

            graphics.drawText(displayText, glm::vec2(textX, textY),
                            font.color.toVec4(), font.size);

            // Draw expanded list if needed
            if (isExpanded) {
                renderExpandedList(graphics);
            }
        }

        bool handleEvent(const UIEvent& event) override {
            if (!enabled || !visible) return false;

            // Handle expanded list events first
            if (isExpanded) {
                return handleExpandedEvent(event);
            }

            switch (event.type) {
                case UIEvent::MOUSE_CLICK:
                    if (containsPoint(static_cast<float>(event.mouseX), static_cast<float>(event.mouseY))) {
                        isExpanded = !isExpanded;
                        if (isExpanded) {
                            expandedBounds = UIRect(bounds.x, bounds.y + bounds.height,
                                                  bounds.width, static_cast<float>(options.size()) * bounds.height);
                        }
                        if (onClick) onClick(event);
                        return true;
                    } else if (isExpanded) {
                        isExpanded = false;
                        return true;
                    }
                    break;

                default:
                    break;
            }

            return false;
        }

        void update(float deltaTime) override {}

    private:
        void renderExpandedList(ArabicGraphics& graphics) {
            // Draw background for expanded list
            graphics.drawQuad(
                glm::vec3(expandedBounds.x, expandedBounds.y, 0.0f),
                expandedBounds.width, expandedBounds.height,
                UIColor(1.0f, 1.0f, 1.0f).toVec4()
            );

            // Draw border
            graphics.drawLine(
                glm::vec3(expandedBounds.x, expandedBounds.y, 0.0f),
                glm::vec3(expandedBounds.x + expandedBounds.width, expandedBounds.y, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(expandedBounds.x + expandedBounds.width, expandedBounds.y, 0.0f),
                glm::vec3(expandedBounds.x + expandedBounds.width, expandedBounds.y + expandedBounds.height, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(expandedBounds.x + expandedBounds.width, expandedBounds.y + expandedBounds.height, 0.0f),
                glm::vec3(expandedBounds.x, expandedBounds.y + expandedBounds.height, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(expandedBounds.x, expandedBounds.y + expandedBounds.height, 0.0f),
                glm::vec3(expandedBounds.x, expandedBounds.y, 0.0f),
                borderColor.toVec4(), borderWidth
            );

            // Draw options
            for (size_t i = 0; i < options.size(); ++i) {
                float optionY = expandedBounds.y + static_cast<float>(i) * bounds.height;
                UIRect optionRect(expandedBounds.x, optionY, expandedBounds.width, bounds.height);

                // Highlight selected option
                if (static_cast<int>(i) == selectedIndex) {
                    graphics.drawQuad(
                        glm::vec3(optionRect.x, optionRect.y, 0.0f),
                        optionRect.width, optionRect.height,
                        UIColor(0.8f, 0.9f, 1.0f).toVec4()
                    );
                }

                // Draw option text
                float textX = optionRect.x + 5.0f;
                float textY = optionY + bounds.height / 2.0f;

                graphics.drawText(options[i], glm::vec2(textX, textY),
                                font.color.toVec4(), font.size);
            }
        }

        bool handleExpandedEvent(const UIEvent& event) {
            switch (event.type) {
                case UIEvent::MOUSE_CLICK:
                    if (expandedBounds.contains(static_cast<float>(event.mouseX), static_cast<float>(event.mouseY))) {
                        // Find which option was clicked
                        float relativeY = static_cast<float>(event.mouseY) - expandedBounds.y;
                        int optionIndex = static_cast<int>(relativeY / bounds.height);

                        if (optionIndex >= 0 && optionIndex < static_cast<int>(options.size())) {
                            setSelectedIndex(optionIndex);
                            isExpanded = false;

                            UIEvent valueEvent = event;
                            valueEvent.type = UIEvent::VALUE_CHANGED;
                            if (onValueChange) onValueChange(valueEvent);

                            return true;
                        }
                    } else {
                        // Click outside, close dropdown
                        isExpanded = false;
                        return true;
                    }
                    break;

                default:
                    break;
            }

            return false;
        }
    };

    /**
     * @brief ملصق نصي UI
     */
    class UILabel : public UIElement {
    public:
        UILabel(const std::string& labelId = "") : UIElement(labelId) {
            setBackgroundColor(UIColor(0, 0, 0, 0)); // Transparent background
            setBorderWidth(0.0f);
        }

        UILabel(const std::string& labelText, const glm::vec2& pos, const UIColor& color) : UIElement("") {
            setText(labelText);
            setBounds(pos.x, pos.y, 200.0f, 30.0f); // Default size
            font.color = color;
            setBackgroundColor(UIColor(0, 0, 0, 0));
            setBorderWidth(0.0f);
        }

        UIElementType getType() const override { return LABEL; }

        void render(ArabicGraphics& graphics) override {
            if (!visible || text.empty()) return;

            float textX = bounds.x;
            float textY = bounds.y + bounds.height / 2.0f;

            graphics.drawText(text, glm::vec2(textX, textY),
                            font.color.toVec4(), font.size);
        }

        bool handleEvent(const UIEvent& event) override {
            return false; // Labels usually don't handle events
        }
    };

    /**
     * @brief منطقة نصية UI (متعددة الأسطر)
     */
    class UITextArea : public UIElement {
    private:
        bool isFocused;
        std::vector<std::string> lines;
        int cursorLine;
        int cursorColumn;
        float blinkTimer;

    public:
        UITextArea(const std::string& textAreaId = "")
            : UIElement(textAreaId), isFocused(false), cursorLine(0), cursorColumn(0), blinkTimer(0.0f) {
            setBackgroundColor(UIColor(1.0f, 1.0f, 1.0f));
            setBorderColor(UIColor(0.5f, 0.5f, 0.5f));
            lines.push_back("");
        }

        UIElementType getType() const override { return TEXTAREA; }

        void setFontSize(float size) {
            font.size = size;
        }

        float getFontSize() const {
            return font.size;
        }

        void setText(const std::string& t) {
            UIElement::setText(t);
            lines.clear();
            std::string currentLine;
            for (char c : t) {
                if (c == '\n') {
                    lines.push_back(currentLine);
                    currentLine = "";
                } else {
                    currentLine += c;
                }
            }
            lines.push_back(currentLine);
            if (lines.empty()) lines.push_back("");
            cursorLine = 0;
            cursorColumn = 0;
        }

        void setLines(const std::vector<std::string>& l) {
            lines = l;
            if (lines.empty()) lines.push_back("");
            cursorLine = 0;
            cursorColumn = 0;
            
            // Reconstruct full text
            std::string fullText;
            for (size_t i = 0; i < lines.size(); ++i) {
                fullText += lines[i];
                if (i < lines.size() - 1) fullText += "\n";
            }
            text = fullText;
        }

        const std::vector<std::string>& getLines() const { return lines; }

        void render(ArabicGraphics& graphics) override {
            if (!visible) return;

            UIColor currentColor = getCurrentColor();

            // Draw background
            graphics.drawQuad(
                glm::vec3(bounds.x, bounds.y, 0.0f),
                bounds.width, bounds.height,
                currentColor.toVec4()
            );

            // Draw border
            graphics.drawLine(
                glm::vec3(bounds.x, bounds.y, 0.0f),
                glm::vec3(bounds.x + bounds.width, bounds.y, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(bounds.x + bounds.width, bounds.y, 0.0f),
                glm::vec3(bounds.x + bounds.width, bounds.y + bounds.height, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(bounds.x + bounds.width, bounds.y + bounds.height, 0.0f),
                glm::vec3(bounds.x, bounds.y + bounds.height, 0.0f),
                borderColor.toVec4(), borderWidth
            );
            graphics.drawLine(
                glm::vec3(bounds.x, bounds.y + bounds.height, 0.0f),
                glm::vec3(bounds.x, bounds.y, 0.0f),
                borderColor.toVec4(), borderWidth
            );

            // Draw lines
            float lineHeight = font.size * 1.2f;
            for (size_t i = 0; i < lines.size(); ++i) {
                float lineY = bounds.y + 5.0f + i * lineHeight;
                if (lineY + lineHeight > bounds.y + bounds.height) break;

                graphics.drawText(lines[i], glm::vec2(bounds.x + 5.0f, lineY + lineHeight/2.0f),
                                font.color.toVec4(), font.size);
            }

            // Draw cursor
            if (isFocused && fmod(blinkTimer, 1.0f) < 0.5f) {
                float cursorX = bounds.x + 5.0f + cursorColumn * font.size * 0.6f;
                float cursorY = bounds.y + 5.0f + cursorLine * lineHeight;

                graphics.drawLine(
                    glm::vec3(cursorX, cursorY, 0.0f),
                    glm::vec3(cursorX, cursorY + lineHeight, 0.0f),
                    UIColor(0.0f, 0.0f, 0.0f).toVec4(), 1.0f
                );
            }
        }

        bool handleEvent(const UIEvent& event) override {
            if (!enabled || !visible) return false;

            switch (event.type) {
                case UIEvent::MOUSE_CLICK:
                    if (containsPoint(static_cast<float>(event.mouseX), static_cast<float>(event.mouseY))) {
                        isFocused = true;
                        state = FOCUSED;
                        return true;
                    } else {
                        isFocused = false;
                        state = NORMAL;
                    }
                    break;

                case UIEvent::TEXT_INPUT:
                    if (isFocused) {
                        lines[cursorLine].insert(cursorColumn, event.textInput);
                        cursorColumn += static_cast<int>(event.textInput.length());
                        return true;
                    }
                    break;

                case UIEvent::KEY_PRESS:
                    if (isFocused) {
                        if (event.keyCode == VK_RETURN) {
                            std::string remaining = lines[cursorLine].substr(cursorColumn);
                            lines[cursorLine] = lines[cursorLine].substr(0, cursorColumn);
                            lines.insert(lines.begin() + cursorLine + 1, remaining);
                            cursorLine++;
                            cursorColumn = 0;
                            return true;
                        } else if (event.keyCode == VK_BACK) {
                            if (cursorColumn > 0) {
                                lines[cursorLine].erase(cursorColumn - 1, 1);
                                cursorColumn--;
                                return true;
                            } else if (cursorLine > 0) {
                                cursorColumn = static_cast<int>(lines[cursorLine-1].length());
                                lines[cursorLine-1] += lines[cursorLine];
                                lines.erase(lines.begin() + cursorLine);
                                cursorLine--;
                                return true;
                            }
                        }
                    }
                    break;

                default:
                    break;
            }
            return false;
        }

        void update(float deltaTime) override {
            blinkTimer += deltaTime;
        }
    };

private:
    std::shared_ptr<ArabicGraphics> graphics;
    std::vector<std::shared_ptr<UIElement>> elements;
    std::unordered_map<std::string, std::shared_ptr<UIElement>> elementMap;

    // Event handling
    std::queue<UIEvent> eventQueue;
    std::shared_ptr<UIElement> focusedElement;
    std::shared_ptr<UIElement> hoveredElement;

    // Layout system
    TextDirection layoutDirection;
    float designWidth;
    float designHeight;

    // Window handle for input
    HWND windowHandle;

    // Performance stats
    struct UIStats {
        int elementCount;
        int renderTime;
        int eventProcessTime;

        UIStats() : elementCount(0), renderTime(0), eventProcessTime(0) {}
    } stats;

public:
    ArabicUI();
    ~ArabicUI() = default;

    // منع النسخ والتعيين
    ArabicUI(const ArabicUI&) = delete;
    ArabicUI& operator=(const ArabicUI&) = delete;

    /**
     * @brief تهيئة نظام UI
     */
    bool initialize(std::shared_ptr<ArabicGraphics> graphicsPtr, HWND hwnd = nullptr);

    /**
     * @brief إنهاء نظام UI
     */
    void shutdown();

    /**
     * @brief إضافة عنصر UI
     */
    void addElement(std::shared_ptr<UIElement> element);

    /**
     * @brief إزالة عنصر UI
     */
    void removeElement(const std::string& elementId);

    /**
     * @brief الحصول على عنصر UI
     */
    std::shared_ptr<UIElement> getElement(const std::string& elementId) const;

    /**
     * @brief تحديث جميع عناصر UI
     */
    void update(float deltaTime);

    /**
     * @brief رسم جميع عناصر UI
     */
    void render(std::shared_ptr<ArabicGraphics> graphicsPtr = nullptr);

    /**
     * @brief معالجة حدث
     */
    void processEvent(const UIEvent& event);

    /**
     * @brief معالجة أحداث Windows
     */
    void processWindowsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief إنشاء زر
     */
    std::shared_ptr<UIButton> createButton(const std::string& id, const std::string& text,
                                         const UIRect& bounds);

    /**
     * @brief إنشاء مربع نص
     */
    std::shared_ptr<UITextBox> createTextBox(const std::string& id, const std::string& placeholder,
                                           const UIRect& bounds);

    /**
     * @brief إنشاء قائمة منسدلة
     */
    std::shared_ptr<UIDropdown> createDropdown(const std::string& id, const UIRect& bounds);

    /**
     * @brief إنشاء ملصق نصي
     */
    std::shared_ptr<UILabel> createLabel(const std::string& id, const std::string& text,
                                       const UIRect& bounds);

    /**
     * @brief إنشاء منطقة نصية
     */
    std::shared_ptr<UITextArea> createTextArea(const std::string& id, const UIRect& bounds);

    /**
     * @brief إعداد تخطيط RTL
     */
    void setLayoutDirection(TextDirection direction);

    /**
     * @brief تحويل إحداثيات RTL
     */
    UIRect convertToRTL(const UIRect& rect) const;

    /**
     * @brief الحصول على إحصائيات UI
     */
    const UIStats& getUIStats() const { return stats; }

    /**
     * @brief تحميل خط عربي
     */
    bool loadArabicFont(const std::string& fontName, float size);

    /**
     * @brief إعداد ألوان UI الافتراضية
     */
    void setDefaultTheme();

    /**
     * @brief حفظ تخطيط UI
     */
    bool saveLayout(const std::string& filePath) const;

    /**
     * @brief تحميل تخطيط UI
     */
    bool loadLayout(const std::string& filePath);

    /**
     * @brief إنشاء تخطيط متجاوب
     */
    void makeResponsive(float screenWidth, float screenHeight);
};

// ────────────────────────────────────────────────────────
// دوال مساعدة للـ UI
// ────────────────────────────────────────────────────────

namespace UIHelpers {

    /**
     * @brief إنشاء تخطيط RTL
     */
    ArabicUI::UIRect createRTLLayout(float screenWidth, const ArabicUI::UIRect& original);

    /**
     * @brief قياس نص عربي
     */
    glm::vec2 measureArabicText(const std::string& text, const ArabicUI::UIFont& font);

    /**
     * @brief فصل النص العربي إلى كلمات
     */
    std::vector<std::string> splitArabicText(const std::string& text);

    /**
     * @brief التحقق من اتجاه النص
     */
    ArabicUI::TextDirection detectTextDirection(const std::string& text);

    /**
     * @brief عكس تخطيط RTL
     */
    ArabicUI::UIRect mirrorRTL(const ArabicUI::UIRect& rect, float screenWidth);

    /**
     * @brief إنشاء ألوان UI قياسية
     */
    ArabicUI::UIColor getStandardColor(const std::string& colorName);

    /**
     * @brief تحويل أحداث Windows إلى أحداث UI
     */
    ArabicUI::UIEvent windowsMessageToUIEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief إنشاء تأثيرات بصرية للـ UI
     */
    void createUIEffects(ArabicGraphics& graphics, const ArabicUI::UIElement& element,
                        ArabicUI::UIState state);

}

} // namespace ArabicLanguage
