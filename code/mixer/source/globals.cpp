#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "serialization.hpp"
#include "draw.hpp"
#include "string_manager.hpp"
#include "singleton.hpp"
#include "utilities.hpp"
#include "geometry.hpp"
#include "nanovg.hpp"
#include "imgui.hpp"
#include "ui.hpp"
#include "geometry.hpp"
#include "input.hpp"

namespace tabletopify
{
    // globals

    void init_globals()
    {
        // work queue
        init_work_manager();

        // init logger with desired log verbosity
        init_logger(LogLevel_Info);

        init_font_stash();

        // load default skin
        load_stylesheet();

        // init ui
        imgui::init_context();
        init_ui();

        // init input
        init_input();

        // init audio managers
        init_audio_directory();
        init_track_directory({});
        init_atmosphere_directory({});
        init_playback_manager();

        // load our audio manifests
        load_audio_manifests();

        // init strings
        init_string_manager(get_strings_path() / "en-us.json");
    }

    void free_globals()
    {
        free_string_manager();

        // free audio managers
        free_playback_manager();
        free_atmosphere_directory();
        free_track_directory();
        free_audio_directory();

        // input
        free_input();

        // ui
        free_ui();
        imgui::free_context();

        free_font_stash();

        // close queues off to new work
        free_work_manager();        
    }
}