#pragma once
#include <stdio.h>
#define ESP_LOGE(tag, fmt, ...) do { (void)(tag); if (0) printf(fmt, ##__VA_ARGS__); } while (0)
#define ESP_LOGW(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGV(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
