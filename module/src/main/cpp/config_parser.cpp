#include "config_parser.h"
#include "log.h"
#include <vector>
#include <fstream>
#include "il2cpp-api.h"
#include <inttypes.h>

// 添加通用类型查找函数
Il2CppClass* ConfigParser::FindClass(const char* assemblyName, const char* namespaze, const char* className) {
    auto domain = il2cpp_domain_get();
    size_t size;
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);
    Il2CppClass* foundClass = nullptr;
    int foundCount = 0;
    
    // 如果指定了程序集名称，只在指定程序集中查找
    if (assemblyName) {
        LOGIF("在程序集 %s 中查找类型 %s.%s", assemblyName, namespaze ? namespaze : "<any>", className);
        for (size_t i = 0; i < size; i++) {
            auto image = il2cpp_assembly_get_image(assemblies[i]);
            const char* currentAssemblyName = il2cpp_image_get_name(image);
            
            if (strcmp(currentAssemblyName, assemblyName) == 0) {
                if (namespaze) {
                    auto result = il2cpp_class_from_name(image, namespaze, className);
                    if (result) {
                        LOGIF("在程序集 %s 中找到类型 %s.%s", assemblyName, namespaze, className);
                        return result;
                    }
                } else {
                    // 遍历所有类型
                    size_t classCount = il2cpp_image_get_class_count(image);
                    for (size_t j = 0; j < classCount; j++) {
                        Il2CppClass* klass = (Il2CppClass*)il2cpp_image_get_class(image, j);
                        if (strcmp(il2cpp_class_get_name(klass), className) == 0) {
                            foundClass = klass;
                            foundCount++;
                            LOGIF("在程序集 %s 命名空间 %s 中找到类型 %s", 
                                assemblyName, 
                                il2cpp_class_get_namespace(klass), 
                                className);
                        }
                    }
                }
            }
        }
    } else {
        // 遍历所有程序集
        LOGIF("在所有程序集中查找类型 %s", className);
        for (size_t i = 0; i < size; i++) {
            auto image = il2cpp_assembly_get_image(assemblies[i]);
            const char* currentAssemblyName = il2cpp_image_get_name(image);
            
            if (namespaze) {
                auto result = il2cpp_class_from_name(image, namespaze, className);
                if (result) {
                    foundClass = result;
                    foundCount++;
                    LOGIF("在程序集 %s 命名空间 %s 中找到类型 %s", 
                        currentAssemblyName, 
                        namespaze, 
                        className);
                }
            } else {
                // 遍历所有类型
                size_t classCount = il2cpp_image_get_class_count(image);
                for (size_t j = 0; j < classCount; j++) {
                    Il2CppClass* klass = (Il2CppClass*)il2cpp_image_get_class(image, j);
                    if (strcmp(il2cpp_class_get_name(klass), className) == 0) {
                        foundClass = klass;
                        foundCount++;
                        LOGIF("在程序集 %s 命名空间 %s 中找到类型 %s", 
                            currentAssemblyName, 
                            il2cpp_class_get_namespace(klass), 
                            className);
                    }
                }
            }
        }
    }

    if (foundCount > 1) {
        LOGWF("警告：找到多个同名类型 %s，总共 %d 个", className, foundCount);
    }

    if (foundClass) {
        return foundClass;
    }

    LOGEF("未找到类型 %s", className);
    return nullptr;
}

bool ConfigParser::ParseEquipConfig(const char* inputPath, const char* outputPath) {
    LOGIF("开始解析装备配置文件: %s", inputPath);
    
    // 打开输入文件
    FILE* fp = fopen(inputPath, "rb");
    if (!fp) {
        LOGEF("无法打开输入文件: %s", inputPath);
        return false;
    }

    // 读取文件头
    ResFileHead head;
    if (!ReadFileHead(fp, head)) {
        LOGEF("读取文件头失败");
        fclose(fp);
        return false;
    }

    // 验证文件头tag
    if (head.tag != 0x00002DEF) {
        LOGEF("无效的文件头tag: 0x%08X", head.tag);
        fclose(fp);
        return false;
    }

    LOGIF("文件头tag验证成功");

    // 获取剩余数据大小
    size_t dataSize = head.len - 20;  // 文件头大小为20字节
    std::vector<uint8_t> buffer(dataSize);
    
    if (fread(buffer.data(), 1, dataSize, fp) != dataSize) {
        LOGEF("读取文件数据失败");
        fclose(fp);
        return false;
    }
    
    LOGIF("读取文件数据成功");
    fclose(fp);

    // 创建并初始化 PbReadBuf
    auto pbReadBufClass = FindClass("DodProtoBase.dll", "ProtoBase", "PbReadBuf");
    if (!pbReadBufClass) {
        LOGEF("无法获取 PbReadBuf 类");
        return false;
    }
    LOGIF("获取 PbReadBuf 类成功");

    auto readBuf = il2cpp_object_new(pbReadBufClass);
    if (!readBuf) {
        LOGEF("创建 PbReadBuf 对象失败");
        return false;
    }

    LOGIF("创建 PbReadBuf 对象成功");
    // 调用构造函数
    auto ctor = il2cpp_class_get_method_from_name(pbReadBufClass, ".ctor", 0);
    if (!ctor) {
        LOGEF("找不到 PbReadBuf 的构造函数");
        return false;
    }

    LOGIF("获取 PbReadBuf 的构造函数成功");

    Il2CppException* exc = nullptr;
    il2cpp_runtime_invoke(ctor, readBuf, nullptr, &exc);
    if (exc) {
        LOGEF("调用 PbReadBuf 构造函数失败");
        return false;
    }

    LOGIF("调用 PbReadBuf 构造函数成功");

    // 创建字节数组并设置数据
    auto byteArrayClass = il2cpp_array_class_get(
        il2cpp_class_from_name(il2cpp_get_corlib(), "System", "Byte"), 
        1
    );
    LOGIF("获取 Byte 数组类成功");
    
    auto byteArray = il2cpp_array_new(byteArrayClass, dataSize);
    if (!byteArray) {
        LOGEF("创建字节数组失败");
        return false;
    }

    LOGIF("创建字节数组成功");

    // 复制数据到字节数组
    uint8_t* arrayData = (uint8_t*)((char*)byteArray + kIl2CppSizeOfArray);
    memcpy(arrayData, buffer.data(), dataSize);

    LOGIF("复制数据到字节数组成功");

    // 调用 set 方法设置数据
    auto setMethod = il2cpp_class_get_method_from_name(pbReadBufClass, "set", 2);
    if (!setMethod) {
        LOGEF("找不到 PbReadBuf.set 方法");
        return false;
    }

    LOGIF("获取 PbReadBuf.set 方法成功");

    int32_t len = static_cast<int32_t>(dataSize);
    void* setParams[] = { byteArray, &len };
    LOGIF("准备调用 set 方法，参数: byteArray=%p, len=%d", byteArray, len);
    exc = nullptr;
    il2cpp_runtime_invoke(setMethod, readBuf, setParams, &exc);
    if (exc) {
        auto excClass = il2cpp_object_get_class((Il2CppObject*)exc);
        const char* excName = excClass ? il2cpp_class_get_name(excClass) : "Unknown";
        LOGEF("调用 PbReadBuf.set 方法失败: %s", excName);
        return false;
    }

    LOGIF("调用 PbReadBuf.set 方法成功，数据大小: %d 字节", len);
    
    // 检查 readBuf 的一些属性
    auto bufferField = il2cpp_class_get_field_from_name(pbReadBufClass, "buffer");
    auto posField = il2cpp_class_get_field_from_name(pbReadBufClass, "pos");
    auto lenField = il2cpp_class_get_field_from_name(pbReadBufClass, "len");
    
    if (bufferField && posField && lenField) {
        void* buffer = nullptr;
        int32_t pos = 0;
        int32_t bufLen = 0;
        
        il2cpp_field_get_value((Il2CppObject*)readBuf, bufferField, &buffer);
        il2cpp_field_get_value((Il2CppObject*)readBuf, posField, &pos);
        il2cpp_field_get_value((Il2CppObject*)readBuf, lenField, &bufLen);
        
        LOGIF("PbReadBuf 状态: buffer=%p, pos=%d, len=%d", buffer, pos, bufLen);
    }

    // 获取 EquipConfig 类并创建结果数组
    auto equipConfigClass = GetEquipConfigClass();
    if (!equipConfigClass) {
        LOGEF("获取 EquipConfig 类失败");
        return false;
    }

    LOGIF("获取 EquipConfig 类成功");
    
    // 打印类的字段信息
    void* iter = nullptr;
    FieldInfo* field;
    LOGIF("EquipConfig 类的字段列表:");
    while ((field = il2cpp_class_get_fields(equipConfigClass, &iter)) != nullptr) {
        const char* fieldName = il2cpp_field_get_name(field);
        const char* fieldType = il2cpp_type_get_name(il2cpp_field_get_type(field));
        LOGIF("  字段: %s, 类型: %s", fieldName, fieldType);
    }
    
    // 打印类的方法信息
    iter = nullptr;
    const MethodInfo* method;
    LOGIF("EquipConfig 类的方法列表:");
    while ((method = il2cpp_class_get_methods(equipConfigClass, &iter)) != nullptr) {
        const char* methodName = il2cpp_method_get_name(method);
        LOGIF("  方法: %s, 参数个数: %d", methodName, il2cpp_method_get_param_count(method));
    }

    auto configArray = il2cpp_array_new(equipConfigClass, head.resnum);
    if (!configArray) {
        LOGEF("创建配置数组失败");
        return false;
    }
    LOGIF("创建配置数组成功");

    // 循环解析每个配置项
    for (uint32_t i = 0; i < head.resnum; i++) {
        LOGIF("开始解析配置项 [%u/%u]", i + 1, head.resnum);
        // 创建 EquipConfig 对象
        auto equipConfig = il2cpp_object_new(equipConfigClass);
        if (!equipConfig) {
            LOGEF("创建 EquipConfig 对象失败 [%u/%u]", i + 1, head.resnum);
            return false;
        }
        LOGIF("创建 EquipConfig 对象成功");

        // 调用构造函数
        auto ctor = il2cpp_class_get_method_from_name(equipConfigClass, ".ctor", 0);
        if (!ctor) {
            LOGEF("找不到 EquipConfig 构造函数");
            return false;
        }
        exc = nullptr;
        il2cpp_runtime_invoke(ctor, equipConfig, nullptr, &exc);
        if (exc) {
            LOGEF("调用 EquipConfig 构造函数失败");
            return false;
        }

        // 调用 create 方法初始化对象
        auto createMethod = il2cpp_class_get_method_from_name(equipConfigClass, "create", 0);
        if (!createMethod) {
            LOGEF("找不到 EquipConfig.create 方法");
            return false;
        }
        LOGIF("获取 EquipConfig.create 方法成功");

        exc = nullptr;
        il2cpp_runtime_invoke(createMethod, equipConfig, nullptr, &exc);
        if (exc) {
            auto excClass = il2cpp_object_get_class((Il2CppObject*)exc);
            const char* excName = excClass ? il2cpp_class_get_name(excClass) : "Unknown";
            LOGEF("调用 EquipConfig.create 方法失败: %s", excName);
            return false;
        }
        LOGIF("调用 EquipConfig.create 方法成功");

        // 打印 unpack 前的字段值
        PrintEquipConfigFields(equipConfig, "unpack 前的");

        // 调用 unpack 方法
        auto unpackMethod = il2cpp_class_get_method_from_name(equipConfigClass, "unpack", 3);
        if (!unpackMethod) {
            LOGEF("找不到 EquipConfig.unpack 方法");
            return false;
        }
        LOGIF("获取 EquipConfig.unpack 方法成功");

        uint32_t cutVer = 0;
        void* unpackParams[] = { readBuf, &cutVer, nullptr };
        LOGIF("准备调用 EquipConfig.unpack 方法，参数: readBuf=%p, cutVer=%u, stack=%p", readBuf, cutVer, nullptr);
        exc = nullptr;
        il2cpp_runtime_invoke(unpackMethod, equipConfig, unpackParams, &exc);
        if (exc) {
            auto excClass = il2cpp_object_get_class((Il2CppObject*)exc);
            const char* excName = excClass ? il2cpp_class_get_name(excClass) : "Unknown";
            LOGEF("调用 EquipConfig.unpack 方法失败 [%u/%u]: %s", i + 1, head.resnum, excName);
                    // 打印 unpack 后的字段值
            PrintEquipConfigFields(equipConfig, "unpack 后的");
            return false;
        }
        LOGIF("调用 EquipConfig.unpack 方法成功");

        // 打印 unpack 后的字段值
        PrintEquipConfigFields(equipConfig, "unpack 后的");

        // 存入数组
        void** elementAddr = (void**)((char*)configArray + kIl2CppSizeOfArray + i * sizeof(void*));
        *elementAddr = equipConfig;
        LOGIF("成功解析配置项 [%u/%u]", i + 1, head.resnum);
    }

    // 保存为 JSON
    if (!SaveAsJson(outputPath, configArray)) {
        LOGEF("保存 JSON 文件失败");
        return false;
    }

    LOGIF("成功解析所有配置并保存到: %s", outputPath);
    return true;
}

bool ConfigParser::ReadFileHead(FILE* fp, ResFileHead& head) {
    // 读取文件头结构
    ResFileHead raw_head;
    if (fread(&raw_head, sizeof(raw_head), 1, fp) != 1) {
        LOGEF("读取文件头失败");
        return false;
    }
    
    // 由于磁盘低位在内存高字节，需要进行字节序转换
    head.tag = ((raw_head.tag & 0xFF000000) >> 24) |
               ((raw_head.tag & 0x00FF0000) >> 8) |
               ((raw_head.tag & 0x0000FF00) << 8) |
               ((raw_head.tag & 0x000000FF) << 24);
               
    head.len = ((raw_head.len & 0xFF000000) >> 24) |
               ((raw_head.len & 0x00FF0000) >> 8) |
               ((raw_head.len & 0x0000FF00) << 8) |
               ((raw_head.len & 0x000000FF) << 24);
               
    head.version = ((raw_head.version & 0xFF000000) >> 24) |
                  ((raw_head.version & 0x00FF0000) >> 8) |
                  ((raw_head.version & 0x0000FF00) << 8) |
                  ((raw_head.version & 0x000000FF) << 24);
                  
    head.resnum = ((raw_head.resnum & 0xFF000000) >> 24) |
                 ((raw_head.resnum & 0x00FF0000) >> 8) |
                 ((raw_head.resnum & 0x0000FF00) << 8) |
                 ((raw_head.resnum & 0x000000FF) << 24);
                 
    head.crc32 = ((raw_head.crc32 & 0xFF000000) >> 24) |
                ((raw_head.crc32 & 0x00FF0000) >> 8) |
                ((raw_head.crc32 & 0x0000FF00) << 8) |
                ((raw_head.crc32 & 0x000000FF) << 24);
    
    LOGIF("读取到文件头: tag=0x%08X, len=%u, version=%u, resnum=%u, crc32=0x%08X",
          head.tag, head.len, head.version, head.resnum, head.crc32);
          
    return true;
}

//print all types of a iamge
void print_all_types(const Il2CppImage* image) {
    // LOGIF("show type of image: %s", il2cpp_image_get_name(image));
    
    size_t type_count = il2cpp_image_get_class_count(image);
    for (size_t i = 0; i < type_count; i++) {
        Il2CppClass* type = (Il2CppClass*)(il2cpp_image_get_class(image, i));
        // LOGIF("Type: %s.%s", il2cpp_class_get_namespace(type), il2cpp_class_get_name(type));
    }
}


Il2CppClass* ConfigParser::GetEquipConfigClass() {
    // 先尝试精确查找
    auto result = FindClass("GameProto.dll", "ResDef", "EquipConfig");
    if (result) {
        return result;
    }
    
    // 如果精确查找失败，尝试在所有程序集中查找
    LOGIF("精确查找失败，尝试在所有程序集中查找 EquipConfig 类");
    return FindClass(nullptr, nullptr, "EquipConfig");
}

Il2CppObject* ConfigParser::CreateEquipConfigObject() {
    auto equipConfigClass = GetEquipConfigClass();
    if (!equipConfigClass) {
        return nullptr;
    }
    return il2cpp_object_new(equipConfigClass);
}

// 添加新的辅助函数用于序列化对象
void SerializeObject(std::ofstream& outFile, Il2CppObject* obj, int indent = 0) {
    if (!obj) {
        outFile << "null";
        return;
    }

    auto klass = il2cpp_object_get_class(obj);
    void* iter = nullptr;
    FieldInfo* field;
    bool firstField = true;

    outFile << "{\n";
    std::string indentStr(indent + 2, ' ');

    while ((field = il2cpp_class_get_fields(klass, &iter)) != nullptr) {
        if (!firstField) {
            outFile << ",\n";
        }
        firstField = false;

        const char* fieldName = il2cpp_field_get_name(field);
        const Il2CppType* fieldType = il2cpp_field_get_type(field);
        const char* typeName = il2cpp_type_get_name(fieldType);

        outFile << indentStr << "\"" << fieldName << "\": ";
        SerializeField(outFile, obj, field, indent + 2);
    }

    outFile << "\n" << std::string(indent, ' ') << "}";
}

// 添加新的辅助函数用于序列化数组
void SerializeArray(std::ofstream& outFile, Il2CppArray* arr, int indent = 0) {
    if (!arr) {
        outFile << "null";
        return;
    }

    auto elementClass = il2cpp_class_get_element_class(il2cpp_object_get_class((Il2CppObject*)arr));
    int32_t length = il2cpp_array_length(arr);
    
    outFile << "[\n";
    std::string indentStr(indent + 2, ' ');

    for (int32_t i = 0; i < length; i++) {
        outFile << indentStr;
        
        void* elementAddr = il2cpp_array_addr_with_size(arr, il2cpp_class_value_size(elementClass, nullptr), i);
        if (il2cpp_class_is_valuetype(elementClass)) {
            // 值类型
            SerializeValueType(outFile, elementClass, elementAddr, indent + 2);
        } else {
            // 引用类型
            Il2CppObject* elementObj = *(Il2CppObject**)elementAddr;
            if (elementObj) {
                SerializeObject(outFile, elementObj, indent + 2);
            } else {
                outFile << "null";
            }
        }

        if (i < length - 1) {
            outFile << ",";
        }
        outFile << "\n";
    }

    outFile << std::string(indent, ' ') << "]";
}

// 添加新的辅助函数用于序列化值类型
void SerializeValueType(std::ofstream& outFile, Il2CppClass* klass, void* value, int indent = 0) {
    if (il2cpp_class_is_enum(klass)) {
        // 枚举类型作为整数处理
        uint64_t enumValue = 0;
        memcpy(&enumValue, value, il2cpp_class_value_size(klass, nullptr));
        outFile << enumValue;
        return;
    }

    outFile << "{\n";
    std::string indentStr(indent + 2, ' ');
    
    void* iter = nullptr;
    FieldInfo* field;
    bool firstField = true;

    while ((field = il2cpp_class_get_fields(klass, &iter)) != nullptr) {
        // 跳过静态字段
        if (il2cpp_field_get_flags(field) & FIELD_ATTRIBUTE_STATIC) {
            continue;
        }

        if (!firstField) {
            outFile << ",\n";
        }
        firstField = false;

        const char* fieldName = il2cpp_field_get_name(field);
        outFile << indentStr << "\"" << fieldName << "\": ";

        // 计算字段在值类型中的偏移
        size_t offset = il2cpp_field_get_offset(field);
        void* fieldAddr = (char*)value + offset;
        
        SerializeFieldValue(outFile, field, fieldAddr, indent + 2);
    }

    outFile << "\n" << std::string(indent, ' ') << "}";
}

// 添加新的辅助函数用于序列化字段
void SerializeField(std::ofstream& outFile, Il2CppObject* obj, FieldInfo* field, int indent = 0) {
    const Il2CppType* fieldType = il2cpp_field_get_type(field);
    const char* typeName = il2cpp_type_get_name(fieldType);
    auto fieldClass = il2cpp_class_from_type(fieldType);

    if (strcmp(typeName, "System.Int32") == 0 || 
        strcmp(typeName, "System.UInt32") == 0 ||
        strcmp(typeName, "System.Int64") == 0 ||
        strcmp(typeName, "System.UInt64") == 0 ||
        strcmp(typeName, "System.Int16") == 0 ||
        strcmp(typeName, "System.UInt16") == 0 ||
        strcmp(typeName, "System.Byte") == 0 ||
        strcmp(typeName, "System.SByte") == 0) {
        uint64_t value = 0;
        il2cpp_field_get_value(obj, field, &value);
        outFile << value;
    } else if (strcmp(typeName, "System.Boolean") == 0) {
        bool value = false;
        il2cpp_field_get_value(obj, field, &value);
        outFile << (value ? "true" : "false");
    } else if (strcmp(typeName, "System.String") == 0) {
        Il2CppString* value = nullptr;
        il2cpp_field_get_value(obj, field, &value);
        outFile << "\"";
        if (value) {
            const Il2CppChar* utf16Str = il2cpp_string_chars(value);
            int utf16Len = il2cpp_string_length(value);
            std::string utf8Str = Utf16ToUtf8(utf16Str, utf16Len);
            // 转义 JSON 字符串
            for (char c : utf8Str) {
                switch (c) {
                    case '\"': outFile << "\\\""; break;
                    case '\\': outFile << "\\\\"; break;
                    case '\b': outFile << "\\b"; break;
                    case '\f': outFile << "\\f"; break;
                    case '\n': outFile << "\\n"; break;
                    case '\r': outFile << "\\r"; break;
                    case '\t': outFile << "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) < 0x20) {
                            char buf[8];
                            snprintf(buf, sizeof(buf), "\\u%04x", c);
                            outFile << buf;
                        } else {
                            outFile << c;
                        }
                }
            }
        }
        outFile << "\"";
    } else if (il2cpp_class_is_enum(fieldClass)) {
        // 枚举类型作为整数处理
        uint64_t value = 0;
        il2cpp_field_get_value(obj, field, &value);
        outFile << value;
    } else if (il2cpp_class_get_rank(fieldClass) > 0) {
        // 数组类型
        Il2CppArray* value = nullptr;
        il2cpp_field_get_value(obj, field, &value);
        SerializeArray(outFile, value, indent);
    } else if (il2cpp_class_is_valuetype(fieldClass)) {
        // 值类型
        void* value = alloca(il2cpp_class_value_size(fieldClass, nullptr));
        il2cpp_field_get_value(obj, field, value);
        SerializeValueType(outFile, fieldClass, value, indent);
    } else {
        // 引用类型
        Il2CppObject* value = nullptr;
        il2cpp_field_get_value(obj, field, &value);
        if (value) {
            SerializeObject(outFile, value, indent);
        } else {
            outFile << "null";
        }
    }
}

// 添加新的辅助函数用于序列化字段值（用于值类型）
void SerializeFieldValue(std::ofstream& outFile, FieldInfo* field, void* fieldAddr, int indent = 0) {
    const Il2CppType* fieldType = il2cpp_field_get_type(field);
    const char* typeName = il2cpp_type_get_name(fieldType);
    auto fieldClass = il2cpp_class_from_type(fieldType);

    if (strcmp(typeName, "System.Int32") == 0 || 
        strcmp(typeName, "System.UInt32") == 0 ||
        strcmp(typeName, "System.Int64") == 0 ||
        strcmp(typeName, "System.UInt64") == 0 ||
        strcmp(typeName, "System.Int16") == 0 ||
        strcmp(typeName, "System.UInt16") == 0 ||
        strcmp(typeName, "System.Byte") == 0 ||
        strcmp(typeName, "System.SByte") == 0) {
        uint64_t value = 0;
        memcpy(&value, fieldAddr, il2cpp_class_value_size(fieldClass, nullptr));
        outFile << value;
    } else if (strcmp(typeName, "System.Boolean") == 0) {
        bool value = false;
        memcpy(&value, fieldAddr, sizeof(bool));
        outFile << (value ? "true" : "false");
    } else if (strcmp(typeName, "System.String") == 0) {
        Il2CppString* value = *(Il2CppString**)fieldAddr;
        outFile << "\"";
        if (value) {
            const Il2CppChar* utf16Str = il2cpp_string_chars(value);
            int utf16Len = il2cpp_string_length(value);
            std::string utf8Str = Utf16ToUtf8(utf16Str, utf16Len);
            // 转义 JSON 字符串
            for (char c : utf8Str) {
                switch (c) {
                    case '\"': outFile << "\\\""; break;
                    case '\\': outFile << "\\\\"; break;
                    case '\b': outFile << "\\b"; break;
                    case '\f': outFile << "\\f"; break;
                    case '\n': outFile << "\\n"; break;
                    case '\r': outFile << "\\r"; break;
                    case '\t': outFile << "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) < 0x20) {
                            char buf[8];
                            snprintf(buf, sizeof(buf), "\\u%04x", c);
                            outFile << buf;
                        } else {
                            outFile << c;
                        }
                }
            }
        }
        outFile << "\"";
    } else if (il2cpp_class_is_enum(fieldClass)) {
        // 枚举类型作为整数处理
        uint64_t value = 0;
        memcpy(&value, fieldAddr, il2cpp_class_value_size(fieldClass, nullptr));
        outFile << value;
    } else if (il2cpp_class_get_rank(fieldClass) > 0) {
        // 数组类型
        Il2CppArray* value = *(Il2CppArray**)fieldAddr;
        SerializeArray(outFile, value, indent);
    } else if (il2cpp_class_is_valuetype(fieldClass)) {
        // 值类型
        SerializeValueType(outFile, fieldClass, fieldAddr, indent);
    } else {
        // 引用类型
        Il2CppObject* value = *(Il2CppObject**)fieldAddr;
        if (value) {
            SerializeObject(outFile, value, indent);
        } else {
            outFile << "null";
        }
    }
}

bool ConfigParser::SaveAsJson(const char* outputPath, Il2CppArray* configArray) {
    // 创建输出文件
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        LOGEF("无法创建输出文件: %s", outputPath);
        return false;
    }

    // 写入 JSON 数组开始
    outFile << "[\n";

    // 遍历数组并序列化每个对象
    int32_t length = il2cpp_array_length(configArray);
    for (int32_t i = 0; i < length; i++) {
        // 获取数组元素
        void** elementAddr = (void**)((char*)configArray + kIl2CppSizeOfArray + i * sizeof(void*));
        Il2CppObject* item = (Il2CppObject*)*elementAddr;
        if (!item) {
            LOGEF("数组元素为空 [%d/%d]", i + 1, length);
            outFile.close();
            return false;
        }

        outFile << "  ";
        SerializeObject(outFile, item, 2);
        
        if (i < length - 1) {
            outFile << ",";
        }
        outFile << "\n";
    }

    // 写入 JSON 数组结束
    outFile << "]\n";
    outFile.close();

    LOGIF("成功保存 JSON 文件: %s", outputPath);
    return true;
}

void ConfigParser::PrintEquipConfigFields(Il2CppObject* equipConfig, const char* prefix) {
    auto equipConfigClass = il2cpp_object_get_class(equipConfig);
    LOGIF("%s字段值:", prefix);
    void* iter = nullptr;
    FieldInfo* field;
    while ((field = il2cpp_class_get_fields(equipConfigClass, &iter)) != nullptr) {
        const char* fieldName = il2cpp_field_get_name(field);
        const Il2CppType* fieldType = il2cpp_field_get_type(field);
        const char* typeName = il2cpp_type_get_name(fieldType);
        
        // 根据类型名称判断
        if (strcmp(typeName, "System.Int32") == 0 || 
            strcmp(typeName, "System.UInt32") == 0 ||
            strcmp(typeName, "System.Int64") == 0 ||
            strcmp(typeName, "System.UInt64") == 0 ||
            strcmp(typeName, "System.Int16") == 0 ||
            strcmp(typeName, "System.UInt16") == 0 ||
            strcmp(typeName, "System.Byte") == 0 ||
            strcmp(typeName, "System.SByte") == 0 ||
            strcmp(typeName, "System.Boolean") == 0) {
            uint64_t value = 0;
            il2cpp_field_get_value(equipConfig, field, &value);
            LOGIF("  字段: %s = %" PRIu64 " (类型: %s)", fieldName, value, typeName);
        } else if (strcmp(typeName, "System.String") == 0) {
            Il2CppString* value = nullptr;
            il2cpp_field_get_value(equipConfig, field, &value);
            std::string strValue;
            if (value) {
                const Il2CppChar* utf16Str = il2cpp_string_chars(value);
                int utf16Len = il2cpp_string_length(value);
                strValue = Utf16ToUtf8(utf16Str, utf16Len);
            } else {
                strValue = "null";
            }
            LOGIF("  字段: %s = %s (类型: %s)", fieldName, strValue.c_str(), typeName);
        } else {
            void* value = nullptr;
            il2cpp_field_get_value(equipConfig, field, &value);
            LOGIF("  字段: %s = %p (类型: %s)", fieldName, value, typeName);
        }
    }
}

std::string ConfigParser::Utf16ToUtf8(const Il2CppChar* utf16Str, int utf16Len) {
    std::string utf8Str;
    for (int i = 0; i < utf16Len; i++) {
        Il2CppChar ch = utf16Str[i];
        if (ch <= 0x7F) {
            // ASCII 字符
            utf8Str += static_cast<char>(ch);
        } else if (ch <= 0x7FF) {
            // 2 字节 UTF-8
            utf8Str += static_cast<char>(0xC0 | (ch >> 6));
            utf8Str += static_cast<char>(0x80 | (ch & 0x3F));
        } else {
            // 3 字节 UTF-8
            utf8Str += static_cast<char>(0xE0 | (ch >> 12));
            utf8Str += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
            utf8Str += static_cast<char>(0x80 | (ch & 0x3F));
        }
    }
    return utf8Str;
} 