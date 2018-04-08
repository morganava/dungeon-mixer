#pragma once

namespace tabletopify
{
    typedef enum
    {
        LogLevel_Error = 0,
        LogLevel_Info,

        LogLevel_Count,
    } log_level_t;

    void init_logger(log_level_t lvl);

    void log(log_level_t lvl, std::string&& msg);
    void log(log_level_t lvl, const char* format, ...);
    void log(log_level_t lvl, const std::ostream& msg);

    void assertion_log(const char* msg);
}