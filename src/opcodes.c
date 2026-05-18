#include <string.h>
#include "opcodes.h"

static bool is_shift_opcode(opcode_t opcode)
{
    return opcode == OPCODE_LSL || opcode == OPCODE_LSR;
}

int opcode_from_string(const char *text)
{
    if (text == NULL)
    {
        return -1;
    }

    if (strcmp(text, "ADD") == 0)
    {
        return OPCODE_ADD;
    }
    if (strcmp(text, "SUB") == 0)
    {
        return OPCODE_SUB;
    }
    if (strcmp(text, "MUL") == 0)
    {
        return OPCODE_MUL;
    }
    if (strcmp(text, "MOVI") == 0)
    {
        return OPCODE_MOVI;
    }
    if (strcmp(text, "JEQ") == 0)
    {
        return OPCODE_JEQ;
    }
    if (strcmp(text, "AND") == 0)
    {
        return OPCODE_AND;
    }
    if (strcmp(text, "ORI") == 0 || strcmp(text, "XORI") == 0)
    {
        return OPCODE_ORI;
    }
    if (strcmp(text, "JMP") == 0)
    {
        return OPCODE_JMP;
    }
    if (strcmp(text, "LSL") == 0)
    {
        return OPCODE_LSL;
    }
    if (strcmp(text, "LSR") == 0)
    {
        return OPCODE_LSR;
    }
    if (strcmp(text, "MOVR") == 0)
    {
        return OPCODE_MOVR;
    }
    if (strcmp(text, "MOVM") == 0)
    {
        return OPCODE_MOVM;
    }

    return -1;
}

instruction_format_t opcode_instruction_format(opcode_t opcode)
{
    switch (opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_AND:
    case OPCODE_LSL:
    case OPCODE_LSR:
        return INSTRUCTION_FORMAT_R;
    case OPCODE_JMP:
        return INSTRUCTION_FORMAT_J;
    case OPCODE_MOVI:
    case OPCODE_JEQ:
    case OPCODE_ORI:
    case OPCODE_MOVR:
    case OPCODE_MOVM:
        return INSTRUCTION_FORMAT_I;
    default:
        return INSTRUCTION_FORMAT_INVALID;
    }
}

alu_operation_t opcode_alu_operation(opcode_t opcode)
{
    switch (opcode)
    {
    case OPCODE_ADD:
        return ALU_OP_ADD;
    case OPCODE_SUB:
        return ALU_OP_SUB;
    case OPCODE_MUL:
        return ALU_OP_MUL;
    case OPCODE_AND:
        return ALU_OP_AND;
    case OPCODE_ORI:
        return ALU_OP_XOR;
    case OPCODE_LSL:
        return ALU_OP_SHIFT_LEFT;
    case OPCODE_LSR:
        return ALU_OP_SHIFT_RIGHT;
    case OPCODE_MOVI:
        return ALU_OP_PASS_B;
    default:
        return ALU_OP_NONE;
    }
}

bool opcode_writes_register(opcode_t opcode)
{
    switch (opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_MOVI:
    case OPCODE_AND:
    case OPCODE_ORI:
    case OPCODE_LSL:
    case OPCODE_LSR:
    case OPCODE_MOVR:
        return true;
    default:
        return false;
    }
}

instruction_t encode_r_type(opcode_t opcode, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t shamt)
{
    uint32_t encoded_shamt = shamt & SHAMT_MASK;

    if (!is_shift_opcode(opcode))
    {
        encoded_shamt = shamt & SHAMT_MASK;
    }

    return ((instruction_t)opcode << OPCODE_SHIFT) |
           ((instruction_t)r1 << R1_SHIFT) |
           ((instruction_t)r2 << R2_SHIFT) |
           ((instruction_t)r3 << R3_SHIFT) |
           encoded_shamt;
}

instruction_t encode_i_type(opcode_t opcode, uint32_t r1, uint32_t r2, int32_t immediate)
{
    uint32_t imm = (uint32_t)immediate & IMM_MASK;
    return ((instruction_t)opcode << OPCODE_SHIFT) |
           ((instruction_t)r1 << R1_SHIFT) |
           ((instruction_t)r2 << R2_SHIFT) |
           imm;
}

instruction_t encode_j_type(opcode_t opcode, uint32_t address)
{
    return ((instruction_t)opcode << OPCODE_SHIFT) | (address & ADDR_MASK);
}
