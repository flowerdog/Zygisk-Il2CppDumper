#ifndef IL2CPP_API_H
#define IL2CPP_API_H

#include <cinttypes>
#include "il2cpp-class.h"

// 声明所有 IL2CPP API 函数指针
#define DO_API(r, n, p) extern r (*n) p;
#include "il2cpp-api-functions.h"
#include "il2cpp-array-functions.h"
#undef DO_API

// 初始化函数
void do_init_il2cpp_api(void* handle);

#endif // IL2CPP_API_H 