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
                switch(this->currentScreen)
                {
                    case Screen::Debug:
                        draw_debug(dt); break;
                    case Screen::Playlist:
                        draw_playlist(dt); break;
                    case Screen::Edit:
                        draw_edit(dt); break;
                    case Screen::About:
                        draw_about(dt); break;
                }
            }
        }
    }

    void ui::populate_playlist_data()
    {
        worker(Worker_Background) << []() -> void {
            // TODO: replace this with singleton
            auto& g_atmosphere_directory = *get_atmosphere_directory();

            std::vector<ui::playlist_widget> playlists;
            for (auto playlistHandle : g_playlist_directory.get_playlist_handles())
            {
                auto& playlistEntry = g_playlist_directory.get_entry(playlistHandle);
                ui::playlist_widget currentPlaylistWidget;

                currentPlaylistWidget.title.text = playlistEntry.title;
                for (auto atmosphereHandle : playlistEntry.atmospheres)
                {
                    const auto& atmosphereEntry = g_atmosphere_directory.get_entry(atmosphereHandle);
                    utf8_string atmosphereEntry_title = atmosphereEntry.title;

                    playlist_widget::atmosphere_widget currentAtmosphereWidget;
					// TODO: store in atmosphereEntry
                    currentAtmosphereWidget.favorite = {false};
                    currentAtmosphereWidget.title = imgui::label_data{atmosphereEntry.title};
					// TODO: store in atmosphereEntry
                    currentAtmosphereWidget.volume = {0.0, 1.0, 0.5};
					// TODO: stor ein atmosphereEntry
                    currentAtmosphereWidget.muteUnmute = {false};
                    currentAtmosphereWidget.atmosphereHandle = atmosphereHandle;
                    MX_LOG_VAR(atmosphereHandle);
                    currentPlaylistWidget.entries.push_back(std::move(currentAtmosphereWidget));
                }
                playlists.emplace_back(std::move(currentPlaylistWidget));
            }

            worker(Worker_UI) << [playlists = std::move(playlists)]() -> void
            {
                MX_LOG_VAR(playlists.size());
                for(auto currentPlaylist : playlists) {
                    MX_LOG_VAR(currentPlaylist.title.text);
                }

                g_ui.root.playlist.playlists = std::move(playlists);
                g_ui.root.playlist.currentPlaylistId = playlist_id::all;

                MX_LOG_INFO("PLaylist Populated!");
            };
        };
    }

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
                    self.on_click = [&]() -> void
                    {
                        MX_LOG_INFO("{}", currentPlaylist.title.text);
                        worker(Worker_UI).enqueue([=]() -> void
                        {
                            root.playlist.currentPlaylistId = static_cast<playlist_id_t>(i);
                        });
                    };

                );
            }
            mxButton(&root.sidebar.edit, self.tags = {"mediumText", "sidebarEntry"};
                self.on_click = []() -> void {MX_LOG_INFO("EDIT");});
            mxButton(&root.sidebar.about, self.tags = {"mediumText", "sidebarEntry"};
                self.on_click = []() -> void {MX_LOG_INFO("ABOUT");});
        }
    }

    void ui::draw_playlist(float dt)
    {
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
                    self.on_click = []() -> void {MX_LOG_INFO("NEW MIX");};);
                mxTextBox(&root.playlist.searchBox, self.tags = {"searchBox"};
                    self.on_submit = [](const utf8_string& str) -> void
                    {
                        MX_LOG_INFO("SUBMIT : {}", str);
                    };
                );
                mxButton(&root.playlist.searchButton,
                    self.on_click = []() -> void {MX_LOG_INFO("SEARCH");};);
            }
            mxVScroll(&root.playlist.scroll)
            {
                for (auto& currentEntry : currentPlaylist.entries) {
                    mxHBox(&root.playlist.rowFlex, self.tags={"playlistEntry"})
                    {
                        mxCheckbox(&currentEntry.favorite, self.tags = {"entryFavorite"};
                        self.on_change = [](bool checked) -> void
                        {
                            MX_LOG_VAR(checked);
                        }); // FAVORITE
                        mxLabel(&currentEntry.title, self.tags = {"entryTitle"});
                        mxSlider(&currentEntry.volume, self.tags = {"entryVolume"};
                            self.on_change = [](double val) -> void { MX_LOG_VAR(val);};
                        );
                        mxCheckbox(&currentEntry.muteUnmute, self.tags = {"entryMute"}); // MUTE TOGGLE
                        mxCheckbox(&currentEntry.playStop, self.tags = {"entryPlay"};
                        self.on_change = [&](bool checked) -> void
                        {
                            worker(Worker_Background) << [=]() {
                                // TODO: singletonize
                                auto& g_playback_manager = *get_playback_manager();
                                const auto handle = currentEntry.atmosphereHandle;
                                if (checked) {
                                    g_playback_manager.play_atmosphere(handle);
                                } else {
                                    g_playback_manager.stop_atmosphere(handle);
                                }

                            };
                        }); // PLAY/STOP BUTTON
                    }
                }
            }
        }
    }
}
