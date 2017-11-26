#include "precomp.hpp"

// tabletopify
#include "globals.hpp"
#include "audio.hpp"
#include "utilities.hpp"
#include "draw.hpp"

namespace tabletopify
{
	// test code

    void title_query(const std::string& title)
    {
        std::cout << "title query: " << title << std::endl;
        auto result = g_track_directory.get_track_ids_by_title(title);
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

        auto result = g_track_directory.get_track_ids_by_tag(tags);
        for(auto val : result)
        {
            std::cout << " found:" << val << std::endl;
        }
    }

    void test_track_query()
    {
	    tag_query({"one-shot"});
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
    }

    // globals

	std::vector<std::function<void(void)>> work_queue;

	void init_globals()
	{
		searched_track_ids = g_track_directory.get_track_ids();
		test_track_query();
	}

	void free_globals()
	{

	}
}