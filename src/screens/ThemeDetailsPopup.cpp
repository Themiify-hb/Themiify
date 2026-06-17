/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <SDL2/SDL.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_raii.h>

#include "ThemeDetailsPopup.h"
#include "ThemePreviewPopup.h"
#include "DownloadThemePopup.h"
#include "DeleteThemePopup.h"
#include "ManageThemesScreen.h"
#include "../App.h"
#include "../installer.h"
#include "../DownloadManager.h"
#include "../IconsFontAwesome4.h"
#include "../ImageLoader.h"
#include "../NavBar.h"
#include "../ThemezerAPI.h"
#include "../tracer.hpp"

using std::cout;
using std::endl;
using namespace std::literals;

using ThemezerAPI::WiiuThemeFull;
using ThemezerAPI::WiiuThemeSmall;

namespace ThemeDetailsPopup {
    enum class State {
        hidden,
        waiting,
        ready_themezer,
        ready_local,
        error,
    };

    bool popup_queued;
    State state;
    std::string hexId;
    std::string error;
    WiiuThemeFull theme;
    WiiuThemeSmall smallTheme;
    Installer::installed_theme_data installedThemeData;
    SDL_Texture *localPreview;
    const std::string popup_id = "ThemeDetailsPopup"s;
    bool isCurrent;

    void show_themezer(const std::string& request_id, const WiiuThemeSmall &small_theme) {
        popup_queued = true;
        hexId = request_id;
        state = State::hidden;
        error.clear();
        theme = {};
        smallTheme = small_theme;
        ThemezerAPI::wiiu::theme(hexId,
                                 [](const WiiuThemeFull& t)
                                 {
                                    cout << "Got theme!" << endl;
                                    theme = t;
                                    state = State::ready_themezer;
                                 });
        state = State::waiting;
    }

    void show_local(Installer::installed_theme_data installed_theme_data, SDL_Texture *local_preview, bool is_current) {
        popup_queued = true;
        installedThemeData = installed_theme_data;
        localPreview = local_preview;
        state = State::ready_local;
        isCurrent = is_current;
    }

    void process_ui() {
        if (state == State::hidden)
            return;

        if (popup_queued) {
            ImGui::OpenPopup(popup_id);
            popup_queued = false;
        }

        auto center = ImGui::GetMainViewport()->GetCenter();
        auto *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowSize({viewport->Size.x * 0.85f, 0.0f}, ImGuiCond_Always);
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5f, 0.5f});
        ImGui::RAII::Popup popup{popup_id, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoMove};
        
        if (!popup) {
            state = State::hidden;
            return;
        }
        
        switch (state) {
            case State::waiting:
                ImGui::Text("Fetching theme details...");
                break;
            
            case State::error:
                ImGui::TextWrapped("Error: %s", error.data());
                break;
            
            case State::ready_themezer: {
                ImGui::Text("Theme details");
                ImGui::Separator();
                
                ImGui::TextWrapped("Name: %s", theme.name.data());
                
                ImGui::TextWrapped("Created: %s", theme.createdAt.data());
                if (!theme.updatedAt.empty() && theme.updatedAt != theme.createdAt)
                ImGui::TextWrapped("Updated: %s", theme.updatedAt.data());
                
                ImGui::Text("Downloads: %u", theme.downloadCount);
                
                ImGui::TextWrapped("Author: %s", theme.creator.username.data());
                
                if (theme.creator.avatarUrl) {
                    ImGui::SameLine();
                    auto avatar = ImageLoader::get(*theme.creator.avatarUrl);
                    ImGui::Image((ImTextureID)avatar, {64, 64});
                    ImGui::SetItemTooltip(*theme.creator.avatarUrl);
                }
                
                ImGui::Text("Tags:");
                ImGui::Indent();
                for (auto& tag : theme.tags)
                ImGui::Text(ICON_FA_TAG " %s", tag.name.data());
                ImGui::Unindent();
                
                auto collageImg = ImageLoader::get(theme.collagePreview.sdUrl);
                float collageWidth = 720;
                ImGui::SetCursorPosX(
                    ImGui::GetCursorPosX() +
                    (ImGui::GetContentRegionAvail().x - collageWidth) * 0.5f
                );

                {
                    ImGui::RAII::StyleVar no_padding{ImGuiStyleVar_FramePadding, {0.0f, 0.0f}};
                    if (ImGui::ImageButton("collagePreviewSD",
                                           (ImTextureID)collageImg,
                                           {collageWidth, 405})) {
                        ThemePreviewPopup::show(theme.launcherScreenshot.hdUrl, theme.waraWaraPlazaScreenshot.hdUrl);
                    }
                }

                ImGui::Separator();
                
                if (ImGui::Button("Download")) {
                    DownloadThemePopup::show(smallTheme);
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Preview Theme")) {
                    ThemePreviewPopup::show(theme.launcherScreenshot.hdUrl, theme.waraWaraPlazaScreenshot.hdUrl);
                }
                
                ImGui::Spacing();
                
                break;
            }

            case State::ready_local: {
                ImGui::Text("Theme details");
                ImGui::Separator();
                
                ImGui::TextWrapped("Name: %s", installedThemeData.themeName.c_str());
                ImGui::TextWrapped("Author: %s", installedThemeData.themeAuthor.c_str());
                ImGui::TextWrapped("Theme Version: %s", installedThemeData.themeVersion.c_str());
                
                float collageWidth = 720;
                ImGui::SetCursorPosX(
                    ImGui::GetCursorPosX() +
                    (ImGui::GetContentRegionAvail().x - collageWidth) * 0.5f
                );

                {
                    ImGui::RAII::StyleVar no_padding{ImGuiStyleVar_FramePadding, {0.0f, 0.0f}};
                    ImGui::ImageButton("collagePreviewSD",
                                        (ImTextureID)localPreview,
                                        {collageWidth, 405});
                }

                ImGui::Separator();
                
                {
                    ImGui::RAII::Disabled disabled_if{isCurrent};
                    if (ImGui::Button(ICON_FA_STAR " Make Default")) {
                        Installer::SetCurrentTheme(installedThemeData.themeName, installedThemeData.themeIDPath);
                        ImGui::CloseCurrentPopup();
                        ManageThemesScreen::force_refresh();
                    }
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button(ICON_FA_TRASH " Delete")) {
                    std::filesystem::path themeJsonPath = std::string(THEMIIFY_INSTALLED_THEMES) + "/" + installedThemeData.themeIDPath + ".json";
                    DeleteThemePopup::show(installedThemeData, themeJsonPath);
                }
                
                ImGui::Spacing();
                
                break;
            }            
            
            default:
            break;
        }
        
        ThemePreviewPopup::process_ui();
    }
}
