#include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "tag_map.hpp"
#include "globals.hpp"
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
        g_worker_manager = new worker_manager();
        g_worker_manager->initialize();

        init_logger(LogLevel_Info);

        // init audio managers
        worker(Worker_Background).enqueue([]() -> void
        {
            auto threadId = std::this_thread::get_id();

            /// Audio directory

            g_audio_directory = single_threaded<audio_directory>(threadId);

            /// Track directory

            g_track_directory = single_threaded<track_directory>(threadId);

            /// Atmosphere Directory

            g_atmosphere_directory = single_threaded<atmosphere_directory>(threadId);

            /// Playback Manager

            g_playback_manager = single_threaded<playback_manager>(threadId);
        });

        // load our audio manifests
        load_audio_manifests();
    }

    void free_globals()
    {
        // close queues off to new work
        g_worker_manager->finish();

        delete g_worker_manager; g_worker_manager = nullptr;

        g_playback_manager.free();
        g_atmosphere_directory.free();
        g_track_directory.free();
        g_audio_directory.free();
    }
}