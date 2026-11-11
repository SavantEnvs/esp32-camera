#pragma once
#include "esp_err.h"
#include "esp_log.h"
#define ESP_RETURN_ON_FALSE(a, err_code, log_tag, format, ...) do { \
        if (!(a)) { ESP_LOGE(log_tag, format, ##__VA_ARGS__); return err_code; } \
    } while (0)
#define ESP_GOTO_ON_FALSE(a, err_code, goto_tag, log_tag, format, ...) do { \
        if (!(a)) { ESP_LOGE(log_tag, format, ##__VA_ARGS__); ret = err_code; goto goto_tag; } \
    } while (0)
