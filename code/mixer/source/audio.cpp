#include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "audio.hpp"

namespace tabletopify
{
    single_threaded<audio_directory> g_audio_directory;
    single_threaded<track_directory> g_track_directory;
    single_threaded<atmosphere_directory> g_atmosphere_directory;

    static void verify_file(std::string& path)
    {
        auto cpath = path.c_str();

        auto* file = fopen(cpath, "rb");
        if(file == nullptr)
        {
            throw std::runtime_error(string_format("Could not open file \"%s\"", cpath));
        }
        fclose(file);
        printf("Verified: %s\n", cpath);
    }

    static void to_mono(const sf::SoundBuffer& sourceSound, sf::SoundBuffer& destSound)
    {
        int32_t channels = static_cast<int32_t>(sourceSound.getChannelCount());

        // try and early out if possible
        if(channels == 1)
        {
            // noop
            if(&sourceSound == &destSound)
            {
                return;
            }
            // copy to dest
            else
            {
                destSound = sourceSound;
                return;
            }
        }

        // convert to mono by averaging the samples
        uint64_t sample_count = sourceSound.getSampleCount() / channels;
        std::unique_ptr<int16_t[]> dest_samples(new int16_t[sample_count]);
        const int16_t* source_samples = sourceSound.getSamples();
        
        for(uint64_t i = 0; i < sample_count; i++)
        {
            int32_t value = 0;
            for(int c = 0; c < channels; c++)
            {
                value += source_samples[c];
            }
            // average the channels
            dest_samples[i] = static_cast<int16_t>(value / channels);

            // increment source buffer
            source_samples += channels;
        }

        const uint32_t sample_rate = sourceSound.getSampleRate();

        printf("Sample Count: %lu Channels: %i Sample Rate: %u\n", sample_count, channels, sample_rate);

        destSound.loadFromSamples(dest_samples.release(), sample_count, 1, sample_rate);
    }

    // One-Shot Data

    one_shot_data::one_shot_data(std::vector<std::string>&& paths)
    : _paths(paths)
    { 
        for(auto& currentPath : _paths)
        {
            verify_file(currentPath);
        }

        for(size_t k = 0; k < _paths.size(); k++)
        {
            _buffers.push_back(sf::SoundBuffer());
        }
    }

    one_shot_data::~one_shot_data()
    {

    }

    void one_shot_data::load()
    {
        for(size_t k = 0; k < _paths.size(); k++)
        {
            const auto& currentPath = _paths[k];
            // load sample
            auto& currentSample = _buffers[k];
            currentSample.loadFromFile(currentPath);
            to_mono(currentSample, currentSample);
        }
        _loaded = true;        
    }

    void one_shot_data::unload()
    {
        for(auto& currentBuffer : _buffers)
        {
            currentBuffer = sf::SoundBuffer();
        }
        _loaded = false;
    }

    // Loop Data

    loop_data::loop_data(std::string&& loop_path)
    : _loop_path(loop_path)
    { 
        verify_file(_loop_path);
    }

    loop_data::loop_data(std::string&& intro_path, std::string&& loop_path)
    : _intro_path(intro_path)
    , _loop_path(loop_path)
    {
        verify_file(_intro_path.value());
    }

    loop_data::~loop_data()
    {

    }

    void loop_data::load()
    {
        _loaded = true;
    }

    void loop_data::unload()
    {
        _loaded = false;
    }

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

    /// Atmosphere Directory

    std::vector<atmosphere_id_t> atmosphere_directory::get_atmosphere_ids() const
    {
        std::vector<atmosphere_id_t> ids;
        for(const auto& it : _entries)
        {
            ids.push_back(it.first);
        }
        return ids;
    }

    std::vector<atmosphere_id_t> atmosphere_directory::get_atmosphere_ids_by_title(const std::string& title_query) const
    {
        std::vector<atmosphere_id_t> result;
        
        // first search for exact match
        for(const auto& it : _entries)
        {
            if(it.second.title == title_query)
            {
                auto id = it.first;
                result.push_back(id);
            }
        }
        // used to ensure we don't have duplicates
        std::unordered_set<atmosphere_id_t> resultSet(result.begin(), result.end());

        // next look for subset match
        for(const auto& it : _entries)
        {
            // first ensure we haven't added this result yet
            if(resultSet.find(it.first) == resultSet.end())
            {
                if(contains(to_lower(it.second.title), title_query))
                {
                    auto id = it.first;
                    result.push_back(id);
                    resultSet.insert(id);
                }
            }
        }

        // finally look for 'tag' match
        auto tags_query = split(to_lower(title_query), " ");
        for(const auto& it : _entries)
        {
            // first ensure we haven't added this result yet
            if(resultSet.find(it.first) == resultSet.end())
            {
                bool match = true;
                for(const auto& tag : tags_query)
                {
                    if(contains(to_lower(it.second.title), tag) == false)
                    {
                        match = false;
                        break;
                    }
                }

                if(match)
                {
                    auto id = it.first;
                    result.push_back(id);
                    // don't need to insert the current id into the resultSet, since
                    // we're guaranteed not to encounter it again
                }
            }
        }            

        return result;
    }

    atmosphere_entry& atmosphere_directory::get_entry(atmosphere_id_t id)
    {
        auto it = _entries.find(id);
        assert(it != _entries.end());

        return it->second;
    }

    atmosphere_id_t atmosphere_directory::add_entry(atmosphere_entry&& entry)
    {
        // update source and get id
        auto id = ++_next_id;

        // insert into map and increment the id source
        _entries.emplace(id, entry);

        // return id of entry just inserted
        return id;
    }

    void atmosphere_directory::remove_entry(atmosphere_id_t id)
    {
        // get reference to entry we're removing
        auto it = _entries.find(id);
        assert(it != _entries.end());

        // remove the entry 
        _entries.erase(it);
    }
}
