#include "logger.hpp"
#include "utf8_string.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "utilities.hpp"

namespace dm
{
    extern GLFWwindow* g_window;

    constexpr modifier_keys operator&(const modifier_keys& left, const modifier_keys& right)
    {
        return
        {
            static_cast<button_state_t>(left.shift & right.shift),
            static_cast<button_state_t>(left.control & right.control),
            static_cast<button_state_t>(left.alt & right.alt),
            static_cast<button_state_t>(left.super & right.super),
            static_cast<button_state_t>(left.num_lock & right.num_lock)
        };
    }

    void click_detector::update(float dt)
    {
        _times[0] += dt;
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
                _startPositions[prev] = _startPositions[curr];
                _endPositions[prev] = _endPositions[curr];
                _modifiers[prev] = _modifiers[curr];
            }

            _times[0] = 0.0f;
            _states[0] = buttonState;
            _startPositions[0] = _endPositions[1];
            _endPositions[0] = _endPositions[1];
            _frameCount[0] = 0;
        }
        else
        {
            _frameCount[0] += 1;
        }
        _modifiers[0] = modifiers;
    }

    void click_detector::push_position(vec2 position)
    {
        if (position != _endPositions[0])
        {
            _endPositions[0] = position;
            _frameCount[0] += 1;
        }
    }

    void click_detector::reset()
    {
        for(size_t i = 1; i < BUCKET_COUNT; ++i)
        {
            _times[i] = 0.0f;
            _frameCount[i] = 0;
            _states[i] = button_state::released;
            _startPositions[i] = vec2::zero;
            _endPositions[i] = vec2::zero;
            _modifiers[i] = {};
        }
    }

    bool click_detector::get_input_event(input_event& evt) const
    {
        if (this->triple_clicked_impl())
        {
            evt._type = input_event_type::mouse_triple_click;
            evt._mouse.position = _endPositions[0];
            evt._modifiers = _modifiers[5] & _modifiers[4] & _modifiers[3] & _modifiers[2] & _modifiers[1] & _modifiers[0];
            return true;
        }
        else if (this->double_clicked_impl())
        {
            evt._type = input_event_type::mouse_double_click;
            evt._mouse.position = _endPositions[0];
            evt._modifiers = _modifiers[3] & _modifiers[2] & _modifiers[1] & _modifiers[0];
            return true;
        }
        else if (this->single_clicked_impl())
        {
            evt._type = input_event_type::mouse_single_click;
            evt._mouse.position = _endPositions[0];
            evt._modifiers = _modifiers[1] & _modifiers[0];
            return true;
        }
        else if (this->drag_begin_impl())
        {
            evt._type = input_event_type::mouse_drag_begin;
            evt._mouse.position = _endPositions[0];
            evt._modifiers = _modifiers[0];
            return true;
        }
        else if (this->drag_impl())
        {
            evt._type = input_event_type::mouse_drag;
            evt._mouse.position = _endPositions[0];
            evt._modifiers = _modifiers[0];
            return true;
        }
        else if (this->drag_end_impl())
        {
            evt._type = input_event_type::mouse_drag_end;
            evt._mouse.position = _endPositions[0];
            evt._modifiers = _modifiers[1] & _modifiers[0];
            return true;
        }

        return false;
    }

    bool click_detector::single_clicked_impl() const
    {
        if (_states[0] == button_state::released &&
            _states[1] == button_state::pressed)
        {
            // trigger on the first update after release
            if (_frameCount[0] == 0)
            {
                if (this->squared_movement_past_n_buckets(2) < SQUARED_MOVEMENT_THRESHOLD)
                {
                    return true;
                }
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
            // trigger on the first update after release
            if (_frameCount[0] == 0)
            {
                float timeSum = _times[1] + _times[2] + _times[3];
                if ( timeSum < DOUBLE_CLICK_TIME)
                {
                    if (this->squared_movement_past_n_buckets(4) < SQUARED_MOVEMENT_THRESHOLD)
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
            // triggger on the first update after release
            if (_frameCount[0] == 0)
            {
                float timeSum = _times[1] + _times[2] + _times[3];
                if (timeSum < DOUBLE_CLICK_TIME)
                {
                    timeSum = _times[3] + _times[4] + _times[5];
                    if (timeSum < DOUBLE_CLICK_TIME)
                    {
                        if (this->squared_movement_past_n_buckets(6) < SQUARED_MOVEMENT_THRESHOLD)
                        {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    bool click_detector::drag_begin_impl() const
    {
        if (_states[0] == button_state::pressed &&
            _states[1] == button_state::released)
        {
            // trigger on second update after press (for a move)
            if (_frameCount[0] == 1)
            {
                return true;
            }
        }
        return false;
    }

    bool click_detector::drag_impl() const
    {
        if (_states[0] == button_state::pressed &&
            _states[1] == button_state::released)
        {
            if (_frameCount[0] > 0)
            {
                return true;
            }
        }
        return false;
    }

    bool click_detector::drag_end_impl() const
    {
        if (_states[0] == button_state::released &&
            _states[1] == button_state::pressed &&
            _states[2] == button_state::released)
        {
            // triggger on the first update after release
            if (_frameCount[0] == 0)
            {
                if (_startPositions[1] != _endPositions[1])
                {
                    return true;
                }
            }
        }
        return false;
    }

    double click_detector::squared_movement_past_n_buckets(int32_t bucketCount) const
    {
        double retval = 0.0;
        SMOL_ASSERT(bucketCount <= click_detector::BUCKET_COUNT);
        for(int32_t i = 0; i < bucketCount; ++i)
        {
            retval += (_endPositions[i] - _startPositions[i]).squared_magnitude();
        }

        return retval;
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
        _mouse_cursors[static_cast<size_t>(mouse_cursor::arrow)] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        _mouse_cursors[static_cast<size_t>(mouse_cursor::ibeam)] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        _mouse_cursors[static_cast<size_t>(mouse_cursor::crosshair)] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR );
        _mouse_cursors[static_cast<size_t>(mouse_cursor::hand)] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
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
        _input_events.clear();

        _mouse_state.mouse_buttons[static_cast<size_t>(mouse_button::left)].update(dt);
        _mouse_state.mouse_buttons[static_cast<size_t>(mouse_button::right)].update(dt);
        _mouse_state.mouse_buttons[static_cast<size_t>(mouse_button::middle)].update(dt);

        // consume events
        for(const auto& ie : _raw_events)
        {
            switch(ie.type)
            {
                // keyboard events
                case raw_input_event_type::char_press:
                {
                    input_event evt;
                    evt._type = input_event_type::char_press;
                    evt._keyboard.codepoint = ie.charpress.codepoint;
                    evt._modifiers = ie.charpress.modifiers;
                    _input_events.push_back(evt);
                    break;
                }
                case raw_input_event_type::key_down:
                {
                    input_event evt;
                    evt._type = input_event_type::key_down;
                    evt._keyboard.keycode = ie.keydown.keycode;
                    evt._modifiers = ie.keydown.modifiers;
                    _input_events.push_back(evt);
                    break;
                }
                case raw_input_event_type::key_repeat:
                {
                    input_event evt;
                    evt._type = input_event_type::key_repeat;
                    evt._keyboard.keycode = ie.keyrepeat.keycode;
                    evt._modifiers = ie.keyrepeat.modifiers;
                    _input_events.push_back(evt);
                    break;
                }
                case raw_input_event_type::key_up:
                {
                    input_event evt;
                    evt._type = input_event_type::key_up;
                    evt._keyboard.keycode = ie.keyup.keycode;
                    evt._modifiers = ie.keyup.modifiers;
                    _input_events.push_back(evt);
                    break;
                }
                // mouse events
                case raw_input_event_type::mouse_move:
                {
                    const auto position = ie.mousemove.position;
                    _mouse_state.current.position = position;

                    for(size_t btn = 0; btn < static_cast<size_t>(mouse_button::count); ++btn)
                    {
                        auto& cd = _mouse_state.mouse_buttons[btn];
                        cd.push_position(position);
                        if (input_event evt; cd.get_input_event(evt))
                        {
                            evt._mouse.button = static_cast<mouse_button_t>(btn);
                            _input_events.push_back(evt);
                            if (evt.type() == input_event_type::mouse_triple_click) {
                                cd.reset();
                            }
                       }
                    }

                    input_event evt;
                    evt._type = input_event_type::mouse_move;
                    evt._mouse.position = position;
                    _input_events.push_back(evt);

                    break;
                }
                case raw_input_event_type::mouse_down:
                {
                    const auto button = ie.mousedown.button;
                    const auto buttonIndex = static_cast<size_t>(button);
                    SMOL_ASSERT(button >= mouse_button::left && button < mouse_button::count);
                    const auto modifiers = ie.mousedown.modifiers;
                    const auto position = _mouse_state.current.position;

                    _mouse_state.mouse_buttons[buttonIndex].push_state(button_state::pressed, modifiers);
                    _mouse_state.mouse_buttons[buttonIndex].push_position(position);
                    _mouse_state.current.buttons[buttonIndex] = button_state::pressed;

                    input_event evt;
                    evt._type = input_event_type::mouse_down;
                    evt._mouse.position = position;
                    evt._modifiers = modifiers;
                    _input_events.push_back(evt);

                    break;
                }
                case raw_input_event_type::mouse_up:
                {
                    const auto button = ie.mouseup.button;
                    const auto buttonIndex = static_cast<size_t>(button);
                    SMOL_ASSERT(button >= mouse_button::left && button < mouse_button::count);
                    const auto modifiers = ie.mousedown.modifiers;
                    const auto position = _mouse_state.current.position;

                    _mouse_state.current.buttons[buttonIndex] = button_state::released;
                    auto& cd = _mouse_state.mouse_buttons[buttonIndex];

                    cd.push_state(button_state::released, modifiers);

                    if (input_event evt; cd.get_input_event(evt))
                    {
                        evt._mouse.button = button;
                        _input_events.push_back(evt);
                        if (evt.type() == input_event_type::mouse_triple_click) {
                            cd.reset();
                        }
                    }

                    // mouse down event
                    input_event evt;
                    evt._type = input_event_type::mouse_up;
                    evt._mouse.position = position;
                    evt._modifiers = modifiers;
                    _input_events.push_back(evt);

                    break;
                }
                case raw_input_event_type::scroll:
                {
                    _mouse_state.current.scroll += {ie.scroll.scroll_x, ie.scroll.scroll_y};
                    break;
                }
                default:
                    break;
            }
        }

        // if (_input_events.size())
        // {
        //     MX_LOG_INFO("--- BEGIN ---");
        //     for(auto& evt : _input_events)
        //     {
        //         MX_LOG_INFO("{}", evt);
        //     }
        //     MX_LOG_INFO("--- END ---");
        // }

        _raw_events.clear();
    }

    void input::update_cursor()
    {
        if (_mouse_state.current.cursor != _mouse_state.previous.cursor)
        {
            glfwSetCursor(g_window, _mouse_cursors[static_cast<size_t>(_mouse_state.current.cursor)]);
        }
    }

    void input::push_charpress(utf8proc_int32_t codepoint, modifier_keys modifiers)
    {
        // utf8_stringbuffer sb;
        // sb.push_back(codepoint);
        // MX_LOG_INFO("codepoint : {{ codepoint : 0x{:08x}, char : '{}', modifier_keys : {} }}", codepoint, sb.str(), modifiers);

        raw_input_event ie;
        ie.type = raw_input_event_type::char_press;
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

        raw_input_event ie;
        ie.type = raw_input_event_type::key_down;
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

        raw_input_event ie;
        ie.type = raw_input_event_type::key_repeat;
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

        raw_input_event ie;
        ie.type = raw_input_event_type::key_up;
        ie.keyup.keycode = keycode;
        ie.keyup.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    void input::push_mousemove(const vec2& pos)
    {
        // MX_LOG_INFO("mousemove : {{ position : {} }}", pos);
        raw_input_event ie;
        ie.type = raw_input_event_type::mouse_move;
        ie.mousemove.position = pos;
        _raw_events.push_back(ie);
    }

    void input::push_mousedown(mouse_button_t btn, modifier_keys modifiers)
    {
        // MX_LOG_INFO("mousedown : {{ button : {}, modifier_keys : {} }}", btn, modifiers);
        raw_input_event ie;
        ie.type = raw_input_event_type::mouse_down;
        ie.mousedown.button = btn;
        ie.mousedown.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    void input::push_mouseup(mouse_button_t btn, modifier_keys modifiers)
    {
        // MX_LOG_INFO("mouseup : {{ button : {}, modifier_keys : {} }}", btn, modifiers);
        raw_input_event ie;
        ie.type = raw_input_event_type::mouse_up;
        ie.mouseup.button = btn;
        ie.mouseup.modifiers = modifiers;
        _raw_events.push_back(ie);
    }

    void input::push_scroll(double scroll_x, double scroll_y)
    {
        // MX_LOG_INFO("scroll : {{ x : {}, y : {} }}", scroll_x, scroll_y);
        raw_input_event ie;
        ie.type = raw_input_event_type::scroll;
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
        SMOL_ASSERT(btn >= mouse_button::left && btn < mouse_button::count);
        auto& mouse_state = frame == input_frame::previous ? _mouse_state.previous : _mouse_state.current;
        return mouse_state.buttons[static_cast<size_t>(btn)];
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