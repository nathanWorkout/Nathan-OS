#include <stdint.h>
#include "ring_buffer.h"
#include "../tty/tty.h"
#include "string.h"
#include "rgba.h"
#include "com1.h"


#define VERSION "0.1"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__
#define TTY_COLS 160
#define COL_PROMPT   rgba(255, 176,   0, 255)
#define COL_INPUT    rgba(255, 204, 100, 255)
#define COL_OUTPUT   rgba(255, 140,   0, 255)
#define COL_SAY      rgba(255, 255, 180, 255)
#define COL_ERROR    rgba(200,  80,   0, 255)

extern volatile uint64_t pit_ticks;
static ring_buffer_t rb;

void input_push(char c) {
    if ((rb.head + 1) % 256 == rb.tail)
        return;
    rb.buffer[rb.head] = c;
    rb.head = (rb.head + 1) % 256;
}

char input_pop() {
    if (rb.tail == rb.head)
        return 0;
    char c = rb.buffer[rb.tail];
    rb.tail = (rb.tail + 1) % 256;
    return c;
}

int input_has_data() {
    return rb.tail != rb.head;
}

// Temporaire (en attendant que je fasse le scheduler)
// Bloque jusqu'à ce qu'un touche soit dans le ring_buffer puis return le char
char get_key() {
    uint64_t last = pit_ticks;
    int visible = 1;
    tty_draw_cursor(1);
    while (!input_has_data()) {
        if (pit_ticks - last >= 500) {
            visible ^= 1;
            tty_draw_cursor(visible);
            last = pit_ticks;
        }
        __asm__ volatile("hlt");
    }
    tty_draw_cursor(0);
    return input_pop();
}

void readline(char *buffer, int max) {
    int i = 0;
    while (i < max - 1) {
        char c = get_key();
        if (c == '\n') {
            putchar('\n');
            break;
        } else if (c == '\b') {
            if (i > 0) {
                i--;
                buffer[i] = 0;
                putchar('\b');
            }
        } else {
            putchar(c);
            buffer[i] = c;
            i++;
        }
    }
    buffer[i] = '\0';
}

// ==================== Personnalisation =======================

int strlen(const char *s);

void print_centered(const char *str) {
    int pad = (TTY_COLS - strlen(str)) / 2;
    for (int i = 0; i < pad; i++) putchar(' ');
    puts(str);
}

void print_motd() {
    putchar('\n');
    putchar('\n');
    putchar('\n');
    tty_set_color(rgba(255, 176, 0, 255));
    print_centered("888     888                  888 888          .d88888b.   .d8888b.");
    print_centered("888     888                  888 888         d88P\"Y88b d88P  Y88b");
    print_centered("Y88b   d88P 8888b.  888  888 888 888888      888     888  \"Y888b.");
    print_centered("Y88b d88P     \"88b 888  888 888 888         888     888     \"Y88b.");
    print_centered("  Y88o88P  .d888888 888  888 888 888  888888 888     888       \"888");
    print_centered("   Y888P   888  888 Y88b 888 888 Y88b.       Y88b. .d88P Y88b  d88P");
    print_centered("    Y8P    \"Y888888  \"Y88888 888  \"Y888       \"Y88888P\"   \"Y8888P\"");
    putchar('\n');
    tty_set_color(rgba(255, 140, 0, 255));
    print_centered("Version " VERSION " - " BUILD_DATE " - " BUILD_TIME);
    putchar('\n');
    tty_set_color(rgba(200, 80, 0, 255));
   // print_centered("================================================================================");
    print_centered("--------------------------------------------------------------------------------");
    putchar('\n');
    tty_set_color(rgba(255, 204, 100, 255));
    print_centered("Type 'help' for available commands.");
    putchar('\n');
}

void shell_run() {
    char buf[256];

    print_motd();
    while (1) {
        
        tty_set_color(COL_PROMPT);
        printk("VAULT OS > ");
        tty_set_color(COL_INPUT);
        readline(buf, 256);

        if (strcmp(buf, "help") == 0) {
            tty_set_color(COL_OUTPUT);
            puts("clear say reboot");
            tty_set_color(COL_INPUT);
        }
        else if (strcmp(buf, "clear") == 0)
            tty_clear();
        else if (strcmp(buf, "reboot") == 0)
            tty_reboot();
        else if (strncmp(buf, "say ", 4) == 0) {
            tty_set_color(COL_SAY);
            puts(buf + 4);
            tty_set_color(COL_INPUT);
        }
        else if (strcmp(buf, "") != 0) {
            tty_set_color(COL_ERROR);
            printk("unknown command : %s\n", buf);
            tty_set_color(COL_INPUT);
        }
    }
}
