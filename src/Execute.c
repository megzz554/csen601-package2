#include <stdio.h>
#include <limits.h>

#include "execute.h"
#include "opcodes.h"

static bool add_overflow(int32_t a, int32_t b, int32_t result)
{
    return ((a > 0 && b > 0 && result < 0) ||
            (a < 0 && b < 0 && result > 0));
}

static bool sub_overflow(int32_t a, int32_t b, int32_t result)
{
    return ((a > 0 && b < 0 && result < 0) ||
            (a < 0 && b > 0 && result > 0));
}

ALUResult executeALU(alu_operation_t operation,
                     int32_t operand1,
                     int32_t operand2,
                     int32_t immediate,
                     int shamt)
{
    ALUResult alu;

    alu.result = 0;
    alu.zero = false;
    alu.negative = false;
    alu.carry = false;
    alu.overflow = false;

    uint64_t wideResult;

    switch (operation)
    {
        case ALU_OP_ADD:

            wideResult =
                (uint64_t)(uint32_t)operand1 +
                (uint64_t)(uint32_t)operand2;

            alu.result = operand1 + operand2;

            alu.carry = (wideResult > UINT32_MAX);

            alu.overflow =
                add_overflow(operand1,
                             operand2,
                             alu.result);

            break;

        case ALU_OP_SUB:

            alu.result = operand1 - operand2;

            alu.overflow =
                sub_overflow(operand1,
                             operand2,
                             alu.result);

            break;

        case ALU_OP_MUL:

            alu.result = operand1 * operand2;

            break;

        case ALU_OP_AND:

            alu.result = operand1 & operand2;

            break;

        case ALU_OP_OR:

            alu.result = operand1 | immediate;

            break;

        case ALU_OP_SHIFT_LEFT:

            alu.result =
                (int32_t)((uint32_t)operand1 << shamt);

            break;

        case ALU_OP_SHIFT_RIGHT:

            alu.result =
                (int32_t)((uint32_t)operand1 >> shamt);

            break;

        case ALU_OP_PASS_B:

            alu.result = operand2;

            break;

        case ALU_OP_NONE:
        default:
            break;
    }

    alu.zero = (alu.result == 0);
    alu.negative = (alu.result < 0);

    return alu;
}

bool evaluateBranch(PipelineRegister *stage,
                    uint32_t *targetPC)
{
    if (stage == NULL || targetPC == NULL)
    {
        return false;
    }

    switch (stage->opcode)
    {
        case OPCODE_JEQ:

            if (stage->operand1 == stage->operand2)
            {
                *targetPC =
                    stage->pc +
                    1u +
                    (uint32_t)stage->immediate;

                return true;
            }

            return false;

        case OPCODE_JMP:

            *targetPC =
                (stage->pc & 0xF0000000u) |
                ((uint32_t)stage->address & ADDR_MASK);

            return true;

        default:
            return false;
    }
}

void executeStage(CPU *cpu,
                  PipelineRegister *stage)
{
    (void)cpu;

    if (stage == NULL || !stage->valid)
    {
        return;
    }

    ALUResult alu =
        executeALU(stage->control.alu_op,
                   stage->operand1,
                   stage->operand2,
                   stage->immediate,
                   stage->shamt);

    stage->aluResult = alu.result;

    printf(" [EX] ALU result = %d\n",
           stage->aluResult);

    if (alu.carry)
    {
        printf(" [EX] Carry detected\n");
    }

    if (alu.overflow)
    {
        printf(" [EX] Overflow detected\n");
    }

    if (stage->opcode == OPCODE_MOVR ||
        stage->opcode == OPCODE_MOVM)
    {
        stage->address =
            stage->operand1 +
            stage->immediate;

        stage->memoryData =
            stage->operand2;
    }
}