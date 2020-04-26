#pragma once

namespace tabletopify
{
    fs::path get_skins_path();
    fs::path get_strings_path();

    void load_stylesheet();
    void load_audio_manifests();
    void load_playlists();
    void load_playlist(const fs::path&);

    // void open_user_mix(const fs::path& path);
    // void save_user_mix(const fs::path& path);
}