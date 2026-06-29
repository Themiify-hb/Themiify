/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <print>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <zip.h>
#include <glaze/glaze.hpp>
#include <hips.hpp>
#include <mocha/mocha.h>

#include <sysapp/title.h>
#include <coreinit/systeminfo.h>

#include "installer.h"
#include "utils.h"

using std::cout;
using std::cerr;
using std::endl;
using namespace std::literals;

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

    std::filesystem::path GetMenuContentPath() {
        uint64_t menuTitleID = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);

        uint32_t menuIDParentDir = menuTitleID >> 32;
        uint32_t menuIDChildDir = menuTitleID;

        char splitMenuID[18];
        snprintf(splitMenuID, sizeof splitMenuID, "%08x/%08x", menuIDParentDir, menuIDChildDir);

        return "storage_mlc:/sys/title" / std::filesystem::path{splitMenuID} / "content";
    }

    void CreateCacheFile(std::ifstream &sourceFile, const std::filesystem::path &outputPath) {
        if (!sourceFile.is_open()) {
            cerr << "Invalid or unopened source file" << endl;
            return;
        }

        std::size_t sourceSize = static_cast<std::size_t>(sourceFile.tellg());
        sourceFile.seekg(0, std::ios::beg);

        std::vector<unsigned char> buffer(sourceSize);
        sourceFile.read(reinterpret_cast<char*>(buffer.data()), sourceSize);

        if (!sourceFile) {
            cerr << "Error reading source file to create cache file." << endl;
            return;
        }

        std::ofstream outputFile(outputPath, std::ios::binary | std::ios::trunc);
        if (!outputFile.is_open()) {
            cerr << "Failed to open output file for writing: " << outputPath << endl;
            return;
        }

        outputFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

        if (!outputFile) {
            cerr << "Error writing cache file to " << outputPath << endl;
            return;
        }

        cout << "Successfully cached file to: " << outputPath << endl;
    }

    int GetThemeMetadata(const std::filesystem::path &themePath, theme_data *themeData) {
        zip_t *themeArchive;
        zip_error_t error;
        int err;

        if (!(themeArchive = zip_open(themePath.c_str(), 0, &err))) {
            zip_error_init_with_code(&error, err);
            cerr << "Cannot open theme archive. Error Code: "
                 << zip_error_strerror(&error) << endl;
            zip_error_fini(&error);
            return 0;
        }

        zip_file_t *themeMetadataFile;
        if (!(themeMetadataFile = zip_fopen(themeArchive, "metadata.json", ZIP_RDONLY))) {
            zip_error_init_with_code(&error, err);
            cerr << "Cannot open theme metadata. Error Code: "
                 << zip_error_strerror(&error) << endl;
            zip_error_fini(&error);
            zip_close(themeArchive);
            return 0;
        }

        zip_stat_t metadataStatData;
        if (zip_stat(themeArchive, "metadata.json", 0, &metadataStatData) != 0) {
            zip_error_init_with_code(&error, err);
            cerr << "Cannot stat theme metadata! Error Code: "
                 << zip_error_strerror(&error) << endl;
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
            cerr << "Failed to parse metadata.json: "
                 << glz::format_error(err, buffer) << endl;
            return 0;
        }

        auto& metadata = themeMetadata.at("Metadata");

        themeData->themeID = GetString(metadata, "themeID");

        std::string themeIDPathStr = themeData->themeID;
        themeIDPathStr.erase(std::remove(themeIDPathStr.begin(), themeIDPathStr.end(), ':'), themeIDPathStr.end());
        themeData->themeIDPath = themeIDPathStr;

        themeData->themeName = GetString(metadata, "themeName");
        themeData->themeAuthor = GetString(metadata, "themeAuthor");
        themeData->themeVersion = GetString(metadata, "themeVersion");

        return 1;
    }

    int GetInstalledThemeMetadata(const std::filesystem::path &installedThemeJsonPath,
                                  installed_theme_data *themeData) {
        std::ifstream installedThemeJson{installedThemeJsonPath};

        if (!installedThemeJson.is_open()) {
            cerr << "Cannot open installed theme's json file." << endl;
            return 0;
        }

        std::string jsonStr = ReadWholeFile(installedThemeJson);

        glz::generic installedThemeMetadata;
        if (auto err = glz::read_json(installedThemeMetadata, jsonStr)) {
            cerr << "Failed to parse installed theme json: "
                 << glz::format_error(err, jsonStr) << endl;
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

    void InstallTheme(std::stop_token &stopper,
                      const std::filesystem::path &themePath,
                      theme_data themeData,
                      progress_function_t progressCallback,
                      success_function_t successCallback,
                      error_function_t errorCallback) {

        zip_t *themeArchive = nullptr;
        zip_file_t *patchFile = nullptr;
        std::filesystem::path modpackPath;
        std::filesystem::path installPath;

        try {

            auto reportProgress = [&progressCallback](const std::string &msg) {
                if (progressCallback)
                    progressCallback(msg);
                else
                    cout << msg << endl;
            };

            auto throwIfStopped = [&stopper] {
                if (stopper.stop_requested())
                    throw std::runtime_error{"Installation canceled."};
            };

            OSEnableHomeButtonMenu(FALSE);

            auto menuContentPath = GetMenuContentPath();

            zip_error_t error;
            int err;

            throwIfStopped();

            if (!(themeArchive = zip_open(themePath.c_str(), 0, &err))) {
                zip_error_init_with_code(&error, err);
                std::string msg = "Cannot open theme archive:"s + zip_error_strerror(&error);
                zip_error_fini(&error);
                throw std::runtime_error{msg};
            }

            throwIfStopped();

            reportProgress(std::format("Installing {}...", themeData.themeName));

            modpackPath = sanitize(THEMES_ROOT / (themeData.themeName + " (" + themeData.themeIDPath + ")"));

            reportProgress(std::format("Installing theme to: \"{}\"", modpackPath.string()));

            int64_t numEntries;
            if ((numEntries = zip_get_num_entries(themeArchive, ZIP_FL_UNCHANGED)) < 0) {
                zip_close(themeArchive);
                throw std::runtime_error{"themeArchive is NULL"};
            }

            for (uint64_t i = 0; i < static_cast<uint64_t>(numEntries); ++i) {

                throwIfStopped();

                std::filesystem::path menuFilePath;
                std::string entryName = zip_get_name(themeArchive, i, ZIP_FL_ENC_RAW);

                if (entryName == "Men.bps") {
                    menuFilePath = MEN_PATH;
                }
                else if (entryName == "Men2.bps") {
                    menuFilePath = MEN2_PATH;
                }
                else if (entryName == "cafe_barista_men.bps") {
                    menuFilePath = CAFE_BARISTA_MEN_PATH;
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
                        reportProgress(std::format("Unknown AllMessage Region and Language: \"{}\"", regionLangStr));
                        continue;
                    }

                    menuFilePath = it->second;
                }

                if (entryName != "metadata.json") {
                    reportProgress(std::format("menuFilePath: \"{}\"", menuFilePath.string()));

                    auto menuPath = menuContentPath / menuFilePath;
                    auto cachePath = THEMIIFY_ROOT / "cache" / menuFilePath;
                    auto patchPath = std::filesystem::path{entryName};
                    auto outputPath = modpackPath / "content" / menuFilePath;

                    CreateParentDirectories(cachePath);

                    if (!(patchFile = zip_fopen(themeArchive, patchPath.c_str(), ZIP_RDONLY))) {
                        zip_error_init_with_code(&error, err);
                        std::string msg = std::format("Cannot open \"{}\"!. Error: {}",
                                                      patchPath.string(),
                                                      zip_error_strerror(&error));
                        zip_error_fini(&error);
                        throw std::runtime_error{msg};
                    }

                    zip_stat_t patchStatData;
                    if (zip_stat(themeArchive, patchPath.c_str(), 0, &patchStatData) != 0) {
                        zip_error_init_with_code(&error, err);
                        std::string msg = std::format("Cannot stat \"{}\"!. Error: {}",
                                                      patchPath.string(),
                                                      zip_error_strerror(&error));
                        zip_error_fini(&error);
                        zip_fclose(patchFile);
                        throw std::runtime_error{msg};
                    }

                    throwIfStopped();

                    std::size_t patchSize = patchStatData.size;

                    std::ifstream inputFile(cachePath, std::ios::binary | std::ios::ate);
                    if (!inputFile.is_open()) {
                        reportProgress(std::format("Cache does not exist for \"{}\"",
                                                   menuPath.string()));

                        inputFile.clear();
                        inputFile.open(menuPath, std::ios::binary | std::ios::ate);

                        if (!inputFile.is_open()) {
                            inputFile.clear();
                            zip_fclose(patchFile);
                            // NOTE: don't error out, just report
                            reportProgress(std::format("Could not open source file for \"{}\"",
                                                       patchPath.string()));
                            continue;
                        }
                        reportProgress(std::format("Creating cache for \"{}\"",
                                                   menuPath.string()));
                        CreateCacheFile(inputFile, cachePath);
                    }
                    else {
                        reportProgress(std::format("Found \"{}\" in cache at \"{}\"",
                                                   menuFilePath.string(),
                                                   cachePath.string()));
                    }

                    throwIfStopped();

                    std::size_t inputSize = inputFile.tellg();
                    if (inputSize != inputFile.tellg())
                        throw std::runtime_error{"Input file is too large."};
                    inputFile.seekg(0, std::ios::beg);

                    throwIfStopped();

                    std::vector<uint8_t> patchData(patchSize);
                    std::vector<uint8_t> inputData(inputSize);

                    zip_fread(patchFile, patchData.data(), patchData.size());
                    zip_fclose(patchFile);

                    throwIfStopped();

                    inputFile.read(reinterpret_cast<char*>(inputData.data()), inputData.size());
                    inputFile.close();

                    throwIfStopped();

                    auto [bytes, result] = Hips::patch(
                        inputData.data(),
                        inputData.size(),
                        patchData.data(),
                        patchData.size(),
                        Hips::PatchType::BPS
                    );

                    throwIfStopped();

                    if (result == Hips::Result::Success) {
                        CreateParentDirectories(outputPath);

                        std::ofstream outputFile(outputPath, std::ios::binary);
                        outputFile.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
                        outputFile.close();

                        reportProgress(std::format("File written to \"{}\"",
                                                   outputPath.string()));
                    }
                    else {
                        throw std::runtime_error{std::format("Patch failed! Hips result: {}",
                                                             static_cast<unsigned>(result))};
                    }
                }
            }

            zip_close(themeArchive);
            themeArchive = nullptr;

            throwIfStopped();

            installPath = THEMIIFY_INSTALLED_THEMES / (themeData.themeIDPath + ".json");

            reportProgress(std::format("Creating install metadata: \"{}\"",
                                       installPath.string()));
            CreateParentDirectories(installPath);

            // TODO: use a struct
            glz::generic installedThemeJson;
            installedThemeJson["ThemeData"]["themeName"] = themeData.themeName;
            installedThemeJson["ThemeData"]["themeAuthor"] = themeData.themeAuthor;
            installedThemeJson["ThemeData"]["themeID"] = themeData.themeID;
            installedThemeJson["ThemeData"]["themeIDPath"] = themeData.themeIDPath;
            installedThemeJson["ThemeData"]["themeVersion"] = themeData.themeVersion;
            installedThemeJson["ThemeData"]["themeInstallPath"] = modpackPath;

            auto jsonStr = glz::write<glz::opts{.prettify = true}>(installedThemeJson);
            if (!jsonStr) {
                throw std::runtime_error{std::format("Failed to generate install metadata for \"{}\"",
                                                     themeData.themeName)};
            }
            else {
                std::ofstream outFile(installPath);
                if (outFile.is_open()) {
                    outFile << *jsonStr;
                    outFile.close();
                    reportProgress(std::format("Finished install for \"{}\".",
                                               themeData.themeName));
                }
                else {
                    throw std::runtime_error{std::format("Failed to save install metadata for \"{}\"",
                                                         themeData.themeName)};
                }
            }

            OSEnableHomeButtonMenu(TRUE);

            if (successCallback)
                successCallback();
        }
        catch (std::exception &e) {
            if (themeArchive)
                zip_close(themeArchive);
            cerr << "Deleting theme: " << modpackPath << " and " << installPath << endl;
            DeleteTheme(modpackPath, installPath);
            if (errorCallback)
                errorCallback(e);
            else
                cerr << "ERROR: " << e.what() << endl;
            OSEnableHomeButtonMenu(TRUE);
        }
    }

    bool DeleteTheme(const std::filesystem::path &modpackPath,
                     const std::filesystem::path &installPath) {
        std::filesystem::path thumbnailPath;
        if (!modpackPath.empty())
            DeletePath(modpackPath);
        if (!installPath.empty()) {
            DeletePath(installPath);
            thumbnailPath = THEMIIFY_THUMBNAILS / installPath.stem();
            thumbnailPath.replace_extension(".webp");
            DeletePath(thumbnailPath);
        }

        if ((!modpackPath.empty() && exists(modpackPath))
            ||
            (!installPath.empty() && exists(installPath))
            ||
            (!thumbnailPath.empty() && exists(thumbnailPath)))
            return false;

        return true;
    }

    std::string GetStyleMiiUConfigPath() {
        char environmentPathBuffer[0x100];

        MochaUtilsStatus res;
        if ((res = Mocha_GetEnvironmentPath(environmentPathBuffer, sizeof(environmentPathBuffer))) != MOCHA_RESULT_SUCCESS) {
            cerr << "Failed to get environment path. Are you running on Aroma? Result: "
                 << Mocha_GetStatusStr(res) << endl;
            return ""; // TOOD: should we use the default aroma path as fallback?
        }

        return std::string(environmentPathBuffer) + "/plugins/config/style-mii-u.json";
    }

    bool SetCurrentTheme(const std::string &themeName, const std::string &themeID) {
        std::string styleMiiUConfigPath = GetStyleMiiUConfigPath();

        std::ifstream configFile(styleMiiUConfigPath);
        if (!configFile.is_open()) {
            cerr << "Failed to open config file: " << styleMiiUConfigPath << endl;
            return false;
        }

        std::string jsonStr = ReadWholeFile(configFile);
        configFile.close();

        glz::generic configJson;
        if (auto err = glz::read_json(configJson, jsonStr)) {
            cerr << "Failed to parse config file: "
                 << glz::format_error(err, jsonStr) << endl;
            return false;
        }

        configJson["storageitems"]["enabledThemes"] = sanitize_element(themeName + " (" + themeID + ")");

        auto outputJson = glz::write<glz::opts{.prettify = true}>(configJson);
        if (!outputJson) {
            cerr << "Failed to serialize config json" << endl;
            return false;
        }

        std::ofstream outFile(styleMiiUConfigPath, std::ios::trunc);
        if (!outFile.is_open()) {
            cerr << "Failed to open for write: " << styleMiiUConfigPath << endl;
            return false;
        }

        outFile << *outputJson;
        outFile.close();

        std::println("Succesfully set {} as current StyleMiiU theme!", themeName);

        return true;
    }

    std::string GetCurrentTheme() {
        std::string styleMiiUConfigPath = GetStyleMiiUConfigPath();

        std::ifstream configFile(styleMiiUConfigPath);
        if (!configFile.is_open()) {
            cerr << "Failed to open config file: " << styleMiiUConfigPath << endl;
            return "";
        }

        std::string jsonStr = ReadWholeFile(configFile);
        configFile.close();

        glz::generic configJson;
        if (auto err = glz::read_json(configJson, jsonStr)) {
            cerr << "Failed to parse config file: "
                 << glz::format_error(err, jsonStr) << endl;
            return "";
        }

        return configJson.at("storageitems").at("enabledThemes").get<std::string>();
    }

} // namespace Installer
