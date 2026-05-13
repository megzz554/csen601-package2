#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>
#include <stdint.h>
#include "cpu.h"
#include "instruction.h"

typedef enum
{
    MEMORY_ACCESS_NONE = 0,
    MEMORY_ACCESS_LOAD,
    MEMORY_ACCESS_STORE
} memory_access_t;

typedef struct
{
    memory_access_t access;
    uint32_t address;
    int32_t write_data;
    int32_t passthrough_value;
    uint8_t destination_register;
    bool register_write;
} memory_stage_input_t;

typedef struct
{
    bool success;
    int32_t write_back_value;
    uint8_t destination_register;
    bool register_write;
} memory_stage_output_t;

void memory_init(void);
void memory_store_instruction(uint32_t address, instruction_t instruction);
instruction_t memory_fetch_instruction(uint32_t address);
bool memory_read_data(uint32_t address, int32_t *value);
bool memory_write_data(uint32_t address, int32_t value);
bool memory_load_to_register(uint8_t destination_register, uint32_t address);
bool memory_store_from_register(uint8_t source_register, uint32_t address);
void executeMOVR(CPU *cpu, PipelineRegister *stage);
void executeMOVM(CPU *cpu, PipelineRegister *stage);
void executeMemoryStage(CPU *cpu, PipelineRegister *stage);
void executeMovr(CPU *cpu, PipelineRegister *stage);
void executeMovm(CPU *cpu, PipelineRegister *stage);
memory_stage_output_t memory_execute_stage(memory_stage_input_t input);
void memory_dump_instructions(uint32_t count);
void memory_dump_data(uint32_t start_address, uint32_t count);

#endif // MEMORY_H
