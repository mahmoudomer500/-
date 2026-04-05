// X64Encoder.cpp - تنفيذ مولد كود الآلة x64

#include "X64Encoder.h"
#include <iostream>

namespace ArabicAssembler {

// ═══ Main Encoding Function ═══
std::vector<uint8_t> X64Encoder::encode(const Instruction& instr) {
    code.clear();
    
    switch (instr.type) {
        case InstructionType::MOV:
            encodeMOV(instr.dest, instr.src);
            break;
        case InstructionType::MOVZX:
            encodeMOVZX(instr.dest, instr.src);
            break;
        case InstructionType::LEA:
            encodeLEA(instr.dest, instr.src);
            break;
        case InstructionType::PUSH:
            encodePUSH(instr.dest);
            break;
        case InstructionType::POP:
            encodePOP(instr.dest);
            break;
        case InstructionType::ADD:
            encodeADD(instr.dest, instr.src);
            break;
        case InstructionType::SUB:
            encodeSUB(instr.dest, instr.src);
            break;
        case InstructionType::IMUL:
            encodeIMUL(instr.dest, instr.src);
            break;
        case InstructionType::IDIV:
            encodeIDIV(instr.dest);
            break;
        case InstructionType::CQO:
            encodeCQO();
            break;
        case InstructionType::CMP:
            encodeCMP(instr.dest, instr.src);
            break;
        case InstructionType::TEST:
            encodeTEST(instr.dest, instr.src);
            break;
        case InstructionType::JMP:
            encodeJMP(instr.dest, InstructionType::JMP);
            break;
        case InstructionType::JE:
            encodeJMP(instr.dest, InstructionType::JE);
            break;
        case InstructionType::JNE:
            encodeJMP(instr.dest, InstructionType::JNE);
            break;
        case InstructionType::JL:
            encodeJMP(instr.dest, InstructionType::JL);
            break;
        case InstructionType::JLE:
            encodeJMP(instr.dest, InstructionType::JLE);
            break;
        case InstructionType::JG:
            encodeJMP(instr.dest, InstructionType::JG);
            break;
        case InstructionType::JGE:
            encodeJMP(instr.dest, InstructionType::JGE);
            break;
        case InstructionType::CALL:
            encodeCALL(instr.dest);
            break;
        case InstructionType::RET:
            encodeRET();
            break;
        case InstructionType::NOP:
            encodeNOP();
            break;
        case InstructionType::NEG:
            encodeNEG(instr.dest);
            break;
        case InstructionType::AND:
            encodeAND(instr.dest, instr.src);
            break;
        case InstructionType::OR:
            encodeOR(instr.dest, instr.src);
            break;
        case InstructionType::XOR:
            encodeXOR(instr.dest, instr.src);
            break;
        case InstructionType::INC:
            encodeINC(instr.dest);
            break;
        case InstructionType::DEC:
            encodeDEC(instr.dest);
            break;
        case InstructionType::SHL_:
            encodeSHL_(instr.dest, instr.src);
            break;
        case InstructionType::SHR_:
            encodeSHR_(instr.dest, instr.src);
            break;
        default:
            throw std::runtime_error("Unsupported instruction type");
    }
    
    return code;
}

// ═══ Instruction Encodings ═══

void X64Encoder::encodeMOV(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::IMMEDIATE) {
        Register reg = dest.reg;
        int64_t imm = src.immediate;

        if (is8BitReg(reg)) {
            // MOV reg8, imm8
            emit8(0xB0 + get8BitRegCode(reg));
            emit8(static_cast<uint8_t>(imm));
        } else {
            // MOV reg64, imm64
            encodeREX(true, false, false, isExtendedReg(reg));
            emit8(0xB8 + getRegCode(reg));
            emit64(imm);
        }

    } else if (dest.type == OperandType::REGISTER && src.type == OperandType::REGISTER) {
        Register destReg = dest.reg;
        Register srcReg = src.reg;

        if (is8BitReg(destReg) && is8BitReg(srcReg)) {
            // MOV reg8, reg8
            emit8(0x88);
            encodeModRM(0x3, get8BitRegCode(srcReg), get8BitRegCode(destReg));
        } else {
            // MOV reg64, reg64
            encodeREX(true, isExtendedReg(srcReg), false, isExtendedReg(destReg));
            emit8(0x89);
            encodeModRM(0x3, getRegCode(srcReg), getRegCode(destReg));
        }

    } else if (dest.type == OperandType::REGISTER && src.type == OperandType::MEMORY) {
        Register destReg = dest.reg;
        const auto& mem = src.memory;

        if (is8BitReg(destReg)) {
            // MOV reg8, [mem]
            emit8(0x8A);
            uint8_t mod = (mem.disp == 0) ? 0x0 : 0x2;
            uint8_t rm = getRegCode(mem.base);
            encodeModRM(mod, get8BitRegCode(destReg), rm);
            if (rm == 4) emit8(0x24); // SIB byte for RSP
            if (mem.disp != 0) emit32(mem.disp);
            else if (rm == 5) emit8(0); // RBP needs 8-bit displacement even if 0
        } else {
            // MOV reg64, [mem]
            encodeREX(true, isExtendedReg(destReg), false, isExtendedReg(mem.base));
            emit8(0x8B);
            uint8_t mod = (mem.disp == 0) ? 0x0 : 0x2;
            uint8_t rm = getRegCode(mem.base);
            encodeModRM(mod, getRegCode(destReg), rm);
            if (rm == 4) emit8(0x24); // SIB byte for RSP
            if (mem.disp != 0) emit32(mem.disp);
            else if (rm == 5) emit8(0); // RBP needs 8-bit displacement even if 0
        }

    } else if (dest.type == OperandType::MEMORY && src.type == OperandType::REGISTER) {
        const auto& mem = dest.memory;
        Register srcReg = src.reg;

        if (is8BitReg(srcReg)) {
            // MOV [mem], reg8
            emit8(0x88);
            uint8_t mod = (mem.disp == 0) ? 0x0 : 0x2;
            uint8_t rm = getRegCode(mem.base);
            encodeModRM(mod, get8BitRegCode(srcReg), rm);
            if (rm == 4) emit8(0x24); // SIB byte for RSP
            if (mem.disp != 0) emit32(mem.disp);
            else if (rm == 5) emit8(0); // RBP needs 8-bit displacement even if 0
        } else {
            // MOV [mem], reg64
            encodeREX(true, isExtendedReg(srcReg), false, isExtendedReg(mem.base));
            emit8(0x89);
            uint8_t mod = (mem.disp == 0) ? 0x0 : 0x2;
            uint8_t rm = getRegCode(mem.base);
            encodeModRM(mod, getRegCode(srcReg), rm);
            if (rm == 4) emit8(0x24); // SIB byte for RSP
            if (mem.disp != 0) emit32(mem.disp);
            else if (rm == 5) emit8(0); // RBP needs 8-bit displacement even if 0
        }

    } else if (dest.type == OperandType::MEMORY && src.type == OperandType::IMMEDIATE) {
        // MOV [mem], imm32
        const auto& mem = dest.memory;
        int64_t imm = src.immediate;

        // REX.W prefix for 64-bit operand assignment
        // If we want to support 32-bit assignment, we'd need to check operand size
        // For now, assuming 64-bit store because we are using RBP etc.
        encodeREX(true, false, false, isExtendedReg(mem.base));

        // Opcode for MOV r/m, imm32 is C7
        emit8(0xC7);

        // ModR/M for memory operand with /0 extension in Reg field
        // Reg field (bits 5-3) must be 000 (0)
        uint8_t mod = (mem.disp == 0) ? 0x0 : 0x2;
        uint8_t rm = getRegCode(mem.base);
        encodeModRM(mod, 0, rm);
        if (rm == 4) emit8(0x24); // SIB byte for RSP

        // Displacement if needed
        if (mem.disp != 0) {
            emit32(mem.disp);
        }
        else if (rm == 5) emit8(0); // RBP needs 8-bit displacement even if 0

        // Immediate value (32-bit only allowed for C7)
        emit32(static_cast<uint32_t>(imm));

    } else if (dest.type == OperandType::REGISTER && src.type == OperandType::LABEL) {
        // MOV reg, [RIP + label_offset]
        Register destReg = dest.reg;
        encodeREX(true, isExtendedReg(destReg), false, false);
        emit8(0x8B);
        encodeModRM(0x00, getRegCode(destReg), 0x05);
        emit32(0); // Placeholder for relocation

    } else if (dest.type == OperandType::LABEL && src.type == OperandType::REGISTER) {
        // MOV [RIP + label_offset], reg
        Register srcReg = src.reg;
        encodeREX(true, isExtendedReg(srcReg), false, false);
        emit8(0x89);
        encodeModRM(0x00, getRegCode(srcReg), 0x05);
        emit32(0); // Placeholder for relocation

    } else {
        throw std::runtime_error("Unsupported MOV operands");
    }
}

void X64Encoder::encodeMOVZX(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::REGISTER) {
        Register destReg = dest.reg;
        Register srcReg = src.reg;

        // MOVZX reg64, reg8
        encodeREX(true, isExtendedReg(destReg), false, false); // REX.W for 64-bit dest
        emit8(0x0F);
        emit8(0xB6);
        encodeModRM(0x3, getRegCode(destReg), get8BitRegCode(srcReg));

    } else if (dest.type == OperandType::REGISTER && src.type == OperandType::MEMORY) {
        Register destReg = dest.reg;
        const auto& mem = src.memory;

        // MOVZX reg64, [mem8]
        encodeREX(true, isExtendedReg(destReg), false, isExtendedReg(mem.base));
        emit8(0x0F);
        emit8(0xB6);
        uint8_t mod = (mem.disp == 0) ? 0x0 : 0x2;
        uint8_t rm = getRegCode(mem.base);
        encodeModRM(mod, getRegCode(destReg), rm);
        if (rm == 4) emit8(0x24); // SIB byte for RSP
        if (mem.disp != 0) emit32(mem.disp);
        else if (rm == 5) emit8(0); // RBP needs 8-bit displacement even if 0
    } else {
        throw std::runtime_error("Unsupported MOVZX operands");
    }
}

void X64Encoder::encodeLEA(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::MEMORY) {
        Register destReg = dest.reg;
        
        // REX.W prefix
        encodeREX(true, isExtendedReg(destReg), false, isExtendedReg(src.memory.base));
        
        // LEA opcode
        emit8(0x8D);
        
        // ModR/M for memory operand
        uint8_t mod = 0x2; // 32-bit displacement
        uint8_t rm = getRegCode(src.memory.base);
        encodeModRM(mod, getRegCode(destReg), rm);
        if (rm == 4) emit8(0x24); // SIB byte for RSP
        
        // Displacement
        emit32(src.memory.disp);
    } else if (dest.type == OperandType::REGISTER && src.type == OperandType::LABEL) {
        // [NEW] LEA reg, [RIP + disp32] (RIP-Relative)
        Register destReg = dest.reg;
        
        // REX.W prefix for 64-bit destination
        encodeREX(true, isExtendedReg(destReg), false, false);
        
        // LEA opcode
        emit8(0x8D);
        
        // ModR/M: Mod=00, Reg=dest, R/M=101 (RIP)
        // In 64-bit mode, Mod=00 + R/M=101 ([BP] in 32-bit) means [RIP + disp32]
        encodeModRM(0x00, getRegCode(destReg), 0x05);
        
        // Displacement (0 placeholder, fixed by relocation)
        emit32(0);
    } else {
        throw std::runtime_error("Unsupported LEA operands");
    }
}

void X64Encoder::encodePUSH(const Operand& op) {
    if (op.type == OperandType::REGISTER) {
        Register reg = op.reg;
        
        if (isExtendedReg(reg)) {
            encodeREX(false, false, false, true);
        }
        
        emit8(0x50 + getRegCode(reg));
    } else {
        throw std::runtime_error("Unsupported PUSH operand");
    }
}

void X64Encoder::encodePOP(const Operand& op) {
    if (op.type == OperandType::REGISTER) {
        Register reg = op.reg;
        
        if (isExtendedReg(reg)) {
            encodeREX(false, false, false, true);
        }
        
        emit8(0x58 + getRegCode(reg));
    } else {
        throw std::runtime_error("Unsupported POP operand");
    }
}

void X64Encoder::encodeADD(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::REGISTER) {
        Register destReg = dest.reg;
        Register srcReg = src.reg;
        
        encodeREX(true, isExtendedReg(srcReg), false, isExtendedReg(destReg));
        emit8(0x01);
        encodeModRM(0x3, getRegCode(srcReg), getRegCode(destReg));
        
    } else if (dest.type == OperandType::REGISTER && src.type == OperandType::IMMEDIATE) {
        Register destReg = dest.reg;
        int32_t imm = static_cast<int32_t>(src.immediate);
        
        encodeREX(true, false, false, isExtendedReg(destReg));
        emit8(0x81);
        encodeModRM(0x3, 0x0, getRegCode(destReg));
        emit32(imm);
    } else {
        throw std::runtime_error("Unsupported ADD operands");
    }
}

void X64Encoder::encodeSUB(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::REGISTER) {
        Register destReg = dest.reg;
        Register srcReg = src.reg;
        
        encodeREX(true, isExtendedReg(srcReg), false, isExtendedReg(destReg));
        emit8(0x29);
        encodeModRM(0x3, getRegCode(srcReg), getRegCode(destReg));
        
    } else if (dest.type == OperandType::REGISTER && src.type == OperandType::IMMEDIATE) {
        Register destReg = dest.reg;
        int32_t imm = static_cast<int32_t>(src.immediate);
        
        encodeREX(true, false, false, isExtendedReg(destReg));
        emit8(0x81);
        encodeModRM(0x3, 0x5, getRegCode(destReg));
        emit32(imm);
    } else {
        throw std::runtime_error("Unsupported SUB operands");
    }
}

void X64Encoder::encodeIMUL(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::REGISTER) {
        Register destReg = dest.reg;
        Register srcReg = src.reg;
        
        encodeREX(true, isExtendedReg(destReg), false, isExtendedReg(srcReg));
        emit8(0x0F);
        emit8(0xAF);
        encodeModRM(0x3, getRegCode(destReg), getRegCode(srcReg));
    } else {
        throw std::runtime_error("Unsupported IMUL operands");
    }
}

void X64Encoder::encodeIDIV(const Operand& src) {
    if (src.type == OperandType::REGISTER) {
        Register reg = src.reg;
        
        encodeREX(true, false, false, isExtendedReg(reg));
        emit8(0xF7);
        encodeModRM(0x3, 0x7, getRegCode(reg));
    } else {
        throw std::runtime_error("Unsupported IDIV operand");
    }
}

void X64Encoder::encodeCQO() {
    encodeREX(true, false, false, false);
    emit8(0x99);
}

void X64Encoder::encodeCMP(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::REGISTER) {
        Register destReg = dest.reg;
        Register srcReg = src.reg;
        
        encodeREX(true, isExtendedReg(srcReg), false, isExtendedReg(destReg));
        emit8(0x39);
        encodeModRM(0x3, getRegCode(srcReg), getRegCode(destReg));
        
    } else if (dest.type == OperandType::REGISTER && src.type == OperandType::IMMEDIATE) {
        Register destReg = dest.reg;
        int32_t imm = static_cast<int32_t>(src.immediate);
        
        encodeREX(true, false, false, isExtendedReg(destReg));
        emit8(0x81);
        encodeModRM(0x3, 0x7, getRegCode(destReg));
        emit32(imm);
    } else {
        throw std::runtime_error("Unsupported CMP operands");
    }
}

void X64Encoder::encodeTEST(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::REGISTER) {
        Register destReg = dest.reg;
        Register srcReg = src.reg;
        
        encodeREX(true, isExtendedReg(srcReg), false, isExtendedReg(destReg));
        emit8(0x85);
        encodeModRM(0x3, getRegCode(srcReg), getRegCode(destReg));
    } else {
        throw std::runtime_error("Unsupported TEST operands");
    }
}

void X64Encoder::encodeJMP(const Operand& target, InstructionType type) {
    if (target.type == OperandType::IMMEDIATE || target.type == OperandType::LABEL) {
        int32_t offset = (target.type == OperandType::IMMEDIATE) ? static_cast<int32_t>(target.immediate) : 0;
        
        uint8_t opcode;
        switch (type) {
            case InstructionType::JMP: opcode = 0xE9; break;
            case InstructionType::JE: opcode = 0x84; break;
            case InstructionType::JNE: opcode = 0x85; break;
            case InstructionType::JL: opcode = 0x8C; break;
            case InstructionType::JLE: opcode = 0x8E; break;
            case InstructionType::JG: opcode = 0x8F; break;
            case InstructionType::JGE: opcode = 0x8D; break;
            default: throw std::runtime_error("Unsupported jump type");
        }
        
        if (type == InstructionType::JMP) {
            emit8(opcode);
        } else {
            emit8(0x0F);
            emit8(opcode);
        }
        
        emit32(offset);
    } else {
        throw std::runtime_error("Unsupported JMP target");
    }
}

void X64Encoder::encodeCALL(const Operand& target) {
    if (target.type == OperandType::REGISTER) {
        Register reg = target.reg;
        
        encodeREX(false, false, false, isExtendedReg(reg));
        emit8(0xFF);
        encodeModRM(0x3, 0x2, getRegCode(reg));
    } else if (target.type == OperandType::IMMEDIATE || target.type == OperandType::LABEL) {
        // CALL rel32
        emit8(0xE8);
        int32_t offset = (target.type == OperandType::IMMEDIATE) ? static_cast<int32_t>(target.immediate) : 0;
        emit32(offset);
    } else if (target.type == OperandType::MEMORY && target.memory.base == Register::RIP) {
        // CALL [RIP + disp32] (Indirect call for imports)
        emit8(0xFF);
        encodeModRM(0x00, 0x02, 0x05);
        emit32(0); // Placeholder for relocation
    } else {
        throw std::runtime_error("Unsupported CALL target");
    }
}

void X64Encoder::encodeRET() {
    emit8(0xC3);
}

void X64Encoder::encodeNOP() {
    emit8(0x90);
}

void X64Encoder::encodeNEG(const Operand& op) {
    if (op.type == OperandType::REGISTER) {
        Register reg = op.reg;
        
        encodeREX(true, false, false, isExtendedReg(reg));
        emit8(0xF7);
        encodeModRM(0x3, 0x3, getRegCode(reg));
    } else {
        throw std::runtime_error("Unsupported NEG operand");
    }
}

void X64Encoder::encodeAND(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::REGISTER) {
        Register destReg = dest.reg;
        Register srcReg = src.reg;
        
        encodeREX(true, isExtendedReg(srcReg), false, isExtendedReg(destReg));
        emit8(0x21);
        encodeModRM(0x3, getRegCode(srcReg), getRegCode(destReg));
    } else {
        throw std::runtime_error("Unsupported AND operands");
    }
}

void X64Encoder::encodeOR(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::REGISTER) {
        Register destReg = dest.reg;
        Register srcReg = src.reg;
        
        encodeREX(true, isExtendedReg(srcReg), false, isExtendedReg(destReg));
        emit8(0x09);
        encodeModRM(0x3, getRegCode(srcReg), getRegCode(destReg));
    } else {
        throw std::runtime_error("Unsupported OR operands");
    }
}

void X64Encoder::encodeXOR(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::REGISTER) {
        Register destReg = dest.reg;
        Register srcReg = src.reg;
        
        encodeREX(true, isExtendedReg(srcReg), false, isExtendedReg(destReg));
        emit8(0x31);
        encodeModRM(0x3, getRegCode(srcReg), getRegCode(destReg));
    } else {
        throw std::runtime_error("Unsupported XOR operands");
    }
}

void X64Encoder::encodeINC(const Operand& op) {
    if (op.type == OperandType::REGISTER) {
        Register reg = op.reg;
        encodeREX(true, false, false, isExtendedReg(reg));
        emit8(0xFF);
        encodeModRM(0x3, 0x0, getRegCode(reg));
    } else {
        throw std::runtime_error("Unsupported INC operand");
    }
}

void X64Encoder::encodeDEC(const Operand& op) {
    if (op.type == OperandType::REGISTER) {
        Register reg = op.reg;
        encodeREX(true, false, false, isExtendedReg(reg));
        emit8(0xFF);
        encodeModRM(0x3, 0x1, getRegCode(reg));
    } else {
        throw std::runtime_error("Unsupported DEC operand");
    }
}

void X64Encoder::encodeSHL_(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::IMMEDIATE) {
        Register reg = dest.reg;
        uint8_t imm = static_cast<uint8_t>(src.immediate);
        
        encodeREX(true, false, false, isExtendedReg(reg));
        if (imm == 1) {
            emit8(0xD1);
            encodeModRM(0x3, 0x4, getRegCode(reg));
        } else {
            emit8(0xC1);
            encodeModRM(0x3, 0x4, getRegCode(reg));
            emit8(imm);
        }
    } else {
        throw std::runtime_error("Unsupported SHL operands");
    }
}

void X64Encoder::encodeSHR_(const Operand& dest, const Operand& src) {
    if (dest.type == OperandType::REGISTER && src.type == OperandType::IMMEDIATE) {
        Register reg = dest.reg;
        uint8_t imm = static_cast<uint8_t>(src.immediate);
        
        encodeREX(true, false, false, isExtendedReg(reg));
        if (imm == 1) {
            emit8(0xD1);
            encodeModRM(0x3, 0x5, getRegCode(reg));
        } else {
            emit8(0xC1);
            encodeModRM(0x3, 0x5, getRegCode(reg));
            emit8(imm);
        }
    } else {
        throw std::runtime_error("Unsupported SHR operands");
    }
}

} // namespace ArabicAssembler
