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
        assert(lvl < LogLevel_Count);

        if(lvl <= log_level)
        {
            std::chrono::duration<double> duration(now - start);
            std::cout << '[' << duration.count() << ']' << log_level_names[lvl] << " " << msg << std::endl;
        }
    }
}