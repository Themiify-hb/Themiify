#pragma once

namespace SettingsPopup {
    enum OpenState {
        stylemiiu,
        integrity,
        force_integrity,
        dump,
        cache,
    };

    void show(const OpenState openState);

    void process_ui();
}