#include <string>
#include <filesystem>
#include <memory>
#include <vector>
#include <fstream>
#include <algorithm>

#include <zip.h>
#include <nlohmann/json.hpp>
#include <hips.hpp>
#include <mocha/mocha.h>

#include <whb/log.h>
#include <sysapp/title.h>
#include <coreinit/foreground.h>

#include "installer.h"
#include "utils.h"

namespace json = nlohmann;

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

    std::string GetMenuContentPath() {
        uint64_t menuTitleID = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);

        uint32_t menuIDParentDir = (uint32_t)(menuTitleID >> 32);
        uint32_t menuIDChildDir = (uint32_t)menuTitleID;

        char splitMenuID[18];
        
        snprintf(splitMenuID, sizeof(splitMenuID), "%08x/%08x", menuIDParentDir, menuIDChildDir);

        std::string menuContentPath = "storage_mlc:/sys/title/" + std::string(splitMenuID) + "/content/";

        return menuContentPath;
    }

    void CreateCacheFile(std::ifstream &sourceFile, std::string outputPath) {
        if (!sourceFile.is_open()) {
            WHBLogPrintf("Invalid or unopened source file");
            return;
        }
        
        // Getting the size instantly because sourceFile was opened with std::ios::ate
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

            return 0;          
        }

        zip_stat_t metadataStatData;
        if ((zip_stat(themeArchive, "metadata.json", 0, &metadataStatData)) != 0) {
            zip_error_init_with_code(&error, err);

            WHBLogPrintf("Cannot stat theme metadata! Error Code: %s", zip_error_strerror(&error));

            zip_error_fini(&error);   

            return 0;                      
        }
        
        std::string buffer(metadataStatData.size, '\0');

        zip_fread(themeMetadataFile, &buffer[0], metadataStatData.size);
        zip_fclose(themeMetadataFile);

        zip_close(themeArchive);
        
        std::unique_ptr<json::json> themeMetadata = std::make_unique<json::json>(json::json::parse(buffer));

        themeData->themeID = std::string(themeMetadata->at("Metadata").at("themeID"));

        std::string themeIDPathStr = themeData->themeID;
        themeIDPathStr.erase(std::remove(themeIDPathStr.begin(), themeIDPathStr.end(), ':'), themeIDPathStr.end());
        themeData->themeIDPath = themeIDPathStr;

        themeData->themeName = removeNonASCII(std::string(themeMetadata->at("Metadata").at("themeName")));
        themeData->themeAuthor = removeNonASCII(std::string(themeMetadata->at("Metadata").at("themeAuthor")));
        themeData->themeVersion = std::string(themeMetadata->at("Metadata").at("themeVersion"));

        themeMetadata->clear();

        return 1;
    }

    int GetInstalledThemeMetadata(std::string installedThemeJsonPath, installed_theme_data *themeData) {
        std::ifstream installedThemeJson(installedThemeJsonPath);

        if (!installedThemeJson.is_open()) {
            WHBLogPrintf("Cannot open installed theme's json file.");

            return 0;
        }
        
        std::unique_ptr<json::json> installedThemeMetadata = std::make_unique<json::json>(json::json::parse(installedThemeJson));
 
        themeData->themeID = std::string(installedThemeMetadata->at("ThemeData").at("themeID"));
        themeData->themeIDPath = std::string(installedThemeMetadata->at("ThemeData").at("themeIDPath"));
        themeData->themeName = std::string(installedThemeMetadata->at("ThemeData").at("themeName"));
        themeData->themeAuthor = std::string(installedThemeMetadata->at("ThemeData").at("themeAuthor"));
        themeData->themeVersion = std::string(installedThemeMetadata->at("ThemeData").at("themeVersion"));
        themeData->installedThemePath = std::string(installedThemeMetadata->at("ThemeData").at("themeInstallPath"));

        installedThemeMetadata->clear();

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

            return themeInstallSuccess;
        }
        
        for (uint64_t i = 0; i < static_cast<uint64_t>(numEntries); ++i) {
            std::string menuFilePath = "";
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

                std::string regionLangStr = entryName.substr(allMessageStr.size(), entryName.size() - allMessageStr.size() - extensionStr.size());

                auto it = regionLangMap.find(regionLangStr);
                if (it == regionLangMap.end()) {
                    WHBLogPrintf("Unknown AllMessage Region and Language: %s", regionLangStr.c_str());
                }

                std::string path = it->second;
                menuFilePath += path;
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
                if ((zip_stat(themeArchive, patchPath.c_str(), 0, &patchStatData)) != 0) {
                    zip_error_init_with_code(&error, err);

                    WHBLogPrintf("Cannot stat %s!. Error Code: %s", patchPath.c_str(), zip_error_strerror(&error));

                    zip_error_fini(&error);   

                    themeInstallSuccess = false;  
                    break;           
                }
                std::size_t patchSize = patchStatData.size;

                std::ifstream inputFile(cachePath, std::ios::binary | std::ios::ate);
                if (!inputFile.is_open()) {
                    WHBLogPrintf("Cache does not exist, creating cache for %s", menuPath.c_str());

                    inputFile.clear();
                    inputFile.open(menuPath, std::ios::binary | std::ios::ate);
                    if (!inputFile.is_open()) {
                        WHBLogPrintf("Could not open source file for %s", patchPath.c_str());

                        inputFile.clear();
                        /* The file doesn't exist but theme installation can still continue on.
                        Unless something is seriously wrong, this will only occur if an out of region file
                        (like a text replacement for a specific language) is what's being patched. */
                        continue;
                    }
                    else {
                        CreateCacheFile(inputFile, cachePath);
                    }
                }
                else {
                    WHBLogPrintf("Found %s in cache at %s", std::string(menuFilePath).c_str(), cachePath.c_str());
                }

                std::size_t inputSize = static_cast<std::size_t>(inputFile.tellg());
                inputFile.seekg(0, std::ios::beg);

                std::vector<uint8_t> patchData(patchSize);
                std::vector<uint8_t> inputData(inputSize);

                zip_fread(patchFile, patchData.data(), patchSize);
                zip_fclose(patchFile);

                inputFile.read(reinterpret_cast<char*>(inputData.data()), inputSize);
                inputFile.close();

                auto [bytes, result] = Hips::patch(inputData.data(), inputSize, patchData.data(), patchSize, Hips::PatchType::BPS);
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
            json::json installedThemeJson;
            installedThemeJson["ThemeData"] = {
                {"themeName", themeData.themeName},
                {"themeAuthor", themeData.themeAuthor},
                {"themeID", themeData.themeID},
                {"themeIDPath", themeData.themeIDPath},
                {"themeVersion", themeData.themeVersion},
                {"themeInstallPath", modpackPath}
            };

            std::ofstream outFile(installPath);
            if (outFile.is_open()) {
                outFile << installedThemeJson.dump(4);
                outFile.close();
                WHBLogPrintf("%s saved to Themiify installed directory.", themeData.themeName.c_str());
            }
            else {
                // Not sure exactly what to tell the user here
                WHBLogPrintf("%s failed to save to Themiify installed directory!", themeData.themeName.c_str());
            }

            installedThemeJson.clear();

            SetCurrentTheme(themeData.themeName, themeData.themeIDPath);
        }
        else {
            // In case the theme was somehow already installed
            // Would prolly also happen if two themes use the same theme id but one is broken... Not sure how I feel about that one but we'll have to see
            DeleteTheme(modpackPath, installPath);
        }

        OSEnableHomeButtonMenu(TRUE);
        return themeInstallSuccess;
    }

    bool DeleteTheme(std::string modpackPath, std::string installPath) {
        DeletePath(modpackPath);
        DeletePath(installPath);

        if (std::filesystem::exists(modpackPath) & std::filesystem::exists(installPath)) {
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

        std::string styleMiiUConfigPath = std::string(environmentPathBuffer) + "/plugins/config/style-mii-u.json";

        return styleMiiUConfigPath;
    }

    bool SetCurrentTheme(std::string themeName, std::string themeID) {
        std::string styleMiiUConfigPath = GetStyleMiiUConfigPath();
        
        std::ifstream configFile(styleMiiUConfigPath);
        if (!configFile.is_open()) {
            WHBLogPrintf("Failed to open config file: %s", styleMiiUConfigPath.c_str());
            return false;
        }
        json::json configJson = json::json::parse(configFile);

        configFile.close();

        configJson["storageitems"]["enabledThemes"] = std::string(themeName + " (" + themeID + ")");

        std::ofstream outFile(styleMiiUConfigPath, std::ios::trunc);
        if (!outFile.is_open()) {
            WHBLogPrintf("Failed to open for write: %s", styleMiiUConfigPath.c_str());
            return false;
        }
        outFile << configJson.dump(4);
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
        json::json configJson = json::json::parse(configFile);

        configFile.close();

        std::string themeName = configJson["storageitems"]["enabledThemes"];

        return themeName;
    }
}