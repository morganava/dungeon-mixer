#include "precomp.hpp"

// tabletopify
#include "audio.hpp"
#include "utilities.hpp"

namespace tabletopify
{
    audio_directory* g_audio_directory = nullptr;
    atmosphere_directory* g_atmosphere_directory = nullptr;

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

    void audio_directory::add_entry(
        std::string&& uuid,
        std::string&& title, 
        std::vector<std::string>&& tags, 
        std::shared_ptr<audio_data>&& data)
    {
        size_t idx = _entries.size();

        // normalized lowercase title for search
        std::string lowerTitle(title.begin(), title.end());
        std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);    

        // update our tag map with provided tags
        for(auto& currentTag : tags)
        {
            std::transform(currentTag.begin(), currentTag.end(), currentTag.begin(), ::tolower);
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

    /// Atmosphere Directory

    std::vector<atmosphere_id_t> atmosphere_directory::get_atmosphere_ids() const
    {
        std::vector<atmosphere_id_t> ids;
        for(auto it : _entries)
        {
            ids.push_back(it.first);
        }
        return ids;
    }

    std::vector<atmosphere_id_t> atmosphere_directory::get_atmosphere_ids_by_title(const std::string& title_query) const
    {
        return std::vector<atmosphere_id_t>();
    }

    std::vector<atmosphere_id_t> atmosphere_directory::get_atmosphere_ids_by_tag(const std::vector<std::string>& queries) const
    {
        return std::vector<atmosphere_id_t>();
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

        // update the tag map
        for(auto tag : entry.tags)
        {
            _tag_map.insert({tag, id});
        }

        // insert into map and increment the id source
        _entries.insert({id, entry});

        // return id of entry just inserted
        return id;
    }

    void atmosphere_directory::remove_entry(atmosphere_id_t id)
    {
        // get reference to entry we're removing
        auto entry_it = _entries.find(id);
        auto& entry = entry_it->second;

        // remove it's tags from the tag map
        for(auto tag : entry.tags)
        {
            auto ii = _tag_map.equal_range(tag);
            for(auto it = ii.first; it != ii.second;)
            {
                if(it->second == id)
                {
                    _tag_map.erase(it);
                    break;
                }
            }
        }
        // remove the entry 
        _entries.erase(entry_it);
    }
}
