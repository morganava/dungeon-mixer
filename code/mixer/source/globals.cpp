#include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "tag_map.hpp"
#include "globals.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "work_queue.hpp"

namespace tabletopify
{
    // test code
    void title_query(const std::string& title)
    {
        std::cout << "title query: " << title << std::endl;
        auto result = g_audio_directory->get_audio_ids_by_query(title);
        for(auto val : result)
        {
            std::cout << " found:" << val << std::endl;
        }
    }

    void tag_query(const std::vector<std::string>& tags)
    {
        std::cout << "tag query: ";
        std::stringstream ss;
        for(auto tag : tags)
        {
            std::cout << tag << " ";
            ss << tag << " ";
        }
        std::cout << std::endl;

        auto result = g_audio_directory->get_audio_ids_by_query(ss.str());
        for(auto val : result)
        {
            std::cout << " found:" << val << std::endl;
        }
    }

    void test_split(const std::string& str, const std::string delim)
    {
        auto vals = split(str, delim);
        std::cout << "split( \"" << str << "\", \"" << delim << "\") -> {";
        for(auto s : vals)
        {
            std::cout << s << ",";
        }
        std::cout << "}" << std::endl;
    }

    void test_tag_query()
    {
        tag_query({"one-shot"});
        tag_query({"construction"});
        tag_query({"gibberish"});
        tag_query({"not-found", "one"});
        tag_query({"loop", "one"});
        tag_query({"shared"});
        tag_query({"one-shot", "seagull"});
        tag_query({"one-shot", "saw"});

        title_query("butthead");
        title_query("sea");
        title_query("seagulls");
        title_query("saw");

        test_split("hello world", " ");
        test_split("  what the fuck  ", " ");
        test_split("big   space", " ");
    }

    // globals

    void init_globals()
    {

        // print tabletopify directory
        std::cout << "Tabletopify: " << get_tabletopify_path().native() << std::endl;

        // work queue
        g_worker_manager = new worker_manager();
        g_worker_manager->initialize();

        worker(Worker_Background).enqueue([]() -> void
        {
            auto threadId = std::this_thread::get_id();

            /// Audio directory

            g_audio_directory = single_threaded<audio_directory>(threadId);
            // one-shot
#if 0
            std::vector<std::string> sawing_paths = {get_tabletopify_path() / "68edc865-9d66-46c2-82b3-9b3e5c9e42dc/sawing.wav"};
            g_audio_directory->add_entry(
                "37fbd587-ab8a-4e85-898c-be2c76d79440",
                "Sawing",
                {"construction", "saw", "shared"},
                std::make_shared<one_shot_data>(std::move(sawing_paths)));

            // loop
            std::string seagull_path = get_tabletopify_path() / "68edc865-9d66-46c2-82b3-9b3e5c9e42dc/seagulls.wav";
            g_audio_directory->add_entry(
                "21b24cc6-d659-4c39-ae69-75d7938fd7b3",
                "Seagulls",
                {"seaside", "seagulls", "shared"},
                std::make_shared<loop_data>(std::move(seagull_path)));

            test_tag_query();
#endif

            // populate search
            auto audio_ids = g_audio_directory->get_audio_ids();
            update_ui_edit_trackSearch(audio_ids);

            /// Track directory

            g_track_directory = single_threaded<track_directory>(threadId);

            /// Atmosphere Directory

            g_atmosphere_directory = single_threaded<atmosphere_directory>(threadId);

            /// test data
            g_atmosphere_directory->add_entry({
                "4d9bdcc6-282e-437b-872a-6f352f0b86f4",
                1.0f,
                "Build Site",
                { },
            });
            g_atmosphere_directory->add_entry({
                "0836fcc5-dfe3-42c8-9874-bafbd2e39465",
                0.8f,
                "Sea",
                { },
            });
            g_atmosphere_directory->add_entry({
                "62c09984-532d-4c2d-b18a-200b11194698",
                0.5f,
                "Docks",
                { },
            });

            auto atmosphere_ids = g_atmosphere_directory->get_atmosphere_ids();
            update_ui_playback_atmosphereSearch(atmosphere_ids);
            update_ui_edit_atmosphereList(atmosphere_ids);

            /// Playback Manager

            g_playback_manager = single_threaded<playback_manager>(threadId);
        });

        worker(Worker_FileIO).enqueue([]() -> void
        {
            // iterate over each subdirectory in the tabletopify directory
            for(auto& it : fs::directory_iterator(get_tabletopify_path()))
            {
                auto manifestPath = it.path() / "manifest.json";
                auto manifestSize = fs::file_size(manifestPath);
                std::cout << " manifest: " << manifestPath << std::endl;
                std::cout << " size: " << manifestSize << std::endl;

                std::ifstream manifestFile(manifestPath.native(), std::ios_base::in | std::ios_base::binary);
                std::string manifestData(manifestSize, 0);
                manifestFile.read(manifestData.data(), manifestSize);

                std::cout << manifestData << std::endl;

                // create audio entries for listings in manifest
                worker(Worker_Background).enqueue([basePath=it.path(), manifestData=std::move(manifestData)]() -> void
                {
                    auto json = nl::json::parse(manifestData);
                    auto trackList = json["Tracks"];
                    for(auto track : trackList)
                    {
                        std::string title = track["Title"];
                        std::string uuid = track["UUID"];
                        std::string type = track["Type"];
                        std::vector<std::string> tags = track["Tags"];
                        std::vector<std::string> files = track["Files"];
                        for(auto& currentFile : files)
                        {
                            currentFile = basePath / currentFile;
                        }

                        std::shared_ptr<audio_data> audioData;

                        if(type == "OneShot")
                        {
                            audioData = std::make_shared<one_shot_data>(std::move(files));
                        }
                        else if(type == "Loop")
                        {
                            audioData = std::make_shared<loop_data>(std::move(files.front()));
                        }

                        g_audio_directory->add_entry(
                            std::move(uuid),
                            std::move(title),
                            std::move(tags),
                            std::move(audioData));
                    }

                    auto audio_ids = g_audio_directory->get_audio_ids();
                    update_ui_edit_trackSearch(audio_ids);
                });
            }
        });
    }

    void free_globals()
    {
        // close queues off to new work
        g_worker_manager->finish();

        delete g_worker_manager; g_worker_manager = nullptr;

        g_atmosphere_directory.free();
        g_track_directory.free();
        g_audio_directory.free();

    }
}