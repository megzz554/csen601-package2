#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define MEMORY_SIZE 2048u
#define INSTRUCTION_MEMORY_SIZE 1024u
#define DATA_MEMORY_START 1024u

typedef struct
{
    uint32_t pc;
} cpu_state_t;

extern cpu_state_t cpu;

void cpu_reset(void);

#endif // CPU_H