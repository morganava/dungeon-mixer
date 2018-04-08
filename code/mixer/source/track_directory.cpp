  #include "precomp.hpp"

// tabletopify
#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "tag_map.hpp"
#include "audio.hpp"

namespace tabletopify
{
    single_threaded<track_directory> g_track_directory;
    track_handle_t track_directory::_next_handle = 0;

    /// Track Directory

    track_entry& track_directory::get_entry(track_handle_t handle)
    {
        auto it = _entries.find(handle);
        MX_ASSERT(it != _entries.end());

        return it->second;
    }

    track_handle_t track_directory::add_entry(track_entry&& entry)
    {
        auto handle = ++_next_handle;

        auto it = _entries.find(handle);
        MX_ASSERT(it == _entries.end());

        _entries.emplace(handle, entry);

        return handle;
    }

    void track_directory::remove_entry(track_handle_t handle)
    {
        auto it = _entries.find(handle);
        MX_ASSERT(it != _entries.end());

        _entries.erase(it);
    }
}