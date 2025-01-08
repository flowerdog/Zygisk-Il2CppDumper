#include "json_serializer.h"
#include "log.h"
#include <string>
#include <unordered_map>
#include "il2cpp-class.h"
#include "il2cpp-api.h"
#include "il2cpp-tabledefs.h"

/**
 * 将UTF16字符串转换为UTF8字符串
 * @param utf16Str UTF16字符串指针
 * @param utf16Len UTF16字符串长度
 * @return UTF8编码的字符串
 */
std::string JsonSerializer::Utf16ToUtf8(const Il2CppChar* utf16Str, int utf16Len) {
    if (!utf16Str || utf16Len <= 0) {
        return "";
    }

    std::string utf8Str;
    utf8Str.reserve(utf16Len * 3); // 预分配空间，UTF-8 最多需要 UTF-16 的 3 倍空间

    for (int i = 0; i < utf16Len; i++) {
        uint16_t ch = utf16Str[i];
        
        if (ch < 0x80) {
            // ASCII 字符
            utf8Str.push_back(static_cast<char>(ch));
        }
        else if (ch < 0x800) {
            // 2 字节 UTF-8
            utf8Str.push_back(static_cast<char>(0xC0 | (ch >> 6)));
            utf8Str.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
        }
        else {
            // 检查是否是代理对
            if (ch >= 0xD800 && ch <= 0xDBFF && i + 1 < utf16Len) {
                uint16_t ch2 = utf16Str[i + 1];
                if (ch2 >= 0xDC00 && ch2 <= 0xDFFF) {
                    // 4 字节字符
                    uint32_t codepoint = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000;
                    utf8Str.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
                    utf8Str.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
                    utf8Str.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                    utf8Str.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                    i++; // 跳过下一个代理字符
                    continue;
                }
            }
            // 3 字节 UTF-8
            utf8Str.push_back(static_cast<char>(0xE0 | (ch >> 12)));
            utf8Str.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
            utf8Str.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
        }
    }

    return utf8Str;
}

/**
 * 判断类是否为字符串类型
 * @param klass Il2Cpp类指针
 * @return 是否为字符串类型
 */
bool JsonSerializer::IsString(const Il2CppClass* klass) {
    return strcmp(il2cpp_class_get_name((Il2CppClass*)klass), "String") == 0;
}

/**
 * 判断类是否为FP类型
 * @param klass Il2Cpp类指针
 * @return 是否为FP类型
 */
bool JsonSerializer::IsFP(const Il2CppClass* klass) {
    return strcmp(il2cpp_class_get_name((Il2CppClass*)klass), "FP") == 0;
}

/**
 * 获取基础类型名称
 * @param type Il2Cpp类型指针
 * @return 基础类型名称,如果不是基础类型则返回空字符串
 */
std::string JsonSerializer::GetBasicType(const Il2CppType* type) {
    if (!type) {
        LOGEF("!!!类型指针为空");
        return "";
    }

    const char* typeName = il2cpp_type_get_name(type);
    if (!typeName) {
        LOGEF("!!!获取类型名称失败");
        return "";
    }

    // 如果是System开头的基础类型,直接返回类型名
    if (strncmp(typeName, "System.", 7) == 0) {
        const char* basicTypes[] = {
            "System.Single", "System.Double", "System.Int32", "System.UInt32",
            "System.Int64", "System.UInt64", "System.Int16", "System.UInt16",
            "System.SByte", "System.Byte", "System.Boolean"
        };
        
        for (const char* basicType : basicTypes) {
            if (strcmp(typeName, basicType) == 0) {
                return typeName;
            }
        }
    }
    return "";
}

/**
 * 判断是否为基础类型
 * @param type Il2Cpp类型指针
 * @return 是否为基础类型
 */
bool JsonSerializer::IsBasicType(const Il2CppType* type) {
    return !GetBasicType(type).empty();
}

/**
 * 判断是否为数组类型
 * @param type Il2Cpp类型指针
 * @return 是否为数组类型
 */
bool JsonSerializer::IsArray(const Il2CppType* type) {
    return type->type == IL2CPP_TYPE_SZARRAY || type->type == IL2CPP_TYPE_ARRAY;
}

/**
 * 获取FP类型的浮点值
 * @param fpValue FP对象指针
 * @param klass FP类指针
 * @return 浮点值
 */
float JsonSerializer::GetFPValue(void* fpValue, Il2CppClass* klass) {
    const MethodInfo* asFloatMethod = il2cpp_class_get_method_from_name(klass, "AsFloat", 0);
    if (!asFloatMethod) {
        LOGEF("找不到FP.AsFloat方法");
        return 0.0f;
    }

    Il2CppException* exc = nullptr;
    float result = *(float*)il2cpp_runtime_invoke(asFloatMethod, fpValue, nullptr, &exc);
    if (exc) {
        LOGEF("调用FP.AsFloat失败");
        return 0.0f;
    }
    return result;
}

/**
 * 序列化基础类型值
 * @param outFile 输出文件流
 * @param typeName 类型名称
 * @param value 值指针
 */
void JsonSerializer::SerializeBasicType(std::ofstream& outFile, const std::string& typeName, void* value) {
    if (typeName == "System.SByte") {
        outFile << (int)*(int8_t*)value;
    }
    else if (typeName == "System.Byte") {
        outFile << (int)*(uint8_t*)value;
    }
    else if (typeName == "System.Int16") {
        outFile << *(int16_t*)value;
    }
    else if (typeName == "System.UInt16") {
        outFile << *(uint16_t*)value;
    }
    else if (typeName == "System.Int32") {
        outFile << *(int32_t*)value;
    }
    else if (typeName == "System.UInt32") {
        outFile << *(uint32_t*)value;
    }
    else if (typeName == "System.Int64") {
        outFile << *(int64_t*)value;
    }
    else if (typeName == "System.UInt64") {
        outFile << *(uint64_t*)value;
    }
    else if (typeName == "System.Single") {
        outFile << *(float*)value;
    }
    else if (typeName == "System.Double") {
        outFile << *(double*)value;
    }
    else if (typeName == "System.Boolean") {
        outFile << (*(bool*)value ? "true" : "false");
    }
    else {
        LOGEF("未知的基础类型: %s", typeName.c_str());
        outFile << "null";
    }
}

/**
 * 序列化字符串
 * @param outFile 输出文件流
 * @param str Il2Cpp字符串指针
 */
void JsonSerializer::SerializeString(std::ofstream& outFile, Il2CppString* str) {
    outFile << "\"";
    if (str) {
        const Il2CppChar* utf16Str = il2cpp_string_chars(str);
        int utf16Len = il2cpp_string_length(str);
        outFile << Utf16ToUtf8(utf16Str, utf16Len);
    }
    outFile << "\"";
}

/**
 * 序列化对象
 * @param outFile 输出文件流
 * @param obj Il2Cpp对象指针
 * @param indent 缩进层级
 */
void JsonSerializer::SerializeObject(std::ofstream& outFile, Il2CppObject* obj, int indent) {
    if (!obj) {
        outFile << "null";
        return;
    }

    auto klass = il2cpp_object_get_class(obj);
    
    // 处理字符串类型
    if (IsString(klass)) {
        SerializeString(outFile, (Il2CppString*)obj);
        return;
    }

    // 处理普通对象
    outFile << "{\n";
    std::string indentStr(indent + 2, ' ');
    void* iter = nullptr;
    bool firstField = true;

    // LOGIF("SerializeObject: klass: %s", il2cpp_class_get_name(klass));
    while (FieldInfo* field = il2cpp_class_get_fields(klass, &iter)) {
        // 跳过静态字段和非public字段
        uint32_t flags = il2cpp_field_get_flags(field);
        if ((flags & FIELD_ATTRIBUTE_STATIC) || !(flags & FIELD_ATTRIBUTE_PUBLIC)) {
            continue;
        }

        // LOGIF("SerializeObject: field: %s", il2cpp_field_get_name(field));

        if (!firstField) {
            outFile << ",\n";
        }
        firstField = false;

        outFile << indentStr << "\"" << il2cpp_field_get_name(field) << "\": ";
        SerializeField(outFile, obj, field, indent + 2);
    }

    outFile << "\n" << std::string(indent, ' ') << "}";
}

/**
 * 序列化字段
 * @param outFile 输出文件流
 * @param obj 对象指针
 * @param field 字段信息
 * @param indent 缩进层级
 */
void JsonSerializer::SerializeField(std::ofstream& outFile, Il2CppObject* obj, FieldInfo* field, int indent) {
    // LOGIF("SerializeField: name: %s, type: %s", il2cpp_field_get_name(field), il2cpp_type_get_name(il2cpp_field_get_type(field)));

    const Il2CppType* fieldType = il2cpp_field_get_type(field);
    void* fieldAddr = (char*)obj + il2cpp_field_get_offset(field);
    SerializeFieldValue(outFile, field, fieldAddr, indent);
}

/**
 * 序列化字段值
 * @param outFile 输出文件流
 * @param field 字段信息
 * @param fieldAddr 字段地址
 * @param indent 缩进层级
 */
void JsonSerializer::SerializeFieldValue(std::ofstream& outFile, FieldInfo* field, void* fieldAddr, int indent) {

    // LOGIF("SerializeFieldValue: name: %s, type: %s", il2cpp_field_get_name(field), il2cpp_type_get_name(il2cpp_field_get_type(field)));

    const Il2CppType* fieldType = il2cpp_field_get_type(field);
    auto fieldClass = il2cpp_class_from_type(fieldType);

    // 处理 FP 类型
    if (IsFP(fieldClass)) {
        // LOGIF("SerializeFieldValue: FP");
        outFile << GetFPValue(fieldAddr, fieldClass);
        return;
    }

    // 处理基础类型
    std::string basicType = GetBasicType(fieldType);
    if (!basicType.empty()) {
        // LOGIF("SerializeFieldValue: BasicType: %s", basicType.c_str());
        SerializeBasicType(outFile, basicType, fieldAddr);
        return;
    }

    // 处理字符串类型
    if (IsString(fieldClass)) {
        // LOGIF("SerializeFieldValue: String");
        SerializeString(outFile, *(Il2CppString**)fieldAddr);
        return;
    }

    // 处理数组类型
    if (IsArray(fieldType)) {
        // LOGIF("SerializeFieldValue: Array");
        Il2CppArray* value = *(Il2CppArray**)fieldAddr;
        SerializeArray(outFile, value, indent);
        return;
    }

    // 处理枚举类型
    if (il2cpp_class_is_enum(fieldClass)) {
        // LOGIF("SerializeFieldValue: Enum");
        uint64_t enumValue = 0;
        memcpy(&enumValue, fieldAddr, il2cpp_class_value_size(fieldClass, nullptr)); // 修复:使用 fieldAddr 而不是未定义的 value
        outFile << enumValue;
        return;
    }

    // 处理值类型
    if (il2cpp_class_is_valuetype(fieldClass)) {
        // LOGIF("SerializeFieldValue: ValueType");
        SerializeValueType(outFile, fieldClass, fieldAddr, indent);
        return;
    }

    // LOGIF("SerializeFieldValue: Object");
    // 处理对象类型 - fieldAddr 已经是指向对象指针的地址
    Il2CppObject* value = *(Il2CppObject**)fieldAddr;
    SerializeObject(outFile, value, indent);
}

/**
 * 序列化数组
 * @param outFile 输出文件流
 * @param arr 数组指针
 * @param indent 缩进层级
 */
void JsonSerializer::SerializeArray(std::ofstream& outFile, Il2CppArray* arr, int indent) {
    if (!arr) {
        outFile << "null";
        return;
    }

    auto elementClass = il2cpp_class_get_element_class(il2cpp_object_get_class((Il2CppObject*)arr));
    int32_t length = il2cpp_array_length(arr);
    size_t elementSize = il2cpp_class_is_valuetype(elementClass) ? 
                        il2cpp_class_value_size(elementClass, nullptr) : 
                        sizeof(Il2CppObject*);

    outFile << "[\n";
    std::string indentStr(indent + 2, ' ');
    char* arrayData = ((char*)arr) + kIl2CppSizeOfArray;

    for (int32_t i = 0; i < length; i++) {
        outFile << indentStr;
        void* elementAddr = arrayData + (i * elementSize);

        if (IsFP(elementClass)) {
            outFile << GetFPValue(elementAddr, elementClass);
        }
        else if (il2cpp_class_is_valuetype(elementClass)) {
            SerializeValueType(outFile, elementClass, elementAddr, indent + 2);
        }
        else {
            SerializeObject(outFile, *(Il2CppObject**)elementAddr, indent + 2);
        }

        if (i < length - 1) {
            outFile << ",";
        }
        outFile << "\n";
    }

    outFile << std::string(indent, ' ') << "]";
}

/**
 * 序列化值类型
 * @param outFile 输出文件流
 * @param klass 类指针
 * @param value 值指针
 * @param indent 缩进层级
 */
void JsonSerializer::SerializeValueType(std::ofstream& outFile, Il2CppClass* klass, void* value, int indent) {
    // 处理 FP 类型
    if (IsFP(klass)) {
        outFile << GetFPValue(value, klass);
        return;
    }

    // 处理枚举类型
    if (il2cpp_class_is_enum(klass)) {
        uint64_t enumValue = 0;
        memcpy(&enumValue, value, il2cpp_class_value_size(klass, nullptr));
        outFile << enumValue;
        return;
    }

    // 处理其他值类型
    outFile << "{\n";
    std::string indentStr(indent + 2, ' ');
    void* iter = nullptr;
    bool firstField = true;

    while (FieldInfo* field = il2cpp_class_get_fields(klass, &iter)) {
        if (il2cpp_field_get_flags(field) & FIELD_ATTRIBUTE_STATIC) {
            continue;
        }

        if (!firstField) {
            outFile << ",\n";
        }
        firstField = false;

        outFile << indentStr << "\"" << il2cpp_field_get_name(field) << "\": ";
        void* fieldAddr = (char*)value + il2cpp_field_get_offset(field);
        SerializeFieldValue(outFile, field, fieldAddr, indent + 2);
    }

    outFile << "\n" << std::string(indent, ' ') << "}";
}