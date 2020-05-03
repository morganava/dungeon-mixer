#pragma once

#include "singleton.hpp"
#include "imgui.hpp"
#include "audio.hpp"

namespace tabletopify
{
    enum class screen : uint_fast8_t
    {
        playlist = 0,
        edit,
        about,
    };
    typedef enum screen screen_t;

    struct atmosphere_widget
    {
        imgui::checkbox_data favorite = {false};
        imgui::label_data title;
        imgui::slider_data volume = {0.0, 1.0, 0.0};
        // duplicate value for toggling mute
        double volumeVal = 0.0;
        imgui::checkbox_data muteUnmute = {false};
        imgui::checkbox_data playStop = {false};

        atmosphere_handle_t atmosphereHandle = MX_INVALID_HANDLE;
    };

    struct playlist_widget
    {
        playlist_handle_t handle;
        imgui::label_data title;

        std::vector<atmosphere_handle_t> entries;
    };

    class ui
    {
    public:
        void update(float dt, int32_t width, int32_t height);

        void populate_playlist_widgets(std::vector<playlist_widget>&& playlistWidgets, std::map<atmosphere_handle_t,atmosphere_widget>&& atmosphereWidgets);

        void sync_playlist(playlist_handle_t playlistHandle, std::vector<atmosphere_handle_t>&& atmosphereHandles);

    private:
        friend class playlist_directory;

        void draw_playlist(float dt);
        void draw_edit(float dt) {}
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
                    constraint{100, 100, 1.0},
                    constraint{0.0, unconstrained, 1.0},
                };
                imgui::label_data heading = {"Playlist"};
                imgui::label_data title = {"All"};
                imgui::flex_box_data buttonFlex =
                {
                    constraint{128,128, 1.0},
                    constraint{128,128, 1.0},
                    constraint{0, unconstrained, 2.0},
                    constraint{128,128, 1.0},
                    constraint{256,unconstrained, 1.0},
                    constraint{32,32, 1.0},
                };
                imgui::button_data export_ = {"Export"};
                imgui::button_data import = {"Import"};
                imgui::button_data newMix = {"New Mix"};
                imgui::text_box_data searchBox = {"", "Search Box"};
                imgui::button_data searchButton = {"[ ]"};

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
                    constraint{100, 100, 1.0},
                    constraint{0.0, unconstrained, 1.0},
                };
            } edit;
        } root;
    };
    MX_MAKE_SINGLETON(ui)
}