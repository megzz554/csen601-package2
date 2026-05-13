#include <stdio.h>
#include "fetch.h"
#include "memory.h"
#include "cpu.h"

// Fetch stage: read the raw encoded instruction word at the current PC from memory.
// This stage is intentionally independent from decoding or instruction field layout.
instruction_t fetch_instruction(void)
{
    instruction_t raw = memory_fetch_instruction(cpu.pc);
    printf("FETCH: PC=%u raw=0x%08X\n", cpu.pc, raw);
    return raw;
}

// Simple PC increment logic for Person 1.
void advance_pc(void)
{
    cpu.pc += 1u;
    printf("PC increment -> %u\n", cpu.pc);
}
