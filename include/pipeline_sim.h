#ifndef PIPELINE_SIM_H
#define PIPELINE_SIM_H

#include <stdbool.h>
#include <stdint.h>
#include "cpu.h"
#include "execute.h"

/*
 * Person 6 integration layer.
 *
 * This module intentionally wraps the current teammate modules instead of
 * replacing their APIs.  It owns only simulation orchestration, branch/jump PC
 * updates, flushing, and debug printing.
 */

typedef struct
{
    bool dump_nonzero_memory_only;
    int max_cycles;
} PipelineSimOptions;

void pipeline_sim_run(CPU *target, PipelineSimOptions options);
void pipeline_dump_final_state(CPU *target);

#endif // PIPELINE_SIM_H
