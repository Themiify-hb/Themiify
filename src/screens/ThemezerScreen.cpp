/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cassert>
#include <iostream>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <imgui.h>
#include <imgui_raii.h>

#include <SDL2/SDL_image.h>

#include "ThemezerScreen.h"
#include "ThemeDetailsPopup.h"
#include "DownloadThemePopup.h"
#include "InstallThemePopup.h"
#include "QRCodePopup.h"
#include "../utils.h"
#include "../ImageLoader.h"
#include "../ThemezerAPI.h"
#include "../DownloadManager.h"
#include "../IconsFontAwesome4.h"

// Define this to help seeing the padding and spacing values for windows.
// #define DEBUG_BG_COLOR

using ThemezerAPI::PageInfo;
using ThemezerAPI::WiiuThemeSmall;
using ThemezerAPI::WiiuThemeSmallVec;
using ThemezerAPI::WiiuThemeFull;
using ThemezerAPI::SortOrder;
using ThemezerAPI::ItemSort;

using std::cout;
using std::endl;
using namespace std::literals;

namespace ThemezerScreen {
    bool first_fetch = true;
    uint32_t page = 0;

    ItemSort sort = ItemSort::CREATED;
    SortOrder order = SortOrder::DESC;

    std::string query;

    bool is_item_count_zero = false;
    bool fetching_theme_by_id = false;
    bool exact_id_mode = false;
    bool scroll_to_top = false;

    std::optional<PageInfo> page_info;
    std::optional<WiiuThemeSmallVec> themes;
    std::optional<WiiuThemeSmall> exact_theme;

    SDL_Texture* themezer_logo = nullptr;

    Mix_Chunk *qr_sfx;

    std::string sort_to_label(ItemSort arg) {
        switch (arg) {
            case ItemSort::CREATED:   return "Created";
            case ItemSort::DOWNLOADS: return "Downloads";
            case ItemSort::SAVES:     return "Saves";
            case ItemSort::UPDATED:   return "Updated";
            default: throw std::logic_error{"invalid"};
        }
    }

    std::string order_to_label(SortOrder arg) {
        switch (arg) {
            case SortOrder::ASC:  return ICON_FA_SORT_AMOUNT_ASC;
            case SortOrder::DESC: return ICON_FA_SORT_AMOUNT_DESC;
            default: throw std::logic_error{"invalid"};
        }
    }

    WiiuThemeSmall full_to_small(const WiiuThemeFull& full) {
        WiiuThemeSmall small;

        small.uuid = full.uuid;
        small.hexId = full.hexId;
        small.name = full.name;
        small.slug = full.slug;
        small.creator.username = full.creator.username;
        small.collagePreview.tinyUrl = full.collagePreview.tinyUrl;
        small.collagePreview.thumbUrl = full.collagePreview.thumbUrl;
        small.downloadCount = full.downloadCount;
        small.downloadUrl = full.downloadUrl;

        return small;
    }

    void fetch_theme_by_id(const std::string& hex_id) {
        fetching_theme_by_id = true;
        exact_id_mode = true;
        exact_theme.reset();

        ThemezerAPI::wiiu::theme(
            hex_id,
            [](const WiiuThemeFull& full_theme) {
                cout << "Got exact theme by ID!" << endl;

                exact_theme = full_to_small(full_theme);
                fetching_theme_by_id = false;
            }
        );
    }

    void fetch_page(unsigned new_page) {
        if (!new_page)
            return;

        page = new_page;

        ThemezerAPI::wiiu::themes({
                .paginationArgs = {
                    .limit = 21,
                    .page = page,
                },
                .sort = sort,
                .order = order,
                .query = query,
            },
            [](const WiiuThemeSmallVec& new_themes,
               const PageInfo& new_page_info)
            {
                page_info = new_page_info;
                themes = new_themes;

                is_item_count_zero = page_info->itemCount == 0;
                scroll_to_top = true;
            });
    }

    void initialize(SDL_Renderer* renderer) {
        cout << "Hello from ThemezerScreen init!" << endl;

        themezer_logo = IMG_LoadTexture(renderer, "fs:/vol/content/ui/themezer-logo.png");
        qr_sfx = Mix_LoadWAV("fs:/vol/content/sound/qr-scan.wav");

        first_fetch = true;
    }

    void finalize() {
        if (themezer_logo) {
            SDL_DestroyTexture(themezer_logo);
            themezer_logo = nullptr;
        }

        cout << "Hello from ThemezerScreen finalize!" << endl;
    }

    void show(const WiiuThemeSmall& theme, const ImVec2& inner_size, const ImVec2& padding)
    {
        // NOTE: to create a complex button, we create a button with no text, then overlap
        // the contents.
        using namespace ImGui::RAII;
        const auto& style = ImGui::GetStyle();
        const ImVec2 outer_size = inner_size + 2 * padding;
        ImVec2 start_pos = ImGui::GetCursorPos() + padding;

        if (ImGui::Button("##btn" + theme.hexId, outer_size)) {
            ThemeDetailsPopup::open_themezer(theme.hexId, theme);
        }

        // NOTE: when hovered or activated, change the text color to the background color.
        std::optional<StyleColor> dark_text;
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            const auto& colors = style.Colors;
            dark_text.emplace(ImGuiCol_Text, colors[ImGuiCol_WindowBg]);
        }

        ImGui::SetCursorPos(start_pos);
        Group grp;

        {
            StyleVar no_border{ImGuiStyleVar_ImageBorderSize, 0};
            auto thumbnail = ImageLoader::get(theme.collagePreview.tinyUrl);
            ImVec2 img_size = {inner_size.x, inner_size.x * 9.0f / 16.0f};
            ImGui::Image((ImTextureID)thumbnail, img_size);
        }

        {
            Font font{nullptr, 24};
            ImGui::TextAligned(0.0f, inner_size.x, "%s", theme.name.data());
        }

        {
            // Show author aligned to the left, downloads to the right, on the same line.
            Font font{nullptr, 18};
            std::string downloads_label = ICON_FA_DOWNLOAD " "
                + std::to_string(theme.downloadCount);
            float downloads_width = ImGui::CalcTextSize(downloads_label).x;
            float author_width = inner_size.x - downloads_width - style.ItemSpacing.x;
            ImGui::TextAligned(0.0f, author_width,
                               "by %s", theme.creator.username.data());
            ImGui::SameLine();
            ImGui::SetCursorPosX(start_pos.x + inner_size.x - downloads_width);
            ImGui::Text(downloads_label);
        }
    }

    void process_ui() {
        using namespace ImGui::RAII;

        {
            // NOTE: use a scope to contain all the temporary style changes, so they don't
            // leak into the popups at the bottom.
#ifdef DEBUG_BG_COLOR
            StyleColor green_bg{ImGuiCol_ChildBg, {0.0, 0.5, 0.0, 1.0}};
#endif
            auto &style = ImGui::GetStyle();
            auto orig_padding = style.WindowPadding;
            StyleVar small_padding{ImGuiStyleVar_WindowPadding, {0, style.WindowPadding.y}};
            if (Child themezer_content{"ThemezerContent",
                                       {0, 0},
                                       ImGuiChildFlags_NavFlattened |
                                       ImGuiChildFlags_AlwaysUseWindowPadding}) {
                // Title
                if (themezer_logo) {
                    ImGui::Image((ImTextureID)themezer_logo, {300, 86});
                }

                // Sort and Filter controls
                {
                    Disabled disable_when{ThemezerAPI::is_busy()};

                    if (Child filter_order_search_box{
                            "FilterOrderSearchBox",
                            {700.0f, 75.0f},
                            ImGuiChildFlags_NavFlattened
                        }) {

                        SDL_WiiUSetSWKBDKeyboardMode(SDL_WIIU_SWKBD_KEYBOARD_MODE_RESTRICTED);
                        SDL_WiiUSetSWKBDHintText("Input the name or ID (starting with 'T') of\n"
                                                 "a theme to search for it...");
                        SDL_WiiUSetSWKBDOKLabel("Search");
                        SDL_WiiUSetSWKBDHighlightInitialText(SDL_TRUE);

                        ImGui::SetNextItemWidth(300.0f);
                        if (ImGui::InputTextWithHint("##network_search"s, "Search..."s, query)) {
                            cout << "Searching: " << query << endl;

                            exact_id_mode = false;
                            exact_theme.reset();
                            fetching_theme_by_id = false;

                            fetch_page(1);
                        }

                        ImGui::SameLine();

                        if (ImGui::Button(ICON_FA_QRCODE)) {
                            QRCodePopup::show(qr_sfx);
                        }

                        ImGui::SameLine();

                        ImGui::SetNextItemWidth(220);
                        if (Combo sort_combo{"##sort_combo"s, sort_to_label(sort)}) {
                            for (auto new_sort : ThemezerAPI::ItemSortList) {
                                if (ImGui::Selectable(sort_to_label(new_sort), new_sort == sort)) {
                                    sort = new_sort;

                                    exact_id_mode = false;
                                    exact_theme.reset();
                                    fetching_theme_by_id = false;

                                    fetch_page(1);
                                }
                            }
                        }

                        ImGui::SameLine();

                        if (ImGui::Button(order_to_label(order))) {
                            order = order == SortOrder::ASC ? SortOrder::DESC : SortOrder::ASC;

                            exact_id_mode = false;
                            exact_theme.reset();
                            fetching_theme_by_id = false;

                            fetch_page(1);
                        }
                    }
                }

                ImGui::SameLine();

                // Navigation controls
                {
                    Disabled disabled_when{ThemezerAPI::is_busy() || exact_id_mode};

                    auto& new_page_info = page_info;

                    {
                        bool first_page = true;
                        if (new_page_info)
                            first_page = new_page_info->page == 1;

                        Disabled disable_when{first_page};

                        if (ImGui::Button(ICON_FA_CHEVRON_LEFT))
                            fetch_page(page - 1);
                    }

                    ImGui::SameLine();

                    if (exact_id_mode) {
                        ImGui::Text("ID Result");
                    }
                    else if (new_page_info) {
                        ImGui::Text("Page %u/%u", new_page_info->page, new_page_info->pageCount);
                    }

                    ImGui::SameLine();

                    {
                        bool last_page = true;
                        if (new_page_info)
                            last_page = new_page_info->page == new_page_info->pageCount;

                        Disabled disable_when{last_page};

                        if (ImGui::Button(ICON_FA_CHEVRON_RIGHT))
                            fetch_page(page + 1);
                    }
                }

                // Trigger exact ID lookup after normal search returns 0 results
                if (!ThemezerAPI::is_busy() &&
                    is_item_count_zero &&
                    query.starts_with('T') &&
                    !fetching_theme_by_id &&
                    !exact_id_mode) {

                    std::string hex_id = query.substr(1);

                    cout << "Searching exact Themezer ID: " << hex_id << endl;

                    fetch_theme_by_id(hex_id);
                }

                // Themes List
                {
                    Disabled disable_when{ThemezerAPI::is_busy()};

#ifdef DEBUG_BG_COLOR
                    StyleColor brown_bg{ImGuiCol_ChildBg, {0.3, 0.3, 0.0, 1.0}};
#endif
                    if (Child theme_list{"ThemeList", {0, 0}, ImGuiChildFlags_AlwaysUseWindowPadding}) {

                        if (scroll_to_top) {
                            scroll_to_top = false;
                            ImGui::SetScrollY(0);
                        }

                        const ImVec2 inner_size = {320, 250};
                        const ImVec2 padding = {12, 12};

                        if (exact_id_mode) {
                            if (fetching_theme_by_id) {
                                ImGui::Text("Searching by exact Themezer ID...");
                                if (ImGui::Button("Cancel Search")) {
                                    exact_id_mode = false;
                                    query = "";
                                    fetch_page(1);
                                }
                            }
                            else if (exact_theme) {
                                show(*exact_theme, inner_size, padding);
                                ImGui::Spacing();
                                if (ImGui::Button("Clear Search")) {
                                    exact_id_mode = false;
                                    query = "";
                                    fetch_page(1);
                                }
                            }
                            else {
                                ImGui::Text("No theme found for this Themezer ID.");
                            }
                        }
                        else {
                            auto& new_themes = themes;

                            if (!new_themes) {
                                ImGui::Text("Waiting for Themezer to respond...");
                            }
                            else {
                                ImVec2 start_pos = ImGui::GetCursorPos();
                                for (auto [idx, theme] : *new_themes | std::views::enumerate) {
                                    const ImVec2 outer_size = inner_size + 2 * padding;
                                    const ImVec2 spacing = {18, 18};
                                    ImVec2 grid_pos = { float(idx % 3), float(idx / 3) };
                                    ImVec2 pos = grid_pos * (outer_size + spacing);
                                    ImGui::SetCursorPos(start_pos + pos);
                                    show(theme, inner_size, padding);
                                }
                            }
                        }
                    }
                }
            }
        }

        ThemeDetailsPopup::process_ui();
        DownloadThemePopup::process_ui();
        InstallThemePopup::process_ui();
        QRCodePopup::process_ui();

        if (first_fetch) {
            fetch_page(1);
            first_fetch = false;
        }
    }
}
