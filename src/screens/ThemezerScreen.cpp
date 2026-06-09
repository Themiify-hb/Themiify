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

#include "ThemezerScreen.h"
#include "../ThemezerAPI.h"
#include "../IconsFontAwesome4.h"

using ThemezerAPI::PageInfo;
using ThemezerAPI::WiiuThemeSmall;
using ThemezerAPI::WiiuThemeSmallVec;
using ThemezerAPI::SortOrder;
using ThemezerAPI::ItemSort;

namespace ThemezerScreen {
    uint32_t page = 0;
    
    ItemSort sort = ItemSort::CREATED;
    SortOrder order = SortOrder::ASC;
    std::string query;

    std::optional<PageInfo> page_info;
    std::optional<WiiuThemeSmallVec> themes;
    
    std::string to_label(ItemSort arg) {
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

    std::string to_label(SortOrder arg) {
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
            //Disabled disable_when{ThemezerAPI::is_busy()};
        }
    }
}