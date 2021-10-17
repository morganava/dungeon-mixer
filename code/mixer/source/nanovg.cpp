#include "nanovg.hpp"
#include "service.hpp"
#include "work_queue.hpp"
#include "logger.hpp"
#include "paths.hpp"

using namespace smol;
namespace dm
{
    // NanoVG

    void nanovg::create_context()
    {
        // init nanovg
        _ctx = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_DEBUG);
        SMOL_THROW_IF_NULL(_ctx);

        constexpr static const uint8_t debugPixels[] =
        {
            0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF,
            0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF,
        };

        _nvg_images[static_cast<size_t>(image_handle::debug)] = {nvgCreateImageRGBA(_ctx, 2, 2, NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY | NVG_IMAGE_NEAREST, debugPixels), {8,8}};

        // set command_queue to write mode
        _current.begin_writing();
    }

    font_handle_t nanovg::create_font(fs::path&& path)
    {
        auto fontFilePath = fs::canonical(path);
        auto it = _font_map.find(fontFilePath);
        if(it != _font_map.end()) {
            return it->second;
        }

        // index our font face id will be stored
        const auto index = next_font_handle();
        SMOL_ASSERT(static_cast<size_t>(index) == _nvg_font_faces.size());
        _font_map.insert({fontFilePath, index});
        _nvg_font_faces.push_back(INVALID_NVG_FONT_ID);

        worker_fileio.enqueue([index=index, fontFilePath=std::move(fontFilePath)]() -> void
        {
            auto fontFile = smol::load_file(fontFilePath);
            std::string fontName = fontFilePath.filename();

            worker_ui.enqueue([index=static_cast<size_t>(index), fontName=std::move(fontName), fontFile=std::move(fontFile)]() -> void
            {
                g_nanovg->_nvg_font_faces[index] = nvgCreateFontMem(g_nanovg->_ctx, fontName.c_str(), fontFile.data(), static_cast<int>(fontFile.size()));
                MX_LOG_INFO("Loaded font: {}", fontName);
            });
        });

        return index;
    }

    image_handle_t nanovg::create_image(fs::path&& path)
    {
        if (!fs::exists(path)) {
            MX_LOG_ERROR("Failed to load image {}: file does not exist", path);
            return image_handle::debug;
        }

        auto imageFilePath = fs::canonical(path);
        auto it = _image_map.find(imageFilePath);
        if (it != _image_map.end()) {
            return it->second;
        }

        const auto index = next_image_handle();
        SMOL_ASSERT(static_cast<size_t>(index) == _nvg_images.size());
        _image_map.insert({imageFilePath, index});
        _nvg_images.push_back({INVALID_NVG_IMAGE_ID, {}});

        worker_fileio.enqueue([index=index, imageFilePath=std::move(imageFilePath)]() -> void
        {
            // load file into memory
            auto imageFile = smol::load_file(imageFilePath);
            SMOL_THROW_IF_FALSE(imageFile.size() <= static_cast<size_t>(std::numeric_limits<int>::max()));
            const int imageFileSize = static_cast<int>(imageFile.size());

            // load pixel data from image
            int width = 0;
            int height = 0;
            std::unique_ptr<stbi_uc[], decltype(&stbi_image_free)> pixelData(
                stbi_load_from_memory(imageFile.data(), imageFileSize, &width, &height, nullptr, 4),
                stbi_image_free);

            const auto imageFileName = imageFilePath.filename();

            if (pixelData.get() ==  nullptr)
            {
                MX_LOG_ERROR("Failed to load image {}: {}", imageFileName, stbi_failure_reason());

                // use the magenta checkboard image
                worker_ui.enqueue([index=static_cast<size_t>(index)]() -> void
                {
                    g_nanovg->_nvg_images[index] = g_nanovg->_nvg_images[static_cast<size_t>(image_handle::debug)];
                });
            }
            else
            {
                worker_ui.enqueue([index=static_cast<size_t>(index), imageFileName=std::move(imageFileName), pixelData=std::move(pixelData), width=width, height=height]() -> void
                {
                    g_nanovg->_nvg_images[index] =
                    {
                        nvgCreateImageRGBA(g_nanovg->_ctx, width, height, NVG_IMAGE_GENERATE_MIPMAPS  | NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY, pixelData.get()),
                        {static_cast<float>(width), static_cast<float>(height)}
                    };
                    MX_LOG_INFO("Loaded image: {}", imageFileName);
                });
            }


        });

        return index;
    }

    SMOL_MAKE_ENUM(draw_command, uint8_t,
        set_scissor,
        color_fill_rect,
        color_fill_rounded_rect,
        color_fill_circle,
        image_fill_rect,
        image_fill_rounded_rect,
        image_fill_circle,
        stroke_rect,
        stroke_rounded_rect,
        stroke_circle,
        stroke_path,
        draw_text,
        draw_text_with_caret,
        draw_text_with_selection
    );

    NVGpaint nanovg::make_nvgpaint(image_handle_t ih, const rectf& dim, background_image_mode_t bim) const
    {
        const auto& image = _nvg_images[static_cast<size_t>(ih)];
        switch(bim) {
            case background_image_mode::stretch:
            {
                return nvgImagePattern(_ctx, dim.left(), dim.top(), dim.width(), dim.height(), 0.0f, image.id, 1.0f);
            }
            break;
            case background_image_mode::tile:
            {
                return nvgImagePattern(_ctx, dim.left(), dim.top(), image.width(), image.height(), 0.0f, image.id, 1.0f);
            }
            break;
            case background_image_mode::center:
            {
                rectf imageRect = { dim.center() - image.size / 2.0f, {image.size} };
                nvgScissor(_ctx, imageRect.position.x, imageRect.position.y, imageRect.size.x, imageRect.size.y);
                return nvgImagePattern(_ctx, imageRect.left(), imageRect.top(), imageRect.width(), imageRect.height(), 0.0f, image.id, 1.0f);
            }
            break;
            case background_image_mode::contain:
            {
                // candidate scale factors
                const float s_w = dim.width() / image.width();
                const float s_h = dim.height() / image.height();

                vec2f size;
                if (s_w  < s_h) {
                    size = image.size * s_w;
                } else {
                    size = image.size * s_h;
                }

                rectf imageRect = {dim.center() - size / 2.0f, size};
                nvgScissor(_ctx, imageRect.position.x, imageRect.position.y, imageRect.size.x, imageRect.size.y);
                return nvgImagePattern(_ctx, imageRect.left(), imageRect.top(), imageRect.width(), imageRect.height(), 0.0f, image.id, 1.0f);
            }
            break;
            default:
            {
                SMOL_FAIL_FAST();
            }
       }
       return {};
    }

    void nanovg::update(float dt, double window_width, double window_height, double pixel_ratio)
    {
        nvgBeginFrame(_ctx, window_width, window_height, pixel_ratio);

        draw_command_t cmd;
        auto& cq = _current;

        rectf currentScissor;

        const auto restoreScissor = [&]() -> void {
            nvgScissor(_ctx, currentScissor.position.x, currentScissor.position.y, currentScissor.size.x, currentScissor.size.y);
        };

        cq.begin_reading();
        while(cq.next<draw_command_t>(cmd))
        {
            switch(cmd)
            {
                default:
                {
                    SMOL_THROW_MSG("unknown draw command: {}", static_cast<uint8_t>(cmd));
                }
                break;
                case draw_command::set_scissor:
                {
                    rectf r;
                    SMOL_THROW_IF_FALSE(cq.next(r));

                    currentScissor = r;
                    nvgScissor(_ctx, r.position.x, r.position.y, r.size.x, r.size.y);
                }
                break;
                case draw_command::color_fill_rect:
                {
                    rectf r;
                    color col;
                    SMOL_THROW_IF_FALSE(cq.next(r));
                    SMOL_THROW_IF_FALSE(cq.next(col));

                    nvgBeginPath(_ctx);
                    nvgRect(_ctx, r.position.x, r.position.y, r.size.x, r.size.y);
                    nvgFillColor(_ctx, nvg(col));
                    nvgFill(_ctx);
                }
                break;
                case draw_command::color_fill_rounded_rect:
                {
                    rectf r;
                    cornersf rad;
                    color col;
                    SMOL_THROW_IF_FALSE(cq.next(r));
                    SMOL_THROW_IF_FALSE(cq.next(rad));
                    SMOL_THROW_IF_FALSE(cq.next(col));

                    nvgBeginPath(_ctx);
                    nvgRoundedRectVarying(_ctx, r.position.x, r.position.y, r.size.x, r.size.y, rad.topLeft, rad.topRight, rad.bottomRight, rad.bottomLeft);
                    nvgFillColor(_ctx, nvg(col));
                    nvgFill(_ctx);
                }
                break;
                case draw_command::color_fill_circle:
                {
                    circlef c;
                    color col;
                    SMOL_THROW_IF_FALSE(cq.next(c));
                    SMOL_THROW_IF_FALSE(cq.next(col));

                    nvgBeginPath(_ctx);
                    nvgCircle(_ctx, c.position.x, c.position.y, c.radius);
                    nvgFillColor(_ctx, nvg(col));
                    nvgFill(_ctx);
                }
                break;
                case draw_command::image_fill_rect:
                {
                    rectf r;
                    image_handle_t ih;
                    background_image_mode_t bim;
                    SMOL_THROW_IF_FALSE(cq.next(r));
                    SMOL_THROW_IF_FALSE(cq.next(ih));
                    SMOL_THROW_IF_FALSE(cq.next(bim));
                    // make sure this is a valid handle
                    SMOL_ASSERT(static_cast<size_t>(ih) < _nvg_images.size());

                    NVGpaint paint = make_nvgpaint(ih, r, bim);

                    nvgBeginPath(_ctx);
                    nvgRect(_ctx, r.position.x, r.position.y, r.size.x, r.size.y);
                    nvgFillPaint(_ctx, paint);
                    nvgFill(_ctx);

                    restoreScissor();
                }
                break;
                case draw_command::image_fill_rounded_rect:
                {
                    rectf r;
                    cornersf rad;
                    image_handle_t ih;
                    background_image_mode_t bim;
                    SMOL_THROW_IF_FALSE(cq.next(r));
                    SMOL_THROW_IF_FALSE(cq.next(rad));
                    SMOL_THROW_IF_FALSE(cq.next(ih));
                    SMOL_THROW_IF_FALSE(cq.next(bim));
                    // make sure this is a valid handle
                    SMOL_ASSERT(static_cast<size_t>(ih) < _nvg_images.size());

                    NVGpaint paint = make_nvgpaint(ih, r, bim);

                    nvgBeginPath(_ctx);
                    nvgRoundedRectVarying(_ctx, r.position.x, r.position.y, r.size.x, r.size.y, rad.topLeft, rad.topRight, rad.bottomRight, rad.bottomLeft);
                    nvgFillPaint(_ctx, paint);
                    nvgFill(_ctx);

                    restoreScissor();
                }
                break;
                case draw_command::image_fill_circle:
                {
                    circlef c;
                    image_handle_t ih;
                    background_image_mode_t bim;
                    SMOL_THROW_IF_FALSE(cq.next(c));
                    SMOL_THROW_IF_FALSE(cq.next(ih));
                    SMOL_THROW_IF_FALSE(cq.next(bim));
                    // make sure this is a valid handle
                    SMOL_ASSERT(static_cast<size_t>(ih) < _nvg_images.size());

                    NVGpaint paint = make_nvgpaint(ih, c.bounding_box(), bim);

                    nvgBeginPath(_ctx);
                    nvgCircle(_ctx, c.position.x, c.position.y, c.radius);
                    nvgFillPaint(_ctx, paint);
                    nvgFill(_ctx);

                    restoreScissor();
                }
                break;
                case draw_command::stroke_rect:
                {
                    rectf r;
                    float strokeWidth;
                    color col;
                    SMOL_THROW_IF_FALSE(cq.next(r));
                    SMOL_THROW_IF_FALSE(cq.next(strokeWidth));
                    SMOL_THROW_IF_FALSE(cq.next(col));

                    r.position += vec2f{strokeWidth / 2.0f, strokeWidth / 2.0f};
                    r.size -= vec2f{strokeWidth, strokeWidth};

                    nvgBeginPath(_ctx);
                    nvgRect(_ctx, r.position.x, r.position.y, r.size.x, r.size.y);
                    nvgStrokeColor(_ctx, nvg(col));
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
                    SMOL_THROW_IF_FALSE(cq.next(r));
                    SMOL_THROW_IF_FALSE(cq.next(rad));
                    SMOL_THROW_IF_FALSE(cq.next(strokeWidth));
                    SMOL_THROW_IF_FALSE(cq.next(col));
                    const float halfStrokeWidth = strokeWidth / 2.0f;

                    r.position += vec2f{halfStrokeWidth};
                    r.size -= vec2f{strokeWidth, strokeWidth};
                    rad.topLeft = max(0.0f, rad.topLeft - halfStrokeWidth);
                    rad.topRight = max(0.0f, rad.topRight - halfStrokeWidth);
                    rad.bottomRight = max(0.0f, rad.bottomRight - halfStrokeWidth);
                    rad.bottomLeft = max(0.0f, rad.bottomLeft - halfStrokeWidth);

                    nvgBeginPath(_ctx);
                    nvgRoundedRectVarying(_ctx, r.position.x, r.position.y, r.size.x, r.size.y, rad.topLeft, rad.topRight, rad.bottomRight, rad.bottomLeft);
                    nvgStrokeColor(_ctx, nvg(col));
                    nvgStrokeWidth(_ctx, strokeWidth);
                    nvgStroke(_ctx);
                }
                break;
                case draw_command::stroke_circle:
                {
                    circlef c;
                    float strokeWidth;
                    color col;
                    SMOL_THROW_IF_FALSE(cq.next(c));
                    SMOL_THROW_IF_FALSE(cq.next(strokeWidth));
                    SMOL_THROW_IF_FALSE(cq.next(col));

                    nvgBeginPath(_ctx);
                    nvgCircle(_ctx, c.position.x, c.position.y, c.radius);
                    nvgStrokeWidth(_ctx, strokeWidth);
                    nvgStrokeColor(_ctx, nvg(col));
                    nvgStroke(_ctx);
                }
                break;
                case draw_command::stroke_path:
                {
                    size_t pointCount = 0;
                    SMOL_THROW_IF_FALSE(cq.next(pointCount));
                    SMOL_THROW_IF_FALSE(pointCount >= 2);

                    nvgBeginPath(_ctx);
                    vec2f pt;
                    SMOL_THROW_IF_FALSE(cq.next(pt));
                    nvgMoveTo(_ctx, pt.x, pt.y);
                    for(size_t k = 1; k < pointCount; k++)
                    {
                        SMOL_THROW_IF_FALSE(cq.next(pt));
                        nvgLineTo(_ctx, pt.x, pt.y);
                    }
                    nvgClosePath(_ctx);

                    float strokeWidth;
                    color col;
                    SMOL_THROW_IF_FALSE(cq.next(strokeWidth));
                    SMOL_THROW_IF_FALSE(cq.next(col));

                    nvgStrokeWidth(_ctx, strokeWidth);
                    nvgStrokeColor(_ctx, nvg(col));
                    nvgStroke(_ctx);
                }
                break;
                case draw_command::draw_text:
                {
                    rectf bounds;
                    float textOffset;
                    font_handle_t font;
                    float size;
                    color col;
                    halign_t halign;
                    valign_t valign;
                    utf8_string_view str;
                    SMOL_THROW_IF_FALSE(cq.next(bounds));
                    SMOL_THROW_IF_FALSE(cq.next(textOffset));
                    SMOL_THROW_IF_FALSE(cq.next(font));
                    SMOL_THROW_IF_FALSE(cq.next(size));
                    SMOL_THROW_IF_FALSE(cq.next(col));
                    SMOL_THROW_IF_FALSE(cq.next(halign));
                    SMOL_THROW_IF_FALSE(cq.next(valign));
                    SMOL_THROW_IF_FALSE(cq.next(str));

                    auto startPosition = text_bounds_to_start_position(bounds, halign, valign);
                    startPosition.x -= textOffset;

                    nvgFontFaceId(_ctx, _nvg_font_faces[static_cast<size_t>(font)]);
                    nvgFontSize(_ctx, size);
                    nvgTextAlign(_ctx, static_cast<int>(halign) | static_cast<int>(valign));
                    nvgFillColor(_ctx, nvg(col));
                    nvgText(_ctx, startPosition.x, startPosition.y, str.begin(), str.end());
                }
                break;
                case draw_command::draw_text_with_caret:
                {
                    rectf bounds;
                    float textOffset;
                    font_handle_t font;
                    float fontSize;
                    color fontColor;
                    halign_t halign;
                    valign_t valign;
                    utf8_string_view str;
                    int32_t caretIndex;
                    float caretWidth;
                    color caretColor;

                    SMOL_THROW_IF_FALSE(cq.next(bounds));
                    SMOL_THROW_IF_FALSE(cq.next(textOffset));
                    SMOL_THROW_IF_FALSE(cq.next(font));
                    SMOL_THROW_IF_FALSE(cq.next(fontSize));
                    SMOL_THROW_IF_FALSE(cq.next(fontColor));
                    SMOL_THROW_IF_FALSE(cq.next(halign));
                    SMOL_THROW_IF_FALSE(cq.next(valign));
                    SMOL_THROW_IF_FALSE(cq.next(str));
                    SMOL_THROW_IF_FALSE(cq.next(caretIndex));
                    SMOL_THROW_IF_FALSE(cq.next(caretWidth));
                    SMOL_THROW_IF_FALSE(cq.next(caretColor));

                    auto startPosition = text_bounds_to_start_position(bounds, halign, valign);
                    startPosition.x -= textOffset;

                    // draw the text
                    nvgFontFaceId(_ctx, _nvg_font_faces[static_cast<size_t>(font)]);
                    nvgFontSize(_ctx, fontSize);
                    nvgTextAlign(_ctx, static_cast<int>(halign) | static_cast<int>(valign));
                    nvgFillColor(_ctx, nvg(fontColor));
                    nvgText(_ctx, startPosition.x, startPosition.y, str.begin(), str.end());

                    // calculate caret geometry
                    rectf caretBounds;
                    float lineHeight;
                    nvgTextMetrics(_ctx, nullptr, nullptr, &lineHeight);

                    // calculate caret position
                    const auto caretCount = caretIndex + 1;
                    NVGglyphPosition glyphPositions[caretCount];
                    const auto glyphCount = nvgTextGlyphPositions(_ctx, startPosition.x, startPosition.y, str.begin(), str.end(), glyphPositions, caretCount);
                    SMOL_THROW_IF_FALSE(glyphCount >= caretIndex);

                    float caretPosition;
                    // caret within the string
                    // caret at the end of the string
                    if (caretIndex == 0)
                    {
                        caretPosition = startPosition.x;
                    }
                    else if (caretIndex == glyphCount)
                    {
                        SMOL_ASSERT(caretIndex > 0);
                        caretPosition = glyphPositions[caretIndex - 1].maxx;
                    }
                    else
                    {
                        SMOL_ASSERT(caretIndex < glyphCount);
                        caretPosition = glyphPositions[caretIndex].minx;
                    }

                    caretBounds.position.x = std::round(caretPosition);

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
                        default:
                            SMOL_FAIL_FAST();
                    }
                    caretBounds.size = {caretWidth, lineHeight};

                    // draw caret
                    nvgBeginPath(_ctx);
                    nvgRect(_ctx, caretBounds.position.x, caretBounds.position.y, caretBounds.size.x, caretBounds.size.y);
                    nvgFillColor(_ctx, nvg(caretColor));
                    nvgFill(_ctx);
                }
                break;
                case draw_command::draw_text_with_selection:
                {
                    rectf bounds;
                    float textOffset;
                    font_handle_t font;
                    float fontSize;
                    color fontColor;
                    halign_t halign;
                    valign_t valign;
                    utf8_string_view str;
                    int32_t selectBegin;
                    int32_t selectEnd;
                    color selectedFontColor;
                    color selectionColor;

                    SMOL_THROW_IF_FALSE(cq.next(bounds));
                    SMOL_THROW_IF_FALSE(cq.next(textOffset));
                    SMOL_THROW_IF_FALSE(cq.next(font));
                    SMOL_THROW_IF_FALSE(cq.next(fontSize));
                    SMOL_THROW_IF_FALSE(cq.next(fontColor));
                    SMOL_THROW_IF_FALSE(cq.next(halign));
                    SMOL_THROW_IF_FALSE(cq.next(valign));
                    SMOL_THROW_IF_FALSE(cq.next(str));
                    SMOL_THROW_IF_FALSE(cq.next(selectBegin));
                    SMOL_THROW_IF_FALSE(cq.next(selectEnd));
                    SMOL_THROW_IF_FALSE(cq.next(selectedFontColor));
                    SMOL_THROW_IF_FALSE(cq.next(selectionColor));

                    SMOL_THROW_IF_FALSE(selectEnd > selectBegin);

                    auto startPosition = text_bounds_to_start_position(bounds, halign, valign);
                    startPosition.x -= textOffset;

                    // draw the text
                    nvgFontFaceId(_ctx, _nvg_font_faces[static_cast<size_t>(font)]);
                    nvgFontSize(_ctx, fontSize);
                    nvgTextAlign(_ctx, static_cast<int>(halign) | static_cast<int>(valign));
                    nvgFillColor(_ctx, nvg(fontColor));

                    // glyph positions
                    const auto maxGlyphCount = (int32_t)str.size();
                    NVGglyphPosition glyphPositions[maxGlyphCount];
                    const auto glyphCount = nvgTextGlyphPositions(_ctx, startPosition.x, startPosition.y, str.begin(), str.end(), glyphPositions, maxGlyphCount);
                    selectBegin = clamp(0, selectBegin, glyphCount);
                    selectEnd = clamp(0, selectEnd, glyphCount);

                    float selectBeginX = glyphPositions[selectBegin].minx;
                    float selectEndX = glyphPositions[selectEnd - 1].maxx;

                    // draw left unselected text
                    if (currentScissor.left() < selectBeginX)
                    {
                        rectf leftScissor = currentScissor;
                        {
                            float left = currentScissor.left();
                            float right = selectBeginX;
                            leftScissor.position.x = left;
                            leftScissor.size.x = right - left;
                        }
                        nvgScissor(_ctx, leftScissor.position.x, leftScissor.position.y, leftScissor.size.x, leftScissor.size.y);
                        nvgText(_ctx, startPosition.x, startPosition.y, str.begin(), str.end());
                    }

                    // draw right unselected text
                    if (currentScissor.right() > selectEndX)
                    {
                        rectf rightScissor = currentScissor;
                        {
                            float left = selectEndX;
                            float right = currentScissor.right();
                            rightScissor.position.x = left;
                            rightScissor.size.x = right - left;
                        }
                        nvgScissor(_ctx, rightScissor.position.x, rightScissor.position.y, rightScissor.size.x, rightScissor.size.y);
                        nvgText(_ctx, startPosition.x, startPosition.y, str.begin(), str.end());
                    }


                    // draw selection
                    if (selectBegin != selectEnd)
                    {
                        float lineHeight;
                        nvgTextMetrics(_ctx, nullptr, nullptr, &lineHeight);

                        rectf selectionBounds;
                        selectionBounds.position.x = selectBeginX;
                        switch(valign)
                        {
                            case valign::top:
                                selectionBounds.position.y = bounds.top();
                                break;
                            case valign::middle:
                                selectionBounds.position.y = (bounds.top() + bounds.bottom()) / 2.0f - lineHeight / 2.0f;
                                break;
                            case valign::bottom:
                                selectionBounds.position.y = bounds.bottom() - lineHeight;
                                break;
                            default:
                                SMOL_FAIL_FAST();
                        }
                        selectionBounds.size = {
                            selectEndX - selectBeginX,
                            lineHeight
                        };

                        // draw selection background adn selected text
                        rectf centerScissor = currentScissor;
                        {
                            float left = std::max(selectBeginX, centerScissor.left());
                            float right = std::min(selectEndX, centerScissor.right());
                            centerScissor.position.x = left;
                            centerScissor.size.x = right -  left;
                        }
                        nvgScissor(_ctx, centerScissor.position.x, centerScissor.position.y, centerScissor.size.x, centerScissor.size.y);

                        // background
                        nvgBeginPath(_ctx);
                        nvgRect(_ctx, selectionBounds.position.x, selectionBounds.position.y, selectionBounds.size.x, selectionBounds.size.y);
                        nvgFillColor(_ctx, nvg(selectionColor));
                        nvgFill(_ctx);

                        // text
                        nvgFillColor(_ctx, nvg(selectedFontColor));
                        nvgText(_ctx, startPosition.x, startPosition.y, str.begin(), str.end());

                        // restore current scissor
                        nvgScissor(_ctx, centerScissor.position.x, centerScissor.position.y, centerScissor.size.x, centerScissor.size.y);
                    }
                }
                break;
            }
        }

        nvgEndFrame(_ctx);

        std::swap(_current, _previous);
        _current.begin_writing();
        _current.set_layer(draw_layer::background);
    }

    void nanovg::set_scissor(const rectf& r)
    {
        auto& cq = _current;

        auto currentLayer = cq.get_current_layer();
        for(size_t layer = 0; layer < static_cast<size_t>(draw_layer::count); layer++)
        {
            cq.set_layer(static_cast<draw_layer_t>(layer));
            cq.push_back(draw_command::set_scissor);
            cq.push_back(r);
        }
        cq.set_layer(currentLayer);
    }

    void nanovg::set_draw_layer(const draw_layer_t layer)
    {
        auto& cq = _current;
        cq.set_layer(layer);
    }

    void nanovg::color_fill_rect(const rectf& r, const color& col)
    {
        auto& cq = _current;
        cq.push_back(draw_command::color_fill_rect);
        cq.push_back(r);
        cq.push_back(col);
    }

    void nanovg::color_fill_rounded_rect(const rectf& r, const cornersf& c, const color& col)
    {
        auto& cq = _current;
        cq.push_back(draw_command::color_fill_rounded_rect);
        cq.push_back(r);
        cq.push_back(c);
        cq.push_back(col);
    }

    void nanovg::color_fill_circle(const circlef& c, const color& col)
    {
        auto& cq = _current;
        cq.push_back(draw_command::color_fill_circle);
        cq.push_back(c);
        cq.push_back(col);
    }

    void nanovg::image_fill_rect(const rectf& r, const image_handle_t img, const background_image_mode_t bim)
    {
        auto& cq = _current;
        cq.push_back(draw_command::image_fill_rect);
        cq.push_back(r);
        cq.push_back(img);
        cq.push_back(bim);
    }

    void nanovg::image_fill_rounded_rect(const rectf& r, const cornersf& c, const image_handle_t img, const background_image_mode_t bim)
    {
        auto& cq = _current;
        cq.push_back(draw_command::image_fill_rounded_rect);
        cq.push_back(r);
        cq.push_back(c);
        cq.push_back(img);
        cq.push_back(bim);
    }

    void nanovg::image_fill_circle(const circlef& c, const image_handle_t img, const background_image_mode_t bim)
    {
        auto& cq = _current;
        cq.push_back(draw_command::image_fill_circle);
        cq.push_back(c);
        cq.push_back(img);
        cq.push_back(bim);
    }

    void nanovg::stroke_rect(const rectf& r, float strokeWidth, const color& col)
    {
        auto& cq = _current;
        cq.push_back(draw_command::stroke_rect);
        cq.push_back(r);
        cq.push_back(strokeWidth);
        cq.push_back(col);
    }

    void nanovg::stroke_rounded_rect(const rectf& r, const cornersf& c, float strokeWidth, const color& col)
    {
        auto& cq = _current;
        cq.push_back(draw_command::stroke_rounded_rect);
        cq.push_back(r);
        cq.push_back(c);
        cq.push_back(strokeWidth);
        cq.push_back(col);
    }

    void nanovg::stroke_circle(const circlef& c, float strokeWidth, const color& col)
    {
        auto& cq = _current;
        cq.push_back(draw_command::stroke_circle);
        cq.push_back(c);
        cq.push_back(strokeWidth);
        cq.push_back(col);
    }

    void nanovg::stroke_path(const smol::array<vec2f>& path, float strokeWidth, const color& col)
    {
        auto& cq = _current;
        cq.push_back(draw_command::stroke_path);
        cq.push_back(path);
        cq.push_back(strokeWidth);
        cq.push_back(col);
    }

    void nanovg::draw_text(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const utf8_string& str)
    {
        if (font == font_handle_t::invalid) {
            return;
        }

        auto& cq = _current;
        cq.push_back(draw_command::draw_text);
        cq.push_back(bounds);
        cq.push_back(textOffset);
        cq.push_back(font);
        cq.push_back(fontSize);
        cq.push_back(fontColor);
        cq.push_back(halign);
        cq.push_back(valign);
        cq.push_back(str);
    }

    void nanovg::draw_text_with_caret(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const utf8_string& str, int32_t caretIndex, float caretWidth, const color& caretColor)
    {
        if (font == font_handle_t::invalid) {
            return;
        }

        auto& cq = _current;
        if (caretIndex < 0)
        {
            draw_text(bounds, textOffset, font, fontSize, fontColor, halign, valign, str);
        }
        else
        {
            cq.push_back(draw_command::draw_text_with_caret);
            cq.push_back(bounds);
            cq.push_back(textOffset);
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

    void nanovg::draw_text_with_selection(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, const color& fontColor, halign_t halign, valign_t valign, const utf8_string& str, int32_t selectBegin, int32_t selectEnd, const color& selectedFontColor, const color& selectionColor)
    {
        if (font == font_handle_t::invalid) {
            return;
        }

        auto& cq = _current;
        if (selectBegin == selectEnd)
        {
            draw_text(bounds, textOffset, font, fontSize, fontColor, halign, valign, str);
        }
        else
        {
            cq.push_back(draw_command::draw_text_with_selection);
            cq.push_back(bounds);
            cq.push_back(textOffset);
            cq.push_back(font);
            cq.push_back(fontSize);
            cq.push_back(fontColor);
            cq.push_back(halign);
            cq.push_back(valign);
            cq.push_back(str);
            cq.push_back(selectBegin);
            cq.push_back(selectEnd);
            cq.push_back(selectedFontColor);
            cq.push_back(selectionColor);
        }
    }

    int32_t nanovg::get_caret_position(const rectf& bounds, float textOffset, font_handle_t font, float fontSize, halign_t halign, valign_t valign, const utf8_string& str, const vec2f& clickPoint)
    {
        // TODO: take y into account or is the assumption of this function that a click has happened?
        if (font == font_handle_t::invalid) {
            return -1;
        }

        if (str.size() == 0) {
            return 0;
        }

        auto startPosition = text_bounds_to_start_position(bounds, halign, valign);
        nvgFontFaceId(_ctx, _nvg_font_faces[static_cast<size_t>(font)]);
        nvgFontSize(_ctx, fontSize);
        nvgTextAlign(_ctx, static_cast<int>(halign) | static_cast<int>(valign));

        NVGglyphPosition glyphPositions[str.size()];
        int positions = nvgTextGlyphPositions(_ctx, startPosition.x, startPosition.y, str.begin(), str.end(), glyphPositions, str.size());

        const float x = clickPoint.x + textOffset;
        int32_t index = 0;
        if (x > glyphPositions[0].minx)
        {
            for(int i = 0; i < positions; ++i)
            {
                const auto& gp = glyphPositions[i];
                if (2.0f * x < (gp.maxx + gp.minx))
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

    float nanovg::get_caret_offset(const rectf& bounds, font_handle_t font, float fontSize, halign_t halign, valign_t valign, const utf8_string& str, size_t caretPosition)
    {
        if (font == font_handle_t::invalid) {
            return 0.0f;
        }

        auto startPosition = text_bounds_to_start_position(bounds, halign, valign);
        nvgFontFaceId(_ctx, _nvg_font_faces[static_cast<size_t>(font)]);
        nvgFontSize(_ctx, fontSize);
        nvgTextAlign(_ctx, static_cast<int>(halign) | static_cast<int>(valign));

        NVGglyphPosition glyphPositions[str.size()];
        size_t positions = (size_t)nvgTextGlyphPositions(_ctx, startPosition.x, startPosition.y, str.begin(), str.end(), glyphPositions, str.size());

        SMOL_ASSERT(caretPosition <= positions);
        if (positions == 0)
        {
            SMOL_ASSERT(caretPosition == 0);
            return 0.0f;
        }
        else if(caretPosition == positions)
        {
            return glyphPositions[positions - 1].maxx - glyphPositions[0].minx;
        }
        return glyphPositions[caretPosition].minx - glyphPositions[0].minx;
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
            default:
                SMOL_FAIL_FAST();
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
            default:
                SMOL_FAIL_FAST();
        }

        return vec2f{x, y};
    }
}

using namespace dm;
namespace nlohmann
{
    // font_handle_t
    void from_json(const nl::json& j, font_handle_t& fh)
    {
        SMOL_THROW_IF_FALSE(j.is_string());

        fh = g_nanovg->create_font(paths::skins() / j.get<std::string>());
    }

    // image_handle_t
    void from_json(const nl::json& j, image_handle_t& ih)
    {
        SMOL_THROW_IF_FALSE(j.is_string());

        ih = g_nanovg->create_image(paths::skins() / j.get<std::string>());
    }

    void from_json(const nl::json& j, halign_t& ha)
    {
        SMOL_THROW_IF_FALSE(j.is_string());

        const auto jstr = j.get<std::string>();
        if(jstr == "left")
        {
            ha = halign::left;
        }
        else if(jstr == "center")
        {
            ha = halign::center;
        }
        else if(jstr == "right")
        {
            ha = halign::right;
        }
        else
        {
            SMOL_THROW_MSG("Cannot parse '{}' as halign_t, must be 'left', 'center', or 'right'", jstr);
        }
    }

    void from_json(const nl::json& j, valign_t& va)
    {
        SMOL_THROW_IF_FALSE(j.is_string());

        const auto jstr = j.get<std::string>();
        if(jstr == "top")
        {
            va = valign::top;
        }
        else if(jstr == "middle")
        {
            va = valign::middle;
        }
        else if(jstr == "bottom")
        {
            va = valign::bottom;
        }
        else
        {
            SMOL_THROW_MSG("Cannot parse '{}' as valign_t, must be 'top', 'middle', or 'bottom'", jstr);
        }
    }

    // background image mode
    void from_json(const nl::json& j, background_image_mode_t& bim)
    {
        SMOL_THROW_IF_FALSE(j.is_string());

        const auto jstr = j.get<std::string>();
        constexpr const char* modeStrings[] =
        {
            "stretch",
            "tile",
            "center",
            "contain",
        };

        for(size_t i = 0; i < countof(modeStrings); ++i)
        {
            const auto* currentMode = modeStrings[i];
            if (jstr == currentMode)
            {
                bim = static_cast<background_image_mode_t>(i);
                return;
            }
        }

        SMOL_THROW_MSG("Cannot parse '{}' as background_image_mode_t, must be one of 'stretch', 'tile', 'center', or 'contain'", jstr);
    }
}
