#include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "audio.hpp"

namespace tabletopify
{
    single_threaded<playback_manager> g_playback_manager;

    void playback_manager::update(float dt)
    {
        // for(auto it : _entries)
        // {
        //     auto& playbackEntry = it->second;

        //     if(playbackEntry.sound.getStatus() == Sf::SoundSource::Stopped &&
        //        playbackEntry.timeSincePlaybackEnd == 0.0f)
        //     {
        //         playbackEntry.timeSincePlaybackEnd += dt;
        //     }

        //     if(playbackEntry.timeSincePlaybackEnd >= playbackEntry.repeatDelay)
        //     {
        //         g_back
        //     }            
        // }
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
            playbackEntry.volume = atmosphereEntry.volume * (trackEntry.muted ? 0.0f : trackEntry.volume);

            switch(audioEntry.data->get_type())
            {
                case ONE_SHOT:
                    playbackEntry.repeatMin = trackEntry.oneShot.repeatMin;
                    playbackEntry.repeatMax = trackEntry.oneShot.repeatMax;
                    playbackEntry.positional = true;
                    playbackEntry.position = {trackEntry.oneShot.x, trackEntry.oneShot.y, trackEntry.oneShot.z};
                    break;
                case LOOP:
                    playbackEntry.repeatMin = 0.0f;
                    playbackEntry.repeatMax = 0.0f;
                    playbackEntry.positional = false;
                    break;
            }

            // playbackEntry.repeatDelay = g_background_random->next(playbackEntry.repeatMin, playbackEntry.repeatMax);
            playbackEntry.timeSincePlaybackEnd = 0.0f;

            playbackEntry.sound.setBuffer(audioEntry.data->get_sound_buffer(true));

            _entries.emplace(trackId, std::move(playbackEntry));
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

    }
}   