#pragma once

#include "enums.hpp"

namespace dm
{
    MX_MAKE_ENUM(worker_id, size_t,
        ui,
        fileio,
        background,
        audio,
        logging
    );

    MX_MAKE_FLAG_ENUM(worker_flag, size_t,
        ui         = 1 << static_cast<size_t>(worker_id::ui),
        fileio     = 1 << static_cast<size_t>(worker_id::fileio),
        background = 1 << static_cast<size_t>(worker_id::background),
        audio      = 1 << static_cast<size_t>(worker_id::audio),
        logging    = 1 << static_cast<size_t>(worker_id::logging),

        any = worker_flag::ui         |
              worker_flag::fileio     |
              worker_flag::background |
              worker_flag::audio      |
              worker_flag::logging
    );

    template<typename T>
    struct allowed_threads {
        constexpr static worker_flag_t value = worker_flag::any;
    };

    // utility function for singleton
    bool can_call_from_current_thread(worker_flag_t allowedThreads);
}

