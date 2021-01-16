#pragma once

#include "enums.hpp"
#include "utilities.hpp"
#include "singleton.hpp"
#include "audio.hpp"

namespace dm
{
    nl::json parse_json(const memory_buffer&);

    void load_stylesheet();
    void load_audio_manifests();
    void load_playlists();
    void load_playlist(const fs::path&);

    MX_MAKE_ENUM(content_type, uint_fast8_t,
        stylesheet,
        strings,
        playlist
    );

    class serialization_manager
    {
    public:
        void update(float dt);

        void request_autosave(content_type_t) {}
        void set_autosave_path(content_type_t, const fs::path&) {}
        void set_load_path(content_type_t, const fs::path&) {}
    // private:
        void save_playlist(fs::path&& dest, std::vector<playlist_handle_t>&& playlists);
    };
    MX_MAKE_SINGLETON(serialization_manager);
}