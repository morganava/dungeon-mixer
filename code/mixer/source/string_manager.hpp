 #pragma once

#define MX_GET_CSTRING(X) get_string_manager()->get_cstring(X)

#include "enums.hpp"
#include "utf8_string.hpp"
#include "singleton.hpp"

namespace dm
{
    MX_MAKE_ENUM(mixer_string, uint_fast16_t,
        /* Common */
        playlists,
        create_new_playlist,
        new_playlist_name,
        new_atmosphere_name,
        /* Playlist Names */
        all_playlist,
        favorites_playlist,
        playing_playlist,
        /* Screen Titles */
        playlist_screen,
        edit_screen,
        about_screen,
        /* Playlist Screen */
        export_playlist,
        import_playlist,
        create_new_mix,
        atmosphere_search,
        edit_atmosphere,
        remove_atmosphere,
        copy_atmosphere,
        move_atmosphere,
        delete_atmosphere,
        /* Edit Screen */
        track_search,
        volume,
        enabled,
        disabled,
        position,
        azimuth,
        elevation,
        repeat,
        delay
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
    constexpr inline auto& g_strings = g_string_manager;
}