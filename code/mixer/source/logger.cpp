#include "paths.hpp"
#include "logger.hpp"

namespace dm
{
    logger::logger(log_level_t lvl)
    : _log_level(lvl)
    , _log_file(nullptr)
    , _start(std::chrono::steady_clock::now())
    {
        const auto logPath = paths::temp() / "dm.log";
        _log_file = fopen(logPath.string().c_str(), "wb");
        SMOL_ASSERT(_log_file != nullptr);
    }

    void logger::write_message(log_level_t lvl, std::chrono::steady_clock::time_point now, const std::string& msg)
    {
        std::chrono::duration<double> duration(now - _start);

        std::string fullMsg = fmt::format(_log_format_strings[static_cast<uint_fast8_t>(lvl)], duration.count(), msg);

        std::fwrite(fullMsg.c_str(), sizeof(char), fullMsg.size(), stdout);
        std::fwrite(fullMsg.c_str(), sizeof(char), fullMsg.size(), _log_file);

        std::fflush(stdout);
        std::fflush(_log_file);
    }
}
