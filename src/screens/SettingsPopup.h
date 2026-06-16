#pragma once

namespace SettingsPopup {
    enum OpenState {
        integrity,
        dump,
        cache,
    };

    void show(const OpenState openState);

    void process_ui();
}