#pragma once

#include "color.hpp"
#include "array.hpp"

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

        struct bounds_callback
        {
            constexpr rect eval(const rect& desired) const
            {
                return callback(widget, desired);
            }

            rect (*callback)(void*, const rect&) = [](void*, const rect&) -> rect {MX_ASSERT(!"Should Not Be Called"); return {};};
            void* widget = nullptr;
        };

        typedef rect (*render_bounds_callback_t)(void*, const rect&);

        struct render_bounds
        {
            render_bounds_callback_t callback = nullptr;
            void* callback_data = nullptr;
            rect scissor = {};

            rect eval(const rect& desired) const;
        };

        // widget id        

        typedef size_t widget_id_t;
        constexpr widget_id_t NULL_WIDGET_ID = 0;
        constexpr widget_id_t INVALID_WIDGET_ID = ~NULL_WIDGET_ID;
        widget_id_t next_widget_id();

        // style

        struct stylesheet
        {
            font_id_t font;
            color font_color;
            struct
            {
                NVGcolor color;
            } body;
            struct
            {
                double scrollbar_width = 20.0;
                double min_scrollbar_height = 150.0;            
                double scroll_step = 30.0;
            } vbox;
            struct
            {
                font_id_t font_face;
                color font_color;
                double font_size;
                color background_color;
                color background_hover_color;
                color background_active_color;
            } button;
        };

        // context

        class context
        {
        public:
            void update(double dt);

            // render bound tree
            void push_render_bounds(const render_bounds&);
            const render_bounds& peek_render_bounds() const;
            void pop_render_bounds();

            // widget focus methods
            constexpr void focus_widget(widget_id_t id) { _focused_widget = id; }
            constexpr void unfocus_widget(widget_id_t id) { if (is_focused(id)) { focus_widget(INVALID_WIDGET_ID); } }
            constexpr bool is_focused(widget_id_t id)  const { return id == _focused_widget; }

            // set's render scissor
            void set_render_scissor(const rect&);

            stylesheet style;
        private:
            std::vector<render_bounds> _render_bounds_stack;
            widget_id_t _focused_widget = INVALID_WIDGET_ID;
        };

        inline void init_context()
        {
            singleton<context>::construct();
        }
        constexpr void free_context()
        {
            singleton<context>::free();
        }
        inline constexpr context& g_context = singleton<context>::get();
        inline constexpr stylesheet& g_style = g_context.style;

        struct widget_data
        {
            widget_id_t id = next_widget_id();
        };

        // base widget

        struct widget_base
        {
        public:
            widget_base(widget_data* data);

            constexpr widget_id_t get_widget_id() const
            {
                return _data ? _data->id : NULL_WIDGET_ID;
            }

        protected:
            void focus_widget();
            void unfocus_widget();
            bool is_focused() const;

            // called by widget<T> in destruct and calc_render_bounds
            void destructor_impl(void(*func)(void*));
            void calc_render_bounds_impl(render_bounds(*func)(void*));

        public:
            rect bounds = {};
            rect scissor = {};
        protected:
            widget_data* _data = nullptr;            
        };

        template<typename T, typename WIDGET_DATA=widget_data>
        struct widget : public widget_base
        {
            widget() = delete;
            widget(const widget&) = delete;
            widget(widget&&) = delete;

            constexpr widget(widget_data* pdata)
            : widget_base(pdata)
            { }

            // destructor called after child widgets have been handled
            ~widget()
            {
                destructor_impl([](void* pSelf)
                {
                    auto* self = static_cast<T*>(pSelf);
                    // first update and render after child renders
                    self->middle();
                    self->second_render();
                    // our final update
                    self->end();                    
                });
            }

            // gets and saves this widget's render bounds and scissor area
            // and pushes callback and current scissor for child widgets
            void calc_render_bounds()
            {
                calc_render_bounds_impl([](void* pSelf) -> render_bounds
                {
                    auto* self = static_cast<T*>(pSelf);
                    return {self->get_bounds_callback(), self, self->scissor};
                });
            }

            // widget methods

            constexpr WIDGET_DATA* data()
            {
                return reinterpret_cast<WIDGET_DATA*>(this->_data);
            }

            constexpr const WIDGET_DATA* data() const
            {
                return reinterpret_cast<const WIDGET_DATA*>(this->_data);
            }
        };

        // grouping widgets

        struct vbox_data : widget_data
        {
            // vertical offset down 
            double vertical_scroll = 0.0;
            // is user grabbng scrollbar with mouse
            bool grabbing_scrollbar = false;
            // offset into scrollbar handle user is grabbng
            double grab_offset_y = 0.0;
        };

        struct widget_vbox : public widget<widget_vbox, vbox_data>
        {
            using widget::widget;

            void begin();
            void first_render() const {}
            render_bounds_callback_t get_bounds_callback();
            void middle();
            void second_render() const;
            void end();

        private:
            // cumulative height of the child widgets
            double internal_height = 0.0;
            // area of entire scrollbar widget
            rect scrollbar_area = {};
            // grab area of scrollbar this frame
            rect scrollbar_grab_area = {};
        };

        struct widget_hbox : public widget<widget_hbox>
        {
            using widget::widget;

            void begin() {}
            void first_render() const {}
            render_bounds_callback_t get_bounds_callback();
            void middle() {}
            void second_render() const {}
            void end() {}

            array_view<double> widths;
        private:
            size_t _index = 0;
            float _horizontal_offset = 0.0f;            
        };

        // debug renderer

        struct widget_debug : public widget<widget_debug>
        {
            using widget::widget;

            void begin(); 
            void first_render() const;
            render_bounds_callback_t get_bounds_callback() { return nullptr; }
            void middle() {}
            void second_render() const {}
            void end() {}

            callback<void, vec2> on_mouse_move;
            callback<void, vec2> on_mouse_over;
            NVGcolor bg_color = nvgRGB(0, 0xff, 0xff);
        };

        // label

        struct label_data : widget_data
        {
            utf8_string text;
            font_id_t font = 0;
            float text_size = 12.0f;
            color text_color = color::rgb(0,0,0);
            halign_t text_halign = halign::right;
            valign_t text_valign = valign::top;
        };

        struct widget_label : public widget<widget_label, label_data>
        {
            using widget::widget;
            
            void begin() {}
            void first_render() const;
            render_bounds_callback_t get_bounds_callback() { return nullptr; }
            void middle() {}
            void second_render() const {}
            void end() {}            
        };

        /// button

        struct button_data : widget_data
        {
            utf8_string text;
        };

        struct widget_button : public widget<widget_button, button_data>
        {
            using widget::widget;

            void begin();
            void first_render() const;
            render_bounds_callback_t get_bounds_callback() { return nullptr; }
            void middle() {}
            void second_render() const {}
            void end();

            callback<void> on_click;
        };

    } // imgui namespace


#define MX_WIDGET_CODEGEN(X, pdata, ...)\
    if (imgui::X widget##__LINE__(pdata); \
        [&](auto& self) -> bool {\
            __VA_ARGS__\
            self.calc_render_bounds();\
            self.begin();\
            self.first_render();\
            return true;\
        }(widget##__LINE__))

#define mxVbox(data, ...) MX_WIDGET_CODEGEN(widget_vbox, data, __VA_ARGS__)
#define mxHbox(...) MX_WIDGET_CODEGEN(widget_hbox, nullptr, __VA_ARGS__)
#define mxDebug(...) MX_WIDGET_CODEGEN(widget_debug, nullptr, __VA_ARGS__)
#define mxLabel(data, ...) MX_WIDGET_CODEGEN(widget_label, data, __VA_ARGS__)
#define mxButton(data, ...) MX_WIDGET_CODEGEN(widget_button, data, __VA_ARGS__)

}