#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "opcodes.h"
#include "pipeline_sim.h"

#define IF_CYCLES 1
#define ID_CYCLES 2
#define EX_CYCLES 2
#define MEM_CYCLES 1
#define WB_CYCLES 1

typedef enum
{
    STAGE_IF = 0,
    STAGE_ID,
    STAGE_EX,
    STAGE_MEM,
    STAGE_WB
} StageKind;

typedef struct
{
    PipelineRegister reg;
    int cycles_left;
    bool completed;
} StageSlot;

typedef struct
{
    StageSlot if_stage;
    StageSlot id_stage;
    StageSlot ex_stage;
    StageSlot mem_stage;
    StageSlot wb_stage;

    int cycle;
    bool wait_for_taken_branch_mem;
} PipelineEngine;

static const char *opcode_name(int opcode)
{
    switch (opcode)
    {
    case OPCODE_ADD:
        return "ADD";
    case OPCODE_SUB:
        return "SUB";
    case OPCODE_MUL:
        return "MUL";
    case OPCODE_MOVI:
        return "MOVI";
    case OPCODE_JEQ:
        return "JEQ";
    case OPCODE_AND:
        return "AND";
    case OPCODE_ORI:
        return "ORI";
    case OPCODE_JMP:
        return "JMP";
    case OPCODE_LSL:
        return "LSL";
    case OPCODE_LSR:
        return "LSR";
    case OPCODE_MOVR:
        return "MOVR";
    case OPCODE_MOVM:
        return "MOVM";
    default:
        return "UNKNOWN";
    }
}

static int32_t sign_extend(uint32_t value, unsigned bit_count)
{
    uint32_t mask = (1u << bit_count) - 1u;
    uint32_t sign_bit = 1u << (bit_count - 1u);

    value &= mask;
    if ((value & sign_bit) != 0u)
    {
        value |= ~mask;
    }

    return (int32_t)value;
}

static control_signals_t generate_control_signals(opcode_t opcode)
{
    control_signals_t control;

    memset(&control, 0, sizeof(control));
    control.alu_op = opcode_alu_operation(opcode);

    switch (opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_AND:
        control.reg_write = true;
        break;
    case OPCODE_MOVI:
        control.reg_write = true;
        control.use_immediate = true;
        break;
    case OPCODE_JEQ:
        control.branch = true;
        control.use_immediate = true;
        break;
    case OPCODE_ORI:
        control.reg_write = true;
        control.use_immediate = true;
        break;
    case OPCODE_JMP:
        control.jump = true;
        break;
    case OPCODE_LSL:
    case OPCODE_LSR:
        control.reg_write = true;
        control.use_shamt = true;
        break;
    case OPCODE_MOVR:
        control.reg_write = true;
        control.mem_read = true;
        control.mem_to_reg = true;
        control.use_immediate = true;
        break;
    case OPCODE_MOVM:
        control.mem_write = true;
        control.use_immediate = true;
        break;
    default:
        break;
    }

    return control;
}

static bool slot_is_empty(const StageSlot *slot)
{
    return slot == NULL || !slot->reg.valid;
}

static void clear_slot(StageSlot *slot)
{
    if (slot == NULL)
    {
        return;
    }

    memset(slot, 0, sizeof(*slot));
}

static void set_slot(StageSlot *slot, PipelineRegister reg, int cycles)
{
    slot->reg = reg;
    slot->cycles_left = cycles;
    slot->completed = false;
}

static void decode_raw_instruction(PipelineRegister *reg)
{
    uint32_t raw = reg->rawInstruction;

    reg->opcode = (int)((raw >> OPCODE_SHIFT) & 0xFu);
    reg->format = opcode_instruction_format((opcode_t)reg->opcode);
    reg->control = generate_control_signals((opcode_t)reg->opcode);
    reg->r1 = 0;
    reg->r2 = 0;
    reg->r3 = 0;
    reg->immediate = 0;
    reg->shamt = 0;
    reg->address = 0;
    reg->operand1 = 0;
    reg->operand2 = 0;
    reg->aluResult = 0;
    reg->memoryData = 0;

    if (reg->format == INSTRUCTION_FORMAT_INVALID)
    {
        reg->valid = 0;
        return;
    }

    if (reg->format == INSTRUCTION_FORMAT_J)
    {
        reg->address = (int)(raw & ADDR_MASK);
        return;
    }

    if (reg->format == INSTRUCTION_FORMAT_R)
    {
        reg->r1 = (int)((raw >> R1_SHIFT) & REGISTER_MASK);
        reg->r2 = (int)((raw >> R2_SHIFT) & REGISTER_MASK);
        reg->r3 = (int)((raw >> R3_SHIFT) & REGISTER_MASK);
        reg->shamt = (int)(raw & SHAMT_MASK);
        return;
    }

    if (reg->format == INSTRUCTION_FORMAT_I)
    {
        reg->r1 = (int)((raw >> R1_SHIFT) & REGISTER_MASK);
        reg->r2 = (int)((raw >> R2_SHIFT) & REGISTER_MASK);
        reg->immediate = sign_extend(raw & IMM_MASK, 18u);

        if (reg->opcode == OPCODE_MOVI)
        {
            reg->r2 = 0;
        }

        return;
    }
}

static bool instruction_writes_register(const PipelineRegister *reg)
{
    if (reg == NULL || !reg->valid)
    {
        return false;
    }

    return opcode_writes_register((opcode_t)reg->opcode);
}

static int destination_register(const PipelineRegister *reg)
{
    return instruction_writes_register(reg) ? reg->r1 : -1;
}

/*
 * Lightweight forwarding for integration testing.
 * Person 5 owns complete hazard handling, but this prevents the Person 6 test
 * programs from being dominated by stale register reads while the team engine
 * is still under construction.
 */
static int32_t read_operand_with_forwarding(CPU *target,
                                            PipelineEngine *engine,
                                            int reg_index)
{
    int ex_dest = destination_register(&engine->ex_stage.reg);
    int wb_dest = destination_register(&engine->wb_stage.reg);
    int mem_dest = destination_register(&engine->mem_stage.reg);

    if (reg_index == 0)
    {
        return 0;
    }

    /*
     * EX is ticked before ID in this simulator.  When a producer completes EX
     * in the same cycle that JEQ completes ID, the register file still contains
     * the old value.  Forwarding this completed EX result fixes cases like:
     *   MOVI R2 7
     *   JEQ  R1 R2 2
     * where JEQ previously saw R2 as 0.
     */
    if (ex_dest == reg_index &&
        engine->ex_stage.completed &&
        engine->ex_stage.reg.opcode != OPCODE_MOVR)
    {
        return engine->ex_stage.reg.aluResult;
    }

    if (wb_dest == reg_index)
    {
        if (engine->wb_stage.reg.opcode == OPCODE_MOVR)
        {
            return engine->wb_stage.reg.memoryData;
        }
        return engine->wb_stage.reg.aluResult;
    }

    if (mem_dest == reg_index && engine->mem_stage.reg.opcode != OPCODE_MOVR)
    {
        return engine->mem_stage.reg.aluResult;
    }

    return readRegister(target, reg_index);
}

static void latch_decode_operands(CPU *target, PipelineEngine *engine, PipelineRegister *reg)
{
    decode_raw_instruction(reg);

    switch (reg->opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_AND:
        reg->operand1 = read_operand_with_forwarding(target, engine, reg->r2);
        reg->operand2 = read_operand_with_forwarding(target, engine, reg->r3);
        break;
    case OPCODE_JEQ:
        reg->operand1 = read_operand_with_forwarding(target, engine, reg->r1);
        reg->operand2 = read_operand_with_forwarding(target, engine, reg->r2);
        break;
    case OPCODE_ORI:
    case OPCODE_LSL:
    case OPCODE_LSR:
    case OPCODE_MOVR:
    case OPCODE_MOVM:
        reg->operand1 = read_operand_with_forwarding(target, engine, reg->r2);
        reg->operand2 = read_operand_with_forwarding(target, engine, reg->r1);
        break;
    case OPCODE_MOVI:
        reg->operand1 = 0;
        reg->operand2 = reg->immediate;
        break;
    default:
        break;
    }
}

static void print_instruction_summary(const PipelineRegister *reg)
{
    if (reg == NULL || !reg->valid)
    {
        printf("empty");
        return;
    }

    printf("%s pc=%u raw=0x%08X", opcode_name(reg->opcode), reg->pc, reg->rawInstruction);
}

static void print_instruction_detail(const PipelineRegister *reg)
{
    if (reg == NULL || !reg->valid)
    {
        printf("EMPTY\n");
        return;
    }

    switch (reg->opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_AND:
        printf("%s R%d R%d R%d\n", opcode_name(reg->opcode), reg->r1, reg->r2, reg->r3);
        break;
    case OPCODE_MOVI:
        printf("MOVI R%d %d\n", reg->r1, reg->immediate);
        break;
    case OPCODE_JEQ:
        printf("JEQ R%d R%d %d\n", reg->r1, reg->r2, reg->immediate);
        break;
    case OPCODE_ORI:
    case OPCODE_LSL:
    case OPCODE_LSR:
    case OPCODE_MOVR:
    case OPCODE_MOVM:
        if (reg->opcode == OPCODE_LSL || reg->opcode == OPCODE_LSR)
        {
            printf("%s R%d R%d %d\n", opcode_name(reg->opcode), reg->r1, reg->r2, reg->shamt);
        }
        else
        {
            printf("%s R%d R%d %d\n", opcode_name(reg->opcode), reg->r1, reg->r2, reg->immediate);
        }
        break;
    case OPCODE_JMP:
        printf("JMP %d\n", reg->address);
        break;
    default:
        printf("%s pc=%u raw=0x%08X\n", opcode_name(reg->opcode), reg->pc, reg->rawInstruction);
        break;
    }
}

static void printPipelineState(const PipelineEngine *engine)
{
    printf("  IF : ");
    print_instruction_summary(&engine->if_stage.reg);
    printf("\n  ID : ");
    print_instruction_summary(&engine->id_stage.reg);
    printf("\n  EX : ");
    print_instruction_summary(&engine->ex_stage.reg);
    printf("\n  MEM: ");
    print_instruction_summary(&engine->mem_stage.reg);
    printf("\n  WB : ");
    print_instruction_summary(&engine->wb_stage.reg);
    printf("\n");
}

static void printRegisterUpdate(int reg_index, int32_t old_value, int32_t new_value, const char *stage)
{
    printf("  R%d updated:\n", reg_index);
    printf("    old value = %d\n", old_value);
    printf("    new value = %d\n", new_value);
    printf("    stage = %s\n", stage);

    if (reg_index == 0)
    {
        printf("    note = R0 is protected and remains zero\n");
    }
}

static void printMemoryUpdate(int address, int32_t old_value, int32_t new_value)
{
    printf("  Memory[%d] updated:\n", address);
    printf("    old value = %d\n", old_value);
    printf("    new value = %d\n", new_value);
}

static void commitRegisterWrite(CPU *target, int reg_index, int32_t value, const char *stage)
{
    int32_t old_value = readRegister(target, reg_index);

    /*
     * Person 2 owns the register file and R0 protection.  Person 6 prints the
     * integration-level commit message before calling the teammate API.
     */
    writeRegister(target, reg_index, value);
    printRegisterUpdate(reg_index, old_value, readRegister(target, reg_index), stage);
}

static void commitMemoryWrite(CPU *target, int address, int32_t value)
{
    int32_t old_value = 0;

    if (address >= 0 && address < MEMORY_SIZE)
    {
        old_value = target->memory[address];
    }

    /*
     * Memory bounds and segment ownership remain inside Person 2's writeMemory.
     * This wrapper only adds the Person 6 old/new debug print.
     */
    writeMemory(target, address, value);

    if (address >= 0 && address < MEMORY_SIZE && target->memory[address] != old_value)
    {
        printMemoryUpdate(address, old_value, target->memory[address]);
    }
}

static void flushPipeline(PipelineEngine *engine)
{
    if (!slot_is_empty(&engine->if_stage))
    {
        decode_raw_instruction(&engine->if_stage.reg);
        printf("  [FLUSH] removing IF instruction:\n    ");
        print_instruction_detail(&engine->if_stage.reg);
    }
    if (!slot_is_empty(&engine->id_stage))
    {
        decode_raw_instruction(&engine->id_stage.reg);
        printf("  [FLUSH] removing ID instruction:\n    ");
        print_instruction_detail(&engine->id_stage.reg);
    }

    clear_slot(&engine->if_stage);
    clear_slot(&engine->id_stage);
}

static bool validatePipeline(const PipelineEngine *engine, bool fetched_this_cycle, bool mem_active_at_cycle_start)
{
    bool valid = true;

    /*
     * Validation is intentionally non-fatal.  It gives Person 6 debugging
     * visibility without taking ownership of Person 5's full hazard engine.
     */
    if (fetched_this_cycle && mem_active_at_cycle_start)
    {
        printf("  [VALIDATE] IF/MEM conflict detected in cycle %d\n", engine->cycle);
        valid = false;
    }

    if (fetched_this_cycle && (engine->cycle % 2) == 0)
    {
        printf("  [VALIDATE] fetch happened outside the every-2-cycles cadence\n");
        valid = false;
    }

    if (!slot_is_empty(&engine->id_stage) && engine->id_stage.cycles_left > ID_CYCLES)
    {
        printf("  [VALIDATE] ID cycle counter is invalid\n");
        valid = false;
    }

    if (!slot_is_empty(&engine->ex_stage) && engine->ex_stage.cycles_left > EX_CYCLES)
    {
        printf("  [VALIDATE] EX cycle counter is invalid\n");
        valid = false;
    }

    return valid;
}

static bool calculate_branch_target(CPU *target,
                                    PipelineRegister *reg,
                                    uint32_t *target_pc)
{
    bool taken = false;
    uint32_t old_pc = target->PC;

    if (reg->opcode == OPCODE_JEQ)
    {
        taken = reg->operand1 == reg->operand2;
        *target_pc = reg->pc + 1u + (uint32_t)reg->immediate;
        printf("  [BRANCH] JEQ compares %d and %d -> %s\n",
               reg->operand1,
               reg->operand2,
               taken ? "TAKEN" : "NOT TAKEN");
        if (taken)
        {
            printf("  [BRANCH] updating PC from %u to %u\n", old_pc, *target_pc);
        }
    }
    else if (reg->opcode == OPCODE_JMP)
    {
        uint32_t upper_pc = reg->pc & 0xF0000000u;
        *target_pc = upper_pc | ((uint32_t)reg->address & ADDR_MASK);
        taken = true;
        printf("  [BRANCH] JMP target=%u (upper PC bits preserved)\n", *target_pc);
        printf("  [BRANCH] updating PC from %u to %u\n", old_pc, *target_pc);
    }

    return taken;
}

static void complete_ex_stage(CPU *target, PipelineEngine *engine, PipelineRegister *reg)
{
    uint32_t target_pc = 0u;

    printf("  [EX] inputs: op1=%d op2=%d imm=%d shamt=%d\n",
           reg->operand1,
           reg->operand2,
           reg->immediate,
           reg->shamt);

    switch (reg->control.alu_op)
    {
    case ALU_OP_ADD:
        reg->aluResult = reg->operand1 + reg->operand2;
        break;
    case ALU_OP_SUB:
        reg->aluResult = reg->operand1 - reg->operand2;
        break;
    case ALU_OP_MUL:
        reg->aluResult = reg->operand1 * reg->operand2;
        break;
    case ALU_OP_AND:
        reg->aluResult = reg->operand1 & reg->operand2;
        break;
    case ALU_OP_OR:
        reg->aluResult = reg->operand1 | reg->immediate;
        break;
    case ALU_OP_SHIFT_LEFT:
        reg->aluResult = (int32_t)((uint32_t)reg->operand1 << reg->shamt);
        break;
    case ALU_OP_SHIFT_RIGHT:
        reg->aluResult = (int32_t)((uint32_t)reg->operand1 >> reg->shamt);
        break;
    case ALU_OP_PASS_B:
        reg->aluResult = reg->operand2;
        break;
    case ALU_OP_NONE:
        break;
    }

    if (reg->opcode == OPCODE_MOVR || reg->opcode == OPCODE_MOVM)
    {
        reg->address = reg->operand1 + reg->immediate;
        reg->memoryData = reg->operand2;
    }

    if (reg->control.branch || reg->control.jump)
    {
        if (calculate_branch_target(target, reg, &target_pc))
        {
            cpu_set_pc(target_pc);
            flushPipeline(engine);
            engine->wait_for_taken_branch_mem = true;
            printf("  [PC] updated to %u; fetch waits until branch reaches MEM\n", target_pc);
        }
    }

    printf("  [EX] output: aluResult=%d address=%d\n", reg->aluResult, reg->address);
}

static void complete_mem_stage(CPU *target, PipelineEngine *engine, PipelineRegister *reg)
{
    if (reg->control.mem_read)
    {
        reg->memoryData = readMemory(target, reg->address);
        printf("  [MEM] MOVR loaded %d from address %d\n", reg->memoryData, reg->address);
    }
    else if (reg->control.mem_write)
    {
        commitMemoryWrite(target, reg->address, reg->memoryData);
        printf("  [MEM] MOVM stored %d into address %d\n", reg->memoryData, reg->address);
    }
    else
    {
        printf("  [MEM] no data-memory access\n");
    }

    if (reg->opcode == OPCODE_JEQ || reg->opcode == OPCODE_JMP)
    {
        engine->wait_for_taken_branch_mem = false;
        printf("  [BRANCH] branch/jump reached MEM; target fetch may resume on an eligible IF cycle\n");
    }
}

static void complete_wb_stage(CPU *target, PipelineRegister *reg)
{
    if (!instruction_writes_register(reg) || !reg->control.reg_write)
    {
        printf("  [WB] no register write\n");
        return;
    }

    if (reg->control.mem_to_reg)
    {
        commitRegisterWrite(target, reg->r1, reg->memoryData, "WB");
        printf("  [WB] wrote load value %d to R%d\n", reg->memoryData, reg->r1);
    }
    else
    {
        commitRegisterWrite(target, reg->r1, reg->aluResult, "WB");
        printf("  [WB] wrote ALU value %d to R%d\n", reg->aluResult, reg->r1);
    }
}

static void tick_slot(CPU *target, PipelineEngine *engine, StageSlot *slot, StageKind kind)
{
    if (slot_is_empty(slot) || slot->completed)
    {
        return;
    }

    slot->cycles_left -= 1;
    if (slot->cycles_left > 0)
    {
        return;
    }

    slot->completed = true;

    switch (kind)
    {
    case STAGE_IF:
        printf("  [IF] output raw=0x%08X from PC=%u\n", slot->reg.rawInstruction, slot->reg.pc);
        break;
    case STAGE_ID:
        latch_decode_operands(target, engine, &slot->reg);
        printf("  [ID] decoded %s r1=%d r2=%d r3=%d imm=%d address=%d\n",
               opcode_name(slot->reg.opcode),
               slot->reg.r1,
               slot->reg.r2,
               slot->reg.r3,
               slot->reg.immediate,
               slot->reg.address);
        break;
    case STAGE_EX:
        complete_ex_stage(target, engine, &slot->reg);
        break;
    case STAGE_MEM:
        complete_mem_stage(target, engine, &slot->reg);
        break;
    case STAGE_WB:
        complete_wb_stage(target, &slot->reg);
        break;
    }
}

static bool can_fetch_this_cycle(const CPU *target, const PipelineEngine *engine)
{
    if (target->PC >= (uint32_t)target->instructionCount)
    {
        return false;
    }

    if ((engine->cycle % 2) == 0)
    {
        return false;
    }

    if (!slot_is_empty(&engine->if_stage))
    {
        return false;
    }

    if (!slot_is_empty(&engine->mem_stage))
    {
        return false;
    }

    if (engine->wait_for_taken_branch_mem)
    {
        return false;
    }

    return true;
}

static void fetch_into_if(CPU *target, PipelineEngine *engine)
{
    PipelineRegister reg;
    memset(&reg, 0, sizeof(reg));

    reg.valid = 1;
    reg.pc = target->PC;
    reg.rawInstruction = memory_fetch_instruction(target->PC);
    cpu_set_pc(target->PC + 1u);

    /*
     * Decode opcode early only for readable pipeline visualization.  The ID
     * stage still performs the real field extraction after its two cycles.
     */
    reg.opcode = (int)((reg.rawInstruction >> OPCODE_SHIFT) & 0xFu);

    set_slot(&engine->if_stage, reg, IF_CYCLES);
    printf("  [IF] fetched PC=%u raw=0x%08X; PC incremented to %u\n",
           reg.pc,
           reg.rawInstruction,
           target->PC);
}

static void transfer_completed_stages(PipelineEngine *engine)
{
    if (!slot_is_empty(&engine->wb_stage) && engine->wb_stage.completed)
    {
        clear_slot(&engine->wb_stage);
    }

    if (!slot_is_empty(&engine->mem_stage) &&
        engine->mem_stage.completed &&
        slot_is_empty(&engine->wb_stage))
    {
        set_slot(&engine->wb_stage, engine->mem_stage.reg, WB_CYCLES);
        clear_slot(&engine->mem_stage);
    }

    if (!slot_is_empty(&engine->ex_stage) &&
        engine->ex_stage.completed &&
        slot_is_empty(&engine->mem_stage))
    {
        set_slot(&engine->mem_stage, engine->ex_stage.reg, MEM_CYCLES);
        clear_slot(&engine->ex_stage);
    }

    if (!slot_is_empty(&engine->id_stage) &&
        engine->id_stage.completed &&
        slot_is_empty(&engine->ex_stage))
    {
        set_slot(&engine->ex_stage, engine->id_stage.reg, EX_CYCLES);
        clear_slot(&engine->id_stage);
    }

    if (!slot_is_empty(&engine->if_stage) &&
        engine->if_stage.completed &&
        slot_is_empty(&engine->id_stage))
    {
        set_slot(&engine->id_stage, engine->if_stage.reg, ID_CYCLES);
        clear_slot(&engine->if_stage);
    }
}

static bool pipeline_has_work(const CPU *target, const PipelineEngine *engine)
{
    return target->PC < (uint32_t)target->instructionCount ||
           !slot_is_empty(&engine->if_stage) ||
           !slot_is_empty(&engine->id_stage) ||
           !slot_is_empty(&engine->ex_stage) ||
           !slot_is_empty(&engine->mem_stage) ||
           !slot_is_empty(&engine->wb_stage);
}

static void print_cycle_header(const PipelineEngine *engine, const CPU *target)
{
    printf("\n================ Cycle %d ================\n", engine->cycle);
    printf("  PC=%u instructionCount=%d\n", target->PC, target->instructionCount);
    printPipelineState(engine);
}

void pipeline_sim_run(CPU *target, PipelineSimOptions options)
{
    PipelineEngine engine;
    int max_cycles = options.max_cycles > 0 ? options.max_cycles : 200;

    memset(&engine, 0, sizeof(engine));

    printf("\nStarting Person 6 integrated pipeline simulation\n");
    printf("Timing: IF=1, ID=2, EX=2, MEM=1, WB=1; fetch on odd cycles; IF/MEM conflict blocks fetch.\n");

    while (pipeline_has_work(target, &engine) && engine.cycle < max_cycles)
    {
        bool fetched_this_cycle = false;
        bool mem_active_at_cycle_start = false;

        engine.cycle += 1;
        mem_active_at_cycle_start = !slot_is_empty(&engine.mem_stage);
        print_cycle_header(&engine, target);

        if (can_fetch_this_cycle(target, &engine))
        {
            fetch_into_if(target, &engine);
            fetched_this_cycle = true;
        }
        else
        {
            printf("  [IF] no fetch this cycle");
            if ((engine.cycle % 2) == 0)
            {
                printf(" (fetch cadence)");
            }
            else if (!slot_is_empty(&engine.mem_stage))
            {
                printf(" (IF/MEM conflict)");
            }
            else if (engine.wait_for_taken_branch_mem)
            {
                printf(" (waiting for taken branch to enter MEM)");
            }
            else if (target->PC >= (uint32_t)target->instructionCount)
            {
                printf(" (PC reached loaded program end)");
            }
            printf("\n");
        }

        tick_slot(target, &engine, &engine.wb_stage, STAGE_WB);
        tick_slot(target, &engine, &engine.mem_stage, STAGE_MEM);
        tick_slot(target, &engine, &engine.ex_stage, STAGE_EX);
        tick_slot(target, &engine, &engine.id_stage, STAGE_ID);
        tick_slot(target, &engine, &engine.if_stage, STAGE_IF);

        transfer_completed_stages(&engine);
        validatePipeline(&engine, fetched_this_cycle, mem_active_at_cycle_start);
    }

    if (engine.cycle >= max_cycles && pipeline_has_work(target, &engine))
    {
        printf("\nSIMULATION WARNING: stopped at max_cycles=%d before pipeline drained.\n", max_cycles);
    }

    printf("\nPipeline simulation finished after %d cycles.\n", engine.cycle);
    (void)options.dump_nonzero_memory_only;
    pipeline_dump_final_state(target);
}

void pipeline_dump_final_state(CPU *target)
{
    bool printed_any_memory = false;

    printf("\n================ Final State ================\n");
    dumpRegisters(target);

    printf("\nDUMP: non-zero memory locations only:\n");
    for (int address = 0; address < MEMORY_SIZE; ++address)
    {
        if (target->memory[address] != 0)
        {
            printf("  [%04d] %11d (0x%08X)\n",
                   address,
                   target->memory[address],
                   (uint32_t)target->memory[address]);
            printed_any_memory = true;
        }
    }

    if (!printed_any_memory)
    {
        printf("  all memory locations are zero\n");
    }
}
