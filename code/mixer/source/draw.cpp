#include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "globals.hpp"
#include "work_queue.hpp"

namespace tabletopify
{
    static UI ui;
    UI* g_ui = &ui;

    /* 
    Constants 
    */

    /* Globals */
    constexpr int MX_ROW_HEIGHT = 32;
    constexpr int MX_BUTTON_SIZE = MX_ROW_HEIGHT;

    /* Playback */

    constexpr int MX_CLOSE_BUTTON_SIZE = 24;

    /* Edit */

    constexpr int MX_VOLUME_HEIGHT = 32;
    constexpr int MX_VOLUME_BUTTON_SIZE = 32;

    constexpr int MX_ROW_LABEL_OFFSET = 16;    
    constexpr int MX_ROW_LABEL_WIDTH = 72;

    constexpr int MX_ROW_MUTE_WIDTH = 50;
    constexpr int MX_ROW_MINMAX_WIDTH = 32;
    constexpr int MX_ROW_XYZ_WIDTH = 16;
    constexpr int MX_ROW_XYZ_TEXT_WIDTH = 64;


    // OLD SHIT

    constexpr float MX_GROUP_BORDER = 1.0f;
    // interior spacing of group
    constexpr float MX_GROUP_PADDING_X = 2.5f;
    constexpr float MX_GROUP_PADDING_Y = 5.0f;
    // distance between adjacent widgets
    constexpr float MX_GROUP_SPACING = 5.0f;

    constexpr int MX_TOPBAR_HEIGHT = 100;
    constexpr int MX_GROUP_MARGIN = 0;
    constexpr int MX_TEXT_PADDING = 0;
    constexpr int MX_BUTTON_PADDING = 0;
    constexpr int MX_WIDGET_MARGIN = 2;

    // top bar
    constexpr int MX_MODE_BUTTON_WIDTH = 100;

    // search bars
    constexpr int MX_TEXT_SEARCH_HEIGHT = 24;
    constexpr int MX_TEXT_FIELD_LENGTH = 256;

    // atmosphere entries
    constexpr int MX_ATMOSPHERE_TITLE_HEIGHT = 32;
    constexpr int MX_ATMOSPHERE_BUTTON_SIZE = 20;
    constexpr int MX_ATMOSPHERE_BUTTON_SPACER = 6;
    constexpr int MX_ATMOSPHERE_BUTTON_CONSTELLATION_WIDTH = 3 * MX_ATMOSPHERE_BUTTON_SIZE + 4 * MX_ATMOSPHERE_BUTTON_SPACER;
    constexpr int MX_ATMOSPHERE_BUTTON_CONSTELLATION_HEIGHT = 2 * MX_ATMOSPHERE_BUTTON_SIZE + 3 * MX_ATMOSPHERE_BUTTON_SPACER;
    constexpr int MX_ATMOSPHERE_HEIGHT = MX_ATMOSPHERE_TITLE_HEIGHT + MX_ATMOSPHERE_BUTTON_CONSTELLATION_HEIGHT;


    
    /* 
    State 
    */

    enum 
    {
        MX_MODE_PLAY,
        MX_MODE_EDIT,
    } MX_MODE = MX_MODE_EDIT;

    /*
    Text Formatting
    */

    void print_minmax(char* buffer, size_t len, float val)
    {
        snprintf(buffer, len, "%#3.1f", val);
    }

    void print_position(char* buffer, size_t len, float val)
    {
        snprintf(buffer, len, "%#3.2f", val);
    }

    /*
    UI Update Methods
    */

    void update_ui_playback_atmosphereSearch(const std::vector<atmosphere_id_t>& atmosphere_ids)
    {
        decltype(g_ui->playback.atmosphereSearch.results) searchResults;
        for(auto id : atmosphere_ids)
        {
            auto& currentAtmosphere = g_atmosphere_directory->get_entry(id);
            searchResults.push_back({id, currentAtmosphere.title});
        }

        worker(Worker_UI).enqueue([searchResults = std::move(searchResults)] () -> void
        {
            g_ui->playback.atmosphereSearch.results = searchResults;
        });
    }

    void update_ui_edit_atmosphereList(const std::vector<atmosphere_id_t>& atmosphere_ids)
    {
        decltype(g_ui->edit.atmosphereList.atmospheres) atmospheres;
        for(auto id : atmosphere_ids)
        {
            auto& currentAtmosphere = g_atmosphere_directory->get_entry(id);
            atmospheres.push_back({currentAtmosphere.title, id});
        }

        worker(Worker_UI).enqueue([atmospheres = std::move(atmospheres)]() -> void
        {
            g_ui->edit.atmosphereList.atmospheres = atmospheres;
        });
    }

    void update_ui_edit_atmosphereEdit(atmosphere_id_t atmosphere_id)
    {
        decltype(g_ui->edit.atmosphereEdit.tracks) tracks;

        if(atmosphere_id != INVALID_ATMOSPHERE_ID)
        {
            auto& selectedAtmosphere = g_atmosphere_directory->get_entry(atmosphere_id);
            for(auto trackId : selectedAtmosphere.tracks)
            {
                auto& trackEntry = g_track_directory->get_entry(trackId);

                decltype(tracks)::value_type trackEdit;
                trackEdit.volume = trackEntry.volume;
                trackEdit.muted = trackEntry.muted ? 1 : 0;

                switch(trackEntry.audioType)
                {
                    case ONE_SHOT:
                        {
                            trackEdit.trackId = trackId;
                            trackEdit.title = trackEntry.title;
                            trackEdit.audioType = trackEntry.audioType;
                            trackEdit.oneShot.repeat.min = trackEntry.oneShot.repeatMin;
                            trackEdit.oneShot.repeat.max = trackEntry.oneShot.repeatMax;
                            print_minmax(trackEdit.oneShot.repeat.minBuffer, countof(trackEdit.oneShot.repeat.minBuffer), trackEdit.oneShot.repeat.min);
                            print_minmax(trackEdit.oneShot.repeat.maxBuffer, countof(trackEdit.oneShot.repeat.maxBuffer), trackEdit.oneShot.repeat.max);
                            trackEdit.oneShot.position.x = trackEntry.oneShot.x;
                            trackEdit.oneShot.position.y = trackEntry.oneShot.y;
                            trackEdit.oneShot.position.z = trackEntry.oneShot.z;
                            print_position(trackEdit.oneShot.position.xBuffer, countof(trackEdit.oneShot.position.xBuffer), trackEdit.oneShot.position.x);
                            print_position(trackEdit.oneShot.position.yBuffer, countof(trackEdit.oneShot.position.yBuffer), trackEdit.oneShot.position.y);
                            print_position(trackEdit.oneShot.position.zBuffer, countof(trackEdit.oneShot.position.zBuffer), trackEdit.oneShot.position.z);
                            tracks.push_back(std::move(trackEdit));
                        }
                        break;
                    case LOOP:
                        break;
                }
            }
        }

        worker(Worker_UI).enqueue([tracks = std::move(tracks)]() -> void
        {
            g_ui->edit.atmosphereEdit.tracks = tracks;
        });
    }

    void update_ui_edit_trackSearch(const std::vector<audio_id_t>& audio_ids)
    {
        decltype(g_ui->edit.trackSearch.results) searchResults;
        for(auto id : audio_ids)
        {
            searchResults.push_back(
            {
                g_audio_directory->get_title_by_audio_id(id),
                id
            });
        }

        worker(Worker_UI).enqueue([results = std::move(searchResults)]() -> void
        {
            g_ui->edit.trackSearch.results = results;
        });
    }

    /* 
    Draw Calls
    */  

    void draw_top_bar(nk_context* ctx, int width)
    {
        if (nk_begin(ctx, "top_bar", nk_rect(0, 0, width, MX_TOPBAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_begin(ctx, NK_STATIC, MX_TOPBAR_HEIGHT, 3);
            nk_layout_row_push(ctx, width - 2 * MX_MODE_BUTTON_WIDTH - 4 * MX_WIDGET_MARGIN);
            nk_spacing(ctx, 1);
            nk_layout_row_push(ctx, MX_MODE_BUTTON_WIDTH);
            if(nk_button_label(ctx, "Playback"))
            {
                worker(Worker_UI).enqueue([]() -> void {MX_MODE = MX_MODE_PLAY;});
                worker(Worker_Background).enqueue([]() -> void
                {
                    g_playback_manager->stop_all_tracks();
                });
            }
            nk_layout_row_push(ctx, MX_MODE_BUTTON_WIDTH);
            if(nk_button_label(ctx, "Edit"))
            {
                worker(Worker_UI).enqueue([]() -> void {MX_MODE = MX_MODE_EDIT;});
                worker(Worker_Background).enqueue([]() -> void
                {
                    g_playback_manager->stop_all_tracks();
                });
                auto idx = g_ui->edit.atmosphereList.selectedIndex;
                if(idx != ~size_t(0))
                {
                    auto& currentAtmosphere = g_ui->edit.atmosphereList.atmospheres[idx];
                    auto atmosphereId = currentAtmosphere.atmosphereId;
                    worker(Worker_Background).enqueue([=]() -> void
                    {
                        g_playback_manager->play_atmosphere(atmosphereId);
                    });
                }
            }
        }
        nk_end(ctx);    
    }

    void draw_play_screen(nk_context* ctx, int width, int height, bool hidden)
    {
        float win_x = hidden ? width : 0;

        if (nk_begin(ctx, "play_screen", nk_rect(win_x, MX_TOPBAR_HEIGHT, width, height), NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_begin(ctx, NK_DYNAMIC, height, 2);
            nk_layout_row_push(ctx, 2.0f/3.0f);
            {
                nk_group_begin(ctx, "play_screen>left_group", 0);
                {
                    for(size_t k = 0; k < g_ui->playback.atmosphereList.atmospheres.size(); ++k)
                    {
                        auto& currentAtmosphere = g_ui->playback.atmosphereList.atmospheres[k];
                        nk_layout_row_dynamic(ctx, 2 * (MX_GROUP_BORDER + MX_ROW_HEIGHT + MX_GROUP_PADDING_Y) + MX_GROUP_SPACING, 1);

                        std::stringstream ss;
                        ss << "play_screen>left_group>atmosphere" << string_format("%016lx", k);
                        nk_group_begin(ctx, ss.str().c_str(), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR);
                        {
                            nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                            nk_layout_row_template_push_dynamic(ctx);
                            nk_layout_row_template_push_static(ctx, MX_CLOSE_BUTTON_SIZE);
                            nk_layout_row_template_end(ctx);

                            nk_label(ctx, currentAtmosphere.title.c_str(), NK_TEXT_LEFT);
                            if(nk_button_label(ctx, "X")) 
                            {
                                // remove this atmosphere from active list
                                worker(Worker_UI).enqueue([=]() -> void 
                                {
                                    g_ui->playback.atmosphereList.atmospheres.erase(g_ui->playback.atmosphereList.atmospheres.begin() + k);
                                });
                            }

                            nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                            nk_layout_row_template_push_dynamic(ctx);
                            nk_layout_row_template_push_static(ctx, MX_BUTTON_SIZE);
                            nk_layout_row_template_push_static(ctx, MX_BUTTON_SIZE);
                            nk_layout_row_template_end(ctx);

                            const float volumeStep = 0.001f;
                            if(nk_slider_float(ctx, 0.0f, &currentAtmosphere.volume, 1.0f, volumeStep))
                            {

                            }
                            if(nk_button_label(ctx, "V"))
                            {

                            }
                            if(nk_button_label(ctx, "P"))
                            {
                                worker(Worker_Background).enqueue([=]() -> void
                                {
                                    std::cout << "Playing " << currentAtmosphere.title << std::endl;
                                    g_playback_manager->play(currentAtmosphere.atmosphereId);
                                });
                            }

                        }
                        nk_group_end(ctx);
                    }
                }
                nk_group_end(ctx);
            }
            nk_layout_row_push(ctx, 1.0f - 2.0f/3.0f);
            {
                nk_group_begin(ctx, "play_screen>right_group", NK_WINDOW_NO_SCROLLBAR);
                {
                    nk_layout_row_begin(ctx, NK_DYNAMIC, MX_TEXT_SEARCH_HEIGHT, 1);
                    nk_layout_row_push(ctx, 1.0f);
                   
                    if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx, NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SIG_ENTER | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD, g_ui->playback.atmosphereSearch.searchBuffer, countof(g_ui->playback.atmosphereSearch.searchBuffer), nk_filter_default))
                    {
                        std::string titleQuery = g_ui->playback.atmosphereSearch.searchBuffer;
                        auto& previousQuery = g_ui->playback.atmosphereSearch.prevSearch;

                        if(previousQuery != titleQuery)
                        {
                            previousQuery = titleQuery;
                            worker(Worker_Background).enqueue([titleQuery=std::move(trim(titleQuery))]() -> void
                            {
                                std::vector<atmosphere_id_t> searchResults;
                                if(titleQuery.empty())
                                {
                                    searchResults = std::move(g_atmosphere_directory->get_atmosphere_ids());
                                }
                                else
                                {
                                    searchResults = std::move(g_atmosphere_directory->get_atmosphere_ids_by_title(titleQuery));
                                }

                                update_ui_playback_atmosphereSearch(searchResults);
                            });
                        }
                    }

                    nk_layout_row_begin(ctx, NK_DYNAMIC, height - MX_TEXT_SEARCH_HEIGHT, 1);
                    nk_layout_row_push(ctx, 1.0f);
                    {
                        nk_group_begin(ctx, "play_screen>right_group>search_results", 0);
                        {
                            for(const auto& currentAtmosphere : g_ui->playback.atmosphereSearch.results)
                            {
                                nk_layout_row_dynamic(ctx, 50, 1);
                                if(nk_widget_is_mouse_clicked(ctx, NK_BUTTON_DOUBLE))
                                {
                                    printf("clicked on : %s\n", currentAtmosphere.title.c_str());
                                    worker(Worker_UI).enqueue([=]() -> void
                                    {
                                        g_ui->playback.atmosphereList.atmospheres.push_back(
                                        {
                                            currentAtmosphere.atmosphereId,
                                            currentAtmosphere.title,
                                            1.0f,
                                        });
                                    });
                                }
                                nk_label(ctx, currentAtmosphere.title.c_str(), NK_TEXT_CENTERED);
                            }
                        }
                        nk_group_end(ctx);
                    }
                }
                nk_group_end(ctx);          
            }
            nk_layout_row_end(ctx);
        }
        nk_end(ctx);
    }

    void draw_edit_screen(nk_context* ctx, int width, int height, bool hidden)
    {
        float win_x = hidden ? width : 0;

        if (nk_begin(ctx, "edit_screen", nk_rect(win_x, MX_TOPBAR_HEIGHT, width, height), NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_begin(ctx, NK_DYNAMIC, height, 3);
            nk_layout_row_push(ctx, 1.0f/3.0f);
            {
                nk_group_begin(ctx, "edit_screen>left_group", 0);
                {
                    // float close_ratio = nk_layout_ratio_from_pixel(ctx, MX_CLOSE_BUTTON_SIZE);
                    for(size_t idx = 0; idx < g_ui->edit.atmosphereList.atmospheres.size(); ++idx)
                    {
                        auto& currentAtmosphere = g_ui->edit.atmosphereList.atmospheres[idx];

                        nk_layout_row_dynamic(ctx, MX_ROW_HEIGHT, 1);
                        if(nk_widget_is_mouse_clicked(ctx, NK_BUTTON_LEFT))
                        {
                            printf("Selected %s\n", currentAtmosphere.title.c_str());
                            if(g_ui->edit.atmosphereList.selectedIndex != idx )
                            {
                                g_ui->edit.atmosphereList.selectedIndex = idx;
                                auto atmosphereId = g_ui->edit.atmosphereList.atmospheres[idx].atmosphereId;
                                worker(Worker_Background).enqueue([=]() -> void 
                                {
                                    // populate the atmosphereEdit list based on the atmosphereId
                                    update_ui_edit_atmosphereEdit(atmosphereId);
                                    g_playback_manager->stop_all_tracks();
                                    g_playback_manager->play_atmosphere(atmosphereId);
                                });
                            }
                        }
                        nk_label(ctx, currentAtmosphere.title.c_str(), NK_TEXT_ALIGN_LEFT);
                    }
                }
                nk_group_end(ctx);
            }
            nk_layout_row_push(ctx, 1.0f/3.0f);
            {
                nk_group_begin(ctx, "edit_screen>middle_group", 0);
                {
                    auto currentAtmosphereId = g_ui->edit.atmosphereList.selectedAtmosphereId();
                    const size_t trackCount = g_ui->edit.atmosphereEdit.tracks.size();
                    for(size_t trackIndex = 0 ; trackIndex < trackCount; ++trackIndex)
                    {
                        auto& currentTrack = g_ui->edit.atmosphereEdit.tracks[trackIndex];
                        auto currentTrackId = currentTrack.trackId;

                        nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                        nk_layout_row_template_push_dynamic(ctx);
                        nk_layout_row_template_push_static(ctx, MX_CLOSE_BUTTON_SIZE);
                        nk_layout_row_template_end(ctx);

                        nk_label(ctx, currentTrack.title.c_str(), NK_TEXT_LEFT);
                        
                        if(nk_button_label(ctx, "X"))
                        {
                            printf("Close %s\n", currentTrack.title.c_str());
                            
                            worker(Worker_Background).enqueue([=]() -> void
                            {
                                // stop it from playing
                                g_playback_manager->stop_track(currentTrackId);

                                // remove track from atmosphere
                                auto& atmosphereEntry = g_atmosphere_directory->get_entry(currentAtmosphereId);
                                auto it = find(atmosphereEntry.tracks.begin(), atmosphereEntry.tracks.end(), currentTrackId);
                                assert(it != atmosphereEntry.tracks.end());
                                atmosphereEntry.tracks.erase(it);

                                // remove track
                                g_track_directory->remove_entry(currentTrackId);

                                update_ui_edit_atmosphereEdit(currentAtmosphereId);
                            });
                        }
    
                        // volume
                        {
                            nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                            nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_OFFSET);
                            nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_WIDTH);
                            nk_layout_row_template_push_dynamic(ctx);
                            nk_layout_row_template_push_static(ctx, MX_ROW_MUTE_WIDTH);
                            nk_layout_row_template_end(ctx);

                            nk_spacing(ctx, 1);
                            nk_label(ctx, "Volume:", NK_TEXT_LEFT);
                            const float volumeStep = 1.0f / nk_widget_width(ctx);
                            if(nk_slider_float(ctx, 0.0f, &currentTrack.volume, 1.0f, volumeStep))
                            {
                                worker(Worker_Background).enqueue([=]() -> void
                                {
                                    auto& trackEntry = g_track_directory->get_entry(currentTrackId);
                                    trackEntry.volume = currentTrack.volume;
                                    g_playback_manager->sync_track(currentTrackId);
                                });
                            }
                            if(nk_checkbox_label(ctx, "Mute", &currentTrack.muted))
                            {
                                worker(Worker_Background).enqueue([=]() -> void
                                {
                                    auto& trackEntry = g_track_directory->get_entry(currentTrackId);
                                    trackEntry.muted = currentTrack.muted ? true : false;
                                    g_playback_manager->sync_track(currentTrackId);
                                });
                            }
                        }
    
                        switch(currentTrack.audioType)
                        {
                            case ONE_SHOT:
                            {
                                // Repeat
                                {
                                    // Text input
                                    nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                                    nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_OFFSET);
                                    nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_WIDTH);
                                    nk_layout_row_template_push_static(ctx, MX_ROW_MINMAX_WIDTH);
                                    nk_layout_row_template_push_dynamic(ctx);
                                    nk_layout_row_template_push_static(ctx, MX_ROW_MINMAX_WIDTH);
                                    nk_layout_row_template_push_dynamic(ctx);
                                    nk_layout_row_template_end(ctx);

                                    nk_spacing(ctx, 1);
                                    nk_label(ctx, "Repeat:", NK_TEXT_LEFT);
                                    nk_label(ctx, "Min", NK_TEXT_CENTERED);
                                    if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx, 
                                        NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SIG_ENTER | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD, 
                                        currentTrack.oneShot.repeat.minBuffer, countof(currentTrack.oneShot.repeat.minBuffer), nk_filter_float))
                                    {
                                        float newVal;
                                        if(sscanf(currentTrack.oneShot.repeat.minBuffer, "%f", &newVal))
                                        {
                                            currentTrack.oneShot.repeat.min = NK_CLAMP(0.0f, newVal, currentTrack.oneShot.repeat.max);

                                            worker(Worker_Background).enqueue([=]() -> void
                                            {
                                                auto& trackEntry = g_track_directory->get_entry(currentTrack.trackId);
                                                trackEntry.oneShot.repeatMin = currentTrack.oneShot.repeat.min;
                                                g_playback_manager->sync_track(currentTrackId);
                                            });                                            
                                        }
                                        print_minmax(currentTrack.oneShot.repeat.minBuffer, countof(currentTrack.oneShot.repeat.minBuffer), currentTrack.oneShot.repeat.min);
                                    }
                                    nk_label(ctx, "Max", NK_TEXT_CENTERED);
                                    if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx, 
                                        NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SIG_ENTER | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD, 
                                        currentTrack.oneShot.repeat.maxBuffer, countof(currentTrack.oneShot.repeat.maxBuffer), nk_filter_float))
                                    {
                                        float newVal;
                                        if(sscanf(currentTrack.oneShot.repeat.maxBuffer, "%f", &newVal))
                                        {
                                            currentTrack.oneShot.repeat.max = NK_CLAMP(currentTrack.oneShot.repeat.min, newVal, 120.0f);

                                            worker(Worker_Background).enqueue([=]() -> void
                                            {
                                                auto& trackEntry = g_track_directory->get_entry(currentTrack.trackId);
                                                trackEntry.oneShot.repeatMax = currentTrack.oneShot.repeat.max;
                                                g_playback_manager->sync_track(currentTrackId);
                                            });                                            
                                        }
                                        print_minmax(currentTrack.oneShot.repeat.maxBuffer, countof(currentTrack.oneShot.repeat.maxBuffer), currentTrack.oneShot.repeat.max);
                                    }

                                    // MinMax Slider input
                                    {
                                        nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                                        nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_OFFSET);
                                        nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_WIDTH);
                                        nk_layout_row_template_push_dynamic(ctx);
                                        nk_layout_row_template_end(ctx);

                                        nk_spacing(ctx, 2);
                                        if(nk_minmax_slider_float(ctx, 0.0f, &currentTrack.oneShot.repeat.min, 120.0f, &currentTrack.oneShot.repeat.max, 0.1f))
                                        {
                                            snprintf(currentTrack.oneShot.repeat.minBuffer, countof(currentTrack.oneShot.repeat.minBuffer), "%#3.1f", currentTrack.oneShot.repeat.min);
                                            snprintf(currentTrack.oneShot.repeat.maxBuffer, countof(currentTrack.oneShot.repeat.maxBuffer), "%#3.1f", currentTrack.oneShot.repeat.max);

                                            worker(Worker_Background).enqueue([=]() -> void
                                            {
                                                auto& trackEntry = g_track_directory->get_entry(currentTrack.trackId);
                                                trackEntry.oneShot.repeatMin = currentTrack.oneShot.repeat.min;
                                                trackEntry.oneShot.repeatMax = currentTrack.oneShot.repeat.max;
                                                g_playback_manager->sync_track(currentTrackId);
                                            });
                                        }
                                    }
                                }
                                // Position
                                {
                                    nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                                    nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_OFFSET);
                                    nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_WIDTH);
                                    nk_layout_row_template_push_static(ctx, MX_ROW_XYZ_WIDTH);
                                    nk_layout_row_template_push_static(ctx, MX_ROW_XYZ_TEXT_WIDTH);
                                    nk_layout_row_template_push_dynamic(ctx);
                                    nk_layout_row_template_end(ctx);

                                    auto drawPositionComponent = [](nk_context* ctx, track_id_t trackId, char* componentBuffer, size_t componentBufferLen, float& component, size_t index)
                                    {
                                        assert(index < 3);

                                        const float oldVal = component;
                                        float newVal = component;
                                        bool updateBuffer = false;

                                        if(index == 0)
                                        {
                                            nk_spacing(ctx, 1);
                                            nk_label(ctx, "Position:", NK_TEXT_LEFT);
                                        }
                                        else
                                        {
                                            nk_spacing(ctx, 2);
                                        }
                                        const char* componentLabel[3] = {"X", "Y", "Z"};
                                        nk_label(ctx, componentLabel[index], NK_TEXT_LEFT);
                                        if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx,
                                            NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SIG_ENTER | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD, 
                                            componentBuffer, componentBufferLen, nk_filter_float))
                                        {
                                            if(sscanf(componentBuffer, "%f", &newVal))
                                            {
                                                newVal = NK_CLAMP(-1.0f, newVal, 1.0f);
                                            }
                                            updateBuffer = true;
                                        }
                                        if(nk_slider_float(ctx, -1.0f, &newVal, 1.0f, 0.01f))
                                        {
                                            updateBuffer = true;
                                        }

                                        component = newVal;
                                        if(updateBuffer)
                                        {
                                            print_position(componentBuffer, componentBufferLen, component);
                                        }

                                        if(oldVal != newVal)
                                        {
                                            worker(Worker_Background).enqueue([=]() -> void
                                            {
                                                auto& trackEntry = g_track_directory->get_entry(trackId);
                                                trackEntry.oneShot.position[index] = newVal;
                                                g_playback_manager->sync_track(trackId);
                                            });
                                        }
                                    };
                                    drawPositionComponent(ctx, currentTrack.trackId, currentTrack.oneShot.position.xBuffer, countof(currentTrack.oneShot.position.xBuffer), currentTrack.oneShot.position.x, 0);
                                    drawPositionComponent(ctx, currentTrack.trackId, currentTrack.oneShot.position.yBuffer, countof(currentTrack.oneShot.position.yBuffer), currentTrack.oneShot.position.y, 1);
                                    drawPositionComponent(ctx, currentTrack.trackId, currentTrack.oneShot.position.zBuffer, countof(currentTrack.oneShot.position.zBuffer), currentTrack.oneShot.position.z, 2);
                                }
                                break;
                            }
                            case LOOP:
                            {
                                break;
                            }
                        }
            
                    }
                }
                nk_group_end(ctx);
            }
            nk_layout_row_push(ctx, 1.0f/3.0f);
            {
                nk_group_begin(ctx, "edit_screen>right_group", NK_WINDOW_NO_SCROLLBAR);
                {
                    nk_layout_row_begin(ctx, NK_DYNAMIC, MX_TEXT_SEARCH_HEIGHT, 1);
                    nk_layout_row_push(ctx, 1.0f);
                    if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx, NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SIG_ENTER | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD, g_ui->edit.trackSearch.searchBuffer, countof(g_ui->edit.trackSearch.searchBuffer), nk_filter_default))
                    {
                        // do search and populate on background worker
                        std::string title_query = g_ui->edit.trackSearch.searchBuffer;
                        std::vector<std::string> tag_query = split(g_ui->edit.trackSearch.searchBuffer, " ");
                        
                        worker(Worker_Background).enqueue([=]() -> void 
                        {
                            auto title_results = g_audio_directory->get_audio_ids_by_title(title_query);
                            auto tag_results = g_audio_directory->get_audio_ids_by_tag(tag_query);
                            
                            std::unordered_set<audio_id_t> title_audio(title_results.begin(), title_results.end());
                            auto& aggregate_results = title_results;
                            for(auto id : tag_results)
                            {
                                // only include tag results if they weren't already found in title
                                if(title_audio.find(id) == title_audio.end())
                                {
                                    aggregate_results.push_back(id);
                                }
                            }
                            update_ui_edit_trackSearch(aggregate_results);
                        });
                    }

                    nk_layout_row_begin(ctx, NK_DYNAMIC, height - MX_TEXT_SEARCH_HEIGHT, 1);
                    nk_layout_row_push(ctx, 1.0f);
                    {
                        nk_group_begin(ctx, "edit_screen>right_group>search_results", 0);
                        {
                            for(const auto& currentTrack : g_ui->edit.trackSearch.results)
                            {
                                nk_layout_row_dynamic(ctx, 50, 1);
                                if(nk_widget_is_mouse_clicked(ctx, NK_BUTTON_DOUBLE))
                                {
                                    // create new track instance
                                    printf("clicked on : %s\n", currentTrack.title.c_str());

                                    worker(Worker_Background).enqueue([=]() -> void
                                    {
                                        atmosphere_id_t currentAtmosphereId = g_ui->edit.atmosphereList.selectedAtmosphereId();

                                        track_entry trackEntry;
                                        {
                                            auto& audioEntry = g_audio_directory->get_entry(currentTrack.audioId);

                                            trackEntry.title = audioEntry.title;
                                            trackEntry.audioId = currentTrack.audioId;
                                            trackEntry.audioType = audioEntry.data->get_type();
                                            switch(trackEntry.audioType)
                                            {
                                                case ONE_SHOT:
                                                    trackEntry.volume = 1.0f;
                                                    trackEntry.muted = false;
                                                    trackEntry.oneShot.repeatMin = 5.0f;
                                                    trackEntry.oneShot.repeatMax = 30.0f;
                                                    trackEntry.oneShot.x = 0.0f;
                                                    trackEntry.oneShot.y = 0.0f;
                                                    trackEntry.oneShot.z = 0.0f;
                                                    break;
                                                case LOOP:
                                                    break;
                                            }
                                        }

                                        // add data to directories
                                        track_id_t trackId = g_track_directory->add_entry(std::move(trackEntry));
                                        auto& currentAtmosphere = g_atmosphere_directory->get_entry(currentAtmosphereId);                                            
                                        currentAtmosphere.tracks.push_back(trackId);

                                        // begin playing
                                        g_playback_manager->play_track(trackId);

                                        // updates the ui
                                        update_ui_edit_atmosphereEdit(currentAtmosphereId);
                                    });
                                }
                                nk_label(ctx, currentTrack.title.c_str(), NK_TEXT_CENTERED);
                            }
                        }
                        nk_group_end(ctx);
                    }
                }
                nk_group_end(ctx);
            }
            nk_layout_row_end(ctx);
        }
        nk_end(ctx);
    }

    void draw_init(nk_context* ctx)
    {
        std::cout << "window padding: " << ctx->style.window.padding << std::endl;
        std::cout << "window spacing: " << ctx->style.window.spacing << std::endl;

        // set style
        ctx->style.window.group_border = MX_GROUP_BORDER;
        ctx->style.window.scrollbar_size = nk_vec2(10, 0);
        ctx->style.window.group_padding = nk_vec2(MX_GROUP_PADDING_X, MX_GROUP_PADDING_Y);
        ctx->style.window.spacing = nk_vec2(MX_GROUP_SPACING, MX_GROUP_SPACING);
        ctx->style.button.padding = nk_vec2(0, 0);
        ctx->style.scrollh.padding = nk_vec2(0, 0);
        ctx->style.scrollh.border = 0.0f;
    }

    void draw_update(nk_context* ctx, int width, int height)
    {
        // draw shared top bar
        draw_top_bar(ctx, width);

        draw_play_screen(ctx, width, height - MX_TOPBAR_HEIGHT, MX_MODE != MX_MODE_PLAY);
        draw_edit_screen(ctx, width, height - MX_TOPBAR_HEIGHT, MX_MODE != MX_MODE_EDIT);
    }
}