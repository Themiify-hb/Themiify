/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cmath>
#include <iostream>
#include <filesystem>
#include <string>

#include <imgui.h>
#include <imgui_raii.h>

#include "DownloadThemePopup.h"
#include "InstallThemePopup.h"
#include "../utils.h"
#include "../DownloadManager.h"
#include "../humanize.hpp"
#include "../IconsFontAwesome4.h"
#include "../installer.h"

using std::cout;
using std::endl;
using namespace std::literals;

namespace DownloadThemePopup {
    enum class State {
        hidden,
        confirmation,
        downloading,
        error,
        success,
    };

    State state;

    bool popup_queued;
    const std::string popup_id = "Download Theme";
    std::filesystem::path utheme_path;

    ThemezerAPI::WiiuThemeSmall theme;

    bool set_current = true;

    void open(const ThemezerAPI::WiiuThemeSmall &theme_data) {
        state = State::confirmation;
        popup_queued = true;
        theme = theme_data;
        utheme_path = "";
    }

    void show_confirmation() {
        using namespace ImGui::RAII;

        const auto &style = ImGui::GetStyle();
        {
            Font title_font{nullptr, 35};
            ImGui::Text("Download Confirmation");
        }

        ImGui::TextWrapped("Would you like to download the theme:\n%s ?", theme.name.c_str());

        // Create two buttons with equal widths.
        const ImVec2 available = ImGui::GetContentRegionAvail();

        const std::string download_label = ICON_FA_DOWNLOAD " Download";
        const std::string cancel_label = ICON_FA_TIMES " Cancel";

        const ImVec2 download_size = ImGui::CalcTextSize(download_label);
        const ImVec2 cancel_size = ImGui::CalcTextSize(cancel_label);

        const ImVec2 button_size =
            ImVec2{ std::fmax(download_size.x, cancel_size.x),
                    std::fmax(download_size.y, cancel_size.y) }
            + 2 * style.FramePadding;

        // Place the buttons on the bottom.
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + available.y - button_size.y);

        const float total_width = 2 * button_size.x + style.ItemSpacing.x;

        const float start_x = (available.x - total_width) / 2;

        if (start_x > 0)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

        if (ImGui::Button(download_label, button_size)) {
            if (DownloadManager::add("Theme: " + theme.name,
                                     theme.downloadUrl,
                                     theme.collagePreview.thumbUrl,
                                     THEMES_ROOT / (theme.slug + ".utheme"),
                                     THEMIIFY_THUMBNAILS / ("Themezer" + theme.hexId + ".webp"),
                                     {},
                                     {})) {
            }

            state = State::downloading;
        }
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button(cancel_label, button_size)) {
            ImGui::CloseCurrentPopup();
            state = State::hidden;
        }
    }

    void show_downloading() {
        using namespace ImGui::RAII;

        auto& infos = DownloadManager::get_infos();
        auto& info = infos.at(0);

        utheme_path = info->utheme_output;

        {
            Font title_font{nullptr, 35};
            ImGui::Text("Downloading Theme...");
        }

        ImGui::TextWrapped(info->label);

        ImGui::TextWrapped(info->utheme_url);

        ImGui::TextWrapped("Saving to: %s", info->utheme_output.filename().c_str());

        //auto speed = humanize::value_bin(info->speed) + "B/s";
        //ImGui::Text("DL speed: %s", speed.data());

        // Place the progress bar on the bottom.
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()
                             + ImGui::GetContentRegionAvail().y
                             - ImGui::GetFrameHeight());

        ImGui::ProgressBar(info->progress);

        if (info->progress >= 1.0f) {
            DownloadManager::clear_finished();
            state = State::success;
        }

        // TODO: downloads should be cancelable, should have a cancel button here.
    }

    void show_success() {
        using namespace ImGui::RAII;

        {
            Font title_font{nullptr, 50};
            ImGui::Text("Download successful!");
        }

        ImGui::TextWrapped("Would you like to install this theme for the StyleMiiU plugin?");

        ImGui::Checkbox("Apply theme after install", &set_current);

        // Make two buttons of equal size.
        const std::string install_label = ICON_FA_COGS " Install";
        const std::string cancel_label = ICON_FA_TIMES " Cancel";
        const ImVec2 install_size = ImGui::CalcTextSize(install_label);
        const ImVec2 cancel_size = ImGui::CalcTextSize(cancel_label);

        const auto &style = ImGui::GetStyle();
        const ImVec2 button_size =
            ImVec2{ std::fmax(install_size.x, cancel_size.x),
                    std::fmax(install_size.y, cancel_size.y) }
            + 2 * style.FramePadding;

        const ImVec2 available = ImGui::GetContentRegionAvail();
        // Place the buttons on the bottom.
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + available.y - button_size.y);

        const float total_width = 2 * button_size.x + style.ItemSpacing.x;

        const float start_x = (available.x - total_width) / 2;
        if (start_x > 0.0f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

        if (ImGui::Button(install_label, button_size)) {
            Installer::theme_data theme_data;
            Installer::GetThemeMetadata(utheme_path, &theme_data);

            ImGui::CloseCurrentPopup();
            state = State::hidden;

            InstallThemePopup::show(utheme_path, theme_data, true, set_current);
        }
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button(cancel_label, button_size)) {
            ImGui::CloseCurrentPopup();
            state = State::hidden;
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
        // WORKAROUND: setting an initial size helps with the initial position not jumping around.
        ImGui::SetNextWindowSize(viewport->Size * 0.7f, ImGuiCond_Appearing);
        auto center = viewport->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5f, 0.5f});
        PopupModal popup{popup_id, nullptr,
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize};

        if (!popup) {
            state = State::hidden;
            return;
        }

        switch (state) {
            case State::confirmation:
                show_confirmation();
                break;

            case State::downloading:
                show_downloading();
                break;

            case State::error:
                break;

            case State::success:
                show_success();
                break;

            default:
                ;
        }

    }
}
