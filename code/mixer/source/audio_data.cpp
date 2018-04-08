#include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "serialization.hpp"

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

    // Base Audio Data
    void audio_data::load(std::shared_ptr<audio_data> audio)
    {
        printf("Loading Audio!\n");

        MX_ASSERT(audio->_state == UNLOADED);
        audio->_state = LOADING;

        worker(Worker_FileIO).enqueue([=]() -> void
        {
            audio->load_impl();
            audio->_state = LOADED;
        });
    }

    void audio_data::unload(std::shared_ptr<audio_data> audio)
    {
        printf("Unloading Audio!\n");
        MX_ASSERT(audio->_state == LOADED);
        audio->_state = UNLOADING;

        worker(Worker_FileIO).enqueue([=]() -> void
        {
            audio->unload_impl();
            // need to set this to PENDING_UNLOADED because at this point
            // the audio has 2 references to it, so if we set the flag to UNLOADED
            // the audio_directory::update loop could try and load it again
            audio->_state = PENDING_UNLOADED;
        });
    }

    // One-Shot Data

    one_shot_data::one_shot_data(std::vector<fs::path>&& paths)
    : _paths(paths)
    {
        for(auto& currentPath : _paths)
        {
            MX_ASSERT(fs::exists(currentPath));
        }

        for(size_t k = 0; k < _paths.size(); k++)
        {
            _buffers.push_back(sf::SoundBuffer());
        }
    }

    one_shot_data::~one_shot_data()
    {

    }

    sf::SoundBuffer& one_shot_data::get_sound_buffer(bool positionable)
    {
        MX_ASSERT(this->_state == LOADED);

        auto index = worker(Worker_Background).nextIndex(_buffers.size());
        return _buffers[index];
    }

    void one_shot_data::load_impl()
    {
        printf("Load Impl!\n");
        for(size_t k = 0; k < _paths.size(); k++)
        {
            const auto& currentPath = _paths[k];
            // load sample
            auto& currentSample = _buffers[k];
            currentSample.loadFromFile(currentPath);
            to_mono(currentSample, currentSample);
        }
    }

    void one_shot_data::unload_impl()
    {
        for(auto& currentBuffer : _buffers)
        {
            currentBuffer = sf::SoundBuffer();
        }
    }

    // Loop Data

    loop_data::loop_data(fs::path&& loop_path)
    : _loop_path(std::move(loop_path))
    {
        MX_ASSERT(fs::exists(_loop_path));
    }

    loop_data::~loop_data()
    {

    }

    sf::SoundBuffer& loop_data::get_sound_buffer(bool positionable)
    {
        MX_ASSERT(this->_state == LOADED);

        return positionable ? _mono_loop_buffer : _loop_buffer;
    }

    void loop_data::load_impl()
    {
        _loop_buffer.loadFromFile(_loop_path);
        to_mono(_loop_buffer, _mono_loop_buffer);
    }

    void loop_data::unload_impl()
    {
        _loop_buffer = sf::SoundBuffer();
        _mono_loop_buffer = sf::SoundBuffer();
    }
}