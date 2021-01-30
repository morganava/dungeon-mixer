// dm
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "string_manager.hpp"
#include "serialization.hpp"

namespace dm
{
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
        return _tag_map.get_keys_by_tag_query(query);
    }

    atmosphere_handle_t atmosphere_directory::get_atmosphere_handle_by_track(track_handle_t track) const
    {
        auto it = _track_map.find(track);
        if (it == _track_map.end())
        {
            return atmosphere_handle::invalid;
        }
        return it->second;
    }

    const atmosphere_entry& atmosphere_directory::get_entry(atmosphere_handle_t handle)
    {
        auto it = _entries.find(handle);
        MX_ASSERT(it != _entries.end());

        return it->second;
    }

    atmosphere_handle_t atmosphere_directory::add_entry(atmosphere_entry&& entry)
    {
        const auto atmosphereHandle = next_atmosphere_handle();

        // tag map
        std::vector<utf8_string> tags = entry.title.split();
        _tag_map.add_key(atmosphereHandle, tags);

        // track map
        for(auto& trackHandle : entry.tracks)
        {
            _track_map.insert({trackHandle, atmosphereHandle});
        }

        // entry map
        _entries.insert({atmosphereHandle, std::move(entry)});

        return atmosphereHandle;
    }

    void atmosphere_directory::remove_entry(atmosphere_handle_t atmosphere)
    {
        auto it = _entries.find(atmosphere);
        if (it == _entries.end()) return;

        auto& entry = it->second;

        // tag map
        _tag_map.remove_key(atmosphere);

        // track map
        for(auto& trackHandle : entry.tracks)
        {
            _track_map.erase(trackHandle);
        }

        // entry map
        _entries.erase(it);
    }

    void atmosphere_directory::set_title(atmosphere_handle_t atmosphere, utf8_string&& newName)
    {
        auto it = _entries.find(atmosphere);
        if (it == _entries.end()) return;

        auto& entry = it->second;

        // early out if the same name
        if(entry.title == newName) return;

        // update the name and tags
        auto tags = newName.split();
        entry.title = std::move(newName);

        // update tag map
        _tag_map.remove_key(atmosphere);
        _tag_map.add_key(atmosphere, tags);

        g_serialization_manager.request_autosave(content_type::playlist);
    }

    void atmosphere_directory::set_volume(atmosphere_handle_t atmosphere, float volume)
    {
        auto it = _entries.find(atmosphere);
        if (it == _entries.end()) return;

        auto& entry = it->second;

        // early out if same
        if (entry.volume == volume) return;

        entry.volume = volume;
        g_serialization_manager.request_autosave(content_type::playlist);
    }

    void atmosphere_directory::set_muted(atmosphere_handle_t atmosphere, bool muted)
    {
        auto it = _entries.find(atmosphere);
        if (it == _entries.end()) return;

        auto& entry = it->second;

        // early out if same
        if (entry.muted == muted) return;

        entry.muted = muted;
        g_serialization_manager.request_autosave(content_type::playlist);
    }

    void atmosphere_directory::set_favorite(atmosphere_handle_t atmosphere, bool favorite)
    {
        auto it = _entries.find(atmosphere);
        if (it == _entries.end()) return;

        auto& entry = it->second;

        // early out if same
        if (entry.favorite == favorite) return;

        entry.favorite = favorite;
        g_serialization_manager.request_autosave(content_type::playlist);
    }

    void atmosphere_directory::add_track(atmosphere_handle_t atmosphere, track_handle_t track)
    {
        auto it = _entries.find(atmosphere);
        if (it == _entries.end()) return;

        auto& entry = it->second;

        // verify track isn't already in atmosphere
        MX_ASSERT(std::find(entry.tracks.begin(), entry.tracks.end(), track) == entry.tracks.end());

        // track map
        _track_map.insert({track, atmosphere});

        // entry
        entry.tracks.push_back(track);

        g_serialization_manager.request_autosave(content_type::playlist);
    }

    void atmosphere_directory::remove_track(atmosphere_handle_t atmosphere, track_handle_t track)
    {
        auto it = _entries.find(atmosphere);
        if (it == _entries.end()) return;

        auto& entry = it->second;

        // track map
        _track_map.erase(track);

        // entry
        if(auto it = std::find(entry.tracks.begin(), entry.tracks.end(), track); it != entry.tracks.end())
        {
            entry.tracks.erase(it);
            g_serialization_manager.request_autosave(content_type::playlist);
        }
    }
}