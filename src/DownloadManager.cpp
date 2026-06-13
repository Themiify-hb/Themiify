/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

#include <curl/curl.h>

#include "DownloadManager.h"
#include "screens/DownloadThemePopup.h"
#include "utils.h"
#include "tracer.hpp"

using std::cout;
using std::cerr;
using std::endl;
using namespace std::literals;

namespace DownloadManager {

    std::string user_agent;

    struct Download {
        std::shared_ptr<Info> info;

        success_function_t success_func;
        failure_function_t failure_func;

        CURL* utheme_easy = nullptr;
        CURL* thumbnail_easy = nullptr;

        std::filebuf utheme_file;
        std::filebuf thumbnail_file;

        std::filesystem::path thumbnail_output;

        bool utheme_content_started = false;
        bool thumbnail_content_started = false;

        bool utheme_done = false;
        bool thumbnail_done = false;

        Download(Download&&) = delete;

        Download(std::shared_ptr<Info> info_,
                 success_function_t success_func_,
                 failure_function_t failure_func_)
            : info{std::move(info_)},
              success_func{std::move(success_func_)},
              failure_func{std::move(failure_func_)}
        {
            create_directories(info->utheme_output.parent_path());
            create_directories(info->thumbnail_output.parent_path());

            if (!utheme_file.open(info->utheme_output, std::ios::out | std::ios::binary | std::ios::trunc))
                throw std::runtime_error{"could not open "s + info->utheme_output.string()};

            if (!thumbnail_file.open(info->thumbnail_output, std::ios::out | std::ios::binary | std::ios::trunc))
                throw std::runtime_error{"could not open "s + info->thumbnail_output.string()};

            setup_easy(utheme_easy, info->utheme_url, this, true);
            setup_easy(thumbnail_easy, info->thumbnail_url, this, false);
        }

        ~Download()
        {
            if (utheme_easy) {
                curl_easy_cleanup(utheme_easy);
                utheme_easy = nullptr;
            }

            if (thumbnail_easy) {
                curl_easy_cleanup(thumbnail_easy);
                thumbnail_easy = nullptr;
            }
        }

        static void setup_easy(CURL*& easy,
                               const std::string& url,
                               Download* self,
                               bool is_utheme)
        {
            easy = curl_easy_init();
            if (!easy)
                throw std::runtime_error{"curl_easy_init() failed"};

            if (!user_agent.empty())
                curl_easy_setopt(easy, CURLOPT_USERAGENT, user_agent.c_str());

            curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
            curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(easy, CURLOPT_AUTOREFERER, 1L);
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");
            curl_easy_setopt(easy, CURLOPT_TRANSFER_ENCODING, 1L);
            curl_easy_setopt(easy, CURLOPT_BUFFERSIZE, 65536L);
            curl_easy_setopt(easy, CURLOPT_TCP_NODELAY, 0L);
            curl_easy_setopt(easy, CURLOPT_FAILONERROR, 1L);

            curl_easy_setopt(easy, CURLOPT_WRITEDATA, self);
            curl_easy_setopt(easy,
                             CURLOPT_WRITEFUNCTION,
                             is_utheme ? utheme_write_callback : thumbnail_write_callback);

            curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(easy, CURLOPT_XFERINFODATA, self);
            curl_easy_setopt(easy, CURLOPT_XFERINFOFUNCTION, progress_callback);
        }

        static size_t utheme_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
        {
            auto* self = static_cast<Download*>(userdata);
            const size_t total = size * nmemb;

            self->utheme_content_started = true;

            return self->utheme_file.sputn(ptr, total);
        }

        static size_t thumbnail_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
        {
            auto* self = static_cast<Download*>(userdata);
            const size_t total = size * nmemb;

            self->thumbnail_content_started = true;

            return self->thumbnail_file.sputn(ptr, total);
        }

        static int progress_callback(void* userdata,
                                     curl_off_t dltotal,
                                     curl_off_t dlnow,
                                     curl_off_t,
                                     curl_off_t)
        {
            auto* self = static_cast<Download*>(userdata);

            if (self->utheme_content_started && dltotal)
                self->info->progress = float(dlnow) / float(dltotal);

            double speed = 0.0;

            if (self->utheme_easy)
                curl_easy_getinfo(self->utheme_easy, CURLINFO_SPEED_DOWNLOAD_T, &speed);

            self->info->speed = static_cast<std::uint64_t>(speed);

            return 0;
        }

        bool owns(CURL* easy) const
        {
            return easy == utheme_easy || easy == thumbnail_easy;
        }

        void mark_done(CURL* easy)
        {
            if (easy == utheme_easy)
                utheme_done = true;
            else if (easy == thumbnail_easy)
                thumbnail_done = true;
        }

        bool is_done() const
        {
            return utheme_done && thumbnail_done;
        }

        void finish()
        {
            utheme_file.close();
            thumbnail_file.close();

            if (success_func)
                success_func(*info);
        }

        void finish(const std::exception& e) noexcept
        try {
            utheme_file.close();
            thumbnail_file.close();

            if (failure_func)
                failure_func(e);
        }
        catch (...) {
        }
    };

    struct Resources {
        CURLM* multi = nullptr;

        std::vector<std::shared_ptr<const Info>> infos;
        std::list<Download> downloads;

        Resources()
        {
            TRACE_FUNC;

            multi = curl_multi_init();
            if (!multi)
                throw std::runtime_error{"curl_multi_init() failed"};

            curl_multi_setopt(multi, CURLMOPT_MAXCONNECTS, 5L);
            curl_multi_setopt(multi, CURLMOPT_MAX_TOTAL_CONNECTIONS, 5L);
            curl_multi_setopt(multi, CURLMOPT_MAX_HOST_CONNECTIONS, 1L);
        }

        ~Resources() noexcept
        {
            for (auto& d : downloads) {
                if (d.utheme_easy)
                    curl_multi_remove_handle(multi, d.utheme_easy);

                if (d.thumbnail_easy)
                    curl_multi_remove_handle(multi, d.thumbnail_easy);
            }

            if (multi) {
                curl_multi_cleanup(multi);
                multi = nullptr;
            }
        }

        void process()
        {
            int running = 0;
            curl_multi_perform(multi, &running);

            int msgs_left = 0;
            while (auto* msg = curl_multi_info_read(multi, &msgs_left)) {
                if (msg->msg != CURLMSG_DONE)
                    continue;

                CURL* completed_easy = msg->easy_handle;

                auto completed = std::ranges::find_if(
                    downloads,
                    [completed_easy](const Download& d) {
                        return d.owns(completed_easy);
                    }
                );

                try {
                    if (completed == downloads.end())
                        throw std::logic_error{"BUG: transfer not found"};

                    if (msg->data.result != CURLE_OK) {
                        throw std::runtime_error{
                            curl_easy_strerror(msg->data.result)
                        };
                    }

                    completed->mark_done(completed_easy);
                }
                catch (std::exception& e) {
                    cerr << "DownloadManager::Resources::process(): ERROR: "
                         << e.what()
                         << endl;

                    if (completed != downloads.end())
                        completed->finish(e);

                    if (completed != downloads.end()) {
                        if (completed->utheme_easy)
                            curl_multi_remove_handle(multi, completed->utheme_easy);

                        if (completed->thumbnail_easy)
                            curl_multi_remove_handle(multi, completed->thumbnail_easy);

                        downloads.erase(completed);
                    }

                    continue;
                }

                if (completed != downloads.end()) {
                    curl_multi_remove_handle(multi, completed_easy);

                    if (completed->is_done()) {
                        completed->finish();
                        downloads.erase(completed);
                    }
                }
            }
        }

        const std::vector<std::shared_ptr<const Info>>& get_infos() const
        {
            return infos;
        }

        bool add(const std::string& label,
                 const std::string& utheme_url,
                 const std::string& thumbnail_url,
                 const std::filesystem::path& utheme_output,
                 const std::filesystem::path& thumbnail_output,
                 success_function_t success_func,
                 failure_function_t failure_func)
        {
            for (auto& entry : infos) {
                if (entry->utheme_url == utheme_url)
                    return false;
            }

            auto sanitized_utheme_output = removeNonASCII(std::string(utheme_output));
            auto sanitized_thumbnail_output = removeNonASCII(std::string(thumbnail_output));

            auto info = std::make_shared<Info>(
                label,
                utheme_url,
                thumbnail_url,
                sanitized_utheme_output,
                sanitized_thumbnail_output,
                0.0f,
                0,
                State::queued
            );

            infos.push_back(info);

            downloads.emplace_back(
                std::move(info),
                std::move(success_func),
                std::move(failure_func)
            );

            auto& download = downloads.back();

            curl_multi_add_handle(multi, download.utheme_easy);
            curl_multi_add_handle(multi, download.thumbnail_easy);

            cout << "Added download:"
                 << "\n    " << label
                 << "\n    " << utheme_url
                 << "\n    " << thumbnail_url
                 << "\n    " << utheme_output
                 << "\n    " << thumbnail_output
                 << endl;

            return true;
        }
    };

    std::optional<Resources> res;

    void initialize(const std::string& new_user_agent)
    {
        TRACE_FUNC;

        curl_global_init(CURL_GLOBAL_DEFAULT);

        user_agent = new_user_agent;

        res.emplace();
    }

    void finalize()
    {
        TRACE_FUNC;

        res.reset();

        curl_global_cleanup();
    }

    void process()
    {
        if (res)
            res->process();
    }

    void pause_all()
    {
        TRACE_FUNC;
    }

    void resume_all()
    {
        TRACE_FUNC;
    }

    void cancel_all()
    {
        TRACE_FUNC;
    }

    void clear_finished()
    {
        res->infos.clear();
    }

    bool add(const std::string& label,
             const std::string& utheme_url,
             const std::string& thumbnail_url,
             const std::filesystem::path& utheme_output,
             const std::filesystem::path& thumbnail_output,
             success_function_t success_func,
             failure_function_t failure_func)
    {
        TRACE_FUNC;
        assert(res);

        return res->add(
            label,
            utheme_url,
            thumbnail_url,
            utheme_output,
            thumbnail_output,
            std::move(success_func),
            std::move(failure_func)
        );
    }

    void pause(const std::string& url)
    {
        TRACE_FUNC;
        assert(res);

        cout << "Pausing " << url << endl;
    }

    void cancel(const std::string& url)
    {
        TRACE_FUNC;
        assert(res);

        cout << "Canceling " << url << endl;
    }

    const std::vector<std::shared_ptr<const Info>>& get_infos()
    {
        assert(res);
        return res->get_infos();
    }

}