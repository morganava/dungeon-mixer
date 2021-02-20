#include "ui.hpp"
#include "random.hpp"
#include "serialization.hpp"
#include "paths.hpp"
#include "file_dialog.hpp"
#include "string_manager.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace dm
{
    // Playlist Screen Methods

    void ui::on_start_playlist_rename(playlist_handle_t playlistHandle)
    {
        worker_ui.enqueue([=]() -> void
        {
            g_ui->root.playlist.edit_title = true;
            auto& playlist = g_ui->get_playlist_widget(playlistHandle);
            playlist.title_textbox = imgui::text_box_data(playlist.title, "Playlist Name");
            playlist.title_textbox.selectBegin = 0;
            playlist.title_textbox.selectEnd = playlist.title_textbox.sb.size();
            imgui::g_context->focus_widget(playlist.title_textbox.id);
        });
    }

    void ui::on_set_playlist_name(playlist_handle_t playlistHandle, utf8_string name)
    {
        worker_ui.enqueue([=,name=std::move(name)]() -> void
        {
            auto& playlist = g_ui->get_playlist_widget(playlistHandle);
            playlist.title = name;
            g_ui->root.playlist.edit_title = false;

            worker_audio.enqueue([=,name=std::move(name)]() mutable -> void
            {
                g_playlist_directory->set_title(playlistHandle, std::move(name));
            });
        });
    }

    void ui::on_start_playlist_export(playlist_handle_t playlistHandle)
    {
        worker_ui.enqueue([=]() -> void
        {
            auto& playlist = g_ui->get_playlist_widget(playlistHandle);

            auto optDestination = file_dialog::save({"json"}, paths::home(), fmt::format("{}.json", playlist.title));
            if (optDestination.has_value())
            {
                // it seems like this code should not be necessary since we are providing the file extension >:[
                auto dest = std::move(optDestination.value());
                if (dest.extension() != ".json")
                {
                    dest += ".json";
                }

                MX_LOG_INFO("saving playlist '{}' to {}",playlist.title, dest);
                g_serialization_manager->save_playlist(std::move(dest), playlistHandle);
            }
        });
    }

    void ui::on_start_playlist_import()
    {
        worker_ui.enqueue([=]() -> void
        {
            const auto optSource = file_dialog::open({"json"}, paths::home());
            if (optSource.has_value())
            {
                auto source = optSource.value();
                MX_LOG_INFO("loading playlist from {}", source);
                g_serialization_manager->load_playlist(std::move(source));
            }
        });
    }

    void ui::on_set_atmosphere_favorite(atmosphere_handle_t handle, bool favorite)
    {
        worker_ui.enqueue([=]() -> void
        {
            if (favorite) {
                g_ui->add_atmosphere_to_playlist(playlist_handle::favorites, handle);
            } else {
                g_ui->remove_atmosphere_from_playlist(playlist_handle::favorites, handle);
            }
        });

        worker_audio.enqueue([=]() -> void
        {
            g_atmosphere_directory->set_favorite(handle, favorite);

            if (favorite) {
                g_playlist_directory->add_atmosphere(playlist_handle::favorites, handle);
            } else {
                g_playlist_directory->remove_atmosphere(playlist_handle::favorites, handle);
            }
        });
    }

    void ui::on_set_atmosphere_volume(atmosphere_widget& widget, atmosphere_handle_t handle, double volume)
    {
        worker_ui.enqueue([=]() -> void
        {
            auto it = g_ui->root.playlist.atmospheres.find(handle);
            MX_ASSERT(it != g_ui->root.playlist.atmospheres.end());
            auto& widget = it->second;
            widget.volumeVal = volume;
            widget.muteUnmute.checked  = false;
        });

        worker_audio.enqueue([=]() -> void
        {
            g_atmosphere_directory->set_volume(handle, volume);
            g_atmosphere_directory->set_muted(handle, false);
            g_playback_manager->sync_atmosphere(handle);
        });
    }

    void ui::on_set_atmosphere_mute(atmosphere_widget& widget, atmosphere_handle_t handle, bool muted)
    {
        worker_ui.enqueue([=]() -> void
        {
            auto it = g_ui->root.playlist.atmospheres.find(handle);
            MX_ASSERT(it != g_ui->root.playlist.atmospheres.end());
            auto& widget = it->second;
            widget.volume.value = muted ? 0.0f : widget.volumeVal;
        });

        worker_audio.enqueue([=]() -> void
        {
            g_atmosphere_directory->set_muted(handle, muted);
            g_playback_manager->sync_atmosphere(handle);
        });
    }

    void ui::on_set_atmosphere_play(atmosphere_handle_t handle, bool playing)
    {
        worker_ui.enqueue([=]()
        {
            if (playing) {
                g_ui->add_atmosphere_to_playlist(playlist_handle::playing, handle);
            } else {
                g_ui->remove_atmosphere_from_playlist(playlist_handle::playing, handle);
            }
        });

        worker_audio.enqueue([=]()
        {
            if (playing) {
                g_playlist_directory->add_atmosphere(playlist_handle::playing, handle);
                g_playback_manager->play_atmosphere(handle);
            } else {
                g_playlist_directory->remove_atmosphere(playlist_handle::playing, handle);
                g_playback_manager->stop_atmosphere(handle);
            }
        });
    }

    // this functionality needs to be pulled out into helper functions for EDIT button
    void ui::on_create_new_atmosphere(playlist_handle_t playlistHandle)
    {
        worker_audio.enqueue([=]()
        {
            atmosphere_entry atmosphereEntry =
            {
                g_strings[mixer_string::new_atmosphere_name],
                1.0f,
                false,
                false,
                {}
            };

            auto atmosphereHandle = g_atmosphere_directory->add_entry(std::move(atmosphereEntry));
            MX_LOG_INFO_BLOCKING("atmosphereHandle : {}", atmosphereHandle);

            // make sure we don't double add if we're adding from the 'All' playlist
            if (playlistHandle != playlist_handle::all) {
                g_playlist_directory->add_atmosphere(playlist_handle::all, atmosphereHandle);
            }
            g_playlist_directory->add_atmosphere(playlistHandle, atmosphereHandle);

            worker_ui.enqueue([=]()
            {
                atmosphere_widget atmosphereWidget;
                atmosphereWidget.title = g_strings[mixer_string::new_atmosphere_name];
                atmosphereWidget.atmosphereHandle = atmosphereHandle;

                // this atmosphere should not exist in our map yet
                MX_ASSERT(g_ui->root.playlist.atmospheres.find(atmosphereHandle) == g_ui->root.playlist.atmospheres.end());
                g_ui->root.playlist.atmospheres.insert({atmosphereHandle, std::move(atmosphereWidget)});

                // add handle to our playlists
                if (playlistHandle != playlist_handle::all) {
                    g_ui->add_atmosphere_to_playlist(playlist_handle::all, atmosphereHandle);
                }
                g_ui->add_atmosphere_to_playlist(playlistHandle, atmosphereHandle);
            });
        });
    }

    void ui::on_toggle_atmosphere_context_menu(playlist_handle_t playlistHandle, atmosphere_handle_t atmosphereHandle)
    {
        worker_ui.enqueue([=]() -> void
        {
            auto& acm = g_ui->root.playlist.atmosphereContextMenu;

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
            g_ui->remove_atmosphere_from_playlist(playlistHandle, atmosphereHandle);
            g_ui->hide_atmosphere_context_menu();
        });

        worker_audio.enqueue([=]() -> void
        {
            g_playlist_directory->remove_atmosphere(playlistHandle, atmosphereHandle);
        });
    }

    void ui::on_open_atmosphere_copy_menu()
    {
        worker_ui.enqueue([]() -> void
        {
            g_ui->root.playlist.atmosphereContextMenu.copyVisible = true;
            g_ui->root.playlist.atmosphereContextMenu.moveVisible = false;
        });
    }

    void ui::on_copy_atmosphere(playlist_handle_t playlistHandle, atmosphere_handle_t atmosphereHandle)
    {
        worker_ui.enqueue([=]() -> void
        {
           g_ui->hide_atmosphere_context_menu();
           g_ui->add_atmosphere_to_playlist(playlistHandle, atmosphereHandle);
        });

        worker_audio.enqueue([=]() -> void
        {
            g_playlist_directory->add_atmosphere(playlistHandle, atmosphereHandle);
        });
    }

    void ui::on_open_atmosphere_move_menu()
    {
        worker_ui.enqueue([]() -> void
        {
            g_ui->root.playlist.atmosphereContextMenu.copyVisible = false;
            g_ui->root.playlist.atmosphereContextMenu.moveVisible = true;
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
               g_ui->hide_atmosphere_context_menu();
               g_ui->add_atmosphere_to_playlist(playlistDest, atmosphereHandle);
               g_ui->remove_atmosphere_from_playlist(playlistSrc, atmosphereHandle);
            });

            worker_audio.enqueue([=]() -> void
            {
                g_playlist_directory->add_atmosphere(playlistDest, atmosphereHandle);
                g_playlist_directory->remove_atmosphere(playlistSrc, atmosphereHandle);
            });
        }
        else
        {
            worker_ui.enqueue([]() -> void
            {
                g_ui->hide_atmosphere_context_menu();
            });
        }
    }

    void ui::on_delete_atmosphere(atmosphere_handle_t atmosphereHandle)
    {
        worker_ui.enqueue([=]() -> void
        {
            for(auto& widget : g_ui->root.playlist.playlists)
            {
                auto& entries = widget.entries;
                auto it = std::find(entries.begin(), entries.end(), atmosphereHandle);
                if (it != entries.end())
                {
                    entries.erase(it);
                }
            }

            if (atmosphereHandle == g_ui->root.edit.handle)
            {
                g_ui->root.edit.handle = atmosphere_handle::invalid;
            }
        });

        worker_audio.enqueue([=]() -> void
        {
            g_playback_manager->stop_atmosphere(atmosphereHandle);
            for(auto playlistHandle : g_playlist_directory->get_playlist_handles())
            {
                g_playlist_directory->remove_atmosphere(playlistHandle, atmosphereHandle);
            }
            g_atmosphere_directory->remove_entry(atmosphereHandle);
        });
    }

    void ui::on_do_atmosphere_search(playlist_handle_t playlist, const utf8_string& query)
    {
        worker_audio.enqueue([=]() -> void
        {
            auto filtered = g_playlist_directory->filtered_atmospheres(playlist, query);

            worker_ui.enqueue([=,filtered=std::move(filtered)]() mutable -> void
            {
                auto& playlistWidget = g_ui->get_playlist_widget(playlist);
                playlistWidget.entries = std::move(filtered);
            });
        });
    }

    // Draw

    void ui::draw_playlist(float dt)
    {
        // skip drawing if no playlists
        if (root.playlist.playlists.size() == 0) {
            return;
        }

        auto& currentPlaylist = get_current_playlist_widget();
        const auto currentPlaylistHandle = currentPlaylist.handle;
        mxVBox(self.tags = {"content"})
        {
            mxLabel(g_strings[mixer_string::playlist_screen], self.tags= {"header", "darkText"});
            if (root.playlist.edit_title)
            {
                mxTextBox(&currentPlaylist.title_textbox, self.tags = {"header", "title", "lightText"},
                    self.on_submit = [=](const utf8_string& str) -> void
                    {
                        g_ui->on_set_playlist_name(currentPlaylistHandle, str);
                    };
                    if (!self.is_focused())
                    {
                        g_ui->on_set_playlist_name(currentPlaylistHandle, self.text());
                    });
            }
            else
            {
                if (currentPlaylistHandle >= playlist_handle::__first_generated)
                {
                    mxButton(currentPlaylist.title, self.tags = {"header", "title", "lightText"},
                        self.on_double_click = [=]() -> void
                        {
                            g_ui->on_start_playlist_rename(currentPlaylistHandle);
                        });
                }
                else
                {
                    mxLabel(currentPlaylist.title, self.tags={"header", "title", "lightText"});
                }
            }

            mxHBox(self.tags={"playlistButtonRow"})
            {
                mxButton(g_strings[mixer_string::export_playlist],
                    self.on_click = [=]() -> void
                    {
                        g_ui->on_start_playlist_export(currentPlaylist.handle);
                    });
                mxButton(g_strings[mixer_string::import_playlist],
                    self.on_click = []() -> void
                    {
                        g_ui->on_start_playlist_import();
                    });
                mxSpacer();
                mxButton(g_strings[mixer_string::create_new_mix],
                    self.on_click = [&]() -> void
                    {
                        g_ui->on_create_new_atmosphere(currentPlaylist.handle);
                    });
                currentPlaylist.searchBox.placeholderText = g_strings[mixer_string::atmosphere_search];
                mxTextBox({&currentPlaylist.searchBox}, self.tags = {"search"};
                    self.on_change = [=](const utf8_string& query) -> void
                    {
                        g_ui->on_do_atmosphere_search(currentPlaylist.handle, query);
                    });
            }
            mxVScroll(&root.playlist.scroll)
            {
                for (auto& currentAtmosphereHandle : currentPlaylist.entries)
                {
                    auto it = root.playlist.atmospheres.find(currentAtmosphereHandle);
                    MX_ASSERT(it != root.playlist.atmospheres.end());
                    auto& currentEntry = it->second;

                    mxHBox(self.tags={"playlistEntry"})
                    {
                        mxCheckbox(&currentEntry.favorite, self.tags = {"entryFavorite"};
                        self.on_change = [=](bool favorited) -> void
                        {
                            g_ui->on_set_atmosphere_favorite(currentAtmosphereHandle, favorited);
                        }); // FAVORITE
                        mxButton(currentEntry.title, self.tags = {"entryTitle"},
                        self.on_click = [=]() -> void
                        {
                            g_ui->on_toggle_atmosphere_context_menu(currentPlaylist.handle, currentAtmosphereHandle);

                        }); // TOGGLE CONTEXT MENU
                        mxSlider(&currentEntry.volume, self.tags = {"entryVolume"};
                        self.on_change = [&](double volume) -> void
                        {
                            g_ui->on_set_atmosphere_volume(currentEntry, currentAtmosphereHandle, volume);
                        }); // VOLUME
                        mxCheckbox(&currentEntry.muteUnmute, self.tags = {"entryMute"};
                        self.on_change = [&](bool muted) -> void
                        {
                            g_ui->on_set_atmosphere_mute(currentEntry, currentAtmosphereHandle, muted);
                        }); // MUTE
                        mxCheckbox(&currentEntry.playStop, self.tags = {"entryPlay"};
                        self.on_change = [=](bool playing) -> void
                        {
                            g_ui->on_set_atmosphere_play(currentAtmosphereHandle, playing);
                        }); // PLAY/STOP BUTTON
                    }

                    if (root.playlist.atmosphereContextMenu.visible &&
                        root.playlist.atmosphereContextMenu.atmosphereHandle == currentAtmosphereHandle)
                    {
                        mxButton(g_strings[mixer_string::edit_atmosphere], self.tags={"indent", "contextMenuEntry"},
                        self.on_click = [=]() -> void
                        {
                            g_ui->on_set_screen_edit(currentAtmosphereHandle);
                        });

                        // only enable removal from user created playlists
                        if (currentPlaylistHandle >= playlist_handle::__first_generated)
                        {
                            mxButton(g_strings[mixer_string::remove_atmosphere], self.tags={"indent", "contextMenuEntry"},
                            self.on_click = [=]() -> void
                            {
                                MX_LOG_INFO("Remove atmosphere {} from playlist {}", currentAtmosphereHandle, currentPlaylistHandle);
                                g_ui->on_remove_atmosphere(currentPlaylistHandle, currentAtmosphereHandle);
                            });
                        }

                        mxButton(g_strings[mixer_string::copy_atmosphere], self.tags={"indent", "contextMenuEntry"},
                        self.on_click = []() -> void
                        {
                            g_ui->on_open_atmosphere_copy_menu();
                        });

                        if (g_ui->root.playlist.atmosphereContextMenu.copyVisible)
                        {
                            for(auto& playlist : g_ui->root.playlist.playlists) {
                                if (playlist.handle >= playlist_handle::__first_generated)
                                {
                                    mxButton(playlist.title, self.tags={"indent", "contextMenuEntry"},
                                    self.on_click = [&]() -> void
                                    {
                                        MX_LOG_INFO("Copy to '{}'", playlist.title);
                                        g_ui->on_copy_atmosphere(playlist.handle, currentAtmosphereHandle);

                                    });
                                }
                            }
                        }

                        if (currentPlaylistHandle >= playlist_handle::__first_generated)
                        {
                            mxButton(g_strings[mixer_string::move_atmosphere], self.tags={"indent", "contextMenuEntry"},
                            self.on_click = []() -> void
                            {
                                g_ui->on_open_atmosphere_move_menu();
                            });

                            if (g_ui->root.playlist.atmosphereContextMenu.moveVisible)
                            {
                                for(auto& playlist : g_ui->root.playlist.playlists) {
                                    if (playlist.handle >= playlist_handle::__first_generated)
                                    {
                                        mxButton(playlist.title, self.tags={"indent", "contextMenuEntry"},
                                        self.on_click = [&]() -> void
                                        {
                                            MX_LOG_INFO("Move to '{}'", playlist.title);
                                            g_ui->on_move_atmosphere(currentPlaylistHandle, playlist.handle, currentAtmosphereHandle);
                                        });
                                    }
                                }
                            }
                        }

                        mxButton(g_strings[mixer_string::delete_atmosphere], self.tags={"indent", "contextMenuEntry"},
                        self.on_click = [=]() -> void
                        {
                            MX_LOG_INFO("Delete atmosphere : {}", currentAtmosphereHandle);
                            g_ui->on_delete_atmosphere(currentAtmosphereHandle);
                        });
                    }
                }
            }
        }
    }
}
