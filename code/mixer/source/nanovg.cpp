#include "geometry.hpp"
#include "hash.hpp"
#include "utilities.hpp"
#include "color.hpp"
#include "nanovg.hpp"
#include "singleton.hpp"
#include "work_queue.hpp"
#include "utf8_string.hpp"

namespace tabletopify
{
    extern GLFWwindow* g_window;

    // NanoVG

    void nanovg::create_context()
    {
        // init nanovg
        _ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
        MX_THROW_IF_NULL(_ctx);
    }

    font_id nanovg::create_font(fs::path&& path)
    {
        auto fontFilePath = fs::canonical(path);
        auto it = _font_map.find(fontFilePath);
        if(it == _font_map.end())
        {
            // index our font face id will be stored
            auto index = _nvg_font_faces.size();
            _font_map.insert({fontFilePath, index});
            _nvg_font_faces.push_back(INVALID_NVG_FONT_ID);

            worker(Worker_FileIO).enqueue([index=index, fontFilePath=std::move(fontFilePath)]() -> void
            {
                memory_buffer fontFile(fontFilePath);
                std::string fontName = fontFilePath.filename();

                worker(Worker_UI).enqueue([index=index, fontName=std::move(fontName), fontFile=std::move(fontFile)]() -> void
                {
                    g_nanovg._nvg_font_faces[index] = nvgCreateFontMem(g_nanovg._ctx, fontName.c_str(), fontFile.data(), static_cast<int>(fontFile.size()));
                    MX_LOG_INFO("Loaded font: {}", fontName);
                });
            });

            return index;
        }
        return it->second;
    }

    bool nanovg::early_exit()
    {
        if (_command_queue.current == _command_queue.previous)
        {
            ++_same_frames;
        }
        else
        {
            _same_frames = 0;
        }

        // double buffering means we need at least 2 same frames
        // before we can start ignoring draw commands
        return (_same_frames >= 2);
    }

    void nanovg::swap_buffers()
    {
        glfwSwapBuffers(g_window);
        std::swap(_command_queue.current, _command_queue.previous);
        _command_queue.current.clear();
    }

    typedef enum draw_command : uint8_t
    {
        set_scissor,
        fill_rect,
        fill_rounded_rect,
        stroke_rect,
        stroke_rounded_rect,
        draw_text,
        draw_text_with_caret,
    } draw_command_t;

    void nanovg::update(float dt, double window_width, double window_height, double pixel_ratio)
    {
        if(this->early_exit())
        {
            this->swap_buffers();
            return;
        }

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

        nvgBeginFrame(_ctx, window_width, window_height, pixel_ratio);

        draw_command_t cmd;
        auto& cq = _command_queue.current;

        while(cq.next<draw_command_t>(cmd))
        {
            switch(cmd)
            {
                case draw_command::set_scissor:
                {
                    rectf r;
                    MX_THROW_IF_FALSE(cq.next(r));

                    nvgScissor(_ctx, r.position.x, r.position.y, r.size.x, r.size.y);
                }
                break;
                case draw_command::fill_rect:
                {
                    rectf r;
                    color col;
                    MX_THROW_IF_FALSE(cq.next(r));
                    MX_THROW_IF_FALSE(cq.next(col))

                    nvgBeginPath(_ctx);
                    nvgRect(_ctx, r.position.x, r.position.y, r.size.x, r.size.y);
                    nvgFillColor(_ctx, col);
                    nvgFill(_ctx);
                }
                break;
                case draw_command::fill_rounded_rect:
                {
                    rectf r;
                    cornersf rad;
                    color col;
                    MX_THROW_IF_FALSE(cq.next(r));
                    MX_THROW_IF_FALSE(cq.next(rad));
                    MX_THROW_IF_FALSE(cq.next(col));

                    nvgBeginPath(_ctx);
                    nvgRoundedRectVarying(_ctx, r.position.x, r.position.y, r.size.x, r.size.y, rad.topLeft, rad.topRight, rad.bottomRight, rad.bottomLeft);
                    nvgFillColor(_ctx, col);
                    nvgFill(_ctx);
                }
                break;
                case draw_command::stroke_rect:
                {
                    rectf r;
                    float strokeWidth;
                    color col;
                    MX_THROW_IF_FALSE(cq.next(r));
                    MX_THROW_IF_FALSE(cq.next(strokeWidth));
                    MX_THROW_IF_FALSE(cq.next(col));

                    r.position += vec2f{strokeWidth / 2.0f, strokeWidth / 2.0f};
                    r.size -= vec2f{strokeWidth, strokeWidth};

                    nvgBeginPath(_ctx);
                    nvgRect(_ctx, r.position.x, r.position.y, r.size.x, r.size.y);
                    nvgStrokeColor(_ctx, col);
                    nvgStrokeWidth(_ctx, strokeWidth);
                    nvgStroke(_ctx);
                }
                break;
                case draw_command::stroke_rounded_rect:
                {
                    rectf r;
                    cornersf rad;
                    float strokeWidth;
                    color col;
                    MX_THROW_IF_FALSE(cq.next(r));
                    MX_THROW_IF_FALSE(cq.next(rad));
                    MX_THROW_IF_FALSE(cq.next(strokeWidth));
                    MX_THROW_IF_FALSE(cq.next(col));
                    float halfStrokeWidth = strokeWidth / 2.0f;

                    r.position += vec2f{halfStrokeWidth};
                    r.size -= vec2f{strokeWidth, strokeWidth};
                    rad.topLeft = max(0.0f, rad.topLeft - halfStrokeWidth);
                    rad.topRight = max(0.0f, rad.topRight - halfStrokeWidth);
                    rad.bottomRight = max(0.0f, rad.bottomRight - halfStrokeWidth);
                    rad.bottomLeft = max(0.0f, rad.bottomLeft - halfStrokeWidth);

                    nvgBeginPath(_ctx);
                    nvgRoundedRectVarying(_ctx, r.position.x, r.position.y, r.size.x, r.size.y, rad.topLeft, rad.topRight, rad.bottomRight, rad.bottomLeft);
                    nvgStrokeColor(_ctx, col);
                    nvgStrokeWidth(_ctx, strokeWidth);
                    nvgStroke(_ctx);
                }
                break;
                case draw_command::draw_text:
                {
                    rectf bounds;
                    font_id font;
                    float size;
                    color col;
                    halign_t halign;
                    valign_t valign;
                    utf8_string_view str;
                    MX_THROW_IF_FALSE(cq.next(bounds));
                    MX_THROW_IF_FALSE(cq.next(font));
                    MX_THROW_IF_FALSE(cq.next(size));
                    MX_THROW_IF_FALSE(cq.next(col));
                    MX_THROW_IF_FALSE(cq.next(halign));
                    MX_THROW_IF_FALSE(cq.next(valign));
                    MX_THROW_IF_FALSE(cq.next(str));

                    auto pos = text_bounds_to_start_position(bounds, halign, valign);

                    nvgFontFaceId(_ctx, _nvg_font_faces[font]);
                    nvgFontSize(_ctx, size);
                    nvgTextAlign(_ctx, static_cast<int>(halign) | static_cast<int>(valign));
                    nvgFillColor(_ctx, col);
                    nvgText(_ctx, pos.x, pos.y, str.begin(), str.end());
                }
                break;
                case draw_command::draw_text_with_caret:
                {
                    rectf bounds;
                    font_id font;
                    float fontSize;
                    color fontColor;
                    halign_t halign;
                    valign_t valign;
                    utf8_string_view str;
                    int32_t caretIndex;
                    float caretWidth;
                    color caretColor;

                    MX_THROW_IF_FALSE(cq.next(bounds));
                    MX_THROW_IF_FALSE(cq.next(font));
                    MX_THROW_IF_FALSE(cq.next(fontSize));
                    MX_THROW_IF_FALSE(cq.next(fontColor));
                    MX_THROW_IF_FALSE(cq.next(halign));
                    MX_THROW_IF_FALSE(cq.next(valign));
                    MX_THROW_IF_FALSE(cq.next(str));
                    MX_THROW_IF_FALSE(cq.next(caretIndex));
                    MX_THROW_IF_FALSE(cq.next(caretWidth));
                    MX_THROW_IF_FALSE(cq.next(caretColor));

                    MX_LOG_INFO("caretIndex : {}", caretIndex);
                    auto startPosition = text_bounds_to_start_position(bounds, halign, valign);

                    nvgFontFaceId(_ctx, _nvg_font_faces[font]);
                    nvgFontSize(_ctx, fontSize);
                    nvgTextAlign(_ctx, static_cast<int>(halign) | static_cast<int>(valign));
                    nvgFillColor(_ctx, fontColor);
                    nvgText(_ctx, startPosition.x, startPosition.y, str.begin(), str.end());

                    const auto caretCount = caretIndex + 1;
                    NVGglyphPosition glyphPositions[caretCount];
                    const auto glyphCount = nvgTextGlyphPositions(_ctx, startPosition.x, startPosition.y, str.begin(), str.end(), glyphPositions, caretCount);

                    float lineHeight;
                    nvgTextMetrics(_ctx, nullptr, nullptr, &lineHeight);

                    rectf caretBounds;

                    // check of at the end of the string
                    if (caretIndex == glyphCount) {
                        caretBounds.position.x = glyphPositions[caretIndex - 1].maxx - caretWidth;
                    }
                    // in between glyphs
                    else {
                        caretBounds.position.x = glyphPositions[caretIndex].minx;
                    }
                    caretBounds.position.x = std::round(caretBounds.position.x);

                    switch(valign)
                    {
                        case valign::top:
                            caretBounds.position.y = bounds.top();
                            break;
                        case valign::middle:
                            caretBounds.position.y = (bounds.top() + bounds.bottom()) / 2.0f - lineHeight / 2.0f;
                            break;
                        case valign::bottom:
                            caretBounds.position.y = bounds.bottom() - lineHeight;
                            break;
                    }
                    caretBounds.size = {caretWidth, lineHeight};

                    nvgBeginPath(_ctx);
                    nvgRect(_ctx, caretBounds.position.x, caretBounds.position.y, caretBounds.size.x, caretBounds.size.y);
                    nvgFillColor(_ctx, caretColor);
                    nvgFill(_ctx);
                }
                break;
            }
        }

        nvgEndFrame(_ctx);

        this->swap_buffers();
    }

    void nanovg::set_scissor(const rectf& r)
    {
        auto& cq = _command_queue.current;
        cq.push_back(draw_command::set_scissor);
        cq.push_back(r);
    }

    void nanovg::fill_rect(const rectf& r, const color& col)
    {
        auto& cq = _command_queue.current;
        cq.push_back(draw_command::fill_rect);
        cq.push_back(r);
        cq.push_back(col);
    }

    void nanovg::fill_rounded_rect(const rectf& r, const cornersf& c, const color& col)
    {
        auto& cq = _command_queue.current;
        cq.push_back(draw_command::fill_rounded_rect);
        cq.push_back(r);
        cq.push_back(c);
        cq.push_back(col);
    }

    void nanovg::stroke_rect(const rectf& r, float strokeWidth, const color& col)
    {
        auto& cq = _command_queue.current;
        cq.push_back(draw_command::stroke_rect);
        cq.push_back(r);
        cq.push_back(strokeWidth);
        cq.push_back(col);
    }

    void nanovg::stroke_rounded_rect(const rectf& r, const cornersf& c, float strokeWidth, const color& col)
    {
        auto& cq = _command_queue.current;
        cq.push_back(draw_command::stroke_rounded_rect);
        cq.push_back(r);
        cq.push_back(c);
        cq.push_back(strokeWidth);
        cq.push_back(col);
    }

    void nanovg::draw_text(const rectf& bounds, font_id font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const utf8_string& str)
    {
        if (font == font_id::invalid) {
            return;
        }

        auto& cq = _command_queue.current;
        cq.push_back(draw_command::draw_text);
        cq.push_back(bounds);
        cq.push_back(font);
        cq.push_back(fontSize);
        cq.push_back(fontColor);
        cq.push_back(halign);
        cq.push_back(valign);
        cq.push_back(str);
    }

    void nanovg::draw_text_with_caret(const rectf& bounds, font_id font, float fontSize, const color& fontColor,halign_t halign, valign_t valign, const utf8_string& str, int32_t caretIndex, float caretWidth, const color& caretColor)
    {
        if (font == font_id::invalid) {
            return;
        }

        auto& cq = _command_queue.current;
        if (caretIndex < 0)
        {
            draw_text(bounds, font, fontSize, fontColor, halign, valign, str);
        }
        else
        {
            cq.push_back(draw_command::draw_text_with_caret);
            cq.push_back(bounds);
            cq.push_back(font);
            cq.push_back(fontSize);
            cq.push_back(fontColor);
            cq.push_back(halign);
            cq.push_back(valign);
            cq.push_back(str);
            cq.push_back(caretIndex);
            cq.push_back(caretWidth);
            cq.push_back(caretColor);
        }
    }

    int32_t nanovg::get_caret_position(const rectf& bounds, font_id font, float size, halign_t halign, valign_t valign, const utf8_string& str, const vec2f& clickPoint)
    {
        // TODO: take y into account or is the assumption of this function that a click has happened?
        if (font == font_id::invalid) {
            return -1;
        }

        auto startPosition = text_bounds_to_start_position(bounds, halign, valign);
        nvgFontFaceId(_ctx, _nvg_font_faces[font]);
        nvgFontSize(_ctx, size);
        nvgTextAlign(_ctx, static_cast<int>(halign) | static_cast<int>(valign));

        NVGglyphPosition glyphPositions[str.size()];
        int positions = nvgTextGlyphPositions(_ctx, startPosition.x, startPosition.y, str.begin(), str.end(), glyphPositions, str.size());

        const float x = clickPoint.x;
        int32_t index = 0;
        if (x > glyphPositions[0].minx)
        {
            for(int i = 0; i < positions; ++i)
            {
                const auto& gp = glyphPositions[i];
                if (x < gp.maxx)
                {
                    break;
                }
                else
                {
                    index++;
                }
            }
        }

        return index;
    }

    vec2f nanovg::text_bounds_to_start_position(const rectf& bounds, halign_t halign, valign_t valign)
    {
        const vec2f& pos = bounds.position;
        const vec2f& size = bounds.size;
        float x = 0.0f;
        float y = 0.0f;

        switch(halign)
        {
            case halign::left:
                x = pos.x;
                break;
            case halign::center:
                x = (pos.x + pos.x + size.x) / 2.0f;
                break;
            case halign::right:
                x = pos.x + size.x;
                break;
        }

        switch(valign)
        {
            case valign::top:
                y = pos.y;
                break;
            case valign::middle:
                y = (pos.y + pos.y + size.y) / 2.0f;
                break;
            case valign::bottom:
                y = pos.y + size.y;
                break;
        }

        return vec2f{x, y};
    }
}