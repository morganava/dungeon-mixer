// C++
#include <string>
#include <cstdint>
#include <utility>
#include <memory>
#include <cstdio>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>

// SFML
#include <SFML/Audio.hpp> 

// tabletopify
#include "audio.hpp"
#include "utilities.hpp"

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

class one_shot_data : public audio_data
{
public:
    one_shot_data(std::vector<std::string>&& paths)
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

    ~one_shot_data() override {}
private:
    void load() override
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
    
    void unload() override 
    {
        for(auto& currentBuffer : _buffers)
        {
            currentBuffer = sf::SoundBuffer();
        }
        _loaded = false;
    }

    std::vector<std::string> _paths;
    std::vector<sf::SoundBuffer> _buffers;

    friend class one_shot_track;
};

class loop_data : public audio_data
{
public:    
    loop_data(std::string&& loop_path)
    : _loop_path(loop_path)
    { 
        verify_file(_loop_path);
    }

    loop_data(std::string&& intro_path, std::string&& loop_path)
    : _intro_path(intro_path)
    , _loop_path(loop_path)
    {
        verify_file(_intro_path.value());
    }

    ~loop_data() override {}
private:
    void load() override 
    {
        _loaded = true;
    }

    void unload() override 
    {
        _loaded = false;
    }

    std::optional<std::string> _intro_path;
    std::string _loop_path;

    std::optional<sf::SoundBuffer> _intro_buffer;
    sf::SoundBuffer _loop_buffer;

    std::optional<sf::SoundBuffer> _mono_intro_buffer;
    std::optional<sf::SoundBuffer> _mono_loop_buffer;

    friend class loop_track;
};

class one_shot_track : public track
{
public:    
    one_shot_track(std::string&& title, std::vector<std::string>&& tags, size_t index)
    {

    }
    ~one_shot_track() override {}
    void update() override {}
private:


};

class loop_track : public track
{
    ~loop_track() override {}
    void update() override {}
};

track_directory::track_directory()
{
    // one-shot
    std::vector<std::string> sawing_paths = {"/home/pospeselr/Downloads/sawing.wav"};
    add_entry(
        "37fbd587-ab8a-4e85-898c-be2c76d79440",
        "Sawing",
        {"construction", "saw", "shared"},
        std::make_shared<one_shot_data>(std::move(sawing_paths)));

    std::string seagull_path = "/home/pospeselr/Downloads/seagulls.wav";
    add_entry(
        "21b24cc6-d659-4c39-ae69-75d7938fd7b3",
        "Seagulls",
        {"seaside", "seagulls", "shared"},
        std::make_shared<loop_data>(std::move(seagull_path)));

    // temp remove this and load on demand
    for(auto& current_entry : _entries)
    {
        current_entry.data->load();
    }
}

void track_directory::add_entry(
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

void track_directory::update(float dt)
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

std::string track_directory::get_title_by_index(size_t idx) const
{
    return "";
}

std::vector<std::string> track_directory::get_tags_by_index(size_t idx) const
{
    return std::vector<std::string>();
}

std::unique_ptr<track> track_directory::get_track_by_index(size_t idx) const
{
    return std::unique_ptr<track>();
}

std::vector<size_t> track_directory::get_tracks_by_title(const std::string& title_query) const
{
    return std::vector<size_t>();
}

std::vector<size_t> track_directory::get_tracks_by_tag(const std::vector<std::string>& queries) const
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
    std::unordered_set<size_t> resultSet;
    for(size_t idx = 0; idx < _entries.size(); idx++)
    {
        resultSet.insert(idx);
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
                    if(currentPair.second.find(*trackIt) == currentPair.second.end())
                    {
                        trackIt = resultSet.erase(trackIt);
                    }
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

    std::vector<size_t> result(resultSet.begin(), resultSet.end());
    std::sort(result.begin(), result.end());
    return result;
}
