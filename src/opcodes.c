#include <string.h>
#include "opcodes.h"

// This file isolates raw instruction encoding for Person 1.
// The parser only calls these helpers and does not depend on the exact bit layout.
// If the official Package 2 instruction format is later specified, update this file
// and its header without changing parser or fetch logic.
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

// Build an R-type instruction word from opcode and register fields.
instruction_t encode_r_type(opcode_t opcode, uint32_t rs, uint32_t rt, uint32_t rd)
{
    return ((instruction_t)opcode << OPCODE_SHIFT) | ((instruction_t)rs << REG_SHIFT_RS) | ((instruction_t)rt << REG_SHIFT_RT) | ((instruction_t)rd << REG_SHIFT_RD);
}

instruction_t encode_i_type(opcode_t opcode, uint32_t rs, uint32_t rt, int32_t immediate)
{
    uint32_t imm = (uint32_t)immediate & IMM_MASK;
    return ((instruction_t)opcode << OPCODE_SHIFT) | ((instruction_t)rs << REG_SHIFT_RS) | ((instruction_t)rt << REG_SHIFT_RT) | imm;
}

instruction_t encode_j_type(opcode_t opcode, uint32_t address)
{
    return ((instruction_t)opcode << OPCODE_SHIFT) | (address & ADDR_MASK);
}
