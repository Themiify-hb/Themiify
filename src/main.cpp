#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_console.h>
#include <sysapp/title.h>
#include <mocha/mocha.h>
#include <sysapp/launch.h>
#include <vpad/input.h>
#include "hips.hpp"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <filesystem>

void error() {
    SYSLaunchMenu();
}

bool create_filepath(std::string filepath) {
        return std::filesystem::create_directories(filepath);
}

int main(int argc, char **argv)
{
    WHBProcInit();
    WHBLogUdpInit();
    WHBLogConsoleInit();
    WHBLogConsoleSetColor(0x000000);
    Mocha_InitLibrary();
    VPADInit();
    VPADStatus vpadStatusBuff[1];

    // Mount storage_mlc
    // NOTE: storage_mlc is the system mlc_storage where the system menu as well as other system titles are located
    //       it could be risky when not handled properly, because wrong commands may override important system files.
    Mocha_MountFS("storage_mlc", nullptr, "/vol/storage_mlc01");

    uint64_t menuID = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);  // Retrieve the Wii U system menu titleID
    
    uint32_t menuIDParentDir = (uint32_t)(menuID >> 32);
    uint32_t menuIDChildDir = (uint32_t)menuID;

    char menuIDStr[17];
    snprintf(menuIDStr, sizeof(menuIDStr), "%016llx", menuID);

    char splitMenuID[18];
    snprintf(splitMenuID, sizeof(splitMenuID), "%08x/%08x", menuIDParentDir, menuIDChildDir);

    // Testing for now - Nathaniel
    // Minor modifications by AlphaCraft9658
    std::string modpacksRoot = "fs:/vol/external01/wiiu/sdcafiine/" + std::string(menuIDStr) + "/";  // Will later be used for getting modpack paths, which depend on the theme name
    std::string modPath = modpacksRoot + "test_theme/";
    std::string menuContentPath = "storage_mlc:/sys/title/" + std::string(splitMenuID) + "/content/";

    // Themiify resource directories
    std::string themiifyRoot = "fs:/vol/external01/wiiu/themiify/";
    std::string themiifyTmp = themiifyRoot + "tmp/";
    create_filepath(themiifyTmp);
    std::string themesPath = themiifyRoot + "themes/";

    std::string themeID = "test_theme";
    std::string themePath = themesPath + themeID + "/";

    // temp stuff till we implement more than one file patching - Nathaniel
    std::string inputPath = menuContentPath + "Common/Package/Men.pack"; // Gonna read from the NAND now like a big boy - Nathaniel
    std::string patchPath = themePath + "Men.bps";
    // NOTE: until we implement recursive file writing ur gonna have to manually create this directory - Nathaniel
    // So sd:/wiiu/sdcafiine/(title ID)/test_theme/content/Common/Package - Nathaniel
    // Recursive directory structure creation done (the basics) :D - AlphaCraft9658
    std::string outputPath = modPath + "/content/Common/Package/Men.pack"; // Hopefully this gives you an idea of how this is gonna work - Nathaniel
    create_filepath(modPath + "/content/Common/Package");

    // I'm great at menu design :D - Nathaniel
    WHBLogPrintf("Themiify");
    WHBLogPrintf("-----------------------------------------------------");
    WHBLogPrintf("Installed Wii U Menu title ID: %s", menuIDStr);

    // Retrieve the paths of the input file and the patch file
    // Lil disclaimer, fs:/vol/external01/ is the path to the root of the sd card
    // Open input file
    std::FILE* inputFile = std::fopen(inputPath.c_str(), "rb");
    if (!inputFile) {
        WHBLogPrintf("Failed to open input file: %s\n", inputPath.c_str());
        error();
    }

    // Open patch file
    std::FILE* patchFile = std::fopen(patchPath.c_str(), "rb");
    if (!patchFile) {
        WHBLogPrintf("Failed to open patch file: %s\n", patchPath.c_str());
        std::fclose(inputFile);
        error();
    }

    // Get the size of the input file
    std::fseek(inputFile, 0, SEEK_END);
    std::size_t inputSize = std::ftell(inputFile);
    std::rewind(inputFile);

    // Get the size of the patch file
    std::fseek(patchFile, 0, SEEK_END);
    std::size_t patchSize = std::ftell(patchFile);
    std::rewind(patchFile);

    // Read the files into arrays
    std::vector<uint8_t> inputData(inputSize);
    std::vector<uint8_t> patchData(patchSize);

    if (std::fread(inputData.data(), 1, inputSize, inputFile) != inputSize) {
        WHBLogPrintf("Failed to read input file.\n");
        std::fclose(inputFile);
        std::fclose(patchFile);
        error();
    }

    if (std::fread(patchData.data(), 1, patchSize, patchFile) != patchSize) {
        WHBLogPrintf("Failed to read patch file.\n");
        std::fclose(inputFile);
        std::fclose(patchFile);
        error();
    }

    std::fclose(inputFile);
    std::fclose(patchFile);

    Hips::PatchType patchType;
    std::string extension = patchPath;

    auto [bytes, result] = Hips::patch(inputData.data(), inputSize, patchData.data(), patchSize, Hips::PatchType::BPS);
    if (result == Hips::Result::Success) {
        WHBLogPrintf("Patch applied successfully");
        WHBLogPrintf("Writing file, please wait...");
        WHBLogPrintf("(Your console isn't frozen in this case!)");
        WHBLogPrintf("");
        WHBLogConsoleDraw();

        // Write the patch file and test it out haha
        std::FILE* outputFile = std::fopen(outputPath.c_str(), "wb");
        if (!outputFile) {
            WHBLogPrintf("Failed to open output file: Men-patched.pack\n");
            error();
        }

        if (std::fwrite(bytes.data(), 1, bytes.size(), outputFile) != bytes.size()) {
            WHBLogPrintf("Failed to write to output file: Men-patched.pack\n");
            std::fclose(outputFile);
            error();
        }

        std::fclose(outputFile);
        WHBLogPrintf("Patch succeeded! (Press A to close)");
    } else {
        WHBLogPrintf("Patching failed :(\n");
    }

    while (WHBProcIsRunning()) {
        VPADRead(VPAD_CHAN_0, vpadStatusBuff, 1, NULL);
        if (vpadStatusBuff[0].trigger == VPAD_BUTTON_A) SYSLaunchMenu();
        WHBLogConsoleDraw();
    }

    Mocha_UnmountFS("storage_mlc");
    WHBLogConsoleFree();
    WHBLogUdpDeinit();
    WHBProcShutdown();

    return 0;
}