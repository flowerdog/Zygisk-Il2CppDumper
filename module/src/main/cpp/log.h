//
// Created by Perfare on 2020/7/4.
//

#ifndef ZYGISK_IL2CPPDUMPER_LOG_H
#define ZYGISK_IL2CPPDUMPER_LOG_H

#include <android/log.h>
#include "game.h"
#include <string>   

#define LOG_TAG "Perfare"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOG_FILE_PATH (std::string("/sdcard/Android/data/") + GamePackageName + "/files/dump_logs.txt")

void log_to_file(const char* level, const char* fmt, ...);

#define LOGDF(fmt, ...) do { \
    LOGD(fmt, ##__VA_ARGS__); \
    log_to_file("DEBUG", fmt, ##__VA_ARGS__); \
} while(0)

#define LOGWF(fmt, ...) do { \
    LOGW(fmt, ##__VA_ARGS__); \
    log_to_file("WARN", fmt, ##__VA_ARGS__); \
} while(0)

#define LOGEF(fmt, ...) do { \
    LOGE(fmt, ##__VA_ARGS__); \
    log_to_file("ERROR", fmt, ##__VA_ARGS__); \
} while(0)

#define LOGIF(fmt, ...) do { \
    LOGI(fmt, ##__VA_ARGS__); \
    log_to_file("INFO", fmt, ##__VA_ARGS__); \
} while(0)

#endif //ZYGISK_IL2CPPDUMPER_LOG_H
