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
                }
            }
        }

        g_nanovg.set_draw_layer(draw_layer::foreground);
        imgui::g_context.push_scissor({{0,0},{(double)width,(double)height}});
        g_nanovg.color_fill_circle({{64,64}, 32}, color::yellow);

        g_nanovg.set_draw_layer(draw_layer::background);
        imgui::g_context.push_scissor({{0,0},{(double)width,(double)height}});
        g_nanovg.color_fill_circle({{48,48}, 32}, color::teal);

        imgui::g_context.pop_scissor();
    }

    void ui::sync_playlists()
    {
        worker(Worker_Background) << []()
        {
            std::vector<playlist_widget> playlists;
            std::map<atmosphere_handle_t, atmosphere_widget> atmospheres;

            for (auto playlistHandle : g_playlist_directory.get_playlist_handles())
            {
                auto& playlistEntry = g_playlist_directory.get_entry(playlistHandle);
                playlist_widget currentPlaylistWidget;

                currentPlaylistWidget.handle = playlistHandle;
                currentPlaylistWidget.title.text = playlistEntry.title;
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

            worker(Worker_UI) << [playlists=std::move(playlists), atmospheres=std::move(atmospheres)]() mutable-> void
            {
                g_ui.root.playlist.playlists = std::move(playlists);
                g_ui.root.playlist.atmospheres = std::move(atmospheres);
            };
        };
    }

    playlist_widget& ui::get_playlist_widget(playlist_handle_t handle)
    {
        for(auto& playlistWidget : root.playlist.playlists) {
            if (playlistWidget.handle == handle) {
                return playlistWidget;
            }
        }

        return *static_cast<playlist_widget*>(nullptr);
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

        entries.push_back(atmosphere);
    }

    void ui::set_atmosphere_favorite(atmosphere_handle_t handle, bool favorited)
    {
        worker(Worker_Background) << [=]() -> void
        {
            if (favorited) {
                g_playlist_directory.favorite_atmosphere(handle);
            } else {
                g_playlist_directory.unfavorite_atmosphere(handle);
            }

            worker(Worker_UI) << [=]() -> void
            {
                if (favorited) {
                    g_ui.add_atmosphere_to_playlist(playlist_handle::favorites, handle);
                } else {
                    g_ui.remove_atmosphere_from_playlist(playlist_handle::favorites, handle);
                }
            };
        };
    }

    void ui::set_atmosphere_volume(atmosphere_widget& widget, atmosphere_handle_t handle, double volume)
    {
        widget.volumeVal = volume;
        widget.muteUnmute.checked = false;
        worker(Worker_Background) << [=]() -> void
        {
            g_playback_manager.set_atmosphere_volume(handle, volume);
            g_playback_manager.set_atmosphere_mute(handle, false);
        };
    }

    void ui::set_atmosphere_mute(atmosphere_widget& widget, atmosphere_handle_t handle, bool muted)
    {
        widget.volume.currentVal = muted ? 0.0f : widget.volumeVal;
        worker(Worker_Background) << [=]() -> void
        {
            g_playback_manager.set_atmosphere_mute(handle, muted);
        };
    }

    void ui::set_atmosphere_play(atmosphere_handle_t handle, bool playing)
    {
        worker(Worker_Background) << [=]()
        {
            if (playing) {
                g_playback_manager.play_atmosphere(handle);
                g_playlist_directory.add_atmosphere(playlist_handle::playing, handle);
            } else {
                g_playback_manager.stop_atmosphere(handle);
                g_playlist_directory.remove_atmosphere(playlist_handle::playing, handle);
            }

            worker(Worker_UI) << [=]()
            {
                if (playing) {
                    g_ui.add_atmosphere_to_playlist(playlist_handle::playing, handle);
                } else {
                    g_ui.remove_atmosphere_from_playlist(playlist_handle::playing, handle);
                }
            };
        };
    }

    void ui::create_new_atmosphere(playlist_handle_t playlistHandle)
    {
        constexpr static auto* defaultTitle = "New Mix";
        worker(Worker_Background) << [=]()
        {
            atmosphere_entry atmosphereEntry =
            {
                worker(Worker_Background).nextUUID(),
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

            worker(Worker_UI) << [=]()
            {
                atmosphere_widget atmosphereWidget;
                atmosphereWidget.title.text = defaultTitle;
                atmosphereWidget.atmosphereHandle = atmosphereHandle;

                MX_LOG_INFO("title : {}", atmosphereWidget.title.text);

                // this atmosphere should not exist in our map yet
                MX_ASSERT(g_ui.root.playlist.atmospheres.find(atmosphereHandle) == g_ui.root.playlist.atmospheres.end());
                g_ui.root.playlist.atmospheres.insert({atmosphereHandle, std::move(atmosphereWidget)});

                // add handle to our playlists
                if (playlistHandle != playlist_handle::all) {
                    g_ui.add_atmosphere_to_playlist(playlist_handle::all, atmosphereHandle);
                }
                g_ui.add_atmosphere_to_playlist(playlistHandle, atmosphereHandle);
            };
        };
    }

    void ui::do_audio_search(const utf8_string& query)
    {
        worker(Worker_Background) << [=]() -> void
        {
            auto audioHandles = g_audio_directory.get_audio_handles_by_query(query);
            std::vector<audio_query_result_widget> queryWidgets;
            for(auto handle : audioHandles) {
                const auto& entry = g_audio_directory.get_entry(handle);
                queryWidgets.push_back({handle, {entry.title}});
            }

            worker(Worker_UI) << [queryWidgets=std::move(queryWidgets)]() mutable -> void
            {
                g_ui.root.edit.search.queryResults = std::move(queryWidgets);
            };
        };
    }

    /// Draw Methods

    void ui::draw_sidebar(float dt)
    {
        mxVScroll(&root.sidebar.scroll, self.tags = {"sidebar"})
        {
            mxSpacer(self.requestHeight(100.0));
            mxButton(&root.sidebar.playlists, self.tags = {"mediumText", "sidebarEntry"});
            mxButton(&root.sidebar.newPlaylist, self.tags = {"lightText", "sidebarEntry", "indent"};
                self.on_click = []() -> void {MX_LOG_INFO("NEW PLAYLIST");});
            for( size_t i = 0; i < root.playlist.playlists.size(); ++i)
            {
                auto& currentPlaylist = root.playlist.playlists[i];
                mxButton(&currentPlaylist.title, self.tags = {"lightText","sidebarEntry", "indent"};
                    self.on_click = [=]() -> void
                    {
                        worker(Worker_UI) << [=]() -> void
                        {
                            g_ui.root.currentScreen = screen::playlist;
                            g_ui.root.playlist.currentPlaylistHandle = currentPlaylist.handle;
                        };
                    };
                );
            }
            mxButton(&root.sidebar.edit, self.tags = {"mediumText", "sidebarEntry"};
                self.on_click = []() -> void
                {
                    MX_LOG_INFO("EDIT");
                    worker(Worker_UI) << []() -> void
                    {
                        g_ui.root.currentScreen = screen::edit;
                    };
                });
            mxButton(&root.sidebar.about, self.tags = {"mediumText", "sidebarEntry"};
                self.on_click = []() -> void
                {
                    MX_LOG_INFO("ABOUT");
                    worker(Worker_UI) << []() -> void
                    {
                        g_ui.root.currentScreen = screen::about;
                    };
                });
        }
    }

    void ui::draw_playlist(float dt)
    {
        // skip drawing if no playlists
        if (root.playlist.playlists.size() == 0) {
            return;
        }

        auto& currentPlaylist = root.playlist.currentPlaylist();
        mxVBox(&root.playlist.vflex, self.tags = {"content"})
        {
            mxLabel(&root.playlist.heading, self.tags= {"header", "darkText"});
            mxLabel(&currentPlaylist.title, self.tags = {"header", "title", "lightText"});
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
                    g_ui.create_new_atmosphere(currentPlaylist.handle);
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
                for (auto& currentAtmosphereHandle : currentPlaylist.entries) {
                    MX_ASSERT(root.playlist.atmospheres.find(currentAtmosphereHandle) != root.playlist.atmospheres.end());
                    auto& currentEntry = root.playlist.atmospheres[currentAtmosphereHandle];

                    mxHBox(&root.playlist.rowFlex, self.tags={"playlistEntry"})
                    {
                        mxCheckbox(&currentEntry.favorite, self.tags = {"entryFavorite"};
                        self.on_change = [=](bool favorited) -> void
                        {
                            g_ui.set_atmosphere_favorite(currentAtmosphereHandle, favorited);
                        }); // FAVORITE
                        mxLabel(&currentEntry.title, self.tags = {"entryTitle"});
                        mxSlider(&currentEntry.volume, self.tags = {"entryVolume"};
                        self.on_change = [&](double volume) -> void
                        {
                            g_ui.set_atmosphere_volume(currentEntry, currentAtmosphereHandle, volume);
                        }); // VOLUME
                        mxCheckbox(&currentEntry.muteUnmute, self.tags = {"entryMute"};
                        self.on_change = [&](bool muted) -> void
                        {
                            g_ui.set_atmosphere_mute(currentEntry, currentAtmosphereHandle, muted);
                        }); // MUTE
                        mxCheckbox(&currentEntry.playStop, self.tags = {"entryPlay"};
                        self.on_change = [=](bool playing) -> void
                        {
                            g_ui.set_atmosphere_play(currentAtmosphereHandle, playing);
                        }); // PLAY/STOP BUTTON
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
                    g_ui.do_audio_search(query);
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
                        mxButton(&queryResult.title, self.tags={"audioResult"};
                        self.on_click = [&]() -> void
                        {
                            MX_LOG_INFO("CLICKED : {}, {}", queryResult.title.text, queryResult.handle);
                        });
                        mxSpacer();
                    }
                }
            }
        }
    }

}
