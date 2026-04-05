# 📖 دليل التنفيذ: الاستغناء التدريجي عن C++

## 🎯 الهدف

هذا الدليل يوضح كيفية تنفيذ خطة الاستغناء التدريجي عن C++ خطوة بخطوة.

---

## 📋 الخطوة 1: إنشاء Bridge للمحلل اللغوي

### 1.1 إنشاء الملفات الأساسية ✅

تم إنشاء:
- `src/core/bridge/ArabicLexerBridge.h` ✅
- `src/core/bridge/ArabicLexerBridge.cpp` ✅

### 1.2 الخطوات التالية:

#### أ) تحميل المحلل_اللغوي.عربي في Runtime

```cpp
// في ArabicLexerBridge.cpp
void ArabicLexerBridge::initializeLexer() {
    // 1. قراءة ملف المحلل_اللغوي.عربي
    std::string lexerCode = readFile("اللغة_العربية/المحلل_اللغوي.عربي");
    
    // 2. تحميله في Runtime
    runtime->loadArabicCode(lexerCode);
    
    // 3. تنفيذه لتهيئة المحلل
    executor->execute(lexerCode);
}
```

#### ب) استدعاء دالة "حلل" من المحلل العربي

```cpp
std::vector<Token> ArabicLexerBridge::tokenize(const std::string& sourceCode) {
    // 1. استدعاء دالة "حلل" من المحلل_اللغوي.عربي
    Value result = runtime->callFunction("حلل", sourceCode);
    
    // 2. تحويل النتيجة إلى vector<Token>
    return convertToTokens(result);
}
```

#### ج) تحويل النتيجة إلى Tokens

```cpp
std::vector<Token> convertToTokens(const Value& result) {
    std::vector<Token> tokens;
    
    if (result.type == ValueType::ARRAY) {
        for (const auto& tokenValue : result.elements) {
            Token token(
                tokenValue.map_elements.at("نوع").string_value,
                tokenValue.map_elements.at("قيمة").string_value,
                (int)valueToNumber(tokenValue.map_elements.at("سطر"))
            );
            tokens.push_back(token);
        }
    }
    
    return tokens;
}
```

---

## 📋 الخطوة 2: دمج Bridge مع ArabicParser

### 2.1 تعديل ArabicParser.cpp

```cpp
// في ArabicParser.cpp
#include "bridge/ArabicLexerBridge.h"

class ArabicParser {
private:
    std::unique_ptr<ArabicLexerBridge> lexerBridge;
    bool useArabicLexer = true; // ✅ علم لاستخدام المحلل العربي
    
public:
    ArabicParser() {
        if (useArabicLexer) {
            lexerBridge = std::make_unique<ArabicLexerBridge>();
            if (!lexerBridge->isReady()) {
                useArabicLexer = false; // الرجوع للمحلل C++
            }
        }
    }
    
    std::vector<Token> tokenize(const std::string& code) {
        if (useArabicLexer && lexerBridge) {
            return lexerBridge->tokenize(code); // ✅ استخدام المحلل العربي
        }
        return tokenizeCpp(code); // استخدام المحلل C++ القديم
    }
};
```

---

## 📋 الخطوة 3: الاختبار

### 3.1 اختبار Bridge

```cpp
// test_lexer_bridge.cpp
void testLexerBridge() {
    ArabicLexerBridge bridge;
    
    assert(bridge.isReady() == true);
    
    std::string code = "مت س = 10";
    auto tokens = bridge.tokenize(code);
    
    assert(tokens.size() > 0);
    assert(tokens[0].type == "كلمة_مفتاحية");
    assert(tokens[0].value == "مت");
}
```

### 3.2 اختبار التكامل

```cpp
// test_integration.cpp
void testIntegration() {
    ArabicParser parser;
    auto tokens = parser.tokenize("مت س = 10");
    
    // التحقق من أن النتيجة صحيحة
    assert(tokens.size() == 5); // مت، س، =، 10، \n
}
```

---

## 📋 الخطوة 4: الاستبدال التدريجي

### 4.1 المرحلة 1: التشغيل المتوازي

```cpp
// تشغيل المحلل العربي والمحلل C++ معاً
auto tokens_arabic = lexerBridge->tokenize(code);
auto tokens_cpp = tokenizeCpp(code);

// مقارنة النتائج
assert(tokens_arabic == tokens_cpp);
```

### 4.2 المرحلة 2: الاستبدال الكامل

```cpp
// بعد التأكد من صحة النتائج
if (useArabicLexer) {
    return lexerBridge->tokenize(code);
}
// إزالة tokenizeCpp تدريجياً
```

---

## 🔧 أدوات مساعدة

### 1. دالة قراءة الملف

```cpp
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
```

### 2. دالة تحويل Value إلى Token

```cpp
Token valueToToken(const Value& v) {
    return Token(
        v.map_elements.at("نوع").string_value,
        v.map_elements.at("قيمة").string_value,
        (int)valueToNumber(v.map_elements.at("سطر"))
    );
}
```

---

## ⚠️ ملاحظات مهمة

1. **النسخ الاحتياطي**: احتفظ بنسخة من الكود C++ القديم
2. **الاختبار المستمر**: اختبر بعد كل تغيير
3. **الأداء**: راقب الأداء وقارنه مع النسخة C++
4. **التوثيق**: وثق كل خطوة

---

## 📊 مؤشرات التقدم

- [ ] Bridge للمحلل اللغوي مكتمل
- [ ] اختبارات الوحدة ناجحة
- [ ] اختبارات التكامل ناجحة
- [ ] الاستبدال الكامل للمحلل اللغوي
- [ ] لا تدهور في الأداء

---

*ابدأ بالخطوة 1.2: تحميل المحلل_اللغوي.عربي في Runtime*

