#include <stdio.h>

#include "cpu.h"
#include "fetch.h"
#include "memory.h"
#include "parser.h"

int main(void)
{
    printf("Starting Person 1 simulator for Fillet-O-Neumann...\n");

    // Initialize CPU state and shared memory before parsing instructions.
    cpu_reset();
    memory_init();

    // Load assembly text, encode each line, and store into instruction memory.
    parse_assembly_file("input/program.txt");
    memory_dump_instructions(16);

    printf("\nBeginning fetch stage. PC starts at 0.\n");
    for (int step = 0; step < 16; ++step)
    {
        instruction_t raw = fetch_instruction();
        if (raw == 0u)
        {
            printf("FETCH STOP: empty instruction at PC=%u\n", cpu.pc);
            break;
        }
        advance_pc();
    }

    printf("Fetch stage finished. Final PC=%u\n", cpu.pc);
    return 0;
}
