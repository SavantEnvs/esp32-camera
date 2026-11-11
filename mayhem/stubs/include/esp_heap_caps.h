#pragma once
#include <stdlib.h>
#define MALLOC_CAP_SPIRAM 0x400
#define MALLOC_CAP_8BIT 0x4
#define MALLOC_CAP_DEFAULT 0x1000
static inline void *heap_caps_malloc(size_t size, unsigned caps) { (void)caps; return malloc(size); }
