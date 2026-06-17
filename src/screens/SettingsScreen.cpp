#include "SettingsScreen.h"
#include "SettingsPopup.h"
#include "../utils.h"

#include <iostream>

#include <SDL2/SDL_mixer.h>

#include <imgui.h>
#include <imgui_raii.h>

#include <glaze/glaze.hpp>

using std::cout;
using std::endl;

namespace SettingsScreen {
    int volume;
    bool isFirstBoot;
    bool checkIntegrityAtBoot;
    bool bootIntegrityCheckPending;
    
    // Will expand and add more stuff
    struct Settings {
        bool is_first_boot = true;
        bool check_integrity_at_boot = false;
        int music_volume = 75;
    };

    Settings settings;

    const std::string settings_path = std::string(THEMIIFY_ROOT) + "/settings.json";

    void save_settings() {
        std::filesystem::create_directories(THEMIIFY_ROOT);

        auto json = glz::write<glz::opts{.prettify = true}>(settings);

        if (!json) {
            std::cout << "Failed to serialize settings\n";
            return;
        }

        std::ofstream file(settings_path, std::ios::trunc);
        if (!file.is_open()) {
            std::cout << "Failed to open settings file\n";
            return;
        }

        file << *json;        
    }

    void load_settings() {
        std::ifstream file(settings_path);
        if (!file.is_open())
            return;

        std::string json{
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        };

        if (auto err = glz::read_json(settings, json)) {
            std::cout << "Failed to parse settings\n";
        }
    }

    bool check_is_first_boot() {
        if (settings.is_first_boot)
            return true;
        
        return false;
    }

    void run_first_boot_check() {
        if (!isFirstBoot)
            return;

        SettingsPopup::show(SettingsPopup::OpenState::force_integrity);

        settings.is_first_boot = false;
        save_settings();

        isFirstBoot = false;
    }

    void run_boot_integrity_check() {
        if (!bootIntegrityCheckPending)
            return;

        SettingsPopup::show(SettingsPopup::OpenState::force_integrity);

        bootIntegrityCheckPending = false;
    }    

    void initialize(SDL_Renderer *renderer) {
        cout << "Hello from SettingsScreen init!" << endl;

        load_settings();

        isFirstBoot = settings.is_first_boot;
        checkIntegrityAtBoot = settings.check_integrity_at_boot;
        bootIntegrityCheckPending = settings.check_integrity_at_boot;
        volume = settings.music_volume;

        Mix_Music *bgm = Mix_LoadMUS("fs:/vol/content/sound/bgm.mp3");
        
        int mix_volume = (volume * MIX_MAX_VOLUME) / 100;
        Mix_VolumeMusic(mix_volume);
        
        Mix_PlayMusic(bgm, -1);
    }

    void finalize() {
        cout << "Hello from SettingsScreen finalize" << endl;
    }

    void process_ui() {
        using namespace ImGui::RAII;
        
        Child settings_content{"SettingsContent", {0, 0}, ImGuiChildFlags_None};
        if (!settings_content)
            return;
        
        {
            Font font_guard{nullptr, 45};
            ImGui::Text("Settings");
        }

        ImGui::Separator();

        ImGui::Spacing();

        if (ImGui::Button("Check integrity of Wii U Menu files")) {
            SettingsPopup::show(SettingsPopup::OpenState::integrity);
        }
        
        ImGui::SameLine();

        if (ImGui::Checkbox("Check at every boot", &checkIntegrityAtBoot)) {
            settings.check_integrity_at_boot = checkIntegrityAtBoot;
            save_settings();
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

        ImGui::Separator();

        ImGui::Text("Background music volume level:");
        if (ImGui::SliderInt("##volume", &volume, 0, 100, "%d%%")) {
            settings.music_volume = volume;
            
            int mix_volume = (volume * MIX_MAX_VOLUME) / 100;
            Mix_VolumeMusic(mix_volume);

            save_settings();
        }

        SettingsPopup::process_ui();
    }
}