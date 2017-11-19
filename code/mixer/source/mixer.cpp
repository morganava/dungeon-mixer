#include <cstdio>

#include <nuklear.h>

void gui_callback(nk_context* ctx, int width, int height)
{
	std::printf("Width: %i Height: %i\n", width, height);
}

int main()
{
	std::printf("Hello World!\n");
	nk_main("Test Window", 800, 600, &gui_callback);
	//nk_window_get_canvas(nullptr);
}