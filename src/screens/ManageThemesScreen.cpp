/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ManageThemesScreen.h"
#include "InstallThemePopup.h"
#include "../installer.h"
#include "../utils.h"
#include "../IconsFontAwesome4.h"

#include <iostream>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <imgui.h>
#include <imgui_raii.h>

using std::cout;
using std::endl;
using namespace std::literals;

namespace ManageThemesScreen {
    enum class Tab {
        manage_installed,
        install_local,
    };

    Tab current_tab = Tab::manage_installed;

    std::vector<std::filesystem::path> local_themes;
    std::vector<std::filesystem::path> json_files;

    std::vector<Installer::installed_theme_data> installed_themes;

    bool local_themes_refresh = true;

    SDL_Renderer *manage_renderer;
    
    SDL_Texture *thumbnail;
    std::unordered_map<std::string, SDL_Texture*> thumbnail_cache;
    SDL_Texture* placeholder_thumbnail = nullptr;    

    std::string search;

    void scan_local_themes() {
        local_themes.clear();

        for (auto& entry : std::filesystem::directory_iterator(THEMES_ROOT)) {
            if (entry.is_regular_file() && entry.path().extension() == ".utheme") {
                local_themes.push_back(entry.path());
            }
        }
    }  
    
    void scan_installed_themes() {
        json_files.clear();
        installed_themes.clear();

        for (auto& entry : std::filesystem::directory_iterator(THEMIIFY_INSTALLED_THEMES)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                json_files.push_back(entry.path());
            }
        }
    }

    void initialize(SDL_Renderer *renderer) {
        cout << "Hello from InstalledScreen init!" << endl;
        std::filesystem::create_directories(THEMES_ROOT);
        std::filesystem::create_directories(THEMIIFY_INSTALLED_THEMES);

        manage_renderer = renderer;

        placeholder_thumbnail = IMG_LoadTexture(manage_renderer, "fs:/vol/content/ui/theme-placeholder-icon.png");
    }

    void finalize() {
        cout << "Hello from InstalledScreen finalize!" << endl;

        for (auto& [path, tex] : thumbnail_cache) {
            if (tex)
                SDL_DestroyTexture(tex);
        }

        thumbnail_cache.clear();

        if (placeholder_thumbnail) {
            SDL_DestroyTexture(placeholder_thumbnail);
            placeholder_thumbnail = nullptr;
        }
    }

    void force_refresh() {
        local_themes_refresh = true;
    }

    SDL_Texture *get_thumbnail(const std::filesystem::path& path) {
        std::string key = path.string();

        auto it = thumbnail_cache.find(key);
        if (it != thumbnail_cache.end())
            return it->second;

        SDL_Texture* tex = IMG_LoadTexture(manage_renderer, key.c_str());

        if (!tex)
            return placeholder_thumbnail;

        thumbnail_cache[key] = tex;
        return tex;
    }    

    void process_ui() {
        using namespace ImGui::RAII;

        Installer::installed_theme_data theme_data;

        Child content{
            "ManageThemesContent",
            {0, 0},
            ImGuiChildFlags_NavFlattened
        };

        if (!content)
            return;

        float tab_width =
            (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.50f;

        constexpr float tab_height = 60.0f;

        if (ImGui::Selectable(
                "Manage Installed Themes",
                current_tab == Tab::manage_installed,
                0,
                {tab_width, tab_height}))
        {
            current_tab = Tab::manage_installed;
            force_refresh();
        }

        ImGui::SameLine();

        if (ImGui::Selectable(
                "Install Local Themes",
                current_tab == Tab::install_local,
                0,
                {tab_width, tab_height}))
        {
            current_tab = Tab::install_local;
            force_refresh();
        }

        ImGui::Separator();
        ImGui::Spacing();

        switch (current_tab) {
            case Tab::manage_installed:
                ImGui::Text("Manage your installed themes here.");

                ImGui::SameLine();

                SDL_WiiUSetSWKBDHintText("Input the name of a theme to search for it...");
                SDL_WiiUSetSWKBDOKLabel("Search");
                SDL_WiiUSetSWKBDShowWordSuggestions(SDL_TRUE);
                SDL_WiiUSetSWKBDHighlightInitialText(SDL_TRUE);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##local_search"s, "Search..."s, search);
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    cout << "Searching: " << search << endl;
                    //fetch_page(1);
                }                

                ImGui::Spacing();

                if (local_themes_refresh) {
                    scan_installed_themes();
                    local_themes_refresh = false;
                }

                for (auto json = json_files.begin(); json != json_files.end(); ) {
                    Installer::GetInstalledThemeMetadata(*json, &theme_data);

                    if (!std::filesystem::exists(theme_data.installedThemePath)) {
                        // Modpack doesn't exist so we should delete the json because the user "uninstalled" the theme themselves
                        DeletePath(*json);
                        json = json_files.erase(json);
                        force_refresh();
                    }
                    else {
                        installed_themes.push_back(theme_data);
                        ++json;
                    }

                    Child theme_frame{theme_data.themeIDPath, {0, 320}, /*ImGuiChildFlags_NavFlattened 
                                    |*/ ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_NoSavedSettings};
                    if (!theme_frame)
                        return;

                    std::filesystem::path thumbnailPath = std::string(std::string(THEMIIFY_ROOT) + "/cache/thumbnails/" + theme_data.themeIDPath + ".webp");
                    SDL_Texture* thumbnail = placeholder_thumbnail;

                    if (std::filesystem::exists(thumbnailPath)) {
                        thumbnail = get_thumbnail(thumbnailPath);
                    }

                    ImGui::Image((ImTextureID)thumbnail, {426, 240});

                    ImGui::SameLine();

                    {
                        Group right_group;
                        ImGui::TextWrapped("Name: %s", theme_data.themeName.c_str());
                        ImGui::TextWrapped("Author: %s", theme_data.themeAuthor.c_str());

                        if (ImGui::Button(ICON_FA_INFO_CIRCLE " Details")) {
                            //ThemeDetailsPopup::show(theme.hexId, theme);
                        }

                        ImGui::SameLine();

                        if (ImGui::Button(ICON_FA_STAR " Make Default")) {
                            //DownloadThemePopup::show(theme);
                        }                    
                    }
                }

                break;

            case Tab::install_local:
                ImGui::Text("Install .utheme files from sd:/wiiu/themes here.");

                ImGui::Spacing();
            
                if (local_themes_refresh) {
                    scan_local_themes();
                    local_themes_refresh = false;
                }
            
                for (const auto& theme_path : local_themes) {
                    std::string id = theme_path.string();

                    Child theme_frame{
                        id.c_str(),
                        {0, 80},
                        ImGuiChildFlags_NavFlattened |
                        ImGuiChildFlags_FrameStyle,
                        ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse
                    };

                    if (!theme_frame)
                        continue;

                    ImGui::TextWrapped(
                        "%s",
                        theme_path.filename().string().c_str()
                    );

                    ImGui::SameLine();

                    ImVec2 button_size{150.0f, 50.0f};

                    float button_x =
                        ImGui::GetWindowWidth()
                        - button_size.x
                        - ImGui::GetStyle().WindowPadding.x;

                    ImGui::SetCursorPosX(button_x);

                    if (ImGui::Button(ICON_FA_DOWNLOAD " Install", button_size)) {
                        Installer::theme_data theme_data;
                        Installer::GetThemeMetadata(theme_path, &theme_data);
                        InstallThemePopup::show(theme_path, theme_data, false, true);
                    }
                }
                break;
        }

        InstallThemePopup::process_ui();
    }

}
