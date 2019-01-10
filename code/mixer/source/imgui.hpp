#pragma once

namespace tabletopify
{
    // immmediate mode gui
    namespace imgui
    {
        template<typename RETVAL, typename... ARGS>
        class callback
        {
        public:
            ~callback()
            {
                delete_func();
            }

            constexpr callback& operator=(RETVAL(*function)(ARGS...))
            {
                exec_func = function;
                delete_func = [](){};
                return *this;
            }

            template<typename FUNCTOR>
            callback& operator=(FUNCTOR&& functor)
            {
                singleton<FUNCTOR>::construct(std::move(functor));
                set_callbacks<FUNCTOR>();
                return *this;
            }

            constexpr RETVAL operator()(ARGS... args)
            {
                return exec_func(std::forward<ARGS>(args)...);
            }
        private:

            template<typename FUNCTOR>
            constexpr void set_callbacks()
            {
                delete_func = []() -> void
                {
                    singleton<FUNCTOR>::free();
                };

                exec_func = [](ARGS... args) -> RETVAL
                {
                    return singleton<FUNCTOR>::get()(args...);
                };
            }

            void(*delete_func)() = [](){};
            RETVAL(*exec_func)(ARGS...) = [](ARGS...) -> RETVAL {return RETVAL();};
        };

        template<typename Return, typename... Args>
        auto make_callback_impl(Return(*)(Args...))
        {
            return callback<Return,Args...>();
        }

        template<typename Func>
        auto make_callback()
        {
            Func* func = nullptr;
            return make_callback_impl(func);
        }

        template<typename T, size_t LEN>
        struct fixed_stack
        {
            constexpr void push(T val)
            {
                _buffer[_idx++] = val;
            }

            constexpr T peek() const
            {
                assert(_idx > 0);
                return _buffer[_idx - 1];
            }

            constexpr void pop()
            {
                assert(_idx > 0);
                --_idx;
            }

            constexpr size_t size() const
            {
                return _idx;
            }

        private:
            size_t _idx = 0;
            T _buffer[LEN] = {};
        };

        struct bounds_callback
        {
            constexpr rect eval(const rect& desired) const
            {
                return callback(element, desired);
            }

            rect (*callback)(void*, const rect&) = [](void*, const rect&) -> rect {MX_ASSERT(!"Should Not Be Called"); return {};};
            void* element = nullptr;
        };

        class context
        {
        public:
            ~context()
            {
                if (_bounds_stack.size() != 0) 
                {
                       MX_LOG_INFO("bounds.size(): {}", _bounds_stack.size());
                }
                MX_ASSERT(_bounds_stack.size() == 0);
            }

            constexpr void push_bounds(const bounds_callback& bounds)
            {
                _bounds_stack.push(bounds);
            }

            constexpr bounds_callback peek_bounds() const
            {
                auto retval = _bounds_stack.peek();
                return retval;
            }

            constexpr void pop_bounds()
            {
                _bounds_stack.pop();
            }
        private:
            fixed_stack<bounds_callback, 16> _bounds_stack;
        };

        // base element

        template<typename T>
        struct element
        {
            element() = delete;
            element(const element&) = delete;
            element(element&&) = delete;
            constexpr element(context& ctx)
            : _ctx(ctx)
            { }

            ~element()
            {
                _ctx.pop_bounds();
            }

            void update_render_bounds()
            {
                _bounds = _ctx.peek_bounds().eval(_bounds);
            }

            void render()
            {
                static_cast<T*>(this)->render(_bounds);
            }

            context& _ctx;
            rect _bounds;
        };

        struct elem_group : public element<elem_group>
        {
            using element::element;
            using element::render;

            void pre_render()
            {

            }

            void render(const rect& bounds)
            {

            }

            void post_render()
            {
                _ctx.push_bounds(
                {
                    [](void* ptr, const rect& desired) -> rect
                    {
                        elem_group* eg = reinterpret_cast<elem_group*>(ptr);
                        rect retval = 
                        {
                            {eg->_bounds.position.x, eg->_bounds.position.y + eg->_vertical_offset},
                            {eg->_bounds.size.x, desired.size.y}
                        };
                        eg->_vertical_offset += desired.size.y;
                        return retval;
                    },
                    this
                });
            }

            size_t _vertical_offset = 0;
        };

        struct elem_static_row : public element<elem_static_row>
        {
            using element::element;
            using element::render;

            void pre_render()
            {

            }

            void render(const rect& bounds)
            {
                // MX_LOG_INFO("RENDERING STATIC_ROW : position {},{} size {},{}", bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y);
            }

            void post_render()
            {
                _ctx.push_bounds(
                {
                    [](void* ptr, const rect&) -> rect
                    {
                        elem_static_row* esr = static_cast<elem_static_row*>(ptr);
                        float w = esr->_widths.front();
                        esr->_widths.erase(esr->_widths.begin());

                        rect retval = 
                        {
                            {esr->_bounds.position.x + esr->_horizontal_offset, esr->_bounds.position.y},
                            {w, esr->_bounds.size.y}
                        };

                        esr->_horizontal_offset += w;

                        return retval;
                    },
                    this
                });
            }

            std::vector<float> _widths;
            float _height = 0.0f;
            float _horizontal_offset = 0.0f;            
        };

        struct elem_debug : public element<elem_debug>
        {
            using element::element;
            using element::render;

            void pre_render()
            {
                auto* input = get_input();
                auto current_position = input->get_mouse_position(false);
                auto previous_position = input->get_mouse_position(true);
                if (point_inside_rect(current_position, _bounds))
                {
                    on_mouse_over(current_position);

                    if (current_position != previous_position) 
                    {
                        on_mouse_move(current_position);
                    }
                }
            }

            void render(const rect& bounds)
            {
                // nvgBeginPath(ctx);
                // nvgRect(ctx, bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y);
                // nvgFillColor(ctx, nvgRGB(0, 0, 0));
                // nvgFill(ctx);

                get_nanovg()->fill_rect(bounds, nvgRGB(0,0,0));
                get_nanovg()->fill_rect(rect{bounds.position + vec2{1.0, 1.0}, {bounds.size - vec2{2.0, 2.0}}}, bg_color);

                // nvgBeginPath(ctx);
                // nvgRect(ctx, bounds.position.x + 1.0f, bounds.position.y + 1.0f, bounds.size.x - 2.0f, bounds.size.y - 2.0f);
                // nvgFillColor(ctx, bg_color);
                // nvgFill(ctx);
            }

            void post_render()
            {
                _ctx.push_bounds({});
            }

            callback<void, vec2> on_mouse_move;
            callback<void, vec2> on_mouse_over;
            NVGcolor bg_color = nvgRGB(0, 0xff, 0xff);
        };
    } // imgui namespace


#define MX_ELEMENT_CODEGEN(X, ...)\
    if (imgui::X group##__LINE__(imgui_ctx); \
        [&](auto& self) -> bool {\
            __VA_ARGS__\
            self.update_render_bounds(); self.pre_render(); self.render(); self.post_render();\
            return true;\
        }(group##__LINE__))

#define mxGroup(...) MX_ELEMENT_CODEGEN(elem_group, __VA_ARGS__)
#define mxStaticRow(...) MX_ELEMENT_CODEGEN(elem_static_row, __VA_ARGS__)
#define mxDebug(...) MX_ELEMENT_CODEGEN(elem_debug, __VA_ARGS__)

}