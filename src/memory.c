#include <stdio.h>
#include "memory.h"
#include "cpu.h"

// Shared memory for the Von Neumann machine.
// Instruction memory occupies addresses 0..1023 and data memory occupies 1024..2047.
static instruction_t memory_space[MEMORY_SIZE];

void memory_init(void)
{
    for (uint32_t index = 0u; index < MEMORY_SIZE; ++index)
    {
        memory_space[index] = 0u;
    }
}

void memory_store_instruction(uint32_t address, instruction_t instruction)
{
    if (address >= INSTRUCTION_MEMORY_SIZE)
    {
        printf("MEMORY ERROR: instruction address %u out of instruction memory range\n", address);
        return;
    }

    memory_space[address] = instruction;
    printf("STORE: memory[%u] = 0x%08X\n", address, instruction);
}

// Read a raw 32-bit value from memory at the given address.
instruction_t memory_fetch_instruction(uint32_t address)
{
    if (address >= MEMORY_SIZE)
    {
        printf("MEMORY ERROR: fetch address %u out of bounds\n", address);
        return 0u;
    }

    return memory_space[address];
}

void memory_dump_instructions(uint32_t count)
{
    uint32_t limit = count;
    if (limit > INSTRUCTION_MEMORY_SIZE)
    {
        limit = INSTRUCTION_MEMORY_SIZE;
    }

    printf("\nDUMP: first %u instruction memory words:\n", limit);
    for (uint32_t index = 0u; index < limit; ++index)
    {
        printf("  [%03u] 0x%08X\n", index, memory_space[index]);
    }
}
