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

    struct modifier_keys
    {
        button_state_t shift     : 1;
        button_state_t control   : 1;
        button_state_t alt       : 1;
        button_state_t super     : 1;
        button_state_t num_lock  : 1;
    };

    typedef enum class input_event_type
    {
        invalid = -1,
        charpress,
        keydown,
        keyrepeat,
        keyup,
        mousemove,
        mousedown,
        mouseup,
        scroll,
    } input_event_type_t;

    class click_detector
    {
    public:
        void update(float dt, button_state_t currentState);
        bool double_clicked() const;
        bool triple_clicked() const;
        void reset();
    private:
        constexpr static int32_t BUCKET_COUNT = 7;
        // TODO: get these constants from window manager
        const float _doubleClickTime = 0.5f;
        float _times[BUCKET_COUNT];
        uint32_t _frameCount[BUCKET_COUNT];
        button_state_t _states[BUCKET_COUNT];
    };

    struct input_event
    {
        input_event_type_t type;
        union
        {
            struct {
            } invalid;
            struct {
                utf8_codepoint codepoint;
                modifier_keys modifiers;
            } charpress;
            struct {
                key_code_t keycode;
                modifier_keys modifiers;
            } keydown;
            struct {
                key_code_t keycode;
                modifier_keys modifiers;
            } keyrepeat;
            struct {
                key_code_t keycode;
                modifier_keys modifiers;
            } keyup;
            struct {
                vec2 position;
            } mousemove;
            struct {
                mouse_button_t button;
                modifier_keys modifiers;
            } mousedown;
            struct {
                mouse_button_t button;
                modifier_keys modifiers;
            } mouseup;
            struct {
                double scroll_x;
                double scroll_y;
            } scroll;
        };

        input_event()
        : type(input_event_type::invalid)
        , invalid({})
        { }
    };
    typedef enum class keyboard_event_type
    {
        invalid = -1,
        charpress,
        keydown,
        keyrepeat,
        keyup,
    } keyboard_event_type_t;

    struct keyboard_event
    {
        keyboard_event_type_t type;
        union
        {
            struct {
            } invalid;
            utf8_codepoint codepoint;
            key_code_t keycode;
        };
        modifier_keys modifiers;

        keyboard_event()
        : type(keyboard_event_type::invalid)
        , invalid({})
        { }
    };

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

        void push_charpress(utf8proc_int32_t, modifier_keys);
        void push_keydown(key_code_t, modifier_keys);
        void push_keyrepeat(key_code_t, modifier_keys);
        void push_keyup(key_code_t, modifier_keys);
        void push_mousemove(const vec2& pos);
        void push_mousedown(mouse_button_t btn, modifier_keys);
        void push_mouseup(mouse_button_t btn, modifier_keys);
        void push_scroll(double scroll_x, double scroll_y);

        vec2 get_mouse_position(input_frame_t frame) const;
        vec2 get_mouse_scroll(input_frame_t frame) const;
        button_state_t get_mouse_button_state(mouse_button_t, input_frame_t frame) const;
        constexpr const std::vector<keyboard_event>& get_keyevents() const { return _key_events; }
        bool get_mouse_clicked(mouse_button_t) const;
        bool get_mouse_clicked(mouse_button_t, const rect& region) const;

        bool get_mouse_double_clicked(mouse_button_t) const;
        bool get_mouse_double_clicked(mouse_button_t, const rect& region) const;

        bool get_mouse_triple_clicked(mouse_button_t) const;
        bool get_mouse_triple_clicked(mouse_button_t, const rect& region) const;

        void set_mouse_cursor(mouse_cursor_t cursor);

        utf8_string get_clipboard_string() const;
        void set_clipboard_string(const utf8_string& str);
    private:
        std::map<key_code_t, utf8_string> _key_names;
        std::vector<input_event> _raw_events;
        std::vector<keyboard_event> _key_events;
        GLFWcursor* _mouse_cursors[mouse_cursor::count] = {};

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
                modifier_keys modifiers;
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

            struct
            {
                click_detector left;
                click_detector right;
                click_detector middle;
            } mouse_buttons;
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
std::ostream& operator<<(std::ostream&, tabletopify::modifier_keys);