// JPEG -> BMP through the public fmt2bmp(PIXFORMAT_JPEG) path (jpg2bmp + esp_jpeg/tjpgd decoder).
#include <stdint.h>
#include <stdlib.h>
#include "img_converters.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint8_t *out = NULL;
    size_t out_len = 0;
    if (fmt2bmp((uint8_t *)data, size, 0, 0, PIXFORMAT_JPEG, &out, &out_len)) {
        volatile uint8_t last = out[out_len - 1];
        (void)last;
        free(out);
    }
    return 0;
}
