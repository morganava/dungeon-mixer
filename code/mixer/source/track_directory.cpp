#include "utilities.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "serialization.hpp"

namespace dm
{
    /// Track Directory

    const track_entry& track_directory::get_entry(track_handle_t handle)
    {
        auto it = _entries.find(handle);
        MX_ASSERT(it != _entries.end());

        return it->second;
    }

    track_handle_t track_directory::add_entry(track_entry&& entry)
    {
        const auto handle = next_track_handle();
        MX_ASSERT(_entries.find(handle) == _entries.end());
        _entries.insert({handle, std::move(entry)});

        g_serialization_manager->request_autosave(content_type::playlist);
        return handle;
    }

    void track_directory::remove_entry(track_handle_t handle)
    {
        if (auto it =_entries.find(handle); it != _entries.end())
        {
            _entries.erase(it);
            g_serialization_manager->request_autosave(content_type::playlist);
        }
    }

    void track_directory::set_volume(track_handle_t track, double volume)
    {
        auto it = _entries.find(track);
        if (it == _entries.end()) return;

        auto& trackEntry = it->second;
        if (trackEntry.volume == volume) return;

        trackEntry.volume = volume;
        g_serialization_manager->request_autosave(content_type::playlist);
    }

    void track_directory::set_muted(track_handle_t track, bool muted)
    {
        auto it = _entries.find(track);
        if (it == _entries.end()) return;

        auto& trackEntry = it->second;
        if (trackEntry.muted == muted) return;

        trackEntry.muted = muted;
        g_serialization_manager->request_autosave(content_type::playlist);
    }

    void track_directory::set_repeat_enabled(track_handle_t track, bool enabled)
    {
        auto it = _entries.find(track);
        if (it == _entries.end()) return;

        auto& trackEntry = it->second;
        if (trackEntry.repeat.enabled == enabled) return;

        trackEntry.repeat.enabled = enabled;
        g_serialization_manager->request_autosave(content_type::playlist);
    }

    void track_directory::set_repeat_min(track_handle_t track, double min)
    {
        auto it = _entries.find(track);
        if (it == _entries.end()) return;

        auto& trackEntry = it->second;
        if (trackEntry.repeat.min == min) return;

        trackEntry.repeat.min = min;
        g_serialization_manager->request_autosave(content_type::playlist);
    }

    void track_directory::set_repeat_max(track_handle_t track, double max)
    {
        auto it = _entries.find(track);
        if (it == _entries.end()) return;

        auto& trackEntry = it->second;
        if (trackEntry.repeat.max == max) return;

        trackEntry.repeat.max = max;
        g_serialization_manager->request_autosave(content_type::playlist);
    }

    void track_directory::set_position_enabled(track_handle_t track, bool enabled)
    {
        auto it = _entries.find(track);
        if (it == _entries.end()) return;

        auto& trackEntry = it->second;
        if (trackEntry.position.enabled == enabled) return;

        trackEntry.position.enabled = enabled;
        g_serialization_manager->request_autosave(content_type::playlist);
    }

    void track_directory::set_position_elevation(track_handle_t track, double elevation)
    {
        auto it = _entries.find(track);
        if (it == _entries.end()) return;

        auto& trackEntry = it->second;
        if (trackEntry.position.elevation == elevation) return;

        trackEntry.position.elevation = elevation;
        g_serialization_manager->request_autosave(content_type::playlist);
    }

    void track_directory::set_position_azimuth(track_handle_t track, double azimuth)
    {
        auto it = _entries.find(track);
        if (it == _entries.end()) return;

        auto& trackEntry = it->second;
        if (trackEntry.position.azimuth == azimuth) return;

        trackEntry.position.azimuth = azimuth;
        g_serialization_manager->request_autosave(content_type::playlist);
    }
}