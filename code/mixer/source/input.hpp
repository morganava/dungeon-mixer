#pragma once

#include "singleton.hpp"

namespace tabletopify
{
    typedef int32_t key_code_t;

    typedef enum class mouse_button : int_fast8_t
    {
        left   = GLFW_MOUSE_BUTTON_LEFT,
        right  = GLFW_MOUSE_BUTTON_RIGHT,
        middle = GLFW_MOUSE_BUTTON_MIDDLE
    } mouse_button_t;

    typedef enum class input_frame : int_fast8_t
    {
        current,
        previous
    } input_frame_t;

    typedef bool button_state_t;
    namespace button_state
    {
        constexpr button_state_t pressed = true;
        constexpr button_state_t released = false;;
    }

    typedef enum
    {
        invalid_event = -1,
        codepoint_event,
        keydown_event,
        keyrepeat_event,
        keyup_event,
        mousemove_event,
        mousedown_event,
        mouseup_event,
        scroll_event,
    } input_event_type_t;

    namespace mouse_cursor
    {
        enum value
        {
            arrow = 0,
            ibeam,
            crosshair,
            hand,

            count
        };
    }
    typedef mouse_cursor::value mouse_cursor_t;

    struct input_event
    {
        input_event_type_t type;
        union
        {
            struct {
            } invalid;
            struct {
                utf8_codepoint codepoint;
            } charpress;
            struct {
                key_code_t keycode;
            } keydown;
            struct {
                key_code_t keycode;
            } keyrepeat;
            struct {
                key_code_t keycode;
            } keyup;
            struct {
                vec2 position;
            } mousemove;
            struct {
                mouse_button_t button;
            } mousedown;
            struct {
                mouse_button_t button;
            } mouseup;
            struct {
                double scroll_x;
                double scroll_y;
            } scroll;
        };

        input_event()
        : type(invalid_event)
        , invalid({})
        { }
    };

    class input
    {
    public:
        void initialize();
        void finish();

        void build_key_map();
        void init_cursors();
        void update(float dt);
        void update_cursor();

        const char* key_code_to_key_name(key_code_t) const;

        void push_charpress(utf8proc_int32_t codepoint);
        void push_keydown(key_code_t);
        void push_keyrepeat(key_code_t);
        void push_keyup(key_code_t);
        void push_mousemove(const vec2& pos);
        void push_mousedown(mouse_button_t btn);
        void push_mouseup(mouse_button_t btn);
        void push_scroll(double scroll_x, double scroll_y);

        vec2 get_mouse_position(input_frame_t frame) const;
        vec2 get_mouse_scroll(input_frame_t frame) const;
        button_state_t get_mouse_button_state(mouse_button_t, input_frame_t frame) const;
        constexpr const std::vector<utf8_codepoint>& get_charpresses() const { return _charpresses; }
        constexpr const std::vector<key_code_t>& get_keypresses() const { return _keypresses; }
        bool get_mouse_clicked(mouse_button_t) const;
        bool get_mouse_clicked(mouse_button_t, const rect& region) const;

        void set_mouse_cursor(mouse_cursor_t cursor);
    private:
        std::map<key_code_t, utf8_string> _key_names;
        std::vector<input_event> _raw_events;
        std::vector<utf8_codepoint> _charpresses;
        std::vector<key_code_t> _keypresses;
        GLFWcursor* _mouse_cursors[mouse_cursor::count] = {};

        struct
        {
            button_state_t shift   : 1;
            button_state_t control : 1;
            button_state_t alt     : 1;
            button_state_t super   : 1;
        } _modifier_keys;

        struct
        {
            struct
            {
                vec2 position = {-1.0, -1.0};
                vec2 scroll = {0.0, 0.0};
                struct
                {
                    button_state_t left    : 1;
                    button_state_t right   : 1;
                    button_state_t middle  : 1;
                };
                mouse_cursor_t cursor;
            } current, previous;

            struct
            {
                vec2 left = {-1.0, -1.0};
                vec2 right = {-1.0, -1.0};
                vec2 middle = {-1.0, -1.0};
            } down_position;

            struct
            {
                vec2 left = {-1.0, -1.0};
                vec2 right = {-1.0, -1.0};
                vec2 middle = {-1.0, -1.0};
            } up_position;
        } _mouse_state;
    };

    inline constexpr input& g_input = singleton<input>::get();
    inline void init_input()
    {
        singleton<input>::construct();
    }

    constexpr void free_input()
    {
        singleton<input>::free();
    }
}

std::ostream& operator<<(std::ostream&, tabletopify::mouse_button_t);