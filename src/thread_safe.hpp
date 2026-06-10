/*
 * RadiiU - an internet radio player for the Wii U.
 *
 * Copyright (C) 2025  Daniel K. O. <dkosmari>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef THREAD_SAFE_HPP
#define THREAD_SAFE_HPP

#include <mutex>
#include <utility>
#include <type_traits>


template<typename T>
class thread_safe {

    mutable std::mutex mutex;
    T data;

public:

    template<typename U>
    class guard {

        std::scoped_lock<std::mutex> guard_;
        U* data_ = nullptr;

        guard(std::mutex& m,
              U* d) :
            guard_{m},
            data_{d}
        {}

        friend class thread_safe<std::remove_cv_t<U>>;

    public:

        guard(guard&& other) :
            guard_{std::move(other.guard_)},
            data_{other.data_}
        {
            other.data_ = nullptr;
        }

        guard&
        operator =(guard&& other)
            noexcept
        {
            if (this != other) {
                guard_ = std::move(other.guard_);
                data_ = other.data_;
                other.data_ = nullptr;
            }
            return *this;
        }

        U&
        operator *()
            noexcept
        {
            return *data_;
        }

        U*
        operator ->()
            const noexcept
        {
            return data_;
        }

    }; // class guard


    thread_safe() = default;


    template<typename U>
    thread_safe(U&& value) :
        data(std::forward<U>(value))
    {}


    guard<T>
    lock()
        &
    {
        return guard<T>{mutex, &data};
    }


    guard<const T>
    lock()
        const &
    {
        return guard<const T>{mutex, &data};
    }


    guard<const T>
    c_lock()
        const &
    {
        return guard<const T>{mutex, &data};
    }


    T
    load()
        const &
    {
        return *lock();
    }


    template<typename U>
    void
    store(U&& new_data)
        &
    {
        *lock() = std::forward<U>(new_data);
    }

};

#endif
