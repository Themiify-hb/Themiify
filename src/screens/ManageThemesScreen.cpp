/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <cctype>
#include <iostream>
#include <optional>
#include <ranges>

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
#include "../tracer.hpp"

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
        TRACE_FUNC;
        create_directories(THEMES_ROOT);
        create_directories(THEMIIFY_INSTALLED_THEMES);

        manage_renderer = renderer;
    }

    void finalize() {
        TRACE_FUNC;
    }

    void force_refresh() {
        local_themes_refresh = true;
    }

    void show_installed_theme(const Installer::installed_theme_data& theme_data,
                              bool is_current,
                              const ImVec2& inner_size,
                              const ImVec2& padding) {
        // NOTE: to create a complex button, we create a button with no text, then overlap
        // the contents.
        using namespace ImGui::RAII;
        const auto& style = ImGui::GetStyle();
        const ImVec2 outer_size = inner_size + 2 * padding;
        ImVec2 start_pos = ImGui::GetCursorPos() + padding;
        auto thumbnailPath = THEMIIFY_THUMBNAILS / (theme_data.themeIDPath + ".webp");

        if (ImGui::Button("##btn" + theme_data.themeID, outer_size)) {
            ThemeDetailsPopup::open_local(theme_data, thumbnailPath, is_current);
        }

        // NOTE: when hovered or activated, change the text color to the window bg color.
        std::optional<StyleColor> dark_text;
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            const auto& colors = style.Colors;
            dark_text.emplace(ImGuiCol_Text, colors[ImGuiCol_WindowBg]);
        }

        ImGui::SetCursorPos(start_pos);
        Group grp;

        {
            StyleVar no_border{ImGuiStyleVar_ImageBorderSize, 0};
            auto img = ImageLoader::get(thumbnailPath);
            ImVec2 img_size = {inner_size.x, inner_size.x * 9.0f / 16.0f};
            ImGui::Image((ImTextureID)img, img_size);
        }

        {
            Font font{nullptr, 24};
            ImGui::TextAligned(0.0f, inner_size.x, theme_data.themeName);
        }

        {
            // Show author aligned to the left, current marker to the right.
            Font font{nullptr, 18};

            std::string current_label = is_current ? ICON_FA_STAR : ICON_FA_STAR_O;
            float current_width = ImGui::CalcTextSize(current_label).x;

            float author_width = inner_size.x - current_width - style.ItemSpacing.x;

            ImGui::TextAligned(0.0f, author_width,
                               "by %s", theme_data.themeAuthor.data());
            ImGui::SameLine();
            ImGui::SetCursorPosX(start_pos.x + inner_size.x - current_width);
            ImGui::Text(current_label);
        }
    }

    void show_utheme(const std::filesystem::path& utheme_path) {
        using namespace ImGui::RAII;

        Child theme_frame{utheme_path.string(),
                          {0, 0},
                          ImGuiChildFlags_NavFlattened |
                          ImGuiChildFlags_AutoResizeY |
                          ImGuiChildFlags_FrameStyle,
                          ImGuiWindowFlags_NoSavedSettings};

        if (!theme_frame)
            return;

        const auto &style = ImGui::GetStyle();

        const std::string install_label = ICON_FA_COGS " Install";
        const std::string remove_label = ICON_FA_TRASH;

        auto install_size = ImGui::CalcTextSize(install_label) + 2 * style.FramePadding;
        auto remove_size = ImGui::CalcTextSize(remove_label) + 2 * style.FramePadding;

        const float text_wrap_pos =
            ImGui::GetCursorPosX()
            + ImGui::GetContentRegionAvail().x
            - style.ItemSpacing.x
            - install_size.x
            - style.ItemSpacing.x
            - remove_size.x;
        {
            TextWrapPos wrap_at{text_wrap_pos};
            ImGui::AlignTextToFramePadding();
            ImGui::Text(utheme_path.filename().string());
        }

        ImGui::SameLine();

        ImGui::SetCursorPosX(text_wrap_pos + style.ItemSpacing.x);

        if (ImGui::Button(install_label, install_size)) {
            Installer::theme_data theme_data;
            Installer::GetThemeMetadata(utheme_path, &theme_data);
            InstallThemePopup::show(utheme_path, theme_data, false, true);
        }

        ImGui::SameLine();

        if (ImGui::Button(remove_label, remove_size)) {
            DeletePath(utheme_path);
            force_refresh();
        }
    }

    void process_ui() {
        using namespace ImGui::RAII;

#ifdef DEBUG_BG_COLOR
        StyleColor green_bg{ImGuiCol_ChildBg, {0.0, 0.5, 0.0, 1.0}};
#endif

        Child content{"ManageThemesContent",
                      {0, 0},
                      ImGuiChildFlags_NavFlattened};

        if (!content)
            return;

        const auto &style = ImGui::GetStyle();

        float tab_width = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x) * 0.50f;

        const float tab_height = ImGui::GetFrameHeight();


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

#ifdef DEBUG_BG_COLOR
        StyleColor brown_bg{ImGuiCol_ChildBg, {0.3, 0.3, 0.0, 1.0}};
#endif
        if (Child contents{"contents",
                           {0, 0},
                           ImGuiChildFlags_NavFlattened}) {

            switch (current_tab) {
                case Tab::manage_installed: {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Search installed theme:");

                    ImGui::SameLine();

                    SDL_WiiUSetSWKBDHintText("Write some search terms...");
                    SDL_WiiUSetSWKBDOKLabel("Search");
                    SDL_WiiUSetSWKBDHighlightInitialText(SDL_TRUE);

                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::InputTextWithHint("##local_search"s, "Search..."s, search);

                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        cout << "Searching: " << search << endl;
                    }

                    if (local_themes_refresh) {
                        scan_installed_themes();
                        current_theme = Installer::GetCurrentTheme();
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

                    // To keep the search widget visible, put the search results inside
                    // another child.
                    if (Child search_results{"search_results"}) {

                        const ImVec2 grid_start_pos = ImGui::GetCursorPos();
                        const ImVec2 inner_size = {320, 260};
                        const ImVec2 padding = {12, 12};
                        const ImVec2 outer_size = inner_size + 2 * padding;
                        const ImVec2 spacing = {18, 18};

                        for (auto [counter, index] : visible_indexes | std::views::enumerate) {
                            auto& theme_data = installed_themes[index];
                            // auto& current_json_path = json_files[index];

                            bool is_current_theme =
                                sanitize_element(theme_data.themeName + " (" + theme_data.themeIDPath + ")") == current_theme;

                            ImVec2 grid_pos = { float(counter % 3), float(counter / 3) };
                            ImVec2 pos = grid_pos * (outer_size + spacing);
                            ImGui::SetCursorPos(grid_start_pos + pos);
                            show_installed_theme(theme_data,
                                                 is_current_theme,
                                                 inner_size,
                                                 padding);
                        }
                    }

                    break;
                }
                case Tab::install_local:
                    ImGui::Text("Install .utheme files from sd:/wiiu/themes here.");

                    ImGui::Separator();

                    if (local_themes_refresh) {
                        scan_local_themes();
                        local_themes_refresh = false;
                    }

                    for (const auto& utheme_path : local_themes)
                        show_utheme(utheme_path);
                    break;
            }
        }

        ThemeDetailsPopup::process_ui();
        InstallThemePopup::process_ui();
        DeleteThemePopup::process_ui();
    }

}
