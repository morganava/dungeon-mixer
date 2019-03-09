#pragma once

#include "work_queue.hpp"

#define MX_LOG_ERROR(...) get_logger()->log(tabletopify::LogLevel_Error, __VA_ARGS__)
#define MX_LOG_INFO(...) get_logger()->log(tabletopify::LogLevel_Info, __VA_ARGS__)
#define MX_LOG_TRACE(...) get_logger()->log(tabletopify::LogLevel_Info, __FILE__"(" __STR_LINE__ "): " __VA_ARGS__)
#define MX_LOG_INFO_BLOCKING(...) get_logger()->blocking_log(tabletopify::LogLevel_Info, __VA_ARGS__)
namespace tabletopify
{
    typedef enum
    {
        LogLevel_Error = 0,
        LogLevel_Info,

        LogLevel_Count,
    } log_level_t;

// the new shit
    class logger
    {
    public:
        logger(log_level_t logLevel);

        void log(log_level_t lvl, const std::string& msg)
        {
            auto now = std::chrono::system_clock::now();

            worker(Worker_Logging).enqueue([=]() -> void
            {
                std::chrono::duration<double> duration(now - _start);
                write_message(lvl, duration.count(), msg.c_str());
            });
        }

        template<size_t N>
        void log(log_level_t lvl, const char (&msg)[N])
        {
            auto now = std::chrono::system_clock::now();

            worker(Worker_Logging).enqueue([=]() -> void
            {
                std::chrono::duration<double> duration(now - _start);
                write_message(lvl, duration.count(), msg);
            });
        }

        template<size_t N, typename... Args>
        void log(log_level_t lvl, const char (&format)[N], const Args&... args)
        {
            auto now = std::chrono::system_clock::now();

            worker(Worker_Logging).enqueue([=]() -> void
            {
                std::chrono::duration<double> duration(now - _start);

                auto msg = fmt::format(format, args...);
                write_message(lvl, duration.count(), msg.c_str());
            });
        }

        void blocking_log(log_level_t lvl, const std::string& msg)
        {
            auto now = std::chrono::system_clock::now();

            volatile bool finished = false;
            worker(Worker_Logging).enqueue([=]() -> void
            {
                std::chrono::duration<double> duration(now - _start);
                write_message(lvl, duration.count(), msg.c_str());
            }, &finished);
            while(!finished) std::this_thread::yield();
        }

        template<size_t N>
        void blocking_log(log_level_t lvl, const char (&msg)[N])
        {
            auto now = std::chrono::system_clock::now();

            volatile bool finished = false;
            worker(Worker_Logging).enqueue([=]() -> void
            {
                std::chrono::duration<double> duration(now - _start);
                write_message(lvl, duration.count(), msg);
            }, &finished);
            while(!finished) std::this_thread::yield();
        }

        template<size_t N, typename... Args>
        void blocking_log(log_level_t lvl, const char (&format)[N], const Args&... args)
        {
            auto now = std::chrono::system_clock::now();

            volatile bool finished = false;
            worker(Worker_Logging).enqueue([=]() -> void
            {
                std::chrono::duration<double> duration(now - _start);

                auto msg = fmt::format(format, args...);
                write_message(lvl, duration.count(), msg.c_str());
            }, &finished);
            while(!finished) std::this_thread::yield();
        }

    private:
        void write_message(log_level_t lvl, double duration, const char* msg);

        log_level_t _log_level;
        FILE* _log_file;
        const static std::chrono::system_clock::time_point _start;
        constexpr static const char* _log_format_strings[] =
        {
            "[{:f}][ERROR] {}\n",
            "[{:f}][INFO] {}\n",
        };
    };

    logger* get_logger();
    void init_logger(log_level_t lvl);
    void free_logger();
}