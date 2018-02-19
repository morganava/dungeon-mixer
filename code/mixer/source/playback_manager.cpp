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

            if(playbackEntry.dirty == true)
            {
                playbackEntry.sound.setAttenuation(0.0f);
                playbackEntry.sound.setVolume(100.0f * playbackEntry.volume);
                if(playbackEntry.positional == true)
                {
                    playbackEntry.sound.setPosition(playbackEntry.position);
                }
                playbackEntry.sound.setLoop(playbackEntry.seamlessLoop);

                playbackEntry.dirty = false;
            }

            if(playbackEntry.sound.getStatus() == sf::SoundSource::Stopped)
            {
                if(playbackEntry.timeSincePlaybackEnd == 0.0f)
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

    void playback_manager::play(atmosphere_id_t atmosphereId)
    {
        this->play_atmosphere(atmosphereId);
    }

    void playback_manager::stop(atmosphere_id_t atmosphereId)
    {
        this->stop_atmosphere(atmosphereId);
    }

    void playback_manager::sync(track_id_t trackId)
    {
        this->sync_track(trackId);
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
        assert(_entries.find(trackId) == _entries.end());

        auto& trackEntry = g_track_directory->get_entry(trackId);
        auto& audioEntry = g_audio_directory->get_entry(trackEntry.audioId);

        playback_entry playbackEntry;
        playbackEntry.audioData = audioEntry.data;
        _entries.emplace(trackId, std::move(playbackEntry));

        this->sync(trackId);        
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
                playbackEntry.repeatMin = trackEntry.oneShot.repeatMin;
                playbackEntry.repeatMax = trackEntry.oneShot.repeatMax;
                playbackEntry.positional = true;
                playbackEntry.position = {trackEntry.oneShot.x, trackEntry.oneShot.y, trackEntry.oneShot.z};
                break;
            case LOOP:
                playbackEntry.seamlessLoop = true;
                playbackEntry.repeatMin = 0.0f;
                playbackEntry.repeatMax = 0.0f;
                playbackEntry.positional = false;
                break;
        }

        playbackEntry.timeSincePlaybackEnd = 0.0f;
        playbackEntry.dirty = true;
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