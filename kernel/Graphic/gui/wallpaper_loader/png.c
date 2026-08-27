#include <stdint.h>
#include <stddef.h>
#include "../lib/string.h"
#include "../memory/kmalloc.h"
#include "png.h"
#include "tty.h"

/* c'est stocké comme ça
    +------------+----------+--------------+----------+
    | Length (4) | Type (4) | Data (Length)| CRC (4)  |
    +------------+----------+--------------+----------+
 */

// l'ordinateur stocke les bits en little endian, mais le format png le stocke en big endian
uint32_t read_u32_be(uint8_t *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |
           ((uint32_t)p[3]);
}

uint32_t reverse_bits(int value, int nbits) {
    int result = 0;
    for (int i = 0; i < nbits; i++) {
        result = (result << 1) | (value & 1);
        value >>= 1;
    }

    return result;
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
    uint32_t value = bit_reader->bits & ((1u << n) - 1);
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

PngContext png_decode(uint8_t *data, uint32_t size) {
    PngContext context = {0};
    printk("png_decode start\n");

    if (size < 8) { printk("header is too small\n"); return context; }
    if (memcmp(data, PNG_MAGIC_NUMBER, 8) != 0) { printk("magic number have problem\n"); return context; }
    printk("magic is good\n");

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
        // un png peut avoir plusieurs chunks IDAT a la suite
        // on doit donc tous les reunir dnas un buffer  

            uint8_t *buffer = kmalloc(context.idat_size + length);

            if (!buffer) return context;

            // si c'est pas le premier IDAT
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
        printk("chunks are good w=%d h=%d idat=%d\n", context.width, context.height, context.idat_size);


    uint32_t bytes_per_pixel;
    switch (context.color_type) {
        case PNG_COLOR_RGB:    bytes_per_pixel = 3; break;
        case PNG_COLOR_RGBA:   bytes_per_pixel = 4; break;
        case PNG_COLOR_GRAY:   bytes_per_pixel = 1; break;
        case PNG_COLOR_GRAYA:  bytes_per_pixel = 2; break;
        default:               bytes_per_pixel = 3; break;
    }

    uint32_t pixels_size = (context.width * bytes_per_pixel + 1) * context.height; // +1 car c'est un filtre byte chaque lignes png commence par un octet qui indique le type de filtre appliqué
    context.pixels = kmalloc(pixels_size); // buffer qui va recevoir la sortie du DEFLATE compresser
    if (!context.pixels) return context;


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

         // Parsing de l'header DFLATE
        uint32_t output_pos = 0;

        int BFINAL = read_bits(&bit_reader, 1);
        int BTYPE  = read_bits(&bit_reader, 2); // type de compression

        do {
            switch (BTYPE) {
                case 0:
                    bit_reader.bits = 0;
                    bit_reader.bit_count = 0;

                    // masque les bit restant et commence au prochain octet
                    // LEN
                    // exemple : on lis 2 octets en little endian 0x05 | (0x00 << 8) = 5
                    uint16_t len = bit_reader.data[bit_reader.pos] | (bit_reader.data[bit_reader.pos + 1] << 8);
                    bit_reader.pos += 2;

                    // NLEN
                    uint16_t nlen = bit_reader.data[bit_reader.pos] | (bit_reader.data[bit_reader.pos + 1] << 8);
                    bit_reader.pos += 2;

                    // est ce que nlen est bien le complément de nlen
                    if ((uint16_t)~len != nlen) return context;

                    memcpy(context.pixels + output_pos, bit_reader.data + bit_reader.pos, len);
                    bit_reader.pos += len;
                    output_pos += len;
                    // pas de decompression on copie len dnas pixels

                    break; // bloc non compresser

                case 1: {
                    // Valeures RFC 1951
                    uint8_t lengths[288];

                    for (int i = 0;   i <= 143; i++) lengths[i] = 8;
                    for (int i = 144; i <= 255; i++) lengths[i] = 9;
                    for (int i = 256; i <= 279; i++) lengths[i] = 7;
                    for (int i = 280; i <= 287; i++) lengths[i] = 8;

                    uint16_t bl_count[16] = {0};
                    // compte la taille de chaques symbole récurssivement
                    for (int i = 0; i < 288; i++) bl_count[lengths[i]]++;

                    /*
                      Exemple :
                      A = 2 bits
                      B = 2 bits
                      C = 3 bits
                      D = 3 bits

                      donc bl_count[2] = 2 et bl_count[3] = 2 car 2 codes de 2 bits et 2 codes de 3

                      Algo :
                      Logiquement :
                      A = 00
                      B = 01
                      C = 10
                      D = 11

                      mais C et D font 3 bits donc << donc next_code[3] = 100 et [4] = 110
                     */

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

                   int huff_code = 0;
                   int nbits = 0;
                   int done = 0;

                   // tableaux rfc
                   static const int length_base[]  = 
                   { 
                   3, 4, 5, 6, 7, 8, 10, 12, 14, 16,
                   18, 22, 26, 30, 34, 42, 50, 58, 66, 82,
                   98, 114, 130, 162, 194, 226, 258
                   };

                   static const int length_extra[] = 
                   {
                   0, 0, 0, 0, 0, 1, 1, 1, 1, 2,
                   2, 2, 2, 2, 3, 3, 3, 3, 4, 4,
                   4, 4, 5, 5, 5, 5, 0
                   };

                   static const int distance_base[] =
                   {
                   1, 2, 3, 4, 5, 7, 9, 13, 17, 25,
                   33, 49, 65, 97, 129, 193, 257, 385, 513, 769,
                   1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289,
                   16385, 24577
                   };

                   static const int distance_extra[] =
                   {
                   0, 0 ,0, 0, 1, 1, 2, 2, 3, 3,
                   4, 4, 5, 5, 6, 6, 7, 7, 8, 8,
                   9, 9, 10, 10, 11, 11, 12, 12,
                   13, 13
                   };
         

                   while (done != 1) {
                       int next_bit = read_bits(&bit_reader, 1);
                       huff_code = (huff_code << 1) | next_bit;
                       nbits++;

                       for (int i = 0; i < 288; i++) {
                            if (table.lengths[i] == nbits && table.codes[i] == huff_code) {
                                if (i >= 0 && i <= 255) { //octet a ecrire
                                    context.pixels[output_pos] = i;
                                    output_pos++;
                                }
                                else if (i == 256) { // fin du bloc
                                    done = 1;
                                }
                                else if (i >= 257 && i <= 285) { // back reference
                                    int length = length_base[i - 257] + read_bits(&bit_reader, length_extra[i - 257]); 
                                    int code_dist = read_bits(&bit_reader, 5);
                                    int distance = distance_base[code_dist] + read_bits(&bit_reader, distance_extra[code_dist]);

                                    for (int j = 0; j < length; j++) {
                                        if (output_pos >= pixels_size) return context;
                                        context.pixels[output_pos] = context.pixels[output_pos - distance];
                                        output_pos++;
                                    }
                                    // ici on copie les blocs similaires (optimisation de la rfc indispensable pour la decompression)
                                    // pas de memcpy car il peut arriver que la source et la dest pointe au meme endroit
                                    // sinon ca pourrai faire pour abc -> abab la boucle donne abcabc
                                }
                                huff_code  = 0;
                                nbits = 0;
                                break;
                            }
                        }    
                    }

                    break; // Huffman fixe
                }

                case 2: {
                    // spec DEFLATE
                    int HLIT  = read_bits(&bit_reader, 5) + 257; 
                    int HDIST = read_bits(&bit_reader, 5) + 1;
                    int HCLEN = read_bits(&bit_reader, 4) + 4;

                    int order[19] = {
                    16,17,18,0,8,
                    7,9,6,10,5,
                    11,4,12,3,
                    13,2,14,1,15
                    };
                    uint8_t code_lengths[19] = {0};

                    for (int i = 0; i < HCLEN; i++) {
                        code_lengths[order[i]] = read_bits(&bit_reader, 3);
                    }
                    
                    // construction de la premiere table huffman (preparation plutot pour les 2 prochaines tables)
                    uint16_t bl_count[16] = {0};
                    for (int i = 0; i < 19; i++) bl_count[code_lengths[i]]++;

                    uint16_t next_code[16] = {0};
                    uint16_t code = 0;
                    for (int bits = 1; bits < 16; bits++) {
                        code = (code + bl_count[bits - 1]) << 1;
                        next_code[bits] = code;
                    }

                    HuffmanTable cl_table = {0};
                    for (int i = 0; i < 19; i++) {
                        if (code_lengths[i] != 0) {
                            cl_table.codes[i]   = next_code[code_lengths[i]]++;
                            cl_table.lengths[i] = code_lengths[i];
                        }
                    }

                    uint8_t all_lengths[HLIT + HDIST];
                    int index = 0;
                    int huff_code = 0;
                    int nbits = 0;

                    while (index < HLIT + HDIST) {
                        int next_bit = read_bits(&bit_reader, 1);
                        huff_code = (huff_code << 1) | next_bit;
                        nbits++;

                        // va servir a construire les 2 tables huffman avec les litteraux/longueur et celle  des distances
                        for (int i = 0; i < 19; i++) {
                            if (cl_table.lengths[i] == nbits && cl_table.codes[i] == huff_code) {
                                if (i >= 0 && i <= 15) {
                                    all_lengths[index] = i;
                                    index++;
                                }
                                else if (i == 16) { 
                                    int repeat = read_bits(&bit_reader, 2) + 3; // spec si on repete moins de 3 fois ca sert a rien autant les ecrire en brut ca economise des bits
                                    for (int j = 0; j < repeat; j++) {
                                        all_lengths[index] = all_lengths[index - 1];
                                        index++;
                                    }
                                    // on copie j fois le symbole que la compression nous indique de faire
                                }
                                else if (i == 17) {
                                    int repeat = read_bits(&bit_reader, 3) + 3; 
                                    for (int j = 0; j < repeat; j++) {
                                        all_lengths[index] = 0;
                                        index++;
                                    }
                                }
                                else if (i == 18) {
                                    int repeat = read_bits(&bit_reader, 7) + 11;
                                    for (int j = 0; j < repeat; j++) {
                                        all_lengths[index] = 0;
                                        index++;
                                    }
                                }
                            }
                        }  
                    }

                    // LIT TABLE
                    uint16_t bl_count2[16] = {0};
                    for (int i = 0; i < HLIT; i++) bl_count2[all_lengths[i]]++;

                    uint16_t next_code2[16] = {0};
                    uint16_t code2 = 0;
                    for (int bits = 1; bits < 16; bits++) {
                        code2 = (code2 + bl_count2[bits - 1]) << 1;
                        next_code2[bits] = code2;
                    }

                    // table qui decode les symboles 0-285 (octets litteraux + back reference)
                    HuffmanTable lit_table = {0};
                    for (int i = 0; i < HLIT; i++) {
                        if (all_lengths[i] != 0) {
                            lit_table.codes[i]   = next_code2[all_lengths[i]]++;
                            lit_table.lengths[i] = all_lengths[i];
                        }
                    }
                               
                    // DIST TABLE
                    uint16_t bl_count3[16] = {0};
                    for (int i = 0; i < HDIST; i++) bl_count3[all_lengths[HLIT + i]]++; 


                    uint16_t next_code3[16] = {0};
                    uint16_t code3 = 0;
                    for (int bits = 1; bits < 16; bits++) {
                        code3 = (code3 + bl_count3[bits - 1]) << 1;
                        next_code3[bits] = code3;
                    }

                    // table qui decode les symboles 0-285 (octets litteraux + back reference)
                    HuffmanTable dist_table = {0};
                    for (int i = 0; i < HDIST; i++) {
                        if (all_lengths[HLIT + i] != 0) { // pour lire les distances sinon on lit les litteraux
                            dist_table.codes[i]   = next_code3[all_lengths[i]]++;
                            dist_table.lengths[i] = all_lengths[HLIT + i];
                        }
                    }

                    static const int length_base[]  = 
                   { 
                   3, 4, 5, 6, 7, 8, 10, 12, 14, 16,
                   18, 22, 26, 30, 34, 42, 50, 58, 66, 82,
                   98, 114, 130, 162, 194, 226, 258
                   };

                   static const int length_extra[] = 
                   {
                   0, 0, 0, 0, 0, 1, 1, 1, 1, 2,
                   2, 2, 2, 2, 3, 3, 3, 3, 4, 4,
                   4, 4, 5, 5, 5, 5, 0
                   };

                   static const int distance_base[] =
                   {
                   1, 2, 3, 4, 5, 7, 9, 13, 17, 25,
                   33, 49, 65, 97, 129, 193, 257, 385, 513, 769,
                   1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289,
                   16385, 24577
                   };

                   static const int distance_extra[] =
                   {
                   0, 0 ,0, 0, 1, 1, 2, 2, 3, 3,
                   4, 4, 5, 5, 6, 6, 7, 7, 8, 8,
                   9, 9, 10, 10, 11, 11, 12, 12,
                   13, 13
                   };

                    int done = 0;
                    while (done != 1) {
                       int next_bit = read_bits(&bit_reader, 1);
                       huff_code = (huff_code << 1) | next_bit;
                       nbits++;

                       for (int i = 0; i < 288; i++) {
                            if (lit_table.lengths[i] == nbits && lit_table.codes[i] == huff_code) {
                                if (i >= 0 && i <= 255) {
                                    context.pixels[output_pos] = i;
                                    output_pos++;
                                }
                                else if (i == 256) {
                                    done = 1;
                                }
                                else if (i >= 257 && i <= 285) {
                                    int length = length_base[i - 257] + read_bits(&bit_reader, length_extra[i - 257]); 
                                    
                                    int dist_huff = 0;
                                    int dist_nbits = 0;
                                    int code_dist = 0;

                                    // on cherche tant que c'est pas un symbole, pour cela il faut que dist_huff et dist_nbits matchent
                                    while (1) {
                                        dist_huff = (dist_huff << 1) | read_bits(&bit_reader, 1); 
                                        dist_nbits++;
                                        for (int d = 0; d < HDIST; d++) {
                                            if (dist_table.lengths[d] == dist_nbits && dist_table.codes[d] == dist_huff) {
                                                code_dist = d;
                                                goto found_dist;
                                            }
                                        }
                                    }
                                    found_dist:;

                                    int distance = distance_base[code_dist] + read_bits(&bit_reader, distance_extra[code_dist]);

                                    for (int j = 0; j < length; j++) {
                                        if (output_pos == 0 || distance > output_pos) return context;
                                        context.pixels[output_pos] = context.pixels[output_pos - distance];
                                        output_pos++;
                                    }
                                }
                                huff_code  = 0;
                                nbits = 0;
                                break;
                            }
                        }    
                    }

                    break; // Hufman dynamique
                }


                case 3: return context; // invalide
            }
        } while (!BFINAL);

        // Lors de la compression, l'algo applique un filtre : il augmente certain pixel ex : 100 -> 101 -> 102...
        // C'est parce que DEFLATE optimise dans ce cas. Il transforme en 1, 1, 1... et applique des opti
        // structure de context.picxels apres DEFLATE : [filtre][pixel][pixel]...[filtre][pixel][pixel]...
        //                                                |ligne 0                 |ligne 1
        for (uint32_t y = 0; y < context.height; y++) {
            uint32_t row_start = y * (context.width * bytes_per_pixel + 1);
            uint8_t filter = context.pixels[row_start];
                
            for (uint32_t x = 0; x < context.width * bytes_per_pixel; x++) {
                uint32_t index = row_start + 1 + x; // +1 pour sauter l'octet de filtre
                uint8_t left;
                uint8_t up;
                uint8_t up_left;

                if (x >= bytes_per_pixel) {
                    left = context.pixels[index - bytes_per_pixel];
                } else {
                    left = 0;
                }

                if (y > 0) {
                    up = context.pixels[index - (context.width * bytes_per_pixel + 1)];
                } else {
                    up = 0;
                }

                if (y > 0 && x >= bytes_per_pixel) {
                    up_left = context.pixels[index - (context.width * bytes_per_pixel + 1) - bytes_per_pixel];
                } else {
                    up_left = 0;
                }

                switch (filter) {
                    case 0: break;
                    case 1: context.pixels[index] += left;            break;
                    case 2: context.pixels[index] += up;              break;
                    case 3: context.pixels[index] += (left + up) / 2; break;
                    case 4: context.pixels[index] += paeth(left, up, up_left);
                }
            }
        }
    }

    return context;
}

