/*
 * RadiiU - an internet radio player for the Wii U.
 *
 * Copyright (C) 2025  Daniel K. O. <dkosmari>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BYTE_STREAM_HPP
#define BYTE_STREAM_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <string>
#include <vector>


class byte_stream {

    std::deque<std::byte> data;

public:

    void
    clear();


    bool
    empty() const noexcept;

    std::size_t
    size() const noexcept;


    std::size_t
    read(void* buf,
         std::size_t count)
        noexcept;

    template<typename T,
             std::size_t E>
    std::size_t
    read(std::span<T, E> buf)
        noexcept
    {
        return read(buf.data(), buf.size_bytes());
    }


    std::vector<std::byte>
    read();

    std::vector<std::byte>
    read(std::size_t count);

    template<typename T>
    requires(sizeof(T) == 1)
    std::vector<T>
    read_as(std::size_t count)
    {
        std::vector<T> result(count);
        auto sz = read(std::span(result));
        result.resize(sz);
        return result;
    }

    template<typename T>
    requires(sizeof(T) == 1)
    std::vector<T>
    read_as()
    {
        return read_as<T>(size());
    }


    [[nodiscard]]
    std::string
    read_str(std::size_t count);

    [[nodiscard]]
    std::string
    read_str();


    [[nodiscard]]
    std::size_t
    peek(void* buf,
         std::size_t count)
        const noexcept;

    template<typename T,
             std::size_t E>
    [[nodiscard]]
    std::size_t
    peek(std::span<T, E> buf)
        const noexcept
    {
        return peek(buf.data(), buf.size_bytes());
    }


    std::size_t
    discard(std::size_t count)
        noexcept;


    std::optional<std::uint8_t>
    try_load_u8();


    std::size_t
    write(const void* buf,
          std::size_t size);

    template<typename T,
             std::size_t E>
    std::size_t
    write(std::span<T, E> buf)
    {
        return write(buf.data(), buf.size_bytes());
    }

    /*
    std::size_t
    write(std::string_view sv);

    std::size_t
    write(const std::string& s);
    */

    std::size_t
    consume(byte_stream& other);

    std::size_t
    consume(byte_stream& other, std::size_t count);

};

#endif
