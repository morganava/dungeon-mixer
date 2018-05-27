#include "string_manager.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace tabletopify
{
    static string_manager* g_string_manager = nullptr;

    string_manager* get_string_manager()
    {
        return g_string_manager;
    }

    void init_string_manager(fs::path&& stringsPath)
    {
        MX_ASSERT(g_string_manager == nullptr);
        g_string_manager = new string_manager();

        worker(Worker_FileIO).enqueue([stringsPath=std::move(stringsPath)]() -> void
        {
            MX_ASSERT(fs::exists(stringsPath));
            auto stringsSize = fs::file_size(stringsPath);

            std::ifstream stringsFile(stringsPath.native(), std::ios_base::in | std::ios_base::binary);
            std::string stringsData(stringsSize, 0);
            stringsFile.read(stringsData.data(), stringsSize);
            stringsFile.close();

            worker(Worker_Background).enqueue([stringsData=std::move(stringsData), stringsPath=std::move(stringsPath)]() -> void
            {
                auto json = nl::json::parse(stringsData);
                auto& stringList = json["Strings"];
                MX_ASSERT(stringList.size() == MX_STR_COUNT);
                for(size_t idx = 0; idx < stringList.size(); ++idx)
                {
                    g_string_manager->_strings[idx] = stringList[idx];
                }
                g_string_manager->_strings_loaded = true;
                MX_LOG_INFO(ss() << "Loaded strings: " << stringsPath);
            });
        });
    }

    void free_string_manager()
    {
        delete g_string_manager;
        g_string_manager = nullptr;
    }

    const char* string_manager::get_cstring(mixer_string_t str) const
    {
        MX_ASSERT(str > MX_STR_INVALID && str < MX_STR_COUNT);
        if(this->_strings_loaded)
        {
            return this->_strings[str].c_str();
        }
        return "";
    }
}