#pragma once

namespace tabletopify
{
    typedef void(*graphics_init_callback)(NVGcontext*);
    typedef void(*graphics_update_callback)(NVGcontext* ctx, float dt, int width, int height);
    typedef void(*graphics_close_callback)(NVGcontext*);

    struct graphics_callbacks
    {
        graphics_init_callback init     = nullptr;
        graphics_update_callback update = nullptr;
        graphics_close_callback close   = nullptr;
    };

    void graphics_main(const graphics_callbacks& callbacks);
}