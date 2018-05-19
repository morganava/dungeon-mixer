#pragma once

#include "hash.hpp"

namespace tabletopify
{
    struct font_key
    {
        std::string filename;
        float size;
        bool operator==(const font_key&) const;
    };

    struct font_key_hash
    {
        size_t operator()(const font_key& fk) const;
    };

    class font_stash
    {
    public:
        void begin();
        nk_user_font* add(const fs::path& fontFile, float fontSize);
        void end();
    private:
        std::unordered_map<font_key, nk_user_font*, font_key_hash> _font_map;
        nk_font_atlas* _font_atlas;
    };
}