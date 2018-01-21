  #include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "audio.hpp"

namespace tabletopify
{
    single_threaded<track_directory> g_track_directory;

    /// Track Directory

    track_entry& track_directory::get_entry(track_id_t id)
    {
        auto it = _entries.find(id);
        assert(it != _entries.end());

        return it->second;
    }

    track_id_t track_directory::add_entry(track_entry&& entry)
    {
        auto id = ++_next_id;

        auto it = _entries.find(id);
        assert(it == _entries.end());

        _entries.emplace(id, entry);

        return id;
    }

    void track_directory::remove_entry(track_id_t id)
    {
        auto it = _entries.find(id);
        assert(it != _entries.end());

        _entries.erase(it);
    }
}