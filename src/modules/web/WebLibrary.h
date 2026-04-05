// WebLibrary.h - مكتبة الويب الشاملة
// الأسبوع الثاني من الشهر الخامس: مكتبة الويب
#ifndef WEB_LIBRARY_H
#define WEB_LIBRARY_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace ArabicLanguage {

// ==================== HTML Generator ====================

// عنصر HTML
class HTMLElement {
private:
    std::string tagName;
    std::map<std::string, std::string> attributes;
    std::string content;
    std::vector<std::shared_ptr<HTMLElement>> children;
    bool selfClosing;

public:
    HTMLElement(const std::string& tag);
    ~HTMLElement() = default;
    
    // إدارة السمات
    HTMLElement& setAttribute(const std::string& name, const std::string& value);
    HTMLElement& setId(const std::string& id);
    HTMLElement& setClass(const std::string& className);
    HTMLElement& setStyle(const std::string& style);
    
    // إدارة المحتوى
    HTMLElement& setText(const std::string& text);
    HTMLElement& setHTML(const std::string& html);
    HTMLElement& addChild(std::shared_ptr<HTMLElement> child);
    HTMLElement& addChild(const std::string& tag, const std::string& content = "");
    
    // توليد HTML
    std::string toString(int indent = 0) const;
    std::string toMinifiedString() const;
    
    // Factory methods
    static std::shared_ptr<HTMLElement> create(const std::string& tag);
    static std::shared_ptr<HTMLElement> div(const std::string& content = "");
    static std::shared_ptr<HTMLElement> span(const std::string& content = "");
    static std::shared_ptr<HTMLElement> p(const std::string& content = "");
    static std::shared_ptr<HTMLElement> h1(const std::string& content = "");
    static std::shared_ptr<HTMLElement> h2(const std::string& content = "");
    static std::shared_ptr<HTMLElement> a(const std::string& href, const std::string& content = "");
    static std::shared_ptr<HTMLElement> img(const std::string& src, const std::string& alt = "");
    static std::shared_ptr<HTMLElement> button(const std::string& content = "");
    static std::shared_ptr<HTMLElement> input(const std::string& type = "text");
    static std::shared_ptr<HTMLElement> form(const std::string& action = "");
};

// مستند HTML
class HTMLDocument {
private:
    std::string title;
    std::string lang;
    std::map<std::string, std::string> metaTags;
    std::vector<std::string> stylesheets;
    std::vector<std::string> scripts;
    std::shared_ptr<HTMLElement> body;
    std::shared_ptr<HTMLElement> head;

public:
    HTMLDocument();
    ~HTMLDocument() = default;
    
    // إعدادات المستند
    HTMLDocument& setTitle(const std::string& t);
    HTMLDocument& setLang(const std::string& l);
    HTMLDocument& addMeta(const std::string& name, const std::string& content);
    HTMLDocument& addStylesheet(const std::string& href);
    HTMLDocument& addScript(const std::string& src);
    HTMLDocument& setBody(std::shared_ptr<HTMLElement> bodyElement);
    HTMLDocument& addToHead(std::shared_ptr<HTMLElement> element);
    
    // توليد HTML
    std::string toString() const;
    std::string toMinifiedString() const;
    
    // Factory
    static std::unique_ptr<HTMLDocument> create();
};

// ==================== CSS Generator ====================

// قاعدة CSS
class CSSRule {
private:
    std::string selector;
    std::map<std::string, std::string> properties;

public:
    CSSRule(const std::string& sel);
    ~CSSRule() = default;
    
    CSSRule& setProperty(const std::string& name, const std::string& value);
    CSSRule& setColor(const std::string& color);
    CSSRule& setBackgroundColor(const std::string& color);
    CSSRule& setFontSize(const std::string& size);
    CSSRule& setMargin(const std::string& margin);
    CSSRule& setPadding(const std::string& padding);
    CSSRule& setWidth(const std::string& width);
    CSSRule& setHeight(const std::string& height);
    CSSRule& setDisplay(const std::string& display);
    CSSRule& setFlexDirection(const std::string& direction);
    CSSRule& setJustifyContent(const std::string& justify);
    CSSRule& setAlignItems(const std::string& align);
    CSSRule& setTextAlign(const std::string& align);
    
    std::string toString(int indent = 0) const;
};

// ورقة أنماط CSS
class Stylesheet {
private:
    std::vector<CSSRule> rules;
    std::vector<std::string> imports;
    std::vector<std::string> mediaQueries;

public:
    Stylesheet();
    ~Stylesheet() = default;
    
    Stylesheet& addRule(const CSSRule& rule);
    Stylesheet& addRule(const std::string& selector);
    CSSRule& getRule(const std::string& selector);
    Stylesheet& addImport(const std::string& url);
    Stylesheet& addMediaQuery(const std::string& query, const std::vector<CSSRule>& rules);
    
    std::string toString() const;
    std::string toMinifiedString() const;
    
    static std::unique_ptr<Stylesheet> create();
};

// ==================== JavaScript Integration ====================

// مولد JavaScript
class JavaScriptGenerator {
private:
    std::vector<std::string> code;
    int indentLevel;

public:
    JavaScriptGenerator();
    ~JavaScriptGenerator() = default;
    
    // دوال
    JavaScriptGenerator& function(const std::string& name, const std::vector<std::string>& params);
    JavaScriptGenerator& arrowFunction(const std::string& name, const std::vector<std::string>& params);
    JavaScriptGenerator& addLine(const std::string& line);
    JavaScriptGenerator& addComment(const std::string& comment);
    
    // DOM manipulation
    JavaScriptGenerator& selectElement(const std::string& selector, const std::string& varName);
    JavaScriptGenerator& addEventListener(const std::string& element, const std::string& event, const std::string& handler);
    JavaScriptGenerator& setInnerHTML(const std::string& element, const std::string& html);
    JavaScriptGenerator& setTextContent(const std::string& element, const std::string& text);
    
    // AJAX/Fetch
    JavaScriptGenerator& fetchRequest(const std::string& url, const std::string& method, const std::string& handler);
    JavaScriptGenerator& ajaxRequest(const std::string& url, const std::string& method, const std::string& handler);
    
    // توليد الكود
    std::string toString() const;
    std::string toMinifiedString() const;
    
    void reset();
    
    static std::unique_ptr<JavaScriptGenerator> create();
};

// ==================== WebAssembly Support ====================

// معلومات WebAssembly
struct WebAssemblyInfo {
    std::string moduleName;
    std::string wasmFile;
    std::string jsLoader;
    std::map<std::string, std::string> exports;
    std::map<std::string, std::string> imports;
    
    WebAssemblyInfo() {}
};

// مولد WebAssembly
class WebAssemblyGenerator {
private:
    WebAssemblyInfo info;
    std::vector<std::string> initializationCode;

public:
    WebAssemblyGenerator();
    ~WebAssemblyGenerator() = default;
    
    WebAssemblyGenerator& setModuleName(const std::string& name);
    WebAssemblyGenerator& setWasmFile(const std::string& file);
    WebAssemblyGenerator& addExport(const std::string& name, const std::string& type);
    WebAssemblyGenerator& addImport(const std::string& name, const std::string& module, const std::string& type);
    WebAssemblyGenerator& addInitialization(const std::string& code);
    
    std::string generateLoader() const;
    std::string generateWrapper() const;
    
    static std::unique_ptr<WebAssemblyGenerator> create();
};

// ==================== Web Library Manager ====================

// مدير مكتبة الويب الشاملة
class WebLibrary {
private:
    std::unique_ptr<HTMLDocument> currentDocument;
    std::unique_ptr<Stylesheet> currentStylesheet;
    std::unique_ptr<JavaScriptGenerator> currentJS;
    std::map<std::string, std::unique_ptr<WebAssemblyGenerator>> wasmModules;

public:
    WebLibrary();
    ~WebLibrary() = default;
    
    // إدارة المستندات
    HTMLDocument& createDocument();
    HTMLDocument& getDocument();
    Stylesheet& createStylesheet();
    Stylesheet& getStylesheet();
    JavaScriptGenerator& createJavaScript();
    JavaScriptGenerator& getJavaScript();
    
    // WebAssembly
    WebAssemblyGenerator& createWebAssemblyModule(const std::string& name);
    WebAssemblyGenerator& getWebAssemblyModule(const std::string& name);
    
    // توليد صفحة كاملة
    std::string generatePage() const;
    void savePage(const std::string& filename) const;
    
    // Factory
    static std::unique_ptr<WebLibrary> create();
};

} // namespace ArabicLanguage

#endif // WEB_LIBRARY_H

