/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ui.h"
using namespace ui;
// #define GENERIC_ENTRY (int) 0
// #define SUBMENU_ENTRY (int) 1
// #define SELECTION_ENTRY (int) 2

Menu::Menu(std::string header) : header(header) {
    this->submenus = std::make_unique<std::vector<Menu>>();
    generate_header_decoration(header);
}

void Menu::generate_header_decoration(std::string header) {
    while (DrawUtils::getTextWidth(this->headerDecoration.c_str()) < DrawUtils::getTextWidth(this->header.c_str())) {
        this->headerDecoration.append("=");
    }
}

void Menu::render_header() {
    DrawUtils::print(0, 20, this->headerDecoration.c_str(), false);
    DrawUtils::print((DrawUtils::getTextWidth(headerDecoration.c_str()) - DrawUtils::getTextWidth(header.c_str())) / 2,
        50, this->header.c_str(), false);
    DrawUtils::print(0, 80, this->headerDecoration.c_str(), false);
}

void Menu::update() {
    render_header();
}

std::string Menu::get_header_string() {
    return this->header;
}

std::string Menu::get_header_decoration() {
    return this->headerDecoration;
}

Entry::Entry(std::string label) : label(label) {}

SubmenuEntry::SubmenuEntry(std::string label, Menu& submenu) : Entry(label), type(SUBMENU_ENTRY), submenu(&submenu) {}

SelectionEntry::SelectionEntry(std::string label) : Entry(label), type(SUBMENU_ENTRY) {}