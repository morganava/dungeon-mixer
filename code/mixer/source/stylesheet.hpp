#pragma once

#include "singleton.hpp"
#include "array.hpp"
#include "geometry.hpp"
#include "color.hpp"
#include "nanovg.hpp"

namespace tabletopify
{
    typedef enum class element_type
    {
        any,

        button,
    } element_type_t;

    typedef enum class element_state
    {
        none,

        hover,
        active,
    } element_state_t;

    struct style_sheet
    {
        // scrollbar
        double scrollbarSize = 20.0;
        double scrollbarMinLength = 40.0;
        color scrollbarForeColor = color::rgb(0x60,0x60,0x60);
        color scrollbarActiveColor = color::rgb(0xc0,0x60,0x00);
        color scrollbarBackColor = color::rgb(0xc0,0xc0,0xc0);
        double scrollbarCornerRadius = 5.0;
        double scrollbarStep = 45.0;
        // margin
        edges margin;
        // border
        double borderSize = 3.0;
        color borderColor = color::rgb(0x00,0x00,0x00);
        corners borderRadius = 0.0;
        // padding
        edges padding;
        // background
        color backgroundColor = color::rgb(0xff,0xff,0xff);
        // font
        font_id fontFamily = font_id::invalid;
        color fontColor = color::rgb(0x00, 0x00, 0x00);
        double fontSize = 12.0;
        // text
        halign_t textHalign = halign::left;
        valign_t textValign = valign::middle;
    };

    class style_resolver
    {
    public:
        const style_sheet* resolve(const char* elementType) const;
    private:

        struct selector
        {
            element_type_t type;
            array<class utf8_string> classes;
            element_state_t state;
        };

        style_sheet _rootStyle;

        friend void from_json(const nl::json&,style_resolver&);
    };
    void from_json(const nl::json&, style_resolver&);

    inline void init_style_resolver()
    {
        singleton<style_resolver>::construct();
    }
    constexpr void free_style_resolver()
    {
        singleton<style_resolver>::free();
    }
    inline constexpr style_resolver& g_style_resolver = singleton<style_resolver>::get();
}