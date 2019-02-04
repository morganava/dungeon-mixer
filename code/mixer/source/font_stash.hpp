#pragma once

#include "hash.hpp"
#include "utf8_string.hpp"
#include "singleton.hpp"
#include "nanovg.hpp"

namespace tabletopify
{
    typedef size_t font_index_t;

    class font_stash
    {
    public:
        font_index_t load_font(fs::path&&);
        nvg_font_face_id_t get_font_face(font_index_t idx) const;
    private:
        std::unordered_map<fs::path, font_index_t, path_hash> _font_map;
        // a given font_index_t is an index into this buffer of nanovg font face ids
        void set_font_face(font_index_t idx, nvg_font_face_id_t id);

        std::vector<nvg_font_face_id_t> _nvg_font_faces;
        friend class nanovg;
    };

    inline void init_font_stash()
    {
        singleton<font_stash>::construct();
    }

    constexpr void free_font_stash()
    {
        singleton<font_stash>::free();
    }

    inline constexpr font_stash& g_font_stash = singleton<font_stash>::get();
}