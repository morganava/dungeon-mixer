#include "utf8_string.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "utilities.hpp"

using tabletopify::mouse_button_t;
using tabletopify::mouse_button;

std::ostream& operator<<(std::ostream& out, mouse_button_t btn)
{
    switch(btn)
    {
        case mouse_button::left: return out << "left";
        case mouse_button::right: return out << "right";
        case mouse_button::middle: return out << "middle";
        default: return out << "(unknown)";
    }
}

namespace tabletopify
{
    static input* g_input = nullptr;

    input* get_input()
    {
        MX_ASSERT(g_input != nullptr);
        return g_input;
    }
    void init_input()
    {
        MX_ASSERT(g_input == nullptr);
        g_input = new input();
        g_input->initialize();
    }
    void free_input()
    {
        g_input->finish();
        delete g_input;
        g_input = nullptr;
    }

    void input::initialize()
    {
        
    }

    void input::finish()
    {

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

        for(auto it : _key_names)
        {
            MX_LOG_INFO("{} : '{}'", it.first, it.second);
        }
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

        _mouse_state.current.scroll = {0.0, 0.0};
        _keypresses.clear();

        for(const auto& ie : _raw_events)
        {
            switch(ie.type) 
            {
                case keypress_event:
                    _keypresses.push_back(ie.keypress.codepoint);
                    break;
                case keydown_event:
                    break;
                case keyup_event:
                    break;
                case mousemove_event:
                    _mouse_state.current.position = ie.mousemove.position;
                    break;
                case mousedown_event:
                    switch(ie.mousedown.button)
                    {
                        case mouse_button::left:
                            _mouse_state.current.left = pressed;
                            break;
                        case mouse_button::right:
                            _mouse_state.current.right = pressed;
                            break;
                        case mouse_button::middle:
                            _mouse_state.current.middle = pressed;
                            break;
                    }
                    break;
                case mouseup_event:
                    switch(ie.mouseup.button)
                    {
                        case mouse_button::left:
                            _mouse_state.current.left = released;
                            break;
                        case mouse_button::right:
                            _mouse_state.current.right = released;
                            break;
                        case mouse_button::middle:
                            _mouse_state.current.middle = released;
                            break;
                    }
                    break;
                case scroll_event:
                    _mouse_state.current.scroll = {ie.scroll.scroll_x, ie.scroll.scroll_y};
                    break;
                default:
                    break;
            }
        }

        _raw_events.clear();
    }

    void input::push_keypress(utf8proc_int32_t codepoint)
    {
        tabletopify::utf8_stringbuffer sb;
        sb.push_back(codepoint);
        MX_LOG_INFO("keypress : {{ codepoint : 0x{:08x}, char : '{}' }}", codepoint, sb.str());

        input_event ie;
        ie.type = keypress_event;
        ie.keypress.codepoint = codepoint;
        _raw_events.push_back(ie);
    }

    void input::push_keydown(key_code_t keycode)
    {
        MX_LOG_INFO("keydown : {{ keycode : 0x{:08x}, key : '{}' }}", keycode, key_code_to_key_name(keycode));

        input_event ie;
        ie.type = keydown_event;
        ie.keydown.keycode = keycode;
        _raw_events.push_back(ie);
    }

    void input::push_keyup(key_code_t keycode)
    {
        MX_LOG_INFO("keyup : {{ keycode : 0x{:08x}, key : '{}' }}", keycode, key_code_to_key_name(keycode));

        input_event ie;
        ie.type = keyup_event;
        ie.keyup.keycode = keycode;
        _raw_events.push_back(ie);        
    }

    void input::push_mousemove(const vec2& pos)
    {
        MX_LOG_INFO("mousemove : {{ position : {} }}", pos);
        input_event ie;
        ie.type = mousemove_event;
        ie.mousemove.position = pos;
        _raw_events.push_back(ie);
    }

    void input::push_mousedown(mouse_button_t btn)
    {
        MX_LOG_INFO("mousedown : {{ button : {} }}", btn);
        input_event ie;
        ie.type = mousedown_event;
        ie.mousedown.button = btn;
        _raw_events.push_back(ie);
    }

    void input::push_mouseup(mouse_button_t btn)
    {
        MX_LOG_INFO("mouseup : {{ button : {} }}", btn);
        input_event ie;
        ie.type = mouseup_event;
        ie.mouseup.button = btn;
        _raw_events.push_back(ie);
    }

    void input::push_scroll(double scroll_x, double scroll_y)
    {
        MX_LOG_INFO("scroll : {{ x : {}, y : {} }}", scroll_x, scroll_y);
        input_event ie;
        ie.type = scroll_event;
        ie.scroll.scroll_x = scroll_x;
        ie.scroll.scroll_y = scroll_y;
        _raw_events.push_back(ie);
    }

    vec2 input::get_mouse_position(bool prev) const
    {
        return prev ? _mouse_state.previous.position : _mouse_state.current.position;
    }

    vec2 input::get_mouse_scroll(bool prev) const
    {
        return prev ? _mouse_state.previous.scroll : _mouse_state.current.scroll;
    }
}