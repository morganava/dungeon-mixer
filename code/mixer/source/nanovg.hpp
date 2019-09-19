#pragma once

#include "utilities.hpp"
#include "geometry.hpp"
#include "singleton.hpp"
#include "hash.hpp"
#include "color.hpp"
#include "hash_buffer.hpp"

namespace tabletopify
{
    class utf8_string;

    typedef enum class halign : uint_fast8_t
    {
        left = NVG_ALIGN_LEFT,
        center = NVG_ALIGN_CENTER,
        right = NVG_ALIGN_RIGHT,
    } halign_t;

    typedef enum class valign : uint_fast8_t
    {
        top = NVG_ALIGN_TOP,
        middle = NVG_ALIGN_MIDDLE,
        bottom = NVG_ALIGN_BOTTOM,
    } valign_t;


    // strongly typed so we can have from_json
    struct font_id
    {
    public:
        constexpr font_id() = default;
        constexpr font_id(size_t handle)
        : _handle(handle)
        { }
        constexpr font_id(const font_id&) = default;
        constexpr font_id(font_id&&) = default;
        font_id& operator=(const font_id&) = default;
        font_id& operator=(font_id&&) = default;
        constexpr operator size_t() const
        {
            return _handle;
        }

        const static font_id invalid;

    private:

        size_t _handle = ~size_t(0);
    };
    inline const font_id font_id::invalid = {};

    class nanovg
    {
    public:
        void create_context();
        void update(float dt, double window_width, double window_height, double pixel_ratio);

        // loads font
        font_id create_font(fs::path&& path);

        // render methods
        void set_scissor(const rectf&);
        void fill_rect(const rectf&, const color&);
        void fill_rounded_rect(const rectf&, const cornersf&, const color&);
        void fill_circle(const circlef&, const color&);
        void stroke_rect(const rectf&, float strokeWidth, const color&);
        void stroke_rounded_rect(const rectf&, const cornersf&, float strokeWidth, const color&);
        void stroke_circle(const circlef&, float strokeWidth, const color&);
        void draw_text(const rectf& bounds, float textOffset, font_id font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const utf8_string& str);
        void draw_text_with_caret(const rectf& bounds, float textOffset, font_id font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const utf8_string& str, int32_t caretIndex, float caretWidth, const color& caretColor);
        void draw_text_with_selection(const rectf& bounds, float textOffset, font_id font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const utf8_string& str, int32_t selectBegin, int32_t selectEnd, const color& selectedFontColor, const color& selectionColor);

        // gets the index into the underlying string buffer clickPoint would be closest to
        int32_t get_caret_position(const rectf& bounds, float textOffset, font_id font, float fontSize, halign_t halign, valign_t valign, const utf8_string& str, const vec2f& clickPoint);

        // gets the offset from bounds beginning of caret
        float get_caret_offset(const rectf& bounds, font_id font, float fontSize, halign_t halign, valign_t valign, const utf8_string& str, size_t caretPosition);
    private:
        NVGcontext* _ctx = nullptr;
        size_t _same_frames = 0;

        // font info
        std::unordered_map<fs::path, font_id> _font_map;
        typedef int nvg_font_face_id_t;
        constexpr static nvg_font_face_id_t INVALID_NVG_FONT_ID = ~nvg_font_face_id_t(0);
        std::vector<nvg_font_face_id_t> _nvg_font_faces;

        bool early_exit();
        void swap_buffers();

        static vec2f text_bounds_to_start_position(const rectf& bounds, halign_t halign, valign_t valign);

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