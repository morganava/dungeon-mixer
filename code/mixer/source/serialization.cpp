#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "hash.hpp"
#include "nanovg.hpp"
#include "imgui.hpp"
#include "color.hpp"
#include "stylesheet.hpp"

namespace tabletopify
{
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
        worker(Worker_FileIO).enqueue([]() -> void
        {
            auto skinPath = get_skins_path() / "default.json";
            MX_LOG_INFO("Loading skin: {}", skinPath);

            memory_buffer mb(skinPath);

            worker(Worker_Background).enqueue([mb=std::move(mb)]() -> void
            {
                auto json = nl::json::parse(mb.begin(), mb.end());
                g_style_resolver = json.get<style_resolver>();
            });
        });
    }

    void load_audio_manifests()
    {
        worker(Worker_FileIO).enqueue([]() -> void
        {
            // iterate over each subdirectory in the library directory
            for(auto& it : fs::directory_iterator(get_library_path()))
            {
                if (!fs::is_directory(it.path())) {
                    MX_LOG_INFO("Skipping: {}" , it.path());
                    continue;
                }

                // TODO: use memory_buffer here
                auto manifestPath = it.path() / "manifest.json";
                MX_LOG_INFO("Loading manifest: {}", manifestPath);
                auto manifestSize = fs::file_size(manifestPath);

                std::ifstream manifestFile(manifestPath.native(), std::ios_base::in | std::ios_base::binary);
                std::string manifestData(manifestSize, 0);
                manifestFile.read(manifestData.data(), manifestSize);
                manifestFile.close();

                // create audio entries for listings in manifest
                worker(Worker_Background).enqueue([basePath=it.path(), manifestData=std::move(manifestData)]() -> void
                {
                    auto json = nl::json::parse(manifestData);
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
        worker(Worker_FileIO).enqueue([]() -> void
        {
            // iterate over each subdirectory in the lilbrary directory
            for(auto& it : fs::directory_iterator(get_playlists_path()))
            {
                auto path = it.path();
                MX_LOG_INFO("Loading playlist: {}", path);
                memory_buffer bytes(path);

                worker(Worker_Background).enqueue([bytes=std::move(bytes)]() -> void
                {
                    // TODO: make these compatible with singleton<T>
                    auto& g_atmosphere_directory = *get_atmosphere_directory();
                    auto& g_track_directory = *get_track_directory();
                    auto& g_audio_directory = *get_audio_directory();

                    auto json = nl::json::parse(bytes.begin(), bytes.end());
                    utf8_string version = json["Version"];
                    // TODO: handle different versions
                    (void)version;

                    // parse our atmosphere_entry list
                    auto atmosphereList = json["Atmospheres"];
                    for (auto atmosphereJson : atmosphereList)
                    {
                        atmosphere_entry atmosphereEntry;
                        atmosphereEntry.id = atmosphereJson["AtmosphereId"].get<uuid>();
                        atmosphereEntry.title = atmosphereJson["Title"].get<utf8_string>();

                        auto trackList = atmosphereJson["Tracks"];
                        for (auto trackJson : trackList)
                        {
                            track_entry trackEntry;

                            // AudioId is the id of the audio_entry referenced
                            const auto audioId = trackJson["AudioId"].get<uuid>();
                            auto audioHandle = g_audio_directory.get_audio_handle_by_id(audioId);

                            if (audioHandle == MX_INVALID_HANDLE) {
                                MX_LOG_ERROR("AudioData with id '{}' does not exist", trackEntry.audioId);
                                continue;
                            }

                            trackEntry.audioId = audioId;
                            trackEntry.volume = 1.0f;
                            trackEntry.atmosphereVolume = 1.0f;

                            if (trackJson.contains("Volume")) {
                                trackEntry.volume = trackJson["Volume"].get<float>();
                            }
                            if (trackJson.contains("RepeatRange")) {
                                // TODO: implement
                                trackEntry.repeat.min = 5.0f;
                                trackEntry.repeat.max = 10.0f;
                            }
                            if (trackJson.contains("Position")) {
                                // TODO: implement
                            } else {
                                trackEntry.position.enabled = false;
                            }


                            // get rest of data from audio directory
                            auto& audioEntry = g_audio_directory.get_entry(audioHandle);
                            trackEntry.audioTitle = audioEntry.title;
                            trackEntry.audioData = audioEntry.data;

                            auto trackHandle = g_track_directory.add_entry(std::move(trackEntry));
                            atmosphereEntry.tracks.push_back(trackHandle);
                        }
                        g_atmosphere_directory.add_entry(std::move(atmosphereEntry));
                    }

                    // parse our playlist_entry list
                    auto playlistList = json["Playlists"];
                    for (auto playlistJson : playlistList)
                    {
                        playlist_entry playlistEntry;
                        playlistEntry.id = playlistJson["PlaylistId"].get<uuid>();
                        playlistEntry.title = playlistJson["Title"].get<utf8_string>();
                        auto atmospheresList = playlistJson["Atmospheres"];
                        for (auto atmosphereId : atmospheresList)
                        {
                            const auto id = atmosphereId.get<uuid>();
                            auto atmosphereHandle = g_atmosphere_directory.get_atmosphere_handle_by_id(id);
                            if (atmosphereHandle == MX_INVALID_HANDLE) {
                                MX_LOG_ERROR("Atmosphere with id '{}' does not exist", atmosphereId);
                            }
                            playlistEntry.atmospheres.push_back(atmosphereHandle);
                        }
                        g_playlist_directory.add_entry(std::move(playlistEntry));
                    }
                });
            }
        });
    }

    void load_playlist(const fs::path& playlistPath)
    {
        worker(Worker_FileIO).enqueue([=]() -> void
        {
            MX_LOG_INFO("Loading playlist: {}", playlistPath);
        });

    }

    void open_user_mix(const fs::path& mixPath)
    {
#if 0
        log(LogLevel_Info, ss() << "Begin open user mix: " << mixPath);

        worker(Worker_FileIO).enqueue([=]() -> void
        {
            auto mixSize = fs::file_size(mixPath);
            std::string mixData(mixSize, 0);

            std::ifstream mixFile(mixPath, std::ios_base::in | std::ios_base::binary);
            mixFile.read(mixData.data(), mixSize);
            mixFile.close();

            worker(Worker_Background).enqueue([mixPath=mixPath, mixData=std::move(mixData)]() -> void
            {
                track_directory trackDirectory;
                atmosphere_directory atmosphereDirectory;

                auto jObj = nl::json::parse(mixData);

                std::string version = jObj["Version"];
                if(version != MIX_VERSION)
                {
                    log(LogLevel_Error, ss() << "Failed to open " << mixPath << ", version mismatch");
                    return;
                }

                using edits_list_t = decltype(g_ui->edit.atmosphereList.atmospheres);
                edits_list_t editList;

                auto& jAtmosphereList = jObj["Atmospheres"];
                for(auto& jAtmosphere : jAtmosphereList)
                {
                    std::string title = jAtmosphere["Title"];
                    uuid atmosphereId = jAtmosphere["AtmosphereId"];
                    std::vector<track_handle_t> tracks;
                    auto& jTrackList = jAtmosphere["Tracks"];
                    for(auto& jTrack : jTrackList)
                    {
                        auto& jAudioId = jTrack["AudioId"];
                        MX_ASSERT(jAudioId.is_string());
                        auto& jVolume = jTrack["Volume"];
                        MX_ASSERT(jVolume.is_number());
                        auto& jMuted = jTrack["Muted"];
                        MX_ASSERT(jMuted.is_boolean());
                        auto& jType = jTrack["Type"];
                        MX_ASSERT(jType.is_string());
                        auto& jPosition = jTrack["Position"];
                        MX_ASSERT(jPosition.is_array() || jPosition.is_null());
                        auto& jRepeat = jTrack["Repeat"];
                        MX_ASSERT(jRepeat.is_array() || jRepeat.is_null());

                        auto audioHandle = get_audio_directory()->get_audio_handle_by_id(jAudioId);

                        track_entry trackEntry;
                        {
                            auto& audioEntry = get_audio_directory()->get_entry(audioHandle);

                            trackEntry.audioTitle = audioEntry.title;
                            trackEntry.audioId = audioEntry.id;
                            trackEntry.audioData = audioEntry.data;
                            trackEntry.audioType = audioEntry.data->get_type();
                            trackEntry.volume = jVolume;
                            trackEntry.muted = jMuted;

                            switch(trackEntry.audioType)
                            {
                                case ONE_SHOT:
                                    MX_ASSERT(jType == "OneShot");
                                    MX_ASSERT(jRepeat.is_array());
                                    MX_ASSERT(jRepeat[0].is_number());
                                    MX_ASSERT(jRepeat[1].is_number());
                                    trackEntry.repeat.min = jRepeat[0];
                                    trackEntry.repeat.max = jRepeat[1];
                                    break;
                                case LOOP:
                                    MX_ASSERT(jType == "Loop");
                                    break;
                            }

                            if(jPosition.is_array())
                            {
                                MX_ASSERT(jPosition[0].is_number());
                                MX_ASSERT(jPosition[1].is_number());
                                MX_ASSERT(jPosition[2].is_number());
                                trackEntry.position.enabled = true;
                                trackEntry.position.x = jPosition[0];
                                trackEntry.position.y = jPosition[1];
                                trackEntry.position.z = jPosition[2];
                            }
                            else
                            {
                                trackEntry.position.enabled = false;
                            }
                        }

                        auto trackHandle = trackDirectory.add_entry(std::move(trackEntry));
                        tracks.push_back(trackHandle);
                    }
                    atmosphere_entry atmosphereEntry =
                    {
                        atmosphereId,
                        title,
                        std::move(tracks),
                    };
                    auto atmosphereHandle = atmosphereDirectory.add_entry(std::move(atmosphereEntry));

                    editList.push_back({title, atmosphereHandle, false});
                }

                auto& jFavoritesList = jObj["Favorites"];
                MX_ASSERT(jFavoritesList.is_array());

                using favorites_list_t = decltype(g_ui->playback.atmosphereList.atmospheres);
                favorites_list_t favoritesList;

                for(auto& jFavorite : jFavoritesList)
                {
                    auto& jAtmosphereId = jFavorite["AtmosphereId"];
                    MX_ASSERT(jAtmosphereId.is_string());
                    auto& jVolume = jFavorite["Volume"];
                    MX_ASSERT(jVolume.is_number());
                    auto& jMuted = jFavorite["Muted"];
                    MX_ASSERT(jMuted.is_boolean());

                    auto atmosphereHandle = atmosphereDirectory.get_atmosphere_handle_by_id(jAtmosphereId);
                    MX_ASSERT(atmosphereHandle != MX_INVALID_HANDLE);
                    auto& atmosphereEntry = atmosphereDirectory.get_entry(atmosphereHandle);

                    favorites_list_t::value_type favorite;
                    {
                        favorite.handle = atmosphereHandle;
                        favorite.title = atmosphereEntry.title;
                        favorite.id = atmosphereEntry.id;
                        favorite.volume = jVolume;
                        favorite.muted = jMuted;
                        favorite.playing = false;
                    }

                    favoritesList.push_back(std::move(favorite));
                }

                get_playback_manager()->stop_all_tracks();
                init_track_directory(std::move(trackDirectory));
                init_atmosphere_directory(std::move(atmosphereDirectory));

                worker(Worker_UI).enqueue([mixPath=mixPath, editList=std::move(editList), favoritesList=std::move(favoritesList)]() -> void
                {
                    g_ui->edit.atmosphereList.atmospheres = std::move(editList);
                    g_ui->playback.atmosphereList.atmospheres = std::move(favoritesList);
                    g_ui->edit.atmosphereList.selectedIndex = MX_INVALID_INDEX;
                    g_ui->edit.atmosphereEdit.tracks.clear();

                    log(LogLevel_Info, ss() << "Finish open user mix: " << mixPath);
                });
            });
        });
#endif
    }

    void save_user_mix(const fs::path& path)
    {
#if 0
        log(LogLevel_Info, ss() << "Begin save user mix: " << path);

        worker(Worker_UI).enqueue([=]() -> void
        {
            // construct favorites list on UI thread
            auto jFavoriteList = nl::json::array();
            for(auto& atmospherePlayback : g_ui->playback.atmosphereList.atmospheres)
            {
                nl::json jFavorite;
                jFavorite["AtmosphereId"] = atmospherePlayback.id;
                jFavorite["Volume"] = atmospherePlayback.volume;
                jFavorite["Muted"] = atmospherePlayback.muted;
                jFavoriteList.push_back(jFavorite);
            }

            // construct rest of JSON on background thread
            worker(Worker_Background).enqueue([path=path, jFavoriteList=std::move(jFavoriteList)]() -> void
            {
                nl::json jObj;
                jObj["Version"] = "0.1";
                jObj["Favorites"] = jFavoriteList;
                auto jAtmosphereList = nl::json::array();
                // serialize each atmosphere
                for(auto atmosphereHandle :  get_atmosphere_directory()->get_atmosphere_handles())
                {
                    auto& atmosphereEntry = get_atmosphere_directory()->get_entry(atmosphereHandle);
                    nl::json jAtmosphere;
                    jAtmosphere["Title"] = atmosphereEntry.title;
                    jAtmosphere["AtmosphereId"] = atmosphereEntry.id;

                    auto jTrackList = nl::json::array();
                    // serialize each track
                    for(auto trackHandle : atmosphereEntry.tracks)
                    {
                        auto& trackEntry = get_track_directory()->get_entry(trackHandle);
                        nl::json jTrack;
                        jTrack["AudioId"] = trackEntry.audioId;
                        jTrack["Volume"] = trackEntry.volume;
                        jTrack["Muted"] = trackEntry.muted;
                        switch(trackEntry.audioType)
                        {
                            case ONE_SHOT:
                                jTrack["Type"] = "OneShot";
                                jTrack["Repeat"] = {trackEntry.repeat.min, trackEntry.repeat.max};
                            break;
                            case LOOP:
                                jTrack["Type"] = "Loop";
                            break;
                        }
                        if(trackEntry.position.enabled)
                        {
                            jTrack["Position"] = {trackEntry.position.x, trackEntry.position.y, trackEntry.position.z};
                        }

                        jTrackList.push_back(jTrack);
                    }
                    jAtmosphere["Tracks"] = jTrackList;
                    jAtmosphereList.push_back(jAtmosphere);
                }
                jObj["Atmospheres"] = jAtmosphereList;

                // save to disk on IO thread
                worker(Worker_FileIO).enqueue([json=std::move(jObj.dump(4)), path=path]() -> void
                {
                    std::ofstream mixFile(path, std::ios_base::out | std::ios_base::binary);
                    mixFile << json;
                    mixFile.close();

                    log(LogLevel_Info, ss() << "Finish save user mix: " << path);
                });
            });
        });
#endif
    }
}