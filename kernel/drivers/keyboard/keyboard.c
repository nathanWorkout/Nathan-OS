/**
 * @file keyboard.c
 * @brief Pilote clavier PS/2 avec disposition AZERTY
 *
 * Gère les scancodes du clavier PS/2 via l'IRQ1.
 * Supporte la disposition AZERTY française avec gestion du Shift.
 */

#include <stdint.h>
#include <stdbool.h>
#include "com1.h"
#include "io.h"
#include "ring_buffer.h"
#include "pic8089.h"

/** @brief État de la touche Shift (gauche ou droite) */
static bool shift;

/**
 * @brief Table de conversion scancode -> caractère AZERTY (sans Shift)
 *
 * Indexée par le scancode PS/2 set 1. Les entrées non définies valent 0.
 */
static char scancode_table[128] = {
    [0x01] = 27,   // ESC
    [0x02] = '&',
    [0x03] = 'é',
    [0x04] = '"',
    [0x05] = '\'',
    [0x06] = '(',
    [0x07] = '-',
    [0x08] = 'è',
    [0x09] = '_',
    [0x0A] = 'ç',
    [0x0B] = 'à',
    [0x0C] = ')',
    [0x0D] = '=',
    [0x0E] = '\b',
    [0x0F] = '\t',

    [0x10] = 'a',
    [0x11] = 'z',
    [0x12] = 'e',
    [0x13] = 'r',
    [0x14] = 't',
    [0x15] = 'y',
    [0x16] = 'u',
    [0x17] = 'i',
    [0x18] = 'o',
    [0x19] = 'p',
    [0x1A] = '^',
    [0x1B] = '$',
    [0x1C] = '\n',

    [0x1E] = 'q',
    [0x1F] = 's',
    [0x20] = 'd',
    [0x21] = 'f',
    [0x22] = 'g',
    [0x23] = 'h',
    [0x24] = 'j',
    [0x25] = 'k',
    [0x26] = 'l',
    [0x27] = 'm',
    [0x28] = 'ù',
    [0x29] = '²',

    [0x2B] = '*',
    [0x2C] = 'w',
    [0x2D] = 'x',
    [0x2E] = 'c',
    [0x2F] = 'v',
    [0x30] = 'b',
    [0x31] = 'n',
    [0x32] = ',',
    [0x33] = ';',
    [0x34] = ':',
    [0x35] = '!',
    [0x39] = ' ',

    [0x37] = '*',
    [0x4A] = '-',
    [0x4E] = '+',
    [0x53] = '.',
};

/**
 * @brief Table de conversion scancode -> caractère AZERTY (avec Shift)
 */
static char scancode_table_shift[128] = {
    [0x01] = 27,   // ESC
    [0x02] = '1',
    [0x03] = '2',
    [0x04] = '3',
    [0x05] = '4',
    [0x06] = '5',
    [0x07] = '6',
    [0x08] = '7',
    [0x09] = '8',
    [0x0A] = '9',
    [0x0B] = '0',
    [0x0C] = '°',
    [0x0D] = '+',
    [0x0E] = '\b',
    [0x0F] = '\t',

    [0x10] = 'A',
    [0x11] = 'Z',
    [0x12] = 'E',
    [0x13] = 'R',
    [0x14] = 'T',
    [0x15] = 'Y',
    [0x16] = 'U',
    [0x17] = 'I',
    [0x18] = 'O',
    [0x19] = 'P',
    [0x1A] = '¨',
    [0x1B] = '£',
    [0x1C] = '\n',

    [0x1E] = 'Q',
    [0x1F] = 'S',
    [0x20] = 'D',
    [0x21] = 'F',
    [0x22] = 'G',
    [0x23] = 'H',
    [0x24] = 'J',
    [0x25] = 'K',
    [0x26] = 'L',
    [0x27] = 'M',
    [0x28] = '%',
    [0x29] = '~',

    [0x2B] = 'µ',
    [0x2C] = 'W',
    [0x2D] = 'X',
    [0x2E] = 'C',
    [0x2F] = 'V',
    [0x30] = 'B',
    [0x31] = 'N',
    [0x32] = '?',
    [0x33] = '.',
    [0x34] = '/',
    [0x35] = '§',
    [0x39] = ' ',

    [0x37] = '*',
    [0x4A] = '-',
    [0x4E] = '+',
    [0x53] = '.',
};

/**
 * @brief Handler de l'IRQ1 — clavier PS/2
 *
 * Appelé à chaque frappe ou relâchement de touche.
 * Lit le scancode depuis le port 0x60, gère l'état Shift,
 * et pousse le caractère ASCII dans le ring buffer d'entrée.
 *
 * @param regs Registres CPU sauvegardés par l'ISR (non utilisés ici)
 *
 * @note Un scancode avec le bit 7 à 1 indique un relâchement (key up).
 * @warning Le else if sur 0xAA/0xB6 dans le bloc key-down est mort
 *          (ces scancodes sont déjà capturés par le test bit 7).
 */
void irq1_handler(uint64_t *regs) {
    (void)regs;
    uint8_t scancode = inb(0x60); /* Port données PS/2 */

    if (scancode & 0x80) { /* Key up : bit 7 à 1 */
        if (scancode == 0xAA || scancode == 0xB6) /* Shift G / Shift D relâché */
            shift = 0;
    } else { /* Key down */
        if (scancode == 0x2A || scancode == 0x36) /* Shift G / Shift D pressé */
            shift = 1;
        else if (scancode == 0xAA || scancode == 0xB6) /* Dead code */
            shift = 0;
        else {
            char c = shift ? scancode_table_shift[scancode] : scancode_table[scancode];
            if (c) input_push(c); /* Pousse dans le ring buffer si touche connue */
        }
    }

    pic_send_eoi(1); /* EOI obligatoire pour autoriser le PIC à envoyer une nouvelle interruption*/
}