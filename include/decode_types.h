#ifndef DECODE_TYPES_H
#define DECODE_TYPES_H

#include <stdbool.h>

typedef enum
{
    INSTRUCTION_FORMAT_INVALID = 0,
    INSTRUCTION_FORMAT_R,
    INSTRUCTION_FORMAT_I,
    INSTRUCTION_FORMAT_J
} instruction_format_t;

typedef enum
{
    ALU_OP_NONE = 0,
    ALU_OP_ADD,
    ALU_OP_SUB,
    ALU_OP_MUL,
    ALU_OP_AND,
    ALU_OP_OR,
    ALU_OP_SHIFT_LEFT,
    ALU_OP_SHIFT_RIGHT,
    ALU_OP_PASS_B
} alu_operation_t;

typedef struct
{
    bool reg_write;
    bool mem_read;
    bool mem_write;
    bool mem_to_reg;
    bool branch;
    bool jump;
    bool use_immediate;
    bool use_shamt;
    alu_operation_t alu_op;
} control_signals_t;

#endif // DECODE_TYPES_H