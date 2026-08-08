#ifndef PNG_H
#define PNG_H
#include <stdint.h>

static const unsigned char PNG_MAGIC_NUMBER[8] = {
    0x89, 0x50, 0x4E, 0x47,0x0D, 0x0A, 0x1A, 0x0A
};

typedef struct {
    uint32_t length;
    char     type[4]; // IHDR, IDAT, IEND
    uint8_t  *data;
} Chunk;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t  bit_depth; // 8 bits par canal pour l'instant
    uint8_t  color_type;
    uint8_t  compression;
    uint8_t  filter;
    uint8_t  interlace;
} IHDR;

#define PNG_COLOR_RGB      2
#define PNG_COLOR_RGBA     6
#define PNG_COLOR_GRAY     0
#define PNG_COLOR_INDEXED  3
#define PNG_COLOR_GRAYA    4

typedef struct {
    uint8_t  *data;
    uint32_t  size;
    uint32_t  pos;
    uint32_t  bits;
    uint32_t  bit_count;
} BitReader;

// Cette struct n'est pas optimiser mais c'est la plus simple pour commencer
typedef struct {
    uint16_t codes[288];   // code binaire pour chaque symbole
    uint8_t  lengths[288];
    int      count;        // nombre de symboles
} HuffmanTable;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t  bit_depth;
    uint8_t  color_type;
    uint8_t  interlace;
    uint8_t  *idat_buffer;
    uint32_t idat_size;
    uint8_t  *pixels;
} PngContext;

static uint32_t read_u32_be(uint8_t *p);
int read_bits(BitReader *bit_reader, int n);
PngContext png_decode(uint8_t *data, uint32_t size);

#endif
