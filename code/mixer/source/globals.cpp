#include "paths.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "serialization.hpp"
#include "string_manager.hpp"
#include "service.hpp"
#include "nanovg.hpp"
#include "imgui.hpp"
#include "ui.hpp"
#include "input.hpp"
#include "nanovg.hpp"
#include "stylesheet.hpp"
#include "random.hpp"
#include "graphics.hpp"
#include "config.hpp"

namespace dm
{
    // globals

    void init_globals()
    {
        smol::register_failure_callback([](std::string_view message) -> void {
            MX_LOG_ERROR_BLOCKING(std::string(message));
        });

        // work queue
        init_work_manager();

        init_config(paths::config() / "config.json");

        // init logger with desired log verbosity
        init_logger(log_level::info, g_config->get_log_filename());

        // init random
        init_random(0);

        // init nanovg
        init_nanovg();

        // init style resolver
        init_style_resolver(paths::skins() / "default.json");

        // init ui
        init_context();
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

        init_graphics();
    }

    void free_globals()
    {
        // wait for task queues to empty
        free_work_manager();

        free_graphics();

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
        free_context();

        free_style_resolver();

        free_nanovg();

        free_random();
    }
}
