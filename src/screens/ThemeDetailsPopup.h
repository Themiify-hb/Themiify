/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <filesystem>
#include <string>
#include <SDL2/SDL.h>
#include "../installer.h"
#include "../ThemezerAPI.h"

namespace ThemeDetailsPopup {
    void open_themezer(const std::string& request_id,
                       const ThemezerAPI::WiiuThemeSmall &small_theme);

    void open_local(Installer::installed_theme_data installed_theme_data,
                    const std::filesystem::path& thumbnail_path,
                    bool is_current);

    void process_ui();
}
