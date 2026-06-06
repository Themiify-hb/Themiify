/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ContentPanel.h"

#include "screens/HomeScreen.h"
#include "screens/InstalledScreen.h"
#include "screens/ThemezerScreen.h"
#include "screens/DownloadScreen.h"

namespace ContentPanel {
    void initialize(SDL_Renderer *renderer) {
        HomeScreen::initialize(renderer);
        InstalledScreen::initialize(renderer);
        ThemezerScreen::initialize(renderer);
        DownloadScreen::initialize(renderer);
    }

    void finalize() {
        HomeScreen::finalize();
        InstalledScreen::finalize();
        ThemezerScreen::finalize();
        DownloadScreen::finalize();
    }

    void process_ui(NavBar::Tab tab) {
        switch (tab) {
            case NavBar::Tab::home:
                HomeScreen::process_ui();
                break;
            
            case NavBar::Tab::installed:
                InstalledScreen::process_ui();
                break;

            case NavBar::Tab::misc:
                break;

            case NavBar::Tab::themezer:
                ThemezerScreen::process_ui();
                break;

            case NavBar::Tab::download:
                DownloadScreen::process_ui();
                break;
        }
    }
}