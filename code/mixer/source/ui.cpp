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
        worker_ui.enqueue([=]() -> void
        {
            g_ui.set_screen(screen::edit);
        });
    }

    void ui::on_set_screen_about()
    {
        worker_ui.enqueue([=]() -> void
        {
            g_ui.set_screen(screen::about);
        });
    }

    // Playlist Screen Methods

    void ui::on_set_atmosphere_favorite(atmosphere_handle_t handle, bool favorited)
    {
        worker_ui.enqueue([=]() -> void
        {
            if (favorited) {
                g_ui.add_atmosphere_to_playlist(playlist_handle::favorites, handle);
            } else {
                g_ui.remove_atmosphere_from_playlist(playlist_handle::favorites, handle);
            }
        });

        worker_background.enqueue([=]() -> void
        {
            auto& entry = g_atmosphere_directory.get_entry(handle);
            entry.favorite = favorited;

            if (favorited) {
                g_playlist_directory.add_atmosphere(playlist_handle::favorites, handle);
            } else {
                g_playlist_directory.remove_atmosphere(playlist_handle::favorites, handle);
            }
        });
    }

    void ui::on_set_atmosphere_volume(atmosphere_widget& widget, atmosphere_handle_t handle, double volume)
    {
        worker_ui.enqueue([=]() -> void
        {
            auto it = g_ui.root.playlist.atmospheres.find(handle);
            MX_ASSERT(it != g_ui.root.playlist.atmospheres.end());
            auto& widget = it->second;
            widget.volumeVal = volume;
            widget.muteUnmute.checked  = false;
        });

        worker_background.enqueue([=]() -> void
        {
            g_playback_manager.set_atmosphere_volume(handle, volume);
            g_playback_manager.set_atmosphere_mute(handle, false);
        });
    }

    void ui::on_set_atmosphere_mute(atmosphere_widget& widget, atmosphere_handle_t handle, bool muted)
    {
        worker_ui.enqueue([=]() -> void
        {
            auto it = g_ui.root.playlist.atmospheres.find(handle);
            MX_ASSERT(it != g_ui.root.playlist.atmospheres.end());
            auto& widget = it->second;
            widget.volume.currentVal = muted ? 0.0f : widget.volumeVal;
        });

        worker_background.enqueue([=]() -> void
        {
            g_playback_manager.set_atmosphere_mute(handle, muted);
        });
    }

    void ui::on_set_atmosphere_play(atmosphere_handle_t handle, bool playing)
    {
        worker_ui.enqueue([=]()
        {
            if (playing) {
                g_ui.add_atmosphere_to_playlist(playlist_handle::playing, handle);
            } else {
                g_ui.remove_atmosphere_from_playlist(playlist_handle::playing, handle);
            }
        });

        worker_background.enqueue([=]()
        {
            if (playing) {
                g_playback_manager.play_atmosphere(handle);
            } else {
                g_playback_manager.stop_atmosphere(handle);
            }
        });
    }

    void ui::on_create_new_atmosphere(playlist_handle_t playlistHandle)
    {
        constexpr auto* defaultTitle = "New Mix";
        worker_background.enqueue([=]()
        {
            atmosphere_entry atmosphereEntry =
            {
                g_random.next_uuid(),
                defaultTitle,
                1.0f,
                false,
                false,
                {}
            };

            auto atmosphereHandle = g_atmosphere_directory.add_entry(std::move(atmosphereEntry));
            MX_LOG_INFO_BLOCKING("atmosphereHandle : {}", atmosphereHandle);

            // make sure we don't double add if we're adding from the 'All' playlist
            if (playlistHandle != playlist_handle::all) {
                g_playlist_directory.add_atmosphere(playlist_handle::all, atmosphereHandle);
            }
            g_playlist_directory.add_atmosphere(playlistHandle, atmosphereHandle);

            worker_ui.enqueue([=]()
            {
                atmosphere_widget atmosphereWidget;
                atmosphereWidget.title = imgui::button_data(defaultTitle);
                atmosphereWidget.atmosphereHandle = atmosphereHandle;

                MX_LOG_INFO("title : {}", atmosphereWidget.title.text());

                // this atmosphere should not exist in our map yet
                MX_ASSERT(g_ui.root.playlist.atmospheres.find(atmosphereHandle) == g_ui.root.playlist.atmospheres.end());
                g_ui.root.playlist.atmospheres.insert({atmosphereHandle, std::move(atmosphereWidget)});

                // add handle to our playlists
                if (playlistHandle != playlist_handle::all) {
                    g_ui.add_atmosphere_to_playlist(playlist_handle::all, atmosphereHandle);
                }
                g_ui.add_atmosphere_to_playlist(playlistHandle, atmosphereHandle);
            });
        });
    }

    void ui::on_toggle_atmosphere_context_menu(playlist_handle_t playlistHandle, atmosphere_handle_t atmosphereHandle)
    {
        worker_ui.enqueue([=]() -> void
        {
            auto& acm = g_ui.root.playlist.atmosphereContextMenu;

            if (acm.visible && acm.atmosphereHandle == atmosphereHandle)
            {
                acm.visible = false;
                acm.atmosphereHandle = atmosphere_handle::invalid;
            } else
            {
                acm.visible = true;
                acm.atmosphereHandle = atmosphereHandle;
            }
        });
    }

    void ui::on_remove_atmosphere(playlist_handle_t playlistHandle, atmosphere_handle_t atmosphereHandle)
    {
        worker_ui.enqueue([=]() -> void
        {
            g_ui.remove_atmosphere_from_playlist(playlistHandle, atmosphereHandle);
            g_ui.hide_atmosphere_context_menu();
        });

        worker_background.enqueue([=]() -> void
        {
            g_playlist_directory.remove_atmosphere(playlistHandle, atmosphereHandle);
        });
    }

    void ui::on_open_atmosphere_copy_menu()
    {
        worker_ui.enqueue([]() -> void
        {
            g_ui.root.playlist.atmosphereContextMenu.copyVisible = true;
            g_ui.root.playlist.atmosphereContextMenu.moveVisible = false;
        });
    }

    void ui::on_copy_atmosphere(playlist_handle_t playlistHandle, atmosphere_handle_t atmosphereHandle)
    {
        worker_ui.enqueue([=]() -> void
        {
           g_ui.hide_atmosphere_context_menu();
           g_ui.add_atmosphere_to_playlist(playlistHandle, atmosphereHandle);
        });

        worker_background.enqueue([=]() -> void
        {
            g_playlist_directory.add_atmosphere(playlistHandle, atmosphereHandle);
        });
    }

    void ui::on_open_atmosphere_move_menu()
    {
        worker_ui.enqueue([]() -> void
        {
            g_ui.root.playlist.atmosphereContextMenu.copyVisible = false;
            g_ui.root.playlist.atmosphereContextMenu.moveVisible = true;
        });
    }

    void ui::on_move_atmosphere(playlist_handle_t playlistSrc, playlist_handle_t playlistDest, atmosphere_handle_t atmosphereHandle)
    {
        MX_ASSERT(playlistSrc >= playlist_handle::__first_generated);
        MX_ASSERT(playlistDest >= playlist_handle::__first_generated);

        if (playlistSrc != playlistDest)
        {
            worker_ui.enqueue([=]() -> void
            {
               g_ui.hide_atmosphere_context_menu();
               g_ui.add_atmosphere_to_playlist(playlistDest, atmosphereHandle);
               g_ui.remove_atmosphere_from_playlist(playlistSrc, atmosphereHandle);
            });

            worker_background.enqueue([=]() -> void
            {
                g_playlist_directory.add_atmosphere(playlistDest, atmosphereHandle);
                g_playlist_directory.remove_atmosphere(playlistSrc, atmosphereHandle);
            });
        }
        else
        {
            worker_ui.enqueue([]() -> void
            {
                g_ui.hide_atmosphere_context_menu();
            });
        }
    }

    // Edit Screen Methods

    void ui::on_do_audio_search(const utf8_string& query)
    {
        worker_background.enqueue([=]() -> void
        {
            auto audioHandles = g_audio_directory.get_audio_handles_by_query(query);
            std::vector<audio_query_result_widget> queryWidgets;
            for(auto handle : audioHandles) {
                const auto& entry = g_audio_directory.get_entry(handle);
                queryWidgets.push_back({handle, {entry.title}});
            }

            worker_ui.enqueue([queryWidgets=std::move(queryWidgets)]() mutable -> void
            {
                g_ui.root.edit.search.queryResults = std::move(queryWidgets);
            });
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
                g_ui.on_set_screen_edit(atmosphere_handle::invalid);
            });
            mxButton(&root.sidebar.about, self.tags = {"mediumText", "sidebarEntry"};
            self.on_click = []() -> void
            {
                g_ui.on_set_screen_about();
            });
        }
    }

    void ui::draw_playlist(float dt)
    {
        // skip drawing if no playlists
        if (root.playlist.playlists.size() == 0) {
            return;
        }

        auto& currentPlaylist = get_current_playlist_widget();
        const auto currentPlaylistHandle = currentPlaylist.handle;
        mxVBox(&root.playlist.vflex, self.tags = {"content"})
        {
            mxLabel(&root.playlist.heading, self.tags= {"header", "darkText"});
            mxLabel(currentPlaylist.title, self.tags = {"header", "title", "lightText"});
            mxHBox(&root.playlist.buttonFlex)
            {
                mxButton(&root.playlist.export_,
                    self.on_click = []() -> void {MX_LOG_INFO("EXPORT");};);
                mxButton(&root.playlist.import,
                    self.on_click = []() -> void {MX_LOG_INFO("IMPORT");};);
                mxSpacer();
                mxButton(&root.playlist.newMix,
                self.on_click = [&]() -> void
                {
                    MX_LOG_INFO("NEW MIX");
                    g_ui.on_create_new_atmosphere(currentPlaylist.handle);
                };);
                mxTextBox(&root.playlist.searchBox, self.tags = {"search"};
                self.on_submit = [](const utf8_string& str) -> void
                {
                    MX_LOG_INFO("SUBMIT : {}", str);
                });
                mxButton(&root.playlist.searchButton, self.tags = {"search"};
                self.on_click = []() -> void
                {
                    MX_LOG_INFO("SEARCH");
                });
            }
            mxVScroll(&root.playlist.scroll)
            {
                for (auto& currentAtmosphereHandle : currentPlaylist.entries)
                {
                    auto it = root.playlist.atmospheres.find(currentAtmosphereHandle);
                    MX_ASSERT(it != root.playlist.atmospheres.end());
                    auto& currentEntry = it->second;

                    mxHBox(&root.playlist.rowFlex, self.tags={"playlistEntry"})
                    {
                        mxCheckbox(&currentEntry.favorite, self.tags = {"entryFavorite"};
                        self.on_change = [=](bool favorited) -> void
                        {
                            g_ui.on_set_atmosphere_favorite(currentAtmosphereHandle, favorited);
                        }); // FAVORITE
                        mxButton(&currentEntry.title, self.tags = {"entryTitle"},
                        self.on_click = [=]() -> void
                        {
                            g_ui.on_toggle_atmosphere_context_menu(currentPlaylist.handle, currentAtmosphereHandle);

                        }); // TOGGLE CONTEXT MENU
                        mxSlider(&currentEntry.volume, self.tags = {"entryVolume"};
                        self.on_change = [&](double volume) -> void
                        {
                            g_ui.on_set_atmosphere_volume(currentEntry, currentAtmosphereHandle, volume);
                        }); // VOLUME
                        mxCheckbox(&currentEntry.muteUnmute, self.tags = {"entryMute"};
                        self.on_change = [&](bool muted) -> void
                        {
                            g_ui.on_set_atmosphere_mute(currentEntry, currentAtmosphereHandle, muted);
                        }); // MUTE
                        mxCheckbox(&currentEntry.playStop, self.tags = {"entryPlay"};
                        self.on_change = [=](bool playing) -> void
                        {
                            g_ui.on_set_atmosphere_play(currentAtmosphereHandle, playing);
                        }); // PLAY/STOP BUTTON
                    }

                    if (root.playlist.atmosphereContextMenu.visible &&
                        root.playlist.atmosphereContextMenu.atmosphereHandle == currentAtmosphereHandle)
                    {
                        mxButton({"Edit"}, self.tags={"indent", "contextMenuEntry"},
                        self.on_click = []() -> void
                        {
                            MX_LOG_INFO("EDIT");
                        });

                        // only enable removal from user created playlists
                        if (currentPlaylistHandle >= playlist_handle::__first_generated)
                        {
                            mxButton({"Remove"}, self.tags={"indent", "contextMenuEntry"},
                            self.on_click = [=]() -> void
                            {
                                MX_LOG_INFO("Remove atmosphere {} from playlist {}", currentAtmosphereHandle, currentPlaylistHandle);
                                g_ui.on_remove_atmosphere(currentPlaylistHandle, currentAtmosphereHandle);
                            });
                        }

                        mxButton({"Copy..."}, self.tags={"indent", "contextMenuEntry"},
                        self.on_click = []() -> void
                        {
                            g_ui.on_open_atmosphere_copy_menu();
                        });

                        if (g_ui.root.playlist.atmosphereContextMenu.copyVisible)
                        {
                            for(auto& playlist : g_ui.root.playlist.playlists) {
                                if (playlist.handle >= playlist_handle::__first_generated)
                                {
                                    mxButton(playlist.title, self.tags={"indent", "contextMenuEntry"},
                                    self.on_click = [&]() -> void
                                    {
                                        MX_LOG_INFO("Copy to '{}'", playlist.title);
                                        g_ui.on_copy_atmosphere(playlist.handle, currentAtmosphereHandle);

                                    });
                                }
                            }
                        }

                        if (currentPlaylistHandle >= playlist_handle::__first_generated)
                        {
                            mxButton({"Move..."}, self.tags={"indent", "contextMenuEntry"},
                            self.on_click = []() -> void
                            {
                                g_ui.on_open_atmosphere_move_menu();
                            });

                            if (g_ui.root.playlist.atmosphereContextMenu.moveVisible)
                            {
                                for(auto& playlist : g_ui.root.playlist.playlists) {
                                    if (playlist.handle >= playlist_handle::__first_generated)
                                    {
                                        mxButton(playlist.title, self.tags={"indent", "contextMenuEntry"},
                                        self.on_click = [&]() -> void
                                        {
                                            MX_LOG_INFO("Move to '{}'", playlist.title);
                                            g_ui.on_move_atmosphere(currentPlaylistHandle, playlist.handle, currentAtmosphereHandle);
                                        });
                                    }
                                }
                            }
                        }

                        mxButton({"Delete"}, self.tags={"indent", "contextMenuEntry"},
                        self.on_click = []() -> void
                        {
                            MX_LOG_INFO("DELETE");
                        });
                    }
                }
            }
        }
    }

    void ui::draw_edit(float dt)
    {
        mxVBox(&root.edit.vflex, self.tags = {"content"})
        {
            mxLabel(&root.edit.heading, self.tags= {"header", "darkText"});
            mxButton(&root.edit.title, self.tags = {"header", "title", "lightText"});
            mxHBox(&root.edit.hflex)
            {
                mxSpacer();
                mxTextBox(&root.edit.searchBox, self.tags = {"search"};
                self.on_submit = [](const utf8_string& query) -> void
                {
                    g_ui.on_do_audio_search(query);
                });
            }
            mxHBox(&root.edit.hflex)
            {
                mxVScroll(&root.edit.tracks.scroll)
                {

                }
                mxVScroll(&root.edit.search.scroll)
                {
                    for(auto& queryResult : root.edit.search.queryResults)
                    {
                        mxButton(queryResult.title, self.tags={"audioResult"};
                        self.on_click = [&]() -> void
                        {
                            MX_LOG_INFO("CLICKED : {}, {}", queryResult.title, queryResult.handle);
                        });
                        mxSpacer();
                    }
                }
            }
        }
    }

}
