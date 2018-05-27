 #pragma once

#define MX_GET_CSTRING(X) get_string_manager()->get_cstring(X)

namespace tabletopify
{
    typedef enum
    {
        MX_STR_INVALID = -1,
        MX_STR_PLAYLISTS,
        MX_STR_CREATE_PLAYLIST,
        MX_STR_ALL_PLAYLIST,
        MX_STR_EDIT_ATMOSPHERE,
        MX_STR_ABOUT,
        MX_STR_PLAYLIST,
        MX_STR_NEW_ATMOSPHERE,
        MX_STR_SEARCH,
        MX_STR_SETTINGS,
        MX_STR_PLAYING,
        MX_STR_STOPPED,
        MX_STR_TITLE,
        MX_STR_VOLUME,

        MX_STR_COUNT,
    } mixer_string_t;

    class string_manager
    {
    public:
        const char* get_cstring(mixer_string_t str) const;
    private:
        std::string _strings[MX_STR_COUNT];
        bool _strings_loaded = false;

        friend void init_string_manager(fs::path&& stringPath);
    };

    string_manager* get_string_manager();
    void init_string_manager(fs::path&& stringPath);
    void free_string_manager();
}