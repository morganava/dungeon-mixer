#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace tabletopify
{
    static log_level_t log_level = LogLevel_Info;
    const static std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
    FILE* log_file = nullptr;
    constexpr auto log_format_string = "[%f]%s %s\n";

    const char* log_level_names[] =
    {
        "[ERROR]",
        "[INFO]",
    };

    void init_logger(log_level_t lvl)
    {
        log_file = fopen("/tmp/tabletopify.log", "wb");
        MX_ASSERT(log_file != nullptr);

        log_level = lvl;
    }

    void free_logger()
    {
        std::fflush(stdout);
        std::fclose(log_file);
    }

    static void log_message(log_level_t lvl,
                            std::string&& msg,
                            volatile bool* finished=nullptr)
    {
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> duration(now - start);

        worker(Worker_Logging).enqueue([lvl=lvl, count=duration.count(), msg=std::move(msg)]() -> void
        {
            std::fprintf(stdout, log_format_string, count, log_level_names[lvl], msg.data());
            std::fprintf(log_file, log_format_string, count, log_level_names[lvl], msg.data());

            std::fflush(stdout);
            std::fflush(log_file);
        }, finished);
    }

    void log(log_level_t lvl, std::string&& msg)
    {
        MX_ASSERT(lvl < LogLevel_Count);
        if(lvl <= log_level)
        {
            log_message(lvl, std::move(msg));
        }
    }

    void log(log_level_t lvl, const char* format, ...)
    {
        MX_ASSERT(lvl < LogLevel_Count);
        if(lvl <= log_level)
        {
            va_list args;
            va_start(args, format);
            std::string msg = vstring_format(format, args);
            va_end(args);

            log_message(lvl, std::move(msg));
        }
    }

    void log(log_level_t lvl, const std::ostream& strm)
    {
        MX_ASSERT(lvl < LogLevel_Count);
        if(lvl <= log_level)
        {
            std::string msg = dynamic_cast<const std::stringstream&>(strm).str();

            log_message(lvl, std::move(msg));
        }
    }

    void assertion_log(const char* msg)
    {
        volatile bool finished = false;
        log_message(LogLevel_Error, msg, &finished);
        while(!finished) std::this_thread::yield();
    }
}
