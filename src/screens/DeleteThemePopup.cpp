/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cmath>
#include <string>

#include <imgui.h>
#include <imgui_raii.h>
#include <imgui_stdlib.h>

#include "DeleteThemePopup.h"
#include "ManageThemesScreen.h"
#include "../IconsFontAwesome4.h"

using namespace std::literals;

namespace DeleteThemePopup {
    enum class State {
        hidden,
        shown,
    };

    State state;

    bool popup_queued;
    const std::string popup_id = "DeleteThemePopup"s;

    Installer::installed_theme_data installedThemeData;
    std::filesystem::path themeJsonPath;

    void show(Installer::installed_theme_data installed_theme_data, std::filesystem::path theme_json_path) {
        popup_queued = true;
        state = State::shown;
        installedThemeData = installed_theme_data;
        themeJsonPath = theme_json_path;
    }

    void process_ui() {
        using namespace ImGui::RAII;
        if (state == State::hidden)
            return;

        if (popup_queued) {
            ImGui::OpenPopup(popup_id);
            popup_queued = false;
        }

        auto center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5f, 0.5f});

        PopupModal popup{popup_id, nullptr,
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize};

        if (!popup) {
            state = State::hidden;
            return;
        }

        const auto &style = ImGui::GetStyle();

        {
            Font title_font{nullptr, 35};
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Delete Confirmation");
        }

        ImGui::TextWrapped("Would you like to delete \"%s\"?", installedThemeData.themeName.c_str());

        // Show two buttons with same size.
        const ImVec2 available = ImGui::GetContentRegionAvail();

        const std::string delete_label = ICON_FA_TRASH " Delete";
        const std::string cancel_label = ICON_FA_TIMES " Cancel";

        const ImVec2 delete_size = ImGui::CalcTextSize(delete_label);
        const ImVec2 cancel_size = ImGui::CalcTextSize(cancel_label);

        const ImVec2 button_size =
            ImVec2{ std::fmax(delete_size.x, cancel_size.x),
                    std::fmax(delete_size.y, cancel_size.y) }
            + 2 * style.FramePadding;

        float spacing = style.ItemSpacing.x;
        float total_width = 2 * button_size.x + spacing;

        float start_x = (available.x - total_width) / 2;

        if (start_x > 0)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

        if (ImGui::Button(delete_label, button_size)) {
            Installer::DeleteTheme(installedThemeData.installedThemePath, themeJsonPath);
            ManageThemesScreen::force_refresh();
            ImGui::CloseCurrentPopup();
            state = State::hidden;
        }
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button(cancel_label, button_size)) {
            ImGui::CloseCurrentPopup();
            state = State::hidden;
        }
    }
}
