#include "cpu.h"

// CPU state for the simulator. Person 1 is responsible for PC management and fetch.
cpu_state_t cpu;

void cpu_reset(void)
{
    cpu.pc = 0u;
}
