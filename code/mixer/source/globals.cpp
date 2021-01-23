#include "utilities.hpp"
#include "paths.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "serialization.hpp"
#include "string_manager.hpp"
#include "singleton.hpp"
#include "utilities.hpp"
#include "geometry.hpp"
#include "nanovg.hpp"
#include "imgui.hpp"
#include "ui.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "nanovg.hpp"
#include "stylesheet.hpp"
#include "random.hpp"

namespace dm
{
    // globals

    void init_globals()
    {
        // work queue
        init_work_manager();

        // init random
        init_random(0);

        // init logger with desired log verbosity
        init_logger(log_level::info);

        // init nanovg
        init_nanovg();

        // init style resolver
        init_style_resolver(paths::skins() / "default.json");

        // init ui
        imgui::init_context();
        init_ui();

        // init input
        init_input();

        // init audio managers
        init_audio_directory();
        init_track_directory();
        init_atmosphere_directory();
        init_playlist_directory();
        init_playback_manager();

        // init strings
        init_string_manager(paths::strings() / "en-us.json");
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

        free_style_resolver();

        free_nanovg();

        free_random();

        // close queues off to new work
        free_work_manager();
    }
}