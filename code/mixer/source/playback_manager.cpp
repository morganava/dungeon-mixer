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

#define LOG_MEMBER(parent, param) std::cout << #param " : " << parent.param << std::endl;

            // LOG_MEMBER(playbackEntry, volume);
            // LOG_MEMBER(playbackEntry, seamlessLoop);
            // LOG_MEMBER(playbackEntry, repeatMin);
            // LOG_MEMBER(playbackEntry, repeatMax);
            // LOG_MEMBER(playbackEntry, positional);
            // LOG_MEMBER(playbackEntry, position);
            // LOG_MEMBER(playbackEntry, dirty);
            // LOG_MEMBER(playbackEntry, repeatDelay);
            // LOG_MEMBER(playbackEntry, timeSincePlaybackEnd);

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
                    LOG_MEMBER(playbackEntry, repeatDelay);
                }
                
                

                playbackEntry.timeSincePlaybackEnd += dt;
                
                if(playbackEntry.timeSincePlaybackEnd >= playbackEntry.repeatDelay)
                {
                    std::cout << "Playing!" << std::endl;
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
        auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereId);
        for(auto trackId : atmosphereEntry.tracks)
        {
            // verify this track isn't already being played
            assert(_entries.find(trackId) == _entries.end());

            auto& trackEntry = g_track_directory->get_entry(trackId);
            auto& audioEntry = g_audio_directory->get_entry(trackEntry.audioId);

            playback_entry playbackEntry;
            playbackEntry.audioData = audioEntry.data;
            _entries.emplace(trackId, std::move(playbackEntry));

            this->sync(trackId);
        }
    }

    void playback_manager::stop(atmosphere_id_t atmosphereId)
    {
        auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereId);
        for(auto trackId : atmosphereEntry.tracks)
        {
            auto it = _entries.find(trackId);
            it->second.sound.stop();
            _entries.erase(it);
        }
    }

    void playback_manager::sync(track_id_t trackId)
    {
        std::cout << "SYNCING!" << std::endl;

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
}   