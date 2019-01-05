#pragma once
namespace tabletopify
{
    typedef int32_t key_code_t;

    typedef enum class mouse_button : int_fast8_t
    {
        left   = GLFW_MOUSE_BUTTON_LEFT,
        right  = GLFW_MOUSE_BUTTON_RIGHT,
        middle = GLFW_MOUSE_BUTTON_MIDDLE
    } mouse_button_t;

    constexpr auto pressed = true;
    constexpr auto released = false;

    typedef enum
    {
        invalid_event = -1,
        keypress_event,
        keydown_event,
        keyup_event,
        mousemove_event,
        mousedown_event,
        mouseup_event,
    } input_event_type_t;

    struct input_event
    {
        input_event_type_t type;
        union
        {
            struct {
            } invalid;
            struct {
                utf8_codepoint codepoint;
            } keypress;
            struct {
                key_code_t keycode;
            } keydown;
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
        void update(float dt);

        const char* key_code_to_key_name(key_code_t) const;

        void push_keypress(utf8proc_int32_t codepoint);
        void push_keydown(key_code_t);
        void push_keyup(key_code_t);
        void push_mousemove(const vec2& pos);
        void push_mousedown(mouse_button_t btn);
        void push_mouseup(mouse_button_t btn);


        vec2 get_mouse_position(bool prev) const;

    private:
        std::map<key_code_t, utf8_string> _key_names;

        std::vector<input_event> _raw_events;

        std::vector<utf8_codepoint> _keypresses;

        struct
        {
            bool shift   : 1;
            bool control : 1;
            bool alt     : 1;
            bool super   : 1;
        } _modifier_keys;

        struct
        {            
            struct
            {
                vec2 position;
                struct
                {
                    bool left    : 1;
                    bool right   : 1;
                    bool middle  : 1;
                };
            } current, previous;
        } _mouse_state;
    };

    input* get_input();
    void init_input();
    void free_input();
}

std::ostream& operator<<(std::ostream&, tabletopify::mouse_button_t);