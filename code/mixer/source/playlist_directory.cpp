#include <cxxabi.h>

#include "audio.hpp"
#include "ui.hpp"

namespace tabletopify
{
    playlist_directory::playlist_directory() { }

    playlist_handle_t playlist_directory::create_playlist()
    {
        return add_entry({});
    }

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

    void playlist_directory::delete_playlist(playlist_handle_t playlist)
    {
        _entries.erase(playlist);
    }

    playlist_handle_t playlist_directory::add_entry(playlist_entry&& entry)
    {
        auto handle = _next_handle;
        _next_handle = static_cast<playlist_handle_t>(static_cast<handle_t>(handle) + 1);
        MX_ASSERT(_entries.find(handle) == _entries.end());
        _entries.insert({handle, std::move(entry)});
        return handle;
    }

    playlist_entry& playlist_directory::get_entry(playlist_handle_t playlist)
    {
        auto it = _entries.find(playlist);
        // make sure the provided playlist exists
        MX_ASSERT(it != _entries.end());
        auto& playlistEntry = it->second;
        return playlistEntry;
    }

    void playlist_directory::add_atmosphere(playlist_handle_t playlist, atmosphere_handle_t atmosphere)
    {
        auto& playlistEntry = this->get_entry(playlist);
        // add if it isn't already in the list
        if(std::find(playlistEntry.atmospheres.begin(), playlistEntry.atmospheres.end(), atmosphere) ==
           playlistEntry.atmospheres.end())
        {
            playlistEntry.atmospheres.push_back(atmosphere);
        }
    }

    void playlist_directory::remove_atmosphere(playlist_handle_t playlist, atmosphere_handle_t atmosphere)
    {
        auto& playlistEntry = this->get_entry(playlist);
        // remove atmosphere
        auto it = std::find(playlistEntry.atmospheres.begin(), playlistEntry.atmospheres.end(), atmosphere);
        MX_ASSERT(it != playlistEntry.atmospheres.end());
        playlistEntry.atmospheres.erase(it);
    }

    void playlist_directory::favorite_atmosphere(atmosphere_handle_t handle)
    {
        MX_LOG_INFO("Favoriting : {}", handle);

        // update the atmopshere_entry
        auto& atmosphere = g_atmosphere_directory.get_entry(handle);
        MX_ASSERT(atmosphere.favorite == false);
        atmosphere.favorite = true;

        // add to the favorites playlist
        add_atmosphere(playlist_handle::favorites, handle);

        // tell ui to update
        update_playlist_ui();
    }

    void playlist_directory::unfavorite_atmosphere(atmosphere_handle_t handle)
    {
        MX_LOG_INFO("Unfavoriting : {}", handle);

        // update the atmopshere_entry
        auto& atmosphere = g_atmosphere_directory.get_entry(handle);
        MX_ASSERT(atmosphere.favorite == true);
        atmosphere.favorite = false;

        // remove from the favorites playlist
        remove_atmosphere(playlist_handle::favorites, handle);

        // tell ui to update
        update_playlist_ui();
    }

    void playlist_directory::update_playlist_ui()
    {
        std::vector<playlist_widget> playlists;
        for (auto playlistHandle : g_playlist_directory.get_playlist_handles())
        {
            auto& playlistEntry = g_playlist_directory.get_entry(playlistHandle);
            playlist_widget currentPlaylistWidget;

            currentPlaylistWidget.title.text = playlistEntry.title;
            for (auto atmosphereHandle : playlistEntry.atmospheres)
            {
                const auto& atmosphereEntry = g_atmosphere_directory.get_entry(atmosphereHandle);
                utf8_string atmosphereEntry_title = atmosphereEntry.title;

                playlist_widget::atmosphere_widget currentAtmosphereWidget;
                currentAtmosphereWidget.favorite =  {atmosphereEntry.favorite};
                currentAtmosphereWidget.title = {atmosphereEntry.title};
                currentAtmosphereWidget.volume = {0.0, 1.0, atmosphereEntry.muted ? 0.0 : atmosphereEntry.volume};
                currentAtmosphereWidget.volumeVal = atmosphereEntry.volume;
                currentAtmosphereWidget.muteUnmute = {atmosphereEntry.muted};
                currentAtmosphereWidget.atmosphereHandle = atmosphereHandle;
                currentPlaylistWidget.entries.push_back(std::move(currentAtmosphereWidget));
            }
            playlists.emplace_back(std::move(currentPlaylistWidget));
        }

        worker(Worker_UI) << [playlists=std::move(playlists)]() mutable-> void
        {
            g_ui.populate_playlist_widgets(std::move(playlists));
        };
    }
}