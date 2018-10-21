/*
Copyright (C) 2018 Daniel Burke

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <stdlib.h>
#include <xcb/xcb.h>

#include "log.h"
int vulkan_init(void);
int vulkan_loop(float current_time);

#define VIDX 1280
#define VIDY 800

#include <sys/time.h>
static long timeGetTime( void ) // Thanks Inigo Quilez!
{
	struct timeval now, res;
	gettimeofday(&now, 0);
	return (long)((now.tv_sec*1000) + (now.tv_usec/1000));
}

xcb_connection_t *xcb;
xcb_window_t window;

int main(int argc, char *argv[])
{
	log_init();
	log_debug("main()");
	xcb_screen_t *screen;
	xcb_intern_atom_reply_t *atom_wm_delete_window;

	int scr;
	xcb = xcb_connect(NULL, &scr);
	const xcb_setup_t *setup;
	xcb_screen_iterator_t iter;

	setup = xcb_get_setup(xcb);
	iter = xcb_setup_roots_iterator(setup);
	while( scr-- > 0) xcb_screen_next(&iter);
	screen = iter.data;

	window = xcb_generate_id(xcb);

	uint32_t value_mask, value_list[32];
	value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value_list[0] = screen->black_pixel;
	value_list[1] = XCB_EVENT_MASK_KEY_RELEASE;

	xcb_create_window(xcb, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0, VIDX, VIDY, 0,
	XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, value_mask, value_list);


	xcb_intern_atom_cookie_t cookie = xcb_intern_atom(xcb, 1, 12, "WM_PROTOCOLS");
	xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(xcb, cookie, 0);

	xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(xcb, 0, 16, "WM_DELETE_WINDOW");
	xcb_intern_atom_reply_t* reply2 = xcb_intern_atom_reply(xcb, cookie2, 0);

	xcb_change_property(xcb, XCB_PROP_MODE_REPLACE, window, (*reply).atom, 4, 32, 1, &(*reply2).atom);

	xcb_map_window(xcb, window);
	xcb_flush(xcb);

	if( vulkan_init() )
	{
		log_fatal("Vulkan Initialisation failed");
		xcb_disconnect(xcb);
		return 1;
	}

	long last_time = timeGetTime();
	int killme = 0;
	while(!killme) {
		xcb_generic_event_t *event = xcb_poll_for_event(xcb);
		while(event)
		{
			switch ( event->response_type & 0x7f) {
			case XCB_KEY_RELEASE:
				log_debug("key release");
				killme = 1;
				break;
			case XCB_DESTROY_NOTIFY:
			case XCB_DESTROY_WINDOW:
			case XCB_KILL_CLIENT:
				log_debug("kill client");
				killme = 1;
				break;
			case XCB_CLIENT_MESSAGE:
				log_debug("client message");
				if( ((xcb_client_message_event_t*)event)->data.data32[0] ==
				reply2->atom )
				{
					log_debug("kill");
					killme = 1;
				}
				break;
			default:
				break;	
			}
			free(event);
			event = xcb_poll_for_event(xcb);
		}
		/* main loop is here! */

		int ret = 0;
		long time_now = timeGetTime();
		ret = vulkan_loop( (time_now - last_time) * 0.001 );

		if(ret)killme = 1;
		/* main loop ends here! */
	}
	xcb_disconnect(xcb);
	log_debug("program exit ok");
	return 0;
}
