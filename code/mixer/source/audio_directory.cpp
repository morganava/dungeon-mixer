 #include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "audio.hpp"

namespace tabletopify
{
    single_threaded<audio_directory> g_audio_directory;	

    /// Audio Directory

    void audio_directory::update(float dt)
    {
        for(auto& current_entry : _entries)
        {
            if(current_entry.data.use_count() == 1)
            {
                current_entry.age += dt;
                if(current_entry.age >= unload_age &&
                   current_entry.data->_loaded)
                {
                    printf("Unloading: %s \n", current_entry.title.c_str());
                    current_entry.data->unload();
                    current_entry.age = 0.0f;
                }
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
        size_t idx = _entries.size();

        // normalized lowercase title for search
        std::string lowerTitle = to_lower(title);

        // update our tag map with provided tags
        for(auto& currentTag : tags)
        {
            currentTag = to_lower(currentTag);
            _tag_map[currentTag].insert(idx);
        }

        // add one-shot or loop tags depending on recieved data
        if(dynamic_cast<one_shot_data*>(data.get()))
        {
            _tag_map["one-shot"].insert(idx);
            tags.push_back("one-shot");
        }
        else if(dynamic_cast<loop_data*>(data.get()))
        {
            _tag_map["loop"].insert(idx);
            tags.push_back("loop");
        }

        std::cout << "title: " << title << std::endl << " tags: ";
        for(auto currentTag : tags)
        {
            std::cout << currentTag << " ";
        }
        std::cout << std::endl;

        // update our uuid map (checking for collisions)
        auto uuidIt = _uuid_map.find(uuid);
        if(uuidIt != _uuid_map.end())
        {
            auto& existingTrack = _entries[uuidIt->second];
            throw std::runtime_error(
                string_format(
                    "Colliding UUID (%s) between track '%s' and '%s'", 
                    uuid.c_str(), 
                    existingTrack.title.c_str(), 
                    title.c_str()));
        }
        _uuid_map[uuid] = idx;

        // add entry to table
        _entries.push_back(
        {
            uuid,
            title,
            std::move(lowerTitle),
            tags,
            data
        });
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

    std::vector<audio_id_t> audio_directory::get_audio_ids_by_title(const std::string& title_query) const
    {
        std::string lowerQuery = title_query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

        std::vector<audio_id_t> result;

        // audio_id_t over all track_entry and search for given title
        for(size_t k = 0; k < _entries.size(); k++)
        {
            auto& currentTitle = _entries[k].lower_title;
            if(currentTitle.find(lowerQuery) != std::string::npos)
            {
                result.push_back(k);
            }
        }

        return result;
    }

    std::vector<audio_id_t> audio_directory::get_audio_ids_by_tag(const std::vector<std::string>& queries) const
    {
        // normalize query strings
        std::vector<std::string> toLowerQueries;
        for(const auto& currentQuery : queries)
        {
            std::string lowerQuery(currentQuery);
            std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
            toLowerQueries.push_back(lowerQuery);
        }

        // start with entire set of tracks to prune from
        std::unordered_set<audio_id_t> resultSet;
        for(size_t id = 0; id < _entries.size(); id++)
        {
            resultSet.insert(id);
        }

        // iterate over query tags
        for(const auto& currentQuery : toLowerQueries)
        {
            bool foundAnyMatch = false;
            // iterate over all existing tags
            for(const auto& currentPair : _tag_map)
            {
                //std::cout << " searching for " << currentQuery << " in " << currentPair.first << std::endl;
                if(currentPair.first.find(currentQuery) != std::string::npos)
                {
                    foundAnyMatch = true;
                    // prune result idx if they do not exist in this tag's set
                    for(auto trackIt = resultSet.begin();
                        trackIt != resultSet.end();)
                    {
                        // current id in our result set not found in this tag bucket, so remove
                        if(currentPair.second.find(*trackIt) == currentPair.second.end())
                        {
                            trackIt = resultSet.erase(trackIt);
                        }
                        // continue search
                        else
                        {
                            ++trackIt;
                        }
                    }

                    // early out if we've pruned the entire result set
                    if(resultSet.size() == 0)
                    {
                        foundAnyMatch = false;
                        break;
                    }                
                }
            }

            // all query tags must get a match
            if(foundAnyMatch == false)
            {
                resultSet.clear();
                break;
            }
        }

        // return track ids in list
        std::vector<audio_id_t> result(resultSet.begin(), resultSet.end());
        return result;
    }    
}