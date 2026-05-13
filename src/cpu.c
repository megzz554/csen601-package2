#include <stdio.h>
#include "cpu.h"

// CPU state for the simulator.
CPU cpu;

static bool is_instruction_address(int address)
{
    return address >= 0 && address < INSTRUCTION_MEMORY_SIZE;
}

static bool is_data_address(int address)
{
    return address >= DATA_MEMORY_START && address < MEMORY_SIZE;
}

void cpu_reset(void)
{
    cpu.PC = 0u;
    cpu.instructionCount = 0;
    cpu.memoryBusy = 0;

    for (int index = 0; index < MEMORY_SIZE; ++index)
    {
        cpu.memory[index] = 0;
    }

    for (int index = 0; index < REGISTER_COUNT; ++index)
    {
        cpu.registers[index] = 0;
    }
}

uint32_t cpu_get_pc(void)
{
    return cpu.PC;
}

void cpu_set_pc(uint32_t value)
{
    cpu.PC = value;
}

void cpu_increment_pc(void)
{
    cpu.PC += 1u;
}

void cpu_set_instruction_count(int count)
{
    cpu.instructionCount = count < 0 ? 0 : count;
}

int32_t readRegister(CPU *target, int reg)
{
    if (target == NULL)
    {
        printf("REGISTER ERROR: CPU pointer is null\n");
        return 0;
    }

    if (reg < 0 || reg >= REGISTER_COUNT)
    {
        printf("REGISTER ERROR: R%d is out of range\n", reg);
        return 0;
    }

    if (reg == 0)
    {
        return 0;
    }

    return target->registers[reg];
}

void writeRegister(CPU *target, int reg, int32_t value)
{
    if (target == NULL)
    {
        printf("REGISTER ERROR: CPU pointer is null\n");
        return;
    }

    if (reg < 0 || reg >= REGISTER_COUNT)
    {
        printf("REGISTER ERROR: R%d is out of range\n", reg);
        return;
    }

    if (reg == 0)
    {
        target->registers[0] = 0;
        printf("[REG] IGNORE write R0 value=%d\n", value);
        return;
    }

    target->registers[reg] = value;
    printf("[REG] WRITE R%d value=%d\n", reg, value);
}

int32_t readMemory(CPU *target, int address)
{
    if (target == NULL)
    {
        printf("MEMORY ERROR: CPU pointer is null\n");
        return 0;
    }

    if (!is_data_address(address))
    {
        printf("MEMORY ERROR: data read address=%d outside data segment [%d..%d]\n",
               address,
               DATA_MEMORY_START,
               MEMORY_SIZE - 1);
        return 0;
    }

    if (target->memoryBusy)
    {
        printf("MEMORY WARNING: memory already busy before data read address=%d\n", address);
    }

    target->memoryBusy = 1;
    printf("[MEM] READ address=%d value=%d\n", address, target->memory[address]);
    target->memoryBusy = 0;
    return target->memory[address];
}

void writeMemory(CPU *target, int address, int32_t value)
{
    if (target == NULL)
    {
        printf("MEMORY ERROR: CPU pointer is null\n");
        return;
    }

    if (!is_data_address(address))
    {
        printf("MEMORY ERROR: data write address=%d outside data segment [%d..%d]\n",
               address,
               DATA_MEMORY_START,
               MEMORY_SIZE - 1);
        return;
    }

    if (target->memoryBusy)
    {
        printf("MEMORY WARNING: memory already busy before data write address=%d\n", address);
    }

    target->memoryBusy = 1;
    target->memory[address] = value;
    printf("[MEM] WRITE address=%d value=%d\n", address, value);
    target->memoryBusy = 0;
}

uint32_t fetchInstruction(CPU *target)
{
    uint32_t raw;

    if (target == NULL)
    {
        printf("MEMORY ERROR: CPU pointer is null\n");
        return 0u;
    }

    if (target->PC >= (uint32_t)target->instructionCount)
    {
        printf("FETCH STOP: PC=%u instructionCount=%d\n", target->PC, target->instructionCount);
        return 0u;
    }

    if (!is_instruction_address((int)target->PC))
    {
        printf("MEMORY ERROR: fetch address %u out of instruction memory range [0..%d]\n",
               target->PC,
               INSTRUCTION_MEMORY_SIZE - 1);
        return 0u;
    }

    if (target->memoryBusy)
    {
        printf("MEMORY WARNING: IF/MEM conflict at PC=%u\n", target->PC);
    }

    target->memoryBusy = 1;
    raw = (uint32_t)target->memory[target->PC];
    printf("FETCH: PC=%u raw=0x%08X\n", target->PC, raw);
    target->PC += 1u;
    printf("PC increment -> %u\n", target->PC);
    target->memoryBusy = 0;

    return raw;
}

void dumpRegisters(CPU *target)
{
    if (target == NULL)
    {
        printf("REGISTER ERROR: CPU pointer is null\n");
        return;
    }

    printf("\nDUMP: register file:\n");
    printf("  PC  = %u\n", target->PC);
    printf("  instructionCount = %d\n", target->instructionCount);
    printf("  memoryBusy = %d\n", target->memoryBusy);
    for (int index = 0; index < REGISTER_COUNT; ++index)
    {
        int32_t value = readRegister(target, index);
        printf("  R%02d = %11d (0x%08X)\n", index, value, (uint32_t)value);
    }
}

void dumpMemory(CPU *target)
{
    if (target == NULL)
    {
        printf("MEMORY ERROR: CPU pointer is null\n");
        return;
    }

    printf("\nDUMP: full memory:\n");
    for (int address = 0; address < MEMORY_SIZE; ++address)
    {
        printf("  [%04d] %11d (0x%08X)\n",
               address,
               target->memory[address],
               (uint32_t)target->memory[address]);
    }
}

int32_t cpu_read_register(uint8_t index)
{
    return readRegister(&cpu, index);
}

bool cpu_write_register(uint8_t index, int32_t value)
{
    if (index >= REGISTER_COUNT)
    {
        printf("REGISTER ERROR: R%u is out of range\n", index);
        return false;
    }

    writeRegister(&cpu, index, value);
    return true;
}

void cpu_dump_registers(void)
{
    dumpRegisters(&cpu);
}
