/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "HomeScreen.h"

#include <whb/log.h>

#include <imgui.h>
#include <misc/cpp/imgui_raii.h>

namespace HomeScreen {
    void initialize(SDL_Renderer *renderer) {
        WHBLogPrintf("Hello from HomeScreen init!");
    }

    void finalize() {
        WHBLogPrintf("Hello from HomeScreen finalize!");
    }

    void process_ui() {
        using namespace ImGui::RAII;

        Child home_content{"HomeContent", {0, 0}, ImGuiChildFlags_NavFlattened};
        if (!home_content)
            return;
        
        {
            Font font_guard{nullptr, 36};
            ImGui::Text("Themiify Home Screen!");
        }
        
    }
}