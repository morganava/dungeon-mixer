#pragma once

namespace tabletopify
{
    void load_audio_manifests();
    void open_user_mix(const fs::path& path);
    void save_user_mix(const fs::path& path);
}