#pragma once
namespace tabletopify
{
    typedef struct
    {
        float x;
        float y;
        float width;
        float height;
        NVGcolor fill_color;
    } style_t;

#if 0
    struct elem
    {
        elem() = delete;

        template<typename T>
        elem(T&& func) : elem()
        {
            func(*this);
            push_position(x, y);
        }

        ~elem()
        {
            pop_position();
        }
        
        void render(auto& on_click, auto& on_mouse_over) const
        {
            if ((on_click || on_mouse_over) && mouse_over()) 
            {
                on_mouse_over(this->x, this->y);
                if (on_click && mouse_clicked()) on_click(this->x,this->y);
            }

            render_elem(x, y, width, height);
        }
    };

    #define ELEMENT(...)    if(elem __e ## __LINE__([](auto& self) {\
                                style_t style;\
                                auto on_click = make_callback<void(int,int)>();\
                                auto on_mouse_over = make_callback<void(int,int)>();\
                                __VA_ARGS__\
                                self.render(on_click, on_mouse_over);\
                            }); true)
#endif


    class ui
    {
    public:
        void update(NVGcontext* ctx, float dt, int32_t width, int32_t height);
        void initialize();
        void finish();

        // imgui methods
        void push_style(const style_t&);
        void pop_style();
        style_t& peek_style();
    private:        

        NVGcontext* _context = nullptr;
        std::vector<style_t> _style_stack;
    };

    ui* get_ui();
    void init_ui();
    void free_ui();    
}