#pragma once

#include "singleton.hpp"
#include "work_queue.hpp"

#define MX_LOG_ERROR(...) dm::g_logger->log<dm::log_level::error>(__VA_ARGS__)
#define MX_LOG_ERROR_BLOCKING(...) MX_LOG_ERROR(__VA_ARGS__).try_wait()
#define MX_LOG_INFO(...) dm::g_logger->log<dm::log_level::info>(__VA_ARGS__)
#define MX_LOG_INFO_BLOCKING(...) MX_LOG_INFO(__VA_ARGS__).try_wait()
#define MX_LOG_TRACE(...) MX_LOG_INFO(__FILE__"(" __STR_LINE__ "): " __VA_ARGS__)
#define MX_LOG_TRACE_BLOCKING(...) MX_LOG_TRACE(__VA_ARGS__).try_wait()
#define MX_LOG_VAR(X) MX_LOG_INFO(#X " : {}", X);
#define MX_LOG_VAR_BLOCKING(X) MX_LOG_INFO_BLOCKING(#X " : {}", X);

namespace dm
{
    SMOL_MAKE_ENUM(log_level, uint_fast8_t,
        error,
        info
    );

    class logger
    {
    public:
        logger(log_level_t logLevel);

        template<log_level_t LEVEL, size_t N, typename... Args>
        task_future log(char const (&format)[N], Args... args)
        {
            const auto now = std::chrono::system_clock::now();
            return worker_logging.enqueue(
                [format=std::forward<decltype(format)>(format), now=now, ...args=logger::marshall(args)]() -> void
                {
                    auto msg = fmt::format(format, std::move(args)...);
                    singleton<logger>::instance.unsafe_get().write_message(LEVEL, now, msg.c_str());
                });
        };

        template<log_level_t LEVEL>
        task_future log(std::string&& msg)
        {
            const auto now = std::chrono::system_clock::now();
            return worker_logging.enqueue(
                [msg=std::move(msg), now=now]() -> void
                {
                    singleton<logger>::instance.unsafe_get().write_message(LEVEL, now, msg);
                });
        };

    private:
        template<typename T>
        std::remove_reference<T>::type&& marshall(T&& t)
        {
            return std::move(t);
        }

        std::string marshall(const char* cstr)
        {
            return std::string(cstr);
        }

        std::string marshall(std::string_view sv)
        {
            return std::string(sv);
        }

        void write_message(log_level_t lvl, std::chrono::system_clock::time_point now, const std::string& msg);

        log_level_t _log_level;
        FILE* _log_file;
        const std::chrono::system_clock::time_point _start;
        constexpr static const char* _log_format_strings[] =
        {
            "[{:f}][ERROR] {}\n",
            "[{:f}][INFO] {}\n",
        };
    };
    MX_MAKE_SINGLETON(logger, worker_flag::any);
}
