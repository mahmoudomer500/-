// X64Encoder.h - مولد كود الآلة x64 المباشر
// الإصدار 1.0 - المرحلة الأولى: التعليمات الأساسية

#ifndef X64_ENCODER_H
#define X64_ENCODER_H

#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <map>

namespace ArabicAssembler {

// ════════════════════════════════════════════════════════════
// 📋 تعريف السجلات x64
// ════════════════════════════════════════════════════════════

enum class Register : uint8_t {
    // السجلات الأساسية (64-bit)
    RAX = 0, RCX = 1, RDX = 2, RBX = 3,
    RSP = 4, RBP = 5, RSI = 6, RDI = 7,
    
    // السجلات الموسعة
    R8 = 8, R9 = 9, R10 = 10, R11 = 11,
    R12 = 12, R13 = 13, R14 = 14, R15 = 15,
    
    // RIP للعناوين النسبية
    RIP = 16,
    
    // السجلات 8-bit
    AL = 32, CL = 33, DL = 34, BL = 35,
    
    NONE = 255  // لا يوجد سجل
};

enum class Register32 : uint8_t {
    EAX = 0, ECX = 1, EDX = 2, EBX = 3,
    ESP = 4, EBP = 5, ESI = 6, EDI = 7,
    R8D = 8, R9D = 9, R10D = 10, R11D = 11,
    R12D = 12, R13D = 13, R14D = 14, R15D = 15
};

// ════════════════════════════════════════════════════════════
// 🎯 أنواع التعليمات
// ════════════════════════════════════════════════════════════

enum class InstructionType {
    // Data Movement
    MOV,      // نقل البيانات
    MOVZX,    // نقل مع توسيع صفري
    LEA,      // تحميل العنوان الفعلي
    PUSH,     // دفع للمكدس
    POP,      // سحب من المكدس
    
    // Arithmetic
    ADD,      // جمع
    SUB,      // طرح
    IMUL,     // ضرب (signed)
    IDIV,     // قسمة (signed)
    CQO,      // Sign extend RAX -> RDX:RAX
    INC,      // زيادة بـ 1
    DEC,      // نقصان بـ 1
    SHL_,      // إزاحة لليسار (ضرب)
    SHR_,      // إزاحة لليمين (قسمة)
    NEG,      // نفي (Two's complement)
    
    
    // Logic
    AND,      // و منطقي
    OR,       // أو منطقي
    XOR,      // أو الحصري
    NOT,      // نفي
    
    // Comparison
    CMP,      // مقارنة
    TEST,     // اختبار
    
    // Control Flow
    CALL,     // استدعاء دالة
    RET,      // رجوع من دالة
    JMP,      // قفز غير مشروط
    JE,       // قفز إذا يساوي
    JNE,      // قفز إذا لا يساوي
    JL,       // قفز إذا أصغر
    JLE,      // قفز إذا أصغر أو يساوي
    JG,       // قفز إذا أكبر
    JGE,      // قفز إذا أكبر أو يساوي
    
    // Special
    NOP       // لا عملية
};

// ════════════════════════════════════════════════════════════
// 🔢 أنواع المعاملات (Operands)
// ════════════════════════════════════════════════════════════

enum class OperandType {
    REGISTER,     // سجل
    IMMEDIATE,    // قيمة فورية
    MEMORY,       // عنوان ذاكرة
    LABEL         // تسمية (للقفزات)
};

struct Operand {
    OperandType type;
    
    union {
        Register reg;          // للسجلات
        int64_t immediate;     // للقيم الفورية
        struct {
            Register base;     // السجل الأساسي
            Register index;    // سجل الفهرس
            uint8_t scale;     // معامل الضرب (1, 2, 4, 8)
            int32_t disp;      // الإزاحة
        } memory;
    };
    
    std::string label;         // اسم التسمية
    
    // Constructors
    Operand() : type(OperandType::REGISTER), reg(Register::NONE) {}
    
    static Operand Reg(Register r) {
        Operand op;
        op.type = OperandType::REGISTER;
        op.reg = r;
        return op;
    }
    
    static Operand Imm(int64_t value) {
        Operand op;
        op.type = OperandType::IMMEDIATE;
        op.immediate = value;
        return op;
    }
    
    static Operand Mem(Register base, int32_t disp = 0) {
        Operand op;
        op.type = OperandType::MEMORY;
        op.memory.base = base;
        op.memory.index = Register::NONE;
        op.memory.scale = 1;
        op.memory.disp = disp;
        return op;
    }
    
    static Operand Label(const std::string& name) {
        Operand op;
        op.type = OperandType::LABEL;
        op.label = name;
        return op;
    }
    
    static Operand LabelMem(const std::string& name) {
        Operand op;
        op.type = OperandType::MEMORY;
        op.memory.base = Register::RIP;
        op.memory.index = Register::NONE;
        op.memory.scale = 1;
        op.memory.disp = 0;
        op.label = name;
        return op;
    }
};

// ════════════════════════════════════════════════════════════
// 📝 هيكل التعليمة
// ════════════════════════════════════════════════════════════

struct Instruction {
    InstructionType type;
    Operand dest;       // المعامل الأول (destination)
    Operand src;        // المعامل الثاني (source)
    
    Instruction(InstructionType t) : type(t) {}
    Instruction(InstructionType t, Operand d) : type(t), dest(d) {}
    Instruction(InstructionType t, Operand d, Operand s) : type(t), dest(d), src(s) {}
};

// ════════════════════════════════════════════════════════════
// ⚙️ محرك الترميز x64
// ════════════════════════════════════════════════════════════

class X64Encoder {
private:
    std::vector<uint8_t> code;
    
    // ═══ Helper Functions ═══
    
    // إضافة بايت واحد
    void emit8(uint8_t byte) {
        code.push_back(byte);
    }
    
    // إضافة 32-bit
    void emit32(uint32_t value) {
        emit8(value & 0xFF);
        emit8((value >> 8) & 0xFF);
        emit8((value >> 16) & 0xFF);
        emit8((value >> 24) & 0xFF);
    }
    
    // إضافة 64-bit
    void emit64(uint64_t value) {
        emit32(value & 0xFFFFFFFF);
        emit32((value >> 32) & 0xFFFFFFFF);
    }
    
    // ═══ REX Prefix ═══
    // REX.W = 64-bit operand
    // REX.R = extension of ModR/M reg field
    // REX.X = extension of SIB index field
    // REX.B = extension of ModR/M r/m, SIB base, or opcode reg
    
    void encodeREX(bool w, bool r, bool x, bool b) {
        uint8_t rex = 0x40;  // REX prefix base
        if (w) rex |= 0x08;  // REX.W
        if (r) rex |= 0x04;  // REX.R
        if (x) rex |= 0x02;  // REX.X
        if (b) rex |= 0x01;  // REX.B
        emit8(rex);
    }
    
    // ═══ ModR/M Byte ═══
    // mod: addressing mode (2 bits)
    // reg: register operand (3 bits)
    // r/m: register/memory operand (3 bits)
    
    void encodeModRM(uint8_t mod, uint8_t reg, uint8_t rm) {
        uint8_t modrm = (mod << 6) | ((reg & 7) << 3) | (rm & 7);
        emit8(modrm);
    }
    
    // ═══ SIB Byte ═══
    // scale: 00=1, 01=2, 10=4, 11=8
    // index: index register (3 bits)
    // base: base register (3 bits)
    
    void encodeSIB(uint8_t scale, uint8_t index, uint8_t base) {
        uint8_t sib = (scale << 6) | ((index & 7) << 3) | (base & 7);
        emit8(sib);
    }
    
    // ═══ Register Helpers ═══
    
    uint8_t getRegCode(Register reg) const {
        return static_cast<uint8_t>(reg) & 7;
    }
    
    bool isExtendedReg(Register reg) const {
        return static_cast<uint8_t>(reg) >= 8 && static_cast<uint8_t>(reg) < 16;
    }
    
    bool is8BitReg(Register reg) const {
        return static_cast<uint8_t>(reg) >= 32 && static_cast<uint8_t>(reg) <= 35;
    }
    
    uint8_t get8BitRegCode(Register reg) const {
        return static_cast<uint8_t>(reg) - 32;
    }
    
    // ═══ Instruction Encoding ═══
    
    void encodeMOV(const Operand& dest, const Operand& src);
    void encodeMOVZX(const Operand& dest, const Operand& src);
    void encodeLEA(const Operand& dest, const Operand& src);
    void encodePUSH(const Operand& op);
    void encodePOP(const Operand& op);
    void encodeADD(const Operand& dest, const Operand& src);
    void encodeSUB(const Operand& dest, const Operand& src);
    void encodeIMUL(const Operand& dest, const Operand& src);
    void encodeIDIV(const Operand& src);
    void encodeCQO();
    void encodeCMP(const Operand& dest, const Operand& src);
    void encodeTEST(const Operand& dest, const Operand& src);
    void encodeJMP(const Operand& target, InstructionType type);
    void encodeCALL(const Operand& target);
    void encodeRET();
    void encodeNOP();
    void encodeNEG(const Operand& op);
    void encodeAND(const Operand& dest, const Operand& src);
    void encodeOR(const Operand& dest, const Operand& src);
    void encodeXOR(const Operand& dest, const Operand& src);
    void encodeINC(const Operand& op);
    void encodeDEC(const Operand& op);
    void encodeSHL_(const Operand& dest, const Operand& src);
    void encodeSHR_(const Operand& dest, const Operand& src);
    
    
public:
    X64Encoder() = default;
    
    // ═══ Main Encoding Function ═══
    std::vector<uint8_t> encode(const Instruction& instr);
    
    // ═══ Batch Encoding ═══
    std::vector<uint8_t> encodeAll(const std::vector<Instruction>& instructions) {
        code.clear();
        for (const auto& instr : instructions) {
            auto bytes = encode(instr);
            code.insert(code.end(), bytes.begin(), bytes.end());
        }
        return code;
    }
    
    // ═══ Helper Factories ═══
    static Instruction MOV(Register dest, int64_t imm) {
        return Instruction(InstructionType::MOV, Operand::Reg(dest), Operand::Imm(imm));
    }
    
    static Instruction MOV(Register dest, Register src) {
        return Instruction(InstructionType::MOV, Operand::Reg(dest), Operand::Reg(src));
    }
    
    static Instruction MOV(Register dest, Operand src) {
        return Instruction(InstructionType::MOV, Operand::Reg(dest), src);
    }
    
    static Instruction MOV(Operand dest, Register src) {
        return Instruction(InstructionType::MOV, dest, Operand::Reg(src));
    }

    static Instruction MOV(Operand dest, Operand src) {
        return Instruction(InstructionType::MOV, dest, src);
    }

    static Instruction MOV(Operand dest, int64_t imm) {
        return Instruction(InstructionType::MOV, dest, Operand::Imm(imm));
    }

    static Instruction MOVZX(Register dest, Register src) {
        return Instruction(InstructionType::MOVZX, Operand::Reg(dest), Operand::Reg(src));
    }
    
    static Instruction MOVZX(Register dest, Operand src) {
        return Instruction(InstructionType::MOVZX, Operand::Reg(dest), src);
    }
    
    static Instruction PUSH(Register reg) {
        return Instruction(InstructionType::PUSH, Operand::Reg(reg));
    }
    
    static Instruction POP(Register reg) {
        return Instruction(InstructionType::POP, Operand::Reg(reg));
    }
    
    static Instruction ADD(Register dest, Register src) {
        return Instruction(InstructionType::ADD, Operand::Reg(dest), Operand::Reg(src));
    }
    
    static Instruction ADD(Register dest, Operand src) {
        return Instruction(InstructionType::ADD, Operand::Reg(dest), src);
    }
    
    static Instruction SUB(Register dest, Register src) {
        return Instruction(InstructionType::SUB, Operand::Reg(dest), Operand::Reg(src));
    }

    static Instruction SUB(Register dest, Operand src) {
        return Instruction(InstructionType::SUB, Operand::Reg(dest), src);
    }
    
    static Instruction RET() {
        return Instruction(InstructionType::RET);
    }
    
    static Instruction IMUL(Register dest, Register src) {
        return Instruction(InstructionType::IMUL, Operand::Reg(dest), Operand::Reg(src));
    }
    
    static Instruction IDIV(Register src) {
        return Instruction(InstructionType::IDIV, Operand::Reg(src));
    }
    
    static Instruction CQO() {
        return Instruction(InstructionType::CQO);
    }
    
    static Instruction NOP() {
        return Instruction(InstructionType::NOP);
    }
    
    static Instruction CMP(Register reg, int64_t imm) {
        return Instruction(InstructionType::CMP, Operand::Reg(reg), Operand::Imm(imm));
    }
    
    static Instruction TEST(Register reg1, Register reg2) {
        return Instruction(InstructionType::TEST, Operand::Reg(reg1), Operand::Reg(reg2));
    }
    
    static Instruction AND(Register dest, Register src) {
        return Instruction(InstructionType::AND, Operand::Reg(dest), Operand::Reg(src));
    }
    
    static Instruction OR(Register dest, Register src) {
        return Instruction(InstructionType::OR, Operand::Reg(dest), Operand::Reg(src));
    }
    
    static Instruction XOR(Register dest, Register src) {
        return Instruction(InstructionType::XOR, Operand::Reg(dest), Operand::Reg(src));
    }
    
    static Instruction JMP(int32_t offset) {
        return Instruction(InstructionType::JMP, Operand::Imm(offset));
    }
    
    static Instruction JE(int32_t offset) {
        return Instruction(InstructionType::JE, Operand::Imm(offset));
    }
    
    static Instruction JNE(int32_t offset) {
        return Instruction(InstructionType::JNE, Operand::Imm(offset));
    }

    static Instruction JMP(const std::string& label) {
        return Instruction(InstructionType::JMP, Operand::Label(label));
    }

    static Instruction JE(const std::string& label) {
        return Instruction(InstructionType::JE, Operand::Label(label));
    }

    static Instruction JNE(const std::string& label) {
        return Instruction(InstructionType::JNE, Operand::Label(label));
    }

    static Instruction JL(const std::string& label) {
        return Instruction(InstructionType::JL, Operand::Label(label));
    }

    static Instruction INC(Register reg) {
        return Instruction(InstructionType::INC, Operand::Reg(reg));
    }

    static Instruction DEC(Register reg) {
        return Instruction(InstructionType::DEC, Operand::Reg(reg));
    }

    static Instruction SHL_(Register dest, int8_t imm) {
        return Instruction(InstructionType::SHL_, Operand::Reg(dest), Operand::Imm(imm));
    }
};

} // namespace ArabicAssembler

#endif // X64_ENCODER_H
