/* Force-included into every host-compiled TU (library, harnesses, probe).
 *
 * esp32-camera targets a 32-bit chip where size_t == unsigned int; upstream relies on that in places
 * (to_jpg.cpp overrides jpge::output_stream::get_size(), declared `unsigned int`, with a `size_t`
 * return; esp_jpeg passes `unsigned int` callbacks where tjpgd expects `size_t` ones). On a 64-bit host
 * those are hard errors. Pull in every system header first, then make size_t 32-bit for the project's own
 * code, so struct layouts and callback types match the device and no upstream file is touched. */
#pragma once
#include <assert.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <limits.h>
#define size_t unsigned int
