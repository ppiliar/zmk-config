// Minimal 1bpp grayscale PNG writer (no dependencies) for the simulator.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint32_t crc_table[256];
static void crc_init(void) {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++)
            c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
        crc_table[n] = c;
    }
}

static uint32_t crc32_buf(const uint8_t *buf, size_t len) {
    uint32_t c = 0xffffffffu;
    for (size_t i = 0; i < len; i++)
        c = crc_table[(c ^ buf[i]) & 0xff] ^ (c >> 8);
    return c ^ 0xffffffffu;
}

static void be32(uint8_t *p, uint32_t v) {
    p[0] = v >> 24;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8) & 0xff;
    p[3] = v & 0xff;
}

static void write_chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    uint8_t hdr[8];
    be32(hdr, len);
    memcpy(hdr + 4, type, 4);
    fwrite(hdr, 1, 8, f);
    if (len)
        fwrite(data, 1, len, f);
    uint32_t crc = crc32_buf((const uint8_t *)type, 4);
    if (len)
        crc = crc32_buf(data, len) ^ 0; // need crc over both
    // recompute properly over type+data
    uint8_t *tmp = malloc(len + 4);
    memcpy(tmp, type, 4);
    if (len)
        memcpy(tmp + 4, data, len);
    crc = crc32_buf(tmp, len + 4);
    free(tmp);
    uint8_t crcb[4];
    be32(crcb, crc);
    fwrite(crcb, 1, 4, f);
}

// The panel's framebuffer is 160x68 landscape, but the module is mounted
// rotated 90 deg on the keyboard, and the widgets draw each 68x68 tile rotated
// into it (rotate_canvas: lv_canvas_transform angle 900 = 90 deg clockwise).
// Rotating the framebuffer 90 deg CCW recovers the portrait view the user
// actually sees on the device.
//
// The nice!view panel is a Sharp LS0xx memory LCD: a 1 bit renders BLACK. The
// widgets author the framebuffer like paper (white bg, black ink), so the
// rotated companion inverts the bits — making it show the physical appearance
// the user sees on the keyboard, not the framebuffer convention.
void sim_png_write_rotated(const uint8_t *fb, int w, int h, const char *path) {
    // 90 deg CCW with polarity inversion:
    // out(x'=y, y'=w-1-x) = !fb(x,y); out is h wide, w tall
    uint8_t *rot = malloc(h * w);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            rot[(w - 1 - x) * h + y] = fb[y * w + x] ? 0 : 1;
    sim_png_write(rot, h, w, path);
    free(rot);
}

// Scale factor so the 160x68 preview is viewable
#define SCALE 3

void sim_png_write(const uint8_t *fb, int w, int h, const char *path) {
    crc_init();
    const int W = w * SCALE, H = h * SCALE;

    // grayscale, 1 byte per pixel; raw scanlines with filter 0
    uint32_t raw_len = H * (1 + W);
    uint8_t *raw = malloc(raw_len);
    uint8_t *p = raw;
    for (int y = 0; y < H; y++) {
        *p++ = 0; // filter none
        const uint8_t *src = &fb[(y / SCALE) * w];
        for (int x = 0; x < W; x++) {
            // 0 = black, 255 = white; fb is 1 = white
            *p++ = src[x / SCALE] ? 0xff : 0x00;
        }
    }

    // zlib stream with stored (uncompressed) deflate blocks
    uint32_t z_len = raw_len + raw_len / 65535 * 5 + 5 + 6;
    uint8_t *z = malloc(z_len);
    uint8_t *zp = z;
    *zp++ = 0x78;
    *zp++ = 0x01;
    size_t off = 0;
    while (off < raw_len) {
        size_t chunk = raw_len - off;
        if (chunk > 65535)
            chunk = 65535;
        *zp++ = (chunk == 65535) ? 0x00 : 0x01;
        *zp++ = chunk & 0xff;
        *zp++ = (chunk >> 8) & 0xff;
        *zp++ = (~chunk) & 0xff;
        *zp++ = ((~chunk) >> 8) & 0xff;
        memcpy(zp, raw + off, chunk);
        zp += chunk;
        off += chunk;
    }
    uint32_t adler_a = 1, adler_b = 0;
    for (size_t i = 0; i < raw_len; i++) {
        adler_a = (adler_a + raw[i]) % 65521;
        adler_b = (adler_b + adler_a) % 65521;
    }
    be32(zp, (adler_b << 16) | adler_a);
    zp += 4;
    uint32_t z_used = zp - z;

    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("fopen");
        exit(1);
    }
    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    fwrite(sig, 1, 8, f);

    uint8_t ihdr[13];
    be32(ihdr, W);
    be32(ihdr + 4, H);
    ihdr[8] = 8;  // bit depth
    ihdr[9] = 0;  // grayscale
    ihdr[10] = 0; // deflate
    ihdr[11] = 0; // filter
    ihdr[12] = 0; // interlace
    write_chunk(f, "IHDR", ihdr, 13);
    write_chunk(f, "IDAT", z, z_used);
    write_chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw);
    free(z);
}
