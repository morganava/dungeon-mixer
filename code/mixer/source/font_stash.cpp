#include "utilities.hpp"
#include "font_stash.hpp"
#include "logger.hpp"

namespace tabletopify
{
    bool font_key::operator==(const font_key& that) const
    {
        return this->filename == that.filename &&
               this->size == that.size;
    }

    size_t font_key_hash::operator()(const font_key& fk) const
    {
        size_t hash = tabletopify::calc_hash(
            fk.filename);
        hash = tabletopify::calc_hash(
            &fk.size,
            sizeof(fk.size),
            hash);
        return hash;
    }

    void font_stash::begin()
    {
        nk_font_stash_begin(&_font_atlas);
    }

    nk_user_font* font_stash::add(const fs::path& fontFile, float fontSize)
    {
        MX_THROW_IF_FALSE(fs::exists(fontFile));

        std::string fontName = fontFile.filename();

        auto it = _font_map.find({fontName, fontSize});
        if(it == _font_map.end())
        {
            nk_font* font = nk_font_atlas_add_from_file(
                                _font_atlas,
                                fontFile.native().data(),
                                fontSize,
                                nullptr);
            auto fontHandle = &(font->handle);
            _font_map.insert({{fontName, fontSize}, fontHandle});
            log(LogLevel_Info, ss() << "Baked font: " << fontName << ", " << fontSize << "px");
            return fontHandle;
        }
        return it->second;
    }

    void font_stash::end()
    {
        nk_font_stash_end();
    }
}