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
        void draw(int32_t width, int32_t height);
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
                int width;
                int indent;
                nk_user_font* font;
            }
            sidebar;

            struct
            {
                int new_atmosphere_button_width;

            }
            edit;

            struct
            {
                nk_user_font* title_font;
                float title_height;
                nk_user_font* heading_font;

                struct
                {
                    int pane_width;
                    int button_width;
                }
                search;
            }
            viewport;

            font_stash font_atlas;
        } _style;

        struct
        {
            struct
            {
                struct playlist
                {
                    std::string title;
                    uuid id;
                    text_buffer searchQuery;
                };
                std::vector<playlist> playlists;
                size_t selectedPlaylist = 0;
                const playlist& get_selected_playlist() const
                {
                    return playlists[selectedPlaylist];
                }
            }
            sidebar;

            mode_t mode = MX_MODE_VIEW_PLAYLIST;
        } _data;
    };

    // renderer functions
    renderer* get_renderer();
    void init_renderer(nk_context* ctx);
    void free_renderer();
}