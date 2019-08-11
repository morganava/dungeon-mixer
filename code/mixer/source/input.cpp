#include "utf8_string.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "utilities.hpp"

namespace tabletopify
{
    extern GLFWwindow* g_window;

    constexpr modifier_keys operator&(const modifier_keys& left, const modifier_keys& right)
    {
        return 
        {
            left.shift & right.shift,
            left.control & right.control,
            left.alt & right.alt,
            left.super & right.super,
            left.num_lock & right.num_lock
        };
    }

    void click_detector::update(float dt)
    {
        _times[0] += dt;
        _frameCount[0] += 1;
    }

    void click_detector::push_state(button_state_t buttonState, modifier_keys modifiers)
    {
        if (buttonState != _states[0])
        {
            for(auto i = BUCKET_COUNT - 1; i > 0; --i)
            {
                const auto curr = i - 1;
                const auto prev = i;
                _times[prev] = _times[curr];
                _frameCount[prev] = _frameCount[curr];
                _states[prev] = _states[curr];
                _positions[prev] = _positions[curr];
                _modifiers[prev] = _modifiers[curr];
            }

            _times[0] = 0.0f;
            _states[0] = buttonState;
            _frameCount[0] = 0;
        }
        _modifiers[0] = modifiers;            
    }

    void click_detector::push_position(vec2 position)
    {
        _positions[0] = position;
    }

    bool click_detector::single_clicked(modifier_keys& mods, vec2& pressPosition, vec2& releasePosition) const
    {
        if (single_clicked_impl() && !double_clicked_impl() && !triple_clicked_impl())
        {
            pressPosition = _positions[1];
            releasePosition = _positions[0];
            mods = _modifiers[1] & _modifiers[0];
            return true;
        }
        return false;
    }

    bool click_detector::double_clicked(modifier_keys& mods, vec2& position) const
    {
        if (!triple_clicked_impl() && double_clicked_impl())
        {
            position = _positions[0];
            mods = _modifiers[3] & _modifiers[2] & _modifiers[1] & _modifiers[0];
            return true;
        }
        return false;
    }

    bool click_detector::triple_clicked(modifier_keys& mods, vec2& position) const
    {
        if (triple_clicked_impl())
        {
            position = _positions[0];
            mods = _modifiers[5] & _modifiers[4] & _modifiers[3] & _modifiers[2] & _modifiers[1] & _modifiers[0];
            return true;
        }
        return false;
    }

    void click_detector::reset()
    {
        for(size_t i = 1; i < BUCKET_COUNT; ++i)
        {
            _times[i] = 0.0f;
            _frameCount[i] = 0;
            _states[i] = button_state::released;
            _positions[i] = vec2::zero;
            _modifiers[i] = {};
        }
    }

    bool click_detector::single_clicked_impl() const
    {
        if (_states[0] == button_state::released &&
            _states[1] == button_state::pressed)
        {
            if (_frameCount[0] == 0)
            {
                return true;    
            }
        }
        return false;
    }

    bool click_detector::double_clicked_impl() const
    {
        if (_states[0] == button_state::released &&
            _states[1] == button_state::pressed &&
            _states[2] == button_state::released &&
            _states[3] == button_state::pressed)
        {
            // trigger on the first frame
            if (_frameCount[0] == 0)
            {
                float timeSum = _times[1] + _times[2] + _times[3];
                if ( timeSum < _doubleClickTime)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool click_detector::triple_clicked_impl() const
    {        
        if (_states[0] == button_state::released &&
            _states[1] == button_state::pressed &&
            _states[2] == button_state::released &&
            _states[3] == button_state::pressed &&
            _states[4] == button_state::released &&
            _states[5] == button_state::pressed)
        {
            // triggger on the first frame
            if (_frameCount[0] == 0)
            {
                float timeSum = _times[1] + _times[2] + _times[3];
                if (timeSum < _doubleClickTime)
                {
                    timeSum = _times[3] + _times[4] + _times[5];
                    if (timeSum < _doubleClickTime)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

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
        _mouse_events.clear();

        _mouse_state.mouse_buttons[mouse_button::left].update(dt);
        _mouse_state.mouse_buttons[mouse_button::right].update(dt);
        _mouse_state.mouse_buttons[mouse_button::middle].update(dt);

        // consume events
        for(const auto& ie : _raw_events)
        {
            switch(ie.type)
            {
                // keyboard events
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
                // mouse events
                case input_event_type::mousemove:
                    _mouse_state.current.position = ie.mousemove.position;
                    // mouse event
                    {
                        mouse_event me;
                        me.type = mouse_event_type::mouse_move;
                        me.mouse_move.previousPosition = _mouse_state.previous.position;
                        me.mouse_move.position = _mouse_state.current.position;
                        _mouse_events.push_back(me);

                        _mouse_state.mouse_buttons[mouse_button::left].push_position(_mouse_state.current.position);
                        _mouse_state.mouse_buttons[mouse_button::right].push_position(_mouse_state.current.position);
                        _mouse_state.mouse_buttons[mouse_button::middle].push_position(_mouse_state.current.position);
                    }
                    break;
                case input_event_type::mousedown:
                    _mouse_state.mouse_buttons[ie.mousedown.button].push_state(button_state::pressed, ie.mousedown.modifiers);
                    _mouse_state.current.buttons[ie.mousedown.button] = button_state::pressed;
                    // mouse event
                    {
                        mouse_event me;
                        me.type = mouse_event_type::mouse_down;
                        me.mouse_down.position = _mouse_state.current.position;
                        me.mouse_down.button = ie.mousedown.button;
                        me.mouse_down.modifiers = ie.mousedown.modifiers;
                        _mouse_events.push_back(me);
                    }
                    break;
                case input_event_type::mouseup:
                    {
                        const auto button = ie.mouseup.button;
                        const auto modifiers = ie.mousedown.modifiers;

                        _mouse_state.current.buttons[button] = button_state::released;
                        auto& cd = _mouse_state.mouse_buttons[button];

                        cd.push_state(button_state::released, modifiers);

                        // single click event
                        if (mouse_event me; cd.single_clicked(me.mouse_single_click.modifiers, me.mouse_single_click.pressPosition, me.mouse_single_click.releasePosition))
                        {
                            me.type = mouse_event_type::mouse_single_click;
                            me.mouse_single_click.button = button;

                            MX_LOG_INFO("SINGLE CLICK {}", button);
                            _mouse_events.push_back(me);
                        }
                        // double click event
                        else if (mouse_event me; cd.double_clicked(me.mouse_double_click.modifiers, me.mouse_double_click.position))
                        {
                            me.type = mouse_event_type::mouse_double_click;
                            me.mouse_double_click.button = button;

                            MX_LOG_INFO("DOUBLE CLICK {}", button);
                            _mouse_events.push_back(me);
                        }
                        // triple click event
                        else if (mouse_event me; cd.triple_clicked(me.mouse_triple_click.modifiers, me.mouse_triple_click.position))
                        {
                            me.type = mouse_event_type::mouse_triple_click;
                            me.mouse_triple_click.button = button;

                            _mouse_events.push_back(me);
                            MX_LOG_INFO("TRIPLE CLICK {}", ie.mouseup.button);
                            cd.reset();
                        }

                        // mouse down event
                        mouse_event me;
                        me.type = mouse_event_type::mouse_up;
                        me.mouse_down.position = _mouse_state.current.position;

                        _mouse_events.push_back(me);
                    }                 
                    break;
                case input_event_type::scroll:
                    _mouse_state.current.scroll += {ie.scroll.scroll_x, ie.scroll.scroll_y};
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
        MX_LOG_INFO("codepoint : {{ codepoint : 0x{:08x}, char : '{}', modifier_keys : {} }}", codepoint, sb.str(), modifiers);

        input_event ie;
        ie.type = input_event_type::charpress;
        ie.charpress.codepoint = codepoint;
        ie.charpress.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    constexpr static key_code_t convert_num_pad_keycode(key_code_t keycode)
    {
        if(keycode>= GLFW_KEY_KP_0 && keycode <= GLFW_KEY_KP_DECIMAL)
        {
            constexpr key_code_t numpadKeys[] =
            {
                GLFW_KEY_INSERT,
                GLFW_KEY_END,
                GLFW_KEY_DOWN,
                GLFW_KEY_PAGE_DOWN,
                GLFW_KEY_LEFT,
                GLFW_KEY_KP_5,
                GLFW_KEY_RIGHT,
                GLFW_KEY_HOME,
                GLFW_KEY_UP,
                GLFW_KEY_PAGE_UP,
                GLFW_KEY_DELETE,
            };

            keycode = numpadKeys[keycode - GLFW_KEY_KP_0];
        }
        return keycode;
    }

    void input::push_keydown(key_code_t keycode, modifier_keys modifiers)
    {
        if (!modifiers.num_lock)
        {
            keycode = convert_num_pad_keycode(keycode);
        }

        // MX_LOG_INFO("keydown : {{ keycode : 0x{:08x}, key : '{}', modifier_keys : {} }}", keycode, key_code_to_key_name(keycode), modifiers);

        input_event ie;
        ie.type = input_event_type::keydown;
        ie.keydown.keycode = keycode;
        ie.keydown.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    void input::push_keyrepeat(key_code_t keycode, modifier_keys modifiers)
    {
        if (!modifiers.num_lock)
        {
            keycode = convert_num_pad_keycode(keycode);
        }

        // MX_LOG_INFO("keyrepeat : {{ keycode : 0x{:08x}, key : '{}', modifier_keys : {} }}", keycode, key_code_to_key_name(keycode), modifiers);

        input_event ie;
        ie.type = input_event_type::keyrepeat;
        ie.keyrepeat.keycode = keycode;
        ie.keyrepeat.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    void input::push_keyup(key_code_t keycode, modifier_keys modifiers)
    {
        if (!modifiers.num_lock)
        {
            keycode = convert_num_pad_keycode(keycode);
        }

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
        MX_ASSERT(btn >= mouse_button::left && btn < mouse_button::count);
        auto& mouse_state = frame == input_frame::previous ? _mouse_state.previous : _mouse_state.current;
        return mouse_state.buttons[btn];
    }

    bool input::get_mouse_single_clicked(mouse_button_t btn) const
    {
        modifier_keys mods;
        vec2 press, release;
        return this->get_mouse_single_clicked(btn, mods, press, release);
    }

    bool input::get_mouse_single_clicked(mouse_button_t btn, modifier_keys& mods, vec2& pressPosition, vec2& releasePosition) const
    {
        for (auto& me : this->_mouse_events)
        {
            if (me.type == mouse_event_type::mouse_single_click && me.mouse_single_click.button == btn)
            {
                mods = me.mouse_single_click.modifiers;
                pressPosition = me.mouse_single_click.pressPosition;
                releasePosition = me.mouse_single_click.releasePosition;
                return true;
            }
        }
        return false;
    }

    bool input::get_mouse_double_clicked(mouse_button_t btn) const
    {
        modifier_keys mods;
        vec2 position;
        return this->get_mouse_double_clicked(btn, mods, position);
    }

    bool input::get_mouse_double_clicked(mouse_button_t btn, const rect& region) const
    {
        return point_inside_rect(_mouse_state.current.position, region) && get_mouse_double_clicked(btn);
    }

    bool input::get_mouse_double_clicked(mouse_button_t btn, modifier_keys& mods, vec2& releasePosition) const
    {
        for (auto& me : this->_mouse_events)
        {
            if (me.type == mouse_event_type::mouse_double_click && me.mouse_double_click.button == btn)
            {
                mods = me.mouse_double_click.modifiers;
                releasePosition = me.mouse_double_click.position;
                return true;
            }
        }
        return false;
    }

    bool input::get_mouse_triple_clicked(mouse_button_t btn) const
    {
        modifier_keys mods;
        vec2 position;
        return this->get_mouse_triple_clicked(btn, mods, position);
    }

    bool input::get_mouse_triple_clicked(mouse_button_t btn, const rect& region) const
    {
        return point_inside_rect(_mouse_state.current.position, region) && get_mouse_triple_clicked(btn);
    }

    bool input::get_mouse_triple_clicked(mouse_button_t btn, modifier_keys& mods, vec2& releasePosition) const
    {
        for (auto& me : this->_mouse_events)
        {
            if (me.type == mouse_event_type::mouse_triple_click && me.mouse_triple_click.button == btn)
            {
                mods = me.mouse_triple_click.modifiers;
                releasePosition = me.mouse_triple_click.position;
                return true;
            }
        }
        return false;
    }

    void input::set_mouse_cursor(mouse_cursor_t cursor)
    {
        _mouse_state.current.cursor = cursor;
    }

    utf8_string input::get_clipboard_string() const
    {
        auto retval = glfwGetClipboardString(g_window);
        MX_LOG_INFO("Get clipboard : '{}'", retval ? retval : "(null)");
        if (retval != nullptr)
        {
            return retval;
        }
        return "";
    }

    void input::set_clipboard_string(const utf8_string& str)
    {
        MX_LOG_INFO("Set clipboard : '{}'", str);
        glfwSetClipboardString(g_window, str.c_str());
    }
}