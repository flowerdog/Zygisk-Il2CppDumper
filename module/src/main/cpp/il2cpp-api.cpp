#include "il2cpp-api.h"
#include "log.h"
#include <dlfcn.h>
#include <vector>
#include <string>
#include "xdl.h"

// 定义所有 IL2CPP API 函数指针
#define DO_API(r, n, p) r (*n) p = nullptr;
#include "il2cpp-api-functions.h"
#undef DO_API

bool do_init_il2cpp_api(void* handle) {
    if (!handle) {
        LOGE("无效的 IL2CPP handle");
        return false;
    }
    
    LOGI("开始初始化 IL2CPP API 函数...");

    auto has_missing_functions = false;
    
    // 初始化所有 IL2CPP API 函数指针
#define DO_API(r, n, p) n = (r (*) p)xdl_sym(handle, #n, nullptr); \
    if (!n) { \
        has_missing_functions = true; \
        LOGE("找不到函数: %s", #n); \
    }
#include "il2cpp-api-functions.h"
#undef DO_API

    if (has_missing_functions) {
        return false;
    }

    LOGI("IL2CPP API 函数初始化完成");
    return true;
} 