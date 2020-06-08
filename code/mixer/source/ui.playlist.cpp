#include "ui.hpp"
#include "random.hpp"

namespace tabletopify
{
    // Playlist Screen Methods

    void ui::on_start_playlist_rename(playlist_handle_t playlistHandle)
    {
        worker_ui.enqueue([=]() -> void
        {
            g_ui.root.playlist.edit_title = true;
            auto& playlist = g_ui.get_playlist_widget(playlistHandle);
            playlist.title_textbox = imgui::text_box_data(playlist.title, "Playlist Name");
            playlist.title_textbox.selectBegin = 0;
            playlist.title_textbox.selectEnd = playlist.title_textbox.sb.size();
            imgui::g_context.focus_widget(playlist.title_textbox.id);
        });
    }

    void ui::on_set_playlist_name(playlist_handle_t playlistHandle, utf8_string name)
    {
        worker_ui.enqueue([=,name=std::move(name)]() -> void
        {
            auto& playlist = g_ui.get_playlist_widget(playlistHandle);
            playlist.title = name;
            g_ui.root.playlist.edit_title = false;

            worker_background.enqueue([=,name=std::move(name)]() -> void
            {
                auto& entry = g_playlist_directory.get_entry(playlistHandle);
                entry.title = std::move(name);
            });
        });
    }

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

    void ui::on_delete_atmosphere(atmosphere_handle_t atmosphereHandle)
    {
        worker_ui.enqueue([=]() -> void
        {
            for(auto& widget : g_ui.root.playlist.playlists)
            {
                auto& entries = widget.entries;
                auto it = std::find(entries.begin(), entries.end(), atmosphereHandle);
                if (it != entries.end())
                {
                    entries.erase(it);
                }
            }
        });

        worker_background.enqueue([=]() -> void
        {
            g_playback_manager.stop_atmosphere(atmosphereHandle);
            for(auto playlistHandle : g_playlist_directory.get_playlist_handles())
            {
                g_playlist_directory.remove_atmosphere(playlistHandle, atmosphereHandle);
            }
            g_atmosphere_directory.remove_entry(atmosphereHandle);
        });
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
            if (root.playlist.edit_title)
            {
                mxTextBox(&currentPlaylist.title_textbox, self.tags = {"header", "title", "lightText"},
                self.on_submit = [=](const utf8_string& str) -> void
                {
                    g_ui.on_set_playlist_name(currentPlaylistHandle, str);
                });
            }
            else
            {
                if (currentPlaylistHandle >= playlist_handle::__first_generated)
                {
                    mxButton(currentPlaylist.title, self.tags = {"header", "title", "lightText"},
                    self.on_double_click = [=]() -> void
                    {
                        g_ui.on_start_playlist_rename(currentPlaylistHandle);
                    });
                }
                else
                {
                    mxLabel(currentPlaylist.title, self.tags={"header", "title", "lightText"});
                }
            }

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
                        self.on_click = [=]() -> void
                        {
                            MX_LOG_INFO("Delete atmosphere : {}", currentAtmosphereHandle);
                            g_ui.on_delete_atmosphere(currentAtmosphereHandle);
                        });
                    }
                }
            }
        }
    }
}