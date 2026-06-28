/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <concepts>
#include <filesystem>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <optional>
#include <print>
#include <queue>
#include <ranges>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include <curl/curl.h>
#include <SDL2/SDL_image.h>
#include <imgui_stdlib.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include "ImageLoader.h"

#include "App.h"
#include "async_queue.hpp"
#include "thread_safe.hpp"
#include "tracer.hpp"

using std::cout;
using std::cerr;
using std::endl;
using namespace std::literals;

namespace ImageLoader {
    std::string user_agent = App::user_agent;

    std::filesystem::path content_prefix = "fs:/vol/content";

    const std::size_t max_cache_size = 64;

    std::uint64_t use_counter = 0;

    SDL_Renderer *renderer = nullptr;
    SDL_Texture *load_error_image = nullptr;
    SDL_Texture *loading_image = nullptr;

    CURLM *multi = nullptr;

    enum class LoadState : int {
        unloaded,
        requested,
        loading,
        loaded,
        error,
    };

    std::string to_string(LoadState st);

    struct CacheEntry {
        std::atomic<LoadState> state{LoadState::unloaded};
        std::uint64_t last_use = 0;

        SDL_Surface *img = nullptr;
        SDL_Texture *tex = nullptr;

        CURL *easy = nullptr;
        curl_slist *headers = nullptr;

        std::optional<std::vector<char>> raw_buf;
        std::string location;
    };

    using cache_t = std::unordered_map<std::string, CacheEntry>;
    thread_safe<cache_t> safe_cache;

    async_queue<std::string> requests_queue;
    std::jthread worker_thread;

    static void destroy_entry(CacheEntry& entry)
    {
        if (multi && entry.easy)
            curl_multi_remove_handle(multi, entry.easy);

        if (entry.headers) {
            curl_slist_free_all(entry.headers);
            entry.headers = nullptr;
        }

        if (entry.easy) {
            curl_easy_cleanup(entry.easy);
            entry.easy = nullptr;
        }

        if (entry.tex) {
            SDL_DestroyTexture(entry.tex);
            entry.tex = nullptr;
        }

        if (entry.img) {
            SDL_FreeSurface(entry.img);
            entry.img = nullptr;
        }

        entry.raw_buf.reset();
    }

    static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata)
    {
        auto* entry = static_cast<CacheEntry*>(userdata);
        const std::size_t total = size * nmemb;

        if (!entry->raw_buf)
            entry->raw_buf.emplace();

        entry->raw_buf->insert(entry->raw_buf->end(), ptr, ptr + total);

        return total;
    }

    void worker_func(std::stop_token token);

    void initialize(SDL_Renderer* rend)
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);

        renderer = rend;

        loading_image = IMG_LoadTexture(
            renderer,
            "fs:/vol/content/ui/theme-placeholder-icon.png");

        load_error_image = IMG_LoadTexture(
            renderer,
            "fs:/vol/content/ui/load-error-image.png");

        if (loading_image)
            SDL_SetTextureBlendMode(loading_image, SDL_BLENDMODE_BLEND);

        if (load_error_image)
            SDL_SetTextureBlendMode(load_error_image, SDL_BLENDMODE_BLEND);

        requests_queue.reset();

        cout << "ImageLoader: launching worker thread." << endl;
        worker_thread = std::jthread{worker_func};

        assert((std::atomic<LoadState>{}.is_lock_free()));
    }

    void finalize()
    {
        cout << "Stopping requests_queue" << endl;
        requests_queue.stop();

        cout << "Destroying thread" << endl;
        worker_thread = {};
        cout << "Thread destroyed" << endl;

        {
            cout << "Clearing safe_cache" << endl;
            auto cache = safe_cache.lock();

            for (auto& [location, entry] : *cache)
                destroy_entry(entry);

            cache->clear();
        }

        cout << "Destroying predefined icons" << endl;

        if (loading_image) {
            SDL_DestroyTexture(loading_image);
            loading_image = nullptr;
        }

        if (load_error_image) {
            SDL_DestroyTexture(load_error_image);
            load_error_image = nullptr;
        }

        curl_global_cleanup();
    }

    SDL_Texture* get(const std::string& location)
    {
        ++use_counter;

        auto cache = safe_cache.lock();
        auto it = cache->find(location);

        try {
            if (it != cache->end()) {
                auto& entry = it->second;
                entry.last_use = use_counter;

                switch (entry.state.load()) {
                    case LoadState::loaded:
                        if (!entry.tex && entry.img) {
                            entry.tex = SDL_CreateTextureFromSurface(renderer, entry.img);
                            if (entry.tex)
                                SDL_SetTextureBlendMode(entry.tex, SDL_BLENDMODE_BLEND);

                            SDL_FreeSurface(entry.img);
                            entry.img = nullptr;
                        }

                        if (entry.tex)
                            return entry.tex;

                        return load_error_image;

                    case LoadState::error:
                        return load_error_image;

                    case LoadState::requested:
                    case LoadState::loading:
                        return loading_image;

                    case LoadState::unloaded:
                        entry.state = LoadState::requested;
                        requests_queue.push(location);
                        return loading_image;

                    default:
                        throw std::logic_error{
                            "invalid entry state: " + to_string(entry.state.load())
                        };
                }
            }

            auto& entry = (*cache)[location];
            entry.state = LoadState::requested;
            entry.last_use = use_counter;

            requests_queue.push(location);

            return loading_image;
        }
        catch (std::exception& e) {
            cerr << "ERROR: ImageLoader::get(): " << e.what() << endl;
            return load_error_image;
        }
    }

    CacheEntry* find(thread_safe<cache_t>::guard<cache_t>& cache, CURL* easy)
    {
        for (auto& [location, entry] : *cache) {
            if (entry.easy == easy)
                return &entry;
        }

        return nullptr;
    }

    void process_one_request(const std::string& location)
    {
        auto cache = safe_cache.lock();
        auto it = cache->find(location);

        if (it == cache->end())
            return;

        auto& entry = it->second;

        LoadState expected = LoadState::requested;
        if (!entry.state.compare_exchange_strong(expected, LoadState::loading)) {
            cerr << "ERROR: ImageLoader::process_one_request() wrong cache entry state: "
                 << to_string(expected)
                 << endl;
            return;
        }

        entry.location = location;

        try {
            if (location.starts_with("http://") || location.starts_with("https://")) {
                entry.easy = curl_easy_init();
                if (!entry.easy)
                    throw std::runtime_error{"curl_easy_init() failed"};

                entry.headers = curl_slist_append(entry.headers, "Accept: image/*");

                curl_easy_setopt(entry.easy, CURLOPT_URL, location.c_str());
                curl_easy_setopt(entry.easy, CURLOPT_HTTPHEADER, entry.headers);
                curl_easy_setopt(entry.easy, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(entry.easy, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(entry.easy, CURLOPT_AUTOREFERER, 1L);
                curl_easy_setopt(entry.easy, CURLOPT_ACCEPT_ENCODING, "");
                curl_easy_setopt(entry.easy, CURLOPT_TRANSFER_ENCODING, 1L);
                curl_easy_setopt(entry.easy, CURLOPT_BUFFERSIZE, 65536L);
                curl_easy_setopt(entry.easy, CURLOPT_TCP_NODELAY, 0L);
                curl_easy_setopt(entry.easy, CURLOPT_FAILONERROR, 1L);
                curl_easy_setopt(entry.easy, CURLOPT_WRITEFUNCTION, write_cb);
                curl_easy_setopt(entry.easy, CURLOPT_WRITEDATA, &entry);

                if (!user_agent.empty())
                    curl_easy_setopt(entry.easy, CURLOPT_USERAGENT, user_agent.c_str());

                curl_multi_add_handle(multi, entry.easy);
            }
            else if (location.starts_with("ui/")) {
                const auto path = content_prefix / location;

                entry.img = IMG_Load(path.c_str());
                if (!entry.img)
                    throw std::runtime_error{IMG_GetError()};

                entry.state = LoadState::loaded;
            }
            else if (location.starts_with("fs:/")) {
                entry.img = IMG_Load(location.c_str());
                if (!entry.img)
                    throw std::runtime_error{IMG_GetError()};
                entry.state = LoadState::loaded;
            }
            else {
                throw std::runtime_error{"invalid location"};
            }
        }
        catch (std::exception& e) {
            std::print(cerr,
                       "ERROR: ImageLoader::process_one_request(): location=\"{}\", exception={}",
                       location,
                       e.what());

            entry.state = LoadState::error;
        }
    }

    template<std::ranges::random_access_range R,
             typename Comp = std::ranges::less,
             class Proj = std::identity>
    requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
    void sift_down_heap(R&& r, Comp comp = {}, Proj proj = {})
    {
        if (std::empty(r))
            return;

        const auto size = std::size(r);
        std::ranges::range_size_t<R> cur_idx = 0;

        for (;;) {
            auto left_idx = 2u * cur_idx + 1u;

            if (left_idx >= size)
                break;

            auto next_idx = left_idx;
            auto right_idx = 2u * cur_idx + 2u;

            if (right_idx < size &&
                std::invoke(comp,
                            std::invoke(proj, r[left_idx]),
                            std::invoke(proj, r[right_idx]))) {
                next_idx = right_idx;
            }

            if (!std::invoke(comp,
                             std::invoke(proj, r[cur_idx]),
                             std::invoke(proj, r[next_idx]))) {
                break;
            }

            std::swap(r[cur_idx], r[next_idx]);
            cur_idx = next_idx;
        }
    }

    std::uint64_t& by_last_use(cache_t::iterator it) noexcept
    {
        return it->second.last_use;
    }

    void trim_cache()
    {
        auto cache = safe_cache.lock();

        if (cache->size() <= max_cache_size)
            return;

        std::size_t excess = cache->size() - max_cache_size;

        std::vector<cache_t::iterator> to_remove(excess);
        auto to_remove_end = to_remove.begin();

        for (auto it = cache->begin(); it != cache->end(); ++it) {
            if (to_remove_end != to_remove.end()) {
                *to_remove_end++ = it;
                std::ranges::push_heap(to_remove.begin(),
                                       to_remove_end,
                                       {},
                                       by_last_use);
            }
            else {
                if (by_last_use(it) < by_last_use(to_remove.front())) {
                    to_remove.front() = it;
                    sift_down_heap(to_remove, {}, by_last_use);
                }
            }
        }

        for (auto it : to_remove) {
            destroy_entry(it->second);
            cache->erase(it);
        }
    }

    void handle_finished_downloads()
    {
        int msgs_left = 0;

        while (auto* msg = curl_multi_info_read(multi, &msgs_left)) {
            if (msg->msg != CURLMSG_DONE)
                continue;

            CURL* easy = msg->easy_handle;

            auto cache = safe_cache.lock();
            auto* entry = find(cache, easy);

            if (!entry) {
                cerr << "ERROR: ImageLoader::handle_finished_downloads(): failed to find entry" << endl;
                curl_multi_remove_handle(multi, easy);
                curl_easy_cleanup(easy);
                continue;
            }

            try {
                if (msg->data.result != CURLE_OK) {
                    throw std::runtime_error{
                        "[" + entry->location + "] " +
                        curl_easy_strerror(msg->data.result)
                    };
                }

                char* content_type = nullptr;
                curl_easy_getinfo(easy, CURLINFO_CONTENT_TYPE, &content_type);

                std::string ct = content_type ? content_type : "";
                if (!ct.starts_with("image/")) {
                    throw std::runtime_error{
                        "Content-Type should be image/* but got \"" + ct + "\""
                    };
                }

                if (!entry->raw_buf || entry->raw_buf->empty())
                    throw std::runtime_error{"empty download"};

                SDL_RWops* rw = SDL_RWFromConstMem(entry->raw_buf->data(),
                                                   static_cast<int>(entry->raw_buf->size()));
                if (!rw)
                    throw std::runtime_error{SDL_GetError()};

                entry->img = IMG_Load_RW(rw, 1);
                if (!entry->img)
                    throw std::runtime_error{IMG_GetError()};

                entry->state = LoadState::loaded;
            }
            catch (std::exception& e) {
                cerr << "ERROR: ImageLoader::handle_finished_downloads(): " << e.what() << endl;
                entry->state = LoadState::error;
            }

            curl_multi_remove_handle(multi, entry->easy);

            if (entry->headers) {
                curl_slist_free_all(entry->headers);
                entry->headers = nullptr;
            }

            if (entry->easy) {
                curl_easy_cleanup(entry->easy);
                entry->easy = nullptr;
            }

            entry->raw_buf.reset();
        }
    }

    void worker_func(std::stop_token token)
    {
        try {
            multi = curl_multi_init();
            if (!multi)
                throw std::runtime_error{"curl_multi_init() failed"};

            curl_multi_setopt(multi, CURLMOPT_MAX_TOTAL_CONNECTIONS, 10L);
            curl_multi_setopt(multi, CURLMOPT_MAXCONNECTS, 10L);

            while (!token.stop_requested()) {
                auto location = requests_queue.try_pop_for(50ms);

                if (location) {
                    process_one_request(*location);
                }
                else if (location.error() == async_queue_error::stop) {
                    break;
                }
                else if (location.error() == async_queue_error::locked) {
                    cout << "WARNING: requests_queue was locked" << endl;
                }

                int running = 0;
                curl_multi_perform(multi, &running);

                handle_finished_downloads();
                trim_cache();

                std::this_thread::sleep_for(50ms);
            }
        }
        catch (std::exception& e) {
            cerr << "ERROR: ImageLoader::worker_func(): " << e.what() << endl;
        }

        if (multi) {
            curl_multi_cleanup(multi);
            multi = nullptr;
        }
    }

    std::string to_string(LoadState st)
    {
        switch (st) {
            case LoadState::unloaded:
                return "unloaded";
            case LoadState::requested:
                return "requested";
            case LoadState::loading:
                return "loading";
            case LoadState::loaded:
                return "loaded";
            case LoadState::error:
                return "error";
            default:
                return "unknown (" + std::to_string(static_cast<int>(st)) + ")";
        }
    }
}
