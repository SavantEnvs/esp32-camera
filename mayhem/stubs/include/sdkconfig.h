/* Host-build stand-in for the ESP-IDF sdkconfig.h: esp_jpeg's Kconfig defaults and a plain ESP32 target. */
#pragma once
#define CONFIG_IDF_TARGET_ESP32 1
#define CONFIG_IDF_TARGET_ESP32S2 0
#define CONFIG_IDF_TARGET_ESP32S3 0
#define CONFIG_CAMERA_CONVERTER_ENABLED 0
#define CONFIG_SPIRAM 0
#define CONFIG_JD_USE_ROM 0
#define CONFIG_JD_SZBUF 512
#define CONFIG_JD_FORMAT 0
#define CONFIG_JD_USE_SCALE 1
#define CONFIG_JD_TBLCLIP 1
#define CONFIG_JD_FASTDECODE 1
