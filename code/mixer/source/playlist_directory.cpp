#include "audio.hpp"
#include "ui.hpp"
#include "serialization.hpp"

namespace tabletopify
{
    playlist_directory::playlist_directory() { }

    std::vector<playlist_handle_t> playlist_directory::get_playlist_handles() const
    {
        std::vector<playlist_handle_t> retval;
		retval.reserve(_entries.size());
        for (auto val : _entries)
        {
            retval.push_back(val.first);
        }
        return retval;
    }

    void playlist_directory::remove_entry(playlist_handle_t playlist)
    {
        _entries.erase(playlist);

        g_serialization_manager.request_autosave(content_type::playlist);
    }

    playlist_handle_t playlist_directory::add_entry(playlist_entry&& entry)
    {
        const auto handle = next_playlist_handle();
        MX_ASSERT(_entries.find(handle) == _entries.end());
        _entries.insert({handle, std::move(entry)});

        g_serialization_manager.request_autosave(content_type::playlist);
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
        auto it = _entries.find(playlist);
        if (it == _entries.end()) return;

        auto& entry = it->second;
        entry.title = std::move(name);

        g_serialization_manager.request_autosave(content_type::playlist);
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

                g_serialization_manager.request_autosave(content_type::playlist);
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

            g_serialization_manager.request_autosave(content_type::playlist);
        }
    }
}