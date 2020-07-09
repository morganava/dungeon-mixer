#include "globals.hpp"
#include "utilities.hpp"
#include "audio.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "string_manager.hpp"
#include "graphics.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "nanovg.hpp"
#include "singleton.hpp"
#include "imgui.hpp"
#include "ui.hpp"

namespace tabletopify
{
    void init_callback()
    {
        MX_LOG_INFO("Init callback!");
        // builds map from keycodes to character name
        g_input.build_key_map();
        g_input.init_cursors();

        g_nanovg.create_context();
        // init renderer
        // init_renderer(ctx);
    }

    void gui_callback(float dt, int width, int height, double pixel_ratio)
    {
        auto fut = worker_background.enqueue([=]() -> void
        {
            g_audio_directory.update(dt);
            g_playback_manager.update(dt);
        });

        // do queued UI work
        g_work_manager.do_tasks(worker_id::ui);

        g_input.update(dt);
        g_ui.update(dt, width, height);
        g_nanovg.update(dt, width, height, pixel_ratio);
        g_input.update_cursor();

        // wait for background audio task to cmplete
        fut.wait();
    }

    // on window exit
    void close_callback()
    {
        MX_LOG_INFO("Closing!");
        free_globals();
    }

    void error_callback(const char* msg)
    {
        MX_LOG_ERROR(std::string(msg));
    }
}

using namespace tabletopify;

void test()
{
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
            utf8proc_encode_char(cp, (utf8proc_uint8_t*)buffer);
            MX_LOG_INFO("0x{:x} : '{}' whitespace ? {}", cp, buffer, cp.is_whitespace());
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
}

int main() try
{

    tabletopify::init_globals();

    graphics_callbacks callbacks =
    {
        &init_callback,
        &gui_callback,
        &close_callback,
    };
    graphics_main(callbacks);
    return 0;
}
catch(const std::exception& ex)
{
    get_logger()->blocking_log(tabletopify::log_level::error, ex.what());
}