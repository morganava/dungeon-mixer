#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "string_manager.hpp"

namespace dm
{
    /// Audio Directory

    void audio_directory::update(float dt)
    {
        for(auto& current_entry : _entries)
        {
            const auto state = current_entry.data->state();
            switch(state)
            {
                case audio_data_state::unloaded:
                {
                    // something other than the audio_directory has an instance
                    // of the shared audio_data, so load it
                    if(current_entry.data.use_count() > 1)
                    {
                        audio_data::load(current_entry.data);
                    }
                }
                break;
                case audio_data_state::loading:
                {
                    // no-op, FileIO worker is doing work
                }
                break;
                case audio_data_state::loaded:
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
                case audio_data_state::unloading:
                {
                    // no-op, FileIO worker is doing work
                }
                break;
                case audio_data_state::pending_unloaded:
                {
                    current_entry.data->_state = audio_data_state::unloaded;
                }
                break;
                default:
                {
                    MX_FAIL_FAST("invalid audio_data_state : {}", static_cast<uint_fast8_t>(state));
                }
            }
        }
    }

    audio_entry& audio_directory::get_entry(audio_handle_t handle)
    {
        MX_ASSERT(handle != audio_handle::invalid && static_cast<size_t>(handle) < _entries.size());
        return _entries[static_cast<size_t>(handle)];
    }

    void audio_directory::add_entry(
        uuid&& id,
        utf8_string&& title,
        std::vector<utf8_string>&& tags,
        std::shared_ptr<audio_data>&& data)
    {
        // update tags
        // add split title
        auto title_tags = title.split();
        for(auto& str : title_tags)
        {
            tags.push_back(std::move(str));
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
        audio_entry entry = {id, std::move(title), data, 0.0f};
        // get key
        audio_handle_t key = next_audio_handle();
        MX_ASSERT(static_cast<size_t>(key) == _entries.size());
        // update tag map
        _tag_map.add_key(key, tags);
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
            return audio_handle::invalid;
        }
        return it->second;
    }

    std::vector<audio_handle_t> audio_directory::get_audio_handles_by_query(const utf8_string& query) const
    {
        return _tag_map.get_keys_by_tag_query(query);
    }
}