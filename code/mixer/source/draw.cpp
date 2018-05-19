#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "nuklear_extensions.hpp"
#include "serialization.hpp"

namespace tabletopify
{
    static renderer* g_renderer = nullptr;
    renderer* get_renderer()
    {
        MX_ASSERT(worker(Worker_UI).getThreadId() == std::this_thread::get_id());
        return g_renderer;
    }
    void init_renderer()
    {
        MX_ASSERT(g_renderer == nullptr);
        g_renderer = new renderer();
    }
    void free_renderer()
    {
        delete g_renderer;
        g_renderer = nullptr;
    }

    static style_data style;
    static ui_data ui;
    ui_data* g_ui = &ui;

    typedef enum
    {
        STR_PLAYLISTS,
        STR_CREATE_PLAYLIST,
        STR_ALL_PLAYLIST,
        STR_EDIT_ATMOSPHERE,
        STR_ABOUT,
        STR_PLAYLIST,
        STR_NEW_ATMOSPHERE,
        STR_SEARCH,
        STR_SETTINGS,

        STR_COUNT,
    } mixer_string_t;

    const char* get_string(mixer_string_t str)
    {
        const char* english_strings[] =
        {
            "PLAYLISTS",
            "+CREATE",
            "Long Playlist Name HAHHA",
            "EDIT",
            "ABOUT",
            "PLAYLIST",
            "NEW ATMOSPHERE",
            "SEARCH",
            "SETTINGS",
        };

        MX_STATIC_ASSERT(countof(english_strings) == STR_COUNT);

        return english_strings[str];
    }

    void load_style(nk_context* ctx)
    {
        auto skinPath = get_skins_path();

        // this would be part of style json loading
        style.font_atlas = std::make_unique<font_stash>();
        auto& font_atlas = style.font_atlas;
        font_atlas->begin();

        style.sidebar.width = 196;
        style.sidebar.indent = 16;
        style.sidebar.font = font_atlas->add(skinPath / "DejaVuSans.ttf", 16.0f);
        nk_style_set_font(ctx, style.sidebar.font);

        style.edit.new_atmosphere_button_width = 64;

        style.viewport.title_font = font_atlas->add(skinPath / "DejaVuSans.ttf", 96.0f);
        style.viewport.heading_font = font_atlas->add(skinPath / "DejaVuSans.ttf", 16.0f);
        style.viewport.title_height = 96.0f;

        style.viewport.search.pane_width = 256;
        style.viewport.search.button_width = 32;

        font_atlas->end();
    }

    void init_draw(nk_context* ctx)
    {
        // load user stye
        load_style(ctx);

         // set nk style
        ctx->style.window.border = 0.0f;
        ctx->style.window.group_border = 0.0f;
        ctx->style.window.padding = nk_vec2(0, 0);
        ctx->style.window.group_padding = nk_vec2(0, 0);

        ctx->style.window.scrollbar_size = nk_vec2(10, 0);
        ctx->style.window.spacing = nk_vec2(0, 0);

        ctx->style.button.padding = nk_vec2(0, 0);
        ctx->style.button.border = 0.0f;

        ctx->style.scrollh.padding = nk_vec2(0, 0);
        ctx->style.scrollh.border = 0.0f;

        // dummy 'All' playlislt
        ui.sidebar.playlists.push_back({
            get_string(STR_ALL_PLAYLIST),
            uuid::null
        });
    }

    void free_draw()
    {
        style.font_atlas.reset(nullptr);
    }

    static void draw_sidebar(nk_context* ctx)
    {
        if(nk_group_begin(ctx, "sidebar", 0))
        {
            nk_style_set_font(ctx, style.sidebar.font);

            // playlist button
            nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 1);
            nk_layout_row_push(ctx, 1.0f);
            nk_label(ctx, get_string(STR_PLAYLISTS), NK_TEXT_LEFT);

            // create playlist button
            nk_layout_row_template_begin(ctx, 0);
            nk_layout_row_template_push_static(ctx, style.sidebar.indent);
            nk_layout_row_template_push_dynamic(ctx);
            nk_layout_row_template_end(ctx);

            nk_spacing(ctx, 1);
            nk_label(ctx, get_string(STR_CREATE_PLAYLIST), NK_TEXT_LEFT);

            for(const auto& pl : ui.sidebar.playlists)
            {
                nk_spacing(ctx, 1);
                nk_label(ctx, pl.title.data(), NK_TEXT_LEFT);
            }

            // edit button
            nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 1);
            nk_layout_row_push(ctx, 1.0f);
            nk_label(ctx, get_string(STR_EDIT_ATMOSPHERE), NK_TEXT_LEFT);

            // settings button
            nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 1);
            nk_layout_row_push(ctx, 1.0f);
            nk_label(ctx, get_string(STR_SETTINGS), NK_TEXT_LEFT);

           // about button
            nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 1);
            nk_layout_row_push(ctx, 1.0f);
            nk_label(ctx, get_string(STR_ABOUT), NK_TEXT_LEFT);


            nk_group_end(ctx);
        }
    }

    static void draw_playlist(nk_context* ctx);
    static void draw_edit(nk_context* ctx);
    static void draw_about(nk_context* ctx);

    static void draw_viewport(nk_context* ctx)
    {
        if(nk_group_begin(ctx, "viewport", 0))
        {
            switch(ui.mode)
            {
                case MX_MODE_VIEW_PLAYLIST:
                    draw_playlist(ctx);
                    break;
                case MX_MODE_EDIT_ATMOSPHERE:
                    draw_edit(ctx);
                    break;
                case MX_MODE_ABOUT:
                    draw_about(ctx);
                    break;
            }
            nk_group_end(ctx);
        }
    }

    static void draw_playlist(nk_context* ctx)
    {
        nk_style_set_font(ctx, style.viewport.heading_font);

        // Playlist label
        nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 1);
        nk_layout_row_push(ctx, 1.0f);
        nk_label(ctx, get_string(STR_PLAYLIST), NK_TEXT_LEFT);

        // Playlist title, and search bar
        auto& playlist = ui.sidebar.get_selected_playlist();

        nk_layout_row_template_begin(ctx, style.viewport.title_height);
        nk_layout_row_template_push_dynamic(ctx);
        const auto not_title_width =
            style.edit.new_atmosphere_button_width +
            style.viewport.search.pane_width +
            style.viewport.search.button_width;
        nk_layout_row_template_push_static(ctx, not_title_width);
        nk_layout_row_template_end(ctx);

#if 0
        nk_layout_row_template_push_static(ctx,
            style.edit.new_atmosphere_button_width);
        nk_layout_row_template_push_static(ctx,
            style.viewport.search.pane_width);
        nk_layout_row_template_push_static(ctx,
            style.viewport.search.button_width);
        nk_layout_row_template_end(ctx);
#endif        
        {
            nk_style_set_font(ctx, style.viewport.title_font);

            // Playlist title
            nk_label(ctx, playlist.title.data(), NK_TEXT_LEFT);

            nk_style_set_font(ctx, style.viewport.heading_font);

            // New Atmosphere button
            // nk_button_label(ctx, get_string(STR_NEW_ATMOSPHERE));

            // nk_spacing(ctx, 1);

            nk_button_label(ctx, get_string(STR_SEARCH));
        }
    }

    static void draw_edit(nk_context* ctx)
    {

    }

    static void draw_about(nk_context* ctx)
    {

    }

    void update_draw(nk_context* ctx, int window_width, int window_height)
    {
        if(nk_begin(ctx, "main", nk_rect(0, 0, window_width, window_height), NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_template_begin(ctx, window_height);
            nk_layout_row_template_push_static(ctx, style.sidebar.width);
            nk_layout_row_template_push_dynamic(ctx);
            nk_layout_row_template_end(ctx);

            draw_sidebar(ctx);
            draw_viewport(ctx);

            nk_end(ctx);
        }
    }
}