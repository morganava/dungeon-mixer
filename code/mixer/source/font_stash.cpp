#include "utilities.hpp"
#include "font_stash.hpp"
#include "logger.hpp"
#include "work_queue.hpp"
#include "nanovg.hpp"

namespace tabletopify
{
    font_index_t font_stash::load_font(fs::path&& path)
    {
        auto it = _font_map.find(path);
        if(it == _font_map.end())
        {
            // index our font face id will be stored
            auto retval = _nvg_font_faces.size();
            _nvg_font_faces.push_back(INVALID_NVG_FONT_ID);

            worker(Worker_FileIO).enqueue([index=retval, path=std::move(path)]() -> void
            {
                auto fontFilePath = fs::canonical(path);
                memory_buffer fontFile(fontFilePath);

                std::string fontName = path.filename();

                worker(Worker_UI).enqueue([index=index, fontName=std::move(fontName), fontFile=std::move(fontFile)]() -> void
                {
                    auto fontFaceId = g_nanovg.create_font(
                        fontName.c_str(),
                        fontFile.data(),
                        static_cast<int>(fontFile.size()));
                    
                    g_font_stash.set_font_face(
                        index, 
                        fontFaceId);;

                    MX_LOG_INFO("Loaded font: {}", fontName);
                });

                g_font_stash._font_map.insert({fontFilePath, index});
            });


            return retval;
        } 

        return it->second;
    }

    void font_stash::set_font_face(font_index_t idx, nvg_font_face_id_t id)
    {
        _nvg_font_faces[idx] = id;
    }

    nvg_font_face_id_t font_stash::get_font_face(font_index_t idx) const
    {
        MX_THROW_IF_FALSE(idx < _nvg_font_faces.size())
        return _nvg_font_faces[idx];
    }
}
