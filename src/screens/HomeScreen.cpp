#include "HomeScreen.h"
#include "../NavBar.h"
#include "../installer.h"
#include "../IconsFontAwesome4.h"
#include "../utils.h"

#include <iostream>
#include <unordered_map>
#include <filesystem>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <imgui.h>
#include <imgui_raii.h>

using std::cout;
using std::endl;

namespace HomeScreen {
    SDL_Renderer *home_renderer = nullptr;

    SDL_Texture *themiify_logo = nullptr;

    std::string current_theme_str;
    std::string current_theme_id_path;
    std::string current_theme_json_path;
    std::string current_theme_thumbnail_path;

    Installer::installed_theme_data current_theme_data;
    SDL_Texture *current_theme_thumbnail = nullptr;

    bool current_theme_refresh = true;

    std::unordered_map<std::string, SDL_Texture*> thumbnail_cache;
    SDL_Texture* placeholder_thumbnail = nullptr;

    SDL_Texture *getThumbnail(const std::filesystem::path& path) {
        std::string key = path.string();

        auto it = thumbnail_cache.find(key);
        if (it != thumbnail_cache.end())
            return it->second;

        SDL_Texture* tex = IMG_LoadTexture(home_renderer, key.c_str());

        if (!tex)
            return placeholder_thumbnail;

        thumbnail_cache[key] = tex;
        return tex;
    }

    std::string get_theme_id(const std::string& str) {
        auto open = str.rfind('(');
        auto close = str.rfind(')');

        if (open == std::string::npos ||
            close == std::string::npos ||
            close <= open)
            return {};

        return str.substr(open + 1, close - open - 1);
    }

    void refresh_current_theme() {
        current_theme_str = Installer::GetCurrentTheme();
        current_theme_id_path = get_theme_id(current_theme_str);

        current_theme_json_path =
            std::string(THEMIIFY_INSTALLED_THEMES) + "/" +
            current_theme_id_path + ".json";

        current_theme_thumbnail_path =
            std::string(THEMIIFY_THUMBNAILS) + "/" +
            current_theme_id_path + ".webp";

        int res = Installer::GetInstalledThemeMetadata(
            current_theme_json_path,
            &current_theme_data
        );

        if (res == 1)
            current_theme_thumbnail = getThumbnail(current_theme_thumbnail_path);
        else
            current_theme_data.themeIDPath = "";

        current_theme_refresh = false;
    }

    void force_refresh() {
        current_theme_refresh = true;
    }

    void initialize(SDL_Renderer *renderer) {
        cout << "Hello from HomeScreen init!" << endl;

        home_renderer = renderer;

        themiify_logo = IMG_LoadTexture(renderer, "fs:/vol/content/ui/themiify-logo.png");
        placeholder_thumbnail = IMG_LoadTexture(renderer, "fs:/vol/content/ui/theme-placeholder-icon.png");

        current_theme_refresh = true;
    }

    void finalize() {
        cout << "Hello from HomeScreen finalize!" << endl;

        for (auto& [path, tex] : thumbnail_cache) {
            if (tex)
                SDL_DestroyTexture(tex);
        }

        thumbnail_cache.clear();

        if (placeholder_thumbnail) {
            SDL_DestroyTexture(placeholder_thumbnail);
            placeholder_thumbnail = nullptr;
        }

        if (themiify_logo) {
            SDL_DestroyTexture(themiify_logo);
            themiify_logo = nullptr;
        }
    }

    void process_ui() {
        using namespace ImGui::RAII;

        Child home_content{"HomeContent", {0, 0}, ImGuiChildFlags_NavFlattened};
        if (!home_content)
            return;

        if (current_theme_refresh)
            refresh_current_theme();

        if (themiify_logo)
            ImGui::Image((ImTextureID)themiify_logo, {520, 124}); 

        {
            Font font_guard{nullptr, 55};
            ImGui::Text("Welcome back!");
        }

        ImGui::Spacing();

        {
            Font font_guard{nullptr, 35};
            ImGui::Text("Your current theme:");
        }

        ImGui::Spacing();

        if (current_theme_data.themeIDPath.empty()) {
            ImGui::Text("No current theme found.");
            ImGui::Spacing();
        }
        else {
            ImGui::Indent(140);

            {
                Child theme_frame{
                    current_theme_data.themeIDPath,
                    {800, 300},
                    ImGuiChildFlags_NavFlattened |
                    ImGuiChildFlags_FrameStyle,
                    ImGuiWindowFlags_NoSavedSettings
                };

                ImGui::Image((ImTextureID)current_theme_thumbnail, {426, 240});

                ImGui::SameLine();

                {
                    Group right_group;

                    {
                        Font font_guard{nullptr, 30};
                        ImGui::TextWrapped("%s", current_theme_data.themeName.c_str());
                        ImGui::TextWrapped("by: %s", current_theme_data.themeAuthor.c_str());
                    }
                }
            }
            
            ImGui::Unindent(140);
        }


        if (ImGui::Button("Download Themezer Themes"))
            NavBar::set_current_tab(NavBar::Tab::themezer);

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(50.0f, 0.0f));
        ImGui::SameLine();

        if (ImGui::Button("Manage Your Installed Themes"))
            NavBar::set_current_tab(NavBar::Tab::manage_themes);

        ImGui::Spacing();
        ImGui::Separator();

        {
            Font font_guard{nullptr, 45};
            ImGui::Text("Credits:");
        }

        ImGui::Spacing();

        ImGui::Text(ICON_FA_CODE " Developers:");
        ImGui::Indent();
        ImGui::Text("• Fangal-Airbag\n• AlphaCraft9658\n• Daniel K. O. (dkosmari)");
        ImGui::Unindent();

        ImGui::Spacing();

        ImGui::Text(ICON_FA_PAINT_BRUSH " UI Design:");
        ImGui::Indent();
        ImGui::Text("• Perrohuevo\n• dewgong\n• Daniel K. O. (dkosmari)");
        ImGui::Unindent();
        
        ImGui::Spacing();
        
        ImGui::Text(ICON_FA_FONT " Fonts:");
        ImGui::Indent();
        ImGui::Text("• Wii U System Font\n• FontAwesome");
        ImGui::Unindent();
        
        ImGui::Spacing();
        
        ImGui::Text(ICON_FA_MUSIC " Music:");
        ImGui::Indent();
        ImGui::Text("• OMORI OST - 029 Good For Health, Bad for Imagination" \
                    "\n  Link: www.youtube.com/watch?v=XeK_I0XQW6U");
        ImGui::Unindent();

        ImGui::Spacing();

        ImGui::Text(ICON_FA_GITHUB " GitHub:");
        ImGui::Indent();
        ImGui::Text("• github.com/Fangal-Airbag/Themiify");
        ImGui::Unindent();
        
        ImGui::Spacing();

        ImGui::Text(ICON_FA_STAR " Special thanks:");
        ImGui::Indent();
        ImGui::Text("• The Theme Café Discord mods, devs and founders!" \
                    "\n• Gatto for the incredible Theme Café docs!" \
                    "\n• Migush and the whole Themezer team!" \
                    "\n• All the amazing Wii U theme creators!" \
                    "\n• And many more!");
        ImGui::Unindent();

        ImGui::Spacing();
    }
}