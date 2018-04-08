#include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"

namespace tabletopify
{
    track_directory* g_track_directory = nullptr;
    track_directory* get_track_directory()
    {
        MX_ASSERT(worker(Worker_Background).getThreadId() == std::this_thread::get_id());
        return g_track_directory;
    }
    void init_track_directory(track_directory&& trackDirectory)
    {
        if(g_track_directory == nullptr)
        {
            g_track_directory = new track_directory(std::forward<track_directory>(trackDirectory));
        }
        else
        {
            MX_ASSERT(worker(Worker_Background).getThreadId() == std::this_thread::get_id());
            *g_track_directory = std::forward<track_directory>(trackDirectory);
        }
    }
    void free_track_directory()
    {
        delete g_track_directory;
        g_track_directory = nullptr;
    }

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