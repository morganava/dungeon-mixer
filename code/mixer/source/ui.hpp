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
        imgui::button_data title;
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
        utf8_string title;
        imgui::text_box_data title_textbox;

        std::vector<atmosphere_handle_t> entries;
    };

    struct track_edit_widget
    {
        track_handle_t handle;
        imgui::checkbox_data visible = {false};
        utf8_string audio_title;
        imgui::slider_data volume_slider = {0.0, 1.0, 0.0};
    };

    struct audio_query_result_widget
    {
        audio_handle_t handle;
        utf8_string title;
    };

    class ui
    {
    public:
        void update(float dt, int32_t width, int32_t height);

        // populates backend playlist data to ui
        void sync_playlists();
    private:
        // ui mutator methods
        void set_screen(screen_t screen);
        playlist_widget& get_playlist_widget(playlist_handle_t handle);
        playlist_widget& get_current_playlist_widget();
        void remove_atmosphere_from_playlist(playlist_handle_t playlist, atmosphere_handle_t atmosphere);
        void add_atmosphere_to_playlist(playlist_handle_t playlist, atmosphere_handle_t atmosphere);
        void hide_atmosphere_context_menu();
        void stop_edit_playback();

        // sidebar methods
        void on_create_new_playlist();
        void on_set_screen_playlist(playlist_handle_t handle);
        void on_set_screen_edit(atmosphere_handle_t handle);
        void on_set_screen_about();

        // playlist screen methods
        void on_start_playlist_rename(playlist_handle_t playlist);
        void on_set_playlist_name(playlist_handle_t playlist, utf8_string name);
        void on_set_atmosphere_favorite(atmosphere_handle_t handle, bool favorited);
        void on_set_atmosphere_volume(atmosphere_widget& widget, atmosphere_handle_t handle, double volume);
        void on_set_atmosphere_mute(atmosphere_widget& widget, atmosphere_handle_t handle, bool muted);
        void on_set_atmosphere_play(atmosphere_handle_t handle, bool playing);
        void on_create_new_atmosphere(playlist_handle_t handle);
        void on_toggle_atmosphere_context_menu(playlist_handle_t playlistHandle, atmosphere_handle_t atmosphereHandle);
        void on_remove_atmosphere(playlist_handle_t playlistHandle, atmosphere_handle_t atmosphereHandle);
        void on_open_atmosphere_copy_menu();
        void on_copy_atmosphere(playlist_handle_t playlist, atmosphere_handle_t atmosphere);
        void on_open_atmosphere_move_menu();
        void on_move_atmosphere(playlist_handle_t playlistSrc, playlist_handle_t playlistDest, atmosphere_handle_t atmosphere);
        void on_delete_atmosphere(atmosphere_handle_t atmosphere);

        // edit screen methods
        void on_do_audio_search(const utf8_string& query);
        void on_add_audio_to_atmosphere(atmosphere_handle_t atmosphereHandle, audio_handle_t audioHandle);
        void on_set_track_volume(track_handle_t trackHandle, double volume);

        // draw methods
        void draw_playlist(float dt);
        void draw_edit(float dt);
        void draw_about(float dt) {}
        void draw_sidebar(float dt);

        struct
        {
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
                imgui::label_data heading = {"Playlist"};
                bool edit_title = false;
                imgui::button_data export_ = {"Export"};
                imgui::button_data import = {"Import"};
                imgui::button_data newMix = {"New Mix"};
                imgui::text_box_data searchBox = {"", "Search Box"};
                imgui::button_data searchButton = {};

                imgui::scroll_area_data scroll;
                imgui::label_data trackTitle = {"Title"};
                imgui::label_data trackVolume = {"Volume"};
                imgui::slider_data slider = {0.0f, 100.0f, 50.0f};

                playlist_handle_t currentPlaylistHandle = playlist_handle::all;

                std::vector<playlist_widget> playlists;
                std::map<atmosphere_handle_t, atmosphere_widget> atmospheres;

                struct {
                    atmosphere_handle_t atmosphereHandle = atmosphere_handle::invalid;
                    bool visible = false;
                    bool copyVisible = false;
                    bool moveVisible = false;
                } atmosphereContextMenu;

            } playlist;
            struct
            {
                atmosphere_handle_t handle = atmosphere_handle::invalid;
                imgui::label_data heading = {"Edit"};
                imgui::button_data title = {"New Mix"};
                imgui::text_box_data searchBox = {"", "Track Search"};
                imgui::button_data searchButton = {};

                struct
                {
                    imgui::scroll_area_data scroll;
                    utf8_string title;
                    std::vector<track_edit_widget> widgets;
                } tracks;

                struct
                {
                    imgui::scroll_area_data scroll;
                    std::vector<audio_query_result_widget> queryResults;
                } search;

            } edit;
        } root;
    };
    MX_MAKE_SINGLETON(ui)
}