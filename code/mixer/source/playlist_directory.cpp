#include "audio.hpp"
#include "ui.hpp"
#include "serialization.hpp"
#include "paths.hpp"
#include "work_queue.hpp"

namespace dm
{
    playlist_directory::playlist_directory()
    {
        worker_fileio.enqueue([]() -> void
        {
            g_serialization_manager->load_playlist(paths::playlist_file());
        });
    }

    std::vector<playlist_handle_t> playlist_directory::get_playlist_handles() const
    {
        std::vector<playlist_handle_t> retval;
		retval.reserve(_entries.size());
        for (auto& [handle, entry] : _entries)
        {
            retval.push_back(handle);
        }
        return retval;
    }

    std::vector<playlist_handle_t> playlist_directory::get_user_playlist_handles() const
    {
        MX_ASSERT(_entries.size() > playlist_handle::__first_generated);

        std::vector<playlist_handle_t> retval;
        // exclude the builtin playlists
        retval.reserve(_entries.size() - static_cast<size_t>(playlist_handle::__first_generated));
        for(auto& [handle, entry] : _entries)
        {
            if (handle >= playlist_handle::__first_generated) {
                retval.push_back(handle);
            }
        }
        return retval;
    }

    void playlist_directory::remove_entry(playlist_handle_t playlist)
    {
        MX_ASSERT(playlist >= playlist_handle::__first_generated);

        _entries.erase(playlist);

        g_serialization_manager->request_autosave(content_type::playlist);
    }

    playlist_handle_t playlist_directory::add_entry(playlist_entry&& entry)
    {
        const auto handle = next_playlist_handle();
        MX_ASSERT(_entries.find(handle) == _entries.end());
        _entries.insert({handle, std::move(entry)});

        g_serialization_manager->request_autosave(content_type::playlist);
        return handle;
    }

    const playlist_entry& playlist_directory::get_entry(playlist_handle_t playlist)
    {
        auto it = _entries.find(playlist);
        // make sure the provided playlist exists
        MX_ASSERT(it != _entries.end());
        auto& playlistEntry = it->second;
        return playlistEntry;
    }

    void playlist_directory::set_title(playlist_handle_t playlist, utf8_string&& name)
    {
        MX_ASSERT(!is_builtin_playlist(playlist));
        auto it = _entries.find(playlist);
        if (it == _entries.end()) return;

        auto& entry = it->second;
        entry.title = std::move(name);

        g_serialization_manager->request_autosave(content_type::playlist);
    }

    void playlist_directory::add_atmosphere(playlist_handle_t playlist, atmosphere_handle_t atmosphere)
    {
        if (auto it = _entries.find(playlist); it != _entries.end())
        {
            auto& playlistEntry = it->second;
            // add if it isn't already in the list
            if(std::find(playlistEntry.atmospheres.begin(), playlistEntry.atmospheres.end(), atmosphere) ==
               playlistEntry.atmospheres.end())
            {
                playlistEntry.atmospheres.push_back(atmosphere);
                if (!is_builtin_playlist(playlist))
                {
                    g_serialization_manager->request_autosave(content_type::playlist);
                }
            }
        }
    }

    void playlist_directory::remove_atmosphere(playlist_handle_t playlist, atmosphere_handle_t atmosphere)
    {
        auto it = _entries.find(playlist);
        if(it == _entries.end()) return;

        auto& playlistEntry = it->second;

        // remove atmosphere
        if(auto it = std::find(playlistEntry.atmospheres.begin(), playlistEntry.atmospheres.end(), atmosphere); it != playlistEntry.atmospheres.end())
        {
            playlistEntry.atmospheres.erase(it);
            if (!is_builtin_playlist(playlist))
            {
                g_serialization_manager->request_autosave(content_type::playlist);
            }
        }
    }

    void playlist_directory::clear_playlist(playlist_handle_t playlist)
    {
        auto it = _entries.find(playlist);
        MX_ASSERT(it != _entries.end());

        it->second.atmospheres.clear();

        if (!is_builtin_playlist(playlist))
        {
            g_serialization_manager->request_autosave(content_type::playlist);
        }
    }

    std::vector<atmosphere_handle_t> playlist_directory::filtered_atmospheres(playlist_handle_t playlist, const utf8_string& query)
    {
        auto& playlistEntry = this->get_entry(playlist);
        // an empty query returns all of our atmospheres
        if (query.empty())
        {
            return playlistEntry.atmospheres;
        }

        // do our query on ALL atmospheres in the system
        auto handles = g_atmosphere_directory->get_atmosphere_handles_by_query(query);

        // now populate our retval value by filtering handles based off of whether or not they
        // are in this playlist
        std::vector<atmosphere_handle_t> retval;
        std::sort(handles.begin(), handles.end());
        for(auto currentHandle : playlistEntry.atmospheres)
        {
            if (std::binary_search(handles.begin(), handles.end(), currentHandle))
            {
                retval.push_back(currentHandle);
            }
        }

        return retval;
    }
}