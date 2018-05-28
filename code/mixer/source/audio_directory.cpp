// tabletopify
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"

namespace tabletopify
{
    audio_directory* g_audio_directory = nullptr;
    audio_directory* get_audio_directory()
    {
        MX_ASSERT(worker(Worker_Background).getThreadId() == std::this_thread::get_id());
        return g_audio_directory;
    }
    void init_audio_directory()
    {
        MX_ASSERT(g_audio_directory == nullptr);
        g_audio_directory = new audio_directory();
    }
    void free_audio_directory()
    {
        delete g_audio_directory;
        g_audio_directory = nullptr;
    }

    /// Audio Directory

    void audio_directory::update(float dt)
    {
        for(auto& current_entry : _entries)
        {
            switch(current_entry.data->state())
            {
                case UNLOADED:
                {
                    // something other than the audio_directory has an instance
                    // of the shared audio_data, so load it
                    if(current_entry.data.use_count() > 1)
                    {
                        audio_data::load(current_entry.data);
                    }
                }
                break;
                case LOADING:
                {
                    // no-op, FileIO worker is doing work
                }
                break;
                case LOADED:
                {
                    // audio loaded, but only the audio_directory has a reference
                    if(current_entry.data.use_count() == 1)
                    {
                        current_entry.age += dt;
                        if(current_entry.age >= unload_age)
                        {
                            audio_data::unload(current_entry.data);
                        }
                    }
                    else
                    {
                        current_entry.age = 0.0f;
                    }
                }
                break;
                case UNLOADING:
                {
                    // no-op, FileIO worker is doing work
                }
                break;
                case PENDING_UNLOADED:
                {
                    current_entry.data->_state = UNLOADED;
                }
                break;
            }
        }
    }

    audio_entry& audio_directory::get_entry(audio_handle_t handle)
    {
        MX_ASSERT(handle != MX_INVALID_HANDLE && handle < _entries.size());
        return _entries[handle];
    }

    void audio_directory::add_entry(
        uuid&& id,
        std::string&& title,
        std::vector<std::string>&& tags,
        std::shared_ptr<audio_data>&& data)
    {
        // update tags
        // add split title
        auto title_tags = split(to_lower(title), " ");
        for(auto& str : title_tags)
        {
            tags.emplace_back(std::move(str));
        }

        // add one-shot or loop tags depending on recieved data
        if(dynamic_cast<one_shot_data*>(data.get()))
        {
            tags.push_back("one-shot");
        }
        else if(dynamic_cast<loop_data*>(data.get()))
        {
            tags.push_back("loop");
        }

        // construct audio_entry
        audio_entry entry = {id, title, tags, data, 0.0f};
        // get key
        audio_handle_t key = _entries.size();
        // update tag map
        _tag_map.add_key(key, entry.tags);
        // update our id map (checking for collisions)
        auto idIt = _id_map.find(id);
        MX_ASSERT(idIt == _id_map.end());
        _id_map[id] = key;

        // push entry
        _entries.push_back(std::move(entry));
    }

    audio_handle_t audio_directory::get_audio_handle_by_id(const uuid& id) const
    {
        auto it = _id_map.find(id);
        if(it == _id_map.end())
        {
            return MX_INVALID_HANDLE;
        }
        return it->second;
    }

    std::vector<audio_handle_t> audio_directory::get_audio_handles() const
    {
        std::vector<audio_handle_t> result;
        const auto count = _entries.size();
        for(audio_handle_t k = 0; k < count; ++k)
        {
            result.push_back(k);
        }
        return result;
    }

    std::vector<audio_handle_t> audio_directory::get_audio_handles_by_query(const std::string& query) const
    {
        return _tag_map.get_keys_by_string_query(query);
    }
}