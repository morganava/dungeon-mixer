#pragma once
namespace tabletopify
{
    class ui
    {
    public:
        void update(float dt, int32_t width, int32_t height);
        void initialize();
        void finish();
    private:
        imgui::context imgui_ctx;
        imgui::group_data gd;
    };

    ui* get_ui();
    void init_ui();
    void free_ui();    
}