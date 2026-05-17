#include <stdio.h>
#include <stdbool.h>

#include "cpu.h"
#include "memory.h"
#include "opcodes.h"
#include "parser.h"
#include "pipeline_sim.h"

static bool record_check(const char *name, bool passed)
{
    printf("  [%s] %s\n", passed ? "PASS" : "FAIL", name);
    return passed;
}

static bool run_person2_self_test(void)
{
    bool passed = true;
    int32_t value = 0;
    memory_stage_output_t mem_output;

   // printf("\n--- Person 2 self-test: memory + register file ---\n");

    cpu_reset();
    memory_init();

    passed &= cpu_write_register(1u, 123);
    passed &= record_check("write R1", cpu_read_register(1u) == 123);

    passed &= cpu_write_register(0u, 999);
    passed &= record_check("R0 remains hardwired to 0", cpu_read_register(0u) == 0);

    passed &= memory_write_data(DATA_MEMORY_START, 55);
    passed &= memory_read_data(DATA_MEMORY_START, &value);
    passed &= record_check("data memory write/read at first data address", value == 55);

    passed &= memory_store_from_register(1u, DATA_MEMORY_START + 1u);
    passed &= memory_load_to_register(2u, DATA_MEMORY_START + 1u);
    passed &= record_check("store R1 then load into R2", cpu_read_register(2u) == 123);

    mem_output = memory_execute_stage((memory_stage_input_t){
        .access = MEMORY_ACCESS_LOAD,
        .address = DATA_MEMORY_START + 1u,
        .write_data = 0,
        .passthrough_value = 0,
        .destination_register = 3u,
        .register_write = true,
    });
    passed &= record_check("MEM stage load reports write-back value", mem_output.success && mem_output.register_write && mem_output.write_back_value == 123);

    mem_output = memory_execute_stage((memory_stage_input_t){
        .access = MEMORY_ACCESS_STORE,
        .address = DATA_MEMORY_START + 2u,
        .write_data = -7,
        .passthrough_value = 0,
        .destination_register = 0u,
        .register_write = false,
    });
    passed &= memory_read_data(DATA_MEMORY_START + 2u, &value);
    passed &= record_check("MEM stage store writes data memory", mem_output.success && value == -7);

    cpu_write_register(4u, DATA_MEMORY_START);
    memory_write_data(DATA_MEMORY_START + 3u, 444);
    PipelineRegister movr_stage = {
        .valid = 1,
        .opcode = OPCODE_MOVR,
        .r1 = 5,
        .r2 = 4,
        .immediate = 3,
    };
    executeMemoryStage(&cpu, &movr_stage);
    passed &= record_check("MOVR loads R1 = MEM[R2 + IMM]", cpu_read_register(5u) == 444 && movr_stage.memoryData == 444);

    cpu_write_register(6u, -22);
    PipelineRegister movm_stage = {
        .valid = 1,
        .opcode = OPCODE_MOVM,
        .r1 = 6,
        .r2 = 4,
        .immediate = 4,
    };
    executeMemoryStage(&cpu, &movm_stage);
    passed &= memory_read_data(DATA_MEMORY_START + 4u, &value);
    passed &= record_check("MOVM stores MEM[R2 + IMM] = R1", value == -22);

    passed &= record_check("reject write into instruction segment", !memory_write_data(100u, 77));
    passed &= record_check("reject write past memory end", !memory_write_data(MEMORY_SIZE, 88));
    passed &= record_check("reject instruction fetch from data segment", memory_fetch_instruction(DATA_MEMORY_START) == 0u);

    memory_store_instruction(0u, 0u);
    memory_store_instruction(1u, 0xDEADBEEFu);
    cpu_set_pc(0u);
    cpu_set_instruction_count(2);
    uint32_t fetched_zero = fetchInstruction(&cpu);
    uint32_t fetched_nonzero = fetchInstruction(&cpu);
    uint32_t fetched_after_end = fetchInstruction(&cpu);
    passed &= record_check("fetch accepts raw instruction 0", fetched_zero == 0u && cpu_get_pc() == 2u);
    passed &= record_check("fetch stops only at instructionCount", fetched_nonzero == 0xDEADBEEFu && fetched_after_end == 0u);

    printf("--- Person 2 self-test complete ---\n\n");
    return passed;
}

int main(int argc, char **argv)
{
    const char *program_path = argc > 1 ? argv[1] : "input/program.txt";

    printf("Starting simulator for Fillet-O-Neumann...\n");

    if (!run_person2_self_test())
    {
        printf("Person 2 self-test failed. Stop and fix memory/register behavior first.\n");
        return 1;
    }

    // Initialize CPU state and shared memory before parsing instructions.
    cpu_reset();
    memory_init();

    // Load assembly text, encode each line, and store into instruction memory.
    printf("Loading assembly program: %s\n", program_path);
    parse_assembly_file(program_path);
    memory_dump_instructions(16);

    pipeline_sim_run(&cpu, (PipelineSimOptions){
                               .dump_nonzero_memory_only = false,
                               .max_cycles = 300,
                           });
    return 0;
}
