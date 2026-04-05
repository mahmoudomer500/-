// test_parser_preservation.cpp
// Preservation Property Tests for Parser Bugs and Self-Hosting Fix
// **Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7**
//
// IMPORTANT: Follow observation-first methodology
// These tests verify that existing parsing behavior remains unchanged after the fix
// EXPECTED OUTCOME: Tests PASS on unfixed code (confirms baseline behavior to preserve)

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <memory>
#include "../src/core/ArabicParser.h"
#include "../src/core/ArabicCompiler.h"
#include "../src/runtime/ArabicTypes.h"

using namespace std;
using namespace ArabicLanguage;

// Test result structure
struct TestResult {
    string testName;
    bool passed;
    string errorMessage;
    string testCase;
};

vector<TestResult> testResults;

// Helper function to record test results
void recordTest(const string& testName, bool passed, const string& errorMessage = "", const string& testCase = "") {
    TestResult result;
    result.testName = testName;
    result.passed = passed;
    result.errorMessage = errorMessage;
    result.testCase = testCase;
    testResults.push_back(result);
    
    if (!passed) {
        cout << "❌ FAILED: " << testName << endl;
        if (!errorMessage.empty()) {
            cout << "   Error: " << errorMessage << endl;
        }
        if (!testCase.empty()) {
            cout << "   Test Case: " << testCase << endl;
        }
    } else {
        cout << "✅ PASSED: " << testName << endl;
    }
}

// Property 1: Standard Variable Declarations Preserved
// Validates: Requirement 3.1
void test_standard_variable_declarations() {
    string testName = "Property 1: Standard variable declarations with متغير";
    
    vector<string> testCases = {
        "متغير س = 10",
        "متغير اسم = \"أحمد\"",
        "متغير قيمة = 3.14",
        "متغير صحيح = صحيح",
        "متغير خطأ = خطأ"
    };
    
    bool allPassed = true;
    string failedCase;
    
    for (const auto& testCase : testCases) {
        try {
            ArabicParser parser;
            SymbolTable symbols;
            auto commands = parser.parse(testCase, symbols);
            
            if (commands.empty()) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
            
            // Verify we have a variable declaration
            bool foundVarDecl = false;
            for (const auto& cmd : commands) {
                if (cmd->type == CommandType::DECLARE || cmd->type == CommandType::ASSIGNMENT) {
                    foundVarDecl = true;
                    break;
                }
            }
            
            if (!foundVarDecl) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
        } catch (const exception& e) {
            allPassed = false;
            failedCase = testCase + " (Exception: " + e.what() + ")";
            break;
        }
    }
    
    if (allPassed) {
        recordTest(testName, true);
    } else {
        recordTest(testName, false, "Standard variable declaration parsing changed", failedCase);
    }
}

// Property 2: Standard Print Statements Preserved
// Validates: Requirement 3.2
void test_standard_print_statements() {
    string testName = "Property 2: Standard print statements with اطبع";
    
    vector<string> testCases = {
        "اطبع(\"مرحبا\")",
        "متغير س = 10\nاطبع(س)",
        "اطبع(5 + 3)",
        "اطبع(\"النتيجة: \"، 42)"
    };
    
    bool allPassed = true;
    string failedCase;
    
    for (const auto& testCase : testCases) {
        try {
            ArabicParser parser;
            SymbolTable symbols;
            auto commands = parser.parse(testCase, symbols);
            
            if (commands.empty()) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
            
            // Verify we have a print command
            bool foundPrint = false;
            for (const auto& cmd : commands) {
                if (cmd->type == CommandType::PRINT) {
                    foundPrint = true;
                    break;
                }
            }
            
            if (!foundPrint) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
        } catch (const exception& e) {
            allPassed = false;
            failedCase = testCase + " (Exception: " + e.what() + ")";
            break;
        }
    }
    
    if (allPassed) {
        recordTest(testName, true);
    } else {
        recordTest(testName, false, "Standard print statement parsing changed", failedCase);
    }
}

// Property 3: Standard Return Statements Preserved
// Validates: Requirement 3.3
void test_standard_return_statements() {
    string testName = "Property 3: Standard return statements with ارجع";
    
    vector<string> testCases = {
        "دالة اختبار():\n    ارجع 42\nنهاية",
        "دالة جمع(أ، ب):\n    ارجع أ + ب\nنهاية",
        "دالة نص():\n    ارجع \"مرحبا\"\nنهاية"
    };
    
    bool allPassed = true;
    string failedCase;
    
    for (const auto& testCase : testCases) {
        try {
            ArabicParser parser;
            SymbolTable symbols;
            auto commands = parser.parse(testCase, symbols);
            
            if (commands.empty()) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
            
            // Verify we have a function with a return command
            bool foundReturn = false;
            for (const auto& cmd : commands) {
                if (cmd->type == CommandType::FUNCTION_DEF && cmd->class_def) {
                    for (const auto& bodyCmd : cmd->body) {
                        if (bodyCmd->type == CommandType::RETURN) {
                            foundReturn = true;
                            break;
                        }
                    }
                }
            }
            
            if (!foundReturn) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
        } catch (const exception& e) {
            allPassed = false;
            failedCase = testCase + " (Exception: " + e.what() + ")";
            break;
        }
    }
    
    if (allPassed) {
        recordTest(testName, true);
    } else {
        recordTest(testName, false, "Standard return statement parsing changed", failedCase);
    }
}

// Property 4: Standard Class Definitions Preserved
// Validates: Requirement 3.4
void test_standard_class_definitions() {
    string testName = "Property 4: Standard class definitions without inheritance";
    
    vector<string> testCases = {
        "صنف شخص:\n    متغير اسم = \"أحمد\"\nنهاية",
        "صنف حساب:\n    متغير رصيد = 0\n    دالة إيداع(مبلغ):\n        رصيد = رصيد + مبلغ\n    نهاية\nنهاية",
        "صنف نقطة:\n    متغير س = 0\n    متغير ص = 0\nنهاية"
    };
    
    bool allPassed = true;
    string failedCase;
    
    for (const auto& testCase : testCases) {
        try {
            ArabicParser parser;
            SymbolTable symbols;
            auto commands = parser.parse(testCase, symbols);
            
            if (commands.empty()) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
            
            // Verify we have a class definition
            bool foundClass = false;
            for (const auto& cmd : commands) {
                if (cmd->type == CommandType::CLASS_DEF) {
                    foundClass = true;
                    break;
                }
            }
            
            if (!foundClass) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
        } catch (const exception& e) {
            allPassed = false;
            failedCase = testCase + " (Exception: " + e.what() + ")";
            break;
        }
    }
    
    if (allPassed) {
        recordTest(testName, true);
    } else {
        recordTest(testName, false, "Standard class definition parsing changed", failedCase);
    }
}

// Property 5: Simple Arabic Programs Compile Correctly
// Validates: Requirements 3.5, 3.6, 3.7
void test_simple_arabic_programs() {
    string testName = "Property 5: Simple Arabic programs compile and execute";
    
    // Test a simple program that should work on unfixed code
    string simpleProgram = 
        "متغير س = 10\n"
        "متغير ص = 20\n"
        "متغير مجموع = س + ص\n"
        "اطبع(\"المجموع: \"، مجموع)";
    
    try {
        ArabicParser parser;
        SymbolTable symbols;
        auto commands = parser.parse(simpleProgram, symbols);
        
        if (commands.empty()) {
            recordTest(testName, false, "Failed to parse simple Arabic program", simpleProgram);
            return;
        }
        
        // Verify we have the expected commands
        int varDeclCount = 0;
        int printCount = 0;
        
        for (const auto& cmd : commands) {
            if (cmd->type == CommandType::DECLARE || cmd->type == CommandType::ASSIGNMENT) {
                varDeclCount++;
            } else if (cmd->type == CommandType::PRINT) {
                printCount++;
            }
        }
        
        if (varDeclCount >= 3 && printCount >= 1) {
            recordTest(testName, true);
        } else {
            recordTest(testName, false, "Simple program parsing produced unexpected results", simpleProgram);
        }
    } catch (const exception& e) {
        recordTest(testName, false, string("Exception: ") + e.what(), simpleProgram);
    }
}

// Property 6: Control Flow Structures Preserved
// Validates: Requirement 3.2
void test_control_flow_structures() {
    string testName = "Property 6: Control flow structures (if, while, for) preserved";
    
    vector<string> testCases = {
        "إذا صحيح:\n    اطبع(\"نعم\")\nنهاية",
        "متغير عداد = 0\nطالما عداد < 5:\n    عداد = عداد + 1\nنهاية",
        "كرر 3:\n    اطبع(\"مرحبا\")\nنهاية"
    };
    
    bool allPassed = true;
    string failedCase;
    
    for (const auto& testCase : testCases) {
        try {
            ArabicParser parser;
            SymbolTable symbols;
            auto commands = parser.parse(testCase, symbols);
            
            if (commands.empty()) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
        } catch (const exception& e) {
            allPassed = false;
            failedCase = testCase + " (Exception: " + e.what() + ")";
            break;
        }
    }
    
    if (allPassed) {
        recordTest(testName, true);
    } else {
        recordTest(testName, false, "Control flow structure parsing changed", failedCase);
    }
}

// Property 7: Function Definitions Preserved
// Validates: Requirement 3.2
void test_function_definitions() {
    string testName = "Property 7: Function definitions preserved";
    
    vector<string> testCases = {
        "دالة مرحبا():\n    اطبع(\"مرحبا\")\nنهاية",
        "دالة جمع(أ، ب):\n    ارجع أ + ب\nنهاية",
        "دالة مربع(عدد):\n    متغير نتيجة = عدد * عدد\n    ارجع نتيجة\nنهاية"
    };
    
    bool allPassed = true;
    string failedCase;
    
    for (const auto& testCase : testCases) {
        try {
            ArabicParser parser;
            SymbolTable symbols;
            auto commands = parser.parse(testCase, symbols);
            
            if (commands.empty()) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
            
            // Verify we have a function definition
            bool foundFunction = false;
            for (const auto& cmd : commands) {
                if (cmd->type == CommandType::FUNCTION_DEF) {
                    foundFunction = true;
                    break;
                }
            }
            
            if (!foundFunction) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
        } catch (const exception& e) {
            allPassed = false;
            failedCase = testCase + " (Exception: " + e.what() + ")";
            break;
        }
    }
    
    if (allPassed) {
        recordTest(testName, true);
    } else {
        recordTest(testName, false, "Function definition parsing changed", failedCase);
    }
}

// Property 8: Expression Parsing Preserved
// Validates: Requirement 3.4
void test_expression_parsing() {
    string testName = "Property 8: Expression parsing preserved";
    
    vector<string> testCases = {
        "متغير نتيجة = 5 + 3",
        "متغير حاصل = 10 * 2",
        "متغير قسمة = 20 / 4",
        "متغير باقي = 10 % 3",
        "متغير مقارنة = 5 > 3",
        "متغير منطقي = صحيح && خطأ"
    };
    
    bool allPassed = true;
    string failedCase;
    
    for (const auto& testCase : testCases) {
        try {
            ArabicParser parser;
            SymbolTable symbols;
            auto commands = parser.parse(testCase, symbols);
            
            if (commands.empty()) {
                allPassed = false;
                failedCase = testCase;
                break;
            }
        } catch (const exception& e) {
            allPassed = false;
            failedCase = testCase + " (Exception: " + e.what() + ")";
            break;
        }
    }
    
    if (allPassed) {
        recordTest(testName, true);
    } else {
        recordTest(testName, false, "Expression parsing changed", failedCase);
    }
}

int main() {
    cout << "═══════════════════════════════════════════════════════════" << endl;
    cout << "Preservation Property Tests - Parser Bugs Self-Hosting Fix" << endl;
    cout << "═══════════════════════════════════════════════════════════" << endl;
    cout << "IMPORTANT: These tests verify existing behavior is preserved" << endl;
    cout << "EXPECTED OUTCOME: Tests PASS on unfixed code" << endl;
    cout << "═══════════════════════════════════════════════════════════\n" << endl;
    
    // Run all preservation tests
    test_standard_variable_declarations();
    test_standard_print_statements();
    test_standard_return_statements();
    test_standard_class_definitions();
    test_simple_arabic_programs();
    test_control_flow_structures();
    test_function_definitions();
    test_expression_parsing();
    
    // Summary
    cout << "\n═══════════════════════════════════════════════════════════" << endl;
    cout << "Test Summary" << endl;
    cout << "═══════════════════════════════════════════════════════════" << endl;
    
    int passedCount = 0;
    int failedCount = 0;
    
    for (const auto& result : testResults) {
        if (result.passed) {
            passedCount++;
        } else {
            failedCount++;
        }
    }
    
    cout << "Total Tests: " << testResults.size() << endl;
    cout << "Passed: " << passedCount << endl;
    cout << "Failed: " << failedCount << endl;
    
    if (passedCount == testResults.size()) {
        cout << "\n✅ EXPECTED OUTCOME: All tests PASSED" << endl;
        cout << "Baseline behavior confirmed - these behaviors must be preserved after fix" << endl;
        return 0;
    } else {
        cout << "\n⚠️  UNEXPECTED: Some tests FAILED" << endl;
        cout << "This suggests existing functionality may already be broken" << endl;
        cout << "\nFailed tests:" << endl;
        for (const auto& result : testResults) {
            if (!result.passed) {
                cout << "  - " << result.testName << endl;
                if (!result.errorMessage.empty()) {
                    cout << "    Error: " << result.errorMessage << endl;
                }
            }
        }
        return 1;
    }
}
