#include "mouse.h"
#include "io.h"
#include "pic8089.h"
#include "framebuffer.h"
#include <stdint.h>

static MouseState state;
static uint8_t packet[3];
static uint8_t packet_idx = 0;

static void ps2_wait_write() {
    while (inb(PS2_STATUS) & 0x02);
}

static void ps2_wait_read() {
    while (!(inb(PS2_STATUS) & 0x01));
}

static void ps2_send_cmd(uint8_t cmd) {
    ps2_wait_write();
    outb(PS2_CMD, cmd);
}

static void ps2_send_data(uint8_t data) {
    ps2_wait_write();
    outb(PS2_DATA, data);
}

static uint8_t ps2_read() {
    ps2_wait_read();
    return inb(PS2_DATA);
}

void mouse_init(void) {
    Canvas cv = fb_get_canvas();

    state.x = cv.width / 2;
    state.y = cv.height / 2;
    state.x_min = 0;
    state.y_min = 0;
    state.x_max = (int32_t)cv.width - 1;
    state.y_max = (int32_t)cv.height - 1;
    state.left = state.right = state.middle = 0;
    packet_idx = 0;

    ps2_send_cmd(0xA8); // active le port

    ps2_send_cmd(0x20);
    uint8_t config = ps2_read();
    config |= 0x02; // active irq12
    config &= ~0x20; // active l'horloge souris
    ps2_send_cmd(0x60); // écrit le config byte
    ps2_send_data(config);

    ps2_send_cmd(0xD4); // prochain bit sur 0x60 -> souris pas au clavier
    ps2_send_data(0xF4); // active l'envoie de paquets(souris)
    ps2_read(); // ack de la souris

    pic_clear_mask(12);
}

void mouse_irq_handler() {
    uint8_t byte = inb(PS2_DATA);
    packet[packet_idx++] = byte;

    if (packet_idx < 3) return; // car la souris envoie des paquets de 3 bits

    packet_idx = 0;

    if (!(packet[0] & 0x08)) return; // paquet invalide

    state.left   = (packet[0] >> 0) & 1; // force le resultat a 0 ou 1
    state.right  = (packet[0] >> 1) & 1;
    state.middle = (packet[0] >> 2) & 1;

    // complément a 2
    int32_t dx = (int32_t)packet[1] - ((packet[0] & 0x10) ? 256 : 0); // si xs a 1 : -256 sinon 0
    int32_t dy = (int32_t)packet[2] - ((packet[0] & 0x20) ? 256 : 0);

    state.x += dx;
    state.y -= dy; // car l'écran compte y vers le haut(conventio de maths)

    if (state.x < state.x_min) state.x = state.x_min;
    if (state.x > state.x_max) state.x = state.x_max;
    if (state.y < state.y_min) state.y = state.y_min;
    if (state.y > state.y_max) state.y = state.y_max;
}

MouseState *mouse_get_state() {
    return &state;
}