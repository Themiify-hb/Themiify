#pragma once

#include <SDL2/SDL_mixer.h>

namespace QRCodePopup {
    void show(Mix_Chunk *qr_sfx);

    void process_ui();
}