 #include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "tag_map.hpp"
#include "audio.hpp"


namespace tabletopify
{
    single_threaded<atmosphere_directory> g_atmosphere_directory;

    /// Atmosphere Directory

    std::vector<atmosphere_id_t> atmosphere_directory::get_atmosphere_ids() const
    {
        std::vector<atmosphere_id_t> result;
        for(auto it : _entries)
        {
            result.push_back(it.first);
        }

        return result;
    }

    std::vector<atmosphere_id_t> atmosphere_directory::get_atmosphere_ids_by_query(const std::string& query_string) const
    {
        return _tag_map.get_keys_by_string_query(query_string);
    }

    atmosphere_entry& atmosphere_directory::get_entry(atmosphere_id_t id)
    {
        auto it = _entries.find(id);
        MX_ASSERT(it != _entries.end());

        return it->second;
    }

    atmosphere_id_t atmosphere_directory::add_entry(atmosphere_entry&& entry)
    {
        const auto key = _next_id++;

        std::vector<std::string> tags = split(to_lower(entry.title), " ");
        _tag_map.add_key(key, tags);

        _entries.insert({key, std::move(entry)});

        return key;
    }

    void atmosphere_directory::remove_entry(atmosphere_id_t id)
    {
        auto it = _entries.find(id);
        MX_ASSERT(it != _entries.end());
        _entries.erase(it);

        _tag_map.remove_key(  id);
    }
}