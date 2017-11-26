#include "precomp.hpp"

// tabletopify
#include "audio.hpp"
#include "utilities.hpp"
#include "globals.hpp"

namespace tabletopify
{

    /* 
    Constants 
    */

    constexpr int MX_TOPBAR_HEIGHT = 100;
    constexpr int MX_GROUP_MARGIN = 0;
    constexpr int MX_TEXT_PADDING = 0;
    constexpr int MX_BUTTON_PADDING = 0;
    constexpr int MX_WIDGET_MARGIN = 2;
    constexpr int MX_CLOSE_BUTTON_SIZE = 24;

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

    constexpr int MX_VOLUME_HEIGHT = 32;
    constexpr int MX_VOLUME_BUTTON_SIZE = 32;

    /* 
    State 
    */

    enum 
    {
        MX_MODE_PLAY,
        MX_MODE_EDIT,
    } MX_MODE = MX_MODE_EDIT;

    /*
    Mock Data
    */

    struct atmosphere
    {
        std::string title;
        float volume;
    };

    std::vector<atmosphere> active_atmospheres =
    {
        {"Busy Tavern", 1.0f},
        {"Esga City Streets", 0.8f},
        {"Sailing", 0.7f},
        {"Below Decks", 0.7f},
        {"On the docks", 0.7f},
        {"Quiet Library", 0.2f},
        {"Camping", 0.95f},
        {"Thunderstorm", 1.0f},
        {"Tundra", 0.2f},
        {"Boss Music", 1.0f},
        {"Esga Temple", 0.62f},
    };

    std::vector<atmosphere> searched_atmospheres =
    {
        {"Spooky Ship", 0.8f},
        {"Workshop", 0.9f},
        {"Opera", 1.0f},
        {"Market", 0.7f},
        {"Jungle", 0.8f},
        {"Forest", 0.75f},
        {"Desert", 0.5f},
        {"Rain", 0.7f},
        {"Battle", 0.8f},
        {"Sports Game", 1.0f},
        {"Loud Fucking", 0.5f},
    };

    std::vector<atmosphere> available_atmospheres =
    {
        {"Spooky Ship", 0.8f},
        {"Workshop", 0.9f},
        {"Opera", 1.0f},
        {"Market", 0.7f},
        {"Jungle", 0.8f},
        {"Forest", 0.75f},
        {"Desert", 0.5f},
        {"Rain", 0.7f},
        {"Battle", 0.8f},
        {"Sports Game", 1.0f},
        {"Loud Fucking", 0.5f},
        {"Busy Tavern", 1.0f},
        {"Esga City Streets", 0.8f},
        {"Sailing", 0.7f},
        {"Below Decks", 0.7f},
        {"On the docks", 0.7f},
        {"Quiet Library", 0.2f},
        {"Camping", 0.95f},
        {"Thunderstorm", 1.0f},
        {"Tundra", 0.2f},
        {"Boss Music", 1.0f},
        {"Esga Temple", 0.62f},    
    };


    struct track_data
    {
        std::string title;
        std::vector<std::string> tags;
        enum
        {
            ONE_SHOT,
            LOOP,
        } type;
        const char* path;
    };

    atmosphere edit_atmosphere = available_atmospheres[0];

    struct instantiated_track
    {
        struct track_data track;
        float volume;
        char min_buffer[5];
        char max_buffer[5];
    };

    std::vector<track_data> available_tracks =
    {
        {"Sawing", {"tools", "construction"}, track_data::ONE_SHOT, "~/Downloads/sawing.wav"},
        {"Seagulls", {"ocean", "birds"}, track_data::LOOP, "~/Downloads/seagulls.wav"},
    };

    std::vector<instantiated_track> instantiated_tracks =
    {
        {{available_tracks[0]}, 1.0f},
        {{available_tracks[1]}, 0.5f},
    };

    /*
    Real Data
    */

    std::vector<size_t> searched_track_ids;

    char atmosphere_search_buffer[MX_TEXT_FIELD_LENGTH];
    char track_search_buffer[MX_TEXT_FIELD_LENGTH];

    /* 
    Draw Calls
    */  

    void draw_top_bar(nk_context* ctx, int width)
    {
        if (nk_begin(ctx, "top_bar", nk_rect(0, 0, width, MX_TOPBAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR))
        {
            nk_layout_row_begin(ctx, NK_STATIC, MX_TOPBAR_HEIGHT, 3);
            nk_layout_row_push(ctx, width - 2 * MX_MODE_BUTTON_WIDTH - 4 * MX_WIDGET_MARGIN);
            //nk_button_label(ctx, "What");
            nk_spacing(ctx, 1);
            nk_layout_row_push(ctx, MX_MODE_BUTTON_WIDTH);
            if(nk_button_label(ctx, "Playback"))
            {
                work_queue.push_back([]() -> void {MX_MODE = MX_MODE_PLAY;});
            }
            nk_layout_row_push(ctx, MX_MODE_BUTTON_WIDTH);
            if(nk_button_label(ctx, "Edit"))
            {
                work_queue.push_back([]() -> void {MX_MODE = MX_MODE_EDIT;});
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
                    for(size_t k = 0; k < active_atmospheres.size(); k++)
                    {
                        auto& currentAtmosphere = active_atmospheres[k];
                        nk_layout_row_dynamic(ctx, MX_ATMOSPHERE_HEIGHT + 2 * MX_GROUP_MARGIN, 1);
                        if(nk_group_begin(ctx, (std::string("player_screen>left_group>") + currentAtmosphere.title).c_str(), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
                        {
                            nk_layout_space_begin(ctx, NK_STATIC, MX_ATMOSPHERE_HEIGHT, 6);
                            {
                                auto bounds = nk_layout_space_bounds(ctx);

                                // title
                                nk_layout_space_push(ctx, nk_rect(0,0, bounds.w, MX_ATMOSPHERE_TITLE_HEIGHT));
                                nk_label(ctx, currentAtmosphere.title.c_str(), NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);

                                // volume slider
                                const int volume_offset_x = 0;
                                const int volume_offset_y = MX_ATMOSPHERE_TITLE_HEIGHT;
                                const int volume_width = bounds.w - MX_ATMOSPHERE_BUTTON_CONSTELLATION_WIDTH;
                                const int volume_height = MX_ATMOSPHERE_BUTTON_CONSTELLATION_HEIGHT;
                                nk_layout_space_push(ctx, nk_rect(volume_offset_x, volume_offset_y, volume_width, volume_height));
                                float step = 1.0f / volume_width;
                                if(nk_slider_float(ctx, 0.0f, &currentAtmosphere.volume, 1.0f, step))
                                {
                                    printf("slider val: %f\n", currentAtmosphere.volume);
                                }

                                // // mute button
                                const int mute_offset_x = volume_width + MX_ATMOSPHERE_BUTTON_SPACER;
                                const int mute_offset_y = volume_offset_y + MX_ATMOSPHERE_BUTTON_SPACER + (MX_ATMOSPHERE_BUTTON_SIZE + MX_ATMOSPHERE_BUTTON_SPACER) / 2;
                                nk_layout_space_push(ctx, nk_rect(mute_offset_x, mute_offset_y, MX_ATMOSPHERE_BUTTON_SIZE, MX_ATMOSPHERE_BUTTON_SIZE));
                                if(nk_button_label(ctx, "V")) 
                                {
                                    //toggle mute
                                }

                                // play button
                                const int play_offset_x = mute_offset_x + MX_ATMOSPHERE_BUTTON_SIZE + MX_ATMOSPHERE_BUTTON_SPACER;
                                const int play_offset_y = mute_offset_y - (MX_ATMOSPHERE_BUTTON_SIZE + MX_ATMOSPHERE_BUTTON_SPACER) / 2;
                                nk_layout_space_push(ctx, nk_rect(play_offset_x, play_offset_y, MX_ATMOSPHERE_BUTTON_SIZE, MX_ATMOSPHERE_BUTTON_SIZE));
                                if(nk_button_label(ctx, "P")) 
                                {
                                    // set to solely
                                }

                                // play plus button
                                const int playplus_offset_x = play_offset_x;
                                const int playplus_offset_y = play_offset_y + MX_ATMOSPHERE_BUTTON_SIZE + MX_ATMOSPHERE_BUTTON_SPACER;
                                nk_layout_space_push(ctx, nk_rect(playplus_offset_x, playplus_offset_y, MX_ATMOSPHERE_BUTTON_SIZE, MX_ATMOSPHERE_BUTTON_SIZE));
                                if(nk_button_label(ctx, "+")) 
                                {
                                    // set to also playing
                                }

                                // close button
                                const int close_offset_x = play_offset_x + MX_ATMOSPHERE_BUTTON_SIZE + MX_ATMOSPHERE_BUTTON_SPACER;
                                const int close_offset_y = mute_offset_y;
                                nk_layout_space_push(ctx, nk_rect(close_offset_x, close_offset_y, MX_ATMOSPHERE_BUTTON_SIZE, MX_ATMOSPHERE_BUTTON_SIZE));
                                if(nk_button_label(ctx, "X")) 
                                {
                                    // remove this atmosphere from active list
                                    work_queue.push_back([=]() -> void
                                    {
                                        active_atmospheres.erase(active_atmospheres.begin() + k);
                                    });
                                }
                            }
                            nk_layout_space_end(ctx);
                        nk_group_end(ctx);      
                        }
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
                    if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx, NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SIG_ENTER | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD, atmosphere_search_buffer, MX_TEXT_FIELD_LENGTH, nk_filter_default))
                    {
                        // do search and populate 
                    }

                    nk_layout_row_begin(ctx, NK_DYNAMIC, height - MX_TEXT_SEARCH_HEIGHT, 1);
                    nk_layout_row_push(ctx, 1.0f);
                    {
                        nk_group_begin(ctx, "play_screen>right_group>search_results", 0);
                        {
                            for(const auto& currentAtmosphere : searched_atmospheres)
                            {
                                nk_layout_row_dynamic(ctx, 50, 1);
                                if(nk_widget_is_mouse_clicked(ctx, NK_BUTTON_DOUBLE))
                                {
                                    printf("clicked on : %s\n", currentAtmosphere.title.c_str());
                                    active_atmospheres.push_back(currentAtmosphere);
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
                    for(const auto& currentAtmosphere : available_atmospheres)
                    {
                        nk_layout_row_dynamic(ctx, 50, 1);
                        nk_label(ctx, currentAtmosphere.title.c_str(), NK_TEXT_CENTERED);
                    }
                }
                nk_group_end(ctx);
            }
            nk_layout_row_push(ctx, 1.0f/3.0f);
            {
                nk_group_begin(ctx, "edit_screen>middle_group", 0);
                {
                    float close_ratio = nk_layout_ratio_from_pixel(ctx, MX_CLOSE_BUTTON_SIZE);
                    float volume_button_ratio = nk_layout_ratio_from_pixel(ctx, MX_VOLUME_BUTTON_SIZE);
                    float volume_slider_ratio = 1.0f - volume_button_ratio;
                    float volume_slider_width = nk_layout_pixel_from_ratio(ctx, volume_slider_ratio);
                    for(auto& currentTrack : instantiated_tracks)
                    {
                        nk_layout_row_begin(ctx, NK_DYNAMIC, MX_TEXT_SEARCH_HEIGHT, 2);
                        nk_layout_row_push(ctx, 1.0f - close_ratio);
                        nk_label(ctx, currentTrack.track.title.c_str(), NK_TEXT_ALIGN_LEFT);
                        nk_layout_row_push(ctx, close_ratio);
                        if(nk_button_label(ctx, "X"))
                        {
                            printf("Close %s\n", currentTrack.track.title.c_str());
                        }
                        nk_layout_row_begin(ctx, NK_DYNAMIC, MX_VOLUME_HEIGHT, 2);
                        nk_layout_row_push(ctx, volume_slider_ratio);
                        float step = 1.0f / volume_slider_width;
                        if(nk_slider_float(ctx, 0.0f, &currentTrack.volume, 1.0f, step))
                        {
                            printf("slider val: %f\n", currentTrack.volume);
                        }
                        nk_layout_row_push(ctx, volume_button_ratio);
                        if(nk_button_label(ctx, "V"))
                        {
                            printf("mute!\n");
                        }

                        if(currentTrack.track.type == track_data::ONE_SHOT)
                        {
                            nk_layout_row_dynamic(ctx, MX_TEXT_SEARCH_HEIGHT, 1);
                            nk_label(ctx, "Play every", NK_TEXT_LEFT);

                            nk_layout_row_begin(ctx, NK_STATIC, MX_TEXT_SEARCH_HEIGHT, 3);
                            nk_layout_row_push(ctx, 20);
                            nk_spacing(ctx, 1);
                            nk_layout_row_push(ctx, 80);
                            if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, currentTrack.min_buffer, countof(currentTrack.min_buffer), nk_filter_float))
                            {
                                printf("min: %s\n", currentTrack.min_buffer);
                            }
                            nk_layout_row_push(ctx, 20);
                            nk_label(ctx, "to", NK_TEXT_LEFT);
                            
                            nk_layout_row_begin(ctx, NK_STATIC, MX_TEXT_SEARCH_HEIGHT, 3);
                            nk_layout_row_push(ctx, 20);
                            nk_spacing(ctx, 1);
                            nk_layout_row_push(ctx, 80);
                            if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, currentTrack.max_buffer, countof(currentTrack.max_buffer), nk_filter_float))
                            {
                                printf("max: %s\n", currentTrack.max_buffer);
                            }
                            nk_layout_row_push(ctx, 80);
                            nk_label(ctx, "seconds", NK_TEXT_LEFT);                        
            
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
                    if(NK_EDIT_COMMITED & nk_edit_string_zero_terminated(ctx, NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SIG_ENTER | NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD, track_search_buffer, MX_TEXT_FIELD_LENGTH, nk_filter_default))
                    {
                        // do search and populate 
                    }

                    nk_layout_row_begin(ctx, NK_DYNAMIC, height - MX_TEXT_SEARCH_HEIGHT, 1);
                    nk_layout_row_push(ctx, 1.0f);
                    {
                        nk_group_begin(ctx, "edit_screen>right_group>search_results", 0);
                        {
                            for(const auto& currentTrack : available_tracks)
                            {
                                nk_layout_row_dynamic(ctx, 50, 1);
                                if(nk_widget_is_mouse_clicked(ctx, NK_BUTTON_DOUBLE))
                                {
                                    printf("clicked on : %s\n", currentTrack.title.c_str());
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

    void draw(nk_context* ctx, int width, int height)
    {
        // draw shared top bar
        draw_top_bar(ctx, width);

        draw_play_screen(ctx, width, height - MX_TOPBAR_HEIGHT, MX_MODE != MX_MODE_PLAY);
        draw_edit_screen(ctx, width, height - MX_TOPBAR_HEIGHT, MX_MODE != MX_MODE_EDIT);
    }
}