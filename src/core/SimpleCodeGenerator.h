// SimpleCodeGenerator.h - مولد كود بسيط من الصفر
// يدعم العمليات الأساسية للمترجم العربي

#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cstdint>

namespace ArabicLanguage {

// تعريفات بسيطة للأوامر
enum class SimpleCommandType {
    PRINT_STRING,
    PRINT_NUMBER,
    VARIABLE_DECLARATION,
    ASSIGNMENT,
    FUNCTION_CALL,
    RETURN
};

struct SimpleValue {
    enum Type { STRING, NUMBER, VARIABLE, FUNCTION_CALL } type;
    std::string string_value;
    int number_value;
    std::string variable_name;
    std::vector<SimpleValue> arguments; // للدوال
};

struct SimpleCommand {
    SimpleCommandType type;
    SimpleValue value;
    std::string variable_name;
    std::vector<SimpleValue> arguments;
};

class SimpleCodeGenerator {
private:
    std::vector<uint8_t> code;
    std::vector<uint8_t> data_section;
    std::map<std::string, int> string_offsets;
    std::map<std::string, int> variable_offsets;
    int next_variable_offset = 0;

    // دوال مساعدة لتوليد الكود
    void emit_byte(uint8_t byte);
    void emit_dword(uint32_t value);
    void emit_string(const std::string& str);
    int add_string_literal(const std::string& str);

    // توليد كود للأوامر المختلفة
    void generate_print_string(const std::string& text);
    void generate_print_number(int number);
    void generate_variable_declaration(const std::string& var_name, const SimpleValue& value);
    void generate_assignment(const std::string& var_name, const SimpleValue& value);
    void generate_function_call(const std::string& func_name, const std::vector<SimpleValue>& args);
    void generate_return();

    // توليد كود x64 بسيط
    void emit_mov_rax_imm64(uint64_t value);
    void emit_mov_rcx_imm64(uint64_t value);
    void emit_mov_rdx_imm64(uint64_t value);
    void emit_mov_r8_imm64(uint64_t value);
    void emit_call(uint64_t address);
    void emit_ret();
    void emit_sub_rsp_imm8(uint8_t value);
    void emit_add_rsp_imm8(uint8_t value);

public:
    SimpleCodeGenerator();

    // الواجهة الرئيسية
    std::vector<uint8_t> generate(const std::vector<SimpleCommand>& commands);
    bool build_executable(const std::string& output_path);

    // دوال مساعدة
    void reset();
    size_t get_code_size() const { return code.size(); }
    size_t get_data_size() const { return data_section.size(); }
};

} // namespace ArabicLanguage
