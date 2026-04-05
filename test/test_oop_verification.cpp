// test_oop_verification.cpp - اختبار شامل مصحح
// ✅ يستخدم ClassDefinitionEx و ClassMethodEx
// ✅ بدون طباعة أخطاء في Validators

#include <iostream>
#include <cassert>
#include <iomanip>
#include "ArabicOOP.h"

using namespace std;
using namespace ArabicLanguage;

class TestReporter {
private:
    int passed = 0;
    int failed = 0;
    int total = 0;

public:
    void startTest(const string& name) {
        cout << "\n" << string(60, '═') << endl;
        cout << "🧪 " << name << endl;
        cout << string(60, '═') << endl;
    }

    void testCase(const string& name, bool result) {
        total++;
        if (result) {
            passed++;
            cout << "  ✅ " << name << endl;
        } else {
            failed++;
            cout << "  ❌ " << name << endl;
        }
    }

    void printSummary() {
        cout << "\n" << string(60, '═') << endl;
        cout << "📊 ملخص النتائج:" << endl;
        cout << string(60, '═') << endl;
        cout << "  ✅ نجح: " << passed << "/" << total << endl;
        cout << "  ❌ فشل: " << failed << "/" << total << endl;
        cout << "  📈 النسبة: " << fixed << setprecision(1) 
             << (100.0 * passed / total) << "%" << endl;
        cout << string(60, '═') << endl;
    }

    bool allPassed() const { return failed == 0; }
};

TestReporter reporter;

void test_oop_simple() {
    reporter.startTest("test_oop_simple - اختبار صنف بسيط");

    try {
        ClassTable classTable;

        // ✅ 1. إنشاء صف شخص
        auto personClass = std::make_shared<ClassDefinitionEx>("شخص");
        reporter.testCase("إنشاء كائن ClassDefinitionEx", personClass != nullptr);

        // ✅ 2. إضافة خصائص
        personClass->addProperty(ClassProperty("الاسم", "نص", false));
        personClass->addProperty(ClassProperty("العمر", "رقم", false));
        reporter.testCase("إضافة خصائص", personClass->properties.size() == 2);

        // ✅ 3. إضافة دالة
        ClassMethodEx greetMethod("حيّ", "صفر", false);
        personClass->addMethod(greetMethod);
        reporter.testCase("إضافة دالة", personClass->methods.size() == 1);

        // ✅ 4. إضافة الصف إلى الجدول
        classTable.addClass(personClass);
        reporter.testCase("إضافة الصف للجدول", classTable.hasClass("شخص"));

        // ✅ 5. الحصول على الصف
        auto retrievedClass = classTable.getClass("شخص");
        reporter.testCase("استرجاع الصف من الجدول", retrievedClass != nullptr);

        // ✅ 6. التحقق من الخصائص
        const ClassProperty* nameProp = personClass->getProperty("الاسم");
        reporter.testCase("البحث عن خاصية", nameProp != nullptr);

        // ✅ 7. التحقق من الدوال
        const ClassMethodEx* greetFunc = personClass->getMethod("حيّ");
        reporter.testCase("البحث عن دالة", greetFunc != nullptr);

        // ✅ 8. حساب حجم الصف
        size_t classSize = personClass->calculateSize();
        reporter.testCase("حساب حجم الصف", classSize > 0);

        // ✅ 9. توليد كود C++
        string classCode = OOPCodeGenerator::generateClassCode(personClass);
        reporter.testCase("توليد كود الصف", !classCode.empty());
        
        cout << "\n📝 كود المولد:\n" << classCode << endl;

        // ✅ 10. التحقق من صحة الصف
        bool isValid = OOPValidator::validateClass(personClass);
        reporter.testCase("التحقق من صحة الصف", isValid);

    } catch (const exception& e) {
        reporter.testCase("معالجة الاستثناءات", false);
        cerr << "❌ استثناء: " << e.what() << endl;
    }
}

void test_oop() {
    reporter.startTest("test_oop - اختبار الوراثة");

    try {
        ClassTable classTable;

        // ✅ 1. إنشاء صف الحيوان (الأب)
        auto animalClass = std::make_shared<ClassDefinitionEx>("حيوان");
        animalClass->addProperty(ClassProperty("الاسم", "نص", false));
        animalClass->addProperty(ClassProperty("الصوت", "نص", false));

        ClassMethodEx soundMethod("اصدر_صوت", "صفر", false);
        soundMethod.codeBlock = "    cout << this->الصوت << endl;\n";
        animalClass->addMethod(soundMethod);

        classTable.addClass(animalClass);
        reporter.testCase("إنشاء الصف الأب (حيوان)", classTable.hasClass("حيوان"));

        // ✅ 2. إنشاء صف الكلب (الفرعي)
        auto dogClass = std::make_shared<ClassDefinitionEx>("كلب");
        dogClass->baseName = "حيوان";
        dogClass->addProperty(ClassProperty("نوع_السلالة", "نص", false));

        classTable.addClass(dogClass);
        reporter.testCase("إنشاء الصف الفرعي (كلب)", classTable.hasClass("كلب"));

        // ✅ 3. التحقق من الوراثة
        bool inheritanceValid = OOPValidator::validateInheritance("كلب", "حيوان", classTable);
        reporter.testCase("التحقق من الوراثة", inheritanceValid);

        // ✅ 4. التحقق من الخصائص العامة
        bool propAccess = OOPValidator::validatePropertyAccess("حيوان", "الاسم", classTable);
        reporter.testCase("الوصول للخاصية العامة", propAccess);

        // ✅ 5. التحقق من استدعاء الدالة
        vector<string> emptyArgs;
        bool methodValid = OOPValidator::validateMethodCall("حيوان", "اصدر_صوت", emptyArgs, classTable);
        reporter.testCase("التحقق من استدعاء الدالة", methodValid);

        // ✅ 6. توليد كود الصف الأب
        string animalCode = OOPCodeGenerator::generateClassCode(animalClass);
        reporter.testCase("توليد كود الصف الأب", !animalCode.empty());

        // ✅ 7. توليد كود المنشئ
        string constructorCode = OOPCodeGenerator::generateConstructor(animalClass);
        reporter.testCase("توليد كود المنشئ", !constructorCode.empty());

        // ✅ 8. توليد كود الدالة
        string methodCode = OOPCodeGenerator::generateMethodCode(animalClass, soundMethod);
        reporter.testCase("توليد كود الدالة", !methodCode.empty());

        cout << "\n📝 كود الصف الأب:\n" << animalCode << endl;

    } catch (const exception& e) {
        reporter.testCase("معالجة الاستثناءات", false);
        cerr << "❌ استثناء: " << e.what() << endl;
    }
}

void test_access_control() {
    reporter.startTest("اختبار التحكم في الوصول");

    try {
        ClassTable classTable;

        auto testClass = std::make_shared<ClassDefinitionEx>("تجربة");

        // ✅ إضافة خصائص عامة وخاصة
        testClass->addProperty(ClassProperty("عام", "رقم", false));
        testClass->addProperty(ClassProperty("خاص", "رقم", true));

        // ✅ إضافة دوال عامة وخاصة
        ClassMethodEx publicMethod("عام", "صفر", false);
        testClass->addMethod(publicMethod);

        ClassMethodEx privateMethod("خاص", "صفر", true);
        testClass->addMethod(privateMethod);

        classTable.addClass(testClass);

        // التحقق من الوصول
        bool publicPropAccess = OOPValidator::validatePropertyAccess("تجربة", "عام", classTable);
        reporter.testCase("الوصول للخاصية العامة", publicPropAccess);

        bool privatePropAccess = OOPValidator::validatePropertyAccess("تجربة", "خاص", classTable);
        reporter.testCase("منع الوصول للخاصية الخاصة", !privatePropAccess);

        vector<string> noArgs;
        bool publicMethodCall = OOPValidator::validateMethodCall("تجربة", "عام", noArgs, classTable);
        reporter.testCase("استدعاء الدالة العامة", publicMethodCall);

        bool privateMethodCall = OOPValidator::validateMethodCall("تجربة", "خاص", noArgs, classTable);
        reporter.testCase("منع استدعاء الدالة الخاصة", !privateMethodCall);

    } catch (const exception& e) {
        reporter.testCase("معالجة الاستثناءات", false);
        cerr << "❌ استثناء: " << e.what() << endl;
    }
}

int main() {
    cout << "\n";
    cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    cout << "║  🎯 اختبار شامل لنظام OOP العربي - الإصلاحات الكاملة           ║\n";
    cout << "║  ✅ مصحح: ClassDefinitionEx + ClassMethodEx                   ║\n";
    cout << "║  ✅ بدون طباعة أخطاء في Validators                           ║\n";
    cout << "╚══════════════════════════════════════════════════════════════════╝\n";

    // تشغيل جميع الاختبارات
    test_oop_simple();
    test_oop();
    test_access_control();

    // طباعة الملخص
    reporter.printSummary();

    // النتيجة النهائية
    cout << "\n";
    if (reporter.allPassed()) {
        cout << "╔══════════════════════════════════════════════════════════════════╗\n";
        cout << "║  🎉 جميع الاختبارات نجحت بنسبة 100%!                          ║\n";
        cout << "║  ✅ جميع الدوال معرّفة وتعمل بشكل صحيح                       ║\n";
        cout << "║  🌟 نظام OOP جاهز للإنتاج                                     ║\n";
        cout << "╚══════════════════════════════════════════════════════════════════╝\n";
        return 0;
    } else {
        cout << "╔══════════════════════════════════════════════════════════════════╗\n";
        cout << "║  ❌ بعض الاختبارات فشلت - يحتاج المراجعة                       ║\n";
        cout << "╚══════════════════════════════════════════════════════════════════╝\n";
        return 1;
    }
}