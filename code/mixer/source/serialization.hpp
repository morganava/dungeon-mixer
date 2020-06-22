#pragma once

#include "utilities.hpp"

namespace tabletopify
{
    nl::json parse_json(const memory_buffer&);

    fs::path get_skins_path();
    fs::path get_strings_path();


    void load_stylesheet();
    void load_audio_manifests();
    void load_playlists();
    void load_playlist(const fs::path&);


}