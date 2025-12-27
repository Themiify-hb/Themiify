/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string>
#include <filesystem>
#include <memory>
#include <vector>
#include <fstream>

#include <zip.h>
#include <nlohmann/json.hpp>
#include <hips.hpp>

#include <whb/log.h>
#include <sysapp/title.h>

#include "installer.h"
#include "utils.h"

#define SDCAFIINE_ROOT "fs:/vol/external01/wiiu/sdcafiine"

namespace json = nlohmann;

namespace Installer {   
    Region GetSystemRegion() {
        uint64_t menuTitleID = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);

        Region cRegion = USA; // doesn't matter what this defualts to but MURICAAAAAAA

        switch (menuTitleID) {
            case WII_U_MENU_JPN_TID:
                cRegion = JPN;
                break;
            case WII_U_MENU_USA_TID:
                cRegion = USA;
                break;
            case WII_U_MENU_EUR_TID:
                cRegion = EUR;
                break;
        }

        return cRegion;
    } 

    std::pair<std::string, std::string> GetMenuStrings() {
        uint64_t menuTitleID = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);

        uint32_t menuIDParentDir = (uint32_t)(menuTitleID >> 32);
        uint32_t menuIDChildDir = (uint32_t)menuTitleID;

        char menuIDStr[17];
        char splitMenuID[18];
        
        snprintf(menuIDStr, sizeof(menuIDStr), "%016llx", menuTitleID);
        snprintf(splitMenuID, sizeof(splitMenuID), "%08x/%08x", menuIDParentDir, menuIDChildDir);

        std::string menuContentPath = "storage_mlc:/sys/title/" + std::string(splitMenuID) + "/content/";
        std::string sdcafiineMenuPath = "fs:/vol/external01/wiiu/sdcafiine/" + std::string(menuIDStr) + "/";

        return {menuContentPath, sdcafiineMenuPath};
    }

    void CreateCacheFile(std::FILE *sourceFile, std::string outputPath) {
        std::FILE *outputFile = std::fopen(outputPath.c_str(), "wb");

        std::fseek(sourceFile, 0, SEEK_END);
        std::size_t sourceSize = std::ftell(sourceFile);
        std::rewind(sourceFile);

        std::vector<unsigned char> buffer(sourceSize);
        std::size_t bytesRead, bytesWritten;

        while ((bytesRead = std::fread(buffer.data(), 1, sourceSize, sourceFile)) > 0) {
            bytesWritten = std::fwrite(buffer.data(), 1, bytesRead, outputFile);

            if (bytesRead != bytesRead) {
                WHBLogPrintf("Error writing cache file!");
                std::fclose(outputFile);
                return;
            }
        }

        std::fclose(outputFile);

        WHBLogPrintf("Successfully cached file to: %s", outputPath.c_str());

        return;
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

        std::string cRegion = std::string(themeMetadata->at("Metadata").at("themeRegion"));

        if (cRegion.compare("JPN") == 0) {
            themeData->themeRegion = JPN;
        }
        else if (cRegion.compare("USA") == 0) {
            themeData->themeRegion = USA;
        }
        else if (cRegion.compare("EUR") == 0) {
            themeData->themeRegion = EUR;
        }
        else if (cRegion.compare("UNIVERSAL") == 0) {
            themeData->themeRegion = UNIVERSAL;
        }

        themeData->themeID = std::string(themeMetadata->at("Metadata").at("themeID"));
        themeData->themeName = std::string(themeMetadata->at("Metadata").at("themeName"));
        themeData->themeAuthor = std::string(themeMetadata->at("Metadata").at("themeAuthor"));

        themeMetadata->clear();

        return 1;
    }

    int GetInstalledThemeMetadata(std::string installedThemeJsonPath, installed_theme_data *themeData) {
        std::FILE *installedThemeJson;

        if (!(installedThemeJson = std::fopen(installedThemeJsonPath.c_str(), "rb"))) {
            WHBLogPrintf("Cannot open installed theme's json file.");

            return 0;
        }

        std::fseek(installedThemeJson, 0, SEEK_END);
        std::size_t jsonSize = std::ftell(installedThemeJson);
        std::rewind(installedThemeJson);
        
        std::string buffer(jsonSize, '\0');

        std::fread(&buffer[0], 1, jsonSize, installedThemeJson);
        std::fclose(installedThemeJson);
        
        std::unique_ptr<json::json> themeMetadata = std::make_unique<json::json>(json::json::parse(buffer));
 
        themeData->themeID = std::string(themeMetadata->at("ThemeData").at("themeID"));
        themeData->themeName = std::string(themeMetadata->at("ThemeData").at("themeName"));
        themeData->themeAuthor = std::string(themeMetadata->at("ThemeData").at("themeAuthor"));
        themeData->themeRegion = std::string(themeMetadata->at("ThemeData").at("themeRegion"));
        themeData->installedThemePath = std::string(themeMetadata->at("ThemeData").at("themeInstallPath"));

        themeMetadata->clear();

        return 1;
    }

    bool InstallTheme(std::string themePath, theme_data themeData) {
        bool themeInstallSuccess = false;
        
        auto [menuContentPath, sdcafiineMenuPath] = GetMenuStrings();
        
        zip_t *themeArchive;
        zip_error_t error;
        int err;
        
        if (!(themeArchive = zip_open(themePath.c_str(), 0, &err))) {
            zip_error_init_with_code(&error, err);

            WHBLogPrintf("Cannot open theme archive. Error Code: %s", zip_error_strerror(&error));

            zip_error_fini(&error);

            themeInstallSuccess = false;
        }

        zip_file_t *themeMetadataFile; 
        if (!(themeMetadataFile = zip_fopen(themeArchive, "metadata.json", ZIP_RDONLY))) {
            zip_error_init_with_code(&error, err);

            WHBLogPrintf("Cannot open theme metadata. Error Code: %s", zip_error_strerror(&error));

            zip_error_fini(&error);  

            themeInstallSuccess = false;          
        }

        zip_stat_t metadataStatData;
        
        if ((zip_stat(themeArchive, "metadata.json", 0, &metadataStatData)) != 0) {
            zip_error_init_with_code(&error, err);

            WHBLogPrintf("Cannot stat theme metadata! Error Code: %s", zip_error_strerror(&error));

            zip_error_fini(&error);   

            themeInstallSuccess = false;                     
        }
        
        std::string buffer(metadataStatData.size, '\0');

        zip_fread(themeMetadataFile, &buffer[0], metadataStatData.size);
        zip_fclose(themeMetadataFile);
        
        std::unique_ptr<json::json> themeMetadata = std::make_unique<json::json>(json::json::parse(buffer));

        std::string themeRegionStr = std::string(themeMetadata->at("Metadata").at("themeRegion"));

        WHBLogPrintf("Installing %s...", themeData.themeName.c_str());

        std::string modpackPath = sdcafiineMenuPath + themeData.themeID;
        WHBLogPrintf("Installing theme to: %s", modpackPath.c_str());

        for (auto& [patchFilename, menuFilepath] : themeMetadata->at("Patches").items()) {
            std::string menuPath = menuContentPath + std::string(menuFilepath);
            std::string cachePath = std::string(THEMIIFY_ROOT) + "/cache/" + std::string(menuFilepath);
            std::string patchPath = std::string(patchFilename);
            std::string outputPath = modpackPath + "/content/" + std::string(menuFilepath);

            CreateParentDirectories(cachePath);

            zip_file_t *patchFile;
            if (!(patchFile = zip_fopen(themeArchive, patchFilename.c_str(), ZIP_RDONLY))) {
                zip_error_init_with_code(&error, err);

                WHBLogPrintf("Cannot open %s!. Error Code: %s", patchFilename.c_str(), zip_error_strerror(&error));

                zip_error_fini(&error);  

                themeInstallSuccess = false;
                break;
            }

            zip_stat_t patchStatData;
            if ((zip_stat(themeArchive, patchFilename.c_str(), 0, &patchStatData)) != 0) {
                zip_error_init_with_code(&error, err);

                WHBLogPrintf("Cannot stat %s!. Error Code: %s", patchFilename.c_str(), zip_error_strerror(&error));

                zip_error_fini(&error);   

                themeInstallSuccess = false;  
                break;           
            }
            std::size_t patchSize = patchStatData.size;

            std::FILE *inputFile;
            if (!(inputFile = std::fopen(cachePath.c_str(), "rb"))) {
                WHBLogPrint("Cache does not exist");
                WHBLogPrintf("Creating cache for %s", menuPath.c_str());

                if (!(inputFile = std::fopen(menuPath.c_str(), "rb"))) {
                    WHBLogPrintf("Could not open source file for %s", std::string(patchFilename).c_str());
                    themeInstallSuccess = false;  
                    break;
                } else {
                    CreateCacheFile(inputFile, cachePath.c_str());
                }
            }
            else {
                WHBLogPrintf("Found %s in cache at %s", std::string(menuFilepath).c_str(), cachePath.c_str());
            }

            std::fseek(inputFile, 0, SEEK_END);
            std::size_t inputSize = std::ftell(inputFile);
            std::rewind(inputFile);

            std::vector<uint8_t> patchData(patchSize);
            std::vector<uint8_t> inputData(inputSize);

            zip_fread(patchFile, patchData.data(), patchSize);
            zip_fclose(patchFile);

            std::fread(inputData.data(), 1, inputSize, inputFile);
            std::fclose(inputFile);

            auto [bytes, result] = Hips::patch(inputData.data(), inputSize, patchData.data(), patchSize, Hips::PatchType::BPS);
            if (result == Hips::Result::Success) {
                CreateParentDirectories(outputPath);
                std::FILE *outputFile = std::fopen(outputPath.c_str(), "wb");
                std::fwrite(bytes.data(), 1, bytes.size(), outputFile);
                std::fclose(outputFile);

                themeInstallSuccess = true;

                WHBLogPrintf("File written to %s", outputPath.c_str());
            }
            else {
                WHBLogPrintf("Patch failed. Hips result: %d", result);

                themeInstallSuccess = false;
                break;
            }
        }

        themeMetadata->clear();

        zip_close(themeArchive);

        std::string installPath = std::string(THEMIIFY_INSTALLED_THEMES) + "/" + themeData.themeID + ".json";

        if (themeInstallSuccess) {
            WHBLogPrintf("Install Path: %s", installPath.c_str());
            CreateParentDirectories(installPath);
            json::json installedThemeJson;
            installedThemeJson["ThemeData"] = {
                {"themeName", themeData.themeName},
                {"themeAuthor", themeData.themeAuthor},
                {"themeID", themeData.themeID},
                {"themeRegion", themeRegionStr},
                {"themeInstallPath", modpackPath}
            };

            std::ofstream outFile(installPath);
            if (outFile.is_open()) {
                outFile << installedThemeJson.dump(4);
                outFile.close();
                WHBLogPrintf("%s saved to Themiify installed directory.", themeData.themeID.c_str());
            }
            else {
                // Not sure exactly what to tell the user here
                WHBLogPrintf("%s failed to save to Themiify installed directory!", themeData.themeID.c_str());
            }

            installedThemeJson.clear();
        }
        else {
            // In case the theme was somehow already installed
            // Would prolly also happen if two themes use the same theme id but one is broken... Not sure how I feel about that one but we'll have to see
            DeleteTheme(modpackPath, installPath);
        }

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
}