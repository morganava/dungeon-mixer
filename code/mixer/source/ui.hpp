#pragma once
namespace tabletopify
{
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