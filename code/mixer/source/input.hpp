#pragma once

#include "singleton.hpp"

namespace tabletopify
{
    typedef int32_t key_code_t;

    MX_MAKE_ENUM(mouse_button, uint_fast8_t,
        left,
        right,
        middle
    );
    static_assert(static_cast<uint_fast8_t>(mouse_button::left) == GLFW_MOUSE_BUTTON_LEFT);
    static_assert(static_cast<uint_fast8_t>(mouse_button::right) == GLFW_MOUSE_BUTTON_RIGHT);
    static_assert(static_cast<uint_fast8_t>(mouse_button::middle) == GLFW_MOUSE_BUTTON_MIDDLE);
    static_assert(static_cast<uint_fast8_t>(mouse_button::count) == 3);

    MX_MAKE_ENUM(input_frame, uint_fast8_t,
        current,
        previous
    );

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

    class click_detector
    {
    public:
        void update(float dt);
        void push_state(button_state_t buttonState, modifier_keys modifiers);
        void push_position(vec2 position);

        void reset();

        bool get_input_event(class input_event& evt) const;

    private:
        bool single_clicked_impl() const;
        bool double_clicked_impl() const;
        bool triple_clicked_impl() const;
        bool drag_begin_impl() const;
        bool drag_impl() const;
        bool drag_end_impl() const;

        double squared_movement_past_n_buckets(int32_t bucketCount) const;

        constexpr static int32_t BUCKET_COUNT = 6;
        // TODO: get this constant from window manager
        constexpr static double DOUBLE_CLICK_TIME = 0.5f;
        // TODO: add a 'jiggle' amount of allowed movement (in pixels)
        constexpr static double MOVEMENT_THRESHOLD = 10;
        constexpr static double SQUARED_MOVEMENT_THRESHOLD = MOVEMENT_THRESHOLD*MOVEMENT_THRESHOLD;
        float _times[BUCKET_COUNT];
        uint32_t _frameCount[BUCKET_COUNT];
        button_state_t _states[BUCKET_COUNT];
        vec2 _startPositions[BUCKET_COUNT];
        vec2 _endPositions[BUCKET_COUNT];
        modifier_keys _modifiers[BUCKET_COUNT];
    };

    // raw events from glfw
    MX_MAKE_ENUM(raw_input_event_type, uint_fast8_t,
        char_press,
        key_down,
        key_repeat,
        key_up,
        mouse_move,
        mouse_down,
        mouse_up,
        scroll
    );

    struct raw_input_event
    {
        raw_input_event_type_t type;
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

        raw_input_event()
        : type(raw_input_event_type::invalid)
        , invalid({})
        { }
    };

    MX_MAKE_ENUM(input_event_type, uint_fast8_t,
        // keyboard
        char_press,
        key_down,
        key_repeat,
        key_up,
        // mouse
        mouse_move,
        mouse_down,
        mouse_up,
        mouse_single_click,
        mouse_double_click,
        mouse_triple_click,
        mouse_drag_begin,
        mouse_drag,
        mouse_drag_end,
        // scroll
        wheel_scroll
    );

    constexpr bool is_keyboard_event(input_event_type_t evt)
    {
        return evt >= input_event_type::char_press &&
               evt <= input_event_type::key_up;
    }

    constexpr bool is_mouse_event(input_event_type_t evt)
    {
        return evt >= input_event_type::mouse_move &&
               evt <= input_event_type::mouse_drag_end;
    }

    class input_event
    {
    public:
        input_event()
        : _type(input_event_type::invalid)
        { }

        input_event_type_t type() const
        {
            return _type;
        }

        utf8_codepoint codepoint() const
        {
            MX_ASSERT(is_keyboard_event(_type));
            return _keyboard.codepoint;
        }

        key_code_t keycode() const
        {
            MX_ASSERT(is_keyboard_event(_type));
            return _keyboard.keycode;
        }

        mouse_button_t mouse_button() const
        {
            MX_ASSERT(is_mouse_event(_type));
            return _mouse.button;
        }

        vec2 position() const
        {
            MX_ASSERT(is_mouse_event(_type));
            return _mouse.position;
        }

        modifier_keys modifiers() const
        {
            return _modifiers;
        }

        vec2 scroll() const
        {
            MX_ASSERT(_type == input_event_type::wheel_scroll);
            return _scroll;
        }
    private:
        input_event_type_t _type;
        union
        {
            struct {
                utf8_codepoint codepoint;
                key_code_t keycode;
            } _keyboard;
            struct {
                mouse_button_t button;
                vec2 position;
            } _mouse;
        };
        vec2 _scroll;
        modifier_keys _modifiers;
        friend class input;
        friend class click_detector;
    };

    MX_MAKE_ENUM(mouse_cursor, uint_fast8_t,
        arrow,
        ibeam,
        crosshair,
        hand
    );

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

        // TODO: kill these getters
        vec2 get_mouse_position(input_frame_t frame) const;
        vec2 get_mouse_scroll(input_frame_t frame) const;
        button_state_t get_mouse_button_state(mouse_button_t, input_frame_t frame) const;
        // TODO: but leave this one obviously
        constexpr const std::vector<input_event>& get_input_events() const { return _input_events; }

        void set_mouse_cursor(mouse_cursor_t cursor);

        utf8_string get_clipboard_string() const;
        void set_clipboard_string(const utf8_string& str);
    private:
        std::map<key_code_t, utf8_string> _key_names;
        std::vector<raw_input_event> _raw_events;
        std::vector<input_event> _input_events;
        GLFWcursor* _mouse_cursors[static_cast<size_t>(mouse_cursor::count)] = {};

        struct
        {
            struct
            {
                vec2 position = {-1.0, -1.0};
                vec2 scroll = {0.0, 0.0};
                button_state_t buttons[static_cast<size_t>(mouse_button::count)];
                mouse_cursor_t cursor;
            } current, previous;

            click_detector mouse_buttons[static_cast<size_t>(mouse_button::count)];
        } _mouse_state;
    };
    MX_MAKE_SINGLETON(input);
}

std::ostream& operator<<(std::ostream&, tabletopify::input_event);
std::ostream& operator<<(std::ostream&, tabletopify::modifier_keys);
