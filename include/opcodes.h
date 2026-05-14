#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>

#include "decode_types.h"
#include "instruction.h"

// Package 2 bit layout.
//   R-format: [31:28] opcode, [27:23] R1, [22:18] R2, [17:13] R3, [12:0] shamt
//   I-format: [31:28] opcode, [27:23] R1, [22:18] R2, [17:0] immediate
//   J-format: [31:28] opcode, [27:0] address

#define OPCODE_SHIFT 28u
#define R1_SHIFT 23u
#define R2_SHIFT 18u
#define R3_SHIFT 13u
#define SHAMT_MASK 0x1FFFu
#define IMM_MASK 0x3FFFFu
#define ADDR_MASK 0x0FFFFFFFu
#define REGISTER_MASK 0x1Fu

typedef enum
{
    OPCODE_ADD = 0,
    OPCODE_SUB = 1,
    OPCODE_MUL = 2,
    OPCODE_MOVI = 3,
    OPCODE_JEQ = 4,
    OPCODE_AND = 5,
    OPCODE_ORI = 6,
    OPCODE_JMP = 7,
    OPCODE_LSL = 8,
    OPCODE_LSR = 9,
    OPCODE_MOVR = 10,
    OPCODE_MOVM = 11
} opcode_t;

// Backward-compatible alias for the earlier Person 1 sample program.
#define OPCODE_XORI OPCODE_ORI

int opcode_from_string(const char *text);
instruction_format_t opcode_instruction_format(opcode_t opcode);
alu_operation_t opcode_alu_operation(opcode_t opcode);
bool opcode_writes_register(opcode_t opcode);
instruction_t encode_r_type(opcode_t opcode, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t shamt);
instruction_t encode_i_type(opcode_t opcode, uint32_t r1, uint32_t r2, int32_t immediate);
instruction_t encode_j_type(opcode_t opcode, uint32_t address);

#endif // OPCODES_H
