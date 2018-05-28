#include "globals.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace tabletopify
{
    void init_callback(nk_context* ctx)
    {
        // init renderer
        init_renderer(ctx);
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

    void error_callback(const char* msg)
    {
        MX_LOG_ERROR(std::string(msg));
    }
}

using namespace tabletopify;

int main() try
{
    tabletopify::init_globals();

    struct nk_callbacks callbacks = 
    {
        &tabletopify::init_callback,
        &tabletopify::gui_callback,
        &tabletopify::close_callback,
        &error_callback,
    };

    nk_main("Tabletop Mixer", 800, 600, callbacks);
}
catch(const std::exception& ex)
{
    get_logger()->blocking_log(LogLevel_Error, ex.what());
}