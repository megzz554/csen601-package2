#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>
#include "instruction.h"

// NOTE: Official Package 2 instruction bit layout is not explicitly supplied here.
// The current encoding is a provisional, isolated implementation for Person 1.
// This file is intended to keep encoding details separate from parser/fetch.
// When the project specification defines the exact layout, only this file
// and the matching implementation in src/opcodes.c should need update.
//
// Current provisional layout:
//   [31:26] opcode (6 bits)
//   [25:21] rs     (5 bits)
//   [20:16] rt     (5 bits)
//   [15:11] rd     (5 bits)
//   [15:0]  immediate for I-type
//   [25:0]  address for J-type

#define OPCODE_SHIFT 26u
#define REG_SHIFT_RS 21u
#define REG_SHIFT_RT 16u
#define REG_SHIFT_RD 11u
#define IMM_MASK 0xFFFFu
#define ADDR_MASK 0x03FFFFFFu

typedef enum
{
    OPCODE_ADD = 0,
    OPCODE_SUB = 1,
    OPCODE_MUL = 2,
    OPCODE_MOVI = 3,
    OPCODE_JEQ = 4,
    OPCODE_AND = 5,
    OPCODE_XORI = 6,
    OPCODE_JMP = 7,
    OPCODE_LSL = 8,
    OPCODE_LSR = 9,
    OPCODE_MOVR = 10,
    OPCODE_MOVM = 11
} opcode_t;

int opcode_from_string(const char *text);
instruction_t encode_r_type(opcode_t opcode, uint32_t rs, uint32_t rt, uint32_t rd);
instruction_t encode_i_type(opcode_t opcode, uint32_t rs, uint32_t rt, int32_t immediate);
instruction_t encode_j_type(opcode_t opcode, uint32_t address);

#endif // OPCODES_H
