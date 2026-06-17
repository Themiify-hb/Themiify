#include "QRCodePopup.h"
#include "ThemezerScreen.h"
#include "../Camera.h"

#include <iostream>
#include <cstring>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_raii.h>

#include <quirc.h>
#include <whb/log.h>

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

    static quirc* qr = nullptr;
    static std::string last_qr_payload;
    static int scan_frame_counter = 0;

    Mix_Chunk *qr_scan_sfx;

    static void initialize_qr_scanner() {
        if (qr)
            return;

        qr = quirc_new();

        if (!qr) {
            cout << "quirc_new failed" << endl;
            return;
        }

        if (quirc_resize(qr, Camera::get_width(), Camera::get_height()) < 0) {
            cout << "quirc_resize failed" << endl;
            quirc_destroy(qr);
            qr = nullptr;
        }
    }

    static void scan_qr_code() {
        if (!qr)
            return;

        const uint8_t* gray = Camera::get_grayscale_buffer();

        if (!gray)
            return;

        int w = 0;
        int h = 0;

        uint8_t* image = quirc_begin(qr, &w, &h);

        if (!image)
            return;

        for (int y = 0; y < Camera::get_height(); y++) {
            std::memcpy(
                image + y * Camera::get_width(),
                gray + y * Camera::get_pitch(),
                Camera::get_width()
            );
        }

        quirc_end(qr);

        int count = quirc_count(qr);

        for (int i = 0; i < count; i++) {
            quirc_code code {};
            quirc_data data {};

            quirc_extract(qr, i, &code);

            quirc_decode_error_t err = quirc_decode(&code, &data);

            if (err == QUIRC_SUCCESS) {
                last_qr_payload = reinterpret_cast<const char*>(data.payload);

                cout << "QR " << last_qr_payload << endl;
                Mix_PlayChannel(-1, qr_scan_sfx, 0);

                std::string hex_id;

                std::size_t start = last_qr_payload.find("/themes/");
                if (start != std::string::npos) {
                    start += 8; // length of "/themes/"

                    std::size_t end = last_qr_payload.find('/', start);

                    if (end != std::string::npos) {
                        hex_id = last_qr_payload.substr(start, end - start);
                    }
                }

                std::cout << hex_id << std::endl;

                ImGui::CloseCurrentPopup();
                ThemezerScreen::fetch_theme_by_id(hex_id);

                break;
            }
        }
    }
    
    void show(Mix_Chunk *qr_sfx) {
        popup_queued = true;
        state = State::shown;

        qr_scan_sfx = qr_sfx;
    }

    void process_ui() {
        using namespace ImGui::RAII;
        
        if (state == State::hidden) {
            return;
        }

        if (popup_queued) {
            ImGui::OpenPopup(popup_id);
            initialize_qr_scanner();
            scan_frame_counter = 0;
            popup_queued = false;
        }

        auto center = ImGui::GetMainViewport()->GetCenter();
        auto *viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowSize({viewport->Size.x * 0.85f, 0.0f}, ImGuiCond_Always);
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5f, 0.5f});

        Popup popup{
            popup_id,
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoMove
        };
        
        if (!popup) {
            state = State::hidden;
            return;
        }

        switch (state) {
            case State::shown: {
                ImGui::Text("Scan Theme QR Code");
                ImGui::Separator();
                
                ImGui::Text(
                    "Place a Themezer QR Code in front of the camera and wait for the\n"
                    "code to be scanned automatically."
                );

                ImGui::Spacing();

                Camera::update_texture();

                // Don't scan every single frame if it feels too heavy.
                scan_frame_counter++;

                if (scan_frame_counter >= 6) {
                    scan_qr_code();
                    scan_frame_counter = 0;
                }

                SDL_Texture* camera_texture = Camera::get_texture();

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

                ImGui::TextWrapped("Tip: If the QR code is too bright, lower the brightness of the device its displayed on until it is clear " \
                                    "enough to be seen on the camera preview.");

                break;
            }

            default:
                break;
        }
    }
}