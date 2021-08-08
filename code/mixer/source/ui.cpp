#include "utf8_string.hpp"
#include "utilities.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "singleton.hpp"
#include "hash.hpp"
#include "nanovg.hpp"
#include "imgui.hpp"
#include "ui.hpp"
#include "linear_constraint_solver.hpp"
#include "array.hpp"
#include "audio.hpp"
#include "random.hpp"
#include "string_manager.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace dm
{
    void ui::update(float dt, int32_t width, int32_t height)
    {
        mxRoot(rect(vec2::zero, vec2(width, height)))
        {
            mxHBox(self.tags={"root"})
            {
                draw_sidebar(dt);
                switch(root.currentScreen)
                {
                    case screen::playlist:
                        draw_playlist(dt); break;
                    case screen::edit:
                        draw_edit(dt); break;
                    case screen::about:
                        draw_about(dt); break;
                    default:
                        break;
                }
            }
        }
    }

    void ui::sync_playlists()
    {
        worker_audio.enqueue([]()
        {
            std::vector<playlist_widget> playlists;
            std::map<atmosphere_handle_t, atmosphere_widget> atmospheres;

            for (auto playlistHandle : g_playlist_directory->get_playlist_handles())
            {
                const auto& playlistEntry = g_playlist_directory->get_entry(playlistHandle);
                playlist_widget currentPlaylistWidget;

                currentPlaylistWidget.handle = playlistHandle;
                currentPlaylistWidget.title = playlistEntry.title;
                for (auto atmosphereHandle : playlistEntry.atmospheres)
                {
                    // create widget if we haven't already for this atmosphere
                    if (atmospheres.find(atmosphereHandle) == atmospheres.end()) {
                        const auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereHandle);

                        atmosphere_widget currentAtmosphereWidget =
                        {
                            atmosphereEntry.favorite,
                            atmosphereEntry.title,
                            {0.0, 1.0, atmosphereEntry.muted ? 0.0 : atmosphereEntry.volume},
                            atmosphereEntry.volume,
                            atmosphereEntry.muted,
                            false,
                            atmosphereHandle
                        };

                        atmospheres.insert({atmosphereHandle, std::move(currentAtmosphereWidget)});
                    }
                    currentPlaylistWidget.entries.push_back(atmosphereHandle);
                }
                playlists.push_back(std::move(currentPlaylistWidget));
            }

            worker_ui.enqueue([playlists=std::move(playlists), atmospheres=std::move(atmospheres)]() mutable-> void
            {
                g_ui->root.playlist.playlists = std::move(playlists);
                g_ui->root.playlist.atmospheres = std::move(atmospheres);
            });
        });
    }

    // Mutators

    void ui::set_screen(screen_t screen)
    {
        const auto prevScreen = root.currentScreen;

        switch(prevScreen)
        {
            case screen::edit:
                stop_edit_playback();
                break;
            case screen::playlist:
                hide_atmosphere_context_menu();
                break;
            default:
                break;
        }

        root.currentScreen = screen;
    }

    playlist_widget& ui::get_playlist_widget(playlist_handle_t handle)
    {
        for(auto& playlistWidget : root.playlist.playlists) {
            if (playlistWidget.handle == handle) {
                return playlistWidget;
            }
        }

        SMOL_FAIL_FAST("No playlist_widget exists for handle {}", handle);

        return *static_cast<playlist_widget*>(nullptr);
    }

    playlist_widget& ui::get_current_playlist_widget()
    {
        return get_playlist_widget(root.playlist.currentPlaylistHandle);
    }


    atmosphere_widget& ui::get_atmosphere_widget(atmosphere_handle_t handle)
    {
        auto it = root.playlist.atmospheres.find(handle);
        SMOL_ASSERT(it != root.playlist.atmospheres.end());
        return it->second;
    }

    void ui::remove_atmosphere_from_playlist(playlist_handle_t playlist, atmosphere_handle_t atmosphere)
    {
        auto& widget = get_playlist_widget(playlist);
        auto& entries = widget.entries;

        entries.erase(std::find(entries.begin(), entries.end(), atmosphere));
    }

    void ui::add_atmosphere_to_playlist(playlist_handle_t playlist, atmosphere_handle_t atmosphere)
    {
        auto& widget = get_playlist_widget(playlist);
        auto& entries = widget.entries;

        if(std::find(entries.begin(), entries.end(), atmosphere) == entries.end())
        {
            entries.push_back(atmosphere);
        }
    }

    void ui::hide_atmosphere_context_menu()
    {
        g_ui->root.playlist.atmosphereContextMenu.visible = false;
        g_ui->root.playlist.atmosphereContextMenu.copyVisible = false;
        g_ui->root.playlist.atmosphereContextMenu.moveVisible = false;
        g_ui->root.playlist.atmosphereContextMenu.atmosphereHandle = atmosphere_handle::invalid;
    }

    void ui::stop_edit_playback()
    {
        if (g_ui->root.edit.handle == atmosphere_handle::invalid)
        {
            return;
        }

        // stop the edit playback and restore atmosphere's volume and mute settings
        worker_ui.enqueue([]() -> void
        {
            const auto atmosphereHandle = g_ui->root.edit.handle;
            auto it = g_ui->root.playlist.atmospheres.find(atmosphereHandle);
            SMOL_ASSERT(it != g_ui->root.playlist.atmospheres.end());

            auto& widget = it->second;
            const bool muted = widget.muteUnmute.checked;
            const double volume = widget.volumeVal;

            worker_audio.enqueue([=]() -> void
            {
                g_playback_manager->stop_atmosphere(atmosphereHandle);
                g_atmosphere_directory->set_volume(atmosphereHandle, volume);
                g_atmosphere_directory->set_muted(atmosphereHandle, muted);
                g_playback_manager->sync_atmosphere(atmosphereHandle);
            });
        });
    }

    // Sidebar Methods

    void ui::on_create_new_playlist()
    {
        worker_audio.enqueue([]() -> void
        {
            auto playlistHandle = g_playlist_directory->add_entry(
            {
                g_strings[mixer_string::new_playlist_name],
                {}
            });

            MX_LOG_INFO("Create new playlist : {}", playlistHandle);

            worker_ui.enqueue([=]() -> void
            {
                g_ui->root.playlist.playlists.push_back({playlistHandle, g_strings[mixer_string::new_playlist_name]});
            });
        });
    }

    void ui::on_set_screen_playlist(playlist_handle_t handle)
    {
        worker_ui.enqueue([=]() -> void
        {
            g_ui->set_screen(screen::playlist);
            g_ui->root.playlist.currentPlaylistHandle = handle;

            // applies the filter for the playlist
            // a bit of delay since it has to enqueue work on the audio thread
            auto& playlistEntry = g_ui->get_playlist_widget(handle);
            g_ui->on_do_atmosphere_search(handle, playlistEntry.searchBox.text);
        });
    }

    void ui::on_set_screen_edit(atmosphere_handle_t handle)
    {
        const auto prevAtmosphereHandle = g_ui->root.edit.handle;
        // populate the track edit widgets and start playing our edit widget
        worker_audio.enqueue([=]() -> void
        {
            auto atmosphereHandle = handle;

            // just use the previous widgets if we haen't changed the handle
            if(prevAtmosphereHandle != atmosphereHandle || atmosphereHandle == atmosphere_handle::invalid)
            {
                utf8_string atmosphereTitle;
                std::vector<track_edit_widget> editWidgets;

                // create a new atmosphere and begin editing
                if (atmosphereHandle == atmosphere_handle::invalid)
                {
                    atmosphereTitle = g_strings[mixer_string::new_atmosphere_name];
                    atmosphere_entry atmosphereEntry = { atmosphereTitle };
                    atmosphereHandle = g_atmosphere_directory->add_entry(std::move(atmosphereEntry));
                    g_playlist_directory->add_atmosphere(playlist_handle::all, atmosphereHandle);

                    // add this new atmosphere to the All playlist
                    worker_ui.enqueue([=,title=atmosphereTitle]() -> void
                    {
                        atmosphere_widget atmosphereWidget;
                        atmosphereWidget.title = std::move(title);
                        atmosphereWidget.volume.value = 1.0;
                        atmosphereWidget.volumeVal = 1.0;

                        g_ui->root.playlist.atmospheres.insert({atmosphereHandle, std::move(atmosphereWidget)});
                        g_ui->add_atmosphere_to_playlist(playlist_handle::all, atmosphereHandle);
                    });
                }
                // get existing atmosphere data
                else
                {
                    const auto& atmosphere = g_atmosphere_directory->get_entry(atmosphereHandle);
                    atmosphereTitle = atmosphere.title;
                    for(auto trackHandle : atmosphere.tracks)
                    {
                        auto& track = g_track_directory->get_entry(trackHandle);
                        auto& audio = g_audio_directory->get_entry(track.audio);

                        track_edit_widget widget = {
                            trackHandle,
                            audio.type(),
                            audio.title,
                            track.volume,
                            track.position.enabled,
                            track.position.azimuth,
                            track.position.elevation,
                            track.repeat.enabled,
                            track.repeat.min,
                            track.repeat.max
                        };

                        editWidgets.push_back(std::move(widget));
                    }
                }

                // populate edit ui
                worker_ui.enqueue([=, atmosphereTitle=std::move(atmosphereTitle), editWidgets=std::move(editWidgets)]() -> void
                {
                    // populate edit widgets
                    g_ui->root.edit.handle = atmosphereHandle;
                    g_ui->root.edit.title = std::move(atmosphereTitle);
                    g_ui->root.edit.tracks.widgets = std::move(editWidgets);
                    g_ui->set_screen(screen::edit);
                });
            }
            else
            {
                worker_ui.enqueue([=]() -> void
                {
                    g_ui->set_screen(screen::edit);
                });
            }

            MX_LOG_INFO("playing : {}", atmosphereHandle);

            // stop all currently playing tracks
            g_playback_manager->stop_all_tracks();
            auto& playlistEntry = g_playlist_directory->get_entry(playlist_handle::playing);

            // empty out the playing playlist and update the play button
            worker_ui.enqueue([atmospheres=playlistEntry.atmospheres]() -> void
            {
                for(auto& atmosphereHandle : atmospheres)
                {
                    auto it = g_ui->root.playlist.atmospheres.find(atmosphereHandle);
                    SMOL_ASSERT(it != g_ui->root.playlist.atmospheres.end());
                    it->second.playStop = false;
                }
                auto& playlistWidget = g_ui->get_playlist_widget(playlist_handle::playing);
                playlistWidget.entries.clear();
            });
            g_playlist_directory->clear_playlist(playlist_handle::playing);

            // begin playing the editing atmosphere
            g_atmosphere_directory->set_volume(atmosphereHandle, 1.0f);
            g_atmosphere_directory->set_muted(atmosphereHandle, false);
            g_playback_manager->play_atmosphere(atmosphereHandle);
        });

        worker_ui.enqueue([]() -> void
        {
            g_ui->on_do_audio_search(g_ui->root.edit.searchBox.text);
        });
    }

    void ui::on_set_screen_about()
    {
        worker_ui.enqueue([=]() -> void
        {
            g_ui->set_screen(screen::about);
        });
    }

    /// Draw Methods

    void ui::draw_sidebar(float dt)
    {
        mxVScroll(&root.sidebar.scroll, self.tags = {"sidebar"})
        {
            mxSpacer(self.requestHeight(100.0));
            mxLabel(g_strings[mixer_string::playlists], self.tags = {"mediumText", "sidebarEntry"});
            mxButton(g_strings[mixer_string::create_new_playlist], self.tags = {"lightText", "sidebarEntry", "indent"};
            self.on_click = []() -> void
            {
                g_ui->on_create_new_playlist();
            });
            for( size_t i = 0; i < root.playlist.playlists.size(); ++i)
            {
                auto& currentPlaylist = root.playlist.playlists[i];
                imgui::button_data bd(currentPlaylist.title);
                utf8_string const& playlistTitle = [&]()
                {
                    switch(currentPlaylist.handle)
                    {
                        case playlist_handle::all:
                            return g_strings[mixer_string::all_playlist];
                        case playlist_handle::favorites:
                            return g_strings[mixer_string::favorites_playlist];
                        case playlist_handle::playing:
                            return g_strings[mixer_string::playing_playlist];
                        default:
                            return currentPlaylist.title;
                    }
                }();
                mxButton(playlistTitle, self.tags = {"lightText","sidebarEntry", "indent"};
                    self.on_click = [=]() -> void
                    {
                        g_ui->on_set_screen_playlist(currentPlaylist.handle);
                    };
                );
            }
            mxButton(g_strings[mixer_string::edit_screen], self.tags = {"mediumText", "sidebarEntry"};
            self.on_click = []() -> void
            {
                g_ui->on_set_screen_edit(g_ui->root.edit.handle);
            });
            mxButton(g_strings[mixer_string::about_screen], self.tags = {"mediumText", "sidebarEntry"};
            self.on_click = []() -> void
            {
                g_ui->on_set_screen_about();
            });
        }
    }
}
