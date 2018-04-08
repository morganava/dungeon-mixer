#include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "tag_map.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "nuklear_extensions.hpp"
#include "serialization.hpp"

namespace tabletopify
{
    static UI ui;
    UI* g_ui = &ui;

    mode_t MX_MODE = MX_MODE_EDIT;

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

    static void update_ui_playback_atmosphereSearch(const std::vector<atmosphere_handle_t>& atmosphereHandles)
    {
        decltype(g_ui->playback.atmosphereSearch.results) searchResults;
        for(auto handle : atmosphereHandles)
        {
            auto& currentAtmosphere = g_atmosphere_directory->get_entry(handle);
            searchResults.push_back({handle, currentAtmosphere.title});
        }

        worker(Worker_UI).enqueue([searchResults = std::move(searchResults)] () -> void
        {
            g_ui->playback.atmosphereSearch.results = searchResults;
        });
    }

    static void update_ui_edit_atmosphereList(const std::vector<atmosphere_handle_t>& atmosphereHandles)
    {
        decltype(g_ui->edit.atmosphereList.atmospheres) atmospheres;
        for(auto handle : atmosphereHandles)
        {
            auto& currentAtmosphere = g_atmosphere_directory->get_entry(handle);
            atmospheres.push_back({currentAtmosphere.title, handle, false});
        }

        worker(Worker_UI).enqueue([atmospheres = std::move(atmospheres)]() -> void
        {
            g_ui->edit.atmosphereList.atmospheres = atmospheres;
        });
    }

    static void update_ui_edit_atmosphereEdit(atmosphere_handle_t atmosphereHandle)
    {
        decltype(g_ui->edit.atmosphereEdit.tracks) tracks;

        if(atmosphereHandle != MX_INVALID_HANDLE)
        {
            auto& selectedAtmosphere = g_atmosphere_directory->get_entry(atmosphereHandle);
            for(auto trackHandle : selectedAtmosphere.tracks)
            {
                auto& trackEntry = g_track_directory->get_entry(trackHandle);

                decltype(tracks)::value_type trackEdit;
                trackEdit.volume = trackEntry.volume;
                trackEdit.muted = trackEntry.muted ? 1 : 0;
                trackEdit.handle = trackHandle;
                trackEdit.title = trackEntry.audioTitle;
                trackEdit.audioType = trackEntry.audioType;
                trackEdit.position.enabled = trackEntry.position.enabled ? 1 : 0;

                switch(trackEntry.audioType)
                {
                    case ONE_SHOT:
                    {
                        trackEdit.repeat.min = trackEntry.repeat.min;
                        trackEdit.repeat.max = trackEntry.repeat.max;
                        print_minmax(trackEdit.repeat.minBuffer, countof(trackEdit.repeat.minBuffer), trackEdit.repeat.min);
                        print_minmax(trackEdit.repeat.maxBuffer, countof(trackEdit.repeat.maxBuffer), trackEdit.repeat.max);
                        trackEdit.position.x = trackEntry.position.x;
                        trackEdit.position.y = trackEntry.position.y;
                        trackEdit.position.z = trackEntry.position.z;
                        print_position(trackEdit.position.xBuffer, countof(trackEdit.position.xBuffer), trackEdit.position.x);
                        print_position(trackEdit.position.yBuffer, countof(trackEdit.position.yBuffer), trackEdit.position.y);
                        print_position(trackEdit.position.zBuffer, countof(trackEdit.position.zBuffer), trackEdit.position.z);
                        break;
                    }
                    case LOOP:
                    {
                        trackEdit.position.x = trackEntry.position.x;
                        trackEdit.position.y = trackEntry.position.y;
                        trackEdit.position.z = trackEntry.position.z;
                        print_position(trackEdit.position.xBuffer, countof(trackEdit.position.xBuffer), trackEdit.position.x);
                        print_position(trackEdit.position.yBuffer, countof(trackEdit.position.yBuffer), trackEdit.position.y);
                        print_position(trackEdit.position.zBuffer, countof(trackEdit.position.zBuffer), trackEdit.position.z);

                        break;
                    }
                }
                tracks.push_back(std::move(trackEdit));
            }
        }

        worker(Worker_UI).enqueue([tracks = std::move(tracks)]() -> void
        {
            g_ui->edit.atmosphereEdit.tracks = std::move(tracks);
        });
    }

    static void update_ui_edit_trackSearch(const std::vector<audio_handle_t>& audioHandles)
    {
        decltype(g_ui->edit.audioSearch.results) searchResults;
        for(auto currentHandle : audioHandles)
        {
            auto& audioEntry = g_audio_directory->get_entry(currentHandle);
            searchResults.push_back(
            {
                audioEntry.title,
                currentHandle
            });
        }

        worker(Worker_UI).enqueue([results = std::move(searchResults)]() -> void
        {
            g_ui->edit.audioSearch.results = std::move(results);
        });
    }

    static void enter_ui_playback()
    {
        if(MX_MODE != MX_MODE_PLAY)
        {
            worker(Worker_UI).enqueue([]() -> void
            {
                MX_MODE = MX_MODE_PLAY;
                for(auto& currentAtmosphere : g_ui->playback.atmosphereList.atmospheres)
                {
                    currentAtmosphere.playing = false;
                }
            });

            worker(Worker_Background).enqueue([]() -> void
            {
                g_playback_manager->stop_all_tracks();
                // update UI search results with current query (in case edit changed them)
                auto searchResults = g_atmosphere_directory->get_atmosphere_handles_by_query(g_ui->playback.atmosphereSearch.prevSearch);
                update_ui_playback_atmosphereSearch(searchResults);
            });
        }
    }

    static void enter_ui_edit()
    {
        if(MX_MODE != MX_MODE_EDIT)
        {
            worker(Worker_UI).enqueue([]() -> void
            {
                MX_MODE = MX_MODE_EDIT;
            });

            auto atmosphereHandle = g_ui->edit.atmosphereList.selectedAtmosphereHandle();
            worker(Worker_Background).enqueue([=]() -> void
            {
                g_playback_manager->stop_all_tracks();
                if(atmosphereHandle != MX_INVALID_HANDLE)
                {
                    g_playback_manager->play_atmosphere(atmosphereHandle);
                }

                auto searchResults = g_audio_directory->get_audio_handles_by_query(g_ui->edit.audioSearch.previousQuery);
                update_ui_edit_trackSearch(searchResults);
            });
        }
    }

    static void remove_atmosphere_from_favorites(size_t index, atmosphere_handle_t atmosphereHandle)
    {
        // remove this atmosphere from active list
        worker(Worker_UI).enqueue([=]() -> void
        {
            auto& atmospheres = g_ui->playback.atmosphereList.atmospheres;
            // disable playback of atmosphere if playing
            if(atmospheres[index].playing)
            {
                worker(Worker_Background).enqueue([=]() -> void
                {
                    g_playback_manager->stop_atmosphere(atmosphereHandle);
                });
            }
            atmospheres.erase(atmospheres.begin() + index);
        });

    }

    static void set_atmosphere_volume(atmosphere_handle_t atmosphereHandle, float volume)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereHandle);
            for(auto trackHandle : atmosphereEntry.tracks)
            {
                auto& currentTrack = g_track_directory->get_entry(trackHandle);
                currentTrack.atmosphereVolume = volume;
                g_playback_manager->sync_track(trackHandle);
            }
        });
    }

    static void set_atmosphere_mute(atmosphere_handle_t atmosphereHandle, bool muted)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereHandle);
            for(auto trackHandle : atmosphereEntry.tracks)
            {
                auto& currentTrack = g_track_directory->get_entry(trackHandle);
                currentTrack.atmosphereMuted = muted;
                g_playback_manager->sync_track(trackHandle);
            }
        });
    }

    static void set_atmosphere_playing(atmosphere_handle_t atmosphereHandle, bool playing)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            if(playing)
            {
                g_playback_manager->play_atmosphere(atmosphereHandle);
            }
            else
            {
                g_playback_manager->stop_atmosphere(atmosphereHandle);
            }
        });
    }

    static void do_atmosphere_query(std::string& query)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            auto searchResults = g_atmosphere_directory->get_atmosphere_handles_by_query(query);
            update_ui_playback_atmosphereSearch(searchResults);
        });
    }

    static void add_atmosphere_to_favorites(atmosphere_handle_t atmosphereHandle)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereHandle);

            worker(Worker_UI).enqueue([=]() -> void
            {
                g_ui->playback.atmosphereList.atmospheres.push_back(
                {
                    atmosphereHandle,
                    atmosphereEntry.title,
                    atmosphereEntry.id,
                    1.0f,
                    false,
                    false,
                });
            });
        });
    }


    static void select_atmosphere_for_edit(size_t idx)
    {
        if(g_ui->edit.atmosphereList.selectedIndex != idx )
        {
            worker(Worker_UI).enqueue([=]() -> void
            {
                g_ui->edit.atmosphereList.selectedIndex = idx;
                auto atmosphereHandle = g_ui->edit.atmosphereList.selectedAtmosphereHandle();
                worker(Worker_Background).enqueue([=]() -> void
                {
                    // populate the atmosphereEdit list based on the atmosphereHandle
                    update_ui_edit_atmosphereEdit(atmosphereHandle);
                    g_playback_manager->stop_all_tracks();
                    g_playback_manager->play_atmosphere(atmosphereHandle);
                });
            });
        }
    }

    static void add_new_atmosphere()
    {
        printf("Add atmosphere\n");
        worker(Worker_Background).enqueue([]() -> void
        {
            atmosphere_entry atmosphereEntry =
            {
                worker(Worker_Background).nextUUID(),
                "New Atmosphere",
                {}
            };
            auto newAtmosphereHandle = g_atmosphere_directory->add_entry(std::move(atmosphereEntry));
            auto atmosphereHandles = g_atmosphere_directory->get_atmosphere_handles();
            update_ui_edit_atmosphereList(atmosphereHandles);
            update_ui_edit_atmosphereEdit(newAtmosphereHandle);
            worker(Worker_UI).enqueue([]() -> void
            {
                g_ui->edit.atmosphereList.selectedIndex = g_ui->edit.atmosphereList.atmospheres.size() - 1;
            });
        });
    }

    static void remove_atmosphere(atmosphere_handle_t atmosphereHandle)
    {
        printf("Remove atmosphere\n");
        if(atmosphereHandle != MX_INVALID_HANDLE)
        {
            worker(Worker_Background).enqueue([=]() -> void
            {
                g_playback_manager->stop_atmosphere(atmosphereHandle);

                auto& deadAtmosphere = g_atmosphere_directory->get_entry(atmosphereHandle);
                for(auto trackHandle : deadAtmosphere.tracks)
                {
                    g_track_directory->remove_entry(trackHandle);
                }
                g_atmosphere_directory->remove_entry(atmosphereHandle);
                auto atmosphereHandles = g_atmosphere_directory->get_atmosphere_handles();
                update_ui_edit_atmosphereList(atmosphereHandles);
                update_ui_edit_atmosphereEdit(MX_INVALID_HANDLE);

                // remove the remove id from the atmosphere search list
                std::vector<atmosphere_handle_t> searchedAtmosphereHandles;
                for(auto& searchResult : g_ui->playback.atmosphereSearch.results)
                {
                    if(searchResult.handle != atmosphereHandle)
                    {
                        searchedAtmosphereHandles.push_back(searchResult.handle);
                    }
                }
                update_ui_playback_atmosphereSearch(searchedAtmosphereHandles);
                worker(Worker_UI).enqueue([]() -> void
                {
                    g_ui->edit.atmosphereList.selectedIndex = MX_INVALID_INDEX;
                });
            });
        }
    }

    static void begin_rename_atmosphere(auto& atmosphereEdit)
    {
        atmosphereEdit.renaming = true;
        g_ui->edit.atmosphereList.renameBuffer = atmosphereEdit.title;
    }

    static void finish_rename_atmosphere(auto& atmosphereEdit)
    {
        atmosphereEdit.renaming = false;
        atmosphereEdit.title = g_ui->edit.atmosphereList.renameBuffer;

        for(auto& atmospherePlayback : g_ui->playback.atmosphereList.atmospheres)
        {
            if(atmospherePlayback.handle == atmosphereEdit.handle)
            {
                atmospherePlayback.title = atmosphereEdit.title;
                break;
            }
        }

        worker(Worker_Background).enqueue([=]() -> void
        {
            g_atmosphere_directory->rename_entry(atmosphereEdit.handle, atmosphereEdit.title);
        });
    }

    static void remove_track(atmosphere_handle_t atmosphereHandle, track_handle_t trackHandle)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            // stop it from playing
            g_playback_manager->stop_track(trackHandle);

            // remove track from atmosphere
            auto& atmosphereEntry = g_atmosphere_directory->get_entry(atmosphereHandle);
            auto it = find(atmosphereEntry.tracks.begin(), atmosphereEntry.tracks.end(), trackHandle);
            MX_ASSERT(it != atmosphereEntry.tracks.end());
            atmosphereEntry.tracks.erase(it);

            // remove track
            g_track_directory->remove_entry(trackHandle);

            update_ui_edit_atmosphereEdit(atmosphereHandle);
        });
    }

    static void set_track_volume(track_handle_t trackHandle, float volume)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            auto& trackEntry = g_track_directory->get_entry(trackHandle);
            trackEntry.volume = volume;
            g_playback_manager->sync_track(trackHandle);
        });
    }

    static void set_track_mute(track_handle_t trackHandle, bool muted)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            auto& trackEntry = g_track_directory->get_entry(trackHandle);
            trackEntry.muted = muted;
            g_playback_manager->sync_track(trackHandle);
        });
    }

    static void set_track_repeat_minmax(track_handle_t trackHandle, float repeatMin, float repeatMax)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            auto& trackEntry = g_track_directory->get_entry(trackHandle);
            trackEntry.repeat.min = repeatMin;
            trackEntry.repeat.max = repeatMax;
            g_playback_manager->sync_track(trackHandle);
        });
    }

    static void set_track_position_enabled(track_handle_t trackHandle, bool positionEnabled)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            auto& trackEntry = g_track_directory->get_entry(trackHandle);
            trackEntry.position.enabled = positionEnabled;
            g_playback_manager->sync_track(trackHandle);
        });
    }

    static void set_track_position_component(track_handle_t trackHandle, size_t component, float value)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            auto& trackEntry = g_track_directory->get_entry(trackHandle);
            trackEntry.position.raw[component] = value;
            g_playback_manager->sync_track(trackHandle);
        });
    }

    static void do_audio_query(std::string& query)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            auto searchResults = g_audio_directory->get_audio_handles_by_query(query);
            update_ui_edit_trackSearch(searchResults);
        });
    }

    static void add_track(atmosphere_handle_t atmosphereHandle, audio_handle_t audioHandle)
    {
        worker(Worker_Background).enqueue([=]() -> void
        {
            track_entry trackEntry;
            {
                auto& audioEntry = g_audio_directory->get_entry(audioHandle);

                trackEntry.audioTitle = audioEntry.title;
                trackEntry.audioId = audioEntry.id;
                trackEntry.audioData = audioEntry.data;
                trackEntry.audioType = audioEntry.data->get_type();
                trackEntry.volume = 1.0f;
                trackEntry.muted = false;

                switch(trackEntry.audioType)
                {
                    case ONE_SHOT:
                        trackEntry.repeat.min = 5.0f;
                        trackEntry.repeat.max = 30.0f;
                        trackEntry.position.enabled = true;
                        break;
                    case LOOP:
                        trackEntry.position.enabled = false;
                        break;
                }

                trackEntry.position.x = 0.0f;
                trackEntry.position.y = 0.0f;
                trackEntry.position.z = 0.0f;
            }

            // add data to directories
            track_handle_t trackHandle = g_track_directory->add_entry(std::move(trackEntry));
            auto& currentAtmosphere = g_atmosphere_directory->get_entry(atmosphereHandle);
            currentAtmosphere.tracks.push_back(trackHandle);

            // begin playing
            g_playback_manager->play_track(trackHandle);

            // updates the ui
            update_ui_edit_atmosphereEdit(atmosphereHandle);
        });
    }

    /*
    Draw Calls
    */

    void draw_top_bar(nk_context* ctx, int width)
    {
        if (nk_begin(ctx, "top_bar", nk_rect(0, 0, width, MX_TOPBAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_template_begin(ctx, MX_TOPBAR_HEIGHT);
            nk_layout_row_template_push_static(ctx, MX_MODE_BUTTON_WIDTH);
            nk_layout_row_template_push_static(ctx, MX_MODE_BUTTON_WIDTH);
            nk_layout_row_template_push_dynamic(ctx);
            nk_layout_row_template_push_static(ctx, MX_MODE_BUTTON_WIDTH);
            nk_layout_row_template_push_static(ctx, MX_MODE_BUTTON_WIDTH);
            nk_layout_row_template_end(ctx);

            if(nk_button_label(ctx, "OPEN"))
            {
                auto openPath = open_dialog("json");
                if(!openPath.empty())
                {
                    open_user_mix(openPath);
                }

            }
            if(nk_button_label(ctx, "SAVE"))
            {
                auto savePath = save_dialog("json");
                if(!savePath.empty())
                {
                    save_user_mix(savePath);
                }
            }
            nk_spacing(ctx, 1);
            if(nk_button_label(ctx, "Playback"))
            {
                enter_ui_playback();
            }
            if(nk_button_label(ctx, "Edit"))
            {
                enter_ui_edit();
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
                                remove_atmosphere_from_favorites(k, currentAtmosphere.handle);
                            }

                            nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                            nk_layout_row_template_push_dynamic(ctx);
                            nk_layout_row_template_push_static(ctx, MX_BUTTON_SIZE);
                            nk_layout_row_template_push_static(ctx, MX_BUTTON_SIZE);
                            nk_layout_row_template_end(ctx);

                            const float volumeStep = 0.001f;
                            if(nk_slider_float(ctx, 0.0f, &currentAtmosphere.volume, 1.0f, volumeStep))
                            {
                                set_atmosphere_volume(currentAtmosphere.handle, currentAtmosphere.volume);
                            }
                            if(nk_button_label(ctx, "V"))
                            {
                                currentAtmosphere.muted = !currentAtmosphere.muted;
                                set_atmosphere_mute(currentAtmosphere.handle, currentAtmosphere.muted);
                            }
                            if(nk_button_label(ctx, currentAtmosphere.playing ? "S" : "P"))
                            {
                                currentAtmosphere.playing = !currentAtmosphere.playing;
                                set_atmosphere_playing(currentAtmosphere.handle, currentAtmosphere.playing);
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

                    if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx,
                        NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SIG_ENTER | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD,
                        g_ui->playback.atmosphereSearch.searchBuffer, nk_filter_default))
                    {
                        std::string query = g_ui->playback.atmosphereSearch.searchBuffer.buffer();
                        auto& previousQuery = g_ui->playback.atmosphereSearch.prevSearch;

                        if(previousQuery != query)
                        {
                            previousQuery = query;
                            do_atmosphere_query(query);
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
                                    add_atmosphere_to_favorites(currentAtmosphere.handle);
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

        auto selectedAtmosphereHandle = g_ui->edit.atmosphereList.selectedAtmosphereHandle();;

        if (nk_begin(ctx, "edit_screen", nk_rect(win_x, MX_TOPBAR_HEIGHT, width, height), NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_begin(ctx, NK_DYNAMIC, height, 3);
            nk_layout_row_push(ctx, 1.0f/3.0f);
            {
                nk_group_begin(ctx, "edit_screen>left_group", NK_WINDOW_NO_SCROLLBAR);
                {
                    nk_layout_row_begin(ctx, NK_DYNAMIC, height - MX_ATMOSPHERE_BUTTON, 1);
                    nk_layout_row_push(ctx, 1.0f);
                    {
                        nk_group_begin(ctx, "edit_screen>left_group>atmosphere_list", 0);
                        {
                            for(size_t idx = 0; idx < g_ui->edit.atmosphereList.atmospheres.size(); ++idx)
                            {
                                auto& currentAtmosphere = g_ui->edit.atmosphereList.atmospheres[idx];

                                nk_layout_row_dynamic(ctx, MX_ROW_HEIGHT, 1);
                                if(nk_widget_is_mouse_clicked(ctx, NK_BUTTON_DOUBLE))
                                {
                                    begin_rename_atmosphere(currentAtmosphere);
                                }
                                else if(currentAtmosphere.renaming == false &&
                                        nk_widget_is_mouse_clicked(ctx, NK_BUTTON_LEFT))
                                {
                                    printf("Selected %s\n", currentAtmosphere.title.c_str());
                                    select_atmosphere_for_edit(idx);
                                }
                                if(currentAtmosphere.renaming)
                                {
                                    constexpr auto editFlags = NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SIG_ENTER | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD;
                                    nk_edit_focus(ctx, editFlags);
                                    if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx,
                                        editFlags, g_ui->edit.atmosphereList.renameBuffer, nk_filter_default))
                                    {
                                        finish_rename_atmosphere(currentAtmosphere);
                                    }
                                }
                                else
                                {
                                    nk_label(ctx, currentAtmosphere.title.c_str(), NK_TEXT_ALIGN_LEFT);
                                }
                            }
                        }
                        nk_group_end(ctx);
                    }

                    nk_layout_row_template_begin(ctx, MX_ATMOSPHERE_BUTTON);
                    nk_layout_row_template_push_static(ctx, MX_ATMOSPHERE_BUTTON);
                    nk_layout_row_template_push_dynamic(ctx);
                    nk_layout_row_template_push_static(ctx, MX_ATMOSPHERE_BUTTON);
                    nk_layout_row_template_end(ctx);
                    if(nk_button_label(ctx, "+"))
                    {
                        add_new_atmosphere();
                    }
                    nk_spacing(ctx, 1);
                    if(nk_button_label(ctx, "-"))
                    {
                        remove_atmosphere(selectedAtmosphereHandle);
                    }
                }
                nk_group_end(ctx);
            }
            nk_layout_row_push(ctx, 1.0f/3.0f);
            {
                nk_group_begin(ctx, "edit_screen>middle_group", 0);
                {
                    auto currentAtmosphereHandle = g_ui->edit.atmosphereList.selectedAtmosphereHandle();
                    const size_t trackCount = g_ui->edit.atmosphereEdit.tracks.size();
                    for(size_t trackIndex = 0 ; trackIndex < trackCount; ++trackIndex)
                    {
                        auto& currentTrack = g_ui->edit.atmosphereEdit.tracks[trackIndex];
                        auto currentTrackHandle = currentTrack.handle;

                        nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                        nk_layout_row_template_push_dynamic(ctx);
                        nk_layout_row_template_push_static(ctx, MX_CLOSE_BUTTON_SIZE);
                        nk_layout_row_template_end(ctx);

                        nk_label(ctx, currentTrack.title.c_str(), NK_TEXT_LEFT);

                        if(nk_button_label(ctx, "X"))
                        {
                            remove_track(currentAtmosphereHandle, currentTrack.handle);
                        }

                        struct
                        {
                            bool volume = false;
                            bool repeat = false;
                            bool position = false;
                        } widgets;

                        switch(currentTrack.audioType)
                        {
                            case ONE_SHOT:
                                widgets.volume = true;
                                widgets.repeat = true;
                                widgets.position = true;
                                break;
                            case LOOP:
                                widgets.volume = true;
                                widgets.position = true;
                                break;
                        }

                        // volume
                        if(widgets.volume)
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
                                set_track_volume(currentTrackHandle, currentTrack.volume);
                            }
                            if(nk_checkbox_label(ctx, "Mute", &currentTrack.muted))
                            {
                                set_track_mute(currentTrackHandle, currentTrack.muted);
                            }
                        }

                        // Repeat
                        if(widgets.repeat)
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
                                currentTrack.repeat.minBuffer, countof(currentTrack.repeat.minBuffer), nk_filter_float))
                            {
                                float newVal;
                                if(sscanf(currentTrack.repeat.minBuffer, "%f", &newVal))
                                {
                                    currentTrack.repeat.min = NK_CLAMP(0.0f, newVal, currentTrack.repeat.max);
                                    set_track_repeat_minmax(currentTrack.handle, currentTrack.repeat.min, currentTrack.repeat.max);
                                }
                                print_minmax(currentTrack.repeat.minBuffer, countof(currentTrack.repeat.minBuffer), currentTrack.repeat.min);
                            }
                            nk_label(ctx, "Max", NK_TEXT_CENTERED);
                            if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx,
                                NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SIG_ENTER | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD,
                                currentTrack.repeat.maxBuffer, countof(currentTrack.repeat.maxBuffer), nk_filter_float))
                            {
                                float newVal;
                                if(sscanf(currentTrack.repeat.maxBuffer, "%f", &newVal))
                                {
                                    currentTrack.repeat.max = NK_CLAMP(currentTrack.repeat.min, newVal, 120.0f);
                                    set_track_repeat_minmax(currentTrack.handle, currentTrack.repeat.min, currentTrack.repeat.max);
                                }
                                print_minmax(currentTrack.repeat.maxBuffer, countof(currentTrack.repeat.maxBuffer), currentTrack.repeat.max);
                            }

                            // MinMax Slider input
                            {
                                nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                                nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_OFFSET);
                                nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_WIDTH);
                                nk_layout_row_template_push_dynamic(ctx);
                                nk_layout_row_template_end(ctx);

                                nk_spacing(ctx, 2);
                                if(nk_minmax_slider_float(ctx, 0.0f, &currentTrack.repeat.min, 120.0f, &currentTrack.repeat.max, 0.1f))
                                {
                                    snprintf(currentTrack.repeat.minBuffer, countof(currentTrack.repeat.minBuffer), "%#3.1f", currentTrack.repeat.min);
                                    snprintf(currentTrack.repeat.maxBuffer, countof(currentTrack.repeat.maxBuffer), "%#3.1f", currentTrack.repeat.max);

                                    set_track_repeat_minmax(currentTrack.handle, currentTrack.repeat.min, currentTrack.repeat.max);
                                }
                            }
                        }
                        // Position
                        if(widgets.position)
                        {
                            nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                            nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_OFFSET);
                            nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_WIDTH);
                            nk_layout_row_template_push_dynamic(ctx);
                            nk_layout_row_template_end(ctx);

                            nk_spacing(ctx, 1);
                            nk_label(ctx, "Position:", NK_TEXT_LEFT);
                            if(nk_checkbox_label(ctx, "Enabled", &currentTrack.position.enabled))
                            {
                                set_track_position_enabled(currentTrack.handle, currentTrack.position.enabled);
                            }

                            if(currentTrack.position.enabled)
                            {
                                nk_layout_row_template_begin(ctx, MX_ROW_HEIGHT);
                                nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_OFFSET);
                                nk_layout_row_template_push_static(ctx, MX_ROW_LABEL_WIDTH);
                                nk_layout_row_template_push_static(ctx, MX_ROW_XYZ_WIDTH);
                                nk_layout_row_template_push_static(ctx, MX_ROW_XYZ_TEXT_WIDTH);
                                nk_layout_row_template_push_dynamic(ctx);
                                nk_layout_row_template_end(ctx);

                                auto drawPositionComponent = [](nk_context* ctx, track_handle_t trackHandle, char* componentBuffer, size_t componentBufferLen, float& component, size_t index)
                                {
                                    MX_ASSERT(index < 3);

                                    const float oldVal = component;
                                    float newVal = component;
                                    bool updateBuffer = false;

                                    nk_spacing(ctx, 2);

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
                                        set_track_position_component(trackHandle, index, newVal);
                                    }
                                };
                                drawPositionComponent(ctx, currentTrack.handle, currentTrack.position.xBuffer, countof(currentTrack.position.xBuffer), currentTrack.position.x, 0);
                                drawPositionComponent(ctx, currentTrack.handle, currentTrack.position.yBuffer, countof(currentTrack.position.yBuffer), currentTrack.position.y, 1);
                                drawPositionComponent(ctx, currentTrack.handle, currentTrack.position.zBuffer, countof(currentTrack.position.zBuffer), currentTrack.position.z, 2);
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
                    if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx,
                        NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SIG_ENTER | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD,
                        g_ui->edit.audioSearch.searchBuffer,
                        nk_filter_default))
                    {
                        // do search and populate on background worker
                        std::string query = g_ui->edit.audioSearch.searchBuffer;
                        auto& previousQuery = g_ui->edit.audioSearch.previousQuery;

                        if(previousQuery != query)
                        {
                            previousQuery = query;
                            do_audio_query(query);
                        }
                    }

                    nk_layout_row_begin(ctx, NK_DYNAMIC, height - MX_TEXT_SEARCH_HEIGHT, 1);
                    nk_layout_row_push(ctx, 1.0f);
                    {
                        nk_group_begin(ctx, "edit_screen>right_group>search_results", 0);
                        {
                            for(const auto& currentAudio : g_ui->edit.audioSearch.results)
                            {
                                nk_layout_row_dynamic(ctx, 50, 1);
                                if(nk_widget_is_mouse_clicked(ctx, NK_BUTTON_DOUBLE))
                                {
                                    add_track(g_ui->edit.atmosphereList.selectedAtmosphereHandle(), currentAudio.handle);
                                }
                                nk_label(ctx, currentAudio.title.c_str(), NK_TEXT_CENTERED);
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

        ctx->style.window.border = 0.0f;
        ctx->style.window.group_border = 0.0f;
        ctx->style.window.padding = nk_vec2(0, 0);
        ctx->style.window.group_padding = nk_vec2(0, 0);

        ctx->style.window.scrollbar_size = nk_vec2(10, 0);
        ctx->style.window.spacing = nk_vec2(0, 0);

        ctx->style.button.padding = nk_vec2(0, 0);
        ctx->style.button.border = 0.0f;

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