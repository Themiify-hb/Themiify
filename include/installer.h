#pragma once

#include <string>
#include <filesystem>

namespace Installer {
    struct theme_data {
        std::string themeID;
        std::string themeIDPath;
        std::string themeName;
        std::string themeAuthor;
        std::string themeVersion;
    };

    struct installed_theme_data {
        std::string themeID;
        std::string themeIDPath;
        std::string themeName;
        std::string themeAuthor;
        std::string themeVersion;
        std::string installedThemePath;
    };

    int GetThemeMetadata(std::string themePath, theme_data *themeData);
    int GetInstalledThemeMetadata(std::string installedThemeJsonPath, installed_theme_data *themeData);
    bool InstallTheme(std::filesystem::path themePath, theme_data themeData);
    bool DeleteTheme(std::string modpackPath, std::string installPath);
    bool SetCurrentTheme(std::string themeName, std::string themeAuthor);
    std::string GetCurrentTheme();
}