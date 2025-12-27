/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <string>

namespace Installer {
    enum Region {
        JPN = 0,
        USA = 1,
        EUR = 2,
        UNIVERSAL = 3
    };

    struct theme_data {
        std::string themeID;
        std::string themeName;
        std::string themeAuthor;
        Region themeRegion;
    };

    struct installed_theme_data {
        std::string themeID;
        std::string themeName;
        std::string themeAuthor;
        std::string themeRegion;
        std::string installedThemePath;
    };

    Region GetSystemRegion();
    int GetThemeMetadata(std::string themePath, theme_data *themeData);
    int GetInstalledThemeMetadata(std::string installedThemeJsonPath, installed_theme_data *themeData);
    bool InstallTheme(std::string themePath, theme_data themeData);
    bool DeleteTheme(std::string modpackPath, std::string installPath);
}