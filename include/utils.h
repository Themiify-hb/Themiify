/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <string>
#include <filesystem>
#include <algorithm>

#include <whb/log.h>

#define THEMIIFY_VERSION "vAlpha0"

#define THEMIIFY_ROOT "fs:/vol/external01/themiify"
#define THEMIIFY_INSTALLED_THEMES "fs:/vol/external01/themiify/installed"
#define THEMES_ROOT "fs:/vol/external01/wiiu/themes"

#define WII_U_MENU_JPN_TID (0x0005001010040000)
#define WII_U_MENU_USA_TID (0x0005001010040100)
#define WII_U_MENU_EUR_TID (0x0005001010040200)

#define MEN_PATH "Common/Package/Men.pack"
#define MEN2_PATH "Common/Package/Men2.pack"
#define CAFE_BARISTA_MEN_PATH "Common/Sound/Men/cafe_barista_men.bfsar"

#define BACKGROUND_COLOUR (0x00382AFF)
#define BACKGROUND_ERR_COLOUR (0x750000FF)
#define BACKGROUND_WARNING_COLOUR (0xAAAA00FF)
#define BACKGROUND_SUCCESS_COLOUR (0x007500FF)

inline bool CreateParentDirectories(std::string inputPath) {
    std::filesystem::path fullPath = inputPath;
    std::filesystem::path parentPath = fullPath.parent_path();
        
    if (!(std::filesystem::create_directories(parentPath))) {
        return false;
    }

    return true;
}

inline void DeletePath(std::string inputPath) {
    if (!std::filesystem::exists(inputPath)) {
        WHBLogPrintf("%s could not be found!", inputPath.c_str());
        return;
    }

    if (std::filesystem::is_directory(inputPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(inputPath)) {
            DeletePath(entry.path());
            WHBLogPrintf("Successfully deleted %s", entry.path().c_str());
        }
    }
    else {
        WHBLogPrintf("Deleting %s...", inputPath.c_str());
    }

    try {
        std::filesystem::remove(inputPath);
    }
    catch (const std::filesystem::filesystem_error& e) {
        WHBLogPrintf("Error deleting... Error: %s", e.what());
        return;
    }
}

inline std::string removeNonASCII(std::string s) {
    std::string str = s;
    str.erase(std::remove_if(str.begin(), str.end(), [](unsigned char c) { return c > 127; }), str.end());

    return str;
}

inline bool isAscii(std::string str) {
    for (unsigned char c : str) {
        if (c > 0x7F)
            return false; // Non-ASCII byte found
    }
    return true;    
}

