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

    std::vector<atmosphere_handle_t> atmosphere_directory::get_atmosphere_handles() const
    {
        std::vector<atmosphere_handle_t> result;
        for(auto it : _entries)
        {
            result.push_back(it.first);
        }

        return result;
    }

    std::vector<atmosphere_handle_t> atmosphere_directory::get_atmosphere_handles_by_query(const std::string& query) const
    {
        return _tag_map.get_keys_by_string_query(query);
    }

    atmosphere_handle_t atmosphere_directory::get_atmosphere_handle_by_id(const std::string& id) const
    {
        auto it = _id_map.find(id);
        if(it == _id_map.end())
        {
            return MX_INVALID_HANDLE;
        }
        return it->second;
    }

    atmosphere_entry& atmosphere_directory::get_entry(atmosphere_handle_t handle)
    {
        auto it = _entries.find(handle);
        MX_ASSERT(it != _entries.end());

        return it->second;
    }

    atmosphere_handle_t atmosphere_directory::add_entry(atmosphere_entry&& entry)
    {
        const auto key = _next_handle++;

        std::vector<std::string> tags = split(to_lower(entry.title), " ");
        _tag_map.add_key(key, tags);

        _id_map.insert({entry.id, key});
        _entries.insert({key, std::move(entry)});

        return key;
    }

    void atmosphere_directory::remove_entry(atmosphere_handle_t handle)
    {
        auto it = _entries.find(handle);
        MX_ASSERT(it != _entries.end());

        _id_map.erase(_id_map.find(it->second.id));
        _entries.erase(it);

        _tag_map.remove_key(handle);
    }

    void atmosphere_directory::rename_entry(atmosphere_handle_t handle, const std::string& newName)
    {
        auto it = _entries.find(handle);
        MX_ASSERT(it != _entries.end());

        // early out if the same name
        if(it->second.title == newName) return;
        it->second.title = newName;

        _tag_map.remove_key(handle);
        std::vector<std::string> tags = split(to_lower(newName), " ");
        _tag_map.add_key(handle, tags);
    }
}