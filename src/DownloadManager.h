/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace DownloadManager {

    enum class State {
        queued,
        in_progress,
        finished,
        paused,
        canceled,
    };

    struct Info {

        std::string label;
        std::string url;
        std::filesystem::path output;
        float progress = 0;
        std::uint64_t speed = 0;
        State state;

    }; // struct Info


    using success_function_sig = void (const Info& info);
    using failure_function_sig = void (const std::exception& error);

    using success_function_t = std::move_only_function<success_function_sig>;
    using failure_function_t = std::move_only_function<failure_function_sig>;


    void
    initialize(const std::string& user_agent = "");

    void
    finalize();

    void
    process();


    void
    pause_all();

    void
    resume_all();

    void
    cancel_all();

    void
    clear_finished();


    bool
    add(const std::string& label,
        const std::string& url,
        const std::filesystem::path& output,
        success_function_t success_func,
        failure_function_t failure_func);

    void
    pause(const std::string& url);

    void
    cancel(const std::string& url);


    const std::vector<std::shared_ptr<const Info>>&
    get_infos();

}
