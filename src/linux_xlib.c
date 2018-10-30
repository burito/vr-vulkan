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
#include <X11/Xlib.h>

#include "log.h"
///////////////////////////////////////////////////////////////////////////////
//////// Public Interface to the rest of the program
///////////////////////////////////////////////////////////////////////////////

#include "keyboard.h"

int killme = 0;
int sys_width  = 1980;	/* dimensions of default screen */
int sys_height = 1200;
int sys_dpi = 1.0;
int vid_width  = 1280;	/* dimensions of our part of the screen */
int vid_height = 720;
int win_width  = 0;		/* used for switching from fullscreen back to window */
int win_height = 0;
int mouse_x;
int mouse_y;
int mickey_x;
int mickey_y;
char mouse[] = {0,0,0};
#define KEYMAX 128
char keys[KEYMAX];

int fullscreen=0;
int fullscreen_toggle=0;

int main_init(int argc, char *argv[]);
void main_loop(void);
void main_end(void);

const int sys_ticksecond = 1000000;
long long sys_time(void)
{
	struct timeval tv;
	tv.tv_usec = 0;	// tv.tv_sec = 0;
	gettimeofday(&tv, NULL);
	return tv.tv_usec + tv.tv_sec * sys_ticksecond;
}

void shell_browser(char *url)
{
	int c=1000;
	char buf[c];
	memset(buf, 0, sizeof(char)*c);
	snprintf(buf, c, "sensible-browser %s &", url);
	system(buf);
}
///////////////////////////////////////////////////////////////////////////////
//////// End Public Interface
///////////////////////////////////////////////////////////////////////////////

Display *display;
Window window;

int main(int argc, char *argv[])
{
	log_init();
	log_debug("Program Start");

	display = XOpenDisplay(NULL);
	log_debug("XOpenDisplay");

	int screen = DefaultScreen(display);
	int white_pixel = WhitePixel(display, screen);
	int black_pixel = BlackPixel(display, screen);

	window = XCreateSimpleWindow( display, RootWindow(display, screen), 0, 0, VIDX, VIDY, 0, white_pixel, black_pixel );
	log_debug("XCreateSimpleWindow");

	XSetWindowAttributes winAttr;
	winAttr.override_redirect = 1;
	XChangeWindowAttributes(display, window, CWOverrideRedirect, &winAttr);

	XWarpPointer(display, None, window, 0, 0, 0, 0, VIDX, 0);
	XMapWindow(display, window);
	XMapRaised(display, window);
	XFlush(display);

	/* Vulkan Initialisation is here! */
	if( main_init(argc, argv) )
		killme = 1;

	XGrabKeyboard(display, window, True, GrabModeAsync,GrabModeAsync,CurrentTime);

	XEvent event;
	while( !XCheckTypedEvent(display, KeyPress, &event) )
	{

		/* main loop is here! */
		main_loop();
	}
	main_end();
	return 0;
}