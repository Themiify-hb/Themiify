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

using std::cout;
using std::endl;
using namespace std::literals;

namespace InstallThemePopup {
    enum class State {
        hidden,
        confirmation,
        installing,
        error,
        success,
    };

    State state = State::hidden;

    bool popup_queued;
    const std::string popup_id = "InstallThemePopup"s;
    
    std::filesystem::path utheme_path;
    Installer::theme_data theme_data;

    bool set_current = true;

    std::jthread install_thread;
    std::atomic<bool> install_done = false;
    std::atomic<bool> install_success = false;
    bool install_started = false;

    void show(const std::filesystem::path &uthemePath, Installer::theme_data themeData, bool confirmationCompleted, bool setCurrent) {
        std::filesystem::create_directories(THEMES_ROOT);

        utheme_path = uthemePath;
        theme_data = themeData;
        
        if (confirmationCompleted)
            state = State::installing;
        else
            state = State::confirmation;
        
        set_current = setCurrent;
        popup_queued = true;

        install_started = false;
        install_done = false;
        install_success = false;        
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

        PopupModal popup{popup_id, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar};
        
        if (!popup) {
            state = State::hidden;
            return;
        }

        switch (state) {
            case State::confirmation: {
                {
                    Font title_font{nullptr, 35};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Install Confirmation");
                }

                ImGui::Text("Would you like to install the theme:\n%s ?", theme_data.themeName.c_str());

                ImGui::Checkbox("Set as current StyleMiiU theme after installation", &set_current);

                ImGui::Spacing();

                ImVec2 button_size{180.0f, 60.0f};

                float spacing = ImGui::GetStyle().ItemSpacing.x;
                float total_width = button_size.x * 2.0f + spacing;

                float start_x = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;

                if (start_x > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

                if (ImGui::Button("Yes", button_size)) {
                    state = State::installing;
                }

                ImGui::SameLine();

                if (ImGui::Button("No", button_size)) {
                    ImGui::CloseCurrentPopup();
                    state = State::hidden;
                }

                ImGui::Spacing();

                break;
            }
            case State::installing: {
                {
                    Font title_font{nullptr, 40};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Installing %s...", theme_data.themeName.c_str());
                }

                ImGui::Spacing();

                ImGui::Text("This may take time, do not turn off your Wii U.");

                if (!install_started) {
                    install_started = true;
                    install_done = false;

                    install_thread = std::jthread([] {
                        bool installSuccess = Installer::InstallTheme(utheme_path, theme_data);

                        if (installSuccess && set_current)
                            Installer::SetCurrentTheme(theme_data.themeName, theme_data.themeIDPath);

                        install_success = installSuccess;
                        install_done = true;
                    });
                }

                if (install_done) {
                    install_thread = {};

                    state = install_success ? State::success : State::error;
                }

                break;
            }
            case State::error: {
                {
                    Font title_font{nullptr, 50};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Installation unsuccessful!");
                }

                ImGui::Text("It is possible the files on your NAND have been modified.\nPlease note that Themiify requires stock files " \
                            "to be present\non your NAND for theme installation to work.");

                ImGui::Spacing();

                ImVec2 button_size{180.0f, 60.0f};

                float start_x =
                    (ImGui::GetContentRegionAvail().x - button_size.x) * 0.5f;

                if (start_x > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

                if (ImGui::Button("Close", button_size)) {
                    ImGui::CloseCurrentPopup();
                    state = State::hidden;
                }    

                ImGui::Spacing();
                
                break;    
            }
            case State::success: {
                {
                    Font title_font{nullptr, 50};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Installation successful!");
                }

                ImGui::Text("To save storage space, it is recommended to delete the\n%s file.", utheme_path.filename().c_str());
                ImGui::Text("Would you like to delete this file?");

                ImGui::Spacing();

                ImVec2 button_size{180.0f, 60.0f};

                float spacing = ImGui::GetStyle().ItemSpacing.x;
                float total_width = button_size.x * 2.0f + spacing;

                float start_x = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;

                if (start_x > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

                if (ImGui::Button("Yes", button_size)) {
                    DeletePath(utheme_path);
                    ImGui::CloseCurrentPopup();
                    state = State::hidden;
                    ManageThemesScreen::force_refresh();
                }

                ImGui::SameLine();

                if (ImGui::Button("No", button_size)) {
                    ImGui::CloseCurrentPopup();
                    state = State::hidden;
                    ManageThemesScreen::force_refresh();
                }                

                ImGui::Spacing();
                
                break;
            }
            default:
                break;
        }

    }
}
