/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "App.h"
#include "NavBar.h"
#include "ContentPanel.h"
#include "ThemezerAPI.h"

#include <vector>

#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_cafe.h>

#include <coreinit/memory.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h> 

#include <imgui.h>
#include <misc/cpp/imgui_raii.h>
#include <misc/cpp/imgui_stdlib.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <misc/freetype/imgui_freetype.h>

#include <curl/curl.h>

namespace App {
    SDL_Window *window;
    SDL_Renderer *renderer;

    std::vector<SDL_GameController *> controllers;
    
    void initialize_imgui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        io.Fonts->FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_LoadColor;
        io.Fonts->FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_Bitmap;

        io.ConfigDragScroll = true;
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.MouseDragThreshold = 25;

        io.ConfigInputTrickleEventQueue = false;

        auto &style = ImGui::GetStyle();
        style.ScaleAllSizes(3);
        style.WindowBorderSize = 0.0f;
        style.WindowPadding = {6.0f, 6.0f};

        style.FrameRounding = 12.0f;
        style.ChildRounding = 12.0f;
        style.GrabRounding = 12.0f;
        style.PopupRounding = 12.0f;

        auto& colors = style.Colors;
        colors[ImGuiCol_WindowBg] = {0.0, 0.0f, 0.0f, 1.0f};

        style.FontSizeBase = 30;

        ImFontConfig fontConfig;
        fontConfig.Flags |= ImFontFlags_NoLoadError;
        fontConfig.EllipsisChar = U'…';
        fontConfig.GlyphOffset.y = -30 * (4.0f / 32.0f);

        // Get Wii U System fonts.
        // Down the line move this to its own font loading function because we'll
        // be loading more fonts than just this one.
        void *fontData = nullptr;
        uint32_t fontSize = 0;
        OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, &fontData, &fontSize);

        io.Fonts->AddFontFromMemoryTTF(fontData, fontSize, 30, &fontConfig);
        fontConfig.MergeMode = true;
        io.Fonts->AddFontFromFileTTF("fs:/vol/content/fonts/fontawesome-webfont.ttf", 30, &fontConfig);

        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);
    }

    void initialize() {
        WHBLogUdpInit();
        WHBLogCafeInit();

        curl_global_init(CURL_GLOBAL_DEFAULT);

        ThemezerAPI::initialize("Themiify/1.0 (Wii U)");
    
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER);
        IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_WEBP);
        Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG);

        Mix_OpenAudioDevice(48000, MIX_DEFAULT_FORMAT, 2, 1024, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);

        window = SDL_CreateWindow("Themiify", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        WHBLogPrint("Hello world from Themiify!");
        
        initialize_imgui();
        
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            SDL_GameController *controller = nullptr;
            if (SDL_IsGameController(i)) {
                controller = SDL_GameControllerOpen(i);
                if (controller) {
                    controllers.push_back(controller);
                }
            }
        }

        NavBar::initialize(renderer);
        ContentPanel::initialize(renderer);
    }

    void finalize() {
        NavBar::finalize();
        ContentPanel::finalize();
        
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        SDL_Quit();

        ThemezerAPI::finalize();

        curl_global_cleanup();

        WHBLogUdpDeinit();
        WHBLogCafeDeinit();
    }

    bool run() {
        bool isRunning = true;

        while (isRunning) {
            try {
                ThemezerAPI::process();
            }
            catch (std::exception& e) {
                WHBLogPrintf("ERROR in ThemezerAPI::process(): %s", e.what());
            }
            
            SDL_Event e;
            while(SDL_PollEvent(&e)) {
                ImGui_ImplSDL2_ProcessEvent(&e);
                switch (e.type) {
                    case SDL_QUIT: { 
                        WHBLogPrint("Quitting Themiify!");
                        isRunning = false;
                        break;
                    }
                    case SDL_CONTROLLERDEVICEADDED: {
                        SDL_GameController *controller = SDL_GameControllerOpen(e.cdevice.which);
                        if (controller) {
                            controllers.push_back(controller);
                        }
                        break;
                    }
                    case SDL_CONTROLLERDEVICEREMOVED: {
                        SDL_GameController *controller = SDL_GameControllerFromInstanceID(e.cdevice.which); 
                        for (auto it = controllers.begin(); it != controllers.end(); ++it) {
                            if (*it == controller) {
                                SDL_GameControllerClose(*it);
                                controllers.erase(it);
                                break;
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();

            ImGui::NewFrame();

            ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
            ImGui::SetNextWindowSize({1280, 720}, ImGuiCond_Always);

            if (ImGui::RAII::Window main_window{"Themiify",
                                            nullptr,
                                            ImGuiWindowFlags_NoTitleBar |
                                            ImGuiWindowFlags_NoMove |
                                            ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_NoResize}) {                            
                NavBar::process_ui();
                ImGui::SameLine();
                ContentPanel::process_ui(NavBar::get_current_tab());
            }

            // ImGui::ShowDemoWindow();
            // ImGui::ShowStyleEditor();

            ImGui::EndFrame();

            ImGui::Render();

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

            // Wii U clip fix
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderDrawPoint(renderer, 0, 0);

            SDL_RenderPresent(renderer);
        }

        return isRunning;
    }
}