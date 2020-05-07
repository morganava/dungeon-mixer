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

    typedef enum class background_image_mode : uint_fast8_t
    {
        stretch,
        tile,
        center,
        contain,
    } background_image_mode_t;

    // strongly typed so we can have from_json
    MX_STRONG_HANDLE(font_handle);
    MX_STRONG_HANDLE(image_handle);

    class nanovg
    {
    public:
        void create_context();
        void update(float dt, double window_width, double window_height, double pixel_ratio);

        // content loading
        font_handle_t create_font(fs::path&& path);
        image_handle_t create_image(fs::path&& path);

        // render methods
        void set_scissor(const rectf&);
        void color_fill_rect(const rectf&, const color&);
        void color_fill_rounded_rect(const rectf&, const cornersf&, const color&);
        void color_fill_circle(const circlef&, const color&);
        void image_fill_rect(const rectf&, const image_handle_t, const background_image_mode_t);
        void image_fill_rounded_rect(const rectf&, const cornersf&, const image_handle_t, const background_image_mode_t);
        void image_fill_circle(const circlef&, const image_handle_t, const background_image_mode_t);
        void stroke_rect(const rectf&, float strokeWidth, const color&);
        void stroke_rounded_rect(const rectf&, const cornersf&, float strokeWidth, const color&);
        void stroke_circle(const circlef&, float strokeWidth, const color&);
        void draw_text(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const utf8_string& str);
        void draw_text_with_caret(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const utf8_string& str, int32_t caretIndex, float caretWidth, const color& caretColor);
        void draw_text_with_selection(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const utf8_string& str, int32_t selectBegin, int32_t selectEnd, const color& selectedFontColor, const color& selectionColor);

        // gets the index into the underlying string buffer clickPoint would be closest to
        int32_t get_caret_position(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, halign_t halign, valign_t valign, const utf8_string& str, const vec2f& clickPoint);

        // gets the offset from bounds beginning of caret
        float get_caret_offset(const rectf& bounds, font_handle_t font, float fontSize, halign_t halign, valign_t valign, const utf8_string& str, size_t caretPosition);
    private:
        NVGcontext* _ctx = nullptr;
        size_t _same_frames = 0;

        // font info
        std::unordered_map<fs::path, font_handle_t> _font_map;
        typedef int nvg_font_face_id_t;
        constexpr static const nvg_font_face_id_t INVALID_NVG_FONT_ID = ~nvg_font_face_id_t(0);
        std::vector<nvg_font_face_id_t> _nvg_font_faces;

        // texture info
        std::unordered_map<fs::path, image_handle_t> _image_map;

        typedef int nvg_image_id_t;
        constexpr static const nvg_image_id_t INVALID_NVG_IMAGE_ID = ~nvg_image_id_t(0);
        typedef struct nvg_image
        {
            nvg_image_id_t id = INVALID_NVG_IMAGE_ID;
            vec2f size;

            constexpr float width() const { return size.x; };
            constexpr float height() const { return size.y; };
        } nvg_image_t;
        constexpr static const image_handle_t MX_DEBUG_IMAGE_HANDLE = 0;
        // the first nvg_image_t is the black and magenta debug texture
        std::vector<nvg_image_t> _nvg_images = {{}};

        bool early_exit();
        void swap_buffers();

        static vec2f text_bounds_to_start_position(const rectf& bounds, halign_t halign, valign_t valign);

        NVGpaint make_nvgpaint(image_handle_t ih, const rectf& dim, background_image_mode_t bim) const;

        struct
        {
            hash_buffer previous, current;
        } _command_queue;
    };
    MX_MAKE_SINGLETON(nanovg);
}