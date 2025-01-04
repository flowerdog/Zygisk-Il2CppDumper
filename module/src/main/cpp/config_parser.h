#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <cstdio>
#include <string>
#include <stdint.h>
#include <fstream>
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
    // 基础工具函数
    static std::string Utf16ToUtf8(const Il2CppChar* utf16Str, int utf16Len);
    static Il2CppClass* FindClass(const char* assemblyName, const char* namespaze, const char* className);
    
    // 序列化相关函数（按依赖顺序排列）
    static void SerializeFieldValue(std::ofstream& outFile, FieldInfo* field, void* fieldAddr, int indent = 0);
    static void SerializeValueType(std::ofstream& outFile, Il2CppClass* klass, void* value, int indent = 0);
    static void SerializeField(std::ofstream& outFile, Il2CppObject* obj, FieldInfo* field, int indent = 0);
    static void SerializeArray(std::ofstream& outFile, Il2CppArray* arr, int indent = 0);
    static void SerializeObject(std::ofstream& outFile, Il2CppObject* obj, int indent = 0);
    
    // 配置解析相关函数
    static bool ReadFileHead(FILE* fp, ResFileHead& head);
    static bool ReadEquipConfig(FILE* fp, uint32_t count);
    static Il2CppClass* GetEquipConfigClass();
    static Il2CppObject* CreateEquipConfigObject();
    static bool UnpackEquipConfig(Il2CppObject* equipConfig, const uint8_t* data, size_t dataLen);
    static bool SaveAsJson(const char* outputPath, Il2CppArray* configArray);
    static void PrintEquipConfigFields(Il2CppObject* equipConfig, const char* prefix);
};

#endif // CONFIG_PARSER_H 