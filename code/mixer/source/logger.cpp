#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

class date {
  int year_, month_, day_;
public:
  date(int year, int month, int day): year_(year), month_(month), day_(day) {}

  friend std::ostream&operator<<(std::ostream&os, const date &d) {
    return os << d.year_ << '-' << d.month_ << '-' << d.day_;
  }
};


namespace tabletopify
{
    const std::chrono::system_clock::time_point logger::_start = std::chrono::system_clock::now();
    static logger* g_logger = nullptr;

    logger* get_logger()
    {
        return g_logger;
    }

    void init_logger(log_level_t lvl)
    {
        MX_ASSERT(g_logger == nullptr);
        g_logger = new logger(lvl);
    }

    void free_logger()
    {
        delete g_logger;
        g_logger = nullptr;
    }

    logger::logger(log_level_t lvl)
    : _log_level(lvl)
    , _log_file(nullptr)
    {
        _log_file = fopen("/tmp/tabletopify.log", "wb");
        MX_ASSERT(_log_file != nullptr);
    }

    void logger::write_message(log_level_t lvl, double duration, const char* msg)
    {
        std::string fullMsg = fmt::format(_log_format_strings[lvl], duration, msg);

        std::fwrite(fullMsg.c_str(), sizeof(char), fullMsg.size(), stdout);
        std::fwrite(fullMsg.c_str(), sizeof(char), fullMsg.size(), _log_file);

        std::fflush(stdout);
        std::fflush(_log_file);
    }
}
