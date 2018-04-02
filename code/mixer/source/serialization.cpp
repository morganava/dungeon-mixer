#include "precomp.hpp"

#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "tag_map.hpp"
#include "audio.hpp"
#include "draw.hpp"

namespace tabletopify
{
    void load_audio_manifests()
    {
        worker(Worker_FileIO).enqueue([]() -> void
        {
            // iterate over each subdirectory in the tabletopify directory
            for(auto& it : fs::directory_iterator(get_tabletopify_path()))
            {
                auto manifestPath = it.path() / "manifest.json";
                auto manifestSize = fs::file_size(manifestPath);
                std::cout << " manifest: " << manifestPath << std::endl;
                std::cout << " size: " << manifestSize << std::endl;

                std::ifstream manifestFile(manifestPath.native(), std::ios_base::in | std::ios_base::binary);
                std::string manifestData(manifestSize, 0);
                manifestFile.read(manifestData.data(), manifestSize);
                manifestFile.close();

                std::cout << manifestData << std::endl;

                // create audio entries for listings in manifest
                worker(Worker_Background).enqueue([basePath=it.path(), manifestData=std::move(manifestData)]() -> void
                {
                    auto json = nl::json::parse(manifestData);
                    auto trackList = json["Tracks"];
                    for(auto track : trackList)
                    {
                        std::string title = track["Title"];
                        std::string uuid = track["UUID"];
                        std::string type = track["Type"];
                        std::vector<std::string> tags = track["Tags"];
                        std::vector<std::string> files = track["Files"];
                        for(auto& currentFile : files)
                        {
                            currentFile = basePath / currentFile;
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
                            std::move(uuid),
                            std::move(title),
                            std::move(tags),
                            std::move(audioData));
                    }
                });
            }
        });
    }

    void save_user_mix(const fs::path& path)
    {
        log(LogLevel_Info, ss() << "Begin saving user mix: " << path);

        worker(Worker_UI).enqueue([=]() -> void
        {
            // construct favorites list on UI thread
            auto jFavoriteList = nl::json::array();
            for(auto& atmospherePlayback : g_ui->playback.atmosphereList.atmospheres)
            {
                nl::json jFavorite;
                jFavorite["UUID"] = atmospherePlayback.uuid;
                jFavorite["Volume"] = atmospherePlayback.volume;
                jFavorite["Muted"] = atmospherePlayback.muted;
                jFavoriteList.push_back(jFavorite);
            }

            // construct rest of JSON on background thread
            worker(Worker_Background).enqueue([path=path, jFavoriteList=std::move(jFavoriteList)]() -> void
            {
                nl::json jObj;
                jObj["Version"] = "1.0";
                jObj["Favorites"] = jFavoriteList;
                auto jAtmosphereList = nl::json::array();
                // serialize each atmosphere
                for(auto atmosphereId :  g_atmosphere_directory->get_atmosphere_ids())
                {
                    auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereId);
                    nl::json jAtmosphere;
                    jAtmosphere["Title"] = atmosphereEntry.title;
                    jAtmosphere["UUID"] = atmosphereEntry.uuid;

                    auto jTrackList = nl::json::array();
                    // serialize each track
                    for(auto trackId : atmosphereEntry.tracks)
                    {
                        auto& trackEntry = g_track_directory->get_entry(trackId);
                        nl::json jTrack;
                        jTrack["UUID"] = trackEntry.audioUuid;
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
                    std::ofstream mixFile(path.native(), std::ios_base::out | std::ios_base::binary);
                    mixFile << json;
                    mixFile.close();

                    log(LogLevel_Info, ss() << "Finish saving user mix: " << path);
                });
            });
        });
    }
}