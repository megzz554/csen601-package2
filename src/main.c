#include <stdio.h>
#include "cpu.h"
#include "memory.h"
#include "opcodes.h"
#include "parser.h"
#include "pipeline_sim.h"

int main(int argc, char **argv)
{
    const char *program_path = argc > 1 ? argv[1] : "input/program.txt";

    printf("Starting simulator for Fillet-O-Neumann...\n");

    // Initialize CPU state and shared memory before parsing instructions.
    cpu_reset();
    memory_init();

    // Load assembly text, encode each line, and store into instruction memory.
    printf("Loading assembly program: %s\n", program_path);
    parse_assembly_file(program_path);

    pipeline_sim_run(&cpu, (PipelineSimOptions){
                               .dump_nonzero_memory_only = true,
                               .max_cycles = 0,
                           });
    return 0;
}
