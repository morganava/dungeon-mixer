#pragma once

#include "service.hpp"
#include "work_queue.hpp"

#define MX_LOG_ERROR(...) dm::g_logger->log<dm::log_level::error>(__VA_ARGS__)
#define MX_LOG_ERROR_BLOCKING(...) MX_LOG_ERROR(__VA_ARGS__).try_wait()
#define MX_LOG_INFO(...) dm::g_logger->log<dm::log_level::info>(__VA_ARGS__)
#define MX_LOG_INFO_BLOCKING(...) MX_LOG_INFO(__VA_ARGS__).try_wait()
#define MX_LOG_VERBOSE(...) dm::g_logger->log<dm::log_level::verbose>(__VA_ARGS__)
#define MX_LOG_VERBOSE_BLOCKING(...) MX_LOG_INFO(__VA_ARGS__).try_wait()
#define MX_LOG_TRACE(...) MX_LOG_INFO(__FILE__"(" __STR_LINE__ ") " __VA_ARGS__)
#define MX_LOG_TRACE_BLOCKING(...) MX_LOG_TRACE(__VA_ARGS__).try_wait()
#define MX_LOG_VAR(X) MX_LOG_INFO(#X " : {}", X);
#define MX_LOG_VAR_BLOCKING(X) MX_LOG_INFO_BLOCKING(#X " : {}", X);

namespace dm
{
    SMOL_MAKE_ENUM(log_level, uint_fast8_t,
        error,
        info,
        verbose
    );

    class logger
    {
    public:
        logger(log_level_t logLevel, utf8_string logFileName);

        template<log_level_t LEVEL, size_t N, typename... Args>
        task_future log(char const (&format)[N], Args... args)
        {
            if (LEVEL > this->_log_level)
            {
                return task_future::fulfilled();
            }

            const auto now = std::chrono::steady_clock::now();
            return worker_logging.enqueue(
                [format=std::forward<decltype(format)>(format), now=now, ...args=logger::marshall(args)]() -> void
                {
                    auto msg = fmt::format(format, std::move(args)...);
                    singleton<service<logger>>::instance->unsafe_get().write_message(LEVEL, now, msg.c_str());
                });
        };

        template<log_level_t LEVEL>
        task_future log(std::string&& msg)
        {
            if (LEVEL > this->_log_level)
            {
                return task_future::fulfilled();
            }

            const auto now = std::chrono::steady_clock::now();
            return worker_logging.enqueue(
                [msg=std::move(msg), now=now]() -> void
                {
                    singleton<service<logger>>::instance->unsafe_get().write_message(LEVEL, now, msg);
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
            if (cstr == nullptr)
            {
                return std::string("(null)");
            }
            return std::string(cstr);
        }

        std::string marshall(std::string_view sv)
        {
            return std::string(sv);
        }

        void write_message(log_level_t lvl, std::chrono::steady_clock::time_point now, const std::string& msg);

        log_level_t _log_level;
        FILE* _log_file;
        const std::chrono::steady_clock::time_point _start;
        constexpr static const char* _log_format_strings[] =
        {
            "[{:f}][ERROR] {}\n",
            "[{:f}][INFO] {}\n",
            "[{:f}][VERBOSE] {}\n",
        };
        static_assert(countof(_log_format_strings) == log_level::count);
    };
    MX_MAKE_SERVICE(logger, worker_flag::any);
}

SMOL_MAKE_ENUM_TO_JSON(dm, log_level)
SMOL_MAKE_ENUM_FROM_JSON(dm, log_level)