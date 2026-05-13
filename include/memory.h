#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include "cpu.h"
#include "instruction.h"

void memory_init(void);
void memory_store_instruction(uint32_t address, instruction_t instruction);
instruction_t memory_fetch_instruction(uint32_t address);
void memory_dump_instructions(uint32_t count);

#endif // MEMORY_H
