#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>

#define RING_BUFFER_SIZE 256

typedef struct {
    char buffer[RING_BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
} ring_buffer_t;

void input_push(char c);
char input_pop();
int input_has_data();
void shell_run();

#endif
