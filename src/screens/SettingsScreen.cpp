#include "SettingsScreen.h"
#include "SettingsPopup.h"

#include <iostream>

#include <SDL2/SDL_mixer.h>

#include <imgui.h>
#include <imgui_raii.h>

using std::cout;
using std::endl;

namespace SettingsScreen {
    int volume = 75;

    void initialize(SDL_Renderer *renderer) {
        cout << "Hello from SettingsScreen init!" << endl;
        Mix_VolumeMusic((75 * MIX_MAX_VOLUME) / 100);
    }

    void finalize() {
        cout << "Hello from SettingsScreen finalize" << endl;
    }

    void process_ui() {
        using namespace ImGui::RAII;

        Child settings_content{"HomeContent", {0, 0}, ImGuiChildFlags_NavFlattened};
        if (!settings_content)
            return;
        
        {
            Font font_guard{nullptr, 45};
            ImGui::Text("Settings");
        }

        ImGui::Separator();

        ImGui::Spacing();

        // TODO: do all of these

        if (ImGui::Button("Check integrity of Wii U Menu files")) {
            SettingsPopup::show(SettingsPopup::OpenState::integrity);
        }

        ImGui::Spacing();

        if (ImGui::Button("Dump Wii U Menu files")) {
            SettingsPopup::show(SettingsPopup::OpenState::dump);
        }

        ImGui::Spacing();

        if (ImGui::Button("Clear Themiify cache")) {
            SettingsPopup::show(SettingsPopup::OpenState::cache);
        }

        ImGui::Spacing();

        ImGui::Text("Background music volume level:");
        if (ImGui::SliderInt("##volume", &volume, 0, 100, "%d%%")) {
            int mix_volume = (volume * MIX_MAX_VOLUME) / 100;
            Mix_VolumeMusic(mix_volume);
        }

        SettingsPopup::process_ui();
    }
}