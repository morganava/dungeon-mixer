#pragma once

#include "singleton.hpp"
#include "work_queue.hpp"

#define MX_LOG_ERROR(...) get_logger()->log(tabletopify::LogLevel_Error, __VA_ARGS__)
#define MX_LOG_INFO(...) get_logger()->log(tabletopify::LogLevel_Info, __VA_ARGS__)
#define MX_LOG_TRACE(...) get_logger()->log(tabletopify::LogLevel_Info, __FILE__"(" __STR_LINE__ "): " __VA_ARGS__)
#define MX_LOG_VAR(X) MX_LOG_INFO(#X " : {}", X);
#define MX_LOG_INFO_BLOCKING(...) get_logger()->blocking_log(tabletopify::LogLevel_Info, __VA_ARGS__)
namespace tabletopify
{
    typedef enum
    {
        LogLevel_Error = 0,
        LogLevel_Info,

        LogLevel_Count,
    } log_level_t;

    class logger
    {
    public:
        logger(log_level_t logLevel);

        void log(log_level_t lvl, std::string msg);

        template<size_t N>
        void log(log_level_t lvl, const char (&msg)[N])
        {
            log_internal(lvl, nullptr, std::forward<decltype(msg)>(msg));
        }

        template<size_t N, typename... Args>
        void log(log_level_t lvl, const char (&format)[N], Args... args)
        {
            log_internal(lvl, nullptr, std::forward<decltype(format)>(format), std::move(args)...);
        }

        void blocking_log(log_level_t lvl, std::string msg);

        template<size_t N>
        void blocking_log(log_level_t lvl, const char (&msg)[N])
        {
            volatile bool finished = false;
            log_internal(lvl, &finished, std::forward<decltype(msg)>(msg));
        }

        template<size_t N, typename... Args>
        void blocking_log(log_level_t lvl, const char (&format)[N], Args... args)
        {
            volatile bool finished = false;
            log_internal(lvl, &finished, std::forward<decltype(format)>(format), std::move(args)...);
        }

    private:
        void write_message(log_level_t lvl, std::chrono::system_clock::time_point now, const char* msg);

        void log_internal(log_level_t lvl, volatile bool* finished, std::string&& msg);
        template<size_t N>
        void log_internal(log_level_t lvl, volatile bool* finished, const char (&msg)[N]);
        template<size_t N, typename... Args>
        void log_internal(log_level_t lvl, volatile bool* finished, const char (&format)[N], Args&&... args);

        void wait(volatile bool* finished);

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

    template<size_t N>
    void logger::log_internal(log_level_t lvl, volatile bool* finished, const char (&msg)[N])
    {
        auto now = std::chrono::system_clock::now();
        worker(Worker_Logging).enqueue([=]() -> void
        {
            g_logger.write_message(lvl, now, msg);
        }, finished);
        wait(finished);
    }

    template<size_t N, typename... Args>
    void logger::log_internal(log_level_t lvl, volatile bool* finished, const char (&format)[N], Args&&... args)
    {
        auto now = std::chrono::system_clock::now();
        worker(Worker_Logging).enqueue([=, ...args=std::move(args)]() -> void
        {
            auto msg = fmt::format(format, args...);
            g_logger.write_message(lvl, now, msg.c_str());
        }, finished);
        wait(finished);
    }
}