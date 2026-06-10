/*
 * RadiiU - an internet radio player for the Wii U.
 *
 * Copyright (C) 2025  Daniel K. O. <dkosmari>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ASYNC_QUEUE_HPP
#define ASYNC_QUEUE_HPP

#include <condition_variable>
#include <expected>
#include <mutex>
#include <queue>
#include <utility>              // forward(), move()


enum class async_queue_error {
    stop,
    empty,
    locked,
};


template<typename T,
         typename Q = std::queue<T>>
class async_queue {

    std::timed_mutex mutex;
    std::condition_variable empty_cond;
    Q queue;
    bool should_stop = false;

public:

    // Makes the pool usable again after a stop().
    void
    reset()
    {
        std::lock_guard guard{mutex};
        should_stop = false;
    }


    // This will make all future pop() calls throw async_queue_exception::stop.
    // It also wakes up all threads waiting on empty_cond.
    void
    stop()
    {
        std::lock_guard guard{mutex};
        should_stop = true;
        empty_cond.notify_all(); // make sure all threads can see should_stop changed.
    }


    bool
    is_stopping()
        const
    {
        std::lock_guard guard{mutex};
        return should_stop;
    }


    bool
    empty()
        const
    {
        std::lock_guard guard{mutex};
        return queue.empty();
    }


    template<typename U>
    void
    push(U&& x)
    {
        std::lock_guard guard{mutex};
        queue.push(std::forward<U>(x));
        empty_cond.notify_one();
    }


    template<typename U>
    bool
    try_push(U&& x)
    {
        std::unique_lock guard{mutex, std::try_to_lock};
        if (!guard)
            return false;

        queue.push(std::forward<U>(x));
        return true;
    }


    T
    pop()
    {
        std::unique_lock guard{mutex};
        // Wait until a stop is requested, or the queue has data.
        empty_cond.wait(guard, [this] { return should_stop || !queue.empty(); });

        if (should_stop)
            throw async_queue_error::stop;

        T result = std::move(queue.front());
        queue.pop();
        return result;
    }


    std::expected<T, async_queue_error>
    try_pop()
        noexcept
    {
        std::unique_lock guard{mutex, std::try_to_lock};
        if (!guard)
            return std::unexpected{async_queue_error::locked};

        if (should_stop)
            return std::unexpected{async_queue_error::stop};

        if (queue.empty())
            return std::unexpected{async_queue_error::empty};

        T result = std::move(queue.front());
        queue.pop();
        return result;
    }


    template<typename Rep,
             typename Period>
    std::expected<T, async_queue_error>
    try_pop_for(const std::chrono::duration<Rep, Period>& timeout)
        noexcept
    {
        std::unique_lock guard{mutex, timeout};
        if (!guard)
            return std::unexpected{async_queue_error::locked};

        if (should_stop)
            return std::unexpected{async_queue_error::stop};

        if (queue.empty())
            return std::unexpected{async_queue_error::empty};

        T result = std::move(queue.front());
        queue.pop();
        return result;
    }

}; // class async_queue

#endif
