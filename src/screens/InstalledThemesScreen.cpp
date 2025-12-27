/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <filesystem>
#include <vector>

#include "gfx.h"
#include "installer.h"
#include "utils.h"
#include "InstalledThemesScreen.h"

InstalledThemesScreen::InstalledThemesScreen()
    :   mMenuState(MENU_STATE_INIT),
        mMenuStateFailure(false),
        mThemeIdx(0),
        mScrollOffset(0)
{
}

InstalledThemesScreen::~InstalledThemesScreen()
{
}

void InstalledThemesScreen::Draw()
{
    switch (mMenuState) {
        case MENU_STATE_INIT:
            break;
        case MENU_STATE_DIR_ITERATOR:
            if (mNoInstalledThemesFound) {
                Gfx::SetBackgroundColour(BACKGROUND_ERR_COLOUR);
                Gfx::Print(-4, 2, "Could not find any themes installed at:\n%s", THEMIIFY_INSTALLED_THEMES);
                Gfx::Print(-4, 17, "                             B - Back");
            }
            else if (mMenuStateFailure) {
                Gfx::SetBackgroundColour(BACKGROUND_ERR_COLOUR);
                Gfx::Print(-4, 2, "Error retrieving the metadata for your\ninstalled themes.");
                Gfx::Print(-4, 17, "                             B - Back");
            }
            else {
                Gfx::Print(-4, 2, "Iterating through\n%s...", THEMIIFY_INSTALLED_THEMES);
            }

            break;
        case MENU_STATE_DEFAULT: {
            Gfx::SetBackgroundColour(BACKGROUND_COLOUR);
            Gfx::Print(-3, 2, "Select a theme to it set as your current theme, uninstall it\nor view information about it.");

            int yIni = 5;
            for (std::size_t i = 0; i < 12 && (mScrollOffset + i) < mFileList.size(); i++) {
                int y = yIni + i;
                std::size_t themeIdx = mScrollOffset + i;
                if (mThemeIDs.at(themeIdx) == mCurrentTheme) {
                    Gfx::Print(-4, y, (mThemeIdx == static_cast<int>(themeIdx)) ? "> %s (Current Theme)" : "  %s (Current Theme)", mThemeNames.at(themeIdx).c_str());
                }
                else {
                    Gfx::Print(-4, y, (mThemeIdx == static_cast<int>(themeIdx)) ? "> %s" : "  %s", mThemeNames.at(themeIdx).c_str());
                }
            }

            Gfx::Print(-4, 17, "A - Select                D-Pad/Stick - Move               B - Back");
            break;            
        }
        case MENU_STATE_THEME_SHOW_DETAILS:
            Gfx::SetBackgroundColour(BACKGROUND_COLOUR);
            // TODO: Figure out the "select default theme" appraoch we're gonna do
            Gfx::Print(-4, 2, "Theme Details:");
            Gfx::Print(-3, 4, "Theme Name: %s\nTheme Author: %s\nTheme Region: %s\nTheme ID: %s\n\nTheme was installed to:\n%s", mSelectedThemeData.themeName.c_str(), mSelectedThemeData.themeAuthor.c_str(), RegionToString(mSelectedThemeData.themeRegion).c_str(), mSelectedThemeData.themeID.c_str(), mSelectedThemeData.installedThemePath.c_str());
            if (mSelectedThemeData.themeID == mCurrentTheme) {
                Gfx::Print(-3, 12, "This Theme has been set as the current theme.");
                Gfx::Print(-4, 17, "          X - Uninstall Theme                    B - Back");
            }
            else {
                Gfx::Print(-4, 17, "X - Uninstall Theme       Y - Set as Current Theme       B - Back");
            }

            break;
        case MENU_STATE_DELETE_THEME_PROMPT:
            Gfx::SetBackgroundColour(BACKGROUND_WARNING_COLOUR);
            Gfx::Print(-4, 2, "Warning!\n\nProceeding will delete the theme:\n%s\nfrom your SD Card.\n\nThis includes both its theme data, located at:\n%s\nand any metadata saved by Themiify.\n\nWould you like to proceed with the uninstallation?", mSelectedThemeData.themeName.c_str(), mSelectedThemeData.installedThemePath.c_str());
            Gfx::Print(-4, 17, "          A - Uninstall Theme                    B - Back");

            break;
        case MENU_STATE_THEME_DELETED:
            if (mMenuStateFailure) {
                Gfx::SetBackgroundColour(BACKGROUND_ERR_COLOUR);

                Gfx::Print(-4, 2, "Error uninstalling %s\nPossible sign of SD Card corruption...\n\nYou may have to manually delete the theme yourself.", mSelectedThemeData.themeName.c_str());
                Gfx::Print(-4, 17, "                             B - Back");
            }
            else {
                Gfx::SetBackgroundColour(BACKGROUND_SUCCESS_COLOUR);

                Gfx::Print(-4, 2, "Successfully uninstalled the theme:\n%s", mSelectedThemeData.themeName.c_str());
                Gfx::Print(-4, 17, "                          A/B - Continue");
            }

            break;

    }
}

bool deleteRes;

bool InstalledThemesScreen::Update(VPADStatus status)
{   
    Installer::installed_theme_data themeData;
    
    switch (mMenuState) {
        case MENU_STATE_INIT:
            std::filesystem::create_directories(THEMIIFY_INSTALLED_THEMES);
            mNoInstalledThemesFound = false;
            mMenuState = MENU_STATE_DIR_ITERATOR;
            break;
        case MENU_STATE_DIR_ITERATOR:
            mFileList.clear();

            for (const auto & entry : std::filesystem::directory_iterator(THEMIIFY_INSTALLED_THEMES)) {
                if (entry.path().extension() == ".json") {
                    mFileList.push_back(entry.path());
                }
            }

            for (auto listIt = mFileList.begin(); listIt != mFileList.end(); ) {
                if ((Installer::GetInstalledThemeMetadata(*listIt, &themeData)) == 0) {
                    mMenuStateFailure = true;
                    break;
                }

                if (!std::filesystem::exists(themeData.installedThemePath)) {
                    // Modpack doesn't exist so we should delete the json because the user "uninstalled" the theme themselves
                    DeletePath(*listIt);
                    listIt = mFileList.erase(listIt);
                }
                else {
                    mThemeDataList.push_back(themeData);
                    mThemeNames.push_back(themeData.themeName);
                    mThemeIDs.push_back(themeData.themeID);
                    ++listIt;
                }  
            }

            if (mFileList.size() == 0) {
                mNoInstalledThemesFound = true;    
            }

            if (!mMenuStateFailure & !mNoInstalledThemesFound) {
                mCurrentTheme = Installer::GetCurrentTheme();
                mMenuState = MENU_STATE_DEFAULT;
                break;
            }
            else {
                if (status.trigger & VPAD_BUTTON_B) {
                    return false;
                }
            }

            break;
        case MENU_STATE_DEFAULT:
            if (status.trigger & (VPAD_STICK_L_EMULATION_UP | VPAD_BUTTON_UP)) {
                mThemeIdx--;
            }
            else if (status.trigger & (VPAD_STICK_L_EMULATION_DOWN | VPAD_BUTTON_DOWN)) {
                mThemeIdx++;
            }
            
            if (status.trigger & VPAD_BUTTON_A) {
                mSelectedThemeData = mThemeDataList.at(mThemeIdx);
                mMenuState = MENU_STATE_THEME_SHOW_DETAILS;
            }
            else if (status.trigger & VPAD_BUTTON_B) {
                return false;
            }

            if (mThemeIdx < 0) {
                mThemeIdx = mFileList.size() - 1;
            }            
            else if (mThemeIdx > static_cast<int>(mFileList.size() - 1)) {
                mThemeIdx = 0;
            }

            if (mThemeIdx < static_cast<int>(mScrollOffset)) {
                mScrollOffset = mThemeIdx;
            }
            else if (mThemeIdx >= static_cast<int>(mScrollOffset + 12)) {
                mScrollOffset = mThemeIdx - 12 + 1;
            }

            break;
        case MENU_STATE_THEME_SHOW_DETAILS:
            if (status.trigger & VPAD_BUTTON_B) {
                mMenuState = MENU_STATE_DIR_ITERATOR;
            }
            else if (status.trigger & VPAD_BUTTON_X) {
                mMenuState = MENU_STATE_DELETE_THEME_PROMPT;
            }
            else if ((mSelectedThemeData.themeID != mCurrentTheme) && (status.trigger & VPAD_BUTTON_Y)) {
                Installer::SetCurrentTheme(mSelectedThemeData.themeID);
                mMenuState = MENU_STATE_DIR_ITERATOR;
            }

            break;
        case MENU_STATE_DELETE_THEME_PROMPT:
            if (status.trigger & VPAD_BUTTON_A) {
                deleteRes = Installer::DeleteTheme(mSelectedThemeData.installedThemePath, mFileList.at(mThemeIdx));
                mMenuState = MENU_STATE_THEME_DELETED;
            }
            else if (status.trigger & VPAD_BUTTON_B) {
                mMenuState = MENU_STATE_THEME_SHOW_DETAILS;
            }

            break;
        case MENU_STATE_THEME_DELETED:
            if (!deleteRes) {
                mMenuStateFailure = true;
            }    

            if (mMenuStateFailure) {
                if (status.trigger & VPAD_BUTTON_B) {
                    mMenuState = MENU_STATE_DIR_ITERATOR;
                }
            }
            else {
                if (status.trigger & (VPAD_BUTTON_A | VPAD_BUTTON_B)) {
                    mMenuState = MENU_STATE_DIR_ITERATOR;
                }
            }

            break;
                
    }

    return true;
}

std::string InstalledThemesScreen::RegionToString(std::string region)
{
    std::string regionStr;

    if (region.compare("JPN") == 0) {
        regionStr = "Japan";
    }
    else if (region.compare("USA") == 0) {
        regionStr = "America";
    }
    else if (region.compare("EUR") == 0) {
        regionStr = "Europe";
    }
    else if (region.compare("UNIVERSAL") == 0) {
        regionStr = "Universal";
    }

    return regionStr;
}
