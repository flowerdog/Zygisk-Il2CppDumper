#include "config_parser.h"
#include "log.h"
#include <vector>
#include <fstream>
#include "il2cpp-api.h"
#include <inttypes.h>

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

    // 获取剩余数据大小
    size_t dataSize = head.len - 20;  // 文件头大小为20字节
    std::vector<uint8_t> buffer(dataSize);
    
    if (fread(buffer.data(), 1, dataSize, fp) != dataSize) {
        LOGEF("读取文件数据失败");
        fclose(fp);
        return false;
    }
    
    fclose(fp);

    // 创建并初始化 PbReadBuf
    auto pbReadBufClass = il2cpp_class_from_name(
        il2cpp_assembly_get_image(il2cpp_domain_get_assemblies(il2cpp_domain_get(), nullptr)[0]), 
        "ProtoBase", 
        "PbReadBuf"
    );
    
    if (!pbReadBufClass) {
        LOGEF("无法获取 PbReadBuf 类");
        return false;
    }

    auto readBuf = il2cpp_object_new(pbReadBufClass);
    if (!readBuf) {
        LOGEF("创建 PbReadBuf 对象失败");
        return false;
    }

    // 调用构造函数
    auto ctor = il2cpp_class_get_method_from_name(pbReadBufClass, ".ctor", 0);
    if (!ctor) {
        LOGEF("找不到 PbReadBuf 的构造函数");
        return false;
    }

    Il2CppException* exc = nullptr;
    il2cpp_runtime_invoke(ctor, readBuf, nullptr, &exc);
    if (exc) {
        LOGEF("调用 PbReadBuf 构造函数失败");
        return false;
    }

    // 创建字节数组并设置数据
    auto byteArrayClass = il2cpp_array_class_get(
        il2cpp_class_from_name(il2cpp_get_corlib(), "System", "Byte"), 
        1
    );
    
    auto byteArray = il2cpp_array_new(byteArrayClass, dataSize);
    if (!byteArray) {
        LOGEF("创建字节数组失败");
        return false;
    }

    // 复制数据到字节数组
    uint8_t* arrayData = (uint8_t*)((char*)byteArray + kIl2CppSizeOfArray);
    memcpy(arrayData, buffer.data(), dataSize);

    // 调用 set 方法设置数据
    auto setMethod = il2cpp_class_get_method_from_name(pbReadBufClass, "set", 2);
    if (!setMethod) {
        LOGEF("找不到 PbReadBuf.set 方法");
        return false;
    }

    int32_t len = static_cast<int32_t>(dataSize);
    void* setParams[] = { byteArray, &len };
    exc = nullptr;
    il2cpp_runtime_invoke(setMethod, readBuf, setParams, &exc);
    if (exc) {
        LOGEF("调用 PbReadBuf.set 方法失败");
        return false;
    }

    // 获取 EquipConfig 类并创建结果数组
    auto equipConfigClass = GetEquipConfigClass();
    if (!equipConfigClass) {
        LOGEF("获取 EquipConfig 类失败");
        return false;
    }

    auto configArray = il2cpp_array_new(equipConfigClass, head.resnum);
    if (!configArray) {
        LOGEF("创建配置数组失败");
        return false;
    }

    // 循环解析每个配置项
    for (uint32_t i = 0; i < head.resnum; i++) {
        // 创建 EquipConfig 对象
        auto equipConfig = il2cpp_object_new(equipConfigClass);
        if (!equipConfig) {
            LOGEF("创建 EquipConfig 对象失败 [%u/%u]", i + 1, head.resnum);
            return false;
        }

        // 调用 unpack 方法
        auto unpackMethod = il2cpp_class_get_method_from_name(equipConfigClass, "unpack", 3);
        if (!unpackMethod) {
            LOGEF("找不到 EquipConfig.unpack 方法");
            return false;
        }

        uint32_t cutVer = 0;
        void* stack = nullptr;
        void* unpackParams[] = { readBuf, &cutVer, stack };
        
        exc = nullptr;
        il2cpp_runtime_invoke(unpackMethod, equipConfig, unpackParams, &exc);
        if (exc) {
            auto excClass = il2cpp_object_get_class((Il2CppObject*)exc);
            const char* excName = excClass ? il2cpp_class_get_name(excClass) : "Unknown";
            LOGEF("调用 EquipConfig.unpack 方法失败 [%u/%u]: %s", i + 1, head.resnum, excName);
            return false;
        }

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
    // 获取GameLogic.dll的Image
    auto domain = il2cpp_domain_get();
    size_t size;
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);
    
    const Il2CppImage* gameLogicImage = nullptr;
    for (size_t i = 0; i < size; i++) {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        if (strcmp(il2cpp_image_get_name(image), "GameProto.dll") == 0) {
            gameLogicImage = image;
            break;
        }
    }

    if (!gameLogicImage) {
        LOGEF("找不到GameLogic.dll");
        return nullptr;
    }

    print_all_types(gameLogicImage);

    // 获取EquipConfig类
    auto equipConfigClass = il2cpp_class_from_name(gameLogicImage, "ResDef", "EquipConfig");
    if (!equipConfigClass) {
        LOGEF("找不到EquipConfig类");
        return nullptr;
    }

    return equipConfigClass;
}

Il2CppObject* ConfigParser::CreateEquipConfigObject() {
    auto equipConfigClass = GetEquipConfigClass();
    if (!equipConfigClass) {
        return nullptr;
    }
    return il2cpp_object_new(equipConfigClass);
}

bool ConfigParser::SaveAsJson(const char* outputPath, Il2CppArray* configArray) {
    // 获取UnityEngine.JsonUtility类
    auto domain = il2cpp_domain_get();
    size_t size;
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);
    
    const Il2CppImage* unityEngineImage = nullptr;
    for (size_t i = 0; i < size; i++) {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        if (strcmp(il2cpp_image_get_name(image), "UnityEngine.CoreModule.dll") == 0) {
            unityEngineImage = image;
            break;
        }
    }

    if (!unityEngineImage) {
        LOGEF("找不到UnityEngine.CoreModule.dll");
        return false;
    }

    auto jsonUtilityClass = il2cpp_class_from_name(unityEngineImage, "UnityEngine", "JsonUtility");
    if (!jsonUtilityClass) {
        LOGEF("找不到JsonUtility类");
        return false;
    }

    // 获取ToJson方法
    auto toJsonMethod = il2cpp_class_get_method_from_name(jsonUtilityClass, "ToJson", 1);
    if (!toJsonMethod) {
        LOGEF("找不到ToJson方法");
        return false;
    }

    // 创建输出文件
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        LOGEF("无法创建输出文件: %s", outputPath);
        return false;
    }

    // 写入JSON数组开始
    outFile << "[\n";

    // 遍历数组并序列化每个对象
    int32_t length = il2cpp_array_length(configArray);
    for (int32_t i = 0; i < length; i++) {
        // 获取数组元素地址
        void** elementAddr = (void**)il2cpp_array_addr_with_size(configArray, sizeof(void*), i);
        if (!elementAddr) {
            LOGEF("获取数组元素地址失败 [%d/%d]", i + 1, length);
            outFile.close();
            return false;
        }
        
        Il2CppObject* item = (Il2CppObject*)*elementAddr;
        if (!item) {
            LOGEF("数组元素为空 [%d/%d]", i + 1, length);
            outFile.close();
            return false;
        }

        // 调用ToJson
        void* params[] = { item };
        Il2CppException* exc = nullptr;
        auto jsonStr = (Il2CppString*)il2cpp_runtime_invoke(toJsonMethod, nullptr, params, &exc);
        if (exc) {
            auto excClass = il2cpp_object_get_class((Il2CppObject*)exc);
            const char* excName = excClass ? il2cpp_class_get_name(excClass) : "Unknown";
            LOGEF("序列化对象失败 [%d/%d]: %s", i + 1, length, excName);
            outFile.close();
            return false;
        }

        // 将 UTF-16 转换为 UTF-8
        const Il2CppChar* utf16Str = il2cpp_string_chars(jsonStr);
        int utf16Len = il2cpp_string_length(jsonStr);
        std::string utf8Str;
        
        for (int j = 0; j < utf16Len; j++) {
            Il2CppChar ch = utf16Str[j];
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

        // 写入 JSON
        outFile << utf8Str;
        if (i < length - 1) {
            outFile << ",\n";
        } else {
            outFile << "\n";
        }
    }

    // 写入JSON数组结束
    outFile << "]\n";
    outFile.close();

    return true;
} 