#pragma once

#include "singleton.hpp"
#include "imgui.hpp"
#include "audio.hpp"
#include "enums.hpp"

namespace tabletopify
{
    MX_MAKE_ENUM(screen, uint_fast8_t,
        playlist,
        edit,
        about
    );

    struct atmosphere_widget
    {
        imgui::checkbox_data favorite = {false};
        imgui::label_data title;
        imgui::slider_data volume = {0.0, 1.0, 0.0};
        // duplicate value for toggling mute
        double volumeVal = 0.0;
        imgui::checkbox_data muteUnmute = {false};
        imgui::checkbox_data playStop = {false};

        atmosphere_handle_t atmosphereHandle = atmosphere_handle::invalid;
    };

    struct playlist_widget
    {
        playlist_handle_t handle;
        imgui::label_data title;

        std::vector<atmosphere_handle_t> entries;
    };

    struct audio_query_result_widget
    {
        audio_handle_t handle;
        imgui::button_data title;
    };

    class ui
    {
    public:
        void update(float dt, int32_t width, int32_t height);

        // populates backend playlist data to ui
        void sync_playlists();
    private:
        playlist_widget& get_playlist_widget(playlist_handle_t handle);
        void remove_atmosphere_from_playlist(playlist_handle_t playlist, atmosphere_handle_t atmosphere);
        void add_atmosphere_to_playlist(playlist_handle_t playlist, atmosphere_handle_t atmosphere);

        // playlist screen methods
        void set_atmosphere_favorite(atmosphere_handle_t handle, bool favorited);
        void set_atmosphere_volume(atmosphere_widget& widget, atmosphere_handle_t handle, double volume);
        void set_atmosphere_mute(atmosphere_widget& widget, atmosphere_handle_t handle, bool muted);
        void set_atmosphere_play(atmosphere_handle_t handle, bool playing);
        void create_new_atmosphere(playlist_handle_t handle);

        // edit screen methods
        void do_audio_search(const utf8_string& query);

        // TODO: nuke this
        friend class playlist_directory;

        // draw methods
        void draw_playlist(float dt);
        void draw_edit(float dt);
        void draw_about(float dt) {}
        void draw_sidebar(float dt);

        struct
        {
            imgui::flex_box_data flex =
            {
                constraint{0.0, unconstrained, 1.0},
                constraint{0.0, unconstrained, 4.0},
            };

            struct
            {
                imgui::scroll_area_data scroll;
                imgui::button_data playlists = {"Playlists"};
                imgui::button_data newPlaylist = {"New Playlist"};
                imgui::button_data starred = {"Starred"};
                imgui::button_data playing = {"Playing"};
                imgui::button_data all = {"All"};
                imgui::button_data edit = {"Edit"};
                imgui::button_data about = {"About"};
            } sidebar;

            screen_t currentScreen = screen::playlist;

            struct
            {
                imgui::flex_box_data vflex =
                {
                    constraint{35, 35, 1.0},
                    constraint{70, 70, 1.0},
                    constraint{50, 50, 1.0},
                    constraint{0.0, unconstrained, 1.0},
                };
                imgui::label_data heading = {"Playlist"};
                imgui::label_data title = {"All"};
                imgui::flex_box_data buttonFlex =
                {
                    constraint{128,128, 2.0},
                    constraint{128,128, 2.0},
                    constraint{0, unconstrained, 2.0},
                    constraint{128,128, 2.0},
                    constraint{256,unconstrained, 1.0},
                    constraint{32,32, 1.0},
                };
                imgui::button_data export_ = {"Export"};
                imgui::button_data import = {"Import"};
                imgui::button_data newMix = {"New Mix"};
                imgui::text_box_data searchBox = {"", "Search Box"};
                imgui::button_data searchButton = {};

                imgui::scroll_area_data scroll;
                imgui::flex_box_data rowFlex =
                {
                    constraint{64,64,1.0},
                    constraint{256,unconstrained,2.0},
                    constraint{256,256,1.0},
                    constraint{64,64,1.0},
                    constraint{64,64,1.0},
                };

                imgui::label_data trackTitle = {"Title"};
                imgui::label_data trackVolume = {"Volume"};
                imgui::slider_data slider = {0.0f, 100.0f, 50.0f};

                playlist_handle_t currentPlaylistHandle = playlist_handle::invalid;
                size_t currentPlaylistIndex = -1;

                std::vector<playlist_widget> playlists;
                std::map<atmosphere_handle_t, atmosphere_widget> atmospheres;

                playlist_widget& currentPlaylist()
                {
                    for(auto& currentWidget : playlists) {
                        if (currentWidget.handle == currentPlaylistHandle) {
                            return currentWidget;
                        }
                    }

                    // fallback to 'all'
                    MX_ASSERT(playlists.front().handle == playlist_handle::all);
                    return playlists.front();
                }
            } playlist;
            struct
            {
                imgui::flex_box_data vflex =
                {
                    constraint{35, 35, 1.0},
                    constraint{70, 70, 1.0},
                    constraint{50, 50, 1.0},
                    constraint{0.0, unconstrained, 1.0},
                };
                imgui::label_data heading = {"Edit"};
                imgui::button_data title = {"New Mix"};
                imgui::flex_box_data hflex =
                {
                    constraint{0, unconstrained, 2.0},
                    constraint{0,unconstrained, 1.0},
                };
                imgui::text_box_data searchBox = {"", "Track Search"};
                imgui::button_data searchButton = {};

                struct
                {
                    imgui::scroll_area_data scroll;
                } tracks;

                struct
                {
                    imgui::scroll_area_data scroll;
                    std::vector<audio_query_result_widget> queryResults;
                } search;

            } edit;

            imgui::context_menu_data contextMenu;
            imgui::scroll_area_data contextMenuScroll;
        } root;
    };
    MX_MAKE_SINGLETON(ui)
}