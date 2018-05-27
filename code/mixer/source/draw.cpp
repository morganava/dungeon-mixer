#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "nuklear_extensions.hpp"
#include "serialization.hpp"
#include "string_manager.hpp"

namespace tabletopify
{
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
            MX_GET_CSTRING(MX_STR_ALL_PLAYLIST),
            uuid::null
        });

        // debug data
        _data.sidebar.playlists.push_back({
            "Dummy Playlist",
            worker(Worker_UI).nextUUID()
        });

        _data.viewport.playlist.atmospheres.push_back({
            "Farmland",
            worker(Worker_UI).nextUUID(),
            1.0f,
            MX_UNMUTED,
            MX_STOPPED,
        });

        _data.viewport.playlist.atmospheres.push_back({
            "Lightning Storm",
            worker(Worker_UI).nextUUID(),
            0.8f,
            MX_UNMUTED,
            MX_PLAYING,
        });

        _data.viewport.playlist.atmospheres.push_back({
            "Raging Battle",
            worker(Worker_UI).nextUUID(),
            0.9f,
            MX_MUTED,
            MX_PLAYING,
        });
    }    

    void renderer::load_style()
    {
        auto skinPath = get_skins_path();

        // this would be part of style json loading
        auto& font_atlas = _style.font_atlas;
        font_atlas.begin();

        _style.shared.edit_string_font = font_atlas.add(skinPath / "DejaVuSans.ttf", 16.0f);

        _style.sidebar.width = 196;
        _style.sidebar.indent = 16;
        _style.sidebar.section_font = font_atlas.add(skinPath / "DejaVuSans.ttf", 18.0f);
        _style.sidebar.section_height = 22.0f;

        _style.viewport.shared.title_font = font_atlas.add(skinPath / "DejaVuSans.ttf", 56.0f);
        _style.viewport.shared.title_height = 64.0f;
        _style.viewport.shared.heading_font = font_atlas.add(skinPath / "DejaVuSans.ttf", 18.0f);
        _style.viewport.shared.heading_height = 18.0f;
        _style.viewport.shared.search_height = 24.0f;
        _style.viewport.shared.search_field_width = 300.0f;
        _style.viewport.shared.search_button_width = 24.0f;
        _style.viewport.shared.search_button_icon = nk_load_image(skinPath / "magnifying-glass.png");
        _style.viewport.shared.topbar_padding = 4.0f;
        _style.viewport.shared.context_menu_font = font_atlas.add(skinPath / "DejaVuSans.ttf", 18.0f);
        _style.viewport.shared.context_menu_height = 22.0f;
        _style.viewport.shared.context_menu_width = 300.0f;

        _style.viewport.playlist.new_atmosphere_font = font_atlas.add(skinPath / "DejaVuSans.ttf", 14.0f);
        _style.viewport.playlist.new_atmosphere_button_width = 132.0f;
        _style.viewport.playlist.header_font = font_atlas.add(skinPath / "DejaVuSans.ttf", 22.0f);
        _style.viewport.playlist.header_height = 30.0f;
        _style.viewport.playlist.row_font = font_atlas.add(skinPath / "DejaVuSans.ttf", 16.0f);
        _style.viewport.playlist.row_height = 30.0f;
        _style.viewport.playlist.volume_width = 240.0f;
        _style.viewport.playlist.mute_width = 32.0f;
        _style.viewport.playlist.muted_button_icon = nk_load_image(skinPath / "mute.png");
        _style.viewport.playlist.unmuted_button_icon = nk_load_image(skinPath / "volume.png");
        _style.viewport.playlist.play_stop_width = 32.0f;
        _style.viewport.playlist.stop_button_icon = nk_load_image(skinPath / "stop.png");
        _style.viewport.playlist.play_button_icon = nk_load_image(skinPath / "play.png");

        font_atlas.end();

        nk_style_set_font(_ctx, _style.sidebar.section_font);
    }

    void renderer::update(float dt, int32_t width, int32_t height)
    {
        _data.window.width = width;
        _data.window.height = height;

        draw();
    }

    void renderer::draw()
    {
        if(nk_begin(_ctx, "main", nk_rect(0, 0, _data.window.width, _data.window.height), NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_template_begin(_ctx, _data.window.height);
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
        if(nk_group_begin(_ctx, "main>sidebar", 0))
        {
            nk_style_set_font(_ctx, _style.sidebar.section_font);

            // playlist button
            nk_layout_row_dynamic(_ctx, _style.sidebar.section_height, 1);
            nk_label(_ctx, MX_GET_CSTRING(MX_STR_PLAYLISTS), NK_TEXT_LEFT);

            // create playlist button
            nk_layout_row_template_begin(_ctx, _style.sidebar.section_height);
            nk_layout_row_template_push_static(_ctx, _style.sidebar.indent);
            nk_layout_row_template_push_dynamic(_ctx);
            nk_layout_row_template_end(_ctx);

            nk_empty_fill(_ctx, 1);
            nk_label(_ctx, MX_GET_CSTRING(MX_STR_CREATE_PLAYLIST), NK_TEXT_LEFT);

            for(const auto& pl : _data.sidebar.playlists)
            {
                nk_empty_fill(_ctx, 1);
                nk_label(_ctx, pl.title.data(), NK_TEXT_LEFT);
            }

            // edit button
            nk_layout_row_dynamic(_ctx, _style.sidebar.section_height, 1);
            nk_label(_ctx, MX_GET_CSTRING(MX_STR_EDIT_ATMOSPHERE), NK_TEXT_LEFT);

            // settings button
            nk_layout_row_dynamic(_ctx, _style.sidebar.section_height, 1);
            nk_label(_ctx, MX_GET_CSTRING(MX_STR_SETTINGS), NK_TEXT_LEFT);

           // about button
            nk_layout_row_dynamic(_ctx, _style.sidebar.section_height, 1);
            nk_label(_ctx, MX_GET_CSTRING(MX_STR_ABOUT), NK_TEXT_LEFT);

            nk_group_end(_ctx);
        }
    }

    void renderer::draw_viewport()
    {
        if(nk_group_begin(_ctx, "viewport", NK_WINDOW_NO_SCROLLBAR))
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
        auto& playlist = _data.sidebar.get_selected_playlist();

        // Top Bar

        const auto topbarHeight = 
            _style.viewport.shared.heading_height +
            _style.viewport.shared.title_height +
            _style.viewport.shared.search_height +
            _style.viewport.shared.topbar_padding;

        nk_layout_row_dynamic(_ctx, topbarHeight, 1);
        if(nk_group_begin(_ctx, "main>viewport>topbar", NK_WINDOW_NO_SCROLLBAR))
        {
            // Heading

            nk_style_set_font(_ctx, _style.viewport.shared.heading_font);
            nk_layout_row_dynamic(_ctx, _style.viewport.shared.heading_height, 1);
            nk_label(_ctx, MX_GET_CSTRING(MX_STR_PLAYLIST), NK_TEXT_LEFT);

            // Playlist title

            nk_style_set_font(_ctx, _style.viewport.shared.title_font);
            nk_layout_row_dynamic(_ctx, _style.viewport.shared.title_height, 1);
            nk_label(_ctx, playlist.title.data(), NK_TEXT_LEFT);

            // Search bar

            nk_layout_row_template_begin(_ctx, _style.viewport.shared.search_height);
            nk_layout_row_template_push_dynamic(_ctx);
            nk_layout_row_template_push_dynamic(_ctx);
            nk_layout_row_template_push_static(_ctx, _style.viewport.playlist.new_atmosphere_button_width);
            nk_layout_row_template_push_variable(_ctx, _style.viewport.shared.search_field_width);
            nk_layout_row_template_push_static(_ctx, _style.viewport.shared.search_button_width);
            nk_layout_row_template_end(_ctx);

            {
                nk_spacing(_ctx, 2);
                
                // New Atmosphere Button
                nk_style_set_font(_ctx, _style.viewport.playlist.new_atmosphere_font);
                if(nk_button_label(_ctx, MX_GET_CSTRING(MX_STR_NEW_ATMOSPHERE)))
                {

                }

                // Atmosphere Search
                nk_style_set_font(_ctx, _style.shared.edit_string_font);
                if(nk_edit_string_zero_terminated(_ctx, NK_EDIT_FIELD, playlist.searchQuery, nk_filter_default))
                {

                }

                // Search Button
                if(nk_button_image(_ctx, _style.viewport.shared.search_button_icon))
                {

                }
            }

            nk_group_end(_ctx);
        }

        // Playlist

        const auto playlistHeight = _data.window.height - topbarHeight;

        nk_layout_row_dynamic(_ctx, playlistHeight, 1);
        if(nk_group_begin(_ctx, "main>viewport>playlist", 0))
        {
            // Header

            nk_layout_row_template_begin(_ctx, _style.viewport.playlist.header_height);
            nk_layout_row_template_push_dynamic(_ctx);
            nk_layout_row_template_push_static(_ctx, _style.viewport.playlist.volume_width + 
                                                     _style.viewport.playlist.mute_width +
                                                     _style.viewport.playlist.play_stop_width);
            nk_layout_row_template_end(_ctx);

            nk_style_set_font(_ctx, _style.viewport.playlist.header_font);
            nk_label(_ctx, MX_GET_CSTRING(MX_STR_TITLE), NK_TEXT_LEFT);
            nk_label(_ctx, MX_GET_CSTRING(MX_STR_VOLUME), NK_TEXT_LEFT);

            // Atmospheres

            nk_style_set_font(_ctx, _style.viewport.playlist.row_font);
            for(size_t idx = 0; idx < _data.viewport.playlist.atmospheres.size(); ++idx)
            {
                nk_layout_row_dynamic(_ctx, _style.viewport.playlist.row_height, 1);
                auto rowBounds = nk_widget_bounds(_ctx);

                // Row
                if(nk_group_begin(_ctx, string_format("main>viewport>playlist>%u", (uint32_t)idx).c_str(), NK_WINDOW_NO_SCROLLBAR))
                {
                    auto& atmosphere = _data.viewport.playlist.atmospheres[idx];

                    nk_layout_row_template_begin(_ctx, _style.viewport.playlist.row_height);
                    nk_layout_row_template_push_dynamic(_ctx);
                    nk_layout_row_template_push_static(_ctx, _style.viewport.playlist.volume_width);
                    nk_layout_row_template_push_static(_ctx, _style.viewport.playlist.mute_width);
                    nk_layout_row_template_push_static(_ctx, _style.viewport.playlist.play_stop_width);
                    nk_layout_row_template_end(_ctx);

                    // Title
                    nk_label(_ctx, atmosphere.title.c_str(), NK_TEXT_LEFT);

                    // Volume Slider
                    if(nk_slider_float(_ctx, 0.0f, &atmosphere.volume, 1.0f, 0.01f))
                    {

                    }

                    if(nk_button_image(_ctx, atmosphere.muted ? 
                                             _style.viewport.playlist.muted_button_icon : 
                                             _style.viewport.playlist.unmuted_button_icon))
                    {

                    }

                    if(nk_button_image(_ctx, atmosphere.playing ? 
                                             _style.viewport.playlist.play_button_icon : 
                                             _style.viewport.playlist.stop_button_icon))
                    {

                    }

                    nk_group_end(_ctx);
                }

                // Context Menu
                auto contextMenuSize = nk_vec2(
                    _style.viewport.shared.context_menu_width,
                    _style.viewport.shared.context_menu_height * 4
                );
                if(nk_contextual_begin(_ctx, NK_WINDOW_NO_SCROLLBAR, contextMenuSize, rowBounds))
                {
                    nk_layout_row_dynamic(_ctx, contextMenuSize.y, 1);
                    nk_contextual_end(_ctx);
                }
            }

            nk_group_end(_ctx);
        }
    }

    void renderer::draw_edit() {}
    void renderer::draw_about() {}
}