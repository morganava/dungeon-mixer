 #pragma once

#define MX_GET_CSTRING(X) get_string_manager()->get_cstring(X)

#include "utf8_string.hpp"

namespace tabletopify
{
    MX_MAKE_ENUM(mixer_string, uint_fast16_t,
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
        MX_STR_VOLUME
    )

    class string_manager
    {
    public:
        string_manager(fs::path&& stringPath);
        const char* get_cstring(mixer_string_t str) const;
    private:
        std::string _strings[static_cast<size_t>(mixer_string::count)];
        bool _strings_loaded = false;
    };
    MX_MAKE_SINGLETON(string_manager);
}