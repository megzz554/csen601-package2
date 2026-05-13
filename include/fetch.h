#ifndef FETCH_H
#define FETCH_H

#include <stdint.h>
#include "instruction.h"

instruction_t fetch_instruction(void);
void advance_pc(void);

#endif // FETCH_H
