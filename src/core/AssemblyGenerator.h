// AssemblyGenerator.h - مع دعم البرمجة الكائنية ونظام الأنواع
#ifndef ASSEMBLY_GENERATOR_H
#define ASSEMBLY_GENERATOR_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include "ArabicTypes.h"

namespace ArabicLanguage {

    // ✅ نظام الأنواع
    enum class DataType {
        INTEGER,    // أرقام صحيحة 64-بت
        STRING,     // مؤشر لنص
        POINTER,    // مؤشر عام
        ARRAY,      // مصفوفة
        UNKNOWN     // نوع غير محدد
    };

    class AssemblyGenerator {
    public:
        AssemblyGenerator();
        std::string generate(const std::vector<std::shared_ptr<Command>>& commands);
        std::map<std::string, int> getStatistics() const;

    private:
        std::map<std::string, std::map<std::string, std::string>> variables;
        std::map<std::string, std::map<std::string, std::string>> arrays;
        std::map<std::string, std::map<std::string, std::vector<std::string>>> functions;
        std::set<std::string> currentFunctionParameters;

        // ✅ تتبع أنواع المتغيرات
        std::map<std::string, DataType> variableTypes;

        // ✅ دعم البرمجة الكائنية
        std::map<std::string, std::shared_ptr<ClassDefinition>> classes;
        std::map<std::string, std::string> objectInstances;  // اسم_الكائن -> نوع_الصنف

        std::map<std::string, int> statistics;
        int textCounter;
        int labelCounter;

        std::vector<std::string> programCode;
        std::vector<std::string> globalData;
        std::vector<std::string> textData;

        // دوال مساعدة
        std::string generateLabel(const std::string& prefix);
        std::string generateTextLabel();
        bool isKnownVariable(const std::string& variable);

        // ✅ دوال نظام الأنواع
        DataType inferValueType(const Value& value);
        std::string getFormatString(DataType type, bool newline);

        // التحليل والتوليد
        void preliminaryDataAnalysis(const std::vector<std::shared_ptr<Command>>& commands);
        void processCommandList(const std::vector<std::shared_ptr<Command>>& commands, bool inFunction = false);
    
        // ✅ New: Organize flat commands into hierarchical structure  
        std::vector<std::shared_ptr<Command>> organizeBlocks(const std::vector<std::shared_ptr<Command>>& flatCommands);

        std::vector<std::string> generateAllFunctions(const std::vector<std::shared_ptr<Command>>& commands);
        std::vector<std::string> generateBlockCode(const std::vector<std::shared_ptr<Command>>& commands);
        std::vector<std::string> generateValueCode(const Value& value);
        std::vector<std::string> generateFunctionCode(const Command& command);
        std::vector<std::string> generateAssignment(const Command& command);
        std::vector<std::string> generatePrint(const Command& command, bool newLine = true);
        std::vector<std::string> generateCondition(const Command& command, bool inFunction = false);
        std::vector<std::string> generateConditionCode(const Value& condition);
        std::vector<std::string> generateForLoop(const Command& command);
        std::vector<std::string> generateWhileLoop(const Command& command);
        std::vector<std::string> generateFunctionCall(const Command& command);
        std::vector<std::string> generateFunctionCall(const Value& value);
        std::vector<std::string> generateReturn(const Command& command);
        std::vector<std::string> generateWait();

        // ✅ دوال جديدة للبرمجة الكائنية
        std::vector<std::string> generateObjectCreation(const Command& command);
        std::vector<std::string> generateMethodCall(const Command& command);
        std::vector<std::string> generatePropertyAccess(const Value& value);

        // تعريف المتغيرات
        void defineVariable(const std::string& variable);
        void defineArray(const std::string& variable, const std::vector<Value>& elements);

        // الهيكل الأساسي
        std::vector<std::string> createBasicStructure();
        std::vector<std::string> programStart();
        std::vector<std::string> programEnd();
        std::vector<std::string> generateBuiltInFunctions();

        // دوال التحقق
        bool containsReturn(const Command& command);
    };

} // namespace ArabicLanguage

#endif // ASSEMBLY_GENERATOR_H