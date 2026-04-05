// SimpleArabicTranslator.cpp - مترجم عربي مبسط للاختبار الأولي
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>

class SimpleArabicTranslator {
private:
    std::map<std::string, std::string> translations = {
        {"دالة", "void"},
        {"أرجع", "return"},
        {"إذا", "if"},
        {"وإلا", "else"},
        {"اطبع", "std::cout <<"},
        {"متغير", "auto"},
        {"نص", "std::string"},
        {"رقم", "int"},
        {"منطقي", "bool"},
        {"مصفوفة", "std::vector"}
    };

public:
    std::string translateFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        return translateCode(content);
    }

    std::string translateCode(const std::string& arabicCode) {
        std::string result = "// Generated from Arabic code\n";
        result += "#include <iostream>\n";
        result += "#include <string>\n";
        result += "#include <vector>\n\n";

        std::istringstream input(arabicCode);
        std::string line;

        while (std::getline(input, line)) {
            result += translateLine(line) + "\n";
        }

        return result;
    }

private:
    std::string translateLine(const std::string& line) {
        std::string result = line;

        // تجاهل التعليقات
        if (result.find("#") == 0) {
            return "//" + result.substr(1);
        }

        // ترجمة الكلمات المفتاحية
        for (const auto& [arabic, cpp] : translations) {
            std::regex wordRegex("\\b" + arabic + "\\b");
            result = std::regex_replace(result, wordRegex, cpp);
        }

        // معالجة حالات خاصة
        result = handleSpecialCases(result);

        return result;
    }

    std::string handleSpecialCases(const std::string& line) {
        std::string result = line;

        // استبدال الفواصل العربية بالإنجليزية
        std::regex commaRegex("،");
        result = std::regex_replace(result, commaRegex, ",");

        // معالجة دالة الطول
        std::regex lengthRegex("\\.طول\\(\\)");
        result = std::regex_replace(result, lengthRegex, ".size()");

        // معالجة دالة القسم
        std::regex splitRegex("\\.قسم\\(\"([^\"]+)\"\\)");
        result = std::regex_replace(result, splitRegex, ".split(\"$1\")");

        // إضافة std::endl لدالة الطباعة
        std::regex printRegex("(std::cout << [^;]+);");
        result = std::regex_replace(result, printRegex, "$1 << std::endl;");

        return result;
    }
};

#ifdef STANDALONE_TRANSLATOR
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <arabic_file>" << std::endl;
        return 1;
    }

    try {
        SimpleArabicTranslator translator;
        std::string cppCode = translator.translateFile(argv[1]);
        std::cout << cppCode;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
#endif