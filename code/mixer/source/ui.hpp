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

    class ui
    {
    public:
        void update(NVGcontext* ctx, float dt, int32_t width, int32_t height);
        void initialize();
        void finish();
    private:        
        NVGcontext* _context = nullptr;
    };

    ui* get_ui();
    void init_ui();
    void free_ui();    
}