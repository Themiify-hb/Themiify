/*
 * RadiiU - an internet radio player for the Wii U.
 *
 * Copyright (C) 2025-2026  Daniel K. O. <dkosmari>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>

#include "humanize.hpp"


using std::chrono::days;
using std::chrono::duration_cast;
using std::chrono::hours;
using std::chrono::minutes;
using std::chrono::seconds;
using std::string;
using std::to_string;

using namespace std::literals;


namespace humanize {

    string
    duration(seconds t)
    {
        if (t <= 90s)
            return std::to_string(t.count()) + " s";

        if (t < 60min) {
            auto t_min = duration_cast<minutes>(t);
            auto t_sec = duration_cast<seconds>(t - t_min);
            string result = to_string(t_min.count()) + " min";
            if (t_sec >= 1s)
                result += ", " + to_string(t_sec.count()) + " s";
            return result;
        }

        if (t < 24h) {
            auto t_hrs = duration_cast<hours>(t);
            auto t_min = duration_cast<minutes>(t - t_hrs);
            string result = to_string(t_hrs.count()) + " hr";
            if (t_min >= 1min)
                result += ", " + to_string(t_min.count()) + " min";
            return result;
        }

        auto t_days = duration_cast<days>(t);
        auto t_hrs = duration_cast<hours>(t - t_days);
        auto t_min = duration_cast<minutes>(t - t_days - t_hrs);
        string result = to_string(t_days.count());
        if (t_days >= 48h)
            result += " days";
        else
            result += " day";
        if (t_hrs >= 1h)
            result += ", " + to_string(t_hrs.count()) + " hr";
        if (t_min >= 1min)
            result += ", " + to_string(t_min.count()) + " min";
        return result;
    }


    std::string
    duration_brief(std::chrono::seconds t)
    {
        unsigned total = t.count();
        unsigned s = total % 60u;
        total /= 60u;
        unsigned m = total % 60u;
        total /= 60;
        unsigned h = total % 24;
        total /= 24;
        unsigned d = total;
        char buf[64];
        if (d)
            std::snprintf(buf, sizeof buf, "%ud %02u:%02u:%02u", d, h, m, s);
        else
            std::snprintf(buf, sizeof buf, "%02u:%02u:%02u", h, m, s);
        return buf;
    }


    string
    value(std::uint64_t x)
    {
        if (x < 1'000u)
            return to_string(x);

        constexpr float kilo = 1e3f;
        constexpr float mega = 1e6f;
        constexpr float giga = 1e9f;
        constexpr float tera = 1e12f;

        char buf[32];

        if (x < 10'000u) {
            float fx = x / kilo;
            std::snprintf(buf, sizeof buf, "%.1fk", fx);
            return buf;
        }

        if (x < 1'000'000u) {
            float fx = x / kilo;
            std::snprintf(buf, sizeof buf, "%.0fk", fx);
            return buf;
        }


        if (x < 10'000'000u) {
            float fx = x / mega;
            std::snprintf(buf, sizeof buf, "%.1fM", fx);
            return buf;
        }

        if (x < 1'000'000'000u) {
            float fx = x / mega;
            std::snprintf(buf, sizeof buf, "%.0fM", fx);
            return buf;
        }


        if (x < 10'000'000'000u) {
            float fx = x / giga;
            std::snprintf(buf, sizeof buf, "%.1fG", fx);
            return buf;
        }


        if (x < 1'000'000'000'000u) {
            float fx = x / giga;
            std::snprintf(buf, sizeof buf, "%.0fG", fx);
            return buf;
        }

        float fx = x / tera;
        std::snprintf(buf, sizeof buf, "%.1fT", fx);
        return buf;
    }


    std::string
    value_bin(std::uint64_t x)
    {
        if (x < (1u << 10u))
            return to_string(x);

        constexpr float kibi = 1024.0f;
        constexpr float mebi = 1048576.0f;
        constexpr float gibi = 1073741824.0f;
        constexpr float tebi = 1099511627776.0f;

        char buf[32];

        if (x < (1u << 14u)) {
            float fx = x / kibi;
            std::snprintf(buf, sizeof buf, "%.1fKi", fx);
            return buf;
        }

        if (x < (1u << 20u)) {
            float fx = x / kibi;
            std::snprintf(buf, sizeof buf, "%.0fKi", fx);
            return buf;
        }


        if (x < (1u << 24u)) {
            float fx = x / mebi;
            std::snprintf(buf, sizeof buf, "%.1fMi", fx);
            return buf;
        }

        if (x < (1u << 30u)) {
            float fx = x / mebi;
            std::snprintf(buf, sizeof buf, "%.0fMi", fx);
            return buf;
        }


        if (x < (1ull << 34ull)) {
            float fx = x / gibi;
            std::snprintf(buf, sizeof buf, "%.1fGi", fx);
            return buf;
        }


        if (x < (1ull << 40ull)) {
            float fx = x / gibi;
            std::snprintf(buf, sizeof buf, "%.0fGi", fx);
            return buf;
        }

        float fx = x / tebi;
        std::snprintf(buf, sizeof buf, "%.1fTi", fx);
        return buf;
    }

} // namespace humanize
