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

        std::unique_ptr<font_stash> font_atlas;
    };

    struct ui_data
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

    };
    extern ui_data* g_ui;

    void init_draw(nk_context* ctx);
    void free_draw();
	void update_draw(nk_context* ctx, int width, int height);

    class renderer
    {

    };

    // renderer functions
    renderer* get_renderer();
    void init_renderer();
    void free_renderer();
}