#include <stdint.h>
#include "../lib/string.h"
#include "../memory/kmalloc.h"
#include "png.h"

/* c'est stocké comme ça
    +------------+----------+--------------+----------+
    | Length (4) | Type (4) | Data (Length)| CRC (4)  |
    +------------+----------+--------------+----------+
 */

// l'ordinateur stocke les bits en little endian, mais le format png le stocke en big endian
static uint32_t read_u32_be(uint8_t *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |
           ((uint32_t)p[3]);
}

int read_bits(BitReader *bit_reader, int n) {
    // DEFLATE lit les bits LSB -> MSB dans chaque octet
    if (n <= 0 || n > 32) return -1; // au cas ou

    while (bit_reader->bit_count < n) {
        if (bit_reader->pos >= bit_reader->size) return -1;
        bit_reader->bits |= ((uint32_t)bit_reader->data[bit_reader->pos]) << bit_reader->bit_count; // on convertit car bits fait 32 bits
        /*
          Exemple : bit_reader->bits = 00000000000000000000000000000101;
                    bit_reader->bit_count = 3;                      [  ] bits valides

                    on charge le prochian octet ; 11010110
                    (uint32_t) : 00000000000000000000000011010110 (conversion 32 bits)
                    << 3       : 00000000000000000000011010110000
                    |=         : 00000000000000000000011010110101
         */
        bit_reader->pos++;
        bit_reader->bit_count += 8;
    }

    // On masque les n bits lsb pour extraire uniquement les bits qu'on veut
    /*
        Exemple avec n = 3 :

        bits = 11010110
        mask = 00000111

        bits & mask :

        bit  : 11010110
        mask : 00000111

        result : 00000110
    */

    // On extrait les bits demmandé puis on décale le buffer pour retiré les bits deja lu et avancé le curseur de lecture
    uint32_t value = bit_reader->bits & ((1u << n) - 1);
    bit_reader->bits >>= n;
    bit_reader->bit_count -= n;

    return value;
}


PngContext png_decode(uint8_t *data, uint32_t size) {
    PngContext context = {0};

    if (size < 8) return context;
    if (memcmp(data, PNG_MAGIC_NUMBER, 8) != 0) return context;

    uint8_t *ptr = data + 8; // on saute le magic number
    uint8_t *end = data + size;

    while (ptr < end) {
        uint32_t length = read_u32_be(ptr);
        ptr += 4;

        char type[4];
        memcpy(type, ptr, 4);
        ptr += 4;

        uint8_t *chunk_data = ptr;

        if (memcmp(type, "IHDR", 4) == 0) {
            IHDR ihdr;

            ihdr.width       = read_u32_be(chunk_data);
            ihdr.height      = read_u32_be(chunk_data + 4);
            ihdr.bit_depth   = chunk_data[8];
            ihdr.color_type  = chunk_data[9];
            ihdr.compression = chunk_data[10];
            ihdr.filter      = chunk_data[11];
            ihdr.interlace   = chunk_data[12];

            context.width      = ihdr.width;
            context.height     = ihdr.height;
            context.bit_depth  = ihdr.bit_depth;
            context.color_type = ihdr.color_type;
            context.interlace  = ihdr.interlace;
        }

        else if (memcmp(type, "IDAT", 4) == 0) {

            uint8_t *buffer = kmalloc(context.idat_size + length);

            if (!buffer) return context;

            if (context.idat_buffer != NULL) {
                memcpy(buffer, context.idat_buffer, context.idat_size);
                kfree(context.idat_buffer);
            }

            memcpy(buffer + context.idat_size, chunk_data, length);

            context.idat_buffer = buffer;
            context.idat_size += length;
        }

        else if (memcmp(type, "IEND", 4) == 0) {
            break;
        }

        ptr += length;
        ptr += 4;
    }

    uint32_t bytes_per_pixel;
    switch (context.color_type) {
        case PNG_COLOR_RGB:    bytes_per_pixel = 3; break;
        case PNG_COLOR_RGBA:   bytes_per_pixel = 4; break;
        case PNG_COLOR_GRAY:   bytes_per_pixel = 1; break;
        case PNG_COLOR_GRAYA:  bytes_per_pixel = 2; break;
        default:               bytes_per_pixel = 3; break;
    }

    uint32_t pixels_size = (context.width * bytes_per_pixel + 1) * context.height;
    context.pixels = kmalloc(pixels_size);
    if (!context.pixels) return context;


    if (context.idat_size >= 6) { // 2 + DEFLATE + 4 ;  4 + 2 = 6
        // header zlib
        uint8_t *compressed_data = context.idat_buffer + 2; // Pas besoin de décoder CMF et FLG pour la v1
        uint32_t compressed_size = context.idat_size - 6;

        BitReader bit_reader = {
            .data = compressed_data,
            .size = compressed_size,
            .pos  = 0,
            .bits = 0,
            .bit_count = 0
        };

         // Parsing de l'header DFLATE
        uint32_t output_pos = 0;

        int BFINAL = read_bits(&bit_reader, 1);
        int BTYPE  = read_bits(&bit_reader, 2);

        do {
            switch (BTYPE) {
                case 0:
                    bit_reader.bits = 0;
                    bit_reader.bit_count = 0;

                    // masque les bit restant et commence au prochain octet
                    // LEN
                    uint16_t len = bit_reader.data[bit_reader.pos] | (bit_reader.data[bit_reader.pos + 1] << 8);
                    bit_reader.pos += 2;

                    // NLEN
                    uint16_t nlen = bit_reader.data[bit_reader.pos] | (bit_reader.data[bit_reader.pos + 1] << 8);
                    bit_reader.pos += 2;

                    if ((uint16_t)~len != nlen) return context; // Vérification au cas ou

                    memcpy(context.pixels + output_pos, bit_reader.data + bit_reader.pos, len);
                    bit_reader.pos += len;
                    output_pos += len;

                    break; // bloc non compresser

                case 1:
                    // Valeures RFC 1951
                    uint8_t lengths[288];

                    for (int i = 0;   i <= 143; i++) lengths[i] = 8;
                    for (int i = 144; i <= 255; i++) lengths[i] = 9;
                    for (int i = 256; i <= 279; i++) lengths[i] = 7;
                    for (int i = 280; i <= 287; i++) lengths[i] = 8;

                    uint16_t bl_count[16] = {0};
                    // compte la taille de chaques symbole récurssivement
                    for (int i = 0; i < 288; i++) bl_count[lengths[i]]++;

                    uint16_t next_code[16] = {0};
                    uint16_t code = 0;
                    for (int bits = 1; bits < 16; bits++) {
                        code = (code + bl_count[bits - 1]) << 1;
                        next_code[bits] = code;
                    }

                    HuffmanTable table = {0};
                    for (int i = 0; i < 288; i++) {
                        if (lengths[i] != 0) {
                            table.codes[i]   = next_code[lengths[i]]++;
                            table.lengths[i] = lengths[i];
                        }
                    }

                    break; // Huffman fixe
                case 2: break; // Hufmann dynamique
                case 3: return context; // invalide
            }
        } while (!BFINAL);
    }

    return context;
}
