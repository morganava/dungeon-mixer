#include "string_manager.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "serialization.hpp"
#include "paths.hpp"

namespace dm
{
    string_manager::string_manager(fs::path&& stringsPath)
    {
        worker_fileio.enqueue([]() -> void
        {
            g_serialization_manager->load_strings(paths::strings_file());
        });
    }

    utf8_string const& string_manager::operator[](mixer_string_t str) const
    {
        return strings[static_cast<uint_fast16_t>(str)];
    }
}