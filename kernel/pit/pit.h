#ifndef PIT_H
#define PIT_H
#include "io.h"
#include <stdint.h>

#define CANAL0           0x40

#define CANAL2           0x42

#define COMMAND_REGISTER 0x43

void pit_init(freq);

#endif
