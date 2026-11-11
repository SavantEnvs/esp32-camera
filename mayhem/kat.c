// Known-answer probe for the esp32-camera image converters. Prints `key=value` lines; mayhem/test.sh
// compares each against a value worked out by hand from the format definitions (BMP layout, RGB565 /
// grayscale / YUYV packing, JPEG pixel content produced by an independent encoder).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "img_converters.h"

#include "kat_jpegs.h"

static void hex(const char *key, const uint8_t *p, size_t n)
{
    printf("%s=", key);
    for (size_t i = 0; i < n; i++) {
        printf("%02x", p[i]);
    }
    printf("\n");
}

// one of the three channels near full scale, the other two near zero (red or blue, whatever the byte order)
static int pure_primary(const uint8_t *p)
{
    int hi = 0, lo = 0;
    for (int i = 0; i < 3; i++) {
        hi += p[i] >= 0xF0;
        lo += p[i] <= 0x10;
    }
    return hi == 1 && lo == 2;
}

static void bmp_pixel(const char *key, const uint8_t *bmp, int width, int x, int y)
{
    hex(key, bmp + 54 + 3 * ((size_t)y * width + x), 3);
}

int main(void)
{
    uint8_t out[256];
    size_t out_len;
    uint8_t *bmp = NULL;

    const uint8_t rgb565[] = {0xF8, 0x00, 0x07, 0xE0, 0x00, 0x1F};
    memset(out, 0xEE, sizeof(out));
    fmt2rgb888(rgb565, sizeof(rgb565), PIXFORMAT_RGB565, out);
    hex("rgb565_to_rgb888", out, 9);

    const uint8_t gray[] = {0x12, 0xAB};
    memset(out, 0xEE, sizeof(out));
    fmt2rgb888(gray, sizeof(gray), PIXFORMAT_GRAYSCALE, out);
    hex("gray_to_rgb888", out, 7);

    const uint8_t yuyv[] = {0x00, 0x80, 0xFF, 0x80};
    memset(out, 0xEE, sizeof(out));
    fmt2rgb888(yuyv, sizeof(yuyv), PIXFORMAT_YUV422, out);
    hex("yuyv_to_rgb888", out, 7);

    memset(out, 0xEE, sizeof(out));
    printf("bounded_gray_5=%d\n", fmt2rgb888_bounded(gray, sizeof(gray), PIXFORMAT_GRAYSCALE, out, 5));
    printf("bounded_gray_6=%d\n", fmt2rgb888_bounded(gray, sizeof(gray), PIXFORMAT_GRAYSCALE, out, 6));
    hex("bounded_gray_6_out", out, 7);

    uint8_t rgb[] = {1, 2, 3, 4, 5, 6};
    if (fmt2bmp(rgb, sizeof(rgb), 2, 1, PIXFORMAT_RGB888, &bmp, &out_len)) {
        printf("bmp_rgb888_len=%u\n", out_len);
        hex("bmp_rgb888", bmp, out_len);
        free(bmp);
    }

    uint8_t g2[] = {0x7F, 0x80};
    if (fmt2bmp(g2, sizeof(g2), 2, 1, PIXFORMAT_GRAYSCALE, &bmp, &out_len)) {
        printf("bmp_gray_len=%u\n", out_len);
        hex("bmp_gray_header", bmp, 54);
        hex("bmp_gray_pal0", bmp + 54, 4);
        hex("bmp_gray_pal1", bmp + 58, 4);
        hex("bmp_gray_pal255", bmp + 54 + 4 * 255, 4);
        hex("bmp_gray_pixels", bmp + 54 + 1024, 2);
        free(bmp);
    }

    uint8_t p565[] = {0xF8, 0x00};
    if (fmt2bmp(p565, sizeof(p565), 1, 1, PIXFORMAT_RGB565, &bmp, &out_len)) {
        printf("bmp_rgb565_len=%u\n", out_len);
        hex("bmp_rgb565", bmp, out_len);
        free(bmp);
    }

    if (fmt2bmp((uint8_t *)gray_jpg, sizeof(gray_jpg), 0, 0, PIXFORMAT_JPEG, &bmp, &out_len)) {
        printf("jpg_gray_bmp_len=%u\n", out_len);
        hex("jpg_gray_bmp_header", bmp, 54);
        bmp_pixel("jpg_gray_px_0_0", bmp, 16, 0, 0);
        bmp_pixel("jpg_gray_px_7_0", bmp, 16, 7, 0);
        bmp_pixel("jpg_gray_px_8_0", bmp, 16, 8, 0);
        bmp_pixel("jpg_gray_px_15_0", bmp, 16, 15, 0);
        bmp_pixel("jpg_gray_px_0_7", bmp, 16, 0, 7);
        bmp_pixel("jpg_gray_px_15_7", bmp, 16, 15, 7);
        free(bmp);
    }

    if (fmt2bmp((uint8_t *)rb444_jpg, sizeof(rb444_jpg), 0, 0, PIXFORMAT_JPEG, &bmp, &out_len)) {
        printf("jpg_rb_bmp_len=%u\n", out_len);
        const uint8_t *l1 = bmp + 54, *l2 = bmp + 54 + 3 * (8 * 16 + 7), *r1 = bmp + 54 + 3 * 8, *r2 = bmp + 54 + 3 * (15 * 16 + 15);
        printf("jpg_rb_left_pure=%d\n", pure_primary(l1) && pure_primary(l2));
        printf("jpg_rb_right_pure=%d\n", pure_primary(r1) && pure_primary(r2));
        printf("jpg_rb_sides_differ=%d\n", memcmp(l1, r1, 3) != 0 && memcmp(l1, l2, 3) == 0 && memcmp(r1, r2, 3) == 0);
        free(bmp);
    }

    uint8_t o565[256];
    memset(o565, 0xEE, sizeof(o565));
    printf("jpg_gray_565=%d\n", jpg2rgb565_bounded(gray_jpg, sizeof(gray_jpg), o565, 16 * 8 * 2, JPG_SCALE_NONE));
    hex("jpg_gray_565_x0", o565, 2);
    hex("jpg_gray_565_x8", o565 + 16, 2);
    printf("jpg_gray_565_small=%d\n", jpg2rgb565_bounded(gray_jpg, sizeof(gray_jpg), o565, 16 * 8 * 2 - 1, JPG_SCALE_NONE));
    printf("jpg_gray_565_half=%d\n", jpg2rgb565_bounded(gray_jpg, sizeof(gray_jpg), o565, 8 * 4 * 2, JPG_SCALE_2X));
    hex("jpg_gray_565_half_x0", o565, 2);
    hex("jpg_gray_565_half_x4", o565 + 8, 2);

    uint8_t o888[16 * 8 * 3];
    printf("jpg_gray_888_small=%d\n", fmt2rgb888_bounded(gray_jpg, sizeof(gray_jpg), PIXFORMAT_JPEG, o888, sizeof(o888) - 1));
    printf("jpg_gray_888=%d\n", fmt2rgb888_bounded(gray_jpg, sizeof(gray_jpg), PIXFORMAT_JPEG, o888, sizeof(o888)));
    hex("jpg_gray_888_x7", o888 + 21, 3);
    hex("jpg_gray_888_x8", o888 + 24, 3);

    printf("jpg_garbage=%d\n", fmt2bmp((uint8_t *)rgb, sizeof(rgb), 0, 0, PIXFORMAT_JPEG, &bmp, &out_len));

    // encoder round trip: left half red, right half blue
    uint8_t img[16 * 16 * 3];
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint8_t *p = img + 3 * (y * 16 + x);
            p[0] = x < 8 ? 255 : 0;
            p[1] = 0;
            p[2] = x < 8 ? 0 : 255;
        }
    }
    uint8_t *jpg = NULL;
    size_t jpg_len = 0;
    jpgSetChroma(CHROMA_444);
    if (fmt2jpg(img, sizeof(img), 16, 16, PIXFORMAT_RGB888, 90, &jpg, &jpg_len)) {
        printf("enc_len_ok=%d\n", jpg_len > 4 && jpg_len < 128 * 1024);
        hex("enc_soi", jpg, 2);
        hex("enc_eoi", jpg + jpg_len - 2, 2);
        uint8_t back[16 * 16 * 3];
        printf("enc_decode=%d\n", fmt2rgb888_bounded(jpg, jpg_len, PIXFORMAT_JPEG, back, sizeof(back)));
        const uint8_t *l = back + 3 * (8 * 16 + 3), *r = back + 3 * (8 * 16 + 12);
        int ok_l = l[1] < 40 && ((l[0] > 215 && l[2] < 40) || (l[2] > 215 && l[0] < 40));
        int ok_r = r[1] < 40 && ((r[0] > 215 && r[2] < 40) || (r[2] > 215 && r[0] < 40));
        printf("enc_roundtrip_left_primary=%d\n", ok_l);
        printf("enc_roundtrip_right_primary=%d\n", ok_r);
        printf("enc_roundtrip_sides_differ=%d\n", l[0] != r[0] && l[2] != r[2]);
        free(jpg);
    }
    return 0;
}
