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

namespace tabletopify
{
    void ui::update(float dt, int32_t width, int32_t height)
    {
        mxRoot(rect(vec2::zero, vec2(width, height)))
        {
            mxHBox(&root.flex)
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
        worker_background.enqueue([]()
        {
            std::vector<playlist_widget> playlists;
            std::map<atmosphere_handle_t, atmosphere_widget> atmospheres;

            for (auto playlistHandle : g_playlist_directory.get_playlist_handles())
            {
                auto& playlistEntry = g_playlist_directory.get_entry(playlistHandle);
                playlist_widget currentPlaylistWidget;

                currentPlaylistWidget.handle = playlistHandle;
                currentPlaylistWidget.title = playlistEntry.title;
                for (auto atmosphereHandle : playlistEntry.atmospheres)
                {
                    // create widget if we haven't already for this atmosphere
                    if (atmospheres.find(atmosphereHandle) == atmospheres.end()) {
                        const auto& atmosphereEntry = g_atmosphere_directory.get_entry(atmosphereHandle);
                        utf8_string atmosphereEntry_title = atmosphereEntry.title;

                        atmosphere_widget currentAtmosphereWidget;
                        currentAtmosphereWidget.favorite =  {atmosphereEntry.favorite};
                        currentAtmosphereWidget.title = {atmosphereEntry.title};
                        currentAtmosphereWidget.volume = {0.0, 1.0, atmosphereEntry.muted ? 0.0 : atmosphereEntry.volume};
                        currentAtmosphereWidget.volumeVal = atmosphereEntry.volume;
                        currentAtmosphereWidget.muteUnmute = {atmosphereEntry.muted};
                        currentAtmosphereWidget.atmosphereHandle = atmosphereHandle;

                        atmospheres.insert({atmosphereHandle, std::move(currentAtmosphereWidget)});
                    }
                    currentPlaylistWidget.entries.push_back(atmosphereHandle);
                }
                playlists.push_back(std::move(currentPlaylistWidget));
            }

            worker_ui.enqueue([playlists=std::move(playlists), atmospheres=std::move(atmospheres)]() mutable-> void
            {
                g_ui.root.playlist.playlists = std::move(playlists);
                g_ui.root.playlist.atmospheres = std::move(atmospheres);
            });
        });
    }

    // Mutators

    void ui::set_screen(screen_t screen)
    {
        root.currentScreen = screen;

        hide_atmosphere_context_menu();
        if (screen != screen::edit)
        {
            stop_edit_playback();
        }
    }

    playlist_widget& ui::get_playlist_widget(playlist_handle_t handle)
    {
        for(auto& playlistWidget : root.playlist.playlists) {
            if (playlistWidget.handle == handle) {
                return playlistWidget;
            }
        }

        MX_FAIL_FAST("No playlist_widget exists for handle {}", handle);

        return *static_cast<playlist_widget*>(nullptr);
    }

    playlist_widget& ui::get_current_playlist_widget()
    {
        return get_playlist_widget(root.playlist.currentPlaylistHandle);
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
        g_ui.root.playlist.atmosphereContextMenu.visible = false;
        g_ui.root.playlist.atmosphereContextMenu.copyVisible = false;
        g_ui.root.playlist.atmosphereContextMenu.moveVisible = false;
        g_ui.root.playlist.atmosphereContextMenu.atmosphereHandle = atmosphere_handle::invalid;
    }

    void ui::stop_edit_playback()
    {
        if (g_ui.root.edit.handle == atmosphere_handle::invalid)
        {
            return;
        }

        // stop the edit playback and restore atmosphere's volume and mute settings
        worker_ui.enqueue([]() -> void
        {
            const auto atmosphereHandle = g_ui.root.edit.handle;
            auto it = g_ui.root.playlist.atmospheres.find(atmosphereHandle);
            MX_ASSERT(it != g_ui.root.playlist.atmospheres.end());

            auto& widget = it->second;
            const bool muted = widget.muteUnmute.checked;
            const double volume = widget.volumeVal;

            worker_background.enqueue([=]() -> void
            {
                g_playback_manager.stop_atmosphere(atmosphereHandle);

                auto& entry = g_atmosphere_directory.get_entry(atmosphereHandle);
                entry.muted = muted;
                entry.volume = static_cast<float>(volume);
            });
        });
    }

    // Sidebar Methods

    void ui::on_create_new_playlist()
    {
        worker_background.enqueue([]() -> void
        {
            auto playlistHandle = g_playlist_directory.create_playlist();
            MX_LOG_INFO("Create new playlist : {}", playlistHandle);

            worker_ui.enqueue([=]() -> void
            {
                g_ui.root.playlist.playlists.push_back({playlistHandle, "New Playlist"});
            });
        });
    }

    void ui::on_set_screen_playlist(playlist_handle_t handle)
    {
        worker_ui.enqueue([=]() -> void
        {
            g_ui.set_screen(screen::playlist);
            g_ui.root.playlist.currentPlaylistHandle = handle;
        });
    }

    void ui::on_set_screen_edit(atmosphere_handle_t handle)
    {
        // populate the track edit widgets
        worker_background.enqueue([=]() -> void
        {
            auto atmosphereHandle = handle;
            utf8_string atmosphereTitle;
            std::vector<track_edit_widget> editWidgets;

            // create a new atmosphere and begin editing
            if (atmosphereHandle == atmosphere_handle::invalid)
            {
                atmosphereTitle = "New Atmosphere";
                atmosphere_entry atmosphereEntry =
                {
                    g_random.next_uuid(),
                    atmosphereTitle
                };
                atmosphereHandle = g_atmosphere_directory.add_entry(std::move(atmosphereEntry));

                // add this new atmosphere to the All playlist
                worker_ui.enqueue([=,title=atmosphereTitle]() -> void
                {
                    atmosphere_widget atmosphereWidget;
                    atmosphereWidget.title.text = std::move(title);
                    atmosphereWidget.volume.value = 1.0;
                    atmosphereWidget.volumeVal = 1.0;

                    g_ui.root.playlist.atmospheres.insert({atmosphereHandle, std::move(atmosphereWidget)});
                    g_ui.add_atmosphere_to_playlist(playlist_handle::all, atmosphereHandle);
                });
            }
            // get existing atmosphere data
            else
            {
                auto& atmosphere = g_atmosphere_directory.get_entry(atmosphereHandle);
                atmosphereTitle = atmosphere.title;
                for(auto trackHandle : atmosphere.tracks)
                {
                    auto& track = g_track_directory.get_entry(trackHandle);

                    track_edit_widget widget;
                    widget.visible.checked = false;
                    widget.handle = trackHandle;
                    widget.audio_title = track.audioTitle;
                    widget.volume_slider.value = track.volume;

                    editWidgets.push_back(std::move(widget));
                }
            }

            // populate ui
            worker_ui.enqueue([=, atmosphereTitle=std::move(atmosphereTitle), editWidgets=std::move(editWidgets)]() -> void
            {
                // set all the atmosphere widgets playing to false
                for (auto& [atmosphereHandle, widget] : g_ui.root.playlist.atmospheres)
                {
                    widget.playStop.checked = false;
                }

                // populate edit widgets
                g_ui.root.edit.handle = atmosphereHandle;
                g_ui.root.edit.tracks.title = std::move(atmosphereTitle);
                g_ui.root.edit.tracks.widgets = std::move(editWidgets);
                g_ui.set_screen(screen::edit);
            });

            MX_LOG_INFO("playing : {}", atmosphereHandle);

            // stop all currently playing tracks
            g_playback_manager.stop_all_tracks();

            // begin playing the editing atmosphere
            auto& atmosphereEntry = g_atmosphere_directory.get_entry(atmosphereHandle);
            atmosphereEntry.volume = 1.0f;
            atmosphereEntry.muted = false;
            g_playback_manager.play_atmosphere(atmosphereHandle);
        });
    }

    void ui::on_set_screen_about()
    {
        worker_ui.enqueue([=]() -> void
        {
            g_ui.set_screen(screen::about);
        });
    }

    /// Draw Methods

    void ui::draw_sidebar(float dt)
    {
        mxVScroll(&root.sidebar.scroll, self.tags = {"sidebar"})
        {
            mxSpacer(self.requestHeight(100.0));
            mxButton(&root.sidebar.playlists, self.tags = {"mediumText", "sidebarEntry"});
            mxButton(&root.sidebar.newPlaylist, self.tags = {"lightText", "sidebarEntry", "indent"};
            self.on_click = []() -> void
            {
                g_ui.on_create_new_playlist();
            });
            for( size_t i = 0; i < root.playlist.playlists.size(); ++i)
            {
                auto& currentPlaylist = root.playlist.playlists[i];
                imgui::button_data bd(currentPlaylist.title);
                mxButton(currentPlaylist.title, self.tags = {"lightText","sidebarEntry", "indent"};
                    self.on_click = [=]() -> void
                    {
                        g_ui.on_set_screen_playlist(currentPlaylist.handle);
                    };
                );
            }
            mxButton(&root.sidebar.edit, self.tags = {"mediumText", "sidebarEntry"};
            self.on_click = []() -> void
            {
                g_ui.on_set_screen_edit(g_ui.root.edit.handle);
            });
            mxButton(&root.sidebar.about, self.tags = {"mediumText", "sidebarEntry"};
            self.on_click = []() -> void
            {
                g_ui.on_set_screen_about();
            });
        }
    }
}
