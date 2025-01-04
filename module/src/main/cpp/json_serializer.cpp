#include "json_serializer.h"
#include "config_parser.h"
#include "il2cpp-api.h"
#include <string>
#include "il2cpp-tabledefs.h"
#include "log.h"

std::string Utf16ToUtf8(const Il2CppChar* utf16Str, int utf16Len) {
    std::string utf8Str;
    for (int i = 0; i < utf16Len; i++) {
        Il2CppChar ch = utf16Str[i];
        if (ch <= 0x7F) {
            utf8Str += static_cast<char>(ch);
        } else if (ch <= 0x7FF) {
            utf8Str += static_cast<char>(0xC0 | (ch >> 6));
            utf8Str += static_cast<char>(0x80 | (ch & 0x3F));
        } else {
            utf8Str += static_cast<char>(0xE0 | (ch >> 12));
            utf8Str += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
            utf8Str += static_cast<char>(0x80 | (ch & 0x3F));
        }
    }
    return utf8Str;
}

void JsonSerializer::SerializeObject(std::ofstream& outFile, Il2CppObject* obj, int indent) {
    // LOGIF("开始序列化对象");
    
    if (!obj) {
        LOGEF("对象为空，写入 null");
        outFile << "null";
        return;
    }

    auto klass = il2cpp_object_get_class(obj);
    const char* className = il2cpp_class_get_name(klass);
    // LOGIF("序列化类型: %s", className);

    if (strcmp(className, "String") == 0) {
        // LOGIF("序列化字符串对象");
        Il2CppString* strObj = (Il2CppString*)obj;
        const Il2CppChar* utf16Str = il2cpp_string_chars(strObj);
        int utf16Len = il2cpp_string_length(strObj);
        std::string utf8Str = Utf16ToUtf8(utf16Str, utf16Len);
        outFile << "\"" << utf8Str << "\"";
        // LOGIF("字符串序列化完成，长度: %d", utf16Len);
        return;
    }

    outFile << "{\n";
    std::string indentStr(indent + 2, ' ');
    void* iter = nullptr;
    FieldInfo* field;
    bool firstField = true;

    // LOGIF("开始序列化字段");
    while ((field = il2cpp_class_get_fields(klass, &iter)) != nullptr) {
        if (il2cpp_field_get_flags(field) & FIELD_ATTRIBUTE_STATIC) {
            // LOGIF("跳过静态字段: %s", il2cpp_field_get_name(field));
            continue;
        }

        if (!firstField) {
            outFile << ",\n";
        }
        firstField = false;

        const char* fieldName = il2cpp_field_get_name(field);
        // LOGIF("序列化字段: %s", fieldName);
        outFile << indentStr << "\"" << fieldName << "\": ";

        JsonSerializer::SerializeField(outFile, obj, field, indent + 2);
    }

    outFile << "\n" << std::string(indent, ' ') << "}";
    // LOGIF("对象序列化完成");
}

void JsonSerializer::SerializeField(std::ofstream& outFile, Il2CppObject* obj, FieldInfo* field, int indent) {
    // LOGIF("开始序列化字段");
    
    const Il2CppType* fieldType = il2cpp_field_get_type(field);
    const char* typeName = il2cpp_type_get_name(fieldType);
    auto fieldClass = il2cpp_class_from_type(fieldType);
    
    // LOGIF("字段类型: %s", typeName);

    if (fieldType->type == IL2CPP_TYPE_SZARRAY || fieldType->type == IL2CPP_TYPE_ARRAY) {
        // LOGIF("序列化数组字段");
        Il2CppArray* value = nullptr;
        il2cpp_field_get_value(obj, field, &value);
        JsonSerializer::SerializeArray(outFile, value, indent);
        // LOGIF("数组字段序列化完成");
    } else if (il2cpp_class_is_enum(fieldClass) || il2cpp_class_is_valuetype(fieldClass) || strcmp(typeName, "System.String") == 0) {
        // LOGIF("序列化值类型/枚举/字符串字段");
        void* fieldAddr = (char*)obj + il2cpp_field_get_offset(field);
        JsonSerializer::SerializeFieldValue(outFile, field, fieldAddr, indent);
        // LOGIF("值类型字段序列化完成");
    } else {
        // LOGIF("序列化对象字段");
        Il2CppObject* value = nullptr;
        il2cpp_field_get_value(obj, field, &value);
        JsonSerializer::SerializeObject(outFile, value, indent);
        // LOGIF("对象字段序列化完成");
    }
    
    // LOGIF("字段序列化完成");
}

void JsonSerializer::SerializeFieldValue(std::ofstream& outFile, FieldInfo* field, void* fieldAddr, int indent) {
    const Il2CppType* fieldType = il2cpp_field_get_type(field);
    const char* typeName = il2cpp_type_get_name(fieldType);
    auto fieldClass = il2cpp_class_from_type(fieldType);

    // 处理数值类型
    if(strcmp(typeName, "System.Single") == 0) {
        float value = *(float*)fieldAddr;
        outFile << value;
    }
    else if(strcmp(typeName, "System.Double") == 0 || strcmp(typeName, "System.Decimal") == 0) {
        double value = *(double*)fieldAddr;
        outFile << value;
    }
    else if(strcmp(typeName, "System.Int32") == 0) {
        int32_t value = *(int32_t*)fieldAddr;
        outFile << value;
    }
    else if(strcmp(typeName, "System.UInt32") == 0) {
        uint32_t value = *(uint32_t*)fieldAddr;
        outFile << value;
    }
    else if(strcmp(typeName, "System.Int64") == 0) {
        int64_t value = *(int64_t*)fieldAddr;
        outFile << value;
    }
    else if(strcmp(typeName, "System.UInt64") == 0) {
        uint64_t value = *(uint64_t*)fieldAddr;
        outFile << value;
    }
    else if(strcmp(typeName, "System.Int16") == 0) {
        int16_t value = *(int16_t*)fieldAddr;
        outFile << value;
    }
    else if(strcmp(typeName, "System.UInt16") == 0) {
        uint16_t value = *(uint16_t*)fieldAddr;
        outFile << value;
    }
    else if(strcmp(typeName, "System.Byte") == 0) {
        uint8_t value = *(uint8_t*)fieldAddr;
        outFile << (int)value;
    }
    else if(strcmp(typeName, "System.SByte") == 0) {
        int8_t value = *(int8_t*)fieldAddr;
        outFile << (int)value;
    }
    // 处理布尔类型
    else if (strcmp(typeName, "System.Boolean") == 0) {
        bool value = *(bool*)fieldAddr;
        outFile << (value ? "true" : "false");
    }
    // 处理字符串类型 
    else if (strcmp(typeName, "System.String") == 0) {
        Il2CppString* value = *(Il2CppString**)fieldAddr;
        outFile << "\"";
        if (value) {
            const Il2CppChar* utf16Str = il2cpp_string_chars(value);
            int utf16Len = il2cpp_string_length(value);
            outFile << Utf16ToUtf8(utf16Str, utf16Len);
        }
        outFile << "\"";
    }
    // 处理枚举类型
    else if (il2cpp_class_is_enum(fieldClass)) {
        uint64_t value = 0;
        memcpy(&value, fieldAddr, il2cpp_class_value_size(fieldClass, nullptr));
        outFile << value;
    }
}

void JsonSerializer::SerializeArray(std::ofstream& outFile, Il2CppArray* arr, int indent) {
    // LOGIF("开始序列化数组");
    if (!arr) {
        LOGEF("数组为空，写入 null");
        outFile << "null";
        return;
    }

    auto elementClass = il2cpp_class_get_element_class(il2cpp_object_get_class((Il2CppObject*)arr));
    int32_t length = il2cpp_array_length(arr);
    // LOGIF("数组长度: %d, 元素类型: %s", length, il2cpp_class_get_name(elementClass));

    outFile << "[\n";
    std::string indentStr(indent + 2, ' ');

    // 计算数组元素的大小
    size_t elementSize = il2cpp_class_value_size(elementClass, nullptr);
    if (!il2cpp_class_is_valuetype(elementClass)) {
        elementSize = sizeof(Il2CppObject*);
    }

    // 获取数组数据的起始地址
    char* arrayData = ((char*)arr) + kIl2CppSizeOfArray;

    for (int32_t i = 0; i < length; i++) {
        // LOGIF("序列化数组元素 [%d/%d]", i + 1, length);
        outFile << indentStr;
        
        // 计算当前元素的地址
        void* elementAddr = arrayData + (i * elementSize);
        
        if (il2cpp_class_is_valuetype(elementClass) || il2cpp_class_is_enum(elementClass)) {
            // LOGIF("元素是值类型或枚举类型");
            JsonSerializer::SerializeValueType(outFile, elementClass, elementAddr, indent + 2);
        } else {
            // LOGIF("元素是引用类型");
            Il2CppObject* elementObj = *(Il2CppObject**)elementAddr;
            JsonSerializer::SerializeObject(outFile, elementObj, indent + 2);
        }

        if (i < length - 1) {
            outFile << ",";
        }
        outFile << "\n";
    }

    outFile << std::string(indent, ' ') << "]";
    // LOGIF("数组序列化完成");
}

void JsonSerializer::SerializeValueType(std::ofstream& outFile, Il2CppClass* klass, void* value, int indent) {
    // LOGIF("开始序列化值类型");
    // LOGIF("类型: %s", il2cpp_class_get_name(klass));

    if (il2cpp_class_is_enum(klass)) {
        // LOGIF("序列化枚举类型");
        uint64_t enumValue = 0;
        memcpy(&enumValue, value, il2cpp_class_value_size(klass, nullptr));
        outFile << enumValue;
        // LOGIF("枚举值: %" PRIu64, enumValue);
        return;
    }

    outFile << "{\n";
    std::string indentStr(indent + 2, ' ');
    
    void* iter = nullptr;
    FieldInfo* field;
    bool firstField = true;

    // LOGIF("开始遍历字段");
    while ((field = il2cpp_class_get_fields(klass, &iter)) != nullptr) {
        if (il2cpp_field_get_flags(field) & FIELD_ATTRIBUTE_STATIC) {
            // LOGIF("跳过静态字段");
            continue;
        }

        if (!firstField) {
            outFile << ",\n";
        }
        firstField = false;

        const char* fieldName = il2cpp_field_get_name(field);
        // LOGIF("序列化字段: %s", fieldName);
        outFile << indentStr << "\"" << fieldName << "\": ";

        size_t offset = il2cpp_field_get_offset(field);
        void* fieldAddr = (char*)value + offset;
        JsonSerializer::SerializeFieldValue(outFile, field, fieldAddr, indent + 2);
    }

    outFile << "\n" << std::string(indent, ' ') << "}";
    // LOGIF("值类型序列化完成");
} 