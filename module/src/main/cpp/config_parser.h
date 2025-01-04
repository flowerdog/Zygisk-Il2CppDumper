#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <cstdio>
#include <string>
#include <stdint.h>
#include "il2cpp-class.h"

// 文件头结构
struct ResFileHead {
    uint32_t tag;      // 固定：0xEF2D0000
    uint32_t len;      // 整个文件的大小
    uint32_t version;  // 可以忽略
    uint32_t resnum;   // 数据条数
    uint32_t crc32;    // crc 可以忽略
};

class ConfigParser {
public:
    static bool ParseEquipConfig(const char* inputPath, const char* outputPath);

private:
    static bool ReadFileHead(FILE* fp, ResFileHead& head);
    static bool ReadEquipConfig(FILE* fp, uint32_t count);
    static Il2CppClass* GetEquipConfigClass();
    static Il2CppObject* CreateEquipConfigObject();
    static bool UnpackEquipConfig(Il2CppObject* equipConfig, const uint8_t* data, size_t dataLen);
    static bool SaveAsJson(const char* outputPath, Il2CppArray* configArray);
};

#endif // CONFIG_PARSER_H 