#include "globals.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace tabletopify
{
    void init_callback(nk_context* ctx)
    {
        init_globals(ctx);
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
            wi = {};
        }

        // draw widgets
        // update_draw(ctx, width, height);
        get_renderer()->update(dt, width, height);

        volatile bool finished = false;
        worker(Worker_Background).enqueue([dt]()
        {
            get_audio_directory()->update(dt);
            get_playback_manager()->update(dt);
        }, &finished);

        // busy wait
        while(!finished) std::this_thread::yield();
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