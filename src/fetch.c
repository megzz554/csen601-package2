#include <stdio.h>
#include "fetch.h"
#include "memory.h"
#include "cpu.h"

// Fetch stage: read the raw encoded instruction word at the current PC from memory.
// This stage is intentionally independent from decoding or instruction field layout.
instruction_t fetch_instruction(void)
{
    return fetchInstruction(&cpu);
}

// fetchInstruction owns PC incrementing. Kept only for older call sites.
void advance_pc(void)
{
}
