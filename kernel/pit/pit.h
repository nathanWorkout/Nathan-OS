#ifndef pit_h
#define pit_h
#include "io.h"
#include <stdint.h>

#define CANAL0           0x40

#define CANAL12          0x42

#define COMMAND_REGISTER 0x43

void pit_init(uint32_t freq);

#endif
