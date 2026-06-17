#include "QRCodePopup.h"
#include "../Camera.h"

#include <iostream>

#include <SDL2/SDL.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_raii.h>

using std::cout;
using std::endl;
using namespace std::literals;

namespace QRCodePopup {
    enum class State {
        hidden,
        shown,
    };

    bool popup_queued;
    State state;
    const std::string popup_id = "QRCodePopup"s;
    
    void show() {
        popup_queued = true;
        state = State::shown;
    }

    void process_ui() {
        using namespace ImGui::RAII;
        
        if (state == State::hidden) {
            Camera::close();
            return;
        }

        if (popup_queued) {
            ImGui::OpenPopup(popup_id);
            Camera::open();
            popup_queued = false;
        }

        auto center = ImGui::GetMainViewport()->GetCenter();
        auto *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowSize({viewport->Size.x * 0.85f, 0.0f}, ImGuiCond_Always);
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5f, 0.5f});
        Popup popup{popup_id, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoMove};
        
        if (!popup) {
            state = State::hidden;
            Camera::close();
            return;
        }

        switch (state) {
            case (State::shown): {
                ImGui::Text("Scan Theme QR Code");
                ImGui::Separator();
                
                ImGui::Text("Place a Themezer QR Code in front of the camera and wait for the\ncode to be scanned automatically");
                ImGui::Spacing();

                Camera::update_texture();
                SDL_Texture *camera_texture = Camera::get_texture();

                if (camera_texture) {
                    constexpr float camera_aspect = 640.0f / 480.0f;

                    float image_height = 400.0f;
                    float image_width = image_height * camera_aspect;

                    float x = (ImGui::GetContentRegionAvail().x - image_width) * 0.5f;
                    if (x > 0.0f)
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x);
                        
                    ImGui::Image(
                        (ImTextureID)camera_texture,
                        {image_width, image_height}
                    );

                }

                break;
            }
            default:
                break;
        }
    }
}