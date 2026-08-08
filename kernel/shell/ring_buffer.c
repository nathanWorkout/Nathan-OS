#include <stdint.h>
#include "ring_buffer.h"
#include "../tty/tty.h"
#include "string.h"
#include "rgba.h"
#include "com1.h"
#include "kernel_panic.h"
#include "framebuffer.h"
#include "2d_renderer.h"
#include "VaultFs/vaultfs.h"
#include "memory/paging.h"
#include "memory/pmm.h"
#include "../Graphic/gui/default_profile.h"
#include <stddef.h>


#define VERSION "0.1"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__
#define TTY_COLS 160
#define COL_ASCII    rgba( 10, 132, 255, 255)
#define COL_PROMPT   rgba( 10, 132, 255, 255)
#define COL_PROMPT2  rgba( 99,  99, 102, 255)
#define COL_INPUT    rgba(229, 229, 234, 255)
#define COL_OUTPUT   rgba(142, 142, 147, 255)
#define COL_SAY      rgba(255, 214,  10, 255)
#define COL_ERROR    rgba(255,  69,  58, 255)

static VaultFs vaultfs;
static uint64_t current_profile_id = 1;
static char current_path[256] = "/";

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
    tty_set_color(COL_ASCII);
    print_centered("888     888                  888 888          .d88888b.   .d8888b.");
    print_centered("888     888                  888 888         d88P\"Y88b d88P  Y88b");
    print_centered("Y88b   d88P 8888b.  888  888 888 888888      888     888  \"Y888b.");
    print_centered("Y88b d88P     \"88b 888  888 888 888         888     888     \"Y88b.");
    print_centered("  Y88o88P  .d888888 888  888 888 888  888888 888     888       \"888");
    print_centered("   Y888P   888  888 Y88b 888 888 Y88b.       Y88b. .d88P Y88b  d88P");
    print_centered("    Y8P    \"Y888888  \"Y88888 888  \"Y888       \"Y88888P\"   \"Y8888P\"");
    putchar('\n');
    tty_set_color(COL_OUTPUT);
    print_centered("Version " VERSION " - " BUILD_DATE " - " BUILD_TIME);
    putchar('\n');
    tty_set_color(COL_PROMPT2);
    print_centered("--------------------------------------------------------------------------------");
    putchar('\n');
    tty_set_color(COL_INPUT);
    print_centered("Type 'help' for available commands.");
    putchar('\n');
}

void shell_run(Canvas *cv) {
    vaultfs_init(&vaultfs);
    vaultfs_create_profile(&vaultfs, 1, "default");

    char buf[256];

    print_motd();
    while (1) {

        tty_set_color(COL_PROMPT);
        printk("vault-os");
        tty_set_color(COL_PROMPT2);
        char cwd_display[256];
        vaultfs_get_cwd_path(&vaultfs, current_profile_id, cwd_display, 256);
        printk(" %s ", cwd_display);
        tty_set_color(COL_PROMPT);
        printk("%% ");
        tty_set_color(COL_INPUT);
        readline(buf, 256);

        if (strcmp(buf, "help") == 0) {
            tty_set_color(COL_OUTPUT);
            puts("clear | say | reboot | kernelpanic | ls | cat | write | rm | mkdir | publish | profile | list profile | create profile | switch profile");
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
        else if (strcmp(buf, "kernelpanic") == 0) kernel_panic_init(cv, NULL);

        // test gui
        else if (strcmp(buf, "testgui") == 0) {
            default_profile_init(*cv);
        }

        // VaultFs
        else if(strcmp(buf, "ls") == 0) {
            tty_set_color(COL_OUTPUT);

            VaultProfile *p = NULL;
            for(uint8_t i = 0; i < vaultfs.profile_count; i++) {
                if(vaultfs.profiles[i].profile_id == current_profile_id) {
                    p = &vaultfs.profiles[i];
                    break;
                }
            }

            if(p == NULL) {
                tty_set_color(COL_ERROR);
                puts("no active profile");
            } else {
                uint64_t cwd = p->cwd_inode;

                VaultNode *n = p->layer2.head;
                while(n) {
                    if(n->parent_inode == cwd && n->type != VAULT_DELETED)
                        puts(n->name);
                    n = n->next;
                }

                n = vaultfs.layer1.head;
                while(n) {
                    if(n->parent_inode == cwd) {
                        int masked = 0;
                        VaultNode *m = p->layer2.head;
                        while(m) {
                            if(strcmp(m->name, n->name) == 0) { masked = 1; break; }
                            m = m->next;
                        }
                        if(!masked) puts(n->name);
                    }
                    n = n->next;
                }

                n = vaultfs.layer0.head;
                while(n) {
                    if(n->parent_inode == cwd) {
                        int masked = 0;
                        VaultNode *m = p->layer2.head;
                        while(m) {
                            if(strcmp(m->name, n->name) == 0) { 
                                masked = 1; 
                                break;
                            }
                            m = m->next;
                        }
                        if(!masked) {
                            m = vaultfs.layer1.head;
                            while(m) {
                                if(strcmp(m->name, n->name) == 0) {
                                    masked = 1; 
                                    break; 
                                }
                                m = m->next;
                            }
                        }
                        if(!masked) puts(n->name);
                    }
                    n = n->next;
                }
            }
            tty_set_color(COL_INPUT);
        }

        else if(strncmp(buf, "write ", 6) == 0) {
            char *args = buf + 6;
            char *space = args;
            while(*space && *space != ' ') space++;
            if(*space == '\0') {
                tty_set_color(COL_ERROR);
                puts("usage: write <name> <content>");
            } else {
                *space = '\0';
                char *name = args;
                char *content = space + 1;
                int ret = vaultfs_write(&vaultfs, current_profile_id, name, (uint8_t *)content, strlen(content) + 1);
                if (ret == 0) {
                    tty_set_color(COL_OUTPUT);
                } else {
                    tty_set_color(COL_ERROR);
                    puts("error");
                }
            }
            tty_set_color(COL_INPUT);
        }

        else if(strncmp(buf, "cat ", 4) == 0) {
            char *path = buf + 4;
            uint64_t size = 0;

            uint8_t *data = vaultfs_read(&vaultfs, current_profile_id, path, &size);

            if(data == NULL) {
                tty_set_color(COL_ERROR);
                puts("file not found");
            } else {
                tty_set_color(COL_OUTPUT);
                for(uint64_t i = 0; i < size; i++) {
                    if(data[i] != '\0') putchar(data[i]);
                }
                putchar('\n');
            }
            tty_set_color(COL_INPUT);
        }

        else if(strncmp(buf, "cd ", 3) == 0) {
            char *path = buf + 3;
            int ret = vaultfs_cd(&vaultfs, current_profile_id, path);
            if(ret != 0) {
                tty_set_color(COL_ERROR);
                puts("dossier introuvable");
            } else {
                memcpy(current_path, path, 255);
                current_path[255] = '\0';
            }
            tty_set_color(COL_INPUT);
        }

        else if(strncmp(buf, "rm ", 3) == 0) {
            char *path = buf + 3;
            int ret = vaultfs_delete(&vaultfs, current_profile_id, path);
            if(ret == 0) {
                tty_set_color(COL_OUTPUT);
            } else {
                tty_set_color(COL_ERROR);
                puts("error");
            }
            tty_set_color(COL_INPUT);
        }

        else if(strncmp(buf, "mkdir ", 6) == 0) {
            char *path = buf + 6;
            int ret = vaultfs_mkdir(&vaultfs, current_profile_id, path);
            if(ret == 0) {
                tty_set_color(COL_OUTPUT);
            } else {
                tty_set_color(COL_ERROR);
                puts("error or existing folder");
            }
            tty_set_color(COL_INPUT);
        }

        else if(strncmp(buf, "publish ", 8) == 0) {
            char *path = buf + 8;
            int ret = vaultfs_publish(&vaultfs, current_profile_id, path);
            if(ret == 0) {
                tty_set_color(COL_OUTPUT);
            } else {
                tty_set_color(COL_ERROR);
                puts("erreur");
            }
            tty_set_color(COL_INPUT);
        }

        else if(strcmp(buf, "list profile") == 0) {
            tty_set_color(COL_OUTPUT);
            for(uint8_t i = 0; i < vaultfs.profile_count; i++) {
                 if(vaultfs.profiles[i].profile_id == current_profile_id)
                    printk("> %s\n", vaultfs.profiles[i].name);
                 else printk("  %s\n", vaultfs.profiles[i].name);
            }
            tty_set_color(COL_INPUT);
        }

        else if (strncmp(buf, "create profile ", 15) == 0) {
            char *name = buf + 15;
            static uint64_t next_id = 2;
            int ret = vaultfs_create_profile(&vaultfs, next_id++, name);
            if (ret == 0) {
                tty_set_color(COL_OUTPUT);

            } else {
                tty_set_color(COL_ERROR);
                puts("erreur ou limite atteinte");
            }
            tty_set_color(COL_INPUT);
        }

        else if(strncmp(buf, "switch profile ", 15) == 0) {
            char *name = buf + 15;
            VaultProfile *p = vaultfs_find_profile_by_name(&vaultfs, name);
            if(p == NULL) {
                tty_set_color(COL_ERROR);
                puts("profil introuvable");
            } else {
                current_profile_id = p->profile_id;
                tty_set_color(COL_OUTPUT);
                printk("profil actif : %s\n", p->name);
            }
            tty_set_color(COL_INPUT);
        }

        else if (strcmp(buf, "") != 0) {
            tty_set_color(COL_ERROR);
            printk("unknown command : %s\n", buf);
            tty_set_color(COL_INPUT);
        }

    }
}
