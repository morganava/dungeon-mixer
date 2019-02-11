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

    //  hash_buffer

    template<>
    void hash_buffer::push_back<rect>(const rect& val)
    {
        push_back<float>(val.position.x);
        push_back<float>(val.position.y);
        push_back<float>(val.size.x);
        push_back<float>(val.size.y);        
    }

    template<>
    void hash_buffer::push_back<utf8_string>(const utf8_string& str)
    {
        push_back<size_t>(str.size());
        push_back(str.c_str(), str.size());
    }

    template<>
    bool hash_buffer::next<utf8_string_view>(utf8_string_view& sv)
    {
        size_t len = 0;
        if (next<size_t>(len))
        {
            if (_read_head + len <= _raw.data() + _raw.size())
            {
                sv = utf8_string_view {
                    reinterpret_cast<const utf8proc_uint8_t*>(_read_head), 
                    reinterpret_cast<const utf8proc_uint8_t*>(_read_head) + len
                };
                _read_head += len;
                return true;
            }
        }
        return false;
    }

    void hash_buffer::push_back(const void* pVal, size_t valSize)
    {
        MX_ASSERT(_raw.data() == _read_head);

        // copy into raw buffer
        auto ptr = reinterpret_cast<const uint8_t*>(pVal);
        _raw.insert(_raw.end(), ptr, ptr+valSize);
        // update read head in case _raw resized internally
        _read_head = _raw.data();

        // update the hash
        _hash = calc_hash(ptr, valSize, _hash);
    }

    // NanoVG

    void nanovg::create_context()
    {
        // init nanovg
        _ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
        MX_THROW_IF_NULL(_ctx);
    }

    font_id_t nanovg::create_font(fs::path&& path)
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
        return false;

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
        draw_text
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
        while(cq.next(cmd))
        {
            switch(cmd)
            {
                case draw_command::set_scissor:
                {
                    float pos_x, pos_y, size_x, size_y;
                    MX_THROW_IF_FALSE(cq.next(pos_x));
                    MX_THROW_IF_FALSE(cq.next(pos_y));
                    MX_THROW_IF_FALSE(cq.next(size_x));
                    MX_THROW_IF_FALSE(cq.next(size_y));

                    nvgScissor(_ctx, pos_x, pos_y, size_x, size_y);
                }
                break;
                case draw_command::fill_rect:
                {
                    float pos_x, pos_y, size_x, size_y;
                    MX_THROW_IF_FALSE(cq.next(pos_x));
                    MX_THROW_IF_FALSE(cq.next(pos_y));
                    MX_THROW_IF_FALSE(cq.next(size_x));
                    MX_THROW_IF_FALSE(cq.next(size_y));

                    NVGcolor col;
                    MX_THROW_IF_FALSE(cq.next(col))

                    nvgBeginPath(_ctx);
                    nvgRect(_ctx, pos_x, pos_y, size_x, size_y);
                    nvgFillColor(_ctx, col);
                    nvgFill(_ctx);
                }
                break;
                case draw_command::draw_text:
                {
                    float pos_x, pos_y, size_x, size_y;
                    MX_THROW_IF_FALSE(cq.next(pos_x));
                    MX_THROW_IF_FALSE(cq.next(pos_y));
                    MX_THROW_IF_FALSE(cq.next(size_x));
                    MX_THROW_IF_FALSE(cq.next(size_y));

                    font_id_t font;
                    float size;
                    color col;
                    halign_t halign;
                    valign_t valign;
                    utf8_string_view str;
                    MX_THROW_IF_FALSE(cq.next(font));
                    MX_THROW_IF_FALSE(cq.next(size));
                    MX_THROW_IF_FALSE(cq.next(col));
                    MX_THROW_IF_FALSE(cq.next(halign));
                    MX_THROW_IF_FALSE(cq.next(valign));
                    MX_THROW_IF_FALSE(cq.next(str));

                    nvgFontFaceId(_ctx, _nvg_font_faces[font]);
                    nvgFontBlur(_ctx, 0.0);
                    nvgFontSize(_ctx, size);

                    float x = 0.0f;
                    float y = 0.0f;
                    int align = static_cast<int>(halign);

                    switch(halign)
                    {
                        case halign::left:
                            x = pos_x;
                            break;
                        case halign::center:
                            x = (pos_x + pos_x + size_x) / 2.0f;
                            break;
                        case halign::right:
                            x = pos_x + size_x;
                            break;
                    }

                    align |= static_cast<int>(valign);

                    switch(valign)
                    {
                        case valign::top:
                            y = pos_y;
                            break;
                        case valign::middle:
                            y = (pos_y + pos_y + size_y) / 2.0f;
                            break;
                        case valign::bottom:
                            y = pos_y + size_y;
                            break;
                    }

                    nvgTextAlign(_ctx, align);
                    nvgFillColor(_ctx, col);
                    nvgText(_ctx, x, y, str.begin(), str.end());
                }
                break;
            }
        }

        nvgEndFrame(_ctx);

        this->swap_buffers();
    }

    void nanovg::set_scissor(const rect& r)
    {
        auto& cq = _command_queue.current;
        cq.push_back(draw_command::set_scissor);
        cq.push_back(r);
    }

    void nanovg::fill_rect(const rect& r, const NVGcolor& col)
    {
        auto& cq = _command_queue.current;
        cq.push_back(draw_command::fill_rect);
        cq.push_back(r);
        cq.push_back(col);
    }

    void nanovg::draw_text(const rect& bounds, font_id_t font, float size, const color& col, halign_t halign, valign_t valign, const utf8_string& str)
    {
        auto& cq = _command_queue.current;
        cq.push_back(draw_command::draw_text);
        cq.push_back(bounds);        
        cq.push_back(font);
        cq.push_back(size);
        cq.push_back(col);
        cq.push_back(halign);
        cq.push_back(valign);
        cq.push_back(str);
    }
}