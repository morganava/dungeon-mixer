#include "precomp.hpp"

// tabletopify
#include "globals.hpp"
#include "audio.hpp"
#include "utilities.hpp"
#include "draw.hpp"
#include "work_queue.hpp"

namespace tabletopify
{
	static std::thread* background_worker = nullptr;
	static std::thread* fileio_worker = nullptr;

	// test code
    void title_query(const std::string& title)
    {
        std::cout << "title query: " << title << std::endl;
        auto result = g_audio_directory->get_audio_ids_by_title(title);
        for(auto val : result)
        {
            std::cout << " found:" << val << std::endl;
        }        
    }

    void tag_query(const std::vector<std::string>& tags)
    {
        std::cout << "tag query: ";
        for(auto tag : tags)
        {
            std::cout << tag << " ";
        }
        std::cout << std::endl;

        auto result = g_audio_directory->get_audio_ids_by_tag(tags);
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

    void test()
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

    static void worker_thread(size_t idx)
    {
    	while(g_work_queue->accepting_work())
    	{
    		// empty queue of work
    		work_item wi;
    		while(g_work_queue->try_dequeue(idx, wi))
    		{
    			wi.func();
	    		if(wi.finished)
	    		{
	    			*wi.finished = true;
	    		}
    		}
    		// sleep for 16 ms before trying again
    		yield(16);
    	}
    }

	void init_globals()
	{
		// work queue
		g_work_queue = new work_queue();

		// worker threads
		fileio_worker = new std::thread(worker_thread, Queue_FileIO);		
		background_worker = new std::thread(worker_thread, Queue_Background);

        g_work_queue->enqueue(Queue_UI, []() -> void
        {
            update_ui_edit_atmosphereList(g_atmosphere_directory->get_atmosphere_ids());
            update_ui_edit_trackSearch(g_audio_directory->get_audio_ids());
        });

        // audio directory
        g_audio_directory = new audio_directory();
        

        /// test data

        // one-shot
        std::vector<std::string> sawing_paths = {"/home/pospeselr/Downloads/sawing.wav"};
        g_audio_directory->add_entry(
            "37fbd587-ab8a-4e85-898c-be2c76d79440",
            "Sawinging",
            {"construction", "saw", "shared"},
            std::make_shared<one_shot_data>(std::move(sawing_paths)));

        // loop
        std::string seagull_path = "/home/pospeselr/Downloads/seagulls.wav";
        g_audio_directory->add_entry(
            "21b24cc6-d659-4c39-ae69-75d7938fd7b3",
            "Seagulls",
            {"seaside", "seagulls", "shared"},
            std::make_shared<loop_data>(std::move(seagull_path)));

        // atmosphere directory
        g_atmosphere_directory = new atmosphere_directory();

        /// test data        
        g_atmosphere_directory->add_entry({
            "4d9bdcc6-282e-437b-872a-6f352f0b86f4",
            1.0f,
            "Build Site",
            {"City", "Construction"},
            {
                {0, ONE_SHOT, 0.5f, {0.0f, 0.0f, 1.0f}, {1.0f, 20.0f}},
            },
        });
        g_atmosphere_directory->add_entry({
            "0836fcc5-dfe3-42c8-9874-bafbd2e39465",
            0.8f,
            "Sea",
            {"Esga"},
            {
                {1, LOOP, 0.5f, {0.0f, 0.0f, 1.0f}},
            },
        });
        g_atmosphere_directory->add_entry({
            "62c09984-532d-4c2d-b18a-200b11194698",
            0.5f,
            "Docks",
            {"Esga", "City"},
            {
                {0, ONE_SHOT, 0.5f, {0.0f, 0.0f, 1.0f}, {1.0f, 20.0f}},
                {1, LOOP, 0.5f, {0.0f, 0.0f, 1.0f}},
            },
        });

		test();
	}

	void free_globals()
	{
        std::cout << "Finishing Work Queues" << std::endl;
        // close queue off to new work
        g_work_queue->finish();

        std::cout << "Waiting for workers" << std::endl;
        // wait for workers to finish
        background_worker->join();
        fileio_worker->join();

        std::cout << "Cleanup" << std::endl;
        // delete workers
        delete background_worker; background_worker = nullptr;
        delete fileio_worker; fileio_worker = nullptr;
        
        // delete queues
        delete g_work_queue; g_work_queue = nullptr;

        delete g_atmosphere_directory; g_atmosphere_directory = nullptr;
        delete g_audio_directory; g_audio_directory =  nullptr;
	}
}