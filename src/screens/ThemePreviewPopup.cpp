#include <iostream>

#include <imgui.h>
#include <misc/cpp/imgui_raii.h>

#include "ThemePreviewPopup.h"
#include "../ImageLoader.h"
#include "../IconsFontAwesome4.h"
#include "../cafe_glyphs.h"

using namespace std::literals;

namespace ThemePreviewPopup {
    enum class State {
        hidden,
        shown,
    };  

    State state;

    bool popup_queued;
    const std::string popup_id = "ThemePreviewPopup"s;

    std::string launcher_url;
    std::string waraWara_url;

    int image_idx;

    bool hide_ui;

    void show(const std::string& launcherUrl, const std::string& waraWaraUrl) {
        state = State::shown;
        popup_queued = true;
        image_idx = 0;
        hide_ui = false;

        launcher_url = launcherUrl;
        waraWara_url = waraWaraUrl;
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
        auto *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowSize({viewport->Size.x, viewport->Size.y}, ImGuiCond_Always);
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5f, 0.5f});
        ImGui::RAII::Popup popup{popup_id, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                 ImGuiWindowFlags_NoNavInputs};        
        
        if (!popup) {
            state = State::hidden;
            return;
        }     

        if (image_idx < 0)
            image_idx = 0;

        if (image_idx > 1)
            image_idx = 1;

        auto *launcher_tex = ImageLoader::get(launcher_url);
        auto *waraWara_tex = ImageLoader::get(waraWara_url);
        image_idx == 0 ? ImGui::Image((ImTextureID)launcher_tex, {viewport->Size.x, viewport->Size.y}) : ImGui::Image((ImTextureID)waraWara_tex, {viewport->Size.x, viewport->Size.y});
        
        
        // Bug with ImGui, I'm assuming cause they never bothered to implement
        // A Nintendo button layout :P
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft))
        hide_ui = !hide_ui;
        
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1))
        image_idx--;
        
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1))
        image_idx++;        
        
        if (!hide_ui) {
            ImVec2 arrow_button_size{60.0f, 60.0f};
            float middle_y = viewport->Pos.y + (viewport->Size.y - arrow_button_size.y) * 0.5f;
            float padding = 30.0f;
    
            ImVec2 button_size{160.0f, 60.0f};
            ImVec2 text_box_size{220.0f, 60.0f};
    
            float spacing = 20.0f;
    
            float total_width = button_size.x + spacing + text_box_size.x + spacing + button_size.x;
    
            float start_x = viewport->Pos.x + (viewport->Size.x - total_width) * 0.5f;
    
            float y = viewport->Pos.y + viewport->Size.y - padding - button_size.y;
            
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.0f, 0.0f, 0.0f, 0.9f));
    
            ImGui::SetCursorScreenPos({start_x, y});
    
            if (ImGui::Button(CAFE_GLYPH_BTN_B " Close", button_size))
            {
                ImGui::CloseCurrentPopup();
            }
    
            ImGui::SetCursorScreenPos({
                start_x + button_size.x + spacing,
                y
            });
    
            ImGui::Button("##info_panel", text_box_size);
    
            std::string label = std::string(ICON_FA_CHEVRON_LEFT) + " " + std::string(CAFE_GLYPH_BTN_L) + "/" + std::string(CAFE_GLYPH_BTN_R) + " " + std::string(ICON_FA_CHEVRON_RIGHT);
    
            ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
    
            ImGui::SetCursorScreenPos({
                start_x + button_size.x + spacing +
                    (text_box_size.x - text_size.x) * 0.5f,
                y + (text_box_size.y - text_size.y) * 0.5f
            });
    
            ImGui::TextUnformatted(label);
    
            ImGui::SetCursorScreenPos({
                start_x + button_size.x + spacing +
                text_box_size.x + spacing,
                y
            });
    
            if (ImGui::Button(CAFE_GLYPH_BTN_X " Hide", button_size)) {
                hide_ui = true;
            }
    
            ImGui::PopStyleColor(3);
    
            {
                ImGui::SetCursorScreenPos({
                    viewport->Pos.x + padding, middle_y
                });
                Disabled disable_left{image_idx == 0};
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.0f, 0.0f, 0.0f, 0.9f));
    
                if (ImGui::Button(ICON_FA_CHEVRON_LEFT, arrow_button_size)) {
                    image_idx--;
                }
    
                ImGui::PopStyleColor(3);
            }
    
            { 
                ImGui::SetCursorScreenPos({
                    viewport->Pos.x + viewport->Size.x - padding - arrow_button_size.x, middle_y
                });
                Disabled disable_right{image_idx == 1};
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.0f, 0.0f, 0.0f, 0.9f));
    
                if (ImGui::Button(ICON_FA_CHEVRON_RIGHT, arrow_button_size)) {
                    image_idx++;
                }
    
                ImGui::PopStyleColor(3);
            }        
        }
        else {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                hide_ui = false;
        }
    }
}