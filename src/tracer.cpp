/*
 * RadiiU - an internet radio player for the Wii U.
 *
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <atomic>
#include <iostream>

#include "tracer.hpp"


using std::cout;
using std::endl;


namespace {

    const unsigned indent_width = 4;

    std::atomic_uint global_level{0};

} // namespace


Tracer::Tracer(const std::string& name)
    noexcept :
    level{global_level++},
    name{name}
{
    std::string indent(indent_width * level, ' ');
    cout << indent << ">>> " << name << endl;
}


Tracer::~Tracer()
    noexcept
{
    std::string indent(indent_width * level, ' ');
    cout << indent << "<<< " << name << endl;
    --global_level;
}
