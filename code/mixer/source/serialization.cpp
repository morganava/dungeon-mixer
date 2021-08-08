#include "serialization.hpp"
#include "paths.hpp"
#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "hash.hpp"
#include "nanovg.hpp"
#include "imgui.hpp"
#include "color.hpp"
#include "stylesheet.hpp"
#include "ui.hpp"
#include "geometry.hpp"
#include "string_manager.hpp"

namespace dm
{
    namespace details
    {
        // must be called from audio thread
        nl::json atmosphere_entry_to_json(const dm::atmosphere_entry& atmosphereEntry)
        {
            SMOL_ASSERT(g_work_manager->current_thread() == worker_id::audio);

            // construct atmosphere object
            auto currentAtmosphereJson = nl::json::object();
            currentAtmosphereJson["Title"] = atmosphereEntry.title;
            if (atmosphereEntry.volume != 1.0) {
               currentAtmosphereJson["Volume"] = atmosphereEntry.volume;
            }
            if (atmosphereEntry.muted) {
                currentAtmosphereJson["Muted"] = atmosphereEntry.muted;
            }
            if (atmosphereEntry.favorite) {
                currentAtmosphereJson["Favorite"] = atmosphereEntry.favorite;
            }
            auto tracksJson = nl::json::array();

            for(auto trackHandle : atmosphereEntry.tracks)
            {
                const auto& trackEntry = g_track_directory->get_entry(trackHandle);
                const auto& audioHandle = trackEntry.audio;
                const auto& audioEntry = g_audio_directory->get_entry(audioHandle);
                auto trackJson = nl::json::object();
                trackJson["Id"] = audioEntry.id;
                if (trackEntry.volume != 1.0) {
                    trackJson["Volume"] = trackEntry.volume;
                }
                if (trackEntry.muted) {
                    trackJson["Muted"] = trackEntry.muted;
                }
                if (trackEntry.repeat.enabled) {
                    trackJson["RepeatRange"] = {trackEntry.repeat.min, trackEntry.repeat.max};
                }
                if (trackEntry.position.enabled) {
                    trackJson["Position"] = {trackEntry.position.elevation, trackEntry.position.azimuth};
                }
                tracksJson.push_back(std::move(trackJson));
            }
            currentAtmosphereJson["Tracks"] = std::move(tracksJson);

            return currentAtmosphereJson;
        }

        void write_json_to_disk(nl::json&& obj, fs::path&& dest)
        {
            // serialize json object to string
            auto tempDest = dest;
            tempDest.replace_extension(".json-temp");
            auto backupDest = dest;
            backupDest.replace_extension(".json-old");

            // writes to a temp file, then backs up the old file, and moves temp to dest location
            if(std::fstream fout(tempDest, std::ios::out | std::ios::binary); fout.is_open())
            {
                fout << std::setw(2) << std::setfill(' ') << obj;
                fout.close();
                if (fs::exists(dest))
                {
                    fs::rename(dest, backupDest);
                }
                fs::rename(tempDest, dest);
            }
            else
            {
                SMOL_THROW_MSG("Could not open {} for writing", tempDest);
            }
        }

        // strips out comments and trailing commas and then passes to json::parse
        nl::json parse_json(const memory_buffer& mb)
        {
            std::vector<uint8_t> js;
            js.reserve(mb.size());

            const uint8_t* read = mb.begin();
            const uint8_t* end = mb.end();

            while(read < end)
            {
                // string
                if (read[0] == '"')
                {
                    // advance until we get to end of string
                    js.push_back(*read);
                    read++;
                    // ensure we're not dealing with an escaped quote
                    while(read[0] != '"' ||
                          read[-1] == '\\')
                    {
                        js.push_back(*read);
                        read++;
                    }
                    js.push_back(*read);
                    read++;
                }
                // single line comment
                else if (read[0] == '/' &&
                         read + 1 < end &&
                         read[1] == '/')
                {
                    // skip over the '//'
                    read += 2;
                    // keep skipping until we get to the newline
                    while(read < end &&
                          read[0] != '\n') {
                        read++;
                    }
                }
                // block comment
                else if (read[0] == '/' &&
                         read +1 < end &&
                         read[1] == '*')
                {
                    // skip over the '/*'
                    read += 2;
                    // keep skipping until we get to the '*/'
                    while(read < end &&
                          (read[0] != '/' || read[-1] != '*'))
                    {
                        read++;
                    }
                    read++;
                }
                // trailing comma
                else if (read[0] == ',')
                {
                    const uint8_t* lookahead = read;
                    lookahead++;
                    bool done = false;
                    // skip over whitespace until
                    // we get to a } or ]
                    while(!done)
                    {
                        switch(*lookahead)
                        {
                            // whitespace
                            case ' ':
                            case '\t':
                            case '\r':
                            case '\n':
                                lookahead++;
                                break;
                            // found end of array or object
                            case ']':
                            case '}':
                                // push a space instead of comma
                                js.push_back(' ');
                                read++;
                                done = true;
                                break;
                            default:
                                // found something else, so we are
                                // in the middle
                                js.push_back(*read);
                                read++;
                                done = true;
                                break;
                        }
                    }
                }
                // everything else
                else
                {
                    js.push_back(*read);
                    read++;
                }
            }

            // second pass to overwrite trailing commas

            for(auto it = js.begin(); it < js.end(); it++)
            {
                if (*it == ',')
                {
                    auto comma = it;
                    bool done = false;
                    while(!done)
                    {
                        it++;
                        switch(*it)
                        {
                            // whitespace
                            case ' ':
                            case '\t':
                            case '\r':
                            case '\n':
                                break;
                            case ']':
                            case '}':
                                *comma = ' ';
                            default:
                                done = true;
                                break;
                        }
                    }
                }
            }

            return nl::json::parse(js.begin(), js.end());
        }
    }

    constexpr const char MIX_VERSION[] = "0.1";

    void serialization_manager::request_autosave(content_type_t content)
    {
        const auto now = std::chrono::system_clock::now();
        worker_fileio.enqueue([=]() -> void
        {
            const auto index = static_cast<size_t>(content);
            auto& entry = g_serialization_manager->record[index];
            entry.autosaveRequested = true;
            entry.timeRequested = now;

            worker_fileio.enqueue([=]() -> void
            {
                auto& entry = g_serialization_manager->record[index];
                if (entry.autosaveRequested)
                {
                    const auto now = std::chrono::system_clock::now();
                    const std::chrono::duration<double> duration(now - entry.timeRequested);

                    if (duration >= serialization_manager::AUTOSAVE_DELAY)
                    {
                        g_serialization_manager->autosave(content);
                        entry.autosaveRequested = false;
                    }
                }


            }, serialization_manager::AUTOSAVE_DELAY);
        });
    }

    void serialization_manager::autosave(content_type_t content)
    {
        switch(content)
        {
            case content_type::playlist:
                worker_audio.enqueue([]() -> void
                {
                    g_serialization_manager->save_all_playlists(paths::config() / "autosaved.json");
                });

                break;
            default:
                break;
        }
    }

    void serialization_manager::load_stylesheet(fs::path&& source)
    {
        worker_fileio.enqueue([source=std::move(source)]() -> void
        {
            MX_LOG_INFO("Loading skin: {}", source);

            memory_buffer mb(source);

            worker_background.enqueue([mb=std::move(mb)]() -> void
            {
                auto json = details::parse_json(mb);
                worker_ui.enqueue([json=std::move(json)] () mutable -> void
                {
                    // this is here so that we can reload skins at runtime
                    // we can just overwrite the existing resolver and things
                    // should jus work
                    auto resolver = json.get<style_resolver>();
                    g_style_resolver = std::move(resolver);
                });
            });
        });
    }

    void serialization_manager::load_audio_manifests()
    {
        worker_fileio.enqueue([]() -> void
        {
            // iterate over each subdirectory in the library directory
            for(auto& it : fs::directory_iterator(paths::libraries()))
            {
                if (!fs::is_directory(it.path())) {
                    MX_LOG_INFO("Skipping: {}" , it.path());
                    continue;
                }

                auto manifestPath = it.path() / "manifest.json";
                MX_LOG_INFO("Loading manifest: {}", manifestPath);
                memory_buffer manifestData(manifestPath);

                worker_background.enqueue([basePath=it.path(),manifestData=std::move(manifestData)]() -> void
                {
                    auto json = details::parse_json(manifestData);

                    // create audio entries for listings in manifest
                    worker_audio.enqueue([basePath=std::move(basePath), json=std::move(json)]() -> void
                    {
                        auto trackList = json["Tracks"];
                        for(auto track : trackList)
                        {
                            utf8_string title = track["Title"];
                            uuid audioId = track["AudioId"];
                            utf8_string type = track["Type"];
                            std::vector<utf8_string> tags = track["Tags"];
                            std::vector<utf8_string> filenames = track["Files"];
                            std::vector<fs::path> files;
                            for(auto& currentFilename : filenames)
                            {
                                files.push_back(basePath / currentFilename.str());
                            }

                            std::shared_ptr<audio_data> audioData;

                            if(type == "OneShot")
                            {
                                audioData = std::make_shared<one_shot_data>(std::move(files));
                            }
                            else if(type == "Loop")
                            {
                                audioData = std::make_shared<loop_data>(std::move(files.front()));
                            }

                            g_audio_directory->add_entry(
                                std::move(audioId),
                                std::move(title),
                                std::move(tags),
                                std::move(audioData));
                        }
                    });
                });
            }
        });
    }

    void serialization_manager::load_playlist(fs::path&& playlistPath)
    {
        worker_fileio.enqueue([playlistPath=std::move(playlistPath)]() -> void
        {
            MX_LOG_INFO("Loading playlist: {}", playlistPath);
            memory_buffer bytes(playlistPath);

            worker_background.enqueue([bytes=std::move(bytes)]() -> void
            {
                auto json = details::parse_json(bytes);

                worker_audio.enqueue([json=std::move(json)]() -> void
                {
                    utf8_string version = json["Version"];
                    MX_LOG_INFO(" Version : {}", version);
                    MX_LOG_INFO(" Atmospheres :");

                    std::vector<atmosphere_handle_t> id_to_handle;

                    // parse our atmosphere_entry list
                    auto atmosphereList = json["Atmospheres"];
                    // for (auto atmosphereJson : atmosphereList)
                    for(size_t k = 0; k < atmosphereList.size(); k++)
                    {
                        auto& atmosphereJson = atmosphereList[k];
                        atmosphere_entry atmosphereEntry;
                        atmosphereEntry.title = atmosphereJson["Title"].get<utf8_string>();

                        MX_LOG_INFO("  AtmosphereTitle : {}", atmosphereEntry.title);

                        if (atmosphereJson.contains("Volume")) {
                            atmosphereEntry.volume = atmosphereJson["Volume"].get<float>();
                        }
                        MX_LOG_INFO("  Volume : {}", atmosphereEntry.volume);

                        if (atmosphereJson.contains("Muted")) {
                            atmosphereEntry.muted = atmosphereJson["Muted"].get<bool>();
                        }
                        MX_LOG_INFO("  Muted : {}", atmosphereEntry.muted);

                        if (atmosphereJson.contains("Favorite")) {
                            atmosphereEntry.favorite = atmosphereJson["Favorite"].get<bool>();
                        }
                        MX_LOG_INFO("  Favorite : {}", atmosphereEntry.favorite);


                        auto trackList = atmosphereJson["Tracks"];
                        for (auto trackJson : trackList)
                        {
                            track_entry trackEntry;

                            // Id is the id of the audio_entry referenced
                            const auto audioId = trackJson["Id"].get<uuid>();
                            auto audioHandle = g_audio_directory->get_audio_handle_by_id(audioId);

                            MX_LOG_INFO("   Id : {}", audioId);
                            MX_LOG_INFO("    AudioHandle : {}", audioHandle);

                            if (audioHandle == audio_handle::invalid) {
                                MX_LOG_ERROR("AudioData with id '{}' does not exist", audioId);
                                continue;
                            }

                            // get rest of data from audio directory
                            auto& audioEntry = g_audio_directory->get_entry(audioHandle);

                            trackEntry.audio = audioHandle;
                            trackEntry.volume = 1.0f;
                            trackEntry.muted = false;
                            trackEntry.repeat.enabled = false;
                            trackEntry.position.enabled = false;


                            MX_LOG_INFO("    AudioTitle : {}", audioEntry.title);

                            if (trackJson.contains("Volume")) {
                                trackEntry.volume = trackJson["Volume"].get<float>();
                            }
                            MX_LOG_INFO("    Volume : {}", trackEntry.volume);
                            if (trackJson.contains("Muted")) {
                                trackEntry.muted = trackJson["Muted"].get<bool>();
                            }
                            MX_LOG_INFO("    Muted : {}", trackEntry.muted);

                            if(audioEntry.type() == audio_type::one_shot)
                            {
                                if (trackJson.contains("RepeatRange")) {
                                    auto range = trackJson["RepeatRange"];
                                    if (range.is_array() &&
                                        range.size() == 2 &&
                                        range[0].is_number() &&
                                        range[1].is_number())
                                    {
                                        float range0 = range[0].get<float>();
                                        float range1 = range[1].get<float>();

                                        trackEntry.repeat.enabled = true;
                                        trackEntry.repeat.min = std::min(range0, range1);
                                        trackEntry.repeat.max = std::max(range0, range1);

                                        MX_LOG_INFO("    RepeatMin : {}", trackEntry.repeat.min);
                                        MX_LOG_INFO("    RepeatMax : {}", trackEntry.repeat.max);
                                    }
                                }
                            }

                            if (trackJson.contains("Position")) {
                                auto positionJson = trackJson["Position"];
                                auto& [enabled, elevation, azimuth] = trackEntry.position;
                                if (positionJson.is_array())
                                {
                                    SMOL_THROW_IF_FALSE(positionJson.size() == 2);
                                    switch(positionJson.size())
                                    {
                                        case 2:
                                            SMOL_THROW_IF_FALSE(positionJson[1].is_number());
                                            azimuth = positionJson[1].get<double>();
                                        case 1:
                                            SMOL_THROW_IF_FALSE(positionJson[0].is_number());
                                            elevation = positionJson[0].get<double>();
                                    }
                                }
                                else if (positionJson.is_object())
                                {
                                    if (positionJson.contains("Azimuth"))
                                    {
                                        const auto& azimuthJson = positionJson["Azimuth"];
                                        SMOL_THROW_IF_FALSE(azimuthJson.is_number());
                                        azimuth = azimuthJson.get<double>();
                                    }
                                    if (positionJson.contains("Elevation"))
                                    {
                                        const auto& elevationJson = positionJson["Elevation"];
                                        SMOL_THROW_IF_FALSE(elevationJson.is_number());
                                        elevation = elevationJson.get<double>();
                                    }
                                }
                                trackEntry.position.enabled = true;

                                MX_LOG_INFO("    Position : {{ elevation : {}, azimuth : {} }}", elevation, azimuth);
                            }

                            auto trackHandle = g_track_directory->add_entry(std::move(trackEntry));
                            atmosphereEntry.tracks.push_back(trackHandle);
                        }

                        const bool favorite = atmosphereEntry.favorite;
                        const auto atmosphereHandle = g_atmosphere_directory->add_entry(std::move(atmosphereEntry));
                        id_to_handle.push_back(atmosphereHandle);
                        if (favorite) {
                            g_playlist_directory->add_atmosphere(playlist_handle::favorites, atmosphereHandle);
                        }
                        g_playlist_directory->add_atmosphere(playlist_handle::all, atmosphereHandle);
                    }

                    // parse our playlist_entry list
                    MX_LOG_INFO(" Playlists :");
                    auto playlistList = json["Playlists"];
                    for (auto playlistJson : playlistList)
                    {
                        playlist_entry playlistEntry;
                        playlistEntry.title = playlistJson["Title"].get<utf8_string>();
                        MX_LOG_INFO("  PlaylistTitle : {}", playlistEntry.title);
                        MX_LOG_INFO("  Atmospheres : ");

                        auto playlistHandle = g_playlist_directory->add_entry(std::move(playlistEntry));

                        auto atmospheresList = playlistJson["Atmospheres"];
                        for (auto atmosphereId : atmospheresList)
                        {
                            const auto id = atmosphereId.get<size_t>();
                            SMOL_THROW_IF_FALSE(id < id_to_handle.size());
                            auto atmosphereHandle = id_to_handle[id];
                            MX_LOG_INFO("   Id : {}", id);
                            g_playlist_directory->add_atmosphere(playlistHandle, atmosphereHandle);
                        }
                    }

                    worker_ui.enqueue([]()
                    {
                        g_ui->sync_playlists();
                    });
                });
            });
        });
    }

    void serialization_manager::load_strings(fs::path&& source)
    {
        worker_fileio.enqueue([source=std::move(source)]() -> void
        {
            MX_LOG_INFO("Loading strings: {}", source);
            memory_buffer bytes(source);

            worker_background.enqueue([bytes=std::move(bytes)]() -> void
            {
                auto json = details::parse_json(bytes);
                string_manager sm;

                for(auto e : enum_values<mixer_string>())
                {
                    size_t idx = static_cast<size_t>(e);
                    if (auto it = json.find(dm::to_string_view(e)); it != json.end() && it->is_string())
                    {
                        sm.strings[idx] = it->get<utf8_string>();
                    }
                    else
                    {
                        sm.strings[idx] = dm::to_string_view(e);
                    }
                    MX_LOG_INFO(" {} : '{}'", e, sm.strings[idx]);
                }

                worker_ui.enqueue([sm=std::move(sm)]() mutable -> void
                {
                    g_string_manager = std::move(sm);
                });
            });
        });
    }

    void serialization_manager::save_playlist(fs::path&& dest, playlist_handle_t playlistHandle)
    {
        worker_audio.enqueue([=,dest=std::move(dest)]() -> void
        {
            const auto& playlistEntry = g_playlist_directory->get_entry(playlistHandle);

            // convert playlist, atmosphere and track entries into an nl::json object
            auto atmospheresJson = nl::json::array();
            auto playlistsJson = nl::json::array();
            auto currentPlaylistJson = nl::json::object();
            auto currentPlaylistAtmospheresJson = nl::json::array();

            currentPlaylistJson["Title"] = playlistEntry.title;

            size_t atmosphereId = 0;
            for(auto atmosphereHandle : playlistEntry.atmospheres)
            {
                const auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereHandle);

                // convert the entry to serialized json
                auto currentAtmosphereJson = details::atmosphere_entry_to_json(atmosphereEntry);
                if (auto it = currentAtmosphereJson.find("Favorite"); it != currentAtmosphereJson.end())
                {
                    currentAtmosphereJson.erase(it);
                }

                // add json to our list
                atmospheresJson.push_back(std::move(currentAtmosphereJson));

                // push atmosphere id to playlist
                currentPlaylistAtmospheresJson.push_back(atmosphereId++);
            }
            currentPlaylistJson["Atmospheres"] = std::move(currentPlaylistAtmospheresJson);

            playlistsJson.push_back(std::move(currentPlaylistJson));

            auto objectJson = nl::json::object();
            objectJson["Version"] = MIX_VERSION;
            objectJson["Atmospheres"] = std::move(atmospheresJson);
            objectJson["Playlists"] = std::move(playlistsJson);

            worker_fileio.enqueue([objectJson=std::move(objectJson),dest=std::move(dest)]() mutable -> void
            {
                details::write_json_to_disk(std::move(objectJson), std::move(dest));
            });
        });
    }

    void serialization_manager::save_all_playlists(fs::path&& dest)
    {
        MX_LOG_INFO("Saving playlists to {}", dest.string());
        worker_audio.enqueue([dest=std::move(dest)]() -> void
        {
            auto atmospheresJson = nl::json::array();
            auto playlistsJson = nl::json::array();

            // maps our runtime handle to our offset into Atmospheres list in json
            std::map<atmosphere_handle_t, size_t> atmosphereHandleToIndex;

            // write all of our atmospheres to json
            size_t atmosphereIndex = 0;
            for(const auto atmosphereHandle : g_atmosphere_directory->get_atmosphere_handles())
            {
                const auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereHandle);

                // convert the entry to serialized json
                auto currentAtmosphereJson = details::atmosphere_entry_to_json(atmosphereEntry);

                // add json to our list
                atmospheresJson.push_back(std::move(currentAtmosphereJson));

                // push atmosphere id to playlist and increment index
                atmosphereHandleToIndex.insert({atmosphereHandle, atmosphereIndex++});
            }

            for(const auto playlistHandle : g_playlist_directory->get_user_playlist_handles())
            {
                auto currentPlaylistJson = nl::json::object();
                auto currentPlaylistAtmospheresJson = nl::json::array();

                const auto& playlistEntry = g_playlist_directory->get_entry(playlistHandle);
                currentPlaylistJson["Title"] = playlistEntry.title;
                for(const auto atmosphereHandle : playlistEntry.atmospheres)
                {
                    auto it = atmosphereHandleToIndex.find(atmosphereHandle);
                    SMOL_ASSERT(it != atmosphereHandleToIndex.end());

                    currentPlaylistAtmospheresJson.push_back(it->second);
                }
                currentPlaylistJson["Atmospheres"] = std::move(currentPlaylistAtmospheresJson);
                playlistsJson.push_back(std::move(currentPlaylistJson));
            }

            auto objectJson = nl::json::object();
            objectJson["Version"] = MIX_VERSION;
            objectJson["Atmospheres"] = std::move(atmospheresJson);
            objectJson["Playlists"] = std::move(playlistsJson);

            worker_fileio.enqueue([objectJson=std::move(objectJson),dest=std::move(dest)]() mutable -> void
            {
                details::write_json_to_disk(std::move(objectJson), std::move(dest));
            });
        });
    }
}