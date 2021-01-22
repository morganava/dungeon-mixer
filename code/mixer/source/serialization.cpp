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

namespace dm
{
    // todo: probably find a better place than this
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

    const char MIX_VERSION[] = "0.1";

    void load_stylesheet()
    {
        worker_fileio.enqueue([]() -> void
        {
            auto skinPath = paths::skins() / "default.json";
            MX_LOG_INFO("Loading skin: {}", skinPath);

            memory_buffer mb(skinPath);

            worker_background.enqueue([mb=std::move(mb)]() -> void
            {
                auto json = parse_json(mb);
                auto resolver = json.get<style_resolver>();
                worker_ui.enqueue([resolver=std::move(resolver)] () mutable -> void
                {
                    g_style_resolver = std::move(resolver);
                });
            });
        });
    }

    void load_audio_manifests()
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

                // create audio entries for listings in manifest
                worker_audio.enqueue([basePath=it.path(), manifestData=std::move(manifestData)]() -> void
                {
                    auto json = parse_json(manifestData);
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

                        get_audio_directory()->add_entry(
                            std::move(audioId),
                            std::move(title),
                            std::move(tags),
                            std::move(audioData));
                    }
                });
            }
        });
    }

    void load_playlists()
    {
        worker_fileio.enqueue([]() -> void
        {
            // iterate over each subdirectory in the library directory
            for(auto& it : fs::directory_iterator(paths::playlists()))
            {
                auto path = it.path();
                memory_buffer bytes(path);

                worker_audio.enqueue([=,bytes=std::move(bytes)]() -> void
                {
                    MX_LOG_INFO("Loading playlist : {}", path);

                    auto json = parse_json(bytes);
                    utf8_string version = json["Version"];
                    MX_LOG_INFO(" Version : {}", version);
                    MX_LOG_INFO(" Atmospheres :");

                    // parse our atmosphere_entry list
                    auto atmosphereList = json["Atmospheres"];
                    for (auto atmosphereJson : atmosphereList)
                    {
                        auto atmosphereId = atmosphereJson["Id"].get<uuid>();
                        if (g_atmosphere_directory.get_atmosphere_handle_by_id(atmosphereId) !=
                            atmosphere_handle::invalid)
                        {
                            MX_LOG_INFO("Skipping DOUBLE load");
                            continue;
                        }

                        atmosphere_entry atmosphereEntry;
                        atmosphereEntry.id = atmosphereId;
                        atmosphereEntry.title = atmosphereJson["Title"].get<utf8_string>();

                        MX_LOG_INFO(" AtmosphereTitle : {}", atmosphereEntry.title);
                        MX_LOG_INFO(" Id : {}", atmosphereEntry.id);

                        if (atmosphereJson.contains("Volume")) {
                            atmosphereEntry.volume = atmosphereJson["Volume"].get<float>();
                        }
                        MX_LOG_INFO(" Volume : {}", atmosphereEntry.volume);

                        if (atmosphereJson.contains("Muted")) {
                            atmosphereEntry.muted = atmosphereJson["Muted"].get<bool>();
                        }
                        MX_LOG_INFO(" Muted : {}", atmosphereEntry.muted);

                        if (atmosphereJson.contains("Favorite")) {
                            atmosphereEntry.favorite = atmosphereJson["Favorite"].get<bool>();
                        }
                        MX_LOG_INFO(" Favorite : {}", atmosphereEntry.favorite);


                        auto trackList = atmosphereJson["Tracks"];
                        for (auto trackJson : trackList)
                        {
                            track_entry trackEntry;

                            // Id is the id of the audio_entry referenced
                            const auto audioId = trackJson["Id"].get<uuid>();
                            auto audioHandle = g_audio_directory.get_audio_handle_by_id(audioId);

                            MX_LOG_INFO("  Id : {}", audioId);
                            MX_LOG_INFO("   AudioHandle : {}", audioHandle);

                            if (audioHandle == audio_handle::invalid) {
                                MX_LOG_ERROR("AudioData with id '{}' does not exist", audioId);
                                continue;
                            }

                            // get rest of data from audio directory
                            auto& audioEntry = g_audio_directory.get_entry(audioHandle);

                            trackEntry.audio = audioHandle;
                            trackEntry.volume = 1.0f;
                            trackEntry.muted = false;
                            trackEntry.repeat.enabled = false;
                            trackEntry.position.enabled = false;


                            MX_LOG_INFO("   AudioTitle : {}", audioEntry.title);

                            if (trackJson.contains("Volume")) {
                                trackEntry.volume = trackJson["Volume"].get<float>();
                            }
                            MX_LOG_INFO("   Volume : {}", trackEntry.volume);
                            if (trackJson.contains("Muted")) {
                                trackEntry.muted = trackJson["Muted"].get<bool>();
                            }
                            MX_LOG_INFO("   Muted : {}", trackEntry.muted);

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

                                        MX_LOG_INFO("   RepeatMin : {}", trackEntry.repeat.min);
                                        MX_LOG_INFO("   RepeatMax : {}", trackEntry.repeat.max);
                                    }
                                }
                            }

                            if (trackJson.contains("Position")) {
                                auto positionJson = trackJson["Position"];
                                auto& [enabled, elevation, azimuth] = trackEntry.position;
                                if (positionJson.is_array())
                                {
                                    MX_THROW_IF_FALSE(positionJson.size() == 2);
                                    switch(positionJson.size())
                                    {
                                        case 2:
                                            MX_THROW_IF_FALSE(positionJson[1].is_number());
                                            azimuth = positionJson[1].get<double>();
                                        case 1:
                                            MX_THROW_IF_FALSE(positionJson[0].is_number());
                                            elevation = positionJson[0].get<double>();
                                    }
                                }
                                else if (positionJson.is_object())
                                {
                                    if (positionJson.contains("Azimuth"))
                                    {
                                        const auto& azimuthJson = positionJson["Azimuth"];
                                        MX_THROW_IF_FALSE(azimuthJson.is_number());
                                        azimuth = azimuthJson.get<double>();
                                    }
                                    if (positionJson.contains("Elevation"))
                                    {
                                        const auto& elevationJson = positionJson["Elevation"];
                                        MX_THROW_IF_FALSE(elevationJson.is_number());
                                        elevation = elevationJson.get<double>();
                                    }
                                }
                                trackEntry.position.enabled = true;

                                MX_LOG_INFO("   Position : {{ elevation : {}, azimuth : {} }}", elevation, azimuth);
                            }

                            auto trackHandle = g_track_directory.add_entry(std::move(trackEntry));
                            atmosphereEntry.tracks.push_back(trackHandle);
                        }

                        const bool favorite = atmosphereEntry.favorite;
                        const auto atmosphereHandle = g_atmosphere_directory.add_entry(std::move(atmosphereEntry));
                        if (favorite) {
                            g_playlist_directory.add_atmosphere(playlist_handle::favorites, atmosphereHandle);
                        }
                        g_playlist_directory.add_atmosphere(playlist_handle::all, atmosphereHandle);

                    }

                    // parse our playlist_entry list
                    MX_LOG_INFO(" Playlists :");
                    auto playlistList = json["Playlists"];
                    for (auto playlistJson : playlistList)
                    {
                        playlist_entry playlistEntry;
                        playlistEntry.id = playlistJson["Id"].get<uuid>();
                        playlistEntry.title = playlistJson["Title"].get<utf8_string>();
                        MX_LOG_INFO("  Id : {}", playlistEntry.id);
                        MX_LOG_INFO("  PlaylistTitle : {}", playlistEntry.title);
                        MX_LOG_INFO("  Atmospheres : ");

                        auto playlistHandle = g_playlist_directory.add_entry(std::move(playlistEntry));

                        auto atmospheresList = playlistJson["Atmospheres"];
                        for (auto atmosphereId : atmospheresList)
                        {
                            const auto id = atmosphereId.get<uuid>();
                            auto atmosphereHandle = g_atmosphere_directory.get_atmosphere_handle_by_id(id);
                            if (atmosphereHandle == atmosphere_handle::invalid) {
                                MX_LOG_ERROR("Atmosphere with id '{}' does not exist", id);
                            }
                            MX_LOG_INFO("  AtmosphereId : {}", id);
                            g_playlist_directory.add_atmosphere(playlistHandle, atmosphereHandle);
                        }
                    }
                });
            }

            worker_ui.enqueue([]()
            {
                g_ui.sync_playlists();
            });
        });
    }

    void load_playlist(const fs::path& playlistPath)
    {
        worker_fileio.enqueue([=]() -> void
        {
            MX_LOG_INFO("Loading playlist: {}", playlistPath);
        });
    }

    void serialization_manager::save_playlist(fs::path&& dest, std::vector<playlist_handle_t>&& playlistHandles)
    {
        worker_audio.enqueue([playlistHandles=std::move(playlistHandles),dest=std::move(dest)]() -> void
        {
            // convert playlist, atmosphere and track entries into an nl::json object
            auto atmospheresJson = nl::json::array();
            auto playlistsJson = nl::json::array();

            std::set<atmosphere_handle_t> recordedAtmospheres;

            for(auto playlistHandle : playlistHandles)
            {
                const auto& playlistEntry = g_playlist_directory.get_entry(playlistHandle);
                auto currentPlaylistJson = nl::json::object();
                auto currentPlaylistAtmospheresJson = nl::json::array();
                currentPlaylistJson["Id"] = playlistEntry.id;
                currentPlaylistJson["Title"] = playlistEntry.title;

                for(auto atmosphereHandle : playlistEntry.atmospheres)
                {
                    const auto& atmosphereEntry = g_atmosphere_directory.get_entry(atmosphereHandle);
                    if (recordedAtmospheres.find(atmosphereHandle) == recordedAtmospheres.end())
                    {
                        // ensure we don't multiply define atmosphere's that live in multiple playlists
                        recordedAtmospheres.insert(atmosphereHandle);

                        // construct atmosphere object
                        auto currentAtmosphereJson = nl::json::object();
                        currentAtmosphereJson["Id"] = atmosphereEntry.id;
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
                            const auto& trackEntry = g_track_directory.get_entry(trackHandle);
                            const auto& audioHandle = trackEntry.audio;
                            const auto& audioEntry = g_audio_directory.get_entry(audioHandle);
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
                        atmospheresJson.push_back(std::move(currentAtmosphereJson));
                    }

                    // push atmosphere id to playlist
                    currentPlaylistAtmospheresJson.push_back(atmosphereEntry.id);
                }
                currentPlaylistJson["Atmospheres"] = std::move(currentPlaylistAtmospheresJson);

                playlistsJson.push_back(std::move(currentPlaylistJson));
            }
            auto objectJson = nl::json::object();
            objectJson["Version"] = "0.1";
            objectJson["Atmospheres"] = std::move(atmospheresJson);
            objectJson["Playlists"] = std::move(playlistsJson);

            worker_fileio.enqueue([objectJson=std::move(objectJson),dest=std::move(dest)]() -> void
            {
                // serialize json object to string
                std::fstream fout(dest, std::ios::out | std::ios::binary);
                fout << std::setw(2) << std::setfill(' ') << objectJson;
            });
        });
    }
}