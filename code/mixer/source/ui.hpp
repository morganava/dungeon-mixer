#pragma once

#include "singleton.hpp"

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
                imgui::label_data playlists = {"Playlists"};
                imgui::button_data newPlaylist = {"New Playlist"};
                imgui::label_data edit = {"Edit"};
                imgui::label_data about = {"About"};
            } sidebar;

            struct
            {

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
}