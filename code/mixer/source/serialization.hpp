#pragma once

namespace tabletopify
{
    fs::path get_skins_path();
    fs::path get_strings_path();

    void load_audio_manifests();
    void open_user_mix(const fs::path& path);
    void save_user_mix(const fs::path& path);
}