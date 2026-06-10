/*
 * RadiiU - an internet radio player for the Wii U.
 *
 * Copyright (C) 2025-2026  Daniel K. O. <dkosmari>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HUMANIZE_HPP
#define HUMANIZE_HPP

#include <chrono>
#include <string>

namespace humanize {

    [[nodiscard]]
    std::string
    duration(std::chrono::seconds t);

    [[nodiscard]]
    std::string
    duration_brief(std::chrono::seconds t);

    [[nodiscard]]
    std::string
    value(std::uint64_t x);

    [[nodiscard]]
    std::string
    value_bin(std::uint64_t x);

} // namespace humanize

#endif
