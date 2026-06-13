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
#include "../installer.h"

namespace InstallThemePopup {
    void show(const std::filesystem::path &uthemePath, Installer::theme_data themeData, bool confirmationCompleted, bool setCurrent);

    void process_ui();
}