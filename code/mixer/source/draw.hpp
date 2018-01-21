#pragma once

namespace tabletopify
{
    struct UI
    {
        struct
        {
            struct 
            {
                struct atmosphere_playback
                {
                    atmosphere_id_t atmosphereId;
                    std::string title;
                    float volume;
                };

                std::vector<atmosphere_playback> atmospheres;
            } atmosphereList;

            struct
            {
                struct atmosphere_search_result
                {
                    atmosphere_id_t atmosphereId;
                    std::string title;
                };

                std::string prevSearch;
                char searchBuffer[64] = "";
                std::vector<atmosphere_search_result> results;
            } atmosphereSearch;
        } playback;
        
        struct
        {
            struct
            {
                struct atmosphere_edit
                {
                    std::string title;
                    atmosphere_id_t atmosphereId;
                };

                std::vector<atmosphere_edit> atmospheres;
                size_t selectedIndex = -1;

                atmosphere_id_t selectedAtmosphereId() const
                {
                    return atmospheres[selectedIndex].atmosphereId;
                }
            } atmosphereList;

            struct
            {
                struct track_edit
                {
                    track_id_t trackId;
                    std::string title;
                    audio_type_t audioType;
                    union
                    {
                        struct
                        {
                            struct
                            {
                                float value;
                                int muted;
                            } volume;
                            struct
                            {
                                float min;
                                float max;
                                char minBuffer[16];
                                char maxBuffer[16];

                            } repeat;
                            struct
                            {
                                float x;
                                float y;
                                float z;
                                char xBuffer[6];
                                char yBuffer[6];
                                char zBuffer[6];
                            } position;
                        } oneShot;
                        struct
                        {

                        } loop;
                    };
                };

                std::vector<track_edit> tracks;
            } atmosphereEdit;

            struct
            {
                struct track_result
                {
                    std::string title;
                    audio_id_t audioId;
                };

                char searchBuffer[64] = "";
                std::vector<track_result> results;
            } trackSearch;
        } edit;  
    };
    extern UI* g_ui;

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

    void update_ui_playback_atmosphereSearch(const std::vector<atmosphere_id_t>& atmosphere_ids);
    void update_ui_edit_atmosphereList(const std::vector<atmosphere_id_t>& atmosphere_ids);
    void update_ui_edit_atmosphereEdit(atmosphere_id_t atmosphere_id);
    void update_ui_edit_trackSearch(const std::vector<audio_id_t>& audio_ids);

    void draw_init(nk_context* ctx);
	void draw_update(nk_context* ctx, int width, int height);
}