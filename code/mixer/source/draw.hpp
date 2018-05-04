#pragma once

#include "text_buffer.hpp"

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
        }
        sidebar;

        struct
        {
            int new_atmosphere_button_width;

        }
        edit;

        struct
        {
            struct
            {
                int pane_width;
                int button_width;
            }
            search;
        }
        viewport;
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

    void draw_init(nk_context* ctx);
	void draw_update(nk_context* ctx, int width, int height);
}