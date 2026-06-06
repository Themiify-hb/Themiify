/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "NavBar.h"
#include "utils.h"

#include <SDL2/SDL_image.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <misc/cpp/imgui_raii.h>
#include <backends/imgui_impl_sdlrenderer2.h>

namespace NavBar {
    SDL_Texture *logo_tex;

    SDL_Texture *home_button_normal_tex;
    SDL_Texture *home_button_active_tex;

    SDL_Texture *installed_button_normal_tex;
    SDL_Texture *installed_button_active_tex;
    
    SDL_Texture *misc_button_normal_tex;
    SDL_Texture *misc_button_active_tex;
    
    SDL_Texture *themezer_button_normal_tex;
    SDL_Texture *themezer_button_active_tex;
    
    SDL_Texture *download_button_normal_tex;
    SDL_Texture *download_button_active_tex;
    
    Tab current_tab = Tab::home;
    
    void initialize(SDL_Renderer *renderer) {
        logo_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/logo.png");

        home_button_normal_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/home-button-normal.png");
        home_button_active_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/home-button-active.png");

        installed_button_normal_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/installed-button-normal.png");
        installed_button_active_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/installed-button-active.png");

        misc_button_normal_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/misc-button-normal.png");
        misc_button_active_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/misc-button-active.png");

        themezer_button_normal_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/themezer-button-normal.png");
        themezer_button_active_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/themezer-button-active.png");

        download_button_normal_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/download-button-normal.png");
        download_button_active_tex = IMG_LoadTexture(renderer, "fs:/vol/content/ui/download-button-active.png");
    }

    void finalize() {
        SDL_DestroyTexture(logo_tex);

        SDL_DestroyTexture(home_button_normal_tex);
        SDL_DestroyTexture(home_button_active_tex);

        SDL_DestroyTexture(installed_button_normal_tex);
        SDL_DestroyTexture(installed_button_active_tex);

        SDL_DestroyTexture(misc_button_normal_tex);
        SDL_DestroyTexture(misc_button_active_tex);

        SDL_DestroyTexture(themezer_button_normal_tex);
        SDL_DestroyTexture(themezer_button_active_tex);

        SDL_DestroyTexture(download_button_normal_tex);
        SDL_DestroyTexture(download_button_active_tex);        
    }

    void process_ui() {
        using namespace ImGui::RAII;

        Child nav_bar{"NavBar", {200.0f, 0.0f}, ImGuiChildFlags_NavFlattened};
        if (!nav_bar)
            return;
        
        StyleVar no_frame_border{ImGuiStyleVar_FrameBorderSize, 0.0f};
        StyleVar no_image_border{ImGuiStyleVar_ImageBorderSize, 0.0f};
        StyleVar no_child_border{ImGuiStyleVar_ChildBorderSize, 0.0f};
        StyleVar no_frame_rounding{ImGuiStyleVar_FrameRounding, 0.0f};
        StyleVar no_frame_padding{ImGuiStyleVar_FramePadding, ImVec2{0, 0}};
        
        ImGui::Image(logo_tex, ImVec2(178, 160));

        if (Child buttons_box{"ButtonsBox", {}, ImGuiChildFlags_NavFlattened}) {
            if (current_tab == Tab::home) {
                ImGui::ImageButton("home_button_active", home_button_active_tex, ImVec2(148, 96));
            }
            else {
                // Implement the App::ImageButton overload in the App namespace to add the sound effect and rumble when clicked
                if (ImGui::ImageButton("home_button_normal", home_button_normal_tex, ImVec2(148, 96))) {
                    current_tab = Tab::home;
                }
            }

            if (current_tab == Tab::installed) {
                ImGui::ImageButton("installed_button_active", installed_button_active_tex, ImVec2(148, 96));
            }
            else {
                if (ImGui::ImageButton("installed_button_normal", installed_button_normal_tex, ImVec2(148, 96))) {
                    current_tab = Tab::installed;
                }
            }
            
            if (current_tab == Tab::misc) {
                ImGui::ImageButton("misc_button_active", misc_button_active_tex, ImVec2(148, 96));
            }
            else {
                if (ImGui::ImageButton("misc_button_normal", misc_button_normal_tex, ImVec2(148, 96))) {
                    current_tab = Tab::misc;
                }
            }
            
            if (current_tab == Tab::themezer) {
                ImGui::ImageButton("themezer_button_active", themezer_button_active_tex, ImVec2(148, 96));
            }
            else {
                if (ImGui::ImageButton("themezer_button_normal", themezer_button_normal_tex, ImVec2(148, 96))) {
                    current_tab = Tab::themezer;
                }
            }
            
            if (current_tab == Tab::download) {
                ImGui::ImageButton("download_button_active", download_button_active_tex, ImVec2(148, 96));
            }
            else {
                if (ImGui::ImageButton("download_button_normal", download_button_normal_tex, ImVec2(148, 96))) {
                    current_tab = Tab::download;
                }
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