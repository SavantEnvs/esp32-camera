// Raw pixel formats -> RGB888 (fmt2rgb888 / fmt2rgb888_bounded). Bytes 0..2 select format and output size.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "img_converters.h"

static const pixformat_t formats[8] = {
    PIXFORMAT_RGB888, PIXFORMAT_RGB565, PIXFORMAT_GRAYSCALE, PIXFORMAT_YUV422,
    PIXFORMAT_YUV420, PIXFORMAT_RAW, PIXFORMAT_RGB444, PIXFORMAT_RAW8,
};

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 3) {
        return 0;
    }
    int sel = data[0] & 7;
    pixformat_t fmt = formats[sel];
    const uint8_t *payload = data + 3;
    size_t src_len = size - 3;

    uint8_t *src = malloc(src_len ? src_len : 1);
    if (!src) {
        return 0;
    }
    memcpy(src, payload, src_len);

    // Bytes the conversion writes for a fully-fitting buffer (0 for formats it leaves untouched).
    size_t need = 0;
    switch (sel) {
    case 0: need = src_len; break;
    case 1: need = (src_len / 2) * 3; break;
    case 2: need = src_len * 3; break;
    case 3: need = (src_len / 4) * 6; break;
    default: break;
    }

    if (data[0] & 8) {
        // bounded API with an arbitrary caller-declared size and a buffer of exactly that size
        size_t out_size = ((size_t)data[1] << 8 | data[2]) % (need + 16);
        uint8_t *out = malloc(out_size ? out_size : 1);
        if (out) {
            fmt2rgb888_bounded(src, src_len, fmt, out, out_size);
            free(out);
        }
    } else {
        // unbounded API: caller provides exactly the documented output size
        uint8_t *out = malloc(need ? need : 1);
        if (out) {
            fmt2rgb888(src, src_len, fmt, out);
            fmt2rgb888_bounded(src, src_len, fmt, out, need);
            free(out);
        }
    }
    free(src);
    return 0;
}
