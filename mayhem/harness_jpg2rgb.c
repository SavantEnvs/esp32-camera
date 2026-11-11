// JPEG -> RGB565/RGB888 decoders: bounded (caller-sized output) and unbounded (output sized from the
// JPEG header via esp_jpeg_get_image_info, as the API documents). Byte 0 selects scale/mode/buffer size.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "img_converters.h"

static uint8_t work[3100];

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint8_t b0 = size ? data[0] : 0;
    const uint8_t *jpg = size ? data + 1 : data;
    size_t jpg_len = size ? size - 1 : 0;
    esp_jpeg_image_scale_t scale = (esp_jpeg_image_scale_t)(b0 & 3);
    int mode = (b0 >> 2) & 3;

    if (mode < 2) {
        size_t out_size = (size_t)1 << (6 + 4 * ((b0 >> 4) & 3));
        uint8_t *out = malloc(out_size);
        if (!out) {
            return 0;
        }
        if (mode == 0) {
            jpg2rgb565_bounded(jpg, jpg_len, out, out_size, scale);
        } else {
            fmt2rgb888_bounded(jpg, jpg_len, PIXFORMAT_JPEG, out, out_size);
        }
        free(out);
        return 0;
    }

    // Unbounded APIs: the caller must provide the size the header declares for the chosen format/scale.
    esp_jpeg_image_cfg_t cfg = {
        .indata = (uint8_t *)jpg,
        .indata_size = jpg_len,
        .out_format = mode == 2 ? JPEG_IMAGE_FORMAT_RGB565 : JPEG_IMAGE_FORMAT_RGB888,
        .out_scale = mode == 2 ? scale : JPEG_IMAGE_SCALE_0,
        .advanced.working_buffer = work,
        .advanced.working_buffer_size = sizeof(work),
    };
    esp_jpeg_image_output_t info = {0};
    if (esp_jpeg_get_image_info(&cfg, &info) != ESP_OK || info.output_len > ((size_t)16 << 20)) {
        return 0;
    }
    uint8_t *out = malloc(info.output_len ? info.output_len : 1);
    if (!out) {
        return 0;
    }
    if (mode == 2) {
        jpg2rgb565(jpg, jpg_len, out, scale);
    } else {
        fmt2rgb888(jpg, jpg_len, PIXFORMAT_JPEG, out);
    }
    free(out);
    return 0;
}
