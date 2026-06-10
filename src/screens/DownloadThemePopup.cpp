#include <iostream>
#include <filesystem>
#include <string>

#include <imgui.h>
#include <misc/cpp/imgui_raii.h>

#include <whb/log.h>

#include "DownloadThemePopup.h"
#include "../DownloadManager.h"
#include "../humanize.hpp"

using namespace std::literals;

namespace DownloadThemePopup {
    enum class State {
        hidden,
        shown,
        error,
        success,
    };

    State state;

    bool popup_queued;
    const std::string popup_id = "DownloadThemePopup"s;

    void show() {
        state = State::shown;
        popup_queued = true;
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

        PopupModal popup{popup_id, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar};
        
        if (!popup) {
            state = State::hidden;
            return;
        }

        auto& infos = DownloadManager::get_infos();
        // Don't really wanna do a multiple downloads approach
        // I think this is cleaner for actually installing themes
        // Afterwards
        auto& info = infos.at(0);
        
        {
            Font title_font{nullptr, 35};
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Downloading Theme...");
        }

        ImGui::Text(info->label);

        ImGui::Text(info->url);

        ImGui::Text("Saving to: %s", info->output.filename().c_str());

        auto speed = humanize::value_bin(info->speed) + "B/s";
        ImGui::Text("DL speed: %s", speed.data());

        ImGui::ProgressBar(info->progress);

        if (info->progress >= 1.0f) {
            DownloadManager::clear_finished();
            ImGui::CloseCurrentPopup();
            state = State::hidden;
        }
    }
}