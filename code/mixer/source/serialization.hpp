#pragma once

#include "enums.hpp"
#include "utilities.hpp"
#include "singleton.hpp"
#include "audio.hpp"

namespace dm
{
    MX_MAKE_ENUM(content_type, uint_fast8_t,
        stylesheet,
        strings,
        playlist
    );

    class serialization_manager
    {
    public:
        void update(float dt);

        void request_autosave(content_type_t);
        void set_autosave_path(content_type_t, const fs::path&) {}
        void set_load_path(content_type_t, const fs::path&) {}

        void load_stylesheet(fs::path&& source);
        void load_audio_manifests();
        void save_playlist(fs::path&& dest, std::vector<playlist_handle_t>&& playlists);
        void load_playlist(fs::path&& source);
    private:
        constexpr static auto AUTOSAVE_DELAY = 2s;
        struct
        {
            bool autosaveRequested = false;
            decltype(std::chrono::system_clock::now()) timeRequested;
        } autosave[static_cast<size_t>(content_type::count)];
    };
    MX_MAKE_SINGLETON(serialization_manager);
}