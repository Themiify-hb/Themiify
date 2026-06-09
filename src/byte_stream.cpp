/*
 * RadiiU - an internet radio player for the Wii U.
 *
 * Copyright (C) 2025  Daniel K. O. <dkosmari>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "byte_stream.hpp"


void
byte_stream::clear()
{
    data.clear();
}


bool
byte_stream::empty()
    const noexcept
{
    return data.empty();
}


std::size_t
byte_stream::size()
    const noexcept
{
    return data.size();
}


std::size_t
byte_stream::read(void* buf,
                  std::size_t count)
    noexcept
{
    if (count > size())
        count = size();
    auto bbuf = static_cast<std::byte*>(buf);
    for (std::size_t i = 0; i < count; ++i) {
        bbuf[i] = data.front();
        data.pop_front();
    }
    return count;
}


std::vector<std::byte>
byte_stream::read()
{
    return read(size());
}


std::vector<std::byte>
byte_stream::read(std::size_t count)
{
    return read_as<std::byte>(count);
}


std::string
byte_stream::read_str(std::size_t count)
{
    std::string result(count, '\0');
    auto sz = read(std::span(result));
    result.resize(sz);
    return result;
}


std::string
byte_stream::read_str()
{
    return read_str(size());
}


std::size_t
byte_stream::peek(void* buf,
                  std::size_t count)
    const noexcept
{
    std::size_t total = 0;
    for (auto b : data) {
        if (total >= count)
            break;
        reinterpret_cast<std::byte*>(buf)[total++] = b;
    }
    return total;
}


std::size_t
byte_stream::discard(std::size_t count)
    noexcept
{
    if (count > data.size())
        count = data.size();
    data.erase(data.begin(), data.begin() + count);
    return count;
}


std::optional<std::uint8_t>
byte_stream::try_load_u8()
{
    if (empty())
        return {};
    auto x = static_cast<std::uint8_t>(data.front());
    data.pop_front();
    return x;
}


std::size_t
byte_stream::write(const void* buf,
                   std::size_t size)
{
    auto cbuf = static_cast<const std::byte*>(buf);
    for (std::size_t i = 0; i < size; ++i)
        data.push_back(cbuf[i]);
    return size;
}


#if 0
std::size_t
byte_stream::write(std::string_view sv)
{
    for (char c : sv)
        data.push_back(std::byte{c});
    return sv.size();
}


std::size_t
byte_stream::write(const std::string& s)
{
    for (char c : s)
        data.push_back(std::byte{c});
    return s.size();
}
#endif


std::size_t
byte_stream::consume(byte_stream& other)
{
    if (this == &other)
        return 0;

    std::size_t consumed = 0;
    while (!other.empty()) {
        data.push_back(other.data.front());
        other.data.pop_front();
        ++consumed;
    }
    return consumed;
}


std::size_t
byte_stream::consume(byte_stream& other,
                     std::size_t count)
{
    if (this == &other)
        return 0;

    std::size_t consumed = 0;
    while (count-- && !other.empty()) {
        data.push_back(other.data.front());
        other.data.pop_front();
        ++consumed;
    }
    return consumed;
}
