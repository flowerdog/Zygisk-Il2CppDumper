#include "util.h"
#include "log.h"
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

namespace util {

bool EnsureDirectoryExists(const std::string& path) {
    // 如果目录已存在，直接返回成功
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return true;
        }
        LOGEF("路径已存在但不是目录: %s", path.c_str());
        return false;
    }

    // 创建所有必需的父目录
    size_t pos = 1;  // 跳过开头的 '/'
    while ((pos = path.find('/', pos)) != std::string::npos) {
        std::string subPath = path.substr(0, pos);
        if (stat(subPath.c_str(), &st) != 0) {
            // 目录不存在，创建它
            if (mkdir(subPath.c_str(), 0755) != 0 && errno != EEXIST) {
                LOGEF("创建目录失败: %s (errno: %d, %s)", 
                      subPath.c_str(), errno, strerror(errno));
                return false;
            }
        } else if (!S_ISDIR(st.st_mode)) {
            // 路径存在但不是目录
            LOGEF("路径已存在但不是目录: %s", subPath.c_str());
            return false;
        }
        pos++;
    }

    // 创建最终目录
    if (mkdir(path.c_str(), 0755) != 0 && errno != EEXIST) {
        LOGEF("创建目录失败: %s (errno: %d, %s)", 
              path.c_str(), errno, strerror(errno));
        return false;
    }

    LOGIF("成功创建目录: %s", path.c_str());
    return true;
}

} // namespace util 