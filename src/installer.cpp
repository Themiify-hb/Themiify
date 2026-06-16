#include <string>
#include <filesystem>
#include <memory>
#include <vector>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <cstdio>

#include <zip.h>
#include <glaze/glaze.hpp>
#include "hips.hpp"
#include <mocha/mocha.h>

#include <sysapp/title.h>
#include <coreinit/foreground.h>

#include "installer.h"
#include "utils.h"

namespace Installer {

    std::unordered_map<std::string, std::string> regionLangMap = {
        {"UsEn", "UsEnglish/Message/AllMessage.szs"},
        {"UsFr", "UsFrench/Message/AllMessage.szs"},
        {"UsPt", "UsPortuguese/Message/AllMessage.szs"},
        {"UsEs", "UsSpanish/Message/AllMessage.szs"},
        {"EuNl", "EuDutch/Message/AllMessage.szs"},
        {"EuEn", "EuEnglish/Message/AllMessage.szs"},
        {"EuFr", "EuFrench/Message/AllMessage.szs"},
        {"EuDe", "EuGerman/Message/AllMessage.szs"},
        {"EuIt", "EuItalian/Message/AllMessage.szs"},
        {"EuPt", "EuPortuguese/Message/AllMessage.szs"},
        {"EuRu", "EuRussian/Message/AllMessage.szs"},
        {"EuEs", "EuSpanish/Message/AllMessage.szs"},
        {"JpJa", "JpJapanese/Message/AllMessage.szs"}
    };

    static std::string ReadWholeFile(std::ifstream& file)
    {
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    static std::string GetString(const glz::generic& obj, const std::string& key)
    {
        return obj.at(key).get<std::string>();
    }

    std::string GetMenuContentPath() {
        uint64_t menuTitleID = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);

        uint32_t menuIDParentDir = (uint32_t)(menuTitleID >> 32);
        uint32_t menuIDChildDir = (uint32_t)menuTitleID;

        char splitMenuID[18];
        snprintf(splitMenuID, sizeof(splitMenuID), "%08x/%08x", menuIDParentDir, menuIDChildDir);

        return "storage_mlc:/sys/title/" + std::string(splitMenuID) + "/content/";
    }

    void CreateCacheFile(std::ifstream &sourceFile, std::string outputPath) {
        if (!sourceFile.is_open()) {
            WHBLogPrintf("Invalid or unopened source file");
            return;
        }

        std::size_t sourceSize = static_cast<std::size_t>(sourceFile.tellg());
        sourceFile.seekg(0, std::ios::beg);

        std::vector<unsigned char> buffer(sourceSize);
        sourceFile.read(reinterpret_cast<char*>(buffer.data()), sourceSize);

        if (!sourceFile) {
            WHBLogPrintf("Error reading source file to create cache file.");
            return;
        }

        std::ofstream outputFile(outputPath, std::ios::binary | std::ios::trunc);
        if (!outputFile.is_open()) {
            WHBLogPrintf("Failed to open output file for writing: %s", outputPath.c_str());
            return;
        }

        outputFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

        if (!outputFile) {
            WHBLogPrintf("Error writing cache file to %s", outputPath.c_str());
            return;
        }

        WHBLogPrintf("Successfully cached file to: %s", outputPath.c_str());
    }

    int GetThemeMetadata(std::string themePath, theme_data *themeData) {
        zip_t *themeArchive;
        zip_error_t error;
        int err;

        if (!(themeArchive = zip_open(themePath.c_str(), 0, &err))) {
            zip_error_init_with_code(&error, err);
            WHBLogPrintf("Cannot open theme archive. Error Code: %s", zip_error_strerror(&error));
            zip_error_fini(&error);
            return 0;
        }

        zip_file_t *themeMetadataFile;
        if (!(themeMetadataFile = zip_fopen(themeArchive, "metadata.json", ZIP_RDONLY))) {
            zip_error_init_with_code(&error, err);
            WHBLogPrintf("Cannot open theme metadata. Error Code: %s", zip_error_strerror(&error));
            zip_error_fini(&error);
            zip_close(themeArchive);
            return 0;
        }

        zip_stat_t metadataStatData;
        if (zip_stat(themeArchive, "metadata.json", 0, &metadataStatData) != 0) {
            zip_error_init_with_code(&error, err);
            WHBLogPrintf("Cannot stat theme metadata! Error Code: %s", zip_error_strerror(&error));
            zip_error_fini(&error);
            zip_fclose(themeMetadataFile);
            zip_close(themeArchive);
            return 0;
        }

        std::string buffer(metadataStatData.size, '\0');
        zip_fread(themeMetadataFile, buffer.data(), metadataStatData.size);
        zip_fclose(themeMetadataFile);
        zip_close(themeArchive);

        glz::generic themeMetadata;
        if (auto err = glz::read_json(themeMetadata, buffer)) {
            WHBLogPrintf("Failed to parse metadata.json: %s",
                         glz::format_error(err, buffer).c_str());
            return 0;
        }

        auto& metadata = themeMetadata.at("Metadata");

        themeData->themeID = GetString(metadata, "themeID");

        std::string themeIDPathStr = themeData->themeID;
        themeIDPathStr.erase(std::remove(themeIDPathStr.begin(), themeIDPathStr.end(), ':'), themeIDPathStr.end());
        themeData->themeIDPath = themeIDPathStr;

        themeData->themeName = removeNonASCII(GetString(metadata, "themeName"));
        themeData->themeAuthor = removeNonASCII(GetString(metadata, "themeAuthor"));
        themeData->themeVersion = GetString(metadata, "themeVersion");

        return 1;
    }

    int GetInstalledThemeMetadata(std::string installedThemeJsonPath, installed_theme_data *themeData) {
        std::ifstream installedThemeJson(installedThemeJsonPath);

        if (!installedThemeJson.is_open()) {
            WHBLogPrintf("Cannot open installed theme's json file.");
            return 0;
        }

        std::string jsonStr = ReadWholeFile(installedThemeJson);

        glz::generic installedThemeMetadata;
        if (auto err = glz::read_json(installedThemeMetadata, jsonStr)) {
            WHBLogPrintf("Failed to parse installed theme json: %s",
                         glz::format_error(err, jsonStr).c_str());
            return 0;
        }

        auto& data = installedThemeMetadata.at("ThemeData");

        themeData->themeID = GetString(data, "themeID");
        themeData->themeIDPath = GetString(data, "themeIDPath");
        themeData->themeName = GetString(data, "themeName");
        themeData->themeAuthor = GetString(data, "themeAuthor");
        themeData->themeVersion = GetString(data, "themeVersion");
        themeData->installedThemePath = GetString(data, "themeInstallPath");

        return 1;
    }

    bool InstallTheme(std::filesystem::path themePath, theme_data themeData) {
        bool themeInstallSuccess = false;
        OSEnableHomeButtonMenu(FALSE);

        std::string menuContentPath = GetMenuContentPath();

        zip_t *themeArchive;
        zip_error_t error;
        int err;

        if (!(themeArchive = zip_open(themePath.c_str(), 0, &err))) {
            zip_error_init_with_code(&error, err);
            WHBLogPrintf("Cannot open theme archive. Error Code: %s", zip_error_strerror(&error));
            zip_error_fini(&error);
            return themeInstallSuccess;
        }

        WHBLogPrintf("Installing %s...", themeData.themeName.c_str());

        std::string modpackPath = std::string(THEMES_ROOT) + "/" + themeData.themeName + " (" + themeData.themeIDPath + ")";
        WHBLogPrintf("Installing theme to: %s", modpackPath.c_str());

        int64_t numEntries;
        if ((numEntries = zip_get_num_entries(themeArchive, ZIP_FL_UNCHANGED)) < 0) {
            WHBLogPrintf("Theme archive is NULL!");
            zip_close(themeArchive);
            return themeInstallSuccess;
        }

        for (uint64_t i = 0; i < static_cast<uint64_t>(numEntries); ++i) {
            std::string menuFilePath;
            std::string entryName = std::string(zip_get_name(themeArchive, i, ZIP_FL_ENC_RAW));

            if (entryName == "Men.bps") {
                menuFilePath += MEN_PATH;
            }
            else if (entryName == "Men2.bps") {
                menuFilePath += MEN2_PATH;
            }
            else if (entryName == "cafe_barista_men.bps") {
                menuFilePath += CAFE_BARISTA_MEN_PATH;
            }
            else if (entryName.contains("AllMessage")) {
                const std::string allMessageStr = "AllMessage_";
                const std::string extensionStr = ".bps";

                std::string regionLangStr = entryName.substr(
                    allMessageStr.size(),
                    entryName.size() - allMessageStr.size() - extensionStr.size()
                );

                auto it = regionLangMap.find(regionLangStr);
                if (it == regionLangMap.end()) {
                    WHBLogPrintf("Unknown AllMessage Region and Language: %s", regionLangStr.c_str());
                    continue;
                }

                menuFilePath += it->second;
            }

            if (entryName != "metadata.json") {
                WHBLogPrintf("menuFilePath: %s", menuFilePath.c_str());

                std::string menuPath = menuContentPath + menuFilePath;
                std::string cachePath = std::string(THEMIIFY_ROOT) + "/cache/" + menuFilePath;
                std::string patchPath = entryName;
                std::string outputPath = modpackPath + "/content/" + menuFilePath;

                CreateParentDirectories(cachePath);

                zip_file_t *patchFile;
                if (!(patchFile = zip_fopen(themeArchive, patchPath.c_str(), ZIP_RDONLY))) {
                    zip_error_init_with_code(&error, err);
                    WHBLogPrintf("Cannot open %s!. Error Code: %s", patchPath.c_str(), zip_error_strerror(&error));
                    zip_error_fini(&error);
                    themeInstallSuccess = false;
                    break;
                }

                zip_stat_t patchStatData;
                if (zip_stat(themeArchive, patchPath.c_str(), 0, &patchStatData) != 0) {
                    zip_error_init_with_code(&error, err);
                    WHBLogPrintf("Cannot stat %s!. Error Code: %s", patchPath.c_str(), zip_error_strerror(&error));
                    zip_error_fini(&error);
                    zip_fclose(patchFile);
                    themeInstallSuccess = false;
                    break;
                }

                std::size_t patchSize = patchStatData.size;

                std::ifstream inputFile(cachePath, std::ios::binary | std::ios::ate);
                if (!inputFile.is_open()) {
                    WHBLogPrintf("Cache does not exist for %s", menuPath.c_str());

                    inputFile.clear();
                    inputFile.open(menuPath, std::ios::binary | std::ios::ate);

                    if (!inputFile.is_open()) {
                        WHBLogPrintf("Could not open source file for %s", patchPath.c_str());
                        inputFile.clear();
                        zip_fclose(patchFile);
                        continue;
                    }
                    else {
                        WHBLogPrintf("Creating cache for %s", menuPath.c_str());
                        CreateCacheFile(inputFile, cachePath);
                    }
                }
                else {
                    WHBLogPrintf("Found %s in cache at %s", menuFilePath.c_str(), cachePath.c_str());
                }

                std::size_t inputSize = static_cast<std::size_t>(inputFile.tellg());
                inputFile.seekg(0, std::ios::beg);

                std::vector<uint8_t> patchData(patchSize);
                std::vector<uint8_t> inputData(inputSize);

                zip_fread(patchFile, patchData.data(), patchSize);
                zip_fclose(patchFile);

                inputFile.read(reinterpret_cast<char*>(inputData.data()), inputSize);
                inputFile.close();

                auto [bytes, result] = Hips::patch(
                    inputData.data(),
                    inputSize,
                    patchData.data(),
                    patchSize,
                    Hips::PatchType::BPS
                );

                if (result == Hips::Result::Success) {
                    CreateParentDirectories(outputPath);

                    std::ofstream outputFile(outputPath, std::ios::binary);
                    outputFile.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
                    outputFile.close();

                    themeInstallSuccess = true;

                    WHBLogPrintf("File written to %s", outputPath.c_str());
                }
                else {
                    WHBLogPrintf("Patch failed. Hips result: %d", result);
                    themeInstallSuccess = false;
                    break;
                }
            }
        }

        zip_close(themeArchive);

        std::string installPath = std::string(THEMIIFY_INSTALLED_THEMES) + "/" + themeData.themeIDPath + ".json";

        if (themeInstallSuccess) {
            WHBLogPrintf("Install Path: %s", installPath.c_str());
            CreateParentDirectories(installPath);

            glz::generic installedThemeJson;
            installedThemeJson["ThemeData"]["themeName"] = themeData.themeName;
            installedThemeJson["ThemeData"]["themeAuthor"] = themeData.themeAuthor;
            installedThemeJson["ThemeData"]["themeID"] = themeData.themeID;
            installedThemeJson["ThemeData"]["themeIDPath"] = themeData.themeIDPath;
            installedThemeJson["ThemeData"]["themeVersion"] = themeData.themeVersion;
            installedThemeJson["ThemeData"]["themeInstallPath"] = modpackPath;

            auto jsonStr = glz::write<glz::opts{.prettify = true}>(installedThemeJson);
            if (!jsonStr) {
                WHBLogPrintf("%s failed to serialize installed theme json!", themeData.themeName.c_str());
            }
            else {
                std::ofstream outFile(installPath);
                if (outFile.is_open()) {
                    outFile << *jsonStr;
                    outFile.close();
                    WHBLogPrintf("%s saved to Themiify installed directory.", themeData.themeName.c_str());
                }
                else {
                    WHBLogPrintf("%s failed to save to Themiify installed directory!", themeData.themeName.c_str());
                }
            }
        }
        else {
            DeleteTheme(modpackPath, installPath);
        }

        OSEnableHomeButtonMenu(TRUE);
        return themeInstallSuccess;
    }

    bool DeleteTheme(std::string modpackPath, std::string installPath) {
        std::filesystem::path install_path = installPath;
        // Dumb hack but I don't wanna change more stuff
        std::filesystem::path thumbnailPath = std::string(THEMIIFY_THUMBNAILS) + "/" + std::string(install_path.stem()) + ".webp";

        DeletePath(modpackPath);
        DeletePath(installPath);
        DeletePath(thumbnailPath);

        if (std::filesystem::exists(modpackPath) && std::filesystem::exists(installPath) && std::filesystem::exists(thumbnailPath)) {
            return false;
        }

        return true;
    }

    std::string GetStyleMiiUConfigPath() {
        char environmentPathBuffer[0x100];

        MochaUtilsStatus res;
        if ((res = Mocha_GetEnvironmentPath(environmentPathBuffer, sizeof(environmentPathBuffer))) != MOCHA_RESULT_SUCCESS) {
            WHBLogPrintf("Failed to get environment path. Are you running on Aroma? Result: %s", Mocha_GetStatusStr(res));
            return "";
        }

        return std::string(environmentPathBuffer) + "/plugins/config/style-mii-u.json";
    }

    bool SetCurrentTheme(std::string themeName, std::string themeID) {
        std::string styleMiiUConfigPath = GetStyleMiiUConfigPath();

        std::ifstream configFile(styleMiiUConfigPath);
        if (!configFile.is_open()) {
            WHBLogPrintf("Failed to open config file: %s", styleMiiUConfigPath.c_str());
            return false;
        }

        std::string jsonStr = ReadWholeFile(configFile);
        configFile.close();

        glz::generic configJson;
        if (auto err = glz::read_json(configJson, jsonStr)) {
            WHBLogPrintf("Failed to parse config file: %s",
                         glz::format_error(err, jsonStr).c_str());
            return false;
        }

        configJson["storageitems"]["enabledThemes"] = themeName + " (" + themeID + ")";

        auto outputJson = glz::write<glz::opts{.prettify = true}>(configJson);
        if (!outputJson) {
            WHBLogPrintf("Failed to serialize config json");
            return false;
        }

        std::ofstream outFile(styleMiiUConfigPath, std::ios::trunc);
        if (!outFile.is_open()) {
            WHBLogPrintf("Failed to open for write: %s", styleMiiUConfigPath.c_str());
            return false;
        }

        outFile << *outputJson;
        outFile.close();

        WHBLogPrintf("Succesfully set %s as current StyleMiiU theme!", themeName.c_str());

        return true;
    }

    std::string GetCurrentTheme() {
        std::string styleMiiUConfigPath = GetStyleMiiUConfigPath();

        std::ifstream configFile(styleMiiUConfigPath);
        if (!configFile.is_open()) {
            WHBLogPrintf("Failed to open config file: %s", styleMiiUConfigPath.c_str());
            return "";
        }

        std::string jsonStr = ReadWholeFile(configFile);
        configFile.close();

        glz::generic configJson;
        if (auto err = glz::read_json(configJson, jsonStr)) {
            WHBLogPrintf("Failed to parse config file: %s",
                         glz::format_error(err, jsonStr).c_str());
            return "";
        }

        return configJson.at("storageitems").at("enabledThemes").get<std::string>();
    }

} // namespace Installer