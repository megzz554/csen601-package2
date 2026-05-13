#include <stdio.h>
#include "memory.h"
#include "cpu.h"
#include "opcodes.h"

static bool is_instruction_address(int address)
{
    return address >= 0 && address < INSTRUCTION_MEMORY_SIZE;
}

static bool is_data_address(int address)
{
    return address >= DATA_MEMORY_START && address < MEMORY_SIZE;
}

void memory_init(void)
{
    for (int index = 0; index < MEMORY_SIZE; ++index)
    {
        cpu.memory[index] = 0;
    }
}

void memory_store_instruction(uint32_t address, instruction_t instruction)
{
    if (!is_instruction_address((int)address))
    {
        printf("MEMORY ERROR: instruction address %u out of instruction memory range\n", address);
        return;
    }

    cpu.memory[address] = (int32_t)instruction;
    printf("STORE: memory[%u] = 0x%08X\n", address, instruction);
}

instruction_t memory_fetch_instruction(uint32_t address)
{
    if (!is_instruction_address((int)address))
    {
        printf("MEMORY ERROR: fetch address %u out of instruction memory range\n", address);
        return 0u;
    }

    return (instruction_t)cpu.memory[address];
}

bool memory_read_data(uint32_t address, int32_t *value)
{
    if (value == NULL)
    {
        printf("MEMORY ERROR: read destination is null\n");
        return false;
    }

    if (!is_data_address((int)address))
    {
        printf("MEMORY ERROR: data read address %u outside data segment [%u..%u]\n",
               address,
               DATA_MEMORY_START,
               MEMORY_SIZE - 1u);
        *value = 0;
        return false;
    }

    *value = readMemory(&cpu, (int)address);
    return true;
}

bool memory_write_data(uint32_t address, int32_t value)
{
    if (!is_data_address((int)address))
    {
        printf("MEMORY ERROR: data write address %u outside data segment [%u..%u]\n",
               address,
               DATA_MEMORY_START,
               MEMORY_SIZE - 1u);
        return false;
    }

    writeMemory(&cpu, (int)address, value);
    return true;
}

bool memory_load_to_register(uint8_t destination_register, uint32_t address)
{
    int32_t value = 0;
    if (!memory_read_data(address, &value))
    {
        return false;
    }

    return cpu_write_register(destination_register, value);
}

bool memory_store_from_register(uint8_t source_register, uint32_t address)
{
    int32_t value = cpu_read_register(source_register);
    return memory_write_data(address, value);
}

void executeMOVR(CPU *target, PipelineRegister *stage)
{
    int effective_address;

    if (target == NULL || stage == NULL || !stage->valid)
    {
        return;
    }

    effective_address = readRegister(target, stage->r2) + stage->immediate;
    stage->address = effective_address;
    if (!is_data_address(effective_address))
    {
        readMemory(target, effective_address);
        stage->valid = 0;
        return;
    }

    stage->memoryData = readMemory(target, effective_address);
    writeRegister(target, stage->r1, stage->memoryData);
}

void executeMOVM(CPU *target, PipelineRegister *stage)
{
    int effective_address;
    int32_t value;

    if (target == NULL || stage == NULL || !stage->valid)
    {
        return;
    }

    effective_address = readRegister(target, stage->r2) + stage->immediate;
    value = readRegister(target, stage->r1);
    stage->address = effective_address;
    stage->memoryData = value;
    if (!is_data_address(effective_address))
    {
        writeMemory(target, effective_address, value);
        stage->valid = 0;
        return;
    }

    writeMemory(target, effective_address, value);
}

void executeMemoryStage(CPU *target, PipelineRegister *stage)
{
    if (stage == NULL || !stage->valid)
    {
        return;
    }

    if (stage->opcode == OPCODE_MOVR)
    {
        executeMOVR(target, stage);
    }
    else if (stage->opcode == OPCODE_MOVM)
    {
        executeMOVM(target, stage);
    }
}

void executeMovr(CPU *target, PipelineRegister *stage)
{
    executeMOVR(target, stage);
}

void executeMovm(CPU *target, PipelineRegister *stage)
{
    executeMOVM(target, stage);
}

memory_stage_output_t memory_execute_stage(memory_stage_input_t input)
{
    memory_stage_output_t output;
    output.success = true;
    output.write_back_value = input.passthrough_value;
    output.destination_register = input.destination_register;
    output.register_write = input.register_write;

    switch (input.access)
    {
    case MEMORY_ACCESS_NONE:
        printf("[MEM] PASS value=%d hex=0x%08X\n",
               output.write_back_value,
               (uint32_t)output.write_back_value);
        break;
    case MEMORY_ACCESS_LOAD:
        output.success = memory_read_data(input.address, &output.write_back_value);
        output.register_write = output.success && input.register_write;
        break;
    case MEMORY_ACCESS_STORE:
        output.success = memory_write_data(input.address, input.write_data);
        output.register_write = false;
        break;
    default:
        printf("MEMORY ERROR: unknown memory access type %d\n", input.access);
        output.success = false;
        output.register_write = false;
        break;
    }

    return output;
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
        printf("  [%03u] 0x%08X\n", index, (uint32_t)cpu.memory[index]);
    }
}

void memory_dump_data(uint32_t start_address, uint32_t count)
{
    if (count == 0u)
    {
        printf("\nDUMP: data memory requested 0 words\n");
        return;
    }

    if (!is_data_address((int)start_address))
    {
        printf("MEMORY ERROR: data dump start address %u outside data segment [%u..%u]\n",
               start_address,
               DATA_MEMORY_START,
               MEMORY_SIZE - 1u);
        return;
    }

    uint32_t end_address = start_address + count;
    if (end_address > MEMORY_SIZE || end_address < start_address)
    {
        end_address = MEMORY_SIZE;
    }

    printf("\nDUMP: data memory words [%u..%u]:\n", start_address, end_address - 1u);
    for (uint32_t address = start_address; address < end_address; ++address)
    {
        printf("  [%04u] %11d (0x%08X)\n",
               address,
               cpu.memory[address],
               (uint32_t)cpu.memory[address]);
    }
}
