#include "ui.hpp"
#include "utilities.hpp"

namespace tabletopify
{
    static ui* g_ui = nullptr;

    ui* get_ui()
    {
        MX_ASSERT(g_ui != nullptr);
        return g_ui;
    }

    void init_ui()
    {
        MX_ASSERT(g_ui == nullptr);
        g_ui = new ui();
        g_ui->initialize();
    }
    void free_ui()
    {
        g_ui->finish();
        delete g_ui;
        g_ui = nullptr;
    }

    void ui::initialize()
    {
        style_t rootStyle;
        rootStyle.x = 0.0f;
        rootStyle.y = 0.0f;
        rootStyle.width = 0.0f;
        rootStyle.height = 0.0f;
        _style_stack.push_back(rootStyle);
    }

    void ui::finish()
    {

    }

    // imgui methods

    void ui::push_style(const style_t& s)
    {
        _style_stack.push_back(s);
    }

    void ui::pop_style()
    {
        _style_stack.pop_back();
    }

    style_t& ui::peek_style()
    {
        return _style_stack.back();
    }

    namespace imgui
    {
        // low cost calbacks for use with immediate mode ui
        template<typename Return, typename... Args>
        class callback
        {
            typedef Return(*function_t)(void*, Args...) ;        
            typedef void(*destroy_t)(void*);    
        public:
            callback() = default;
            callback(const callback&) = delete;
            callback(callback&& that)
            : _function(that._function)
            , _destroy(that._destroy)
            , _data(that._data)
            {
                that._function = nullptr;
                that._destroy = nullptr;
                that._data = nullptr;
            }

            ~callback()
            {
                if(_destroy)
                {
                    _destroy(_data);
                }
            }

            callback& operator=(callback&& that)
            {
                _function = that._function;
                _destroy = that._destroy;
                _data = that._data;

                that._function = nullptr;
                that._destroy = nullptr;
                that._data = nullptr;
            }

            template<typename T>
            callback& operator=(T&& lam)
            {
                if (_destroy != nullptr)
                {
                    _destroy(_data);
                }

                const function_t function = [](void* plam, Args... args) -> Return 
                {
                    return (*reinterpret_cast<T*>(plam))(args...);
                };
                const destroy_t destroy = [](void* plam)
                {
                    reinterpret_cast<T*>(plam)->~T();
                };

                static uint8_t slam[sizeof(T)];
                new (slam) T(std::forward<T>(lam));
                void* data = slam;

                this->_function = function;
                this->_destroy = destroy;
                this->_data = data;

                return *this;
            }

            Return operator()(Args... args) const
            {
                if(_function) {
                    _function(_data, args...);
                }
            }

            operator bool() const
            {
                return _function != nullptr;
            }
        private:
            function_t _function = nullptr;
            destroy_t _destroy = nullptr;
            void* _data =  nullptr; 
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

        template<typename T>
        struct base_element
        {
            base_element() = delete;

            template<typename Func>
            base_element(Func&& func)
            {
                style_t style = func(*this);
                g_ui->push_style(func());
            }

            ~base_element()
            {
                g_ui->pop_style();
            }

            void render()
            {
                T::render_impl(g_ui->peek_style());
            };
        };

        struct circle_element
        {
            circle_element() = delete;
            circle_element(auto&& func)
            {
                style_t s = func(*this);
                g_ui->push_style(s);
            }

            ~circle_element()
            {
                g_ui->pop_style();
            }

            void render(NVGcontext* ctx, const style_t& style)
            {
                nvgBeginPath(ctx);
                nvgCircle(ctx, style.x, style.y, std::min(style.width, style.height));
                nvgFillColor(ctx, style.fill_color);
                nvgFill(ctx);
            }
        };
    } // imgui namespace

    #define mxCircle(...)\
    if(imgui::circle_element __circle ## __LINE__([=](auto& self) {\
        style_t style = g_ui->peek_style();\
        __VA_ARGS__\
        self.render(ctx, style);\
        return style;\
    }); true)

    void ui::update(NVGcontext* ctx, float dt, int32_t width, int32_t height)
    {
        {
            auto& rootStyle =_style_stack.front();
            rootStyle.width = (float)width;
            rootStyle.height = (float)height;
        }

        // imgui::circle_element ce([=](auto& self)
        // {
        //     style_t style = g_ui->peek_style();
        //     style.x = width / 2.0f;
        //     style.y = height / 2.0f;
        //     style.width = width;
        //     style.height = height;
        //     style.fill_color = nvgRGB(255, 128, 0);
        //     self.render(ctx, style);
        //     return style;
        // });

        mxCircle(
            style.x = width/2.0f; 
            style.y = height/2.0f; 
            style.width = width/3.0f;
            style.height = height/3.0f;
            style.fill_color = nvgRGB(255, 255, 0););
        mxCircle(
            style.x = width/2.0f;
            style.y = height/2.0f;
            style.width = width/4.0f;
            style.height = height/4.0f;
            style.fill_color = nvgRGB(16,16, 16);
        );
#if 0        
        MX_LOG_INFO("location: {}, dt: {}, width: {}, height: {}", __PRETTY_FUNCTION__, dt, width, height);
        // NVGcontext

        nvgBeginPath(ctx);
        nvgCircle(ctx, width / 2.0f, height / 2.0f, std::min(width, height) / 3.0f);
        nvgFillColor(ctx, nvgRGBA(255, 196, 0, 255));
        nvgFill(ctx);
#endif

#if 0
        mxGroup(style.position = {0,0}; style.size = {width, height}; style.fill = 0xFFFFFF;);
        {
            mxColumn(style.position = {0,0}; style.size = {width, height};)
            {
                mxStaticRow(style.size = {width, 100}; style.layout = {50, 100, 100};)
                {
                    mxLabel("Hello!");
                    mxSpacer();
                    mxLabel("World!");
                }
                mxDynamicRow(style.size = {width, 100} style.children = 2;)
                {
                    mxLabel("Left");
                    mxLabel("Right");
                }
            }
        }
#endif        
    }
}
