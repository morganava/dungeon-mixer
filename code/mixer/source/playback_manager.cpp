// tabletopify
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"


namespace tabletopify
{
    playback_manager* g_playback_manager = nullptr;
    playback_manager* get_playback_manager()
    {
        MX_ASSERT(worker(Worker_Background).getThreadId() == std::this_thread::get_id());
        return g_playback_manager;
    }
    void init_playback_manager()
    {
        MX_ASSERT(g_playback_manager == nullptr);
        g_playback_manager = new playback_manager();
    }
    void free_playback_manager()
    {
        delete g_playback_manager;
        g_playback_manager = nullptr;
    }

    extern track_directory* g_track_directory;
    extern atmosphere_directory* g_atmosphere_directory;

    void playback_manager::update(float dt)
    {
        for(auto& it : _entries)
        {
            auto& playbackEntry = it.second;
            if(playbackEntry.audioData->state() != LOADED)
            {
                continue;
            }

            if(playbackEntry.dirtyProperties == true)
            {
                playbackEntry.sound.setAttenuation(0.0f);
                playbackEntry.sound.setVolume(100.0f * playbackEntry.volume);
                if(playbackEntry.positional == true)
                {
                    playbackEntry.sound.setPosition(playbackEntry.position);
                }
                else
                {
                    playbackEntry.sound.setPosition({0.0f, 0.0f, 0.0f});
                }
                playbackEntry.sound.setLoop(playbackEntry.seamlessLoop);

                playbackEntry.dirtyProperties = false;
            }

            if(playbackEntry.dirtyBuffer == true)
            {
                auto currentTimeOffset = playbackEntry.sound.getPlayingOffset();
                playbackEntry.sound.stop();
                auto& soundBuffer = playbackEntry.audioData->get_sound_buffer(playbackEntry.positional);
                playbackEntry.sound.setBuffer(soundBuffer);
                playbackEntry.sound.setPlayingOffset(currentTimeOffset);
                playbackEntry.sound.play();
            }

            if(playbackEntry.sound.getStatus() == sf::SoundSource::Stopped)
            {
                if(playbackEntry.seamlessLoop == false && playbackEntry.timeSincePlaybackEnd == 0.0f)
                {
                    playbackEntry.repeatDelay = worker(Worker_Background).nextFloat(playbackEntry.repeatMin, playbackEntry.repeatMax);
                }

                playbackEntry.timeSincePlaybackEnd += dt;

                if(playbackEntry.timeSincePlaybackEnd >= playbackEntry.repeatDelay)
                {
                    auto& soundBuffer = playbackEntry.audioData->get_sound_buffer(playbackEntry.positional);

                    playbackEntry.sound.setBuffer(soundBuffer);
                    playbackEntry.sound.play();
                    playbackEntry.timeSincePlaybackEnd = 0.0f;
                }
            }
        }
    }

    // atmosphere methods

    void playback_manager::play_atmosphere(atmosphere_handle_t atmosphereHandle)
    {
        auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereHandle);
        for(auto trackHandle : atmosphereEntry.tracks)
        {
            this->play_track(trackHandle);
        }
    }

    void playback_manager::stop_atmosphere(atmosphere_handle_t atmosphereHandle)
    {
        auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereHandle);
        for(auto trackHandle : atmosphereEntry.tracks)
        {
            this->stop_track(trackHandle);
        }
    }

    // track methods

    void playback_manager::play_track(track_handle_t trackHandle)
    {
        log(LogLevel_Info, "playing track handle %lu", trackHandle);

        MX_ASSERT(_entries.find(trackHandle) == _entries.end());

        auto& trackEntry = g_track_directory->get_entry(trackHandle);

        playback_entry playbackEntry;
        playbackEntry.audioData = trackEntry.audioData;
        _entries.emplace(trackHandle, std::move(playbackEntry));

        this->sync_track(trackHandle);
    }

    void playback_manager::stop_track(track_handle_t trackHandle)
    {
        auto it = _entries.find(trackHandle);
        MX_ASSERT(it != _entries.end());
        it->second.sound.stop();
        _entries.erase(it);
    }

    void playback_manager::sync_track(track_handle_t trackHandle)
    {
        auto& trackEntry = g_track_directory->get_entry(trackHandle);
        auto it = _entries.find(trackHandle);
        // if this track isn't playing nothing to do
        if(it != _entries.end())
        {
            auto& playbackEntry = it->second;

            playbackEntry.volume = (trackEntry.atmosphereMuted ? 0.0f : trackEntry.atmosphereVolume) * (trackEntry.muted ? 0.0f : trackEntry.volume);

            switch(playbackEntry.audioData->get_type())
            {
                case ONE_SHOT:
                    playbackEntry.seamlessLoop = false;
                    playbackEntry.repeatMin = trackEntry.repeat.min;
                    playbackEntry.repeatMax = trackEntry.repeat.max;
                    playbackEntry.dirtyBuffer = false;
                    break;
                case LOOP:
                    playbackEntry.seamlessLoop = true;
                    if(playbackEntry.positional != trackEntry.position.enabled)
                    {
                        playbackEntry.dirtyBuffer = true;
                    }
                    else
                    {
                        playbackEntry.dirtyBuffer = false;
                    }
                    break;
            }

            playbackEntry.positional = trackEntry.position.enabled;
            playbackEntry.position = {trackEntry.position.x, trackEntry.position.y, trackEntry.position.z};

            playbackEntry.timeSincePlaybackEnd = 0.0f;
            playbackEntry.dirtyProperties = true;
        }
    }

    void playback_manager::stop_all_tracks()
    {
        for(auto& it : _entries)
        {
            it.second.sound.stop();
        }
        _entries.clear();
    }
}