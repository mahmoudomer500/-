// WebLibrary.cpp - تطبيق مكتبة الويب الشاملة
// الأسبوع الثاني من الشهر الخامس: مكتبة الويب
#include "WebLibrary.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

namespace ArabicLanguage {

// ==================== HTMLElement Implementation ====================

HTMLElement::HTMLElement(const std::string& tag) : tagName(tag), selfClosing(false) {
    // تحديد العناصر ذاتية الإغلاق
    std::vector<std::string> selfClosingTags = {"img", "br", "hr", "input", "meta", "link"};
    selfClosing = std::find(selfClosingTags.begin(), selfClosingTags.end(), tag) != selfClosingTags.end();
}

HTMLElement& HTMLElement::setAttribute(const std::string& name, const std::string& value) {
    attributes[name] = value;
    return *this;
}

HTMLElement& HTMLElement::setId(const std::string& id) {
    return setAttribute("id", id);
}

HTMLElement& HTMLElement::setClass(const std::string& className) {
    return setAttribute("class", className);
}

HTMLElement& HTMLElement::setStyle(const std::string& style) {
    return setAttribute("style", style);
}

HTMLElement& HTMLElement::setText(const std::string& text) {
    content = text;
    return *this;
}

HTMLElement& HTMLElement::setHTML(const std::string& html) {
    content = html;
    return *this;
}

HTMLElement& HTMLElement::addChild(std::shared_ptr<HTMLElement> child) {
    children.push_back(child);
    return *this;
}

HTMLElement& HTMLElement::addChild(const std::string& tag, const std::string& content) {
    auto child = create(tag);
    if (!content.empty()) {
        child->setText(content);
    }
    children.push_back(child);
    return *this;
}

std::string HTMLElement::toString(int indent) const {
    std::stringstream ss;
    std::string indentStr(indent * 2, ' ');
    
    ss << indentStr << "<" << tagName;
    
    for (const auto& attr : attributes) {
        ss << " " << attr.first << "=\"" << attr.second << "\"";
    }
    
    if (selfClosing) {
        ss << " />";
        return ss.str();
    }
    
    ss << ">";
    
    if (!content.empty() && children.empty()) {
        ss << content;
    } else {
        if (!content.empty()) {
            ss << "\n" << indentStr << "  " << content;
        }
        for (const auto& child : children) {
            ss << "\n" << child->toString(indent + 1);
        }
        if (!children.empty()) {
            ss << "\n" << indentStr;
        }
    }
    
    ss << "</" << tagName << ">";
    
    return ss.str();
}

std::string HTMLElement::toMinifiedString() const {
    std::stringstream ss;
    
    ss << "<" << tagName;
    for (const auto& attr : attributes) {
        ss << " " << attr.first << "=\"" << attr.second << "\"";
    }
    
    if (selfClosing) {
        ss << " />";
        return ss.str();
    }
    
    ss << ">";
    
    if (!content.empty() && children.empty()) {
        ss << content;
    } else {
        if (!content.empty()) {
            ss << content;
        }
        for (const auto& child : children) {
            ss << child->toMinifiedString();
        }
    }
    
    ss << "</" << tagName << ">";
    
    return ss.str();
}

std::shared_ptr<HTMLElement> HTMLElement::create(const std::string& tag) {
    return std::make_shared<HTMLElement>(tag);
}

std::shared_ptr<HTMLElement> HTMLElement::div(const std::string& content) {
    auto elem = create("div");
    if (!content.empty()) {
        elem->setText(content);
    }
    return elem;
}

std::shared_ptr<HTMLElement> HTMLElement::span(const std::string& content) {
    auto elem = create("span");
    if (!content.empty()) {
        elem->setText(content);
    }
    return elem;
}

std::shared_ptr<HTMLElement> HTMLElement::p(const std::string& content) {
    auto elem = create("p");
    if (!content.empty()) {
        elem->setText(content);
    }
    return elem;
}

std::shared_ptr<HTMLElement> HTMLElement::h1(const std::string& content) {
    auto elem = create("h1");
    if (!content.empty()) {
        elem->setText(content);
    }
    return elem;
}

std::shared_ptr<HTMLElement> HTMLElement::h2(const std::string& content) {
    auto elem = create("h2");
    if (!content.empty()) {
        elem->setText(content);
    }
    return elem;
}

std::shared_ptr<HTMLElement> HTMLElement::a(const std::string& href, const std::string& content) {
    auto elem = create("a");
    elem->setAttribute("href", href);
    if (!content.empty()) {
        elem->setText(content);
    }
    return elem;
}

std::shared_ptr<HTMLElement> HTMLElement::img(const std::string& src, const std::string& alt) {
    auto elem = create("img");
    elem->setAttribute("src", src);
    elem->setAttribute("alt", alt);
    return elem;
}

std::shared_ptr<HTMLElement> HTMLElement::button(const std::string& content) {
    auto elem = create("button");
    if (!content.empty()) {
        elem->setText(content);
    }
    return elem;
}

std::shared_ptr<HTMLElement> HTMLElement::input(const std::string& type) {
    auto elem = create("input");
    elem->setAttribute("type", type);
    return elem;
}

std::shared_ptr<HTMLElement> HTMLElement::form(const std::string& action) {
    auto elem = create("form");
    if (!action.empty()) {
        elem->setAttribute("action", action);
    }
    return elem;
}

// ==================== HTMLDocument Implementation ====================

HTMLDocument::HTMLDocument() : lang("ar"), body(nullptr), head(nullptr) {
    title = "Untitled Document";
    head = HTMLElement::create("head");
}

HTMLDocument& HTMLDocument::setTitle(const std::string& t) {
    title = t;
    return *this;
}

HTMLDocument& HTMLDocument::setLang(const std::string& l) {
    lang = l;
    return *this;
}

HTMLDocument& HTMLDocument::addMeta(const std::string& name, const std::string& content) {
    metaTags[name] = content;
    return *this;
}

HTMLDocument& HTMLDocument::addStylesheet(const std::string& href) {
    stylesheets.push_back(href);
    return *this;
}

HTMLDocument& HTMLDocument::addScript(const std::string& src) {
    scripts.push_back(src);
    return *this;
}

HTMLDocument& HTMLDocument::setBody(std::shared_ptr<HTMLElement> bodyElement) {
    body = bodyElement;
    return *this;
}

HTMLDocument& HTMLDocument::addToHead(std::shared_ptr<HTMLElement> element) {
    head->addChild(element);
    return *this;
}

std::string HTMLDocument::toString() const {
    std::stringstream ss;
    
    ss << "<!DOCTYPE html>\n";
    ss << "<html lang=\"" << lang << "\">\n";
    
    // Head
    ss << "<head>\n";
    ss << "  <meta charset=\"UTF-8\">\n";
    ss << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    ss << "  <title>" << title << "</title>\n";
    
    for (const auto& meta : metaTags) {
        ss << "  <meta name=\"" << meta.first << "\" content=\"" << meta.second << "\">\n";
    }
    
    for (const auto& stylesheet : stylesheets) {
        ss << "  <link rel=\"stylesheet\" href=\"" << stylesheet << "\">\n";
    }
    
    // إضافة محتوى head الإضافي هنا إذا لزم الأمر
    
    ss << "</head>\n";
    
    // Body
    ss << "<body>\n";
    if (body) {
        ss << body->toString(1) << "\n";
    }
    ss << "</body>\n";
    
    // Scripts
    for (const auto& script : scripts) {
        ss << "<script src=\"" << script << "\"></script>\n";
    }
    
    ss << "</html>";
    
    return ss.str();
}

std::string HTMLDocument::toMinifiedString() const {
    // نسخة مبسطة
    return toString();
}

std::unique_ptr<HTMLDocument> HTMLDocument::create() {
    return std::make_unique<HTMLDocument>();
}

// ==================== CSSRule Implementation ====================

CSSRule::CSSRule(const std::string& sel) : selector(sel) {
}

CSSRule& CSSRule::setProperty(const std::string& name, const std::string& value) {
    properties[name] = value;
    return *this;
}

CSSRule& CSSRule::setColor(const std::string& color) {
    return setProperty("color", color);
}

CSSRule& CSSRule::setBackgroundColor(const std::string& color) {
    return setProperty("background-color", color);
}

CSSRule& CSSRule::setFontSize(const std::string& size) {
    return setProperty("font-size", size);
}

CSSRule& CSSRule::setMargin(const std::string& margin) {
    return setProperty("margin", margin);
}

CSSRule& CSSRule::setPadding(const std::string& padding) {
    return setProperty("padding", padding);
}

CSSRule& CSSRule::setWidth(const std::string& width) {
    return setProperty("width", width);
}

CSSRule& CSSRule::setHeight(const std::string& height) {
    return setProperty("height", height);
}

CSSRule& CSSRule::setDisplay(const std::string& display) {
    return setProperty("display", display);
}

CSSRule& CSSRule::setFlexDirection(const std::string& direction) {
    return setProperty("flex-direction", direction);
}

CSSRule& CSSRule::setJustifyContent(const std::string& justify) {
    return setProperty("justify-content", justify);
}

CSSRule& CSSRule::setAlignItems(const std::string& align) {
    return setProperty("align-items", align);
}

CSSRule& CSSRule::setTextAlign(const std::string& align) {
    return setProperty("text-align", align);
}

std::string CSSRule::toString(int indent) const {
    std::stringstream ss;
    std::string indentStr(indent * 2, ' ');
    
    ss << indentStr << selector << " {\n";
    for (const auto& prop : properties) {
        ss << indentStr << "  " << prop.first << ": " << prop.second << ";\n";
    }
    ss << indentStr << "}";
    
    return ss.str();
}

// ==================== Stylesheet Implementation ====================

Stylesheet::Stylesheet() {
}

Stylesheet& Stylesheet::addRule(const CSSRule& rule) {
    rules.push_back(rule);
    return *this;
}

Stylesheet& Stylesheet::addRule(const std::string& selector) {
    rules.push_back(CSSRule(selector));
    return *this;
}

CSSRule& Stylesheet::getRule(const std::string& selector) {
    for (auto& rule : rules) {
        if (rule.toString().find(selector) != std::string::npos) {
            return rule;
        }
    }
    // إنشاء قاعدة جديدة إذا لم توجد
    rules.push_back(CSSRule(selector));
    return rules.back();
}

Stylesheet& Stylesheet::addImport(const std::string& url) {
    imports.push_back(url);
    return *this;
}

Stylesheet& Stylesheet::addMediaQuery(const std::string& query, const std::vector<CSSRule>& queryRules) {
    // محاكاة media query
    return *this;
}

std::string Stylesheet::toString() const {
    std::stringstream ss;
    
    for (const auto& import : imports) {
        ss << "@import url(\"" << import << "\");\n";
    }
    
    for (const auto& rule : rules) {
        ss << rule.toString() << "\n\n";
    }
    
    return ss.str();
}

std::string Stylesheet::toMinifiedString() const {
    std::stringstream ss;
    
    for (const auto& rule : rules) {
        std::string ruleStr = rule.toString();
        // إزالة المسافات الزائدة
        ruleStr.erase(std::remove(ruleStr.begin(), ruleStr.end(), '\n'), ruleStr.end());
        ruleStr.erase(std::remove(ruleStr.begin(), ruleStr.end(), ' '), ruleStr.end());
        ss << ruleStr;
    }
    
    return ss.str();
}

std::unique_ptr<Stylesheet> Stylesheet::create() {
    return std::make_unique<Stylesheet>();
}

// ==================== JavaScriptGenerator Implementation ====================

JavaScriptGenerator::JavaScriptGenerator() : indentLevel(0) {
}

JavaScriptGenerator& JavaScriptGenerator::function(const std::string& name, const std::vector<std::string>& params) {
    std::stringstream ss;
    ss << "function " << name << "(";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << params[i];
    }
    ss << ") {";
    code.push_back(ss.str());
    indentLevel++;
    return *this;
}

JavaScriptGenerator& JavaScriptGenerator::arrowFunction(const std::string& name, const std::vector<std::string>& params) {
    std::stringstream ss;
    ss << "const " << name << " = (";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << params[i];
    }
    ss << ") => {";
    code.push_back(ss.str());
    indentLevel++;
    return *this;
}

JavaScriptGenerator& JavaScriptGenerator::addLine(const std::string& line) {
    std::string indentStr(indentLevel * 2, ' ');
    code.push_back(indentStr + line);
    return *this;
}

JavaScriptGenerator& JavaScriptGenerator::addComment(const std::string& comment) {
    addLine("// " + comment);
    return *this;
}

JavaScriptGenerator& JavaScriptGenerator::selectElement(const std::string& selector, const std::string& varName) {
    addLine("const " + varName + " = document.querySelector(\"" + selector + "\");");
    return *this;
}

JavaScriptGenerator& JavaScriptGenerator::addEventListener(const std::string& element, const std::string& event, const std::string& handler) {
    addLine(element + ".addEventListener(\"" + event + "\", " + handler + ");");
    return *this;
}

JavaScriptGenerator& JavaScriptGenerator::setInnerHTML(const std::string& element, const std::string& html) {
    addLine(element + ".innerHTML = \"" + html + "\";");
    return *this;
}

JavaScriptGenerator& JavaScriptGenerator::setTextContent(const std::string& element, const std::string& text) {
    addLine(element + ".textContent = \"" + text + "\";");
    return *this;
}

JavaScriptGenerator& JavaScriptGenerator::fetchRequest(const std::string& url, const std::string& method, const std::string& handler) {
    addLine("fetch(\"" + url + "\", {");
    indentLevel++;
    addLine("method: \"" + method + "\",");
    addLine("headers: { 'Content-Type': 'application/json' }");
    indentLevel--;
    addLine("})");
    addLine(".then(response => response.json())");
    addLine(".then(data => " + handler + "(data));");
    return *this;
}

JavaScriptGenerator& JavaScriptGenerator::ajaxRequest(const std::string& url, const std::string& method, const std::string& handler) {
    addLine("const xhr = new XMLHttpRequest();");
    addLine("xhr.open(\"" + method + "\", \"" + url + "\");");
    addLine("xhr.onload = function() {");
    indentLevel++;
    addLine("if (xhr.status === 200) {");
    indentLevel++;
    addLine(handler + "(JSON.parse(xhr.responseText));");
    indentLevel--;
    addLine("}");
    indentLevel--;
    addLine("};");
    addLine("xhr.send();");
    return *this;
}

std::string JavaScriptGenerator::toString() const {
    std::stringstream ss;
    for (const auto& line : code) {
        ss << line << "\n";
    }
    return ss.str();
}

std::string JavaScriptGenerator::toMinifiedString() const {
    std::string result;
    for (const auto& line : code) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        result += trimmed;
    }
    return result;
}

void JavaScriptGenerator::reset() {
    code.clear();
    indentLevel = 0;
}

std::unique_ptr<JavaScriptGenerator> JavaScriptGenerator::create() {
    return std::make_unique<JavaScriptGenerator>();
}

// ==================== WebAssemblyGenerator Implementation ====================

WebAssemblyGenerator::WebAssemblyGenerator() {
}

WebAssemblyGenerator& WebAssemblyGenerator::setModuleName(const std::string& name) {
    info.moduleName = name;
    return *this;
}

WebAssemblyGenerator& WebAssemblyGenerator::setWasmFile(const std::string& file) {
    info.wasmFile = file;
    return *this;
}

WebAssemblyGenerator& WebAssemblyGenerator::addExport(const std::string& name, const std::string& type) {
    info.exports[name] = type;
    return *this;
}

WebAssemblyGenerator& WebAssemblyGenerator::addImport(const std::string& name, const std::string& module, const std::string& type) {
    info.imports[name] = module + "::" + type;
    return *this;
}

WebAssemblyGenerator& WebAssemblyGenerator::addInitialization(const std::string& code) {
    initializationCode.push_back(code);
    return *this;
}

std::string WebAssemblyGenerator::generateLoader() const {
    std::stringstream ss;
    
    ss << "async function load" << info.moduleName << "() {\n";
    ss << "  const wasmModule = await WebAssembly.instantiateStreaming(\n";
    ss << "    fetch('" << info.wasmFile << "')\n";
    ss << "  );\n";
    ss << "  return wasmModule.instance.exports;\n";
    ss << "}\n";
    
    return ss.str();
}

std::string WebAssemblyGenerator::generateWrapper() const {
    std::stringstream ss;
    
    ss << "const " << info.moduleName << "Wrapper = {\n";
    ss << "  module: null,\n";
    ss << "  async init() {\n";
    ss << "    this.module = await load" << info.moduleName << "();\n";
    ss << "  },\n";
    
    for (const auto& exp : info.exports) {
        ss << "  " << exp.first << "(...args) {\n";
        ss << "    return this.module." << exp.first << "(...args);\n";
        ss << "  },\n";
    }
    
    ss << "};\n";
    
    return ss.str();
}

std::unique_ptr<WebAssemblyGenerator> WebAssemblyGenerator::create() {
    return std::make_unique<WebAssemblyGenerator>();
}

// ==================== WebLibrary Implementation ====================

WebLibrary::WebLibrary() {
}

HTMLDocument& WebLibrary::createDocument() {
    currentDocument = HTMLDocument::create();
    return *currentDocument;
}

HTMLDocument& WebLibrary::getDocument() {
    if (!currentDocument) {
        createDocument();
    }
    return *currentDocument;
}

Stylesheet& WebLibrary::createStylesheet() {
    currentStylesheet = Stylesheet::create();
    return *currentStylesheet;
}

Stylesheet& WebLibrary::getStylesheet() {
    if (!currentStylesheet) {
        createStylesheet();
    }
    return *currentStylesheet;
}

JavaScriptGenerator& WebLibrary::createJavaScript() {
    currentJS = JavaScriptGenerator::create();
    return *currentJS;
}

JavaScriptGenerator& WebLibrary::getJavaScript() {
    if (!currentJS) {
        createJavaScript();
    }
    return *currentJS;
}

WebAssemblyGenerator& WebLibrary::createWebAssemblyModule(const std::string& name) {
    wasmModules[name] = WebAssemblyGenerator::create();
    wasmModules[name]->setModuleName(name);
    return *wasmModules[name];
}

WebAssemblyGenerator& WebLibrary::getWebAssemblyModule(const std::string& name) {
    if (wasmModules.find(name) == wasmModules.end()) {
        createWebAssemblyModule(name);
    }
    return *wasmModules[name];
}

std::string WebLibrary::generatePage() const {
    std::stringstream ss;
    
    if (currentDocument) {
        ss << currentDocument->toString() << "\n";
    }
    
    if (currentStylesheet) {
        ss << "<style>\n" << currentStylesheet->toString() << "</style>\n";
    }
    
    if (currentJS) {
        ss << "<script>\n" << currentJS->toString() << "</script>\n";
    }
    
    return ss.str();
}

void WebLibrary::savePage(const std::string& filename) const {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << generatePage();
        file.close();
        std::cout << "[INFO] Page saved to: " << filename << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to save page to: " << filename << std::endl;
    }
}

std::unique_ptr<WebLibrary> WebLibrary::create() {
    return std::make_unique<WebLibrary>();
}

} // namespace ArabicLanguage

