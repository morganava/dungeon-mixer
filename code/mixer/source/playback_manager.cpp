#include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "audio.hpp"
#include "work_queue.hpp"

namespace tabletopify
{
    single_threaded<playback_manager> g_playback_manager;

    void playback_manager::update(float dt)
    {
        for(auto& it : _entries)
        {
            auto& playbackEntry = it.second;
            printf("audio_data::state(): %i\n", playbackEntry.audioData->state());

            if(playbackEntry.audioData->state() != LOADED)
            {
                printf("Pending load!\n");
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

    void playback_manager::play_atmosphere(atmosphere_id_t atmosphereId)
    {
        auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereId);
        for(auto trackId : atmosphereEntry.tracks)
        {
            this->play_track(trackId);
        }
    }

    void playback_manager::stop_atmosphere(atmosphere_id_t atmosphereId)
    {
        auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereId);
        for(auto trackId : atmosphereEntry.tracks)
        {
            this->stop_track(trackId);
        }
    }

    // track methods

    void playback_manager::play_track(track_id_t trackId)
    {
        printf("Playing: %lu\n", trackId);

        assert(_entries.find(trackId) == _entries.end());

        auto& trackEntry = g_track_directory->get_entry(trackId);
        auto& audioEntry = g_audio_directory->get_entry(trackEntry.audioId);

        playback_entry playbackEntry;
        playbackEntry.audioData = audioEntry.data;
        _entries.emplace(trackId, std::move(playbackEntry));

        this->sync_track(trackId);
    }

    void playback_manager::stop_track(track_id_t trackId)
    {
        auto it = _entries.find(trackId);
        assert(it != _entries.end());
        it->second.sound.stop();
        _entries.erase(it);
    }

    void playback_manager::sync_track(track_id_t trackId)
    {
        auto& trackEntry = g_track_directory->get_entry(trackId);
        auto it = _entries.find(trackId);
        assert(it != _entries.end());
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

    void playback_manager::stop_all_tracks()
    {
        for(auto& it : _entries)
        {
            it.second.sound.stop();
        }
        _entries.clear();
    }
}