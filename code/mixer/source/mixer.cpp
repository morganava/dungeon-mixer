#include "globals.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "string_manager.hpp"

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

    tabletopify::utf8_stringbuffer sb;

    const char* funny_words[] =
    {
        "",
        "Hello World",
        "cote",
        "coté",
        "côte",
        "côté",
        "grüßen",
        "ὈΔΥΣΣΕΎΣ",
        "[一-龠々〆ヵヶ]",
        "大変、　よ　ろし　くお願い申し上げます。",
    };

    for(const auto& str : funny_words)
    {
        tabletopify::utf8_string utf8(str);
        tabletopify::utf8_string norm = utf8.normalized();

        MX_LOG_INFO("\n str  '{}', {}\n utf8 '{}', {}\n norm '{}', {}", str, std::strlen(str),utf8, utf8.size(), norm, norm.size());
        MX_LOG_INFO("'{}' == '{}' => {}", utf8, norm, utf8 == norm);

        for(auto cp : norm)
        {
            char buffer[5] = {0};
            utf8proc_encode_char(cp.raw, (utf8proc_uint8_t*)buffer);
            MX_LOG_INFO("0x{:x} : '{}' whitespace ? {}", cp.raw, buffer, cp.is_whitespace());
            sb.push_back(cp);
        }
    }

    utf8_codepoint cp = {0x2007};

    MX_LOG_INFO("FULL STRING : '{}'", sb.str());
    sb.clear();
    sb.push_back(cp);
    MX_LOG_INFO("FULL STRING : '{}'", sb.str());

    utf8_string splitme = "hello this is a string all about how my life got twist　turned 大変、　よ　ろし　くお願い申し上げます。";

    for(auto str : splitme.split())
    {
        MX_LOG_INFO("'{}'", str);
    }

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