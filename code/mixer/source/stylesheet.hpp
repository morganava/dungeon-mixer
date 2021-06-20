#pragma once

#include "singleton.hpp"
#include "array.hpp"
#include "geometry.hpp"
#include "color.hpp"
#include "nanovg.hpp"
#include "utf8_string.hpp"
#include "serialization_buffer.hpp"
#include "enums.hpp"
#include "linear_constraint_solver.hpp"

namespace dm
{
    MX_MAKE_ENUM(element_type, uint_fast8_t,
        any,
        root,
        flex_box,
        scroll_area,
        spacer,
        button,
        label,
        text_box,
        slider,
        slider_track,
        slider_handle,
        range_slider,
        range_slider_track,
        range_slider_handle,
        checkbox,
        context_menu
    );
    element_type_t parse_element_type(const std::string&);

    MX_MAKE_FLAG_ENUM(element_state, uint_fast8_t,
        hover    = 1 << 0,
        active   = 1 << 1,
        disabled = 1 << 2,
        checked  = 1 << 3,
    );
    element_state_t parse_element_state(const std::string&);

    struct style_sheet
    {
        #define STYLE_SHEET_ENTRY(NAME, VAL, FLAGS) std::remove_const<std::remove_reference<decltype(VAL)>::type>::type NAME = VAL;
        #include "stylesheet.inl"
        #undef STYLE_SHEET_ENTRY
    };

    typedef enum class style_entry
    {
        #define STYLE_SHEET_ENTRY(NAME, VAL, FLAGS) NAME,
        #include "stylesheet.inl"
        #undef STYLE_SHEET_ENTRY
    } style_entry_t;

    struct style_declaration_set
    {
        style_sheet apply(const style_sheet&) const;
    private:
        mutable serialization_buffer _declarations;
        friend void from_json(const nl::json&, element_state_t, style_declaration_set&);
    };

    void from_json(const nl::json&, style_declaration_set&);

    struct selector
    {
        element_type_t type;
        array<utf8_string> tags;
        element_state_t state;

        bool operator==(const selector&) const;
    };
}

template <> struct std::hash<dm::selector>
{
    size_t operator()(const dm::selector&) const;
};

namespace dm
{
    class style_resolver
    {
    public:
        style_resolver() = default;
        style_resolver(fs::path&& stylesheet);
        const style_sheet* resolve(element_type_t et, const array<utf8_string>& tags, element_state_t es) const;
        box_layout resolve_box_layout(element_type_t et, const array<utf8_string>& tags) const;
    private:
        std::unordered_map<selector, style_declaration_set> _styleFactory;
        mutable std::unordered_map<selector, style_sheet> _cachedStyles;

        friend void from_json(const nl::json&,style_resolver&);
    };
    void from_json(const nl::json&, style_resolver&);
    MX_MAKE_SINGLETON(style_resolver, worker_flag::ui);
}
