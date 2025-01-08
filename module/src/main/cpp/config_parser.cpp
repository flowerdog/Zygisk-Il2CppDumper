#include "config_parser.h"
#include "log.h"
#include <vector>
#include <fstream>
#include "il2cpp-api.h"
#include <inttypes.h>
#include "il2cpp-tabledefs.h"
#include "json_serializer.h"
#include <unordered_map>
#include "util.h"

// 字段属性定义
#define FIELD_ATTRIBUTE_STATIC 0x0010



Il2CppClass* ConfigParser::FindOneClass(const char* assemblyName, const char* namespaze, const char* className) {
    if (!assemblyName || !namespaze || !className) {
        LOGEF("!!!参数不能为空: assemblyName=%s, namespaze=%s, className=%s", 
              assemblyName ? assemblyName : "null",
              namespaze ? namespaze : "null", 
              className ? className : "null");
        return nullptr;
    }

    auto classes = FindClass(assemblyName, namespaze, className);
    if (classes.empty()) {
        return nullptr;
    }
    return classes[0];
}

// 添加静态缓存
static std::unordered_map<std::string, std::vector<Il2CppClass*>> classCache;
static bool hasLoggedTypes = false;

/**
 * 查找 IL2CPP 类型
 * @param assemblyName 程序集名称,可选
 * @param namespaze 命名空间,可选 
 * @param className 类名,必填
 * @return 找到的类型列表
 */
std::vector<Il2CppClass*> ConfigParser::FindClass(const char* assemblyName, const char* namespaze, const char* className) {
    // 参数检查
    if (!className) {
        LOGEF("!!!类名不能为空");
        return {};
    }

    // 检查缓存
    std::string cacheKey = std::string(className) + "|" + 
                          (namespaze ? namespaze : "") + "|" + 
                          (assemblyName ? assemblyName : "");
    auto it = classCache.find(cacheKey);
    if (it != classCache.end()) {
        return it->second;
    }

    // 获取 IL2CPP 运行时信息
    auto domain = il2cpp_domain_get();
    if (!domain) {
        LOGEF("!!!获取 IL2CPP domain 失败");
        return {};
    }

    size_t size;
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);
    if (!assemblies || size == 0) {
        LOGEF("!!!获取程序集列表失败");
        return {};
    }

    // 查找结果
    std::vector<Il2CppClass*> foundClasses;
    std::vector<std::pair<std::string, std::string>> foundLocations;

    // 遍历程序集查找类型
    for (size_t i = 0; i < size; i++) {
        auto assembly = assemblies[i];
        if (!assembly) continue;

        auto image = il2cpp_assembly_get_image(assembly);
        if (!image) continue;

        const char* currentAssemblyName = il2cpp_image_get_name(image);
        if (!currentAssemblyName) continue;

        // 检查程序集名称
        if (assemblyName && strcmp(currentAssemblyName, assemblyName) != 0) {
            continue;
        }

        // 指定命名空间时直接查找
        if (namespaze) {
            auto result = il2cpp_class_from_name(image, namespaze, className);
            if (result) {
                foundClasses.push_back(result);
                foundLocations.push_back({currentAssemblyName, namespaze});
            }
            continue;
        }

        // 遍历所有类型
        size_t classCount = il2cpp_image_get_class_count(image);
        for (size_t j = 0; j < classCount; j++) {
            Il2CppClass* klass = (Il2CppClass*)(il2cpp_image_get_class(image, j));
            if (!klass) continue;

            const char* klassName = il2cpp_class_get_name(klass);
            if (!klassName) continue;

            // 调试日志
            if (!hasLoggedTypes && !assemblyName) {
                LOGIF("遍历类型: %s", klassName);
            }

            // 匹配类名
            if (strcmp(klassName, className) == 0) {
                foundClasses.push_back(klass);
                const char* ns = il2cpp_class_get_namespace(klass);
                foundLocations.push_back({currentAssemblyName, ns ? ns : ""});
            }
        }
    }

    if (!assemblyName) {
        hasLoggedTypes = true;
    }

    // 输出查找结果
    if (foundClasses.size() > 1) {
        LOGWF("!!!警告：找到多个同名类型 %s，总共 %zu 个", className, foundClasses.size());
        for (const auto& loc : foundLocations) {
            LOGWF("!!!  - 程序集: %s, 命名空间: %s", loc.first.c_str(), loc.second.c_str());
        }
    }

    // 保存到缓存
    classCache[cacheKey] = foundClasses;
    return foundClasses;
}

bool ConfigParser::ReadFileHead(FILE* fp, ResFileHead& head) {
    // 读取文件头结构
    ResFileHead raw_head;
    if (fread(&raw_head, sizeof(raw_head), 1, fp) != 1) {
        LOGEF("!!!读取文件头失败");
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
    
    // LOGIF("读取到文件头: tag=0x%08X, len=%u, version=%u, resnum=%u, crc32=0x%08X",
        //   head.tag, head.len, head.version, head.resnum, head.crc32);
          
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


bool ConfigParser::SaveAsJson(const char* outputPath, Il2CppArray* configArray) {
    // 创建输出文件
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        LOGEF("!!!无法创建输出文件: %s", outputPath);
        return false;
    }

    LOGIF("开始写入 JSON 文件: %s", outputPath);

    // 写入 JSON 数组开始
    outFile << "[\n";

    // 遍历数组并序列化每个对象
    int32_t length = il2cpp_array_length(configArray);
    LOGIF("配置数组长度: %d", length);
    for (int32_t i = 0; i < length; i++) {
        // 获取数组元素
        void** elementAddr = (void**)((char*)configArray + kIl2CppSizeOfArray + i * sizeof(void*));
        Il2CppObject* item = (Il2CppObject*)*elementAddr;
        if (!item) {
            LOGEF("!!!数组元素为空 [%d/%d]", i + 1, length);
            outFile.close();
            return false;
        }

        LOGIF("序列化配置项 [%d/%d]", i + 1, length);
        outFile << "  ";
        JsonSerializer::SerializeObject(outFile, item, 2);
        
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



bool ConfigParser::ParseAllConfigs(const std::string& dirPath, const std::string& outputDir) {
    LOGIF("开始解析目录下的所有配置文件: %s", dirPath.c_str());
    
    bool allSuccess = true;
    
    // 处理配置目录
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> configDirs = {
        {"default", {
            {"GameProto.dll", "ResDef"},
            {"BattleCore.dll", "BattleCore"},
            {"GameNative.dll", "ResDef"},
        }},
        {"fp", {
            {"BattleCore.dll", "BattleCore"},
            {"GameProto.dll", "ResDef"},
            {"GameNative.dll", "ResDef"},
        }}
    };

    for (const auto& dirInfo : configDirs) {
        std::string configDir = dirPath + "/" + dirInfo.first;
        DIR* dir = opendir(configDir.c_str());
        if (!dir) {
            LOGEF("!!!无法打开目录: %s", configDir.c_str());
            allSuccess = false;
            continue;
        }

        LOGIF("处理 %s 目录: %s", dirInfo.first.c_str(), configDir.c_str());
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string fileName = entry->d_name;
            if (fileName == "." || fileName == "..") continue;

            std::string filePath = configDir + "/" + fileName;
            // LOGIF("检查文件: %s", filePath.c_str());

            // 检查文件头
            FILE* fp = fopen(filePath.c_str(), "rb");
            if (!fp) {
                LOGEF("!!!无法打开文件: %s", filePath.c_str());
                continue;
            }

            ResFileHead head;
            bool isConfigFile = ReadFileHead(fp, head) && (head.tag == 0x00002DEF);
            fclose(fp);

            if (!isConfigFile) {
                LOGIF("跳过非配置文件: %s", filePath.c_str());
                continue;
            }

            // 处理配置文件
            std::string typeName = fileName.substr(0, fileName.find_last_of('.'));
            Il2CppClass* klass = nullptr;
            std::string targetDir = outputDir + "/" + dirInfo.first;

            // 根据目录选择搜索路径
            for (const auto& assemblyInfo : dirInfo.second) {
                klass = FindOneClass(assemblyInfo.first.c_str(),
                                   assemblyInfo.second.c_str(),
                                   typeName.c_str());
                if (klass) break;
            }

            if (!klass) {
                LOGEF("!!!找不到类型: %s", typeName.c_str());
                allSuccess = false;
                continue;
            }

            if (!util::EnsureDirectoryExists(targetDir)) {
                LOGEF("!!!创建目标目录失败: %s", targetDir.c_str());
                allSuccess = false;
                continue;
            }

            if (!ParseConfigFile(filePath, klass, targetDir)) {
                LOGEF("!!!解析文件失败: %s", filePath.c_str());
                allSuccess = false;
            }
        }
        closedir(dir);
    }

    LOGIF("所有配置文件解析完成!");
    return allSuccess;
}

bool ConfigParser::ParseConfigFile(const std::string& filePath, Il2CppClass* klass, const std::string& outputDir) {
    LOGIF("开始解析配置文件: %s", filePath.c_str());
    
    // 打开输入文件
    FILE* fp = fopen(filePath.c_str(), "rb");
    if (!fp) {
        LOGEF("!!!无法打开输入文件: %s", filePath.c_str());
        return false;
    }

    // 读取文件头
    ResFileHead head;
    if (!ReadFileHead(fp, head)) {
        LOGEF("!!!读取文件头失败");
        fclose(fp);
        return false;
    }

    // 验证文件头tag
    if (head.tag != 0x00002DEF) {
        LOGEF("!!!无效的文件头tag: 0x%08X", head.tag);
        fclose(fp);
        return false;
    }

    // LOGIF("文件头tag验证成功");

    // 获取剩余数据大小
    size_t dataSize = head.len - 20;  // 文件头大小为20字节
    std::vector<uint8_t> buffer(dataSize);
    
    if (fread(buffer.data(), 1, dataSize, fp) != dataSize) {
        LOGEF("!!!读取文件数据失败");
        fclose(fp);
        return false;
    }
    
    // LOGIF("读取文件数据成功");
    fclose(fp);

    // 创建并初始化 PbReadBuf
    auto pbReadBufClass = FindOneClass("DodProtoBase.dll", "ProtoBase", "PbReadBuf");
    if (!pbReadBufClass) {
        LOGEF("!!!无法获取 PbReadBuf 类");
        return false;
    }
    // LOGIF("获取 PbReadBuf 类成功");

    auto readBuf = il2cpp_object_new(pbReadBufClass);
    if (!readBuf) {
        LOGEF("!!!创建 PbReadBuf 对象失败");
        return false;
    }

    // LOGIF("创建 PbReadBuf 对象成功");
    // 调用构造函数
    auto ctor = il2cpp_class_get_method_from_name(pbReadBufClass, ".ctor", 0);
    if (!ctor) {
        LOGEF("!!!找不到 PbReadBuf 的构造函数");
        return false;
    }

    // LOGIF("获取 PbReadBuf 的构造函数成功");

    Il2CppException* exc = nullptr;
    il2cpp_runtime_invoke(ctor, readBuf, nullptr, &exc);
    if (exc) {
        LOGEF("!!!调用 PbReadBuf 构造函数失败");
        return false;
    }

    // LOGIF("调用 PbReadBuf 构造函数成功");

    // 创建字节数组并设置数据
    auto byteArrayClass = il2cpp_array_class_get(
        il2cpp_class_from_name(il2cpp_get_corlib(), "System", "Byte"), 
        1
    );
    // LOGIF("获取 Byte 数组类成功");
    
    auto byteArray = il2cpp_array_new(byteArrayClass, dataSize);
    if (!byteArray) {
        LOGEF("!!!创建字节数组失败");
        return false;
    }

    // LOGIF("创建字节数组成功");

    // 复制数据到字节数组
    uint8_t* arrayData = (uint8_t*)((char*)byteArray + kIl2CppSizeOfArray);
    memcpy(arrayData, buffer.data(), dataSize);

    // LOGIF("复制数据到字节数组成功");

    // 调用 set 方法设置数据
    auto setMethod = il2cpp_class_get_method_from_name(pbReadBufClass, "set", 2);
    if (!setMethod) {
        LOGEF("!!!找不到 PbReadBuf.set 方法");
        return false;
    }

    // LOGIF("获取 PbReadBuf.set 方法成功");

    int32_t len = static_cast<int32_t>(dataSize);
    void* setParams[] = { byteArray, &len };
    // LOGIF("准备调用 set 方法，参数: byteArray=%p, len=%d", byteArray, len);
    exc = nullptr;
    il2cpp_runtime_invoke(setMethod, readBuf, setParams, &exc);
    if (exc) {
        auto excClass = il2cpp_object_get_class((Il2CppObject*)exc);
        const char* excName = excClass ? il2cpp_class_get_name(excClass) : "Unknown";
        LOGEF("!!!调用 PbReadBuf.set 方法失败: %s", excName);
        return false;
    }

    // LOGIF("调用 PbReadBuf.set 方法成功，数据大小: %d 字节", len);

    // 创建配置数组
    auto configArray = il2cpp_array_new(klass, head.resnum);
    if (!configArray) {
        LOGEF("!!!创建配置数组失败");
        return false;
    }
    // LOGIF("创建配置数组成功");

    // 循环解析每个配置项
    for (uint32_t i = 0; i < head.resnum; i++) {
        // LOGIF("开始解析配置项 [%u/%u]", i + 1, head.resnum);
        // 创建配置对象
        auto configObject = il2cpp_object_new(klass);
        if (!configObject) {
            LOGEF("!!!创建配置对象失败 [%u/%u]", i + 1, head.resnum);
            return false;
        }
        // LOGIF("创建配置对象成功");

        // 调用构造函数
        auto ctor = il2cpp_class_get_method_from_name(klass, ".ctor", 0);
        if (!ctor) {
            LOGEF("!!!找不到构造函数");
            return false;
        }
        exc = nullptr;
        il2cpp_runtime_invoke(ctor, configObject, nullptr, &exc);
        if (exc) {
            LOGEF("!!!调用构造函数失败");
            return false;
        }

        // 检查是否存在 create 方法，如果存在则调用
        auto createMethod = il2cpp_class_get_method_from_name(klass, "create", 0);
        if (createMethod) {
            // LOGIF("获取 create 方法成功");
            exc = nullptr;
            il2cpp_runtime_invoke(createMethod, configObject, nullptr, &exc);
            if (exc) {
                auto excClass = il2cpp_object_get_class((Il2CppObject*)exc);
                const char* excName = excClass ? il2cpp_class_get_name(excClass) : "Unknown";
                LOGEF("!!!调用 create 方法失败: %s", excName);
                return false;
            }
            // LOGIF("调用 create 方法成功");
        }

        // 调用 unpack 方法
        auto unpackMethod = il2cpp_class_get_method_from_name(klass, "unpack", 3);
        if (!unpackMethod) {
            LOGEF("!!!找不到 unpack 方法");
            return false;
        }
        // LOGIF("获取 unpack 方法成功");

        uint32_t cutVer = 0;
        void* unpackParams[] = { readBuf, &cutVer, nullptr };
        // LOGIF("准备调用 unpack 方法，参数: readBuf=%p, cutVer=%u, stack=%p", readBuf, cutVer, nullptr);
        exc = nullptr;
        il2cpp_runtime_invoke(unpackMethod, configObject, unpackParams, &exc);
        if (exc) {
            auto excClass = il2cpp_object_get_class((Il2CppObject*)exc);
            const char* excName = excClass ? il2cpp_class_get_name(excClass) : "Unknown";
            LOGEF("!!!调用 unpack 方法失败 [%u/%u]: %s", i + 1, head.resnum, excName);
            return false;
        }
        // LOGIF("调用 unpack 方法成功");

        // 存入数组
        void** elementAddr = (void**)((char*)configArray + kIl2CppSizeOfArray + i * sizeof(void*));
        *elementAddr = configObject;
        // LOGIF("成功解析配置项 [%u/%u]", i + 1, head.resnum);
    }

    // 导出为 JSON
    std::string outputPath = outputDir + "/" + il2cpp_class_get_name(klass) + ".json";
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        LOGEF("!!!无法创建输出文件: %s", outputPath.c_str());
        return false;
    }

    // LOGIF("开始导出为 JSON");

    outFile << "[\n";
    int32_t length = il2cpp_array_length(configArray);
    // LOGIF("导出为 JSON 的配置项数量: %d", length);
    for (int32_t i = 0; i < length; i++) {
        void** elementAddr = (void**)((char*)configArray + kIl2CppSizeOfArray + i * sizeof(void*));
        Il2CppObject* item = (Il2CppObject*)*elementAddr;
        
        outFile << "  ";

        // LOGIF("导出为 JSON 的配置项: %d/%d", i + 1, length);
        JsonSerializer::SerializeObject(outFile, item, 2);
        
        if (i < length - 1) {
            outFile << ",";
        }
        outFile << "\n";
    }
    outFile << "]\n";
    outFile.close();

    // LOGIF("成功导出 JSON 文件: %s", outputPath.c_str());
    return true;
} 