/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <vector>

#include "screen.h"
#include "installer.h"

class InstalledThemesScreen : public Screen
{
public:
    InstalledThemesScreen();
    virtual ~InstalledThemesScreen();

    void Draw();
    
    bool Update(VPADStatus status);
private:
    enum MenuState {
        MENU_STATE_INIT,
        MENU_STATE_DIR_ITERATOR,
        MENU_STATE_DEFAULT,
        MENU_STATE_THEME_SHOW_DETAILS,
        MENU_STATE_DELETE_THEME_PROMPT,
        MENU_STATE_THEME_DELETED
    };
        
    MenuState mMenuState;

    bool mMenuStateFailure, mNoInstalledThemesFound;

    std::vector<std::string> mFileList, mThemeNames, mThemeFullNames;
    std::vector<Installer::installed_theme_data> mThemeDataList;
        
    int mThemeIdx, mScrollOffset;

    std::string mCurrentTheme;

    Installer::installed_theme_data mSelectedThemeData;
};