/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <vector>
#include <string>

#include "screen.h"
#include "installer.h"
#include "MainScreen.h"

class InstallThemeScreen : public Screen
{
public:
    InstallThemeScreen();
    virtual ~InstallThemeScreen();

    void Draw();
    
    bool Update(VPADStatus status);

private:
    enum MenuState {
        MENU_STATE_INIT,
        MENU_STATE_DIR_ITERATOR,
        MENU_STATE_DEFAULT,
        MENU_STATE_INSTALL_THEME_PROMPT,
        MENU_STATE_INSTALLING_THEME,
        MENU_STATE_THEME_INSTALL_SUCCESS,
        MENU_STATE_THEME_INSTALL_ERROR
    };

    std::string RegionToString(Installer::Region region);
    bool IsThemeAlreadyInstalled();

    MenuState mMenuState;
    bool mMenuStateFailure, mThemeRegionMismatch;

    int mFileIdx, mScrollOffset;

    std::vector<std::string> mFileList, mFileListFullPath;

    std::string mSelectedPath, mSelectedPathShort, mThemeName, mThemeAuthor, mThemeID;

    Installer::theme_data mThemeData;

    Installer::Region mSystemRegion, mThemeRegion;
};