#include "SettingsPopup.h"
#include "../utils.h"

#include <sysapp/title.h>

#include <string>
#include <iostream>
#include <fstream>

#include <imgui.h>
#include <imgui_raii.h>

#include <zlib.h>

/*
946CD8A2 Common/Package/Men2.pack
B9A4343A Common/Package/Men.pack
C9C16521 Common/Sound/Men/cafe_barista_men.bfsar

9C91A249 UsEnglish/Message/AllMessage.szs
F80483EE UsFrench/Message/AllMessage.szs
82F3CB76 UsPortuguese/Message/AllMessage.szs
AFA41B10 UsSpanish/Message/AllMessage.szs

A3662453 EuDutch/Message/AllMessage.szs
15DB5A6D EuEnglish/Message/AllMessage.szs
2690B327 EuFrench/Message/AllMessage.szs
F6FD1ADA EuGerman/Message/AllMessage.szs
DAF77D8C EuItalian/Message/AllMessage.szs
E0DC3860 EuPortuguese/Message/AllMessage.szs
0BDED99E EuRussian/Message/AllMessage.szs
89C4AA89 EuSpanish/Message/AllMessage.szs

EEB51547 JpJapanese/Message/AllMessage.szs
*/

using std::cout;
using std::endl;
using namespace std::literals;

namespace SettingsPopup {
    enum State {
        hidden,
        integrity_confirmation,
        checking_integrity,
        integrity_checked,
        dump_confirmation,
        dumping,
        dump_completed,
        cache_confirmation,
        clearing_cache,
        cache_cleared,
    };

    State state;

    bool popup_queued;
    const std::string popup_id = "SettingsPopup"s;

    bool dump_allmessage = false;
    bool delete_thumbnails = false;

    std::string menu_content_path;

    std::array<std::string, 13> all_message_szs_locations = {
        "UsEnglish/Message/AllMessage.szs",
        "UsFrench/Message/AllMessage.szs",
        "UsPortuguese/Message/AllMessage.szs",
        "UsSpanish/Message/AllMessage.szs",
        "EuDutch/Message/AllMessage.szs",
        "EuEnglish/Message/AllMessage.szs",
        "EuFrench/Message/AllMessage.szs",
        "EuGerman/Message/AllMessage.szs",
        "EuItalian/Message/AllMessage.szs",
        "EuPortuguese/Message/AllMessage.szs",
        "EuRussian/Message/AllMessage.szs",
        "EuSpanish/Message/AllMessage.szs",
        "JpJapanese/Message/AllMessage.szs"
    };  
    
    struct FileIntegrityInfo {
        std::string relative_path;
        uint32_t expected_crc;
    };

    const std::array<FileIntegrityInfo, 16> integrity_files = {{
        {"Common/Package/Men2.pack",               0x946CD8A2},
        {"Common/Package/Men.pack",                0xB9A4343A},
        {"Common/Sound/Men/cafe_barista_men.bfsar",0xC9C16521},

        {"UsEnglish/Message/AllMessage.szs",       0x9C91A249},
        {"UsFrench/Message/AllMessage.szs",        0xF80483EE},
        {"UsPortuguese/Message/AllMessage.szs",    0x82F3CB76},
        {"UsSpanish/Message/AllMessage.szs",       0xAFA41B10},

        {"EuDutch/Message/AllMessage.szs",         0xA3662453},
        {"EuEnglish/Message/AllMessage.szs",       0x15DB5A6D},
        {"EuFrench/Message/AllMessage.szs",        0x2690B327},
        {"EuGerman/Message/AllMessage.szs",        0xF6FD1ADA},
        {"EuItalian/Message/AllMessage.szs",       0xDAF77D8C},
        {"EuPortuguese/Message/AllMessage.szs",    0xE0DC3860},
        {"EuRussian/Message/AllMessage.szs",       0x0BDED99E},
        {"EuSpanish/Message/AllMessage.szs",       0x89C4AA89},

        {"JpJapanese/Message/AllMessage.szs",      0xEEB51547}
    }};    

    std::vector<std::string> full_all_message_paths;

    std::vector<std::string> modified_files;

    std::string GetMenuContentPath() {
        uint64_t menuTitleID = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);

        uint32_t menuIDParentDir = (uint32_t)(menuTitleID >> 32);
        uint32_t menuIDChildDir = (uint32_t)menuTitleID;

        char splitMenuID[18];
        snprintf(splitMenuID, sizeof(splitMenuID), "%08x/%08x", menuIDParentDir, menuIDChildDir);

        return "storage_mlc:/sys/title/" + std::string(splitMenuID) + "/content/";
    }    

    void CreateCacheFile(std::ifstream &sourceFile, std::string outputPath) {
        if (!sourceFile.is_open()) {
            cout << "Invalid or unopened source file" << endl;
            return;
        }

        std::size_t sourceSize = static_cast<std::size_t>(sourceFile.tellg());
        sourceFile.seekg(0, std::ios::beg);

        std::vector<unsigned char> buffer(sourceSize);
        sourceFile.read(reinterpret_cast<char*>(buffer.data()), sourceSize);

        if (!sourceFile) {
            cout << "Error reading source file to create cache file." << endl;
            return;
        }

        std::ofstream outputFile(outputPath, std::ios::binary | std::ios::trunc);
        if (!outputFile.is_open()) {
            cout << "Failed to open output file for writing: " << outputPath << endl;
            return;
        }

        outputFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

        if (!outputFile) {
            cout << "Error writing cache file to " << outputPath << endl;
            return;
        }

        cout << "Successfully cached file to: " << outputPath << endl;
    }  
    
    uint32_t CalculateCRC32(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Failed to open file");

        uint32_t crc = crc32(0L, Z_NULL, 0);

        std::vector<char> buffer(64 * 1024);

        while (file) {
            file.read(buffer.data(), buffer.size());

            std::streamsize bytes_read = file.gcount();
            if (bytes_read > 0) {
                crc = crc32(
                    crc,
                    reinterpret_cast<const Bytef*>(buffer.data()),
                    static_cast<uInt>(bytes_read)
                );
            }
        }

        return crc;
    }    

    void show(const OpenState openState) {
        menu_content_path = GetMenuContentPath();

        full_all_message_paths.clear();
        modified_files.clear();

        for (const auto& path : all_message_szs_locations) {
            full_all_message_paths.push_back(menu_content_path + path);
        }

        popup_queued = true;

        switch (openState) {
            case OpenState::integrity:
                state = State::integrity_confirmation;
                break;
            case OpenState::dump:
                state = State::dump_confirmation;
                break;
            case OpenState::cache:
                state = State::cache_confirmation;
                break;
            default:
                state = State::hidden;
                break;
        }

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
            case State::integrity_confirmation: {
                {
                    Font title_font{nullptr, 35};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Check Menu Integrity Confirmation");
                }                

                ImGui::Text("Would you like to check the integrity of your Wii U Menu's\nfiles on your NAND to verify " \
                            "whether they have been modified?\n\nIf your files have been modified, Themiify will " \
                            "always fail to install themes\nuntil you either restore clean files to your NAND, or place clean files" \
                            "\nin sd:/themiify/cache.\n\nPlease check the Theme Café Docs for more info.");
                
                ImGui::Spacing();

                ImVec2 button_size{250.0f, 60.0f};

                float spacing = ImGui::GetStyle().ItemSpacing.x;
                float total_width = button_size.x * 2.0f + spacing;

                float start_x = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;

                if (start_x > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

                if (ImGui::Button("Check Integrity", button_size)) {
                    state = State::checking_integrity;
                }

                ImGui::SameLine();

                if (ImGui::Button("Close", button_size)) {
                    ImGui::CloseCurrentPopup();
                    state = State::hidden;
                }

                ImGui::Spacing();

                break;
            }
            case State::checking_integrity: {
                for (const auto& entry : integrity_files) {
                    std::filesystem::path full_path =
                        menu_content_path + entry.relative_path;

                    if (!std::filesystem::exists(full_path)) {
                        std::cout << "Missing: " << full_path << '\n';
                        continue;
                    }

                    uint32_t crc = CalculateCRC32(full_path);

                    if (crc != entry.expected_crc) {
                        std::cout << "Modified: "
                                << full_path
                                << " Expected: 0x" << std::hex << entry.expected_crc
                                << " Got: 0x" << crc
                                << std::dec << '\n';
                        
                        modified_files.push_back(entry.relative_path);
                    }
                }

                {
                    Font title_font{nullptr, 35};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Checking...");
                }                  
                
                state = State::integrity_checked;
            }
            case State::integrity_checked: {
                {
                    Font title_font{nullptr, 35};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Integrity Checked");
                }                
                
                if (modified_files.empty()) {
                    ImGui::Text("All your menu files are clean and ready for use with Themiify!");
                }
                else {
                    ImGui::Text("The following files appear to be modified: ");
                    
                    ImGui::Spacing();
                    ImGui::Indent();
                    for (auto &file : modified_files) {
                        ImGui::Text("%s", file.c_str());
                    }
                    ImGui::Unindent();
                    ImGui::Spacing();

                    ImGui::Text("Please consult the Theme Cafe docs for steps to restore your original files.");
                }

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
            case State::dump_confirmation: {
                {
                    Font title_font{nullptr, 35};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Dump Menu Files Confirmation");
                }

                ImGui::Text("Would you like to dump the most common Wii U Menu files\nused in theme creation to your SD Card?" \
                            "\n\nThe files dumped are: Men.pack, Men2.pack & cafe_barista_men.bfsar\n\nNote: By installing any " \
                            "theme via Themiify, this will be done automatically.");

                ImGui::Spacing();

                ImGui::Checkbox("Dump AllMessage.szs for all languages.\nConsult the Theme Café docs for more info on these files.", &dump_allmessage);

                ImGui::Spacing();

                ImVec2 button_size{210.0f, 60.0f};

                float spacing = ImGui::GetStyle().ItemSpacing.x;
                float total_width = button_size.x * 2.0f + spacing;

                float start_x = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;

                if (start_x > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

                if (ImGui::Button("Dump Files", button_size)) {
                    state = State::dumping;
                }

                ImGui::SameLine();

                if (ImGui::Button("Close", button_size)) {
                    ImGui::CloseCurrentPopup();
                    state = State::hidden;
                }

                ImGui::Spacing();

                break;
            }
            case State::dumping: {
                std::ifstream men_file(menu_content_path + MEN_PATH);
                CreateParentDirectories(std::string(THEMIIFY_ROOT) + "/cache/" + std::string(MEN_PATH));
                CreateCacheFile(men_file, std::string(THEMIIFY_ROOT) + "/cache/" + std::string(MEN_PATH));
                men_file.close();
                
                std::ifstream men2_file(menu_content_path + MEN2_PATH);
                CreateParentDirectories(std::string(THEMIIFY_ROOT) + "/cache/" + std::string(MEN2_PATH));
                CreateCacheFile(men2_file, std::string(THEMIIFY_ROOT) + "/cache/" + std::string(MEN2_PATH));
                men2_file.close();
                
                std::ifstream barista_file(menu_content_path + CAFE_BARISTA_MEN_PATH);
                CreateParentDirectories(std::string(THEMIIFY_ROOT) + "/cache/" + std::string(CAFE_BARISTA_MEN_PATH));
                CreateCacheFile(barista_file, std::string(THEMIIFY_ROOT) + "/cache/" + std::string(CAFE_BARISTA_MEN_PATH));
                barista_file.close();   

                if (dump_allmessage) {
                    for (size_t i = 0; i < full_all_message_paths.size(); ++i) {
                        std::ifstream source_file(full_all_message_paths.at(i));
                        CreateParentDirectories(full_all_message_paths.at(i));
                        std::string output_path = std::string(THEMIIFY_ROOT) + "/cache/" + all_message_szs_locations.at(i);
                        CreateCacheFile(source_file, output_path);
                        source_file.close();
                    }
                }

                {
                    Font title_font{nullptr, 35};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Dumping...");
                }   
                
                state = State::dump_completed;
            }
            case State::dump_completed: {
                {
                    Font title_font{nullptr, 35};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Dump Completed");
                }                

                ImGui::Text("The dump has been sucessfully completed\n\nYou can find your dumped files at:\n" \
                            "sd:/themiify/cache");

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
            case State::cache_confirmation: {
                {
                    Font title_font{nullptr, 35};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Clear Cache Confirmation");
                }
                
                ImGui::Text("Would you like to delete your Themiify cache located at:\nsd:/themiify/cache ?" \
                            "\n\nDoing so will delete all dumped Wii U Menu files");

                ImGui::Checkbox("Delete theme thumbnails as well?", &delete_thumbnails);
                
                ImVec2 button_size{210.0f, 60.0f};

                float spacing = ImGui::GetStyle().ItemSpacing.x;
                float total_width = button_size.x * 2.0f + spacing;

                float start_x = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;

                if (start_x > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + start_x);

                if (ImGui::Button("Clear Cache", button_size)) {
                    state = State::clearing_cache;
                }

                ImGui::SameLine();

                if (ImGui::Button("Close", button_size)) {
                    ImGui::CloseCurrentPopup();
                    state = State::hidden;
                }    

                ImGui::Spacing();
                
                break;
            }
            case clearing_cache: {
                if (delete_thumbnails)
                    DeletePath(THEMIIFY_THUMBNAILS);
                
                DeletePath(std::string(THEMIIFY_ROOT) + "/cache/Common");
                for (const auto& path : all_message_szs_locations) {
                    std::string full_path = std::string(THEMIIFY_ROOT) + "/cache/" + path;
                    DeletePath(full_path);
                }

                {
                    Font title_font{nullptr, 35};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Clearing Cache...");
                }

                state = State::cache_cleared;

                break;
            }
            case cache_cleared: {
                {
                    Font title_font{nullptr, 35};
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Cache Cleared");
                }                

                ImGui::Text("The Themiify cache has been succesfully cleared.");

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
            default:
                break;
        }
    }
}