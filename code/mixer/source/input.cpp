#include "utf8_string.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "utilities.hpp"

namespace tabletopify
{
    extern GLFWwindow* g_window;

    void input::initialize()
    {

    }

    void input::finish()
    {
        for(auto cursor : _mouse_cursors)
        {
            glfwDestroyCursor(cursor);
        }
    }

    void input::init_cursors()
    {
        _mouse_cursors[mouse_cursor::arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        _mouse_cursors[mouse_cursor::ibeam] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        _mouse_cursors[mouse_cursor::crosshair] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR );
        _mouse_cursors[mouse_cursor::hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    }

    // maps glfw 'key codes' to localized human readable names
    void input::build_key_map()
    {

        // https://www.glfw.org/docs/latest/group__keys.html

        utf8_stringbuffer sb;

        // Printable

        _key_names[GLFW_KEY_SPACE] = "spacebar";
        _key_names[GLFW_KEY_APOSTROPHE] = "'";
        _key_names[GLFW_KEY_COMMA] = ",";
        _key_names[GLFW_KEY_MINUS] = "-";
        _key_names[GLFW_KEY_PERIOD] = ".";
        _key_names[GLFW_KEY_SLASH] = "/";

        // number keys
        for(char c = '0'; c <= '9'; ++c)
        {
            sb.clear();
            sb.push_back(c);
            _key_names[GLFW_KEY_0 + c - '0'] = sb.str();
        }

        _key_names[GLFW_KEY_SEMICOLON] = ";";
        _key_names[GLFW_KEY_EQUAL] = "=";

        // alphabet keys
        for(char c = 'a'; c <= 'z'; ++c)
        {
            sb.clear();
            sb.push_back(c);
            _key_names[GLFW_KEY_A + c - 'a'] = sb.str();
        }

        _key_names[GLFW_KEY_LEFT_BRACKET] = "[";
        _key_names[GLFW_KEY_BACKSLASH] = "\\";
        _key_names[GLFW_KEY_RIGHT_BRACKET] = "]";
        _key_names[GLFW_KEY_GRAVE_ACCENT] = "`";
        _key_names[GLFW_KEY_WORLD_1] = "non-US #1";
        _key_names[GLFW_KEY_WORLD_2] = "non-US #2";

        // Non-Printable

        _key_names[GLFW_KEY_ESCAPE] = "escape";
        _key_names[GLFW_KEY_ENTER] = "enter";
        _key_names[GLFW_KEY_TAB] = "tab";
        _key_names[GLFW_KEY_BACKSPACE] = "backspace";
        _key_names[GLFW_KEY_INSERT] = "insert";
        _key_names[GLFW_KEY_DELETE] = "delete";
        _key_names[GLFW_KEY_RIGHT] = "right";
        _key_names[GLFW_KEY_LEFT] = "left";
        _key_names[GLFW_KEY_DOWN] = "down";
        _key_names[GLFW_KEY_UP] = "up";
        _key_names[GLFW_KEY_PAGE_UP] = "page up";
        _key_names[GLFW_KEY_PAGE_DOWN] = "page down";
        _key_names[GLFW_KEY_HOME] = "home";
        _key_names[GLFW_KEY_END] = "end";
        _key_names[GLFW_KEY_CAPS_LOCK] = "caps lock";
        _key_names[GLFW_KEY_SCROLL_LOCK] = "scroll lock";
        _key_names[GLFW_KEY_NUM_LOCK] = "num lock";
        _key_names[GLFW_KEY_PRINT_SCREEN] = "print screen";
        _key_names[GLFW_KEY_PAUSE] = "pause";

        // f-keys

        for(int f = 1; f <= 25; ++f)
        {
            sb.clear();
            sb.push_back('f');
            sb.push_back(fmt::format("{}", f));
            _key_names[GLFW_KEY_F1 + f - 1]  = sb.str();
        }

        // numpad numbers

        for(int i = 0; i <= 9; ++i)
        {
            sb.clear();
            sb.push_back("num pad ");
            sb.push_back(fmt::format("{}", i));
            _key_names[GLFW_KEY_KP_0 + i]  = sb.str();
        }

        _key_names[GLFW_KEY_KP_DECIMAL] = "num pad .";
        _key_names[GLFW_KEY_KP_DIVIDE] = "num pad /";
        _key_names[GLFW_KEY_KP_MULTIPLY] = "num pad *";
        _key_names[GLFW_KEY_KP_SUBTRACT] = "num pad -";
        _key_names[GLFW_KEY_KP_ADD] = "num pad +";
        _key_names[GLFW_KEY_KP_ENTER] = "num pad enter";
        _key_names[GLFW_KEY_KP_EQUAL] = "num pad =";
        _key_names[GLFW_KEY_LEFT_SHIFT] = "left shift";
        _key_names[GLFW_KEY_LEFT_CONTROL] = "left control";
        _key_names[GLFW_KEY_LEFT_ALT] = "left alt";
        _key_names[GLFW_KEY_LEFT_SUPER] = "left super";
        _key_names[GLFW_KEY_RIGHT_SHIFT] = "right shift";
        _key_names[GLFW_KEY_RIGHT_CONTROL] = "right control";
        _key_names[GLFW_KEY_RIGHT_ALT] = "right alt";
        _key_names[GLFW_KEY_RIGHT_SUPER] = "right super";
        _key_names[GLFW_KEY_MENU] = "menu";

        // Overwrite names with localized versions if available

        for(key_code_t kd = 0; kd <= GLFW_KEY_LAST; ++kd)
        {
            auto key_name = glfwGetKeyName(kd, 0);
            if (key_name != nullptr)
            {
                _key_names[kd] = key_name;
            }
        }

        // for(auto it : _key_names)
        // {
        //     MX_LOG_INFO("{} : '{}'", it.first, it.second);
        // }
    }

    const char* input::key_code_to_key_name(key_code_t kd) const
    {
        const auto& it = _key_names.find(kd);
        if(it != _key_names.cend())
        {
            return it->second.c_str();
        }
        return "(unknown)";
    }

    void input::update(float dt)
    {
        _mouse_state.previous = _mouse_state.current;

        // reset
        _mouse_state.current.scroll = {0.0, 0.0};
        _key_events.clear();

        // consume events
        for(const auto& ie : _raw_events)
        {
            switch(ie.type)
            {
                case input_event_type::charpress:
                {
                    keyboard_event ke;
                    ke.type = keyboard_event_type::charpress;
                    ke.codepoint = ie.charpress.codepoint;
                    ke.modifiers = ie.charpress.modifiers;
                    _key_events.push_back(ke);
                }
                break;
                case input_event_type::keydown:
                {
                    keyboard_event ke;
                    ke.type = keyboard_event_type::keydown;
                    ke.keycode = ie.keydown.keycode;
                    ke.modifiers = ie.keydown.modifiers;
                    _key_events.push_back(ke);
                }
                break;
                case input_event_type::keyrepeat:
                {
                    keyboard_event ke;
                    ke.type = keyboard_event_type::keyrepeat;
                    ke.keycode = ie.keyrepeat.keycode;
                    ke.modifiers = ie.keyrepeat.modifiers;
                    _key_events.push_back(ke);
                }
                break;
                case input_event_type::keyup:
                {
                    keyboard_event ke;
                    ke.type = keyboard_event_type::keyup;
                    ke.keycode = ie.keyup.keycode;
                    ke.modifiers = ie.keyup.modifiers;
                    _key_events.push_back(ke);
                }
                break;
                case input_event_type::mousemove:
                    _mouse_state.current.position = ie.mousemove.position;
                    break;
                case input_event_type::mousedown:
                    switch(ie.mousedown.button)
                    {
                        case mouse_button::left:
                            _mouse_state.current.left = button_state::pressed;
                            _mouse_state.down_position.left = _mouse_state.current.position;
                            _mouse_state.up_position.left = {-1.0, -1.0};
                            break;
                        case mouse_button::right:
                            _mouse_state.current.right = button_state::pressed;
                            _mouse_state.down_position.right = _mouse_state.current.position;
                            _mouse_state.up_position.right = {-1.0, -1.0};
                            break;
                        case mouse_button::middle:
                            _mouse_state.current.middle = button_state::pressed;
                            _mouse_state.down_position.middle = _mouse_state.current.position;
                            _mouse_state.up_position.middle = {-1.0, -1.0};
                            break;
                    }
                    break;
                case input_event_type::mouseup:
                    switch(ie.mouseup.button)
                    {
                        case mouse_button::left:
                            _mouse_state.current.left = button_state::released;
                            _mouse_state.up_position.left = _mouse_state.current.position;
                            break;
                        case mouse_button::right:
                            _mouse_state.current.right = button_state::released;
                            _mouse_state.up_position.right = _mouse_state.current.position;
                            break;
                        case mouse_button::middle:
                            _mouse_state.current.middle = button_state::released;
                            _mouse_state.up_position.middle = _mouse_state.current.position;
                            break;
                    }
                    break;
                case input_event_type::scroll:
                    _mouse_state.current.scroll = {ie.scroll.scroll_x, ie.scroll.scroll_y};
                    break;
                default:
                    break;
            }
        }

        _raw_events.clear();
    }

    void input::update_cursor()
    {
        if (_mouse_state.current.cursor != _mouse_state.previous.cursor)
        {
            glfwSetCursor(g_window, _mouse_cursors[_mouse_state.current.cursor]);
        }
    }

    void input::push_charpress(utf8proc_int32_t codepoint, modifier_keys modifiers)
    {
        tabletopify::utf8_stringbuffer sb;
        sb.push_back(codepoint);
        // MX_LOG_INFO("codepoint : {{ codepoint : 0x{:08x}, char : '{}', modifier_keys : {} }}", codepoint, sb.str(), modifiers);

        input_event ie;
        ie.type = input_event_type::charpress;
        ie.charpress.codepoint = codepoint;
        ie.charpress.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    void input::push_keydown(key_code_t keycode, modifier_keys modifiers)
    {
        // MX_LOG_INFO("keydown : {{ keycode : 0x{:08x}, key : '{}', modifier_keys : {} }}", keycode, key_code_to_key_name(keycode), modifiers);

        input_event ie;
        ie.type = input_event_type::keydown;
        ie.keydown.keycode = keycode;
        ie.keydown.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    void input::push_keyrepeat(key_code_t keycode, modifier_keys modifiers)
    {
        // MX_LOG_INFO("keyrepeat : {{ keycode : 0x{:08x}, key : '{}', modifier_keys : {} }}", keycode, key_code_to_key_name(keycode), modifiers);

        input_event ie;
        ie.type = input_event_type::keyrepeat;
        ie.keyrepeat.keycode = keycode;
        ie.keyrepeat.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    void input::push_keyup(key_code_t keycode, modifier_keys modifiers)
    {
        // MX_LOG_INFO("keyup : {{ keycode : 0x{:08x}, key : '{}', modifier_keys : {} }}", keycode, key_code_to_key_name(keycode), modifiers);

        input_event ie;
        ie.type = input_event_type::keyup;
        ie.keyup.keycode = keycode;
        ie.keyup.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    void input::push_mousemove(const vec2& pos)
    {
        // MX_LOG_INFO("mousemove : {{ position : {} }}", pos);
        input_event ie;
        ie.type = input_event_type::mousemove;
        ie.mousemove.position = pos;
        _raw_events.push_back(ie);
    }

    void input::push_mousedown(mouse_button_t btn, modifier_keys modifiers)
    {
        // MX_LOG_INFO("mousedown : {{ button : {}, modifier_keys : {} }}", btn, modifiers);
        input_event ie;
        ie.type = input_event_type::mousedown;
        ie.mousedown.button = btn;
        ie.mousedown.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    void input::push_mouseup(mouse_button_t btn, modifier_keys modifiers)
    {
        // MX_LOG_INFO("mouseup : {{ button : {}, modifier_keys : {} }}", btn, modifiers);
        input_event ie;
        ie.type = input_event_type::mouseup;
        ie.mouseup.button = btn;
        ie.mouseup.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    void input::push_scroll(double scroll_x, double scroll_y)
    {
        // MX_LOG_INFO("scroll : {{ x : {}, y : {} }}", scroll_x, scroll_y);
        input_event ie;
        ie.type = input_event_type::scroll;
        ie.scroll.scroll_x = scroll_x;
        ie.scroll.scroll_y = scroll_y;
        _raw_events.push_back(ie);
    }

    vec2 input::get_mouse_position(input_frame_t frame) const
    {
        return frame == input_frame::previous ? _mouse_state.previous.position : _mouse_state.current.position;
    }

    vec2 input::get_mouse_scroll(input_frame_t frame) const
    {
        return frame == input_frame::previous ? _mouse_state.previous.scroll : _mouse_state.current.scroll;
    }

    button_state_t input::get_mouse_button_state(mouse_button_t btn, input_frame_t frame) const
    {
        auto& mouse_state = frame == input_frame::previous ? _mouse_state.previous : _mouse_state.current;
        switch(btn)
        {
            case mouse_button::left:
                return mouse_state.left;
                break;
            case mouse_button::right:
                return mouse_state.right;
                break;
            case mouse_button::middle:
                return mouse_state.middle;
                break;
        }
        return button_state::released;
    }

    bool input::get_mouse_clicked(mouse_button_t btn) const
    {
        return (get_mouse_button_state(btn, input_frame::current) == button_state::released) &&
               (get_mouse_button_state(btn, input_frame::previous) == button_state::pressed);
    }

    bool input::get_mouse_clicked(mouse_button_t btn, const rect& region) const
    {
        if (get_mouse_clicked(btn))
        {
            switch(btn)
            {
                case mouse_button::left:
                    return point_inside_rect(_mouse_state.down_position.left, region) &&
                           point_inside_rect(_mouse_state.up_position.left, region);
                case mouse_button::right:
                    return point_inside_rect(_mouse_state.down_position.right, region) &&
                           point_inside_rect(_mouse_state.up_position.right, region);
                case mouse_button::middle:
                    return point_inside_rect(_mouse_state.down_position.middle, region) &&
                           point_inside_rect(_mouse_state.up_position.middle, region);
            }
        }
        return false;
    }

    void input::set_mouse_cursor(mouse_cursor_t cursor)
    {
        _mouse_state.current.cursor = cursor;
    }
}