// tabletopify
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "random.hpp"
#include "geometry.hpp"

namespace tabletopify
{
    void playback_manager::update(float dt)
    {
        for(auto& it : _entries)
        {
            auto& playbackEntry = it.second;
            if(playbackEntry.audioData->state() != audio_data_state::loaded)
            {
                continue;
            }

            if(playbackEntry.dirtyProperties == true)
            {
                playbackEntry.sound.setAttenuation(1.0f);
                playbackEntry.sound.setMinDistance(1.0f);
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

                playbackEntry.dirtyBuffer = false;
            }

            if(playbackEntry.sound.getStatus() == sf::SoundSource::Stopped)
            {
                if(playbackEntry.seamlessLoop == false && playbackEntry.timeSincePlaybackEnd == 0.0f)
                {
                    playbackEntry.repeatDelay = g_random.next_float(playbackEntry.repeatMin, playbackEntry.repeatMax);
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
        MX_LOG_INFO("playing atmosphere {}", atmosphereHandle);
        auto& atmosphereEntry = g_atmosphere_directory.get_entry(atmosphereHandle);
        for(auto trackHandle : atmosphereEntry.tracks)
        {
            this->play_track(trackHandle);
        }
    }

    void playback_manager::stop_atmosphere(atmosphere_handle_t atmosphereHandle)
    {
        MX_LOG_INFO("stopping atmosphere {}", atmosphereHandle);
        auto& atmosphereEntry = g_atmosphere_directory.get_entry(atmosphereHandle);
        for(auto trackHandle : atmosphereEntry.tracks)
        {
            this->stop_track(trackHandle);
        }
    }

    void playback_manager::set_atmosphere_volume(atmosphere_handle_t handle, float volume)
    {
        MX_ASSERT(volume >= 0.0f && volume <= 1.0f);

        auto& atmosphereEntry = g_atmosphere_directory.get_entry(handle);
        atmosphereEntry.volume = volume;
        sync_atmosphere(atmosphereEntry);
    }

    void playback_manager::set_atmosphere_mute(atmosphere_handle_t handle, bool muted)
    {
        auto& atmosphereEntry = g_atmosphere_directory.get_entry(handle);
        atmosphereEntry.muted = muted;
        sync_atmosphere(atmosphereEntry);

    }

    void playback_manager::sync_atmosphere(atmosphere_handle_t handle)
    {
        auto& atmosphereEntry = g_atmosphere_directory.get_entry(handle);
        sync_atmosphere(atmosphereEntry);
    }

    void playback_manager::sync_atmosphere(const atmosphere_entry& entry)
    {
        for(const auto trackHandle : entry.tracks)
        {
            sync_track(trackHandle);
        }
    }

    // track methods

    void playback_manager::play_track(track_handle_t trackHandle)
    {
        MX_LOG_INFO("playing track {}", trackHandle);

        MX_ASSERT(_entries.find(trackHandle) == _entries.end());

        auto& trackEntry = g_track_directory.get_entry(trackHandle);

        playback_entry playbackEntry;
        playbackEntry.audioData = trackEntry.audioData;
        _entries.emplace(trackHandle, std::move(playbackEntry));

        this->sync_track(trackHandle);
    }

    void playback_manager::stop_track(track_handle_t trackHandle)
    {
        MX_LOG_INFO("stopping track {}", trackHandle);

        auto it = _entries.find(trackHandle);
        if (it != _entries.end())
        {
            it->second.sound.stop();
            _entries.erase(it);
        }
    }

    void playback_manager::sync_track(track_handle_t trackHandle)
    {
        auto& trackEntry = g_track_directory.get_entry(trackHandle);
        auto it = _entries.find(trackHandle);
        // if this track isn't playing nothing to do
        if(it != _entries.end())
        {
            auto& atmosphereEntry = g_atmosphere_directory.get_entry(trackEntry.atmosphereHandle);
            auto& playbackEntry = it->second;

            playbackEntry.volume = (atmosphereEntry.muted ? 0.0f : atmosphereEntry.volume) * (trackEntry.muted ? 0.0f : trackEntry.volume);

            switch(playbackEntry.audioData->get_type())
            {
                case audio_type::one_shot:
                {
                    playbackEntry.seamlessLoop = false;
                    if (trackEntry.repeat.enabled)
                    {
                        playbackEntry.repeatMin = (float)trackEntry.repeat.min;
                        playbackEntry.repeatMax = (float)trackEntry.repeat.max;
                    }
                    else
                    {
                        playbackEntry.repeatMin = playbackEntry.repeatMax = std::numeric_limits<float>::max();
                    }

                    playbackEntry.dirtyBuffer = false;
                }
                break;
                case audio_type::loop:
                {
                    playbackEntry.seamlessLoop = true;
                    if(playbackEntry.positional != trackEntry.position.enabled)
                    {
                        playbackEntry.dirtyBuffer = true;
                    }
                    else
                    {
                        playbackEntry.dirtyBuffer = false;
                    }
                }
                break;
                default:
                    MX_FAIL_FAST();
            }

            // position calculation
            {
                const auto& [enabled, elevation, azimuth] = trackEntry.position;
                auto calc_cartesian = [&]()
                {
                    // forward vector
                    vec3 look = {0.0, 0.0, -1.0};
                    double theta = elevation * (constants<double>::pi / 180.0);
                    double phi = azimuth * (constants<double>::pi / 180.0);

                    // elevation
                    matrix3x3 Rx = {
                        1.0, 0.0, 0.0,
                        0.0, cos(theta), -sin(theta),
                        0.0, sin(theta), cos(theta),
                    };

                    // azimuth
                    // negate phi because we want 90 degrees to be to the right, not the left
                    matrix3x3 Ry = {
                        cos(-phi), 0.0, sin(-phi),
                        0.0, 1.0, 0.0,
                        -sin(-phi), 0.0, cos(-phi),
                    };

                    auto position = look * Rx * Ry;
                    return position;
                };

                playbackEntry.positional = enabled;
                playbackEntry.position = calc_cartesian();
            }

            playbackEntry.timeSincePlaybackEnd = 0.0f;
            playbackEntry.dirtyProperties = true;
        }
    }

    void playback_manager::stop_all_tracks()
    {
        MX_LOG_INFO("stopping all tracks");

        for(auto& it : _entries)
        {
            it.second.sound.stop();
        }
        _entries.clear();
    }
}