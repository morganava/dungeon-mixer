#include "utf8_string.hpp"
#include "utilities.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "singleton.hpp"
#include "hash.hpp"
#include "nanovg.hpp"
#include "imgui.hpp"
#include "ui.hpp"
#include "singleton.hpp"
#include "linear_constraint_solver.hpp"
#include "array.hpp"

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
                    mxHBox(&root.playlist.rowFlex)
                    {
                        mxSpacer(); // FAVORITE
                        mxLabel(&currentEntry.title, self.tags = {"entryTitle"});
                        mxSlider(&currentEntry.volume, self.tags = {"entryVolume"};
                            self.on_change = [](double val) -> void { MX_LOG_VAR(val);};
                        );
                        mxSpacer(); // MUTE TOGGLE
                        mxSpacer(); // PLAY/STOP BUTTON
                    }
                }
            }
        }
    }
}
