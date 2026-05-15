#ifndef EXECUTE_H
#define EXECUTE_H

#include <stdbool.h>
#include <stdint.h>

#include "cpu.h"
#include "decode_types.h"

typedef struct
{
    int32_t result;

    bool zero;
    bool negative;
    bool carry;
    bool overflow;

} ALUResult;

ALUResult executeALU(alu_operation_t operation,
                     int32_t operand1,
                     int32_t operand2,
                     int32_t immediate,
                     int shamt);

void executeStage(CPU *cpu,
                  PipelineRegister *stage);

bool evaluateBranch(PipelineRegister *stage,
                    uint32_t *targetPC);

#endif