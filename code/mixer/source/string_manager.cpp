#include "string_manager.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace tabletopify
{
    string_manager::string_manager(fs::path&& stringsPath)
    {
        worker(worker_id::fileio).enqueue([stringsPath=std::move(stringsPath)]() -> void
        {
            MX_ASSERT(fs::exists(stringsPath));
            auto stringsSize = fs::file_size(stringsPath);

            // TODO: switch to use memory_buffer
            std::ifstream stringsFile(stringsPath.native(), std::ios_base::in | std::ios_base::binary);
            std::string stringsData(stringsSize, 0);
            stringsFile.read(stringsData.data(), stringsSize);
            stringsFile.close();

            worker(worker_id::background).enqueue([stringsData=std::move(stringsData), stringsPath=std::move(stringsPath)]() -> void
            {
                auto json = nl::json::parse(stringsData);
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