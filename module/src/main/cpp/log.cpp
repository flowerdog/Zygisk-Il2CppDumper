#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

void ensure_directory_exists(const char* path) {
    char dir_path[256] = {0};
    strncpy(dir_path, path, sizeof(dir_path) - 1);
    
    // 找到最后一个'/'
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';  // 暂时截断路径
        
        // 递归创建目录
        char* p = dir_path;
        while (*p) {
            if (*p == '/') {
                *p = '\0';
                if (access(dir_path, F_OK) != 0) {
                    mkdir(dir_path, 0755);
                }
                *p = '/';
            }
            p++;
        }
        if (access(dir_path, F_OK) != 0) {
            mkdir(dir_path, 0755);
        }
    }
}

void log_to_file(const char* level, const char* fmt, ...) {
    ensure_directory_exists(LOG_FILE_PATH);
    
    FILE* fp = fopen(LOG_FILE_PATH, "a");
    if (!fp) {
        LOGE("Failed to open log file: %s, error: %s", LOG_FILE_PATH, strerror(errno));
        return;
    }

    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    // 写入时间戳和日志级别
    fprintf(fp, "[%s][%s] ", timestamp, level);

    // 写入实际日志内容
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fprintf(fp, "\n");

    fclose(fp);
} 