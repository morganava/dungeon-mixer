 #include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "tag_map.hpp"
#include "audio.hpp"

namespace tabletopify
{
    single_threaded<audio_directory> g_audio_directory;

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
                        printf("audio title: \"%s\" age: %f\n", current_entry.title.c_str(), current_entry.age);
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

    audio_entry& audio_directory::get_entry(audio_id_t id)
    {
        assert(id != INVALID_AUDIO_ID && id < _entries.size());
        return _entries[id];
    }

    void audio_directory::add_entry(
        std::string&& uuid,
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
        audio_entry entry = {uuid, title, to_lower(title), tags, data, 0.0f};
        // get key
        audio_id_t key = _entries.size();
        // update tag map
        _tag_map.add_key(key, entry.tags);
        // update our uuid map (checking for collisions)
        auto uuidIt = _uuid_map.find(uuid);
        assert(uuidIt == _uuid_map.end());
        _uuid_map[uuid] = key;

        // push entry
        _entries.push_back(std::move(entry));
    }

    std::string audio_directory::get_title_by_audio_id(audio_id_t id) const
    {
        return _entries[id].title;
    }

    std::vector<std::string> audio_directory::get_tags_by_audio_id(audio_id_t id) const
    {
        return _entries[id].tags;
    }

    std::shared_ptr<audio_data> audio_directory::get_audio_data_by_audio_id(audio_id_t id) const
    {
        return _entries[id].data;
    }

    std::vector<audio_id_t> audio_directory::get_audio_ids() const
    {
        std::vector<audio_id_t> result;
        const auto count = _entries.size();
        for(audio_id_t k = 0; k < count; ++k)
        {
            result.push_back(k);
        }
        return result;
    }

    std::vector<audio_id_t> audio_directory::get_audio_ids_by_query(const std::string& query_string) const
    {
        return _tag_map.get_keys_by_string_query(query_string);
    }
}