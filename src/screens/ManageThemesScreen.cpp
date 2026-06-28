/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>
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
#include "../ImageLoader.h"

// Define this to help seeing the padding and spacing values for windows.
// #define DEBUG_BG_COLOR

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

    std::filesystem::path thumbnail_path;

    std::string search;
    std::string current_theme;

    std::string as_lower_case(std::string s) {
        for (char &c : s)
            c = std::tolower(static_cast<unsigned char>(c));
        return s;
    }

    int similarity_score(const std::string& haystack_, const std::string& needle_) {
        std::string haystack = as_lower_case(haystack_);
        std::string needle = as_lower_case(needle_);

        if (needle.empty())
            return 0;

        if (needle == haystack)
            return 10000;

        if (haystack.starts_with(needle))
            return 8000 - static_cast<int>(haystack.size());

        if (haystack.contains(needle))
            return 6000 - static_cast<int>(haystack.find(needle));

        int score = 0;
        std::size_t pos = 0;

        for (char c : needle) {
            pos = haystack.find(c, pos);
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

            if (!exists(data.installedThemePath)) {
                DeletePath(entry.path());
                continue;
            }

            json_files.push_back(entry.path());
            installed_themes.push_back(data);
        }
    }

    void initialize(SDL_Renderer *renderer) {
        cout << "Hello from InstalledScreen init!" << endl;
        create_directories(THEMES_ROOT);
        create_directories(THEMIIFY_INSTALLED_THEMES);

        manage_renderer = renderer;
    }

    void finalize() {
        cout << "Hello from InstalledScreen finalize!" << endl;
    }

    void force_refresh() {
        local_themes_refresh = true;
    }

    void process_ui() {
        using namespace ImGui::RAII;

#ifdef DEBUG_BG_COLOR
        StyleColor green_bg{ImGuiCol_ChildBg, {0.0, 0.5, 0.0, 1.0}};
#endif

        Child content{
            "ManageThemesContent",
            {0, 0},
            ImGuiChildFlags_AlwaysUseWindowPadding
        };

        if (!content)
            return;

        const auto &style = ImGui::GetStyle();

        float tab_width = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x) * 0.50f;

        constexpr float tab_height = 60.0f;
        // Installer::installed_theme_data theme_data;


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

#ifdef DEBUG_BG_COLOR
        StyleColor brown_bg{ImGuiCol_ChildBg, {0.3, 0.3, 0.0, 1.0}};
#endif
        if (Child contents{"contents", {0, 0}, ImGuiChildFlags_AlwaysUseWindowPadding}) {

            switch (current_tab) {
                case Tab::manage_installed: {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Search installed theme:");

                    ImGui::SameLine();

                    SDL_WiiUSetSWKBDKeyboardMode(SDL_WIIU_SWKBD_KEYBOARD_MODE_FULL);
                    SDL_WiiUSetSWKBDHintText("Iname of a theme to search for it...");
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
                                return sanitize_element(data.themeName + " (" + data.themeIDPath + ")") == current_theme;
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

                    // To keep the search widget visible, put the search results inside another child.
                    if (Child search_results{"search_results"}) {

                        for (std::size_t index : visible_indexes) {
                            auto& theme_data = installed_themes[index];
                            auto& current_json_path = json_files[index];

                            bool is_current_theme =
                                sanitize_element(theme_data.themeName + " (" + theme_data.themeIDPath + ")") == current_theme;

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

                            auto thumbnailPath =
                                THEMIIFY_THUMBNAILS / (theme_data.themeIDPath + ".webp");

                            auto thumbnail = ImageLoader::get(thumbnailPath);
                            ImGui::Image((ImTextureID)thumbnail, {426, 240});

                            ImGui::SameLine();

                            {
                                Group right_group;

                                ImGui::TextWrapped("%s", theme_data.themeName.c_str());
                                ImGui::TextWrapped("by: %s", theme_data.themeAuthor.c_str());

                                if (ImGui::Button(ICON_FA_INFO_CIRCLE " Details")) {
                                    ThemeDetailsPopup::open_local(theme_data,
                                                                  thumbnailPath,
                                                                  is_current_theme);
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

                        float spacing = style.ItemSpacing.x;

                        float total_width =
                            install_button_size.x +
                            spacing +
                            trash_button_size.x;

                        float start_x =
                            ImGui::GetWindowWidth()
                            - total_width
                            - style.WindowPadding.x;

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
        }

        ThemeDetailsPopup::process_ui();
        InstallThemePopup::process_ui();
        DeleteThemePopup::process_ui();
    }

}
