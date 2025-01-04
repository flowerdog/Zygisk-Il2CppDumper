#pragma once

#include <stdint.h>

// IL2CPP 数组头部大小
#define kIl2CppSizeOfArray 0x20

#ifndef ZYGISK_IL2CPP_DUMPER_IL2CPP_API_H
#define ZYGISK_IL2CPP_DUMPER_IL2CPP_API_H

#include <cinttypes>
#include "il2cpp-class.h"

// 声明所有 IL2CPP API 函数指针
#define DO_API(r, n, p) extern r (*n) p;
#include "il2cpp-api-functions.h"
#undef DO_API

// 初始化函数
bool do_init_il2cpp_api(void* handle);

#endif //ZYGISK_IL2CPP_DUMPER_IL2CPP_API_H 