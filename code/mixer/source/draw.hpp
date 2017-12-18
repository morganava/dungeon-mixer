#pragma once

namespace tabletopify
{
    struct UI
    {
        struct
        {

        } playback;
        
        struct
        {
            struct
            {
                std::vector<std::pair<std::string,atmosphere_id_t>> atmospheres;
                size_t selectedIndex = -1;
            } atmosphereList;

            struct
            {
                struct track_edit
                {
                    audio_id_t audio_id;
                    float volume;
                    union
                    {
                        struct
                        {
                            char minRepeatBuffer[8] = "";
                            char maxRepeatBuffer[8] = "";
                            float minRepeat;
                            float maxRepeat;
                        } oneShot;
                        struct
                        {

                        } loop;
                    };
                    //std::unique_ptr<track_instance> track;
                };

                std::vector<track_edit> tracks;
            } atmosphereEdit;

            struct
            {
                char searchBuffer[64] = "";
                std::vector<std::pair<std::string,audio_id_t>> results;
            } trackSearch;
        } edit;  
    };
    extern UI g_ui;

    // Tag Search Pane
    extern std::vector<size_t> searched_track_ids;
    struct track_display
    {
        std::string title;
        std::vector<std::string> tags;
        size_t id;
    };
    extern std::vector<track_display> searched_track_display;
    extern bool search_track_ids_dirty;

    void update_ui_edit_atmosphereList(const std::vector<atmosphere_id_t>& atmosphere_ids);
    void update_ui_edit_trackSearch(const std::vector<audio_id_t>& audio_ids);

	void draw(nk_context* ctx, int width, int height);
}