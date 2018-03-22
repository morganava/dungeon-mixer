#pragma once

namespace tabletopify
{
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

    constexpr float MX_GROUP_BORDER = 0.0f;
    // interior spacing of group
    constexpr float MX_GROUP_PADDING_X = 0.0f;
    constexpr float MX_GROUP_PADDING_Y = 0.0f;
    // distance between adjacent widgets
    constexpr float MX_GROUP_SPACING = 0.0f;

    constexpr int MX_TOPBAR_HEIGHT = 100;
    // constexpr int MX_GROUP_MARGIN = 0;
    // constexpr int MX_TEXT_PADDING = 0;
    // constexpr int MX_BUTTON_PADDING = 0;
    // constexpr int MX_WIDGET_MARGIN = 2;

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

    // atmosphere create/delete
    constexpr int MX_ATMOSPHERE_BUTTON = 24;

    constexpr size_t INVALID_INDEX = ~size_t(0);

    /*
    Mode
    */

    typedef enum
    {
        MX_MODE_PLAY,
        MX_MODE_EDIT,
    } mode_t;
    extern mode_t MX_MODE;

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
                    bool muted;
                    bool playing;
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
                    bool renaming;
                };

                std::vector<atmosphere_edit> atmospheres;
                size_t selectedIndex = INVALID_INDEX;
                char renameBuffer[64] = "";

                atmosphere_id_t selectedAtmosphereId() const
                {
                    if(selectedIndex != INVALID_INDEX)
                    {
                        return atmospheres[selectedIndex].atmosphereId;
                    }
                    else
                    {
                        return INVALID_ATMOSPHERE_ID;
                    }
                }
            } atmosphereList;

            struct
            {
                struct track_edit
                {
                    track_id_t trackId;
                    std::string title;
                    audio_type_t audioType;
                    float volume;
                    int muted;
                    struct
                    {
                        float min;
                        float max;
                        char minBuffer[16];
                        char maxBuffer[16];

                    } repeat;
                    struct
                    {
                        int enabled;
                        float x;
                        float y;
                        float z;
                        char xBuffer[6];
                        char yBuffer[6];
                        char zBuffer[6];
                    } position;
                };

                std::vector<track_edit> tracks;
            } atmosphereEdit;

            struct
            {
                struct audio_result
                {
                    std::string title;
                    audio_id_t audioId;
                };

                std::string previousQuery;
                char searchBuffer[64] = "";
                std::vector<audio_result> results;
            } audioSearch;
        } edit;
    };
    extern UI* g_ui;

    void draw_init(nk_context* ctx);
	void draw_update(nk_context* ctx, int width, int height);
}