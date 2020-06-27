#include "ui.hpp"

namespace tabletopify
{
    // Edit Screen Methods

    void ui::on_do_audio_search(const utf8_string& query)
    {
        worker_background.enqueue([=]() -> void
        {
            auto audioHandles = g_audio_directory.get_audio_handles_by_query(query);
            std::vector<audio_query_result_widget> queryWidgets;
            for(auto handle : audioHandles) {
                const auto& entry = g_audio_directory.get_entry(handle);
                queryWidgets.push_back({handle, {entry.title}});
            }

            worker_ui.enqueue([queryWidgets=std::move(queryWidgets)]() mutable -> void
            {
                g_ui.root.edit.search.queryResults = std::move(queryWidgets);
            });
        });
    }

    void ui::on_add_audio_to_atmosphere(atmosphere_handle_t atmosphereHandle, audio_handle_t audioHandle)
    {
        // create new track for atmosphere and begin playing it
        worker_background.enqueue([=]() -> void
        {
            auto& audioEntry = g_audio_directory.get_entry(audioHandle);

            auto trackTitle = audioEntry.title;

            // create new track entry for atmosphere
            track_entry trackEntry;
            trackEntry.atmosphereHandle = atmosphereHandle;
            trackEntry.audioTitle = trackTitle;
            trackEntry.audioData = audioEntry.data;

            // add it to the track directory
            auto trackHandle = g_track_directory.add_entry(std::move(trackEntry));

            // add track to atmosphere
            auto& atmosphereEntry = g_atmosphere_directory.get_entry(atmosphereHandle);
            atmosphereEntry.tracks.push_back(trackHandle);

            // add track widget to ui
            worker_ui.enqueue([=,title=std::move(trackTitle)]() -> void
            {
                track_edit_widget widget;
                widget.visible.checked = true;
                widget.handle = trackHandle;
                widget.audio_title = std::move(title);
                widget.volume_slider.value = 1.0;

                g_ui.root.edit.tracks.widgets.push_back(std::move(widget));
            });

            // finally play the track
            g_playback_manager.play_track(trackHandle);
        });
    }

    void ui::on_set_track_volume(track_handle_t trackHandle, double volume)
    {
        worker_background.enqueue([=]() -> void
        {
            auto& entry = g_track_directory.get_entry(trackHandle);
            entry.volume = volume;
            g_playback_manager.sync_track(trackHandle);
        });
    }

    void ui::on_set_track_position_enabled(track_handle_t trackHandle, bool enabled)
    {
        worker_background.enqueue([=]() -> void
        {
            auto& entry = g_track_directory.get_entry(trackHandle);
            entry.position.enabled = enabled;
            g_playback_manager.sync_track(trackHandle);
        });
    }

    void ui::on_set_track_azimuth(track_handle_t trackHandle, double azimuth)
    {
        worker_background.enqueue([=]() -> void
        {
            auto& entry = g_track_directory.get_entry(trackHandle);
            entry.position.azimuth = azimuth;
            g_playback_manager.sync_track(trackHandle);
        });
    }

    void ui::on_set_track_elevation(track_handle_t trackHandle, double elevation)
    {
        worker_background.enqueue([=]() -> void
        {
            auto& entry = g_track_directory.get_entry(trackHandle);
            entry.position.elevation = elevation;
            g_playback_manager.sync_track(trackHandle);
        });
    }

    // Draw

    void ui::draw_edit(float dt)
    {
        mxVBox(self.tags={"content"})
        {
            mxLabel(&root.edit.heading, self.tags= {"header", "darkText"});
            mxButton(g_ui.root.edit.tracks.title, self.tags = {"header", "title", "lightText"});
            mxHBox(self.tags={"editButtonRow"})
            {
                mxSpacer();
                mxTextBox(&root.edit.searchBox, self.tags = {"search"};
                self.on_submit = [](const utf8_string& query) -> void
                {
                    g_ui.on_do_audio_search(query);
                });
            }
            mxHBox(self.tags = {"editHBox"})
            {
                // track edit screen
                mxVScroll(&root.edit.tracks.scroll)
                {
                    for(auto& editWidget : root.edit.tracks.widgets)
                    {
                        const auto trackHandle = editWidget.handle;
                        mxHBox(self.tags={"trackTitle"})
                        {
                            mxCheckbox(&editWidget.visible, self.tags={"entryTitle"});
                            mxLabel(editWidget.audio_title, self.tags={"entryTitle"});
                        }
                        if (editWidget.visible.checked)
                        {
                            mxHBox(self.tags={"trackSetting"})
                            {
                                mxLabel({"VOLUME"});
                                mxSlider(&editWidget.volume_slider,
                                self.on_change = [=](double value) -> void
                                {
                                    g_ui.on_set_track_volume(trackHandle, value);
                                });
                            }
                            mxHBox(self.tags={"trackSetting"})
                            {
                                mxLabel({"POSITION"});
                                mxHBox(self.tags={"trackPositionCheckbox"})
                                {
                                    mxCheckbox(&editWidget.position_checkbox,
                                    self.on_change = [=](bool value) -> void{
                                        g_ui.on_set_track_position_enabled(trackHandle, value);
                                    });
                                    mxLabel({"ENABLED"});
                                }
                            }
                            if (editWidget.position_checkbox.checked)
                            {
                                // azimuth is the horizontal angle
                                mxHBox(self.tags={"trackSetting"})
                                {
                                    mxLabel({"AZIMUTH"});
                                    mxSlider(&editWidget.azimuth_slider,
                                    self.on_change = [=](double value) -> void
                                    {
                                        g_ui.on_set_track_azimuth(trackHandle, value);
                                    });
                                }
                                mxHBox(self.tags={"trackSetting"})
                                {
                                    mxLabel({"ELEVATION"});
                                    mxSlider(&editWidget.elevation_slider,
                                    self.on_change = [=](double value) -> void
                                    {
                                        g_ui.on_set_track_elevation(trackHandle, value);
                                    });
                                }
                            }
                            mxHBox(self.tags={"trackSetting"})
                            {
                                mxLabel({"REPEAT"});
                                mxHBox(self.tags={"trackPositionCheckbox"})
                                {
                                    mxCheckbox(&editWidget.repeat_checkbox);
                                    mxLabel({"ENABLED"});
                                }
                            }
                        }
                    }
                }
                // track search results
                mxVScroll(&root.edit.search.scroll)
                {
                    for(auto& queryResult : root.edit.search.queryResults)
                    {
                        mxButton(queryResult.title, self.tags={"audioResult"};
                        self.on_click = [&]() -> void
                        {
                            g_ui.on_add_audio_to_atmosphere(g_ui.root.edit.handle, queryResult.handle);
                        });
                        mxSpacer();
                    }
                }
            }
        }
    }
}