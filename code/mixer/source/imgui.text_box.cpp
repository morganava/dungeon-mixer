#include "imgui.hpp"
#include "input.hpp"

namespace dm
{
    namespace imgui
    {

        // Text Box

        void widget_text_box::middle_update_impl()
        {
            bool textDirty = false;
            auto& sb = this->data()->sb;
            auto& selectBegin = this->data()->selectBegin;
            auto& selectEnd = this->data()->selectEnd;
            auto& undoStack = this->data()->undoStack;
            auto& redoStack = this->data()->redoStack;

            const auto point_in_textarea = [&](vec2 position) -> bool
            {
                // return point_inside_rect
                return point_inside_rect(position, scissor()) &&
                       point_inside_rounded_rect(position,
                                                 _layout.border_inner_bounds(),
                                                 _layout.borderRadius - _layout.borderSize);
            };

            const auto update_action = [&](auto action) -> void
            {
                auto& previousAction = data()->previousAction;

                // handle undo
                if (action == text_box_action::undo)
                {
                    if (undoStack.size() > 0)
                    {
                        redoStack.push_back({sb.str(), selectBegin, selectEnd});
                        const auto& val = undoStack.back();
                        sb = val.text;
                        selectBegin = val.selectBegin;
                        selectEnd = val.selectEnd;
                        undoStack.pop_back();
                    }
                }
                // handle redo
                else if(action == text_box_action::redo)
                {
                    if (redoStack.size() > 0)
                    {
                        undoStack.push_back({sb.str(), selectBegin, selectEnd});
                        const auto& val = redoStack.back();
                        sb = val.text;
                        selectBegin = val.selectBegin;
                        selectEnd = val.selectEnd;
                        redoStack.pop_back();
                    }
                }
                // handle pushing state
                else
                {
                    bool pushState = false;
                    switch(action)
                    {
                        case text_box_action::write_whitespace:
                            pushState = previousAction != text_box_action::write_whitespace &&
                                        previousAction != text_box_action::write_nonwhitespace;
                            break;
                        case text_box_action::write_nonwhitespace:
                            pushState = previousAction != text_box_action::write_nonwhitespace;
                            break;
                        case text_box_action::erase_left:
                        case text_box_action::erase_right:
                            pushState = previousAction != action;
                            break;
                        default:
                            pushState = true;
                            break;
                    }

                    if (pushState)
                    {
                        undoStack.push_back({sb.str(), selectBegin, selectEnd});
                    }
                }
                previousAction = action;
            };

            for (auto evt : g_input.get_input_events())
            {
                // set to true when the string stored in the string buffer
                // is modified
                bool textChanged = false;
                const bool textSelected = (selectBegin != selectEnd);
                const auto from = std::min(selectBegin, selectEnd);
                const auto to = std::max(selectBegin, selectEnd);

                switch(evt.type())
                {
                    case input_event_type::mouse_drag_begin:
                    {
                        if (point_in_textarea(evt.position()))
                        {
                            this->focus_widget();
                            const auto caretPosition = g_nanovg.get_caret_position(
                                this->_layout.inner_bounds(),
                                this->data()->textOffset,
                                this->style().fontFamily,
                                this->style().fontSize,
                                this->style().textHalign,
                                this->style().textValign,
                                data()->text,
                                evt.position());

                            selectBegin = selectEnd = caretPosition;
                        }
                        else
                        {
                            this->unfocus_widget();
                        }
                        break;
                    }
                    case input_event_type::mouse_drag:
                    {
                        if (this->is_focused())
                        {
                            const auto caretPosition = g_nanovg.get_caret_position(
                                this->_layout.inner_bounds(),
                                this->data()->textOffset,
                                this->style().fontFamily,
                                this->style().fontSize,
                                this->style().textHalign,
                                this->style().textValign,
                                data()->text,
                                evt.position());
                            selectEnd = caretPosition;
                        }
                        break;
                    }
                    case input_event_type::mouse_single_click:
                    {
                        if (point_in_textarea(evt.position()))
                        {
                            this->focus_widget();
                            const auto caretPosition = g_nanovg.get_caret_position(
                                this->_layout.inner_bounds(),
                                this->data()->textOffset,
                                this->style().fontFamily,
                                this->style().fontSize,
                                this->style().textHalign,
                                this->style().textValign,
                                data()->text,
                                evt.position());

                            if (evt.modifiers().shift)
                            {
                                selectEnd = caretPosition;
                            }
                            else
                            {
                                selectBegin = selectEnd = caretPosition;
                            }
                        }
                        else
                        {
                            this->unfocus_widget();
                        }
                        break;
                    }
                    case input_event_type::mouse_double_click:
                    {
                        if (point_in_textarea(evt.position()))
                        {
                            this->focus_widget();
                            const auto caretPosition = g_nanovg.get_caret_position(
                                this->_layout.inner_bounds(),
                                this->data()->textOffset,
                                this->style().fontFamily,
                                this->style().fontSize,
                                this->style().textHalign,
                                this->style().textValign,
                                data()->text,
                                evt.position());

                            selectBegin = sb.find_word_begin(sb.begin() + caretPosition, false) - sb.begin();
                            selectEnd = sb.find_word_end(sb.begin() + caretPosition, false) - sb.begin();
                        }
                        else
                        {
                            this->unfocus_widget();
                        }
                        break;
                    }
                    case input_event_type::mouse_triple_click:
                    {
                        if (point_in_textarea(evt.position()))
                        {
                            this->focus_widget();
                            selectBegin = 0;
                            selectEnd = (int32_t)(sb.end() - sb.begin());
                        }
                        else
                        {
                            this->unfocus_widget();
                        }
                        break;
                    }
                    case input_event_type::key_down:
                    case input_event_type::key_repeat:
                    {
                        if (!this->is_focused()) break;

                        switch(evt.keycode())
                        {
                            //
                            // Navigation
                            //
                            case GLFW_KEY_LEFT:
                                // skip to begining of current word
                                if (evt.modifiers().control)
                                {
                                    auto it = sb.find_word_begin(sb.begin() + selectEnd, true);
                                    selectEnd = (it - sb.begin());
                                    if (!evt.modifiers().shift)
                                    {
                                        selectBegin = selectEnd;
                                    }
                                }
                                else
                                {
                                    if (textSelected)
                                    {
                                        if (evt.modifiers().shift)
                                        {
                                            selectEnd  = std::max(selectEnd - 1, size_t(0));
                                        }
                                        else
                                        {
                                            selectBegin = std::min(selectBegin, selectEnd);
                                            selectEnd = selectBegin;
                                        }
                                    }
                                    else if (selectEnd > 0)
                                    {
                                        // move left 1
                                        selectEnd = std::max(selectEnd - 1, size_t(0));
                                        if (!evt.modifiers().shift)
                                        {
                                            selectBegin = selectEnd;
                                        }
                                    }
                                }
                                break;
                            case GLFW_KEY_RIGHT:
                                // skip to end of current word
                                if (evt.modifiers().control)
                                {
                                    auto it = sb.find_word_end(sb.begin() + selectEnd, true);
                                    selectEnd = (it - sb.begin());
                                    if (!evt.modifiers().shift)
                                    {
                                        selectBegin = selectEnd;
                                    }
                                }
                                else
                                {
                                    if (textSelected)
                                    {
                                        if (evt.modifiers().shift)
                                        {
                                            selectEnd = std::min(selectEnd + 1, sb.size());
                                        }
                                        else
                                        {
                                            selectBegin = std::max(selectBegin, selectEnd);
                                            selectEnd = selectBegin;
                                        }
                                    }
                                    else if (selectEnd < sb.size())
                                    {
                                        // move right 1
                                        selectEnd = std::min(selectEnd + 1, sb.size());
                                        if (!evt.modifiers().shift)
                                        {
                                            selectBegin = selectEnd;
                                        }
                                    }
                                }
                                break;
                            case GLFW_KEY_UP:
                            case GLFW_KEY_HOME:
                                // begin of line
                                selectEnd = 0;
                                if (!evt.modifiers().shift)
                                {
                                    selectBegin = selectEnd;
                                }
                                break;
                            case GLFW_KEY_DOWN:
                            case GLFW_KEY_END:
                                // end of line
                                selectEnd = sb.size();
                                if (!evt.modifiers().shift)
                                {
                                    selectBegin = selectEnd;
                                }
                                break;
                            //
                            // Deletion
                            //
                            case GLFW_KEY_DELETE:
                                // delete selected text
                                if (textSelected)
                                {
                                    update_action(text_box_action::erase_block);
                                    textDirty = true;
                                    textChanged = true;
                                    sb.erase(sb.begin() + from, sb.begin() + to);
                                    selectBegin = selectEnd = from;
                                }
                                // delete next word
                                else if (evt.modifiers().control && selectEnd < sb.size())
                                {
                                    update_action(text_box_action::erase_block);
                                    auto it = sb.find_word_end(sb.begin() + selectEnd, true);
                                    sb.erase(sb.begin() + selectEnd, it);
                                    selectBegin = selectEnd;
                                    textDirty = true;
                                    textChanged = true;
                                }
                                else if (selectEnd < sb.size())
                                {
                                    update_action(text_box_action::erase_right);
                                    textDirty = true;
                                    textChanged = true;
                                    sb.erase(sb.begin() + selectEnd);
                                }
                                break;
                            case GLFW_KEY_BACKSPACE:
                                // delete selected text
                                if (textSelected)
                                {
                                    update_action(text_box_action::erase_block);
                                    textDirty = true;
                                    textChanged = true;
                                    sb.erase(sb.begin() + from, sb.begin() + to);
                                    selectBegin = selectEnd = from;
                                }
                                // delete previous word
                                else if (evt.modifiers().control && selectEnd > 0)
                                {
                                    update_action(text_box_action::erase_block);
                                    auto it = sb.find_word_begin(sb.begin() + selectBegin, true);
                                    sb.erase(it, sb.begin() + selectEnd);
                                    selectBegin = selectEnd = (it - sb.begin());
                                    textDirty = true;
                                    textChanged = true;
                                }
                                // backspace from caret
                                else if (selectEnd > 0)
                                {
                                    update_action(text_box_action::erase_left);
                                    sb.erase(sb.begin() + selectEnd - 1);
                                    selectBegin = selectEnd = selectEnd - 1;
                                    textDirty = true;
                                    textChanged = true;
                                }
                                break;
                            //
                            // Submit
                            //
                            case GLFW_KEY_ENTER:
                            case GLFW_KEY_KP_ENTER:
                                this->on_submit(sb.str());
                                break;
                            //
                            // Keyboard Shortcuts
                            //
                            // Select All
                            case GLFW_KEY_A:
                                if (evt.modifiers().control)
                                {
                                    selectBegin = 0;
                                    selectEnd = sb.size();
                                }
                                break;
                            // Cut
                            case GLFW_KEY_X:
                                if (evt.modifiers().control)
                                {
                                    if (from < to) {
                                        update_action(text_box_action::erase_block);
                                        const auto str = sb.str(sb.begin() + from, sb.begin() + to);
                                        g_input.set_clipboard_string(str);
                                        sb.erase(sb.begin() + from, sb.begin() + to);

                                        selectEnd = selectBegin = from;
                                        textDirty = true;
                                        textChanged = true;
                                    }
                                }
                                break;
                            // Copy
                            case GLFW_KEY_C:
                                if (evt.modifiers().control)
                                {
                                    if (from < to) {
                                        const auto str = sb.str(sb.begin() + from, sb.begin() + to);
                                        g_input.set_clipboard_string(str);
                                    }
                                }
                                break;
                            // Paste
                            case GLFW_KEY_V:
                                if (evt.modifiers().control)
                                {
                                    const auto clipboardString = g_input.get_clipboard_string();

                                    if (from < to || clipboardString.size() > 0)
                                    {
                                        update_action(text_box_action::insert_block);
                                        // remove selected text
                                        sb.erase(sb.begin() + from, sb.begin() + to);
                                        // paste into sb
                                        auto initSize = sb.size();
                                        sb.insert(sb.begin() + from, clipboardString);
                                        auto finalSize = sb.size();

                                        selectEnd = selectBegin = from + (int32_t)(finalSize - initSize);
                                        textDirty = true;
                                        textChanged = true;
                                    }
                                }
                                break;
                            // Undo/Redo
                            case GLFW_KEY_Z:
                                if (evt.modifiers().control)
                                {
                                    // redo
                                    if (evt.modifiers().shift)
                                    {
                                        update_action(text_box_action::redo);
                                        textDirty = true;
                                        textChanged = true;
                                    }
                                    // undo
                                    else
                                    {
                                        update_action(text_box_action::undo);
                                        textDirty = true;
                                        textChanged = true;
                                    }
                                }
                                break;
                        }
                        break;
                    }
                    case input_event_type::char_press:
                    {
                        if (!this->is_focused()) break;
                        // ignore modified char press
                        if (evt.modifiers().control || evt.modifiers().alt) break;

                        textDirty = true;
                        textChanged = true;
                        if (textSelected)
                        {
                            update_action(text_box_action::erase_block);
                            sb.erase(sb.begin() + from, sb.begin() + to);
                            selectBegin = selectEnd = from;
                        }

                        if (evt.codepoint().is_whitespace())
                        {
                            update_action(text_box_action::write_whitespace);
                        }
                        else
                        {
                            update_action(text_box_action::write_nonwhitespace);
                        }

                        sb.insert(sb.begin() + from, evt.codepoint());
                        selectBegin = selectEnd = (from + 1);
                        break;
                    }
                    default:
                        break;
                }

                // if we have an on_change event, then push the current state of the
                // string buffer to our callback
                if (textChanged && this->on_change)
                {
                    this->on_change(sb.str());
                }
            }

            // re-export text from buffer
            if(textDirty)
            {
                this->data()->text = sb.str();
            }

            // update text offset
            if (g_input.get_input_events().size() > 0)
            {
                const auto innerBounds = this->_layout.inner_bounds();
                auto& textOffset = this->data()->textOffset;
                const auto caretOffset = g_nanovg.get_caret_offset(
                        this->_layout.inner_bounds(),
                        this->style().fontFamily,
                        this->style().fontSize,
                        this->style().textHalign,
                        this->style().textValign,
                        data()->text,
                        selectEnd);

                if (caretOffset >= textOffset && caretOffset <= (textOffset + innerBounds.width()))
                {
                    // do nothing text offset is fine
                }
                else if (caretOffset < textOffset)
                {
                    // caret to left of view window
                    textOffset = caretOffset;
                }
                else
                {
                    // caret to right of view window
                    textOffset = caretOffset - innerBounds.width();
                }
                textOffset = std::max(0.0, textOffset);
            }

            if (point_in_textarea(g_input.get_mouse_position(input_frame::current)))
            {
                g_input.set_mouse_cursor(mouse_cursor::ibeam);
            }
        }

        void widget_text_box::first_render_impl() const
        {
            g_context.push_scissor(this->_layout.border_inner_bounds());
            if (this->is_focused())
            {
                if (this->data()->selectBegin == this->data()->selectEnd)
                {
                    // placeholder
                    if (this->data()->text.size() == 0) {
                        g_nanovg.draw_text_with_caret(
                            this->_layout.inner_bounds(),
                            this->data()->textOffset,
                            this->style().fontFamily,
                            this->style().fontSize,
                            this->style().placeholderFontColor,
                            this->style().textHalign,
                            this->style().textValign,
                            this->data()->placeholderText,
                            this->data()->selectBegin,
                            2.0f,
                            this->style().fontColor);
                    } else {
                        g_nanovg.draw_text_with_caret(
                            this->_layout.inner_bounds(),
                            this->data()->textOffset,
                            this->style().fontFamily,
                            this->style().fontSize,
                            this->style().fontColor,
                            this->style().textHalign,
                            this->style().textValign,
                            this->data()->text,
                            this->data()->selectBegin,
                            2.0f,
                            this->style().fontColor);
                    }
                }
                else
                {
                    g_nanovg.draw_text_with_selection(
                        this->_layout.inner_bounds(),
                        this->data()->textOffset,
                        this->style().fontFamily,
                        this->style().fontSize,
                        this->style().fontColor,
                        this->style().textHalign,
                        this->style().textValign,
                        this->data()->text,
                        std::min(this->data()->selectBegin, this->data()->selectEnd),
                        std::max(this->data()->selectBegin, this->data()->selectEnd),
                        this->style().selectedFontColor,
                        this->style().selectionColor);
                }
            }
            else
            {
                // placeholder text
                if (this->data()->text.size() == 0) {
                    g_nanovg.draw_text(
                        this->_layout.inner_bounds(),
                        this->data()->textOffset,
                        this->style().fontFamily,
                        this->style().fontSize,
                        this->style().placeholderFontColor,
                        this->style().textHalign,
                        this->style().textValign,
                        this->data()->placeholderText);
                } else
                {
                    g_nanovg.draw_text(
                        this->_layout.inner_bounds(),
                        this->data()->textOffset,
                        this->style().fontFamily,
                        this->style().fontSize,
                        this->style().fontColor,
                        this->style().textHalign,
                        this->style().textValign,
                        this->data()->text);
                }


            }
            g_context.pop_scissor();
        }

        const utf8_string& widget_text_box::text() const
        {
            return this->data()->text;
        }
    }
}
