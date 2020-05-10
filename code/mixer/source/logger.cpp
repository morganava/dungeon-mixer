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

        std::string fullMsg = fmt::format(_log_format_strings[lvl], duration.count(), msg);

        std::fwrite(fullMsg.c_str(), sizeof(char), fullMsg.size(), stdout);
        std::fwrite(fullMsg.c_str(), sizeof(char), fullMsg.size(), _log_file);

        std::fflush(stdout);
        std::fflush(_log_file);
    }

    void logger::log(log_level_t lvl, std::string msg)
    {
        log_internal(lvl, nullptr, std::move(msg));
    }

    void logger::blocking_log(log_level_t lvl, std::string msg)
    {
        volatile bool finished = false;
        log_internal(lvl, &finished, std::move(msg));
    }

    void logger::log_internal(log_level_t lvl, volatile bool* finished, std::string&& msg)
    {
        auto now = std::chrono::system_clock::now();
        worker(Worker_Logging).enqueue([=,msg=std::move(msg)]() -> void
        {
            g_logger.write_message(lvl, now, msg.c_str());
        }, finished);
        wait(finished);
    }

    void logger::wait(volatile bool* finished)
    {
        if (finished) {
            while(!*finished) {
                std::this_thread::yield();
            }
        }
    }
}
