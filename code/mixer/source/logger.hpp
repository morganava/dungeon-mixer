#pragma once

#include "singleton.hpp"
#include "enums.hpp"

#define MX_LOG_ERROR(...) tabletopify::g_logger.log(tabletopify::log_level::error, __VA_ARGS__)
#define MX_LOG_ERROR_BLOCKING(...) tabletopify::g_logger.blocking_log(tabletopify::log_level::error, __VA_ARGS__)
#define MX_LOG_INFO(...) tabletopify::g_logger.log(tabletopify::log_level::info, __VA_ARGS__)
#define MX_LOG_INFO_BLOCKING(...) tabletopify::g_logger.blocking_log(tabletopify::log_level::info, __VA_ARGS__)
#define MX_LOG_TRACE(...) MX_LOG_INFO(__FILE__"(" __STR_LINE__ "): " __VA_ARGS__)
#define MX_LOG_TRACE_BLOCKING(...) MX_LOG_INFO_BLOCKING(__FILE__"(" __STR_LINE__ "): " __VA_ARGS__)
#define MX_LOG_VAR(X) MX_LOG_INFO(#X " : {}", X);
#define MX_LOG_VAR_BLOCKING(X) MX_LOG_INFO_BLOCKING(#X " : {}", X);

namespace tabletopify
{
    MX_MAKE_ENUM(log_level, uint_fast8_t,
        error,
        info
    );

    class logger
    {
    public:
        logger(log_level_t logLevel);

        void log(log_level_t lvl, std::string msg);

        template<size_t N>
        void log(log_level_t lvl, const char (&msg)[N])
        {
            log_internal(lvl, false, std::forward<decltype(msg)>(msg));
        }

        template<size_t N, typename... Args>
        void log(log_level_t lvl, const char (&format)[N], Args... args)
        {
            log_internal(lvl, false, std::forward<decltype(format)>(format), std::move(args)...);
        }

        void blocking_log(log_level_t lvl, std::string msg);

        template<size_t N>
        void blocking_log(log_level_t lvl, const char (&msg)[N])
        {
            log_internal(lvl, true, std::forward<decltype(msg)>(msg));
        }

        template<size_t N, typename... Args>
        void blocking_log(log_level_t lvl, const char (&format)[N], Args... args)
        {
            log_internal(lvl, true, std::forward<decltype(format)>(format), std::move(args)...);
        }

    private:
        void write_message(log_level_t lvl, std::chrono::system_clock::time_point now, const char* msg);

        void log_internal(log_level_t lvl, bool blocking, std::string&& msg);
        template<size_t N>
        void log_internal(log_level_t lvl, bool blocking, const char (&msg)[N]);
        template<size_t N, typename... Args>
        void log_internal(log_level_t lvl, bool blocking, const char (&format)[N], Args&&... args);

        log_level_t _log_level;
        FILE* _log_file;
        const std::chrono::system_clock::time_point _start = std::chrono::system_clock::now();
        constexpr static const char* _log_format_strings[] =
        {
            "[{:f}][ERROR] {}\n",
            "[{:f}][INFO] {}\n",
        };
    };
    MX_MAKE_SINGLETON(logger);
}
#include "work_queue.hpp"

namespace tabletopify
{
    template<size_t N>
    void logger::log_internal(log_level_t lvl, bool blocking, const char (&msg)[N])
    {
        auto now = std::chrono::system_clock::now();
        auto log_task = [=]() -> void
        {
            g_logger.write_message(lvl, now, msg);
        };

        if (blocking) {
            worker_logging.enqueue_blocking(std::move(log_task));
        } else {
            worker_logging.enqueue(std::move(log_task));
        }
    }

    template<size_t N, typename... Args>
    void logger::log_internal(log_level_t lvl, bool blocking, const char (&format)[N], Args&&... args)
    {
        auto now = std::chrono::system_clock::now();
        auto log_task = [=, ...args=std::move(args)]() -> void
        {
            auto msg = fmt::format(format, std::move(args)...);
            g_logger.write_message(lvl, now, msg.c_str());
        };

        if (blocking) {
            worker_logging.enqueue_blocking(std::move(log_task));
        } else {
            worker_logging.enqueue(std::move(log_task));
        }
    }
}