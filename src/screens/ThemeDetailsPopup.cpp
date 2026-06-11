/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_raii.h>

#include "ThemeDetailsPopup.h"
#include "ThemePreviewPopup.h"
#include "../App.h"
//#include "../DownloadManager.h"
#include "../IconsFontAwesome4.h"
#include "../ImageLoader.h"
#include "../NavBar.h"
#include "../ThemezerAPI.h"
#include "../tracer.hpp"

using std::cout;
using std::endl;
using namespace std::literals;

using ThemezerAPI::WiiuThemeFull;

namespace ThemeDetailsPopup {
    enum class State {
        hidden,
        waiting,
        ready,
        error,
    };

    bool popup_queued;
    State state;
    std::string hexId;
    std::string error;
    WiiuThemeFull theme;
    const std::string popup_id = "ThemeDetailsPopup"s;

    void show(const std::string& request_id) {
        popup_queued = true;
        hexId = request_id;
        state = State::hidden;
        error.clear();
        theme = {};
        ThemezerAPI::wiiu::theme(hexId,
                                 [](const WiiuThemeFull& t)
                                 {
                                    cout << "Got theme!" << endl;
                                    theme = t;
                                    state = State::ready;
                                 });
        state = State::waiting;
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
            
            case State::ready: {
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
                    // download stuff here
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Preview Theme")) {
                    ThemePreviewPopup::show(theme.launcherScreenshot.hdUrl, theme.waraWaraPlazaScreenshot.hdUrl);
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
