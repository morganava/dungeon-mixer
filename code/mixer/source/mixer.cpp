#include "precomp.hpp"

#include "globals.hpp"
#include "utilities.hpp"
#include "tag_map.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace tabletopify
{
    void init_callback(nk_context* ctx)
    {
        init_globals();

        log(LogLevel_Info, "%s: %i", "Hello from logger thread!", __LINE__);
        log(LogLevel_Error, "YOu done FuCked Up!");

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
            wi = {};
        }

        // draw widgets
        draw_update(ctx, width, height);

        volatile bool finished = false;
        worker(Worker_Background).enqueue([dt]()
        {
            g_audio_directory->update(dt);
            g_playback_manager->update(dt);
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