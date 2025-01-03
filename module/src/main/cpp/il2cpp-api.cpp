#include "il2cpp-api.h"
#include "xdl.h"
#include "log.h"

// 定义所有 IL2CPP API 函数指针
#define DO_API(r, n, p) r (*n) p;
#include "il2cpp-api-functions.h"
#include "il2cpp-array-functions.h"
#undef DO_API

// 初始化函数
void do_init_il2cpp_api(void* handle) {
    LOGI("Initializing IL2CPP API functions...");

    // 加载所有函数
    #define DO_API(r, n, p) \
        n = (decltype(n))xdl_sym(handle, #n, nullptr); \
        if (!n) { \
            LOGW("Failed to load %s", #n); \
        }

    #include "il2cpp-api-functions.h"
    #undef DO_API

    LOGI("IL2CPP API functions initialized");
} 