/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>
#include <filesystem>
#include <string>
#include <thread>
#include <atomic>

#include <imgui.h>
#include <imgui_raii.h>

#include "InstallThemePopup.h"
#include "ManageThemesScreen.h"
#include "../utils.h"
#include "../installer.h"
#include "../thread_safe.hpp"
#include "../IconsFontAwesome4.h"

using std::cout;
using std::cerr;
using std::endl;
using namespace std::literals;

namespace InstallThemePopup {

    namespace {

        enum class State {
            hidden,
            confirmation,
            start_install,
            installing,
            error,
            success,
        };

        std::atomic<State> state = State::hidden;

        bool popup_queued;
        const std::string popup_id = "Install Theme"s;

        std::filesystem::path utheme_path;
        Installer::theme_data theme_data;
        bool set_current = true;

        std::jthread install_thread;
        thread_safe<std::vector<std::string>> progress_messages;
        thread_safe<std::string> error_message;
        std::atomic_bool scroll_to_bottom;

        void
        progress_handler(const std::string& msg)
        {
            cout << "PROGRESS: " << msg << endl;
            auto msgs = progress_messages.lock();
            msgs->push_back(msg);
            scroll_to_bottom = true;
        }

        void
        success_handler()
        {
            state = State::success;
            scroll_to_bottom = true;
        }

        void
        error_handler(const std::exception& e)
        {
            cerr << "ERROR: " << e.what() << endl;
            error_message.store(e.what());
            state = State::error;
            scroll_to_bottom = true;
        }

        void
        show_messages()
        {
            using namespace ImGui::RAII;

            // NOTE: take up all available space except for a row of buttons at the bottom.
            ImVec2 size{0.0f, -ImGui::GetFrameHeightWithSpacing()};
            if (Child messages_box{"messages_box",
                                   size,
                                   ImGuiChildFlags_None,
                                   ImGuiWindowFlags_NoSavedSettings}) {

                {
                    Font msg_font{nullptr, 24};
                    // Show progress messages.
                    {
                        auto msgs = progress_messages.c_lock();
                        if (!msgs->empty()) {
                            for (const auto& msg : *msgs)
                                ImGui::TextWrapped(msg);
                            ImGui::Spacing();
                        }
                    }

                    // Show error message.
                    {
                        StyleColor red_text{ImGuiCol_Text, {1.0f, 0.25f, 0.25f, 1.0f}};
                        auto msg = error_message.lock();
                        if (!msg->empty())
                            ImGui::TextWrapped(*msg);
                    }

                    if (scroll_to_bottom) {
                        ImGui::SetScrollHereY(1.0f);
                        scroll_to_bottom = false;
                    }
                }
            }
        }

    } // namespace

    void show(const std::filesystem::path &uthemePath,
              Installer::theme_data themeData,
              bool confirmationCompleted,
              bool setCurrent) {
        create_directories(THEMES_ROOT);

        utheme_path = uthemePath;
        theme_data = themeData;

        if (confirmationCompleted)
            state = State::start_install;
        else
            state = State::confirmation;

        set_current = setCurrent;
        popup_queued = true;

        install_thread = {};

        progress_messages.lock()->clear();
        error_message.lock()->clear();
    }

    void process_ui() {
        using namespace ImGui::RAII;
        if (state == State::hidden)
            return;

        if (popup_queued) {
            ImGui::OpenPopup(popup_id);
            popup_queued = false;
        }

        auto center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5f, 0.5f});

        PopupModal popup{
            popup_id,
            nullptr,
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar
        };

        if (!popup) {
            state = State::hidden;
            install_thread = {};
            return;
        }

        const auto &style = ImGui::GetStyle();

        switch (state) {
            case State::confirmation: {

                ImGui::TextWrapped("Would you like to install the theme:\n%s ?",
                                   theme_data.themeName.c_str());

                ImGui::Checkbox("Set as current theme after installation", &set_current);

                ImVec2 button_size{180, 0};

                float spacing = style.ItemSpacing.x;
                float total_width = button_size.x * 2.0f + spacing;

                float start_x = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;

                if (start_x > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

                if (ImGui::Button(ICON_FA_COGS " Install", button_size)) {
                    state = State::start_install;
                }
                ImGui::SetItemDefaultFocus();

                ImGui::SameLine();

                if (ImGui::Button(ICON_FA_TIMES " Cancel", button_size)) {
                    ImGui::CloseCurrentPopup();
                    state = State::hidden;
                }

                break;
            }
            case State::start_install: {
                state = State::installing;
                install_thread = std::jthread([](std::stop_token stopper) {
                    Installer::InstallTheme(stopper,
                                            utheme_path,
                                            theme_data,
                                            progress_handler,
                                            success_handler,
                                            error_handler);
                    if (state == State::success && set_current)
                        Installer::SetCurrentTheme(theme_data.themeName, theme_data.themeIDPath);
                });

                break;
            }
            case State::installing: {
                // Dummy to have nicer window width here
                ImGui::SetCursorPosX(800.0f);
                ImGui::Dummy({0.0f, 0.0f});
                {
                    Font title_font{nullptr, 40};
                    ImGui::TextWrapped("Installing %s...", theme_data.themeName.c_str());
                }

                ImGui::Separator();

                ImGui::TextWrapped("This may take time, do not turn off your Wii U.");

                show_messages();

                ImVec2 button_size{180, 0};
                float button_x = (ImGui::GetContentRegionAvail().x - button_size.x) * 0.5f;
                if (button_x > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + button_x);
                if (ImGui::Button("Cancel", button_size))
                    install_thread = {};

                break;
            }
            case State::error: {
                {
                    Font title_font{nullptr, 50};
                    ImGui::Text("Installation failed!");
                }

                show_messages();

                ImVec2 button_size{180, 0};

                float start_x =
                    (ImGui::GetContentRegionAvail().x - button_size.x) * 0.5f;

                if (start_x > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

                if (ImGui::Button("Close", button_size)) {
                    ImGui::CloseCurrentPopup();
                    state = State::hidden;
                }

                break;
            }
            case State::success: {
                {
                    Font title_font{nullptr, 50};
                    ImGui::Text("Installation successful!");
                }

                ImGui::TextWrapped(std::format("This file is not needed anymore:\n\"{}\".",
                                               utheme_path.filename().string()));
                ImGui::TextWrapped("Would you like to delete it?");

                ImVec2 button_size{180, 0};

                float spacing = style.ItemSpacing.x;
                float total_width = button_size.x * 2.0f + spacing;

                float start_x = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;

                if (start_x > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

                if (ImGui::Button("Delete", button_size)) {
                    DeletePath(utheme_path);
                    ImGui::CloseCurrentPopup();
                    state = State::hidden;
                    ManageThemesScreen::force_refresh();
                }
                ImGui::SetItemDefaultFocus();

                ImGui::SameLine();

                if (ImGui::Button("Keep", button_size)) {
                    ImGui::CloseCurrentPopup();
                    state = State::hidden;
                    ManageThemesScreen::force_refresh();
                }

                break;
            }
            default:
                break;
        }

    }
}
