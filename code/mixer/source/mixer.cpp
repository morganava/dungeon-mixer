#include <cstdio>

#include <nuklear.h>

int main()
{
	std::printf("Hello World!\n");
	nk_main("Test Window", 800, 600, nullptr);
	//nk_window_get_canvas(nullptr);
}