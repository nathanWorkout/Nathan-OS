#include <stdint.h>
#include "ring_buffer.h"
#include "../tty/tty.h"
#include "string.h"
#include "com1.h"

static ring_buffer_t rb;

void input_push(char c) {
  if((rb.head + 1) % 256 == rb.tail) { // SI head = tail on ignore
    return;
  } else {
    rb.buffer[rb.head] = c;
    rb.head = (rb.head + 1) % 256;    // Réinitialise pour attendre le prochain c
  }
}

char input_pop() {
  if(rb.tail == rb.head) {
    return 0;
  } else {
    char c = rb.buffer[rb.tail];
    rb.tail = (rb.tail + 1) % 256;
    return c;
  }
}

int input_has_data() {
  if(rb.tail == rb.head) {
    return 0;
  } else {
    return 1;
  }
}

// Temporaire (en attendant que je fasse le scheduler)
// Bloque jusqu'à ce qu'un touche soit dans le ring_buffer puis return le char
char get_key() {
    while(!input_has_data()) {
        __asm__ volatile("hlt");
    }
    return input_pop();
}

void readline(char *buffer, int max) {
  int i = 0;

  while(i < max - 1) {
    char c = get_key();
    serial_putchar(c);

    if(c == '\n') {
      putchar('\n');
      break;
    } else if(c == '\b') {
        if(i > 0) {
          i--;
          buffer[i] = 0;
          putchar('\b');
        }
    }
    else {
      putchar(c);
      buffer[i] = c;
      i++;
    }
  }

  buffer[i] = '\0';
}

void shell_run() {
    char buf[256];
    while(1) {
        update_cursor(0, 0);
        printk("Nathan OS > ");
        readline(buf, 256);
        if(strcmp(buf, "help") == 0) {
          tty_set_color(0x1e);
          puts("clear say reboot");
          tty_set_color(0x1f); 
        }

        if(strcmp(buf, "clear") == 0) {
            tty_clear();
        }

        if(strcmp(buf, "reboot") == 0) {
          tty_reboot();
        }

        if(strncmp(buf, "say ", 4) == 0) {
          tty_set_color(0x1e);
          puts(buf + 4);
          tty_set_color(0x1f);
        }
    }
}



