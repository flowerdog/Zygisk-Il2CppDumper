#ifndef UTIL_H
#define UTIL_H

#include <string>

namespace util {
    // 确保目录存在，如果不存在则创建
    // 支持多级目录创建，例如 "/data/data/com.example/files/config/json"
    // 返回值：true 表示目录已存在或创建成功，false 表示创建失败
    bool EnsureDirectoryExists(const std::string& path);
}

#endif // UTIL_H 