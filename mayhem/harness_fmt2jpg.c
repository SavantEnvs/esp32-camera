// Raw pixel formats -> JPEG encoder (fmt2jpg / fmt2jpg_cb, jpge). Bytes 0..4 select format, quality,
// chroma mode, RGB565 byte order, output path and dimensions; the source buffer is width*height*bpp bytes.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "img_converters.h"

static size_t sink_cb(void *arg, size_t index, const void *buf, size_t len)
{
    (void)arg; (void)index; (void)buf;
    return len;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static const pixformat_t formats[4] = {PIXFORMAT_RGB565, PIXFORMAT_YUV422, PIXFORMAT_GRAYSCALE, PIXFORMAT_RGB888};
    static const unsigned bpp[4] = {2, 2, 1, 3};
    if (size < 5) {
        return 0;
    }
    int sel = data[0] & 3;
    jpgSetChroma((chroma_t)(1 + data[2] % 3));
    jpgSetRgb565BE(data[2] & 0x8);
    // YUYV packs two pixels in four bytes, so a YUV422 frame always has an even width.
    uint16_t width = sel == 1 ? 2 * (1 + data[3] % 80) : 1 + data[3] % 160;
    uint16_t height = 1 + data[4] % 160;
    size_t need = (size_t)width * height * bpp[sel];

    uint8_t *src = calloc(need, 1);
    if (!src) {
        return 0;
    }
    size_t have = size - 5 < need ? size - 5 : need;
    memcpy(src, data + 5, have);

    if (data[2] & 0x10) {
        fmt2jpg_cb(src, need, width, height, formats[sel], data[1], sink_cb, NULL);
    } else {
        uint8_t *out = NULL;
        size_t out_len = 0;
        if (fmt2jpg(src, need, width, height, formats[sel], data[1], &out, &out_len)) {
            volatile uint8_t first = out_len ? out[0] : 0;
            (void)first;
            free(out);
        }
    }
    free(src);
    return 0;
}
