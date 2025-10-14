#include "gfx.h"
#include "utils.h"
#include "CreditsScreen.h"

CreditsScreen::CreditsScreen()
{
}

CreditsScreen::~CreditsScreen()
{
}

void CreditsScreen::Draw()
{
    ColourLoop();
    
    Gfx::Print(-4, 2,"Themiify Credits:");
    Gfx::Print(-3, 4, "Programming: Fangal-Airbag & AlphaCraft9658\nIcons: Perrohuevo & dewgong");
    Gfx::Print(-4, 7, "Special thanks to:");
    Gfx::Print(-3, 9, "The Theme Cafe Discord mods, devs and founders!\nThe whole Themezer team!\nAll the amazing theme creators!");
    Gfx::Print(-4, 17, "                             B - Back");
}

bool CreditsScreen::Update(VPADStatus status)
{
    if (status.trigger & VPAD_BUTTON_B) {
        return false;
    }

    return true;
}

void CreditsScreen::ColourLoop()
{
    // super ugly but I had this code from some super old project from years ago :P
    
    static uint32_t r = 255, g = 0, b = 0, a = 255; 

    if (b != 255 && g == 0) {
        b += 3;
    }

    if (b == 255 && r != 0) {
        r -= 3;
    }

    if (b == 255 && g != 255 && r == 0) {
        g += 3;
    }

    if (g == 255 && b != 0) {
        b -= 3;
    }

    if (b == 0 && r != 255) {
        r += 3;
    }

    if (r == 255 && g != 0) {
        g -= 3;
    }

    uint32_t color = (r << 24) | (g << 16) | (b << 8) | a;

    Gfx::SetBackgroundColour(color);
}