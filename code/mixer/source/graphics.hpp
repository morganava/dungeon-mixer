#pragma once

namespace dm
{
    typedef void(*graphics_init_callback)();
    typedef void(*graphics_update_callback)(float dt, int width, int height, double pixel_ratio);
    typedef void(*graphics_close_callback)();

    struct graphics_callbacks
    {
        graphics_init_callback init     = nullptr;
        graphics_update_callback update = nullptr;
        graphics_close_callback close   = nullptr;
    };

    void graphics_main(const graphics_callbacks& callbacks);
}