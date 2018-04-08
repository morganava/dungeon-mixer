#include "precomp.hpp"

// tabletopify
#include "audio.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "serialization.hpp"

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
    }

    void free_globals()
    {
        // close queues off to new work
        free_work_manager();

        // free audio managers
        free_playback_manager();
        free_atmosphere_directory();
        free_track_directory();
        free_audio_directory();
    }
}