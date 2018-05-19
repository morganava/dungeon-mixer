#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "nuklear_extensions.hpp"
#include "serialization.hpp"

namespace tabletopify
{
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

    static renderer* g_renderer = nullptr;
    renderer* get_renderer()
    {
        MX_ASSERT(worker(Worker_UI).getThreadId() == std::this_thread::get_id());
        return g_renderer;
    }
    void init_renderer(nk_context* ctx)
    {
        MX_ASSERT(g_renderer == nullptr);
        g_renderer = new renderer(ctx);
    }
    void free_renderer()
    {
        delete g_renderer;
        g_renderer = nullptr;
    }

    renderer::renderer(nk_context* ctx)
    : _ctx(ctx)
    {
        MX_ASSERT(_ctx != nullptr);

        // load user stye
        load_style();

         // set nk style
        _ctx->style.window.border = 0.0f;
        _ctx->style.window.group_border = 0.0f;
        _ctx->style.window.padding = nk_vec2(0, 0);
        _ctx->style.window.group_padding = nk_vec2(0, 0);

        _ctx->style.window.scrollbar_size = nk_vec2(10, 0);
        _ctx->style.window.spacing = nk_vec2(0, 0);

        _ctx->style.button.padding = nk_vec2(0, 0);
        _ctx->style.button.border = 0.0f;

        _ctx->style.scrollh.padding = nk_vec2(0, 0);
        _ctx->style.scrollh.border = 0.0f;

        // dummy 'All' playlislt
        _data.sidebar.playlists.push_back({
            get_string(STR_ALL_PLAYLIST),
            uuid::null
        });        
    }    

    void renderer::load_style()
    {
        auto skinPath = get_skins_path();

        // this would be part of style json loading
        auto& font_atlas = _style.font_atlas;
        font_atlas.begin();

        _style.sidebar.width = 196;
        _style.sidebar.indent = 16;
        _style.sidebar.font = font_atlas.add(skinPath / "DejaVuSans.ttf", 16.0f);
        nk_style_set_font(_ctx, _style.sidebar.font);

        _style.edit.new_atmosphere_button_width = 64;

        _style.viewport.title_font = font_atlas.add(skinPath / "DejaVuSans.ttf", 96.0f);
        _style.viewport.heading_font = font_atlas.add(skinPath / "DejaVuSans.ttf", 16.0f);
        _style.viewport.title_height = 96.0f;

        _style.viewport.search.pane_width = 256;
        _style.viewport.search.button_width = 32;

        font_atlas.end();
    }

    void renderer::update(float dt, int32_t width, int32_t height)
    {
        draw(width, height);
    }

    void renderer::draw(int32_t width, int32_t height)
    {
        if(nk_begin(_ctx, "main", nk_rect(0, 0, width, height), NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_template_begin(_ctx, height);
            nk_layout_row_template_push_static(_ctx, _style.sidebar.width);
            nk_layout_row_template_push_dynamic(_ctx);
            nk_layout_row_template_end(_ctx);

            draw_sidebar();
            draw_viewport();

            nk_end(_ctx);
        }        
    }

    void renderer::draw_sidebar()
    {
        if(nk_group_begin(_ctx, "sidebar", 0))
        {
            nk_style_set_font(_ctx, _style.sidebar.font);

            // playlist button
            nk_layout_row_begin(_ctx, NK_DYNAMIC, 0, 1);
            nk_layout_row_push(_ctx, 1.0f);
            nk_label(_ctx, get_string(STR_PLAYLISTS), NK_TEXT_LEFT);

            // create playlist button
            nk_layout_row_template_begin(_ctx, 0);
            nk_layout_row_template_push_static(_ctx, _style.sidebar.indent);
            nk_layout_row_template_push_dynamic(_ctx);
            nk_layout_row_template_end(_ctx);

            nk_spacing(_ctx, 1);
            nk_label(_ctx, get_string(STR_CREATE_PLAYLIST), NK_TEXT_LEFT);

            for(const auto& pl : _data.sidebar.playlists)
            {
                nk_spacing(_ctx, 1);
                nk_label(_ctx, pl.title.data(), NK_TEXT_LEFT);
            }

            // edit button
            nk_layout_row_begin(_ctx, NK_DYNAMIC, 0, 1);
            nk_layout_row_push(_ctx, 1.0f);
            nk_label(_ctx, get_string(STR_EDIT_ATMOSPHERE), NK_TEXT_LEFT);

            // settings button
            nk_layout_row_begin(_ctx, NK_DYNAMIC, 0, 1);
            nk_layout_row_push(_ctx, 1.0f);
            nk_label(_ctx, get_string(STR_SETTINGS), NK_TEXT_LEFT);

           // about button
            nk_layout_row_begin(_ctx, NK_DYNAMIC, 0, 1);
            nk_layout_row_push(_ctx, 1.0f);
            nk_label(_ctx, get_string(STR_ABOUT), NK_TEXT_LEFT);


            nk_group_end(_ctx);
        }
    }

    void renderer::draw_viewport()
    {
        if(nk_group_begin(_ctx, "viewport", 0))
        {
            switch(_data.mode)
            {
                case MX_MODE_VIEW_PLAYLIST:
                    draw_playlist();
                    break;
                case MX_MODE_EDIT_ATMOSPHERE:
                    draw_edit();
                    break;
                case MX_MODE_ABOUT:
                    draw_about();
                    break;
            }
            nk_group_end(_ctx);
        }
    }

    void renderer::draw_playlist()
    {
        nk_style_set_font(_ctx, _style.viewport.heading_font);

        // Playlist label
        nk_layout_row_begin(_ctx, NK_DYNAMIC, 0, 1);
        nk_layout_row_push(_ctx, 1.0f);
        nk_label(_ctx, get_string(STR_PLAYLIST), NK_TEXT_LEFT);

        // Playlist title, and search bar
        auto& playlist = _data.sidebar.get_selected_playlist();

        nk_layout_row_template_begin(_ctx, _style.viewport.title_height);
        nk_layout_row_template_push_dynamic(_ctx);
        const auto not_title_width =
            _style.edit.new_atmosphere_button_width +
            _style.viewport.search.pane_width +
            _style.viewport.search.button_width;
        nk_layout_row_template_push_static(_ctx, not_title_width);
        nk_layout_row_template_end(_ctx);

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
            nk_style_set_font(_ctx, _style.viewport.title_font);

            // Playlist title
            nk_label(_ctx, playlist.title.data(), NK_TEXT_LEFT);

            nk_style_set_font(_ctx, _style.viewport.heading_font);

            // New Atmosphere button
            // nk_button_label(ctx, get_string(STR_NEW_ATMOSPHERE));

            // nk_spacing(ctx, 1);

            nk_button_label(_ctx, get_string(STR_SEARCH));
        }        
    }

    void renderer::draw_edit() {}
    void renderer::draw_about() {}
}