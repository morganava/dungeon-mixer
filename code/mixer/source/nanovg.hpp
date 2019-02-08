#pragma once

#include "utilities.hpp"
#include "geometry.hpp"
#include "singleton.hpp"
#include "hash.hpp"

namespace tabletopify
{
    class utf8_string;

    struct hash_buffer
    {
        template<typename TYPE>
        void push_back(const TYPE& val)
        {
            push_back(&val, sizeof(val));
        }

        template<typename TYPE>
        bool next(TYPE& outval)
        {
            if (_read_head + sizeof(TYPE) <= _raw.data() + _raw.size())
            {
                std::memcpy(&outval, _read_head, sizeof(TYPE));
                _read_head += sizeof(TYPE);
                return true;
            }
            return false;
        }

        void clear()
        {
            _raw.clear();
            _hash = 0;
            _read_head = _raw.data();            
        }

        bool operator==(const hash_buffer& that) const
        {
            if (_hash != that._hash) return false;
            if (_raw.size() != that._raw.size()) return false;

            return std::memcmp(_raw.data(), that._raw.data(), _raw.size()) == 0;
        }

    private:
        void push_back(const void* pVal, size_t valSize);

        std::vector<uint8_t> _raw;
        size_t _hash = 0;

        uint8_t* _read_head = _raw.data();
    };

    typedef enum class font_align : uint8_t
    {
        // horizontal
        left = NVG_ALIGN_LEFT,
        center = NVG_ALIGN_CENTER,
        right = NVG_ALIGN_RIGHT,
        // vertical
        top = NVG_ALIGN_TOP,
        middle = NVG_ALIGN_MIDDLE,
        bottom = NVG_ALIGN_BOTTOM,
        baseline = NVG_ALIGN_BASELINE,
    } font_align_t;

    font_align_t operator|(const font_align_t&, const font_align_t&);

    typedef size_t font_id_t;

    class nanovg
    {
    public:
        void create_context();
        void update(float dt, double window_width, double window_height, double pixel_ratio);

        // loads font
        font_id_t create_font(fs::path&& path);

        // render methods
        void set_scissor(const rect&);
        void fill_rect(const rect&, const NVGcolor&);
        void draw_text(const rect& bounds, font_id_t font, float size, font_align_t align, const utf8_string& str);
    private:
        NVGcontext* _ctx = nullptr;
        size_t _same_frames = 0;

        // font info
        std::unordered_map<fs::path, font_id_t, path_hash> _font_map;
        typedef int nvg_font_face_id_t;
        constexpr static nvg_font_face_id_t INVALID_NVG_FONT_ID = ~nvg_font_face_id_t(0);
        std::vector<nvg_font_face_id_t> _nvg_font_faces;

        bool early_exit();
        void swap_buffers();

        struct
        {
            hash_buffer previous, current;
        } _command_queue;
    };

    constexpr nanovg* get_nanovg() { return singleton<nanovg>::addr(); }
    inline void init_nanovg()
    {
        singleton<nanovg>::construct();
    }
    constexpr void free_nanovg()
    {
        singleton<nanovg>::free();
    }
    inline constexpr nanovg& g_nanovg = singleton<nanovg>::get();

}