#include "precomp.hpp"

#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace tabletopify
{
    static log_level_t log_level = LogLevel_Info;
    const static std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

    const char* log_level_names[] =
    {
        "[ERROR]",
        "[INFO]",
    };

    void init_logger(log_level_t lvl)
    {
        log_level = lvl;
    }

    void log_message(log_level_t lvl,
                     std::chrono::system_clock::time_point now,
                     const char* msg)
    {
        MX_ASSERT(lvl < LogLevel_Count);

        if(lvl <= log_level)
        {
            std::chrono::duration<double> duration(now - start);
            std::cout << '[' << duration.count() << ']' << log_level_names[lvl] << " " << msg << std::endl;
        }
    }

    void log(log_level_t lvl, const char* msg)
    {
        auto now = std::chrono::system_clock::now();
        worker(Worker_Logging).enqueue([=]() -> void
        {
            log_message(lvl, now, msg);
        });
    }

    void log(log_level_t lvl, std::string&& msg)
    {
        auto now = std::chrono::system_clock::now();
        worker(Worker_Logging).enqueue([lvl=lvl, now=now, msg=std::forward<std::string>(msg)]() -> void
        {
            log_message(lvl, now, msg.c_str());
        });
    }


    void assertion_log(const char* msg)
    {
        volatile bool finished = false;
        auto now = std::chrono::system_clock::now();
        worker(Worker_Logging).enqueue([=]() -> void
        {
            log_message(LogLevel_Error, now, msg);
        },
        &finished);
        while(!finished) std::this_thread::yield();
    }
}
