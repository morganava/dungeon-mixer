#include "ui.hpp"

namespace tabletopify
{
    // Edit Screen Methods

    void ui::on_start_atmosphere_rename(atmosphere_handle_t atmosphere)
    {
        worker_ui.enqueue([=]() -> void
        {
            g_ui.root.edit.edit_title = true;
            g_ui.root.edit.title_textbox = imgui::text_box_data(g_ui.root.edit.title, "Atmosphere Name");
            g_ui.root.edit.title_textbox.selectBegin = 0;
            g_ui.root.edit.title_textbox.selectEnd = g_ui.root.edit.title_textbox.sb.size();
            imgui::g_context.focus_widget(g_ui.root.edit.title_textbox.id);
        });
    }

    void ui::on_set_atmosphere_name(atmosphere_handle_t atmosphereHandle, utf8_string name)
    {
        worker_ui.enqueue([=,name=std::move(name)]() -> void
        {
            g_ui.root.edit.title = name;
            g_ui.root.edit.edit_title = false;

            auto& atmosphereWidget = g_ui.get_atmosphere_widget(atmosphereHandle);
            atmosphereWidget.title.text = name;

            worker_background.enqueue([=,name=std::move(name)]() mutable -> void
            {
                g_atmosphere_directory.set_title(atmosphereHandle, std::move(name));
            });
        });
    }

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
            trackEntry.audio = audioHandle;

            const auto audioType = audioEntry.type();
            if (audioType == audio_type::one_shot)
            {
                trackEntry.repeat.enabled = true;
            }

            // add it to the track directory
            auto trackHandle = g_track_directory.add_entry(std::move(trackEntry));

            // add track to atmosphere
            g_atmosphere_directory.add_track(atmosphereHandle, trackHandle);

            // add track widget to ui
            worker_ui.enqueue([=,title=std::move(trackTitle)]() -> void
            {
                track_edit_widget widget =
                {
                    trackHandle,
                    audioType,
                    std::move(title)
                };

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
            g_track_directory.set_volume(trackHandle, volume);
            g_playback_manager.sync_track(trackHandle);
        });
    }

    void ui::on_set_track_position_enabled(track_handle_t trackHandle, bool enabled)
    {
        worker_background.enqueue([=]() -> void
        {
            g_track_directory.set_position_enabled(trackHandle, enabled);
            g_playback_manager.sync_track(trackHandle);
        });
    }

    void ui::on_set_track_azimuth(track_edit_widget& widget, double azimuth)
    {
        widget.azimuth_string = fmt::format("{:.1f}°", azimuth);
        const auto trackHandle = widget.handle;

        worker_background.enqueue([=]() -> void
        {
            g_track_directory.set_position_azimuth(trackHandle, azimuth);
            g_playback_manager.sync_track(trackHandle);
        });
    }

    void ui::on_set_track_elevation(track_edit_widget& widget, double elevation)
    {
        widget.elevation_string = fmt::format("{:.1f}°", elevation);
        const auto trackHandle = widget.handle;

        worker_background.enqueue([=]() -> void
        {
            g_track_directory.set_position_elevation(trackHandle, elevation);
            g_playback_manager.sync_track(trackHandle);
        });
    }

    void ui::on_set_track_repeat_enabled(track_handle_t trackHandle, bool enabled)
    {
        worker_background.enqueue([=]() -> void
        {
            g_track_directory.set_repeat_enabled(trackHandle, enabled);
            g_playback_manager.sync_track(trackHandle);
        });
    }

    void ui::on_set_track_repeat(track_edit_widget& widget, double minRepeat, double maxRepeat)
    {
        widget.repeat_min_string = fmt::format("{:.1f}s", minRepeat);
        widget.repeat_max_string = fmt::format("{:.1f}s", maxRepeat);
        const auto trackHandle = widget.handle;

        worker_background.enqueue([=]() -> void
        {
            g_track_directory.set_repeat_min(trackHandle, minRepeat);
            g_track_directory.set_repeat_max(trackHandle, maxRepeat);
            g_playback_manager.sync_track(trackHandle);
        });
    }

    // Draw

    void ui::draw_edit(float dt)
    {
        const auto currentAtmosphereHandle = g_ui.root.edit.handle;
        mxVBox(self.tags={"content"})
        {
            mxLabel(&root.edit.heading, self.tags= {"header", "darkText"});
            if (root.edit.edit_title)
            {
                mxTextBox(&root.edit.title_textbox, self.tags = {"header", "title", "lightText"},
                self.on_submit = [=](const utf8_string& str) -> void
                {
                    g_ui.on_set_atmosphere_name(currentAtmosphereHandle, str);
                };
                if (!self.is_focused())
                {
                    g_ui.on_set_atmosphere_name(currentAtmosphereHandle, self.text());
                });
            }
            else
            {
                mxButton(g_ui.root.edit.title, self.tags = {"header", "title", "lightText"},
                self.on_double_click = [=]() -> void{
                    g_ui.on_start_atmosphere_rename(currentAtmosphereHandle);
                });
            }

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
                            mxLabel(editWidget.title, self.tags={"entryTitle"});
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
                                    mxHBox(self.tags={"sliderSetting"})
                                    {
                                        mxSlider(&editWidget.azimuth_slider,
                                        self.on_change = [&](double value) -> void
                                        {
                                            g_ui.on_set_track_azimuth(editWidget, value);
                                        });
                                        mxLabel(editWidget.azimuth_string);
                                    }
                                }
                                // elevation is the vertical angle
                                mxHBox(self.tags={"trackSetting"})
                                {
                                    mxLabel({"ELEVATION"});
                                    mxHBox(self.tags={"sliderSetting"})
                                    {
                                        mxSlider(&editWidget.elevation_slider,
                                        self.on_change = [&](double value) -> void
                                        {
                                            g_ui.on_set_track_elevation(editWidget, value);
                                        });
                                        mxLabel(editWidget.elevation_string);
                                    }
                                }
                            }

                            if (editWidget.type == audio_type::one_shot)
                            {
                                mxHBox(self.tags={"trackSetting"})
                                {
                                    mxLabel({"REPEAT"});
                                    mxHBox(self.tags={"trackPositionCheckbox"})
                                    {
                                        mxCheckbox(&editWidget.repeat_checkbox,
                                        self.on_change=[=](bool value)
                                        {
                                            g_ui.on_set_track_repeat_enabled(trackHandle, value);
                                        });
                                        mxLabel({"ENABLED"});
                                    }
                                }
                                if (editWidget.repeat_checkbox.checked)
                                {
                                    mxHBox(self.tags={"trackSetting"})
                                    {
                                        mxLabel({"DELAY"});
                                        mxHBox(self.tags={"rangeSliderSetting"})
                                        {
                                            mxLabel(editWidget.repeat_min_string);
                                            mxRangeSlider(&editWidget.repeat_slider,
                                            self.on_change=[&](double low, double high)
                                            {
                                                g_ui.on_set_track_repeat(editWidget, low, high);
                                            });
                                            mxLabel(editWidget.repeat_max_string);
                                        }
                                    }
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