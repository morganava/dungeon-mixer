#include "precomp.hpp"

#include "utilities.hpp"
#include "globals.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "work_queue.hpp"

namespace tabletopify
{
    void init_callback(nk_context* ctx)
    {
        init_globals();

        draw_init(ctx);
    }

    void gui_callback(float dt, nk_context* ctx, int width, int height)
    {
        // do queued UI work
        work_item wi;
        while(worker(Worker_UI).try_dequeue(wi))
        {
            wi.func();
            if(wi.finished)
            {
                *wi.finished = true;
            }
        }

        // draw widgets
        draw_update(ctx, width, height);

        bool finished;
        worker(Worker_Background).enqueue([=]()
        {
            g_playback_manager->update(dt);
        }, &finished);

        while(!finished) yield(1);
    }

    // on window exit
    void close_callback(nk_context* ctx)
    {
        free_globals();
    }
}


int main() try
{
    nk_main("Tabletop Mixer", 800, 600, &tabletopify::init_callback, &tabletopify::gui_callback, &tabletopify::close_callback);
}
catch(const std::exception& ex)
{
    std::fprintf(stderr, "Error: %s\n", ex.what());
}