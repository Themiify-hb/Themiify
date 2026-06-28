/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "HomeScreen.h"
#include "SettingsScreen.h"
#include "SettingsPopup.h"
#include "../NavBar.h"
#include "../installer.h"
#include "../IconsFontAwesome4.h"
#include "../ImageLoader.h"
#include "../utils.h"

#include <iostream>
#include <filesystem>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <imgui.h>
#include <imgui_raii.h>

#include <mocha/mocha.h>

using std::cout;
using std::cerr;
using std::endl;

// Define this to help seeing the padding and spacing values for windows.
// #define DEBUG_BG_COLOR

namespace HomeScreen {
    SDL_Renderer *home_renderer = nullptr;

    SDL_Texture *themiify_logo = nullptr;

    std::string current_theme_str;
    std::string current_theme_id_path;
    std::string current_theme_json_path;
    std::filesystem::path current_theme_thumbnail_path;

    Installer::installed_theme_data current_theme_data;

    bool current_theme_refresh = true;

    bool isFirstBoot = true;
    bool styleMiiUExists = true;
    bool queueStyleMiiUPrompt = false;

    std::string get_theme_id(const std::string& str) {
        auto open = str.rfind('(');
        auto close = str.rfind(')');

        if (open == std::string::npos ||
            close == std::string::npos ||
            close <= open)
            return {};

        return str.substr(open + 1, close - open - 1);
    }

    void refresh_current_theme() {
        current_theme_str = Installer::GetCurrentTheme();
        current_theme_id_path = get_theme_id(current_theme_str);

        current_theme_json_path =
            THEMIIFY_INSTALLED_THEMES / (current_theme_id_path + ".json");

        current_theme_thumbnail_path =
            THEMIIFY_THUMBNAILS / (current_theme_id_path + ".webp");

        int res = Installer::GetInstalledThemeMetadata(
            current_theme_json_path,
            &current_theme_data
        );

        if (res != 1)
            current_theme_data.themeIDPath = "";

        current_theme_refresh = false;
    }

    void force_refresh() {
        current_theme_refresh = true;
    }

    bool check_stylemiiu_exists() {
        char environmentPathBuffer[0x100];

        MochaUtilsStatus res;
        if ((res = Mocha_GetEnvironmentPath(environmentPathBuffer, sizeof(environmentPathBuffer))) != MOCHA_RESULT_SUCCESS) {
            cerr << "Failed to get environment path. Are you running on Aroma? Result: "
                 << Mocha_GetStatusStr(res) << endl;
            return false;
        }

        if (exists(std::filesystem::path{environmentPathBuffer} / "plugins/stylemiiu.wps"))
            return true;

        return false;
    }

    void initialize(SDL_Renderer *renderer) {
        cout << "Hello from HomeScreen init!" << endl;

        home_renderer = renderer;

        themiify_logo = IMG_LoadTexture(renderer, "fs:/vol/content/ui/themiify-logo.png");

        current_theme_refresh = true;

        styleMiiUExists = check_stylemiiu_exists();
        if (!styleMiiUExists)
            queueStyleMiiUPrompt = true;

        cout << "styleMiiUExists: " << styleMiiUExists << endl;

        isFirstBoot = SettingsScreen::check_is_first_boot();
    }

    void finalize() {
        cout << "Hello from HomeScreen finalize!" << endl;

        if (themiify_logo) {
            SDL_DestroyTexture(themiify_logo);
            themiify_logo = nullptr;
        }
    }

    void process_ui() {
        using namespace ImGui::RAII;

        const auto &style = ImGui::GetStyle();
#ifdef DEBUG_BG_COLOR
        StyleColor blue_bg{ImGuiCol_ChildBg, {0.0, 0.5, 0.0, 1.0}};
#endif
        Child home_content{"HomeContent", {0, 0}, ImGuiChildFlags_AlwaysUseWindowPadding};
        if (!home_content)
            return;

        if (current_theme_refresh)
            refresh_current_theme();

        if (themiify_logo) {

            ImGui::Image((ImTextureID)themiify_logo, {520, 124});

            ImGui::SameLine();

            {
                Font font{nullptr, 25};
                ImGui::Text("v%s", THEMIIFY_VERSION);
            }
        }

        {
            Font font{nullptr, 55};
            // Cute lil thing cause why not?
            isFirstBoot ? ImGui::Text("Welcome!") : ImGui::Text("Welcome back!");
        }

        if (queueStyleMiiUPrompt) {
            SettingsPopup::show(SettingsPopup::OpenState::stylemiiu);
            queueStyleMiiUPrompt = false;
        }

        if (styleMiiUExists) {
            // For checking integrity at boot
            SettingsScreen::run_first_boot_check();
            SettingsScreen::run_boot_integrity_check();
        }

        SettingsPopup::process_ui();

#ifdef DEBUG_BG_COLOR
        StyleColor brown_bg{ImGuiCol_ChildBg, {0.3, 0.3, 0.0, 1.0}};
#endif
        Child scrollable_content{"scrollable_content", {0, 0},
                                 ImGuiChildFlags_AlwaysUseWindowPadding};
        if (!scrollable_content)
            return;


        ImGui::Spacing();

        {
            Font font{nullptr, 35};
            ImGui::Text("Your current theme:");
        }

        ImGui::Spacing();

        if (current_theme_data.themeIDPath.empty()) {
            ImGui::Text("No current theme found.");
            ImGui::Spacing();
        }
        else {
            ImGui::Indent(140);

            {
                Child theme_frame{
                    current_theme_data.themeIDPath,
                    {800, 300},
                    ImGuiChildFlags_NavFlattened |
                    ImGuiChildFlags_FrameStyle,
                    ImGuiWindowFlags_NoSavedSettings
                };

                auto img = ImageLoader::get(current_theme_thumbnail_path);
                ImGui::Image((ImTextureID)img, {426, 240});

                ImGui::SameLine();

                {
                    Group right_group;

                    {
                        Font font_guard{nullptr, 30};
                        ImGui::TextWrapped(current_theme_data.themeName);
                        ImGui::TextWrapped("by: %s", current_theme_data.themeAuthor.c_str());
                    }
                }
            }

            ImGui::Unindent(140);
        }


        if (ImGui::Button("Download Themes"))
            NavBar::set_current_tab(NavBar::Tab::themezer);

        ImGui::SameLine();

        {
            // align next button to the right
            auto available = ImGui::GetContentRegionAvail();
            std::string label = "Manage Installed Themes";
            ImVec2 button_size = ImGui::CalcTextSize(label) + 2 * style.FramePadding;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + available.x - button_size.x);
            if (ImGui::Button(label, button_size))
                NavBar::set_current_tab(NavBar::Tab::manage_themes);
        }

        ImGui::Spacing();
        ImGui::Separator();

        {
            Font font_guard{nullptr, 45};
            ImGui::Text("Credits:");
        }

        ImGui::Spacing();

        ImGui::Text(ICON_FA_CODE " Developers:");
        ImGui::Indent();
        ImGui::BulletText("Fangal-Airbag");
        ImGui::BulletText("AlphaCraft9658");
        ImGui::BulletText("Daniel K. O. (dkosmari)");
        ImGui::Unindent();

        ImGui::Spacing();

        ImGui::Text(ICON_FA_PAINT_BRUSH " UI Design:");
        ImGui::Indent();
        ImGui::BulletText("Perrohuevo");
        ImGui::BulletText("dewgong");
        ImGui::BulletText("Daniel K. O. (dkosmari)");
        ImGui::Unindent();

        ImGui::Spacing();

        ImGui::Text(ICON_FA_FONT " Fonts:");
        ImGui::Indent();
        ImGui::BulletText("Wii U System Font");
        ImGui::BulletText("FontAwesome");
        ImGui::Unindent();

        ImGui::Spacing();

        ImGui::Text(ICON_FA_MUSIC " Music:");
        ImGui::Indent();
        ImGui::BulletText("OMORI OST - 029 Good For Health, Bad for Imagination");
        ImGui::Indent();
        ImGui::TextLink("https://www.youtube.com/watch?v=XeK_I0XQW6U");
        ImGui::Unindent();
        ImGui::Unindent();

        ImGui::Spacing();

        ImGui::Text(ICON_FA_GITHUB " GitHub:");
        ImGui::Indent();
        ImGui::Bullet();
        ImGui::TextLink("https://github.com/Themiify-hb/Themiify");
        ImGui::Unindent();

        ImGui::Spacing();

        ImGui::Text(ICON_FA_STAR " Special thanks:");
        ImGui::Indent();
        ImGui::BulletText("Juanen100 for the StyleMiiU Aroma Plugin!");
        ImGui::BulletText("The Theme Café Discord mods, devs and founders!");
        ImGui::BulletText("Gatto for the incredible Theme Café docs!");
        ImGui::BulletText("Migush and the whole Themezer team!");
        ImGui::BulletText("All the amazing Wii U theme creators!");
        ImGui::BulletText("And many more!");
        ImGui::Unindent();

    }
}
