#include "precomp.hpp"

#include "globals.hpp"
#include "audio.hpp"
#include "utilities.hpp"
#include "draw.hpp"

namespace tabletopify
{
    void init_callback(nk_context* ctx)
    {
        init_globals();
    }

    void gui_callback(float dt, nk_context* ctx, int width, int height)
    {
        ctx->style.window.scrollbar_size = nk_vec2(10, 0);
        ctx->style.window.padding = nk_vec2(0,0);
        ctx->style.window.group_padding = nk_vec2(0, 0);
        ctx->style.window.spacing = nk_vec2(4, 0);
        ctx->style.button.padding = nk_vec2(0, 0);
        ctx->style.scrollh.padding = nk_vec2(0, 0);
        ctx->style.scrollh.border = 0.0f;

        draw(ctx, width, height);

        // perform work in queue
        for(auto func : work_queue)
        {
            func();
        }
        work_queue.clear();

        // update our track directory
        g_track_directory.update(dt);
    }


}


int main() try
{
    nk_main("Tabletop Mixer", 800, 600, &tabletopify::init_callback, &tabletopify::gui_callback);
}
catch(const std::exception& ex)
{
    std::fprintf(stderr, "Error: %s\n", ex.what());
}