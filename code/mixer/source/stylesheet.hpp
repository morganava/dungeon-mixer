#pragma once

#include "singleton.hpp"
#include "array.hpp"
#include "geometry.hpp"
#include "color.hpp"
#include "nanovg.hpp"
#include "utf8_string.hpp"
#include "motley_buffer.hpp"

namespace tabletopify
{
    typedef enum class element_type : uint_fast8_t
    {
        any,

        vbox,
        hbox,
        spacer,
        button,
        label,
        debug,

    } element_type_t;

    element_type_t parse_element_type(const std::string&);

    typedef enum class element_state : uint_fast8_t
    {
        none = 0,
        hover = 1 << 0,
        active= 1 << 1,
    } element_state_t;

    element_state_t parse_element_state(const std::string&);

    // element_state_t operator&(const element_state_t&, const element_state_t&);

    struct style_sheet
    {
        #define STYLE_SHEET_ENTRY(TYPE, NAME, VAL) TYPE NAME = VAL;
        #include "stylesheet.inl"
        #undef STYLE_SHEET_ENTRY
    };

    typedef enum style_entry
    {
        #define STYLE_SHEET_ENTRY(TYPE, NAME, VAL) NAME,
        #include "stylesheet.inl"
        #undef STYLE_SHEET_ENTRY
    } style_entry_t;

    struct style_declaration_set
    {
        style_sheet apply(const style_sheet&) const;
    private:
        mutable motley_buffer _declarations;
        friend void from_json(const nl::json&, style_declaration_set&);
    };

    void from_json(const nl::json&, style_declaration_set&);

    struct selector
    {
        element_type_t type;
        utf8_string tag;
        element_state_t state;

        bool operator==(const selector&) const;
    };
}

template <> struct std::hash<tabletopify::selector>
{
    size_t operator()(const tabletopify::selector&) const;
};

namespace tabletopify
{
    class style_resolver
    {
    public:
        const style_sheet* resolve(element_type_t et, const std::vector<utf8_string>& tags, element_state_t es) const;
    private:
        std::unordered_map<selector, style_declaration_set> _styleFactory;
        mutable style_sheet _rootStyle;

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


