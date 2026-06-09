/*
 * RadiiU - an internet radio player for the Wii U.
 *
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRACER_HPP
#define TRACER_HPP

#include <string>


struct Tracer {

    unsigned level;
    const std::string name;

    Tracer(const std::string& name)
        noexcept;

    ~Tracer()
        noexcept;

}; // struct tracer


#define TRACE_MERGE(a, b) a##b

#define TRACE_FUNC Tracer TRACE_MERGE(tracer_, __COUNTER__){__PRETTY_FUNCTION__}

#define TRACE(x) Tracer TRACE_MERGE(tracer_, __COUNTER__){x}

#endif
