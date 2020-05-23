#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace tabletopify
{
    logger::logger(log_level_t lvl)
    : _log_level(lvl)
    , _log_file(nullptr)
    {
        _log_file = fopen("/tmp/tabletopify.log", "wb");
        MX_ASSERT(_log_file != nullptr);
    }

    void logger::write_message(log_level_t lvl, std::chrono::system_clock::time_point now, const char* msg)
    {
        std::chrono::duration<double> duration(now - _start);

        std::string fullMsg = fmt::format(_log_format_strings[static_cast<uint_fast8_t>(lvl)], duration.count(), msg);

        std::fwrite(fullMsg.c_str(), sizeof(char), fullMsg.size(), stdout);
        std::fwrite(fullMsg.c_str(), sizeof(char), fullMsg.size(), _log_file);

        std::fflush(stdout);
        std::fflush(_log_file);
    }

    void logger::log(log_level_t lvl, std::string msg)
    {
        log_internal(lvl, false, std::move(msg));
    }

    void logger::blocking_log(log_level_t lvl, std::string msg)
    {
        log_internal(lvl, true, std::move(msg));
    }

    void logger::log_internal(log_level_t lvl, bool blocking, std::string&& msg)
    {
        auto now = std::chrono::system_clock::now();

        auto fut = worker_logging.enqueue([=,msg=std::move(msg)]() -> void
        {
            g_logger.write_message(lvl, now, msg.c_str());
        });

        if (blocking) {
            fut.wait();
        }

    }
}
