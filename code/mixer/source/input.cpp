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

    void click_detector::update(float dt, button_state_t currentState, vec2 position, modifier_keys modifiers)
    {
        if (currentState != _states[0])
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

            _times[0] = dt;
            _states[0] = currentState;
            _frameCount[0] = 1;
            _positions[0] = position;
            _modifiers[0] = modifiers;
        }
        else
        {
            _times[0] += dt;
            _frameCount[0] += 1;
        }
    }

    bool click_detector::single_clicked(modifier_keys& mods, vec2& pressPosition, vec2& releasePosition) const
    {
        // if (!double_clicked_impl() && single_clicked_impl())
        if (single_clicked_impl())
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

    bool click_detector::single_clicked_impl() const
    {
        if (_states[0] == button_state::released &&
            _states[1] == button_state::pressed)
        {
            if (_frameCount[0] == 1)
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
            if (_frameCount[0] == 1)
            {
                float timeSum = _times[1] + _times[2] + _times[3];
                if ( timeSum < _doubleClickTime)
                {
                    if ( _times[4] >= 0.5f * _doubleClickTime)
                    {
                        return true;
                    }
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
            // trigger on the first frame
            if (_frameCount[0] == 1)
            {
                float timeSum = _times[1] + _times[2] + _times[3] + _times[4] + _times[5];
                if (timeSum < 2.0 * _doubleClickTime)
                {
                    if (_times[6] >= 0.5f * _doubleClickTime)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void click_detector::reset()
    {
        for(auto i = 0 ; i < BUCKET_COUNT; ++i)
        {
            _times[i] = std::numeric_limits<float>::infinity();
            _frameCount[i] = 0;
            _states[i] = button_state::released;
        }
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
                    // reset double click detector on mouse move
                    _mouse_state.mouse_buttons.left.reset();
                    _mouse_state.mouse_buttons.right.reset();
                    _mouse_state.mouse_buttons.middle.reset();
                    // mouse event
                    {
                        mouse_event me;
                        me.type = mouse_event_type::mouse_move;
                        me.mouse_move.previousPosition = _mouse_state.previous.position;
                        me.mouse_move.position = _mouse_state.current.position;
                        _mouse_events.push_back(me);
                    }
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
                    _mouse_state.current.modifiers = ie.mousedown.modifiers;
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
                    _mouse_state.current.modifiers = ie.mousedown.modifiers;
                    // mouse event
                    {
                        mouse_event me;
                        me.type = mouse_event_type::mouse_up;
                        me.mouse_up.position = _mouse_state.current.position;
                        me.mouse_up.button = ie.mousedown.button;
                        me.mouse_up.modifiers = ie.mousedown.modifiers;
                        _mouse_events.push_back(me);
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

        _mouse_state.mouse_buttons.left.update(dt, _mouse_state.current.left, _mouse_state.current.position, {});
        _mouse_state.mouse_buttons.right.update(dt, _mouse_state.current.right, _mouse_state.current.position, {});
        _mouse_state.mouse_buttons.middle.update(dt, _mouse_state.current.middle, _mouse_state.current.position, {});

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

    bool input::get_mouse_single_clicked(mouse_button_t btn) const
    {
        modifier_keys mf;
        vec2 pressed, released;
        switch(btn)
        {
            case mouse_button::left:
                return _mouse_state.mouse_buttons.left.single_clicked(mf, pressed, released);
            case mouse_button::right:
                return _mouse_state.mouse_buttons.right.single_clicked(mf, pressed, released);
            case mouse_button::middle:
                return _mouse_state.mouse_buttons.middle.single_clicked(mf, pressed, released);
        }
        return false;
    }

    bool input::get_mouse_single_clicked(mouse_button_t btn, modifier_keys& mods, vec2& pressPosition, vec2& releasePosition) const
    {
        switch(btn)
        {
            case mouse_button::left:
                return _mouse_state.mouse_buttons.left.single_clicked(mods, pressPosition, releasePosition);
            case mouse_button::right:
                return _mouse_state.mouse_buttons.right.single_clicked(mods, pressPosition, releasePosition);
            case mouse_button::middle:
                return _mouse_state.mouse_buttons.middle.single_clicked(mods, pressPosition, releasePosition);
        }
        return false;
    }

    bool input::get_mouse_double_clicked(mouse_button_t btn) const
    {
        modifier_keys mf;
        vec2 clicked;
        switch(btn)
        {
            case mouse_button::left:
                return _mouse_state.mouse_buttons.left.double_clicked(mf, clicked);
            case mouse_button::right:
                return _mouse_state.mouse_buttons.right.double_clicked(mf, clicked);
            case mouse_button::middle:
                return _mouse_state.mouse_buttons.middle.double_clicked(mf, clicked);
        }
        return false;
    }

    bool input::get_mouse_double_clicked(mouse_button_t btn, const rect& region) const
    {
        return point_inside_rect(_mouse_state.current.position, region) && get_mouse_double_clicked(btn);
    }

    bool input::get_mouse_triple_clicked(mouse_button_t btn) const
    {
        modifier_keys mf;
        vec2 clicked;
        switch(btn)
        {
            case mouse_button::left:
                return _mouse_state.mouse_buttons.left.triple_clicked(mf, clicked);
            case mouse_button::right:
                return _mouse_state.mouse_buttons.right.triple_clicked(mf, clicked);
            case mouse_button::middle:
                return _mouse_state.mouse_buttons.middle.triple_clicked(mf, clicked);
        }
        return false;
    }

    bool input::get_mouse_triple_clicked(mouse_button_t btn, const rect& region) const
    {
        return point_inside_rect(_mouse_state.current.position, region) && get_mouse_triple_clicked(btn);
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