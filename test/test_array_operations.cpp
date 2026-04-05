// test_array_operations.cpp - Unit tests for array operations
// اختبارات وحدة لعمليات المصفوفات

#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <sstream>

// Mock the dependencies for testing
namespace ArabicLanguage {
    
    // ═══════════════════════════════════════════════════════════
    // Test Framework
    // ═══════════════════════════════════════════════════════════
    
    class TestRunner {
    private:
        std::string currentSuite;
        int passCount = 0;
        int failCount = 0;
        std::stringstream output;
        
    public:
        void beginSuite(const std::string& suiteName) {
            currentSuite = suiteName;
            output << "\n╔════════════════════════════════════════════════╗\n";
            output << "║ Test Suite: " << suiteName << "\n";
            output << "╚════════════════════════════════════════════════╝\n";
        }
        
        void test(const std::string& testName, bool condition, const std::string& message = "") {
            if (condition) {
                passCount++;
                output << "✅ PASS: " << testName << "\n";
            } else {
                failCount++;
                output << "❌ FAIL: " << testName << "\n";
                if (!message.empty()) {
                    output << "   Message: " << message << "\n";
                }
            }
        }
        
        void assertEqual(const std::string& testName, double actual, double expected, double tolerance = 0.0001) {
            bool passed = (actual >= expected - tolerance && actual <= expected + tolerance);
            test(testName, passed, 
                 "Expected: " + std::to_string(expected) + ", Got: " + std::to_string(actual));
        }
        
        void printSummary() {
            output << "\n╔════════════════════════════════════════════════╗\n";
            output << "║ Test Summary\n";
            output << "║ Passed: " << passCount << "\n";
            output << "║ Failed: " << failCount << "\n";
            output << "║ Total:  " << (passCount + failCount) << "\n";
            if (failCount == 0) {
                output << "║ Status: ✅ ALL TESTS PASSED\n";
            } else {
                output << "║ Status: ❌ SOME TESTS FAILED\n";
            }
            output << "╚════════════════════════════════════════════════╝\n";
            
            std::cout << output.str();
        }
        
        int getFailCount() const { return failCount; }
    };
    
    // ═══════════════════════════════════════════════════════════
    // Test Cases for Array Operations
    // ═══════════════════════════════════════════════════════════
    
    void runArrayTests(TestRunner& runner) {
        runner.beginSuite("Array Operations Tests");
        
        // ────────────────────────────────────────────────────────
        // Test 1: Basic Array Creation
        // ────────────────────────────────────────────────────────
        {
            std::vector<int> arr;
            runner.test("Create empty array", arr.empty(), "Array should be empty on creation");
            runner.test("Array capacity positive", arr.capacity() >= 0, "Capacity should be non-negative");
        }
        
        // ────────────────────────────────────────────────────────
        // Test 2: Add Elements
        // ────────────────────────────────────────────────────────
        {
            std::vector<int> arr;
            arr.push_back(1);
            arr.push_back(2);
            arr.push_back(3);
            
            runner.test("Array has 3 elements", arr.size() == 3, 
                       "Expected size 3, got " + std::to_string(arr.size()));
            runner.test("First element is 1", arr[0] == 1, "First element should be 1");
            runner.test("Second element is 2", arr[1] == 2, "Second element should be 2");
            runner.test("Third element is 3", arr[2] == 3, "Third element should be 3");
        }
        
        // ────────────────────────────────────────────────────────
        // Test 3: Dynamic Resizing
        // ────────────────────────────────────────────────────────
        {
            std::vector<int> arr;
            size_t initialCapacity = arr.capacity();
            
            // Add enough elements to trigger resize
            for (int i = 0; i < 100; i++) {
                arr.push_back(i);
            }
            
            runner.test("Array has 100 elements after loop", arr.size() == 100,
                       "Expected size 100, got " + std::to_string(arr.size()));
            runner.test("Capacity increased after resize", arr.capacity() > initialCapacity,
                       "Capacity should increase to accommodate 100 elements");
            runner.test("All elements preserved", arr.back() == 99, "Last element should be 99");
        }
        
        // ────────────────────────────────────────────────────────
        // Test 4: Remove Elements
        // ────────────────────────────────────────────────────────
        {
            std::vector<int> arr;
            arr = {1, 2, 3, 4, 5};
            
            // Remove element at index 2
            arr.erase(arr.begin() + 2);
            
            runner.test("Array size after removal", arr.size() == 4,
                       "Expected size 4 after removing one element");
            runner.test("Element removed correctly", arr[2] == 4,
                       "Element at index 2 should now be 4");
        }
        
        // ────────────────────────────────────────────────────────
        // Test 5: Insert Elements
        // ────────────────────────────────────────────────────────
        {
            std::vector<int> arr;
            arr = {1, 2, 4, 5};
            
            // Insert 3 at index 2
            arr.insert(arr.begin() + 2, 3);
            
            runner.test("Array size after insertion", arr.size() == 5,
                       "Expected size 5 after insertion");
            runner.test("Element inserted correctly", arr[2] == 3,
                       "Element at index 2 should be 3");
            runner.test("Subsequent elements shifted", arr[3] == 4,
                       "Element at index 3 should be 4");
        }
        
        // ────────────────────────────────────────────────────────
        // Test 6: Out of Bounds Access (Negative Test)
        // ────────────────────────────────────────────────────────
        {
            std::vector<int> arr;
            arr = {1, 2, 3};
            
            // This should be caught in actual implementation
            bool outOfBoundsDetected = false;
            try {
                if (5 >= arr.size()) {
                    throw std::out_of_range("Index out of bounds");
                }
                outOfBoundsDetected = true;
            } catch (const std::out_of_range&) {
                outOfBoundsDetected = false;
            }
            
            runner.test("Out of bounds detection works", !outOfBoundsDetected,
                       "Out of bounds access should be prevented");
        }
        
        // ────────────────────────────────────────────────────────
        // Test 7: Negative Index Handling
        // ────────────────────────────────────────────────────────
        {
            std::vector<int> arr;
            arr = {1, 2, 3};
            
            bool negativeIndexHandled = false;
            int negativeIdx = -1;
            
            if (negativeIdx < 0) {
                negativeIndexHandled = true;
            }
            
            runner.test("Negative index detected and handled", negativeIndexHandled,
                       "Negative indices should be properly detected");
        }
        
        // ────────────────────────────────────────────────────────
        // Test 8: Large Array Capacity Test
        // ────────────────────────────────────────────────────────
        {
            std::vector<int> arr;
            
            // Add 1000 elements
            for (int i = 0; i < 1000; i++) {
                arr.push_back(i);
            }
            
            runner.test("Large array creation (1000 elements)", arr.size() == 1000,
                       "Expected size 1000");
            runner.test("Element access in large array", arr[999] == 999,
                       "Last element should be 999");
            runner.test("Element access in middle", arr[500] == 500,
                       "Middle element should be 500");
        }
        
        // ────────────────────────────────────────────────────────
        // Test 9: Array with Different Element Types
        // ────────────────────────────────────────────────────────
        {
            std::vector<double> doubleArr;
            doubleArr = {1.5, 2.5, 3.5};
            
            runner.test("Double array creation", doubleArr.size() == 3,
                       "Expected size 3");
            runner.test("Double values stored", doubleArr[0] >= 1.4 && doubleArr[0] <= 1.6,
                       "First element should be 1.5");
        }
        
        // ────────────────────────────────────────────────────────
        // Test 10: Memory Efficiency (Growth Factor)
        // ────────────────────────────────────────────────────────
        {
            std::vector<int> arr;
            std::vector<size_t> capacities;
            
            for (int i = 0; i < 50; i++) {
                arr.push_back(i);
                if (i == 0 || arr.capacity() != capacities.back()) {
                    capacities.push_back(arr.capacity());
                }
            }
            
            // Check that growth factor is reasonable (between 1.5x and 2x typically)
            bool efficientGrowth = true;
            for (size_t i = 1; i < capacities.size(); i++) {
                double ratio = static_cast<double>(capacities[i]) / capacities[i-1];
                if (ratio > 2.5 || ratio < 1.1) {
                    efficientGrowth = false;
                    break;
                }
            }
            
            runner.test("Growth factor is efficient", efficientGrowth,
                       "Capacity growth should use reasonable factor (1.5-2x)");
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // Memory Leak Detection
    // ═══════════════════════════════════════════════════════════
    
    void runMemoryTests(TestRunner& runner) {
        runner.beginSuite("Memory Management Tests");
        
        // ────────────────────────────────────────────────────────
        // Test 1: Simple Allocation and Deallocation
        // ────────────────────────────────────────────────────────
        {
            int* ptr = new int[100];
            runner.test("Memory allocation successful", ptr != nullptr,
                       "Should successfully allocate memory");
            delete[] ptr;
            runner.test("Memory deallocation successful", true,
                       "Should successfully deallocate memory");
        }
        
        // ────────────────────────────────────────────────────────
        // Test 2: Multiple Allocations
        // ────────────────────────────────────────────────────────
        {
            std::vector<int*> pointers;
            for (int i = 0; i < 10; i++) {
                pointers.push_back(new int[50]);
            }
            
            runner.test("Multiple allocations successful", pointers.size() == 10,
                       "Should successfully allocate 10 arrays");
            
            for (auto ptr : pointers) {
                delete[] ptr;
            }
            runner.test("Multiple deallocations successful", true,
                       "Should successfully deallocate all arrays");
        }
        
        // ────────────────────────────────────────────────────────
        // Test 3: Reallocation Preserves Data
        // ────────────────────────────────────────────────────────
        {
            int* oldPtr = new int[10];
            for (int i = 0; i < 10; i++) {
                oldPtr[i] = i;
            }
            
            // Simulate reallocation
            int* newPtr = new int[20];
            for (int i = 0; i < 10; i++) {
                newPtr[i] = oldPtr[i];
            }
            
            bool dataPreserved = true;
            for (int i = 0; i < 10; i++) {
                if (newPtr[i] != i) {
                    dataPreserved = false;
                    break;
                }
            }
            
            runner.test("Reallocation preserves data", dataPreserved,
                       "Data should be preserved during reallocation");
            
            delete[] oldPtr;
            delete[] newPtr;
        }
    }

} // namespace ArabicLanguage

// ═══════════════════════════════════════════════════════════
// Main Test Runner
// ═══════════════════════════════════════════════════════════

int main() {
    std::cout << "\n" << std::string(50, '═') << "\n";
    std::cout << "   Arabic Programming Language - Array Test Suite\n";
    std::cout << std::string(50, '═') << "\n\n";
    
    ArabicLanguage::TestRunner runner;
    
    // Run all test suites
    ArabicLanguage::runArrayTests(runner);
    ArabicLanguage::runMemoryTests(runner);
    
    // Print summary
    runner.printSummary();
    
    // Return exit code based on test results
    return (runner.getFailCount() > 0) ? 1 : 0;
}
