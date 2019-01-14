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
                static_cast<T*>(this)->second_render();
                _ctx.pop_bounds();
            }

            void update_render_bounds()
            {
                bounds = _ctx.peek_bounds().eval(bounds);
            }

            rect bounds;
        protected:
            context& _ctx;
        };

        struct elem_group : public element<elem_group>
        {
            using element::element;

            void handle_input() {}
            void render() const {}
            void update_child_bounds();
            void second_render() const {}

            size_t _vertical_offset = 0;
        };

        struct elem_static_row : public element<elem_static_row>
        {
            using element::element;

            void handle_input() {}
            void render() const {}
            void update_child_bounds();
            void second_render() const {}

            std::vector<float> _widths;
            float _height = 0.0f;
            float _horizontal_offset = 0.0f;            
        };

        struct elem_debug : public element<elem_debug>
        {
            using element::element;

            void handle_input();
            void render() const;
            void update_child_bounds();
            void second_render() const {}

            callback<void, vec2> on_mouse_move;
            callback<void, vec2> on_mouse_over;
            NVGcolor bg_color = nvgRGB(0, 0xff, 0xff);
        };
    } // imgui namespace


#define MX_ELEMENT_CODEGEN(X, ...)\
    if (imgui::X group##__LINE__(imgui_ctx); \
        [&](auto& self) -> bool {\
            __VA_ARGS__\
            self.update_render_bounds();\
            self.handle_input();\
            self.render();\
            self.update_child_bounds();\
            return true;\
        }(group##__LINE__))

#define mxGroup(...) MX_ELEMENT_CODEGEN(elem_group, __VA_ARGS__)
#define mxStaticRow(...) MX_ELEMENT_CODEGEN(elem_static_row, __VA_ARGS__)
#define mxDebug(...) MX_ELEMENT_CODEGEN(elem_debug, __VA_ARGS__)

}