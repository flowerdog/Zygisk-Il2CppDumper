#include "config_parser.h"
#include "log.h"
#include <vector>
#include <fstream>

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

    LOGIF("文件头信息: tag=0x%08X, len=%u, version=%u, resnum=%u, crc32=0x%08X",
          head.tag, head.len, head.version, head.resnum, head.crc32);

    // 验证文件头tag
    if (head.tag != 0xEF2D0000) {
        LOGEF("无效的文件头tag: 0x%08X", head.tag);
        fclose(fp);
        return false;
    }

    // 获取EquipConfig类
    Il2CppClass* equipConfigClass = GetEquipConfigClass();
    if (!equipConfigClass) {
        LOGEF("获取EquipConfig类失败");
        fclose(fp);
        return false;
    }

    // 创建数组存储解析结果
    Il2CppArray* configArray = il2cpp_array_new(equipConfigClass, head.resnum);
    if (!configArray) {
        LOGEF("创建配置数组失败");
        fclose(fp);
        return false;
    }

    // 读取并解析每个配置项
    std::vector<uint8_t> buffer;
    for (uint32_t i = 0; i < head.resnum; i++) {
        // 创建EquipConfig对象
        Il2CppObject* equipConfig = CreateEquipConfigObject();
        if (!equipConfig) {
            LOGEF("创建EquipConfig对象失败 [%u/%u]", i + 1, head.resnum);
            fclose(fp);
            return false;
        }

        // 读取数据
        uint32_t dataLen;
        if (fread(&dataLen, sizeof(dataLen), 1, fp) != 1) {
            LOGEF("读取数据长度失败 [%u/%u]", i + 1, head.resnum);
            fclose(fp);
            return false;
        }

        buffer.resize(dataLen);
        if (fread(buffer.data(), 1, dataLen, fp) != dataLen) {
            LOGEF("读取数据内容失败 [%u/%u]", i + 1, head.resnum);
            fclose(fp);
            return false;
        }

        // 解析数据
        if (!UnpackEquipConfig(equipConfig, buffer.data(), dataLen)) {
            LOGEF("解析装备配置失败 [%u/%u]", i + 1, head.resnum);
            fclose(fp);
            return false;
        }

        // 存入数组
        il2cpp_array_set(configArray, Il2CppObject*, i, equipConfig);
        LOGIF("成功解析配置项 [%u/%u]", i + 1, head.resnum);
    }

    fclose(fp);

    // 保存为JSON
    if (!SaveAsJson(outputPath, configArray)) {
        LOGEF("保存JSON文件失败");
        return false;
    }

    LOGIF("成功解析所有配置并保存到: %s", outputPath);
    return true;
}

bool ConfigParser::ReadFileHead(FILE* fp, ResFileHead& head) {
    return fread(&head, sizeof(head), 1, fp) == 1;
}

Il2CppClass* ConfigParser::GetEquipConfigClass() {
    // 获取GameLogic.dll的Image
    auto domain = il2cpp_domain_get();
    size_t size;
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);
    
    const Il2CppImage* gameLogicImage = nullptr;
    for (size_t i = 0; i < size; i++) {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        if (strcmp(il2cpp_image_get_name(image), "GameLogic.dll") == 0) {
            gameLogicImage = image;
            break;
        }
    }

    if (!gameLogicImage) {
        LOGEF("找不到GameLogic.dll");
        return nullptr;
    }

    // 获取EquipConfig类
    auto equipConfigClass = il2cpp_class_from_name(gameLogicImage, "C6Game", "EquipConfig");
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

bool ConfigParser::UnpackEquipConfig(Il2CppObject* equipConfig, const uint8_t* data, size_t dataLen) {
    // 获取PbReadBuf类
    auto domain = il2cpp_domain_get();
    size_t size;
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);
    
    const Il2CppImage* protoImage = nullptr;
    for (size_t i = 0; i < size; i++) {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        if (strcmp(il2cpp_image_get_name(image), "DodProtoBase.dll") == 0) {
            protoImage = image;
            break;
        }
    }

    if (!protoImage) {
        LOGEF("找不到DodProtoBase.dll");
        return false;
    }

    // 创建PbReadBuf对象
    auto pbReadBufClass = il2cpp_class_from_name(protoImage, "ProtoBase", "PbReadBuf");
    if (!pbReadBufClass) {
        LOGEF("找不到PbReadBuf类");
        return false;
    }

    auto readBuf = il2cpp_object_new(pbReadBufClass);
    if (!readBuf) {
        LOGEF("创建PbReadBuf对象失败");
        return false;
    }

    // 调用set方法设置数据
    auto setMethod = il2cpp_class_get_method_from_name(pbReadBufClass, "set", 2);
    if (!setMethod) {
        LOGEF("找不到PbReadBuf.set方法");
        return false;
    }

    void* params[] = {
        data,
        &dataLen
    };
    Il2CppException* exc = nullptr;
    il2cpp_runtime_invoke(setMethod, readBuf, params, &exc);
    if (exc) {
        LOGEF("调用PbReadBuf.set方法失败");
        return false;
    }

    // 调用Unpack方法解析数据
    auto unpackMethod = il2cpp_class_get_method_from_name(il2cpp_object_get_class(equipConfig), "Unpack", 1);
    if (!unpackMethod) {
        LOGEF("找不到EquipConfig.Unpack方法");
        return false;
    }

    void* unpackParams[] = { readBuf };
    il2cpp_runtime_invoke(unpackMethod, equipConfig, unpackParams, &exc);
    if (exc) {
        LOGEF("调用EquipConfig.Unpack方法失败");
        return false;
    }

    return true;
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
        Il2CppObject* item = il2cpp_array_get(configArray, Il2CppObject*, i);
        if (!item) {
            LOGEF("获取数组项失败 [%d/%d]", i + 1, length);
            outFile.close();
            return false;
        }

        // 调用ToJson
        void* params[] = { item };
        Il2CppException* exc = nullptr;
        auto jsonStr = (Il2CppString*)il2cpp_runtime_invoke(toJsonMethod, nullptr, params, &exc);
        if (exc) {
            LOGEF("序列化对象失败 [%d/%d]", i + 1, length);
            outFile.close();
            return false;
        }

        // 写入JSON
        outFile << "  " << il2cpp_string_chars(jsonStr);
        if (i < length - 1) {
            outFile << ",";
        }
        outFile << "\n";
    }

    // 写入JSON数组结束
    outFile << "]\n";
    outFile.close();

    LOGIF("成功保存JSON文件: %s", outputPath);
    return true;
} 