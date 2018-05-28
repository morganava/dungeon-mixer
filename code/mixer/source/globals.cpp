#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "serialization.hpp"
#include "draw.hpp"
#include "string_manager.hpp"

namespace tabletopify
{
    // globals

    void init_globals()
    {
        // work queue
        init_work_manager();

        // init logger with desired log verbosity
        init_logger(LogLevel_Info);

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
        free_renderer();

        free_string_manager();

        // close queues off to new work
        free_work_manager();

        // free audio managers
        free_playback_manager();
        free_atmosphere_directory();
        free_track_directory();
        free_audio_directory();

        
    }
}