/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "NavBar.h"
#include "screens/ManageThemesScreen.h"
#include "screens/HomeScreen.h"
#include "utils.h"

#include <sysapp/launch.h>

#include <SDL2/SDL_image.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_raii.h>
#include <imgui_impl_sdlrenderer2.h>

namespace NavBar {
    SDL_Texture *logo_tex;

    SDL_Texture *home_button_normal_tex;
    SDL_Texture *home_button_active_tex;

    SDL_Texture *manage_themes_button_normal_tex;
    SDL_Texture *manage_themes_button_active_tex;
    
    SDL_Texture *settings_button_normal_tex;
    SDL_Texture *settings_button_active_tex;
    
    SDL_Texture *themezer_button_normal_tex;
    SDL_Texture *themezer_button_active_tex;
    
    SDL_Texture *exit_button_normal_tex;
    SDL_Texture *exit_button_active_tex;
    
    Tab current_tab = Tab::home;
    
    void initialize(SDL_Renderer *renderer) {
        logo_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/logo.png");

        home_button_normal_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/home-button-normal.png");
        home_button_active_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/home-button-active.png");

        manage_themes_button_normal_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/manage-themes-button-normal.png");
        manage_themes_button_active_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/manage-themes-button-active.png");

        settings_button_normal_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/settings-button-normal.png");
        settings_button_active_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/settings-button-active.png");

        themezer_button_normal_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/themezer-button-normal.png");
        themezer_button_active_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/themezer-button-active.png");

        exit_button_normal_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/exit-button-normal.png");
        exit_button_active_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/exit-button-active.png");
    }

    void finalize() {
        SDL_DestroyTexture(logo_tex);

        SDL_DestroyTexture(home_button_normal_tex);
        SDL_DestroyTexture(home_button_active_tex);

        SDL_DestroyTexture(manage_themes_button_normal_tex);
        SDL_DestroyTexture(manage_themes_button_active_tex);

        SDL_DestroyTexture(settings_button_normal_tex);
        SDL_DestroyTexture(settings_button_active_tex);

        SDL_DestroyTexture(themezer_button_normal_tex);
        SDL_DestroyTexture(themezer_button_active_tex);

        SDL_DestroyTexture(exit_button_normal_tex);
        SDL_DestroyTexture(exit_button_active_tex);        
    }

    void process_ui() {
        using namespace ImGui::RAII;

        Child nav_bar{"NavBar", {160.0f, 0.0f}, ImGuiChildFlags_NavFlattened};
        if (!nav_bar)
            return;
        
        StyleVar no_frame_border{ImGuiStyleVar_FrameBorderSize, 0.0f};
        StyleVar no_image_border{ImGuiStyleVar_ImageBorderSize, 0.0f};
        StyleVar no_child_border{ImGuiStyleVar_ChildBorderSize, 0.0f};
        StyleVar no_frame_rounding{ImGuiStyleVar_FrameRounding, 0.0f};
        StyleVar no_frame_padding{ImGuiStyleVar_FramePadding, ImVec2{0, 0}};
        
        ImGui::Image(logo_tex, ImVec2(152.4f, 138));

        if (Child buttons_box{"ButtonsBox", {}, ImGuiChildFlags_NavFlattened}) {
            if (current_tab == Tab::home) {
                ImGui::ImageButton("home_button_active", home_button_active_tex, ImVec2(148, 96));
            }
            else {
                // Implement the App::ImageButton overload in the App namespace to add the sound effect and rumble when clicked
                if (ImGui::ImageButton("home_button_normal", home_button_normal_tex, ImVec2(148, 96))) {
                    current_tab = Tab::home;
                    HomeScreen::force_refresh();
                }
            }

            if (current_tab == Tab::manage_themes) {
                ImGui::ImageButton("installed_button_active", manage_themes_button_active_tex, ImVec2(148, 96));
            }
            else {
                if (ImGui::ImageButton("installed_button_normal", manage_themes_button_normal_tex, ImVec2(148, 96))) {
                    current_tab = Tab::manage_themes;
                }
            }
            
            if (current_tab == Tab::themezer) {
                ImGui::ImageButton("themezer_button_active", themezer_button_active_tex, ImVec2(148, 96));
            }
            else {
                if (ImGui::ImageButton("themezer_button_normal", themezer_button_normal_tex, ImVec2(148, 96))) {
                    current_tab = Tab::themezer;
                    ManageThemesScreen::force_refresh();
                }
            }
            
            if (current_tab == Tab::settings) {
                ImGui::ImageButton("settings_button_active", settings_button_active_tex, ImVec2(148, 96));
            }
            else {
                if (ImGui::ImageButton("settings_button_normal", settings_button_normal_tex, ImVec2(148, 96))) {
                    current_tab = Tab::settings;
                }
            }

            ImGui::Separator();
            
            if (ImGui::ImageButton("exit_button_normal", exit_button_normal_tex, ImVec2(148, 96))) {
                SYSLaunchMenu();
            }           
        }
    }

    Tab get_current_tab() {
        return current_tab;
    }

    void set_current_tab(Tab tab) {
        current_tab = tab;
    }
}
