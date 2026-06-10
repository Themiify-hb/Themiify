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
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <whb/log.h>

#include <imgui.h>
#include <misc/cpp/imgui_raii.h>

#include <SDL2/SDL_image.h>

#include "ThemezerScreen.h"
#include "ThemeDetailsPopup.h"
#include "../ImageLoader.h"
#include "../ThemezerAPI.h"
#include "../IconsFontAwesome4.h"

using ThemezerAPI::PageInfo;
using ThemezerAPI::WiiuThemeSmall;
using ThemezerAPI::WiiuThemeSmallVec;
using ThemezerAPI::SortOrder;
using ThemezerAPI::ItemSort;

using namespace std::literals;

namespace ThemezerScreen {
    uint32_t page = 0;
    
    ItemSort sort = ItemSort::CREATED;
    SortOrder order = SortOrder::ASC;
    std::string query;

    std::optional<PageInfo> page_info;
    std::optional<WiiuThemeSmallVec> themes;
    
    std::string sort_to_label(ItemSort arg) {
        switch (arg) {
            case ItemSort::CREATED:
                return "Created";
            case ItemSort::DOWNLOADS:
                return "Downloads";
            case ItemSort::SAVES:
                return "Saves";
            case ItemSort::UPDATED:
                return "Updated";
            default:
                throw std::logic_error{"invalid"};
        }
    }

    std::string order_to_label(SortOrder arg) {
        switch (arg) {
            case SortOrder::ASC:
                return ICON_FA_SORT_AMOUNT_ASC;
            case SortOrder::DESC:
                return ICON_FA_SORT_AMOUNT_DESC;
            default:
                throw std::logic_error{"invalid"};
        }
    }    

    void fetch_page(unsigned new_page) {
        if (!new_page)
            return;

        page = new_page;

        ThemezerAPI::wiiu::themes({
                .paginationArgs = {
                    .limit = 20,
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
            });
    }
    
    void initialize(SDL_Renderer *renderer) {
        WHBLogPrintf("Hello from ThemezerScreen init!");

        fetch_page(1);
    }

    void finalize() {
        WHBLogPrintf("Hello from ThemezerScreen finalize!");
    }

    void show(const WiiuThemeSmall& theme) {
        using namespace ImGui::RAII;

        Child theme_frame{theme.uuid, {0, 320}, ImGuiChildFlags_NavFlattened 
                         | ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_NoSavedSettings};
        if (!theme_frame)
            return;

        auto thumbnail = ImageLoader::get(theme.collagePreview.thumbUrl);
        ImGui::Image((ImTextureID)thumbnail, {426, 240});

        ImGui::SameLine();

        {
            Group right_group;
            ImGui::TextWrapped("Name: %s", theme.name.data());
            ImGui::TextWrapped("Author: %s", theme.creator.username.data());
            ImGui::Text("Downloads: %u", theme.downloadCount);

            if (ImGui::Button(ICON_FA_INFO_CIRCLE " Details")) {
                ThemeDetailsPopup::show(theme.hexId);
            }

            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_DOWNLOAD " Download")) {
                // Download stuff here
            }
        }
    }

    void process_ui() {
        using namespace ImGui::RAII;

        Child themezer_content{"ThemezerContent", {0, 0}, ImGuiChildFlags_NavFlattened};
        if (!themezer_content)
            return;

        // Title
        {
            Font title_font{nullptr, 42};
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Themes from Themezer");
        }
        
        // Naviation controls
        {
            Disabled disabled_when{ThemezerAPI::is_busy()};

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

            if (new_page_info) {
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

        ImGui::SameLine();

        // Sort and Filter controls
        {
            Disabled disable_when{ThemezerAPI::is_busy()};

            if (Child filter_order_search_box{"FilterOrderSearchBox", {0.0f, 75.0f}, ImGuiChildFlags_NavFlattened}) {
                ImGui::SetNextItemWidth(220);
                if (Combo sort_combo{"##sort_combo"s, sort_to_label(sort)}) {
                    for (auto new_sort : ThemezerAPI::ItemSortList) {
                        if (ImGui::Selectable(sort_to_label(new_sort), new_sort == sort)) {
                            sort = new_sort;
                            fetch_page(1);
                        }
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button(order_to_label(order))) {
                    order = order == SortOrder::ASC ? SortOrder::DESC : SortOrder::ASC;
                    fetch_page(1);
                }        
                
                ImGui::SameLine();

                SDL_WiiUSetSWKBDHintText("Input the name of a theme to search for it...");
                SDL_WiiUSetSWKBDOKLabel("Search");
                SDL_WiiUSetSWKBDShowWordSuggestions(SDL_TRUE);
                SDL_WiiUSetSWKBDHighlightInitialText(SDL_TRUE);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##network_search"s, "Search..."s, query);
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    WHBLogPrintf("Searching: %s", query.c_str());
                    fetch_page(1);
                }
            }
        }

        // Themes List
        {
            Disabled disable_when(ThemezerAPI::is_busy());
            auto& new_themes = themes;
            if (!new_themes) {
                ImGui::Text("Waiting for Themezer to respond...");
            }
            else {
                if (Child theme_list{"ThemeList"}) {
                    StyleVar child_border_size_style{ImGuiStyleVar_ChildBorderSize, 4.0f};
                    for (auto& theme : *new_themes)
                        show(theme);
                }
            }
        }

        ThemeDetailsPopup::process_ui();
    }
}