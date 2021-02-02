 #pragma once

#define MX_GET_CSTRING(X) get_string_manager()->get_cstring(X)

#include "utf8_string.hpp"

namespace dm
{
    MX_MAKE_ENUM(mixer_string, uint_fast16_t,
        sidebar_playlists,
        sidebar_new_playlist,
        sidebar_all,
        sidebar_starred,
        sidebar_playing,
        sidebar_edit,
        sidebar_about,
        playlist_export,
        playlist_import,
        playlist_new_mix,
        playlist_atmosphere_box,
        edit_new_mix,
        edit_track_search,
        edit_volume,
        edit_enabled,
        edit_disabled,
        edit_position,
        edit_azimuth,
        edit_elevation,
        edit_repeat,
        edit_delay
    )

    class string_manager
    {
    public:
        string_manager() = default;
        string_manager(fs::path&& stringPath);

        utf8_string const& operator[](mixer_string_t) const;
    private:
        constexpr static size_t mixer_string_count = static_cast<size_t>(mixer_string::count);
        std::array<utf8_string, mixer_string_count> strings;

        friend class serialization_manager;
    };
    MX_MAKE_SINGLETON(string_manager);
}