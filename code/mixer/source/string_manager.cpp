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
            g_serialization_manager.load_strings(paths::strings_file());
        });
    }
}