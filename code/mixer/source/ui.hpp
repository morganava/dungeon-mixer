#pragma once

#include "singleton.hpp"
#include "audio.hpp"

namespace tabletopify
{
    class ui
    {
    public:
        void update(float dt, int32_t width, int32_t height);
    private:

        void draw_debug(float dt) {}
        void draw_playlist(float dt);
        void draw_edit(float dt) {}
        void draw_about(float dt) {}
        void draw_sidebar(float dt);

        enum class Screen
        {
            Debug = -1,
            Playlist,
            Edit,
            About
        } currentScreen = Screen::Playlist;
        typedef enum Screen Screen_t;

        struct playlist_id
        {
            enum value
            {
                starred = 0,
                playing,
                all,
            };
        };
        typedef playlist_id::value playlist_id_t;

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

                struct playlist_data
                {
                    imgui::label_data title;
                    struct playlist_entry
                    {
                        imgui::checkbox_data favorite = {false};
                        imgui::label_data title;
                        imgui::slider_data volume = {0.0, 1.0, 0.5};
                        float previousVolume;
                        bool muted = false;
                        imgui::checkbox_data muteUnmute = {false};
                        imgui::button_data playStop;

                        atmosphere_handle_t atmosphereHandle = MX_INVALID_HANDLE;
                    };
                    std::vector<playlist_entry> entries;
                };
                playlist_id_t currentPlaylistId = playlist_id::all;
                std::vector<playlist_data> playlists =
                {
                    // starred
                    {{"Favorites"}, {}},
                    // playing
                    {{"Playing"}, {}},
                    // all
                    {{"All"},
                        {
                            {false, {"Moo Cows"}},
                            {false, {"Rainy Forest"}},
                            {false, {"Marketplace"}},
                        }
                    },
                };

                playlist_data& currentPlaylist() {
                    return playlists[currentPlaylistId];
                }

            } playlist;
            struct
            {

            } edit;
        } root;
    };

    constexpr ui* get_ui()
    {
        return singleton<ui>::addr();
    }

    inline void init_ui()
    {
        singleton<ui>::construct();
    }

    constexpr void free_ui()
    {
        singleton<ui>::free();
    }
    inline constexpr ui& g_ui = singleton<ui>::get();
}