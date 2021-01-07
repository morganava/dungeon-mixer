#include "string_manager.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "serialization.hpp"

namespace dm
{
    string_manager::string_manager(fs::path&& stringsPath)
    {
        worker_fileio.enqueue([stringsPath=std::move(stringsPath)]() -> void
        {
            memory_buffer stringsData(stringsPath);

            worker_background.enqueue([stringsData=std::move(stringsData), stringsPath=std::move(stringsPath)]() -> void
            {
                auto json = parse_json(stringsData);
                auto& stringList = json["Strings"];
                MX_ASSERT(stringList.size() == static_cast<size_t>(mixer_string::count));
                for(size_t idx = 0; idx < stringList.size(); ++idx)
                {
                    g_string_manager._strings[idx] = stringList[idx];
                }
                g_string_manager._strings_loaded = true;
                MX_LOG_INFO("Loaded strings: {}", stringsPath.filename().native());
            });
        });
    }

    const char* string_manager::get_cstring(mixer_string_t str) const
    {
        MX_ASSERT(str != mixer_string::invalid && str < mixer_string::count);
        if(this->_strings_loaded)
        {
            return this->_strings[static_cast<size_t>(str)].c_str();
        }
        return "";
    }
}