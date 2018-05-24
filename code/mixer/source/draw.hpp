#pragma once

#include "text_buffer.hpp"
#include "font_stash.hpp"

namespace tabletopify
{
    typedef enum
    {
        MX_MODE_VIEW_PLAYLIST,
        MX_MODE_EDIT_ATMOSPHERE,
        MX_MODE_ABOUT
    } mode_t;

    typedef enum
    {
        MX_UNMUTED,
        MX_MUTED,
    } muted_t;

    typedef enum
    {
        MX_STOPPED,
        MX_PLAYING,
    } playstate_t;

    class renderer
    {
    public:
        renderer(nk_context* ctx);
        void load_style();
        void update(float dt, int32_t width, int32_t height);
        void draw();
        void draw_sidebar();
        void draw_viewport();
        void draw_playlist();
        void draw_edit();
        void draw_about();

    private:
        nk_context* _ctx;

        struct style_data
        {
            struct
            {
                nk_user_font* edit_string_font;
            }
            shared;

            struct
            {
                int32_t width;
                int32_t indent;
                nk_user_font* section_font;
                float section_height;
            }
            sidebar;

            struct
            {
                struct
                {
                    nk_user_font* title_font;
                    float title_height;

                    nk_user_font* heading_font;
                    float heading_height;

                    float search_height;
                    float search_field_width;
                    float search_button_width;
                    struct nk_image search_button_icon;

                    float topbar_padding;

                    nk_user_font* context_menu_font;
                    float context_menu_height;
                    float context_menu_height;
                } shared;

                struct
                {
                    nk_user_font* new_atmosphere_font;
                    float new_atmosphere_button_width;

                    nk_user_font* header_font;
                    float header_height;

                    nk_user_font* row_font;
                    float row_height;

                    float volume_width;
                    float mute_width;
                    struct nk_image muted_button_icon;
                    struct nk_image unmuted_button_icon;
                    float play_stop_width;
                    struct nk_image stop_button_icon;
                    struct nk_image play_button_icon;
                } playlist;

                struct
                {
                    

                } edit;
            }
            viewport;

            font_stash font_atlas;
        } _style;

        struct draw_data
        {
            struct
            {
                int32_t width;
                int32_t height;
            } window;

            struct
            {
                struct playlist_data
                {
                    std::string title;
                    uuid id;
                    text_buffer searchQuery;
                };
                std::vector<playlist_data> playlists;
                size_t selectedPlaylist = 0;
                playlist_data& get_selected_playlist()
                {
                    return playlists[selectedPlaylist];
                }
            }
            sidebar;

            struct
            {
                struct
                {
                    struct atmosphere_data
                    {
                        std::string title;
                        uuid id;
                        float volume;
                        muted_t muted;
                        playstate_t playing;
                    };

                    std::vector<atmosphere_data> atmospheres;
                } playlist;
            } viewport;

            mode_t mode = MX_MODE_VIEW_PLAYLIST;
        } _data;
    };

    // renderer functions
    renderer* get_renderer();
    void init_renderer(nk_context* ctx);
    void free_renderer();
}