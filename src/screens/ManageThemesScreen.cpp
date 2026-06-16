/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <iostream>
#include <unordered_map>
#include <cctype>
#include <algorithm>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <imgui.h>
#include <imgui_raii.h>

#include "ManageThemesScreen.h"
#include "InstallThemePopup.h"
#include "ThemeDetailsPopup.h"
#include "DeleteThemePopup.h"
#include "../installer.h"
#include "../utils.h"
#include "../IconsFontAwesome4.h"

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
    bool is_current_theme = false;

    SDL_Renderer *manage_renderer;
    
    SDL_Texture *thumbnail;
    std::unordered_map<std::string, SDL_Texture*> thumbnail_cache;
    SDL_Texture* placeholder_thumbnail = nullptr;    

    std::string search;
    std::string current_theme;

    SDL_Texture *getThumbnail(const std::filesystem::path& path) {
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

    std::string normalize_search(std::string s) {
        std::ranges::transform(s, s.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        return s;
    }

    int similarity_score(const std::string& name, const std::string& search) {
        std::string n = normalize_search(name);
        std::string q = normalize_search(search);

        if (q.empty())
            return 0;

        if (n == q)
            return 10000;

        if (n.starts_with(q))
            return 8000 - static_cast<int>(n.size());

        if (n.contains(q))
            return 6000 - static_cast<int>(n.find(q));

        int score = 0;
        std::size_t pos = 0;

        for (char c : q) {
            pos = n.find(c, pos);
            if (pos == std::string::npos)
                return -1;

            score += 10;
            ++pos;
        }

        return score;
    }

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
            if (!entry.is_regular_file() || entry.path().extension() != ".json")
                continue;

            Installer::installed_theme_data data;
            Installer::GetInstalledThemeMetadata(entry.path(), &data);

            if (!std::filesystem::exists(data.installedThemePath)) {
                DeletePath(entry.path());
                continue;
            }

            json_files.push_back(entry.path());
            installed_themes.push_back(data);
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

    void process_ui() {
        using namespace ImGui::RAII;

        Installer::installed_theme_data theme_data;

        Child content{
            "ManageThemesContent",
            {0, 0},
            ImGuiChildFlags_None
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
            case Tab::manage_installed: {
                ImGui::Text("Manage your installed themes here.");

                ImGui::SameLine();

                SDL_WiiUSetSWKBDKeyboardMode(SDL_WIIU_SWKBD_KEYBOARD_MODE_FULL);
                SDL_WiiUSetSWKBDHintText("Input the name of a theme to search for it...");
                SDL_WiiUSetSWKBDOKLabel("Search");
                SDL_WiiUSetSWKBDShowWordSuggestions(SDL_TRUE);
                SDL_WiiUSetSWKBDHighlightInitialText(SDL_TRUE);

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##local_search"s, "Search..."s, search);

                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    cout << "Searching: " << search << endl;
                }

                ImGui::Spacing();

                if (local_themes_refresh) {
                    scan_installed_themes();
                    current_theme = Installer::GetCurrentTheme();

                    auto current_it = std::find_if(
                        installed_themes.begin(),
                        installed_themes.end(),
                        [&](const auto& data) {
                            return data.themeName + " (" + data.themeIDPath + ")" == current_theme;
                        }
                    );

                    if (current_it != installed_themes.end()) {
                        auto index = std::distance(installed_themes.begin(), current_it);

                        std::rotate(installed_themes.begin(), current_it, current_it + 1);
                        std::rotate(json_files.begin(), json_files.begin() + index, json_files.begin() + index + 1);
                    }

                    local_themes_refresh = false;
                }

                std::vector<std::size_t> visible_indexes;

                for (std::size_t i = 0; i < installed_themes.size(); ++i) {
                    int score = similarity_score(installed_themes[i].themeName, search);

                    if (search.empty() || score >= 0)
                        visible_indexes.push_back(i);
                }

                if (!search.empty()) {
                    std::ranges::sort(
                        visible_indexes,
                        [&](std::size_t a, std::size_t b) {
                            return similarity_score(installed_themes[a].themeName, search)
                                > similarity_score(installed_themes[b].themeName, search);
                        }
                    );
                }

                for (std::size_t index : visible_indexes) {
                    auto& theme_data = installed_themes[index];
                    auto& current_json_path = json_files[index];

                    bool is_current_theme =
                        theme_data.themeName + " (" + theme_data.themeIDPath + ")" == current_theme;

                    Child theme_frame{
                        theme_data.themeIDPath,
                        {0, 320},
                        ImGuiChildFlags_NavFlattened |
                        ImGuiChildFlags_FrameStyle,
                        ImGuiWindowFlags_NoSavedSettings
                    };

                    if (!theme_frame)
                        continue;

                    if (is_current_theme) {
                        auto* draw_list = ImGui::GetWindowDrawList();

                        ImVec2 min = ImGui::GetWindowPos();
                        ImVec2 max = {
                            min.x + ImGui::GetWindowSize().x,
                            min.y + ImGui::GetWindowSize().y
                        };

                        constexpr float rounding = 16.0f;

                        draw_list->AddRect(
                            min,
                            max,
                            IM_COL32(50, 220, 50, 255),
                            rounding,
                            ImDrawFlags_RoundCornersAll,
                            6.0f
                        );

                        constexpr float radius = 18.0f;

                        ImVec2 badge_center{
                            max.x - radius - 12.0f,
                            min.y + radius + 12.0f
                        };

                        draw_list->AddCircleFilled(
                            badge_center,
                            radius,
                            IM_COL32(50, 220, 50, 255)
                        );

                        draw_list->AddCircle(
                            badge_center,
                            radius,
                            IM_COL32(255, 255, 255, 255),
                            0,
                            2.0f
                        );

                        const char* star = ICON_FA_STAR;
                        ImVec2 star_size = ImGui::CalcTextSize(star);

                        draw_list->AddText(
                            {
                                badge_center.x - star_size.x * 0.5f,
                                badge_center.y - star_size.y * 0.5f
                            },
                            IM_COL32(255, 255, 255, 255),
                            star
                        );
                    }

                    std::filesystem::path thumbnailPath =
                        std::string(THEMIIFY_THUMBNAILS) + "/" +
                        theme_data.themeIDPath +
                        ".webp";

                    SDL_Texture* thumbnail = getThumbnail(thumbnailPath);

                    ImGui::Image((ImTextureID)thumbnail, {426, 240});

                    ImGui::SameLine();

                    {
                        Group right_group;

                        ImGui::TextWrapped("%s", theme_data.themeName.c_str());
                        ImGui::TextWrapped("by: %s", theme_data.themeAuthor.c_str());

                        if (ImGui::Button(ICON_FA_INFO_CIRCLE " Details")) {
                            ThemeDetailsPopup::show_local(theme_data, thumbnail, is_current_theme);
                        }

                        ImGui::SameLine();

                        {
                            Disabled disable_when{is_current_theme};

                            if (ImGui::Button(ICON_FA_STAR " Make Default")) {
                                Installer::SetCurrentTheme(theme_data.themeName, theme_data.themeIDPath);
                                force_refresh();
                            }
                        }

                        ImGui::Spacing();

                        if (ImGui::Button(ICON_FA_TRASH " Delete")) {
                            DeleteThemePopup::show(theme_data, current_json_path);
                        }
                    }
                }

                break;
            }
            case Tab::install_local:
                ImGui::Text("Install .utheme files from sd:/wiiu/themes here.");

                ImGui::Spacing();
            
                if (local_themes_refresh) {
                    scan_local_themes();
                    local_themes_refresh = false;
                }
            
                for (const auto& utheme_path : local_themes) {
                    std::string id = utheme_path.string();

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
                        utheme_path.filename().string().c_str()
                    );

                    ImGui::SameLine();

                    ImVec2 install_button_size{150.0f, 50.0f};
                    ImVec2 trash_button_size{50.0f, 50.0f};

                    float spacing = ImGui::GetStyle().ItemSpacing.x;

                    float total_width =
                        install_button_size.x +
                        spacing +
                        trash_button_size.x;

                    float start_x =
                        ImGui::GetWindowWidth()
                        - total_width
                        - ImGui::GetStyle().WindowPadding.x;

                    ImGui::SetCursorPosX(start_x);

                    if (ImGui::Button(ICON_FA_DOWNLOAD " Install", install_button_size)) {
                        Installer::theme_data theme_data;
                        Installer::GetThemeMetadata(utheme_path, &theme_data);
                        InstallThemePopup::show(utheme_path, theme_data, false, true);
                    }

                    ImGui::SameLine();

                    if (ImGui::Button(ICON_FA_TRASH, trash_button_size)) {
                        DeletePath(utheme_path);
                        force_refresh();
                    }
                }
                break;
        }

        ThemeDetailsPopup::process_ui();
        InstallThemePopup::process_ui();
        DeleteThemePopup::process_ui();
    }

}
