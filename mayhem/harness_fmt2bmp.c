// Raw pixel formats -> BMP (fmt2bmp). Bytes 0..2: format, width, height; the source buffer is sized
// width*height*bytes-per-pixel as the API requires (fmt2bmp takes no source length for raw formats).
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "img_converters.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static const pixformat_t formats[4] = {PIXFORMAT_RGB565, PIXFORMAT_YUV422, PIXFORMAT_GRAYSCALE, PIXFORMAT_RGB888};
    static const unsigned bpp[4] = {2, 2, 1, 3};
    if (size < 3) {
        return 0;
    }
    int sel = data[0] & 3;
    uint16_t width = 1 + data[1] % 96;
    uint16_t height = 1 + data[2] % 96;
    size_t need = (size_t)width * height * bpp[sel];

    uint8_t *src = calloc(need, 1);
    if (!src) {
        return 0;
    }
    size_t have = size - 3 < need ? size - 3 : need;
    memcpy(src, data + 3, have);

    uint8_t *out = NULL;
    size_t out_len = 0;
    if (fmt2bmp(src, need, width, height, formats[sel], &out, &out_len)) {
        volatile uint8_t last = out[out_len - 1];
        (void)last;
        free(out);
    }
    free(src);
    return 0;
}
