#pragma once

#include "service.hpp"

namespace dm
{
    // for now we should only need 3 layers
    SMOL_MAKE_ENUM(draw_layer, uint_fast8_t,
        background,
        middleground,
        foreground
    );

    SMOL_MAKE_EXPLICIT_ENUM(halign, uint_fast8_t,
        left = NVG_ALIGN_LEFT,
        center = NVG_ALIGN_CENTER,
        right = NVG_ALIGN_RIGHT
    );

    SMOL_MAKE_EXPLICIT_ENUM(valign, uint_fast8_t,
        top = NVG_ALIGN_TOP,
        middle = NVG_ALIGN_MIDDLE,
        bottom = NVG_ALIGN_BOTTOM
    );

    SMOL_MAKE_ENUM(background_image_mode, uint_fast8_t,
        stretch,
        tile,
        center,
        contain
    );

    SMOL_MAKE_HANDLE_ENUM(font_handle);
    SMOL_MAKE_HANDLE_ENUM(image_handle,
        debug
    );

    constexpr NVGcolor nvg(const smol::color& c)
    {
        return {c.r, c.g, c.b, c.a};
    }

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
        void set_draw_layer(const draw_layer_t);
        void color_fill_rect(const rectf&, const color&);
        void color_fill_rounded_rect(const rectf&, const cornersf&, const color&);
        void color_fill_circle(const circlef&, const color&);
        void image_fill_rect(const rectf&, const image_handle_t, const background_image_mode_t);
        void image_fill_rounded_rect(const rectf&, const cornersf&, const image_handle_t, const background_image_mode_t);
        void image_fill_circle(const circlef&, const image_handle_t, const background_image_mode_t);
        void stroke_rect(const rectf&, float strokeWidth, const color&);
        void stroke_rounded_rect(const rectf&, const cornersf&, float strokeWidth, const color&);
        void stroke_circle(const circlef&, float strokeWidth, const color&);
        void stroke_path(const smol::array<vec2f>&, float strokeWidth, const color&);

        void draw_text(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const smol::utf8_string& str);
        void draw_text_with_caret(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const smol::utf8_string& str, int32_t caretIndex, float caretWidth, const color& caretColor);
        void draw_text_with_selection(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const smol::utf8_string& str, int32_t selectBegin, int32_t selectEnd, const color& selectedFontColor, const color& selectionColor);

        // gets the index into the underlying string buffer clickPoint would be closest to
        int32_t get_caret_position(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, halign_t halign, valign_t valign, const smol::utf8_string& str, const vec2f& clickPoint);

        // gets the offset from bounds beginning of caret
        float get_caret_offset(const rectf& bounds, font_handle_t font, float fontSize, halign_t halign, valign_t valign, const smol::utf8_string& str, size_t caretPosition);
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
        // the first nvg_image_t is the black and magenta debug texture
        std::vector<nvg_image_t> _nvg_images = {{}};

        bool early_exit();
        void swap_buffers();

        static vec2f text_bounds_to_start_position(const rectf& bounds, halign_t halign, valign_t valign);

        NVGpaint make_nvgpaint(image_handle_t ih, const rectf& dim, background_image_mode_t bim) const;

        class command_queue
        {
        public:

            bool operator==(const command_queue& that) const
            {
                for(size_t i = 0; i < size_t(draw_layer::count); i++)
                {
                    if (!(this->_commands[i] == that._commands[i]))
                        return false;
                }
                return true;
            }

            template<typename T>
            void push_back(T&& val) {
                SMOL_ASSERT(_currentLayer != draw_layer::invalid && _currentLayer < draw_layer::count);
                _commands[static_cast<size_t>(_currentLayer)].push_back(std::forward<T>(val));
            }

            template<typename T>
            bool next(T& out)
            {
                for(size_t idx = static_cast<size_t>(_currentLayer);
                    idx < static_cast<size_t>(draw_layer::count);
                    idx++)
                {
                    if (_commands[idx].next<T>(out)) {
                        return true;
                    }
                }
                return false;
            }

            void set_layer(draw_layer layer) {
                SMOL_ASSERT(_mode == mode::writing);
                _currentLayer = layer;
            }

            draw_layer_t get_current_layer() const {
                return _currentLayer;
            }

            void begin_writing() {
                _mode = mode::writing;
                _currentLayer = draw_layer::invalid;
                for(auto& buffer : _commands) {
                    buffer.clear();
                }
            }

            void begin_reading() {
                _mode = mode::reading;
                _currentLayer = draw_layer::background;
                for(auto& buffer : _commands) {
                    buffer.reset();
                }
            }
        private:
            enum class mode
            {
                reading,
                writing,
            } _mode = mode::reading;

            draw_layer_t _currentLayer = draw_layer::middleground;
            std::array<hash_buffer, static_cast<size_t>(draw_layer::count)> _commands;
        };

        command_queue _previous, _current;
    };
    MX_MAKE_SERVICE(nanovg, worker_flag::ui);
}

namespace nlohmann
{
    void from_json(const nl::json& j, dm::font_handle_t& fh);
    void from_json(const nl::json& j, dm::image_handle_t& ih);
    void from_json(const nl::json& j, dm::halign_t& ha);
    void from_json(const nl::json& j, dm::valign_t& va);
    void from_json(const nl::json& j, dm::background_image_mode_t& bim);
}
