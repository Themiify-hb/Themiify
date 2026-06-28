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
    std::filesystem::path thumbnail_path;
    const std::string popup_id = "ThemeDetailsPopup"s;
    bool isCurrent;

    void open_themezer(const std::string& request_id, const WiiuThemeSmall &small_theme) {
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

    void open_local(Installer::installed_theme_data installed_theme_data,
                    const std::filesystem::path& thumbnail_path_,
                    bool is_current) {
        popup_queued = true;
        installedThemeData = installed_theme_data;
        thumbnail_path = thumbnail_path_;
        state = State::ready_local;
        isCurrent = is_current;
    }

    void show_label_text(const std::string& label,
                         const std::string& text)
    {
        const auto colors = ImGui::GetStyle().Colors;
        {
            ImGui::RAII::StyleColor label_color{ImGuiCol_Text, colors[ImGuiCol_HeaderActive]};
            ImGui::Text(label);
        }
        ImGui::SameLine();
        ImGui::TextWrapped(text);
    }

    void show_local() {
        using namespace ImGui::RAII;
        const auto &style = ImGui::GetStyle();

        // Split the window into two child windows: left and right
        const float total_width = ImGui::GetContentRegionAvail().x;
        float right_width = 300;
        float left_width = total_width - right_width - style.ItemSpacing.x;

        if (Child left{"left", {left_width, 0}, ImGuiChildFlags_NavFlattened}) {
            {
                Font title_font{nullptr, 50};
                ImGui::TextWrapped(installedThemeData.themeName);
            }
            ImGui::TextWrapped("by %s", installedThemeData.themeAuthor.data());

            // Put content in a scrollable child window.
            if (Child content{"content", {0, 0},
                              ImGuiChildFlags_NavFlattened
                              | ImGuiChildFlags_AlwaysUseWindowPadding}) {
                // Always scroll to the top when the popup first appears.
                if (ImGui::IsWindowAppearing())
                    ImGui::SetScrollY(0);

                ImVec2 preview_size {640, 360};
                auto available = ImGui::GetContentRegionAvail();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (available.x - preview_size.x) / 2);
                auto img = ImageLoader::get(thumbnail_path);
                ImGui::Image((ImTextureID)img, preview_size);
            }
        }

        ImGui::SameLine();

        if (Child right{"right", {right_width, 0}, ImGuiChildFlags_NavFlattened}) {
            {
                // Calculate how much space is needed for the buttons at the bottom.
                const int num_buttons = 3;
                const float buttons_height = num_buttons * ImGui::GetFrameHeightWithSpacing();

                // Put content in a scrollable child window.
                if (Child content{"content", {0 , -buttons_height},
                                  ImGuiChildFlags_NavFlattened
                                  | ImGuiChildFlags_AlwaysUseWindowPadding}) {
                    Font small{nullptr, 24};
                    show_label_text(ICON_FA_CALENDAR_CHECK_O,
                                    installedThemeData.themeVersion.substr(0, 10));
                    ImGui::SetItemTooltip("Theme version.");
                }

                {
                    Disabled disabled_if{isCurrent};
                    if (ImGui::Button(ICON_FA_STAR " Make Default", {-1, 0})) {
                        Installer::SetCurrentTheme(installedThemeData.themeName,
                                                   installedThemeData.themeIDPath);
                        ImGui::CloseCurrentPopup();
                        ManageThemesScreen::force_refresh();
                    }
                    ImGui::SetItemDefaultFocus();
                }

                if (ImGui::Button(ICON_FA_TRASH " Delete", {-1,0})) {
                    auto themeJsonPath =
                        THEMIIFY_INSTALLED_THEMES / (installedThemeData.themeIDPath + ".json");
                    DeleteThemePopup::show(installedThemeData, themeJsonPath);
                }

                if (ImGui::Button("Close", {-1, 0}))
                    ImGui::CloseCurrentPopup();
            }
        }
    }

    void show_themezer() {
        using namespace ImGui::RAII;
        const auto &style = ImGui::GetStyle();

        // Split the window into two child windows: left and right
        const float total_width = ImGui::GetContentRegionAvail().x;
        float right_width = 300;
        float left_width = total_width - right_width - style.ItemSpacing.x;

        if (Child left{"left", {left_width, 0},
                       ImGuiChildFlags_NavFlattened}) {
            {
                Font title_font{nullptr, 50};
                ImGui::TextWrapped(theme.name);
            }
            ImGui::TextWrapped("by %s", theme.creator.username.data());

            // Put content in a scrollable child window.
            if (Child content{"content",
                              {0, 0},
                              ImGuiChildFlags_NavFlattened
                              | ImGuiChildFlags_AlwaysUseWindowPadding}) {
                // Always scroll to the top when the popup first appears.
                if (ImGui::IsWindowAppearing())
                    ImGui::SetScrollY(0);

                auto collage_img = ImageLoader::get(theme.collagePreview.sdUrl);
                ImVec2 collage_size = {640, 360};
                StyleVar no_padding{ImGuiStyleVar_FramePadding, {0, 0}};
                // Center the image.
                auto available = ImGui::GetContentRegionAvail();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (available.x - collage_size.x) / 2);
                if (ImGui::ImageButton("collagePreviewSD",
                                       (ImTextureID)collage_img,
                                       collage_size))
                    ThemePreviewPopup::show(theme.hexId,
                                            {
                                                theme.collagePreview.hdUrl,
                                                theme.launcherScreenshot.hdUrl,
                                                theme.waraWaraPlazaScreenshot.hdUrl
                                            });

                if (theme.description) {
                    Font small{nullptr, 24};
                    ImGui::TextWrapped(*theme.description);
                }
            }

        }

        ImGui::SameLine();

        if (Child right{"right", {right_width, 0},
                        ImGuiChildFlags_NavFlattened}) {

            // Calculate how much space is needed for the buttons at the bottom.
            const int num_buttons = 2;
            const float buttons_height = num_buttons * ImGui::GetFrameHeightWithSpacing();

            // Put content in a scrollable child window.
            if (Child content{"content", {0, -buttons_height},
                              ImGuiChildFlags_NavFlattened |
                              ImGuiChildFlags_AlwaysUseWindowPadding}) {
                // Always scroll to the top when the popup first appears.
                if (ImGui::IsWindowAppearing())
                    ImGui::SetScrollY(0);

                Font small{nullptr, 24};
                StyleVar spacing{ImGuiStyleVar_ItemSpacing, {12, 10}};
                show_label_text("QuickID:", theme.quickId);
                show_label_text(ICON_FA_CALENDAR_PLUS_O, theme.createdAt.substr(0, 10));
                ImGui::SetItemTooltip("Creation date");
                show_label_text(ICON_FA_CALENDAR_CHECK_O,theme.updatedAt.substr(0, 10));
                ImGui::SetItemTooltip("Update date");
                show_label_text(ICON_FA_DOWNLOAD, std::to_string(theme.downloadCount));
                ImGui::SetItemTooltip("Number of downloads");
                if (!theme.tags.empty()) {
                    ImGui::Separator();
                    for (auto& tag : theme.tags)
                        show_label_text(ICON_FA_TAG, tag.name);
                }
            }

            if (ImGui::Button("Download", {-1, 0}))
                DownloadThemePopup::show(smallTheme);
            ImGui::SetItemDefaultFocus();

            if (ImGui::Button("Close", {-1, 0}))
                ImGui::CloseCurrentPopup();
        }
    }

    void process_ui() {
        using namespace ImGui::RAII;

        if (state == State::hidden)
            return;

        if (popup_queued) {
            ImGui::OpenPopup(popup_id);
            popup_queued = false;
        }

        auto viewport = ImGui::GetMainViewport();
        auto center = viewport->GetCenter();
        ImGui::SetNextWindowSize({1140, 680}, ImGuiCond_Always);
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5f, 0.5f});
        Popup popup{popup_id,
                    ImGuiWindowFlags_NoSavedSettings |
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

            case State::ready_themezer:
                show_themezer();
                break;

            case State::ready_local:
                show_local();
                break;

            default:
                ;
        }

        ThemePreviewPopup::process_ui();
    }
}
