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
                nk_user_font* font;
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
                } shared;

                struct
                {
                    float new_atmosphere_button_width;
                } playlist;

                struct
                {
                    nk_user_font* new_atmosphere_font;
                    int new_atmosphere_button_width;

                } edit;
            }
            viewport;

            font_stash font_atlas;
        } _style;

        struct
        {
            struct
            {
                int32_t width;
                int32_t height;
            } window;

            struct
            {
                struct atmosphere_data
                {

                };

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

            } viewport;

            mode_t mode = MX_MODE_VIEW_PLAYLIST;
        } _data;
    };

    // renderer functions
    renderer* get_renderer();
    void init_renderer(nk_context* ctx);
    void free_renderer();
}