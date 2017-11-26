 #pragma once

namespace tabletopify
{
	// UI queues up work here
	extern std::vector<std::function<void(void)>> work_queue;

	// contains the list of track ids to populate the track search area
	extern std::vector<size_t> searched_track_ids;

	void init_globals();
	void free_globals();
}