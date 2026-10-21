#include <stdint.h>
#include <stddef.h>
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "png.h"
#include "tty.h"
#include "com1.h"

/* c'est stocké comme ça
    +------------+----------+--------------+----------+
    | Length (4) | Type (4) | Data (Length)| CRC (4)  |
    +------------+----------+--------------+----------+
 */

typedef struct {
    uint16_t count[16];   // nb de codes par longueur
    uint16_t symbol[288]; // symboles tries par l'algo
} Huffman;

// tableaux rfc pour fixe et dynamique
static const int length_base[] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13,
    15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
    67, 83, 99, 115, 131, 163, 195, 227, 258
};

static const int length_extra[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
    4, 4, 4, 4, 5, 5, 5, 5, 0
};

static const int distance_base[] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25,
    33, 49, 65, 97, 129, 193, 257, 385, 513, 769,
    1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289,
    16385, 24577
};

static const int distance_extra[] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3,
    4, 4, 5, 5, 6, 6, 7, 7, 8, 8,
    9, 9, 10, 10, 11, 11, 12, 12,
    13, 13
};

// l'ordinateur stocke les bits en little endian, mais le format png le stocke en big endian
uint32_t read_u32_be(uint8_t *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |
           ((uint32_t)p[3]);
}

int read_bits(BitReader *bit_reader, int n) {
    // DEFLATE lit les bits LSB -> MSB dans chaque octet
    if (n < 0 || n > 32) return -1; // au cas ou
    if (n == 0) return 0;

    while (bit_reader->bit_count < n) {
        if (bit_reader->pos >= bit_reader->size) return -1;
        bit_reader->bits |= ((uint64_t)bit_reader->data[bit_reader->pos]) << bit_reader->bit_count;
        /*
          Exemple : bit_reader->bits = 00000000000000000000000000000101;
                    bit_reader->bit_count = 3;                      [  ] bits valides

                    on charge le prochian octet : 11010110
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
    uint32_t value = (uint32_t)(bit_reader->bits & ((1ull << n) - 1));
    bit_reader->bits >>= n;
    bit_reader->bit_count -= n;

    return value;
}

// algo de filtrage
int paeth(int a, int b, int c) {
    int p  = a + b - c;
    int pa = p - a; if (pa < 0) pa = -pa;
    int pb = p - b; if (pb < 0) pb = -pb;
    int pc = p - c; if (pc < 0) pc = -pc;

    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc)             return b;
    return c;
}

/*
  Construit la table a partir des longueurs de code de chaque symbole.

  Exemple :
  A = 2 bits
  B = 2 bits
  C = 3 bits
  D = 3 bits

  donc count[2] = 2 et count[3] = 2 car 2 codes de 2 bits et 2 codes de 3
  symbol = [A, B, C, D] (tries par longueur puis par valeur)
  Le decodeur retrouve le symbole avec juste count[] : plus besoin de calculer
  les codes ni de reverse_bits, on lit les bits dans l'ordre ou DEFLATE les stocke.
 */
static void huff_build(Huffman *h, const uint8_t *lengths, int n) {
    uint16_t offs[16];

    for (int i = 0; i < 16; i++) h->count[i] = 0;
    for (int i = 0; i < n; i++) h->count[lengths[i]]++;
    h->count[0] = 0;

    offs[1] = 0;
    for (int l = 1; l < 15; l++) offs[l + 1] = offs[l] + h->count[l];

    for (int i = 0; i < n; i++)
        if (lengths[i]) h->symbol[offs[lengths[i]]++] = i;
}

static int huff_decode(BitReader *br, const Huffman *h) {
    int code = 0, first = 0, index = 0;

    for (int len = 1; len <= 15; len++) {
        int b = read_bits(br, 1);
        if (b < 0) return -1;
        code |= b;

        int count = h->count[len];
        if (code - count < first)
            return h->symbol[index + (code - first)];

        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }
    return -1;
}

static int inflate_codes(BitReader *br, const Huffman *lit, const Huffman *dist,
                         uint8_t *out, uint32_t *pos, uint32_t cap) {
    for (;;) {
        int sym = huff_decode(br, lit);
        if (sym < 0) return -1;

        if (sym < 256) { // octet a ecrire
            if (*pos >= cap) return -1;
            out[(*pos)++] = (uint8_t)sym;
        }
        else if (sym == 256) { // fin du bloc
            return 0;
        }
        else { // back reference
            sym -= 257;
            if (sym >= 29) return -1;

            int extra = read_bits(br, length_extra[sym]);
            if (extra < 0) return -1;
            int length = length_base[sym] + extra;

            int dsym = huff_decode(br, dist);
            if (dsym < 0 || dsym >= 30) return -1;

            int dextra = read_bits(br, distance_extra[dsym]);
            if (dextra < 0) return -1;
            uint32_t distance = distance_base[dsym] + dextra;

            if (distance > *pos || *pos + length > cap) return -1;

            // ici on copie les blocs similaires (optimisation de la rfc indispensable pour la decompression)
            // memcpy seulement si source et dest ne se chevauchent pas
            // sinon ca peut arriver que la source et la dest pointe au meme endroit
            // ex : pour abc -> abab la boucle donne abcabc
            uint8_t *d = out + *pos;
            uint8_t *s = d - distance;
            if (distance >= (uint32_t)length) memcpy(d, s, length);
            else for (int j = 0; j < length; j++) d[j] = s[j];
            *pos += length;
        }
    }
}

static int inflate(BitReader *bit_reader, uint8_t *out, uint32_t cap) {
    uint32_t output_pos = 0;
    int bfinal;

    do {
        bfinal = read_bits(bit_reader, 1);
        int BTYPE = read_bits(bit_reader, 2); // type de compression
        if (bfinal < 0 || BTYPE < 0) return -1;

        switch (BTYPE) {
            case 0: {
                serial_println("cas 0");
                // masque les bit restant et commence au prochain octet
                bit_reader->bits = 0;
                bit_reader->bit_count = 0;

                if (bit_reader->pos + 4 > bit_reader->size) return -1;

                // LEN
                // exemple : on lis 2 octets en little endian 0x05 | (0x00 << 8) = 5
                uint16_t len = bit_reader->data[bit_reader->pos] | (bit_reader->data[bit_reader->pos + 1] << 8);
                bit_reader->pos += 2;

                // NLEN
                uint16_t nlen = bit_reader->data[bit_reader->pos] | (bit_reader->data[bit_reader->pos + 1] << 8);
                bit_reader->pos += 2;

                // est ce que nlen est bien le complément de len
                if ((uint16_t)~len != nlen) return -1;

                if (bit_reader->pos + len > bit_reader->size) return -1;
                if (output_pos + len > cap) return -1;

                // pas de decompression on copie len dans pixels
                memcpy(out + output_pos, bit_reader->data + bit_reader->pos, len);
                bit_reader->pos += len;
                output_pos += len;

                break; // bloc non compresser
            }

            case 1: {
                serial_println("cas 1");
                // Valeures RFC 1951
                uint8_t lengths[288];
                uint8_t dlengths[30];

                for (int i = 0;   i <= 143; i++) lengths[i] = 8;
                for (int i = 144; i <= 255; i++) lengths[i] = 9;
                for (int i = 256; i <= 279; i++) lengths[i] = 7;
                for (int i = 280; i <= 287; i++) lengths[i] = 8;
                for (int i = 0; i < 30; i++) dlengths[i] = 5;

                Huffman lit_table, dist_table;
                huff_build(&lit_table, lengths, 288);
                huff_build(&dist_table, dlengths, 30);

                if (inflate_codes(bit_reader, &lit_table, &dist_table, out, &output_pos, cap) < 0) return -1;

                break; // Huffman fixe
            }

            case 2: {
                // spec DEFLATE
                int HLIT  = read_bits(bit_reader, 5) + 257;
                int HDIST = read_bits(bit_reader, 5) + 1;
                int HCLEN = read_bits(bit_reader, 4) + 4;
                if (HLIT > 286 || HDIST > 30) return -1;

                int order[19] = {
                    16,17,18,0,8,
                    7,9,6,10,5,
                    11,4,12,3,
                    13,2,14,1,15
                };
                uint8_t code_lengths[19] = {0};

                for (int i = 0; i < HCLEN; i++) {
                    int v = read_bits(bit_reader, 3);
                    if (v < 0) return -1;
                    code_lengths[order[i]] = v;
                }

                // construction de la premiere table huffman (preparation plutot pour les 2 prochaines tables)
                Huffman cl_table;
                huff_build(&cl_table, code_lengths, 19);

                uint8_t all_lengths[286 + 30];
                int total = HLIT + HDIST;
                int index = 0;

                while (index < total) {
                    int sym = huff_decode(bit_reader, &cl_table);
                    if (sym < 0) return -1;

                    if (sym <= 15) {
                        all_lengths[index++] = sym;
                    } else {
                        int repeat;
                        uint8_t value = 0;

                        if (sym == 16) {
                            if (index == 0) return -1;
                            int r = read_bits(bit_reader, 2);
                            if (r < 0) return -1;
                            repeat = r + 3;
                            value = all_lengths[index - 1];
                        } else if (sym == 17) {
                            int r = read_bits(bit_reader, 3);
                            if (r < 0) return -1;
                            repeat = r + 3;
                        } else {
                            int r = read_bits(bit_reader, 7);
                            if (r < 0) return -1;
                            repeat = r + 11;
                        }

                        if (index + repeat > total) return -1;
                        for (int j = 0; j < repeat; j++) all_lengths[index++] = value;
                    }
                }

                // LIT TABLE : table qui decode les symboles 0-285 (octets litteraux + back reference)
                Huffman lit_table;
                huff_build(&lit_table, all_lengths, HLIT);

                // DIST TABLE : table qui decode les distances (pour lire les distances sinon on lit les litteraux)
                Huffman dist_table;
                huff_build(&dist_table, all_lengths + HLIT, HDIST);

                if (inflate_codes(bit_reader, &lit_table, &dist_table, out, &output_pos, cap) < 0) return -1;

                break; // Hufman dynamique
            }

            default: return -1; // invalide
        }
    } while (!bfinal);

    return 0;
}

PngContext png_decode(uint8_t *data, uint32_t size) {
    PngContext context = {0};
    serial_println("png_decode start");

    if (size < 8) return context; 
    if (memcmp(data, PNG_MAGIC_NUMBER, 8) != 0) { serial_println("magic number have problem"); return context; }
    serial_println("magic is good");

    uint8_t *start = data + 8; // on saute le magic number
    uint8_t *end = data + size;
    uint8_t *ptr;
    uint32_t total_idat = 0;
    ptr = start;

    while (ptr + 12 <= end) {
        uint32_t length = read_u32_be(ptr);
        uint8_t *type = ptr + 4;
        uint8_t *chunk_data = ptr + 8;

        if (length > (uint32_t)(end - chunk_data) || (uint32_t)(end - chunk_data) - length < 4) break;

        if (memcmp(type, "IHDR", 4) == 0) {
            if (length < 13) return context;
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
            total_idat += length;
        }
        else if (memcmp(type, "IEND", 4) == 0) {
            break;
        }

        ptr = chunk_data + length + 4; // data + CRC
    }

    if (total_idat == 0) return context;

    context.idat_buffer = kmalloc(total_idat);
    if (!context.idat_buffer) return context;

    // 2eme passe : on copie chaque IDAT a la suite
    ptr = start;
    while (ptr + 12 <= end) {
        uint32_t length = read_u32_be(ptr);
        uint8_t *type = ptr + 4;
        uint8_t *chunk_data = ptr + 8;

        if (length > (uint32_t)(end - chunk_data) || (uint32_t)(end - chunk_data) - length < 4) break;

        if (memcmp(type, "IDAT", 4) == 0) {
            memcpy(context.idat_buffer + context.idat_size, chunk_data, length);
            context.idat_size += length;
        }
        else if (memcmp(type, "IEND", 4) == 0) {
            break;
        }

        ptr = chunk_data + length + 4;
    }

    if (context.bit_depth != 8 || context.interlace != 0) {
        serial_println("png non supporte (bit_depth != 8 ou interlace)");
        kfree(context.idat_buffer);
        context.idat_buffer = NULL;
        return context;
    }

    uint32_t bytes_per_pixel;
    switch (context.color_type) {
        case PNG_COLOR_RGB:    bytes_per_pixel = 3; break;
        case PNG_COLOR_RGBA:   bytes_per_pixel = 4; break;
        case PNG_COLOR_GRAY:   bytes_per_pixel = 1; break;
        case PNG_COLOR_GRAYA:  bytes_per_pixel = 2; break;
        default:               bytes_per_pixel = 3; break;
    }

    uint32_t rowbytes = context.width * bytes_per_pixel;
    uint32_t stride = rowbytes + 1; // +1 car c'est un filtre byte chaque lignes png commence par un octet qui indique le type de filtre appliqué
    uint32_t pixels_size = stride * context.height;
    context.pixels = kmalloc(pixels_size); // buffer qui va recevoir la sortie du DEFLATE compresser
    if (!context.pixels) {
        kfree(context.idat_buffer);
        context.idat_buffer = NULL;
        return context;
    }

    if (context.idat_size >= 6) { // 2 + DEFLATE + 4 :  4 + 2 = 6
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

        serial_println("Start parsing DEFLATE");

        // Parsing de l'header DEFLATE
        if (inflate(&bit_reader, context.pixels, pixels_size) < 0) {
            serial_println("erreur DEFLATE");
            kfree(context.pixels);
            context.pixels = NULL;
            kfree(context.idat_buffer);
            context.idat_buffer = NULL;
            return context;
        }

        serial_println("FINISH");

        uint8_t *zero_row = kmalloc(rowbytes);
        if (!zero_row) {
            kfree(context.pixels);
            context.pixels = NULL;
            kfree(context.idat_buffer);
            context.idat_buffer = NULL;
            return context;
        }
        for (uint32_t i = 0; i < rowbytes; i++) zero_row[i] = 0;

        for (uint32_t y = 0; y < context.height; y++) {
            uint8_t *row = context.pixels + y * stride;
            uint8_t filter = row[0];
            uint8_t *cur = row + 1; // +1 pour sauter l'octet de filtre
            uint8_t *prev = (y > 0) ? cur - stride : zero_row;
            uint32_t x;

            switch (filter) {
                case 0: break;

                case 1: // left
                    for (x = bytes_per_pixel; x < rowbytes; x++)
                        cur[x] += cur[x - bytes_per_pixel];
                    break;

                case 2: // up
                    for (x = 0; x < rowbytes; x++)
                        cur[x] += prev[x];
                    break;

                case 3: // moyenne left/up
                    for (x = 0; x < bytes_per_pixel && x < rowbytes; x++)
                        cur[x] += prev[x] / 2;
                    for (x = bytes_per_pixel; x < rowbytes; x++)
                        cur[x] += (cur[x - bytes_per_pixel] + prev[x]) / 2;
                    break;

                case 4: // paeth
                    for (x = 0; x < bytes_per_pixel && x < rowbytes; x++)
                        cur[x] += paeth(0, prev[x], 0);
                    for (x = bytes_per_pixel; x < rowbytes; x++)
                        cur[x] += paeth(cur[x - bytes_per_pixel], prev[x], prev[x - bytes_per_pixel]);
                    break;
            }
        }

        kfree(zero_row);
    }

    kfree(context.idat_buffer);
    context.idat_buffer = NULL;

    return context;
}