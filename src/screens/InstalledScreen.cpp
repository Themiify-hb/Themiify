/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "InstalledScreen.h"

#include <whb/log.h>

#include <imgui.h>
#include <misc/cpp/imgui_raii.h>

namespace InstalledScreen {
    void initialize(SDL_Renderer *renderer) {
        WHBLogPrintf("Hello from InstalledScreen init!");
    }

    void finalize() {
        WHBLogPrintf("Hello from InstalledScreen finalize!");
    }

    void process_ui() {
        using namespace ImGui::RAII;

        Child home_content{"InstalledContent", {0, 0}, ImGuiChildFlags_NavFlattened};
        if (!home_content)
            return;
        
        {
            Font font_guard{nullptr, 36};
            ImGui::Text("Themiify Installed Screen!");
        }
        
    }
}