// tabletopify
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "string_manager.hpp"

namespace tabletopify
{
    atmosphere_directory* g_atmosphere_directory = nullptr;
    atmosphere_directory* get_atmosphere_directory()
    {
        MX_ASSERT(worker(Worker_Background).getThreadId() == std::this_thread::get_id());
        return g_atmosphere_directory;
    }
    void init_atmosphere_directory(atmosphere_directory&& atmosphereDirectory)
    {
        if(g_atmosphere_directory == nullptr)
        {
            g_atmosphere_directory = new atmosphere_directory(std::forward<atmosphere_directory>(atmosphereDirectory));
        }
        else
        {
            MX_ASSERT(worker(Worker_Background).getThreadId() == std::this_thread::get_id());
            *g_atmosphere_directory = std::forward<atmosphere_directory>(atmosphereDirectory);
        }
    }
    void free_atmosphere_directory()
    {
        delete g_atmosphere_directory;
        g_atmosphere_directory = nullptr;
    }

    atmosphere_handle_t atmosphere_directory::_next_handle = 0;

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

    std::vector<atmosphere_handle_t> atmosphere_directory::get_atmosphere_handles_by_query(const utf8_string& query) const
    {
        auto norm = query.normalized();
        auto query_tags = norm.split();
                
        return _tag_map.get_keys_by_tag_query(query_tags);
    }

    atmosphere_handle_t atmosphere_directory::get_atmosphere_handle_by_id(const uuid& id) const
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

        std::vector<utf8_string> tags = entry.title.split();
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

    void atmosphere_directory::rename_entry(atmosphere_handle_t handle, const utf8_string& newName)
    {
        auto it = _entries.find(handle);
        MX_ASSERT(it != _entries.end());

        // early out if the same name
        if(it->second.title == newName) return;
        it->second.title = newName;

        _tag_map.remove_key(handle);
        std::vector<utf8_string> tags = newName.split();
        _tag_map.add_key(handle, tags);
    }
}