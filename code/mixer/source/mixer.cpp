// C++
#include <cstdio>
#include <vector>
#include <string>

#include <nuklear.h>

/* constants */
const int MX_TOPBAR_HEIGHT = 100;
const int MX_VOLUME_HEIGHT = 30;
const int MX_CLOSE_BUTTON_SIZE = 32;
const int MX_GROUP_MARGIN = 4;

const int MX_ATMOSPHERE_TITLE_HEIGHT = 32;
const int MX_ATMOSPHERE_BUTTON_SIZE = 20;
const int MX_ATMOSPHERE_BUTTON_SPACER = 6;
const int MX_ATMOSPHERE_BUTTON_CONSTELLATION_WIDTH = 3 * MX_ATMOSPHERE_BUTTON_SIZE + 4 * MX_ATMOSPHERE_BUTTON_SPACER;
const int MX_ATMOSPHERE_BUTTON_CONSTELLATION_HEIGHT = 2 * MX_ATMOSPHERE_BUTTON_SIZE + 3 * MX_ATMOSPHERE_BUTTON_SPACER;

const int MX_ATMOSPHERE_HEIGHT = MX_ATMOSPHERE_TITLE_HEIGHT + MX_ATMOSPHERE_BUTTON_CONSTELLATION_HEIGHT;

/* state */
enum 
{
	MX_MODE_PLAY,
	MX_MODE_EDIT,
} MX_MODE = MX_MODE_PLAY;

struct atmosphere
{
	std::string title;
	float volume;
};

std::vector<atmosphere> atmospheres =
{
	{"Busy Tavern", 1.0f},
	{"Esga City Streets", 0.8f},
	{"Sailing", 0.7f},
	{"Below Decks", 0.7f},
	{"On the docks", 0.7f},
	{"Quiet Library", 0.2f},
	{"Camping", 0.95f},
	{"Thunderstorm", 1.0f},
	{"Tundra", 0.2f},
	{"Boss Music", 1.0f},
	{"Esga Temple", 0.62f},
};

/* 
	Draw Calls
*/	

void draw_top_bar(nk_context* ctx, int width)
{
	if (nk_begin(ctx, "top_bar", nk_rect(0, 0, width, MX_TOPBAR_HEIGHT), NK_WINDOW_BORDER))
	{

	}
	nk_end(ctx);	
}

void draw_play_screen(nk_context* ctx, int width, int height)
{
	if (nk_begin(ctx, "play_screen", nk_rect(0, MX_TOPBAR_HEIGHT, width, height), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
	{
		nk_layout_row_begin(ctx, NK_DYNAMIC, height, 2);
		nk_layout_row_push(ctx, 2.0f/3.0f);
		{
			nk_group_begin(ctx, "play_screen>left_group", 0);
			{
				for(size_t k = 0; k < atmospheres.size(); k++)
				{
					auto& currentAtmosphere = atmospheres[k];
					nk_layout_row_dynamic(ctx, MX_ATMOSPHERE_HEIGHT + 2 * MX_GROUP_MARGIN, 1);
					if(nk_group_begin(ctx, (std::string("player_screen>left_group>") + currentAtmosphere.title).c_str(), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
					{
						nk_layout_space_begin(ctx, NK_STATIC, MX_ATMOSPHERE_HEIGHT, 6);
						{
							auto bounds = nk_layout_space_bounds(ctx);

							// title
							nk_layout_space_push(ctx, nk_rect(0,0, bounds.w, MX_ATMOSPHERE_TITLE_HEIGHT));
							nk_label(ctx, currentAtmosphere.title.c_str(), NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);

							// volume slider
							const int volume_offset_x = 0;
							const int volume_offset_y = MX_ATMOSPHERE_TITLE_HEIGHT;
							const int volume_width = bounds.w - MX_ATMOSPHERE_BUTTON_CONSTELLATION_WIDTH;
							const int volume_height = MX_ATMOSPHERE_BUTTON_CONSTELLATION_HEIGHT;
							nk_layout_space_push(ctx, nk_rect(volume_offset_x, volume_offset_y, volume_width, volume_height));
							float step = 1.0f / volume_width;
							if(nk_slider_float(ctx, 0.0, &currentAtmosphere.volume, 1.0f, step))
							{
								printf("slider val: %f\n", currentAtmosphere.volume);
							}

							// // mute button
							const int mute_offset_x = volume_width + MX_ATMOSPHERE_BUTTON_SPACER;
							const int mute_offset_y = volume_offset_y + MX_ATMOSPHERE_BUTTON_SPACER + (MX_ATMOSPHERE_BUTTON_SIZE + MX_ATMOSPHERE_BUTTON_SPACER) / 2;
							nk_layout_space_push(ctx, nk_rect(mute_offset_x, mute_offset_y, MX_ATMOSPHERE_BUTTON_SIZE, MX_ATMOSPHERE_BUTTON_SIZE));
							if(nk_button_label(ctx, "V")) 
							{
								//toggle mute
							}

							// play button
							const int play_offset_x = mute_offset_x + MX_ATMOSPHERE_BUTTON_SIZE + MX_ATMOSPHERE_BUTTON_SPACER;
							const int play_offset_y = mute_offset_y - (MX_ATMOSPHERE_BUTTON_SIZE + MX_ATMOSPHERE_BUTTON_SPACER) / 2;
							nk_layout_space_push(ctx, nk_rect(play_offset_x, play_offset_y, MX_ATMOSPHERE_BUTTON_SIZE, MX_ATMOSPHERE_BUTTON_SIZE));
							if(nk_button_label(ctx, "P")) 
							{
								// set to solely
							}

							// play plus button
							const int playplus_offset_x = play_offset_x;
							const int playplus_offset_y = play_offset_y + MX_ATMOSPHERE_BUTTON_SIZE + MX_ATMOSPHERE_BUTTON_SPACER;
							nk_layout_space_push(ctx, nk_rect(playplus_offset_x, playplus_offset_y, MX_ATMOSPHERE_BUTTON_SIZE, MX_ATMOSPHERE_BUTTON_SIZE));
							if(nk_button_label(ctx, "+")) 
							{
								// set to also playing
							}

							// close button
							const int close_offset_x = play_offset_x + MX_ATMOSPHERE_BUTTON_SIZE + MX_ATMOSPHERE_BUTTON_SPACER;
							const int close_offset_y = mute_offset_y;
							nk_layout_space_push(ctx, nk_rect(close_offset_x, close_offset_y, MX_ATMOSPHERE_BUTTON_SIZE, MX_ATMOSPHERE_BUTTON_SIZE));
							if(nk_button_label(ctx, "X")) 
							{
								// remove this atmosphere from active list
							}
						}
						nk_layout_space_end(ctx);
					nk_group_end(ctx);		
					}
				}
			}
			nk_group_end(ctx);
		}
		nk_layout_row_push(ctx, 1.0f - 2.0f/3.0f);
		{
			nk_group_begin(ctx, "play_screen>right_group", 0);
			{
				nk_layout_row_dynamic(ctx, 40, 1);
				nk_button_label(ctx, "right");
			}
			nk_group_end(ctx);			
		}
		nk_layout_row_end(ctx);
	}
	nk_end(ctx);	
}

void draw_edit_screen(nk_context* ctx, int width, int height)
{
	if (nk_begin(ctx, "edit_screen", nk_rect(0, MX_TOPBAR_HEIGHT, width, height), NK_WINDOW_NO_SCROLLBAR))
	{

	}
	nk_end(ctx);
}

void gui_callback(nk_context* ctx, int width, int height)
{
	// draw shared top bar
	draw_top_bar(ctx, width);

	switch(MX_MODE)
	{
		case MX_MODE_PLAY:
			draw_play_screen(ctx, width, height - MX_TOPBAR_HEIGHT);
			break;
		case MX_MODE_EDIT:
			draw_edit_screen(ctx, width, height - MX_TOPBAR_HEIGHT);
			break;
	}

}

int main()
{
	nk_main("Tabletop Mixer", 800, 600, &gui_callback);
}