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

namespace tabletopify
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

    // https://stackoverflow.com/questions/2552416/how-can-i-find-the-users-home-dir-in-a-cross-platform-manner-using-c
    static fs::path get_home_path()
    {
        static fs::path home = std::string(getenv("HOME"));
        return home;
    }

    fs::path get_tabletopify_path()
    {
        static fs::path tabletopify_path = get_home_path() / ".tabletopify";
        return tabletopify_path;
    }

    static fs::path get_library_path()
    {
        static fs::path library_path = get_tabletopify_path() / "libraries";
        return library_path;
    }

    fs::path get_skins_path()
    {
        static fs::path skins_path = get_tabletopify_path() / "skins";
        return skins_path;
    }

    fs::path get_strings_path()
    {
        static fs::path strings_path = get_tabletopify_path() / "strings";
        return strings_path;
    }

    static fs::path get_playlists_path()
    {
        static fs::path playlists_path = get_tabletopify_path() / "playlists";
        return playlists_path;
    }

    void load_stylesheet()
    {
        worker_fileio.enqueue([]() -> void
        {
            auto skinPath = get_skins_path() / "default.json";
            MX_LOG_INFO("Loading skin: {}", skinPath);

            memory_buffer mb(skinPath);

            worker_background.enqueue([mb=std::move(mb)]() -> void
            {
                auto json = parse_json(mb);
                g_style_resolver = json.get<style_resolver>();
            });
        });
    }

    void load_audio_manifests()
    {
        worker_fileio.enqueue([]() -> void
        {
            // iterate over each subdirectory in the library directory
            for(auto& it : fs::directory_iterator(get_library_path()))
            {
                if (!fs::is_directory(it.path())) {
                    MX_LOG_INFO("Skipping: {}" , it.path());
                    continue;
                }

                auto manifestPath = it.path() / "manifest.json";
                MX_LOG_INFO("Loading manifest: {}", manifestPath);
                memory_buffer manifestData(manifestPath);

                // create audio entries for listings in manifest
                worker_background.enqueue([basePath=it.path(), manifestData=std::move(manifestData)]() -> void
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
            // iterate over each subdirectory in the lilbrary directory
            for(auto& it : fs::directory_iterator(get_playlists_path()))
            {
                auto path = it.path();
                memory_buffer bytes(path);

                worker_background.enqueue([=,bytes=std::move(bytes)]() -> void
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
                        atmosphere_entry atmosphereEntry;
                        atmosphereEntry.id = atmosphereJson["AtmosphereId"].get<uuid>();
                        atmosphereEntry.title = atmosphereJson["Title"].get<utf8_string>();

                        MX_LOG_INFO(" AtmosphereTitle : {}", atmosphereEntry.title);
                        MX_LOG_INFO(" AtmosphereId : {}", atmosphereEntry.id);

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

                            // AudioId is the id of the audio_entry referenced
                            const auto audioId = trackJson["AudioId"].get<uuid>();
                            auto audioHandle = g_audio_directory.get_audio_handle_by_id(audioId);

                            MX_LOG_INFO("  AudioId : {}", audioId);
                            MX_LOG_INFO("   AudioHandle : {}", audioHandle);

                            if (audioHandle == audio_handle::invalid) {
                                MX_LOG_ERROR("AudioData with id '{}' does not exist", audioId);
                                continue;
                            }

                            // get rest of data from audio directory
                            auto& audioEntry = g_audio_directory.get_entry(audioHandle);

                            trackEntry.audioTitle = audioEntry.title;
                            trackEntry.audioData = audioEntry.data;
                            trackEntry.volume = 1.0f;
                            trackEntry.muted = false;

                            MX_LOG_INFO("   AudioTitle : {}", trackEntry.audioTitle);

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


                                MX_LOG_INFO("   Position : {{ elevation : {}, azimuth : {} }}", elevation, azimuth);
                            } else {
                                trackEntry.position.enabled = false;
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
                        playlistEntry.id = playlistJson["PlaylistId"].get<uuid>();
                        playlistEntry.title = playlistJson["Title"].get<utf8_string>();
                        MX_LOG_INFO("  PlaylistId : {}", playlistEntry.id);
                        MX_LOG_INFO("  PlaylistTitle : {}", playlistEntry.title);
                        MX_LOG_INFO("  Atmospheres : ");

                        auto playlistHandle = g_playlist_directory.add_entry(std::move(playlistEntry));

                        auto atmospheresList = playlistJson["Atmospheres"];
                        for (auto atmosphereId : atmospheresList)
                        {
                            const auto id = atmosphereId.get<uuid>();
                            auto atmosphereHandle = g_atmosphere_directory.get_atmosphere_handle_by_id(id);
                            if (atmosphereHandle == atmosphere_handle::invalid) {
                                MX_LOG_ERROR("Atmosphere with id '{}' does not exist", atmosphereId);
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
}