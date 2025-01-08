#ifndef JSON_SERIALIZER_H
#define JSON_SERIALIZER_H

#include <fstream>
#include <string>
#include "il2cpp-class.h"
#include "il2cpp-api.h"

class JsonSerializer {
private:
    // 基础类型枚举
    enum class BasicType {
        None,
        Int8,       // System.SByte
        UInt8,      // System.Byte
        Int16,      // System.Int16
        UInt16,     // System.UInt16
        Int32,      // System.Int32
        UInt32,     // System.UInt32
        Int64,      // System.Int64
        UInt64,     // System.UInt64
        Float,      // System.Single
        Double,     // System.Double
        Boolean     // System.Boolean
    };

    // 基础工具函数
    static std::string Utf16ToUtf8(const Il2CppChar* utf16Str, int utf16Len);
    static std::string GetBasicType(const Il2CppType* type);
    
    // 类型判断辅助函数
    static bool IsString(const Il2CppClass* klass);
    static bool IsFP(const Il2CppClass* klass);
    static bool IsBasicType(const Il2CppType* type);
    static bool IsArray(const Il2CppType* type);
    
    // 特殊类型处理
    static float GetFPValue(void* fpValue, Il2CppClass* klass);
    static void SerializeBasicType(std::ofstream& outFile, const std::string& typeName, void* value);
    static void SerializeString(std::ofstream& outFile, Il2CppString* str);

public:
    // 主要序列化函数
    static void SerializeObject(std::ofstream& outFile, Il2CppObject* obj, int indent);
    static void SerializeField(std::ofstream& outFile, Il2CppObject* obj, FieldInfo* field, int indent);
    static void SerializeFieldValue(std::ofstream& outFile, FieldInfo* field, void* fieldAddr, int indent);
    static void SerializeArray(std::ofstream& outFile, Il2CppArray* arr, int indent);
    static void SerializeValueType(std::ofstream& outFile, Il2CppClass* klass, void* value, int indent);
};

#endif // JSON_SERIALIZER_H 