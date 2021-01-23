#pragma once

#include "singleton.hpp"
#include "imgui.hpp"
#include "audio.hpp"
#include "enums.hpp"

namespace dm
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
        audio_type_t type;
        imgui::checkbox_data visible = {true};
        utf8_string title;
        imgui::slider_data volume_slider = {0.0, 1.0, 1.0};
        imgui::checkbox_data position_checkbox = {false};
        // 0 degrees directly forward, -90 directly left, 90 directly right
        imgui::slider_data azimuth_slider = {-180.0, 180.0, 0.0};
        utf8_string azimuth_string = "0.0°";
        imgui::slider_data elevation_slider = {-90, 90.0, 0.0};
        utf8_string elevation_string = "0.0°";
        imgui::checkbox_data repeat_checkbox = {false};
        imgui::range_slider_data repeat_slider = {0.0, 60.0, 0.0, 15.0};
        utf8_string repeat_min_string = "0.0s";
        utf8_string repeat_max_string = "15.0s";

        track_edit_widget(
            track_handle_t handle,
            audio_type_t type,
            utf8_string title)
        : handle(handle)
        , type(type)
        , title(std::move(title))
        { }

        track_edit_widget(
            track_handle_t handle,
            audio_type_t type,
            utf8_string title,
            double volume,
            bool positionEnabled,
            double azimuth,
            double elevation,
            bool repeatEnabled,
            double repeatMin,
            double repeatMax)
        : handle(handle)
        , type(type)
        , visible(false)
        , title(std::move(title))
        , volume_slider(0.0, 1.0, volume)
        , position_checkbox(positionEnabled)
        , azimuth_slider({-180, 180, azimuth})
        , azimuth_string(fmt::format("{:.1f}°", azimuth))
        , elevation_slider({-90,90,elevation})
        , elevation_string(fmt::format("{:.1f}°", elevation))
        , repeat_checkbox(repeatEnabled)
        , repeat_slider(0,60,repeatMin,repeatMax)
        , repeat_min_string(fmt::format("{:.1f}s", repeatMin))
        , repeat_max_string(fmt::format("{:.1f}s", repeatMax))
        { }

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
        atmosphere_widget& get_atmosphere_widget(atmosphere_handle_t handle);
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
        void on_start_playlist_export(playlist_handle_t playlist);
        void on_start_playlist_import();
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
        void on_do_atmosphere_search(playlist_handle_t playlist, const utf8_string& query);

        // edit screen methods
        void on_start_atmosphere_rename(atmosphere_handle_t atmosphere);
        void on_set_atmosphere_name(atmosphere_handle_t atmosphere, utf8_string name);
        void on_do_audio_search(const utf8_string& query);
        void on_add_audio_to_atmosphere(atmosphere_handle_t atmosphereHandle, audio_handle_t audioHandle);
        void on_set_track_volume(track_handle_t trackHandle, double volume);
        void on_set_track_position_enabled(track_handle_t trackHandle, bool enabled);
        void on_set_track_azimuth(track_edit_widget& widget, double azimuth);
        void on_set_track_elevation(track_edit_widget& widget, double elevation);
        void on_set_track_repeat_enabled(track_handle_t trackHandle, bool enabled);
        void on_set_track_repeat(track_edit_widget& widget, double minRepeat, double maxRepeat);

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
                bool edit_title = false;
                utf8_string title = "New Mix";
                imgui::text_box_data title_textbox;
                imgui::text_box_data searchBox = {"", "Track Search"};
                imgui::button_data searchButton = {};

                struct
                {
                    imgui::scroll_area_data scroll;
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
