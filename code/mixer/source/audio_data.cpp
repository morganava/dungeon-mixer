#include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "audio.hpp"
#include "work_queue.hpp"

namespace tabletopify
{
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

    sf::SoundBuffer& one_shot_data::get_sound_buffer(bool mono)
    {
        assert(mono == true);
        if(_loaded == false)
        {
            this->load();
        }

        auto index = worker(Worker_Background).nextIndex(_buffers.size());
        return _buffers[index];
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

    loop_data::~loop_data()
    {

    }

    sf::SoundBuffer& loop_data::get_sound_buffer(bool mono)
    {
        return mono ? _mono_loop_buffer : _loop_buffer;
    }

    void loop_data::load()
    {
        _loaded = true;
    }

    void loop_data::unload()
    {
        _loaded = false;
    }	
}