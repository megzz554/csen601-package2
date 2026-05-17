#ifndef PIPELINE_SIM_H
#define PIPELINE_SIM_H

#include <stdbool.h>
#include <stdint.h>
#include "cpu.h"
#include "execute.h"

/*
 * Pipeline engine.
 *
 * This module owns cycle scheduling, pipeline movement, data-hazard handling,
 * forwarding/stalling, control-hazard flushing, and IF/MEM memory-port timing.
 */

typedef struct
{
    bool dump_nonzero_memory_only;
    int max_cycles;
} PipelineSimOptions;

void pipeline_sim_run(CPU *target, PipelineSimOptions options);
void pipeline_dump_final_state(CPU *target);

#endif // PIPELINE_SIM_H
