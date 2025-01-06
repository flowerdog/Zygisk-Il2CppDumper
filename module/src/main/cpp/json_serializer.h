#ifndef JSON_SERIALIZER_H
#define JSON_SERIALIZER_H

#include <fstream>
#include "il2cpp-class.h"
#include <string>

class JsonSerializer {
private:
    static std::string Utf16ToUtf8(const Il2CppChar* utf16Str, int utf16Len);
public:
    static void SerializeObject(std::ofstream& outFile, Il2CppObject* obj, int indent);
    static void SerializeField(std::ofstream& outFile, Il2CppObject* obj, FieldInfo* field, int indent);
    static void SerializeFieldValue(std::ofstream& outFile, FieldInfo* field, void* fieldAddr, int indent);
    static void SerializeArray(std::ofstream& outFile, Il2CppArray* arr, int indent);
    static void SerializeValueType(std::ofstream& outFile, Il2CppClass* klass, void* value, int indent);
};

#endif // JSON_SERIALIZER_H 