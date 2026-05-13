#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stdint.h>

#define MEMORY_SIZE 2048
#define INSTRUCTION_MEMORY_SIZE 1024
#define DATA_MEMORY_START 1024
#define DATA_MEMORY_SIZE (MEMORY_SIZE - DATA_MEMORY_START)
#define REGISTER_COUNT 32

typedef struct
{
    int32_t memory[MEMORY_SIZE];
    int32_t registers[REGISTER_COUNT];
    uint32_t PC;

    int instructionCount;
    int memoryBusy;
} CPU;

typedef struct
{
    int valid;

    uint32_t rawInstruction;

    int opcode;

    int r1;
    int r2;
    int r3;

    int immediate;
    int shamt;
    int address;

    int32_t operand1;
    int32_t operand2;

    int32_t aluResult;

    int32_t memoryData;

    uint32_t pc;
} PipelineRegister;

typedef CPU cpu_state_t;

extern CPU cpu;

void cpu_reset(void);
uint32_t cpu_get_pc(void);
void cpu_set_pc(uint32_t value);
void cpu_increment_pc(void);
void cpu_set_instruction_count(int count);

int32_t readMemory(CPU *cpu, int address);
void writeMemory(CPU *cpu, int address, int32_t value);
uint32_t fetchInstruction(CPU *cpu);
int32_t readRegister(CPU *cpu, int reg);
void writeRegister(CPU *cpu, int reg, int32_t value);
void dumpRegisters(CPU *cpu);
void dumpMemory(CPU *cpu);

// Compatibility wrappers for the earlier project files.
int32_t cpu_read_register(uint8_t index);
bool cpu_write_register(uint8_t index, int32_t value);
void cpu_dump_registers(void);

#endif // CPU_H
