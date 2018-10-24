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

#include <windows.h>

#include "log.h"


int vulkan_init(void);
int vulkan_loop(float current_time);

HINSTANCE hInst;
HWND hWnd;

///////////////////////////////////////////////////////////////////////////////
//////// Public Interface to the rest of the program
///////////////////////////////////////////////////////////////////////////////

#include "keyboard.h"

int killme=0;
int sys_width  = 1980;	/* dimensions of default screen */
int sys_height = 1200;
float sys_dpi = 1.0;
int vid_width  = 1280;	/* dimensions of our part of the screen */
int vid_height = 720;
int mouse_x = 0;
int mouse_y = 0;
int mickey_x = 0;
int mickey_y = 0;
char mouse[] = {0,0,0,0,0,0,0,0};
#define KEYMAX 512
char keys[KEYMAX];

int fullscreen = 0;
int fullscreen_toggle = 0;

const int sys_ticksecond = 1000;
long long sys_time(void)
{
	return timeGetTime();
}

void shell_browser(char *url)
{
	ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
}
///////////////////////////////////////////////////////////////////////////////
//////// End Public Interface
///////////////////////////////////////////////////////////////////////////////


LRESULT CALLBACK WndProc(HWND hWndProc, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
	case WM_DESTROY:
		DestroyWindow(hWndProc);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		ValidateRect(hWndProc, NULL);
		break;
	case WM_CLOSE:
		killme = 1;
		return 0;
	}
	return DefWindowProc(hWndProc, uMsg, wParam, lParam);
}

char *winClassName = "Kittens";

int APIENTRY WinMain(HINSTANCE hCurrentInst, HINSTANCE hPrev,
	    LPSTR lpszCmdLine, int nCmdShow)
{
	hInst = hCurrentInst;

	log_init();
	log_info("program start");

	WNDCLASSEX wc;
	wc.cbSize	= sizeof(WNDCLASSEX);
	wc.style	= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	= WndProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= hInst;
	wc.hIcon	= LoadIcon(hInst, MAKEINTRESOURCE(0));
	wc.hIconSm	= NULL; //LoadIcon(hInst, IDI_APPLICAITON);
	wc.hCursor	= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground= (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= winClassName;

	if(!RegisterClassEx(&wc))
	{
		log_fatal("RegisterClassEx()");
		return 1;
	}
	RECT wr = {0, 0, vid_width, vid_height};
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	hWnd = CreateWindowEx(0, winClassName, winClassName,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU, // WS_TILEDWINDOW, //|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
		50,50,
		wr.right - wr.left,  // width
		wr.bottom - wr.top,  // height
		NULL,NULL,hInst,NULL);

	if(!hWnd)
	{
		log_fatal("CreateWindowEx()");
		return 2;
	}

	if( vulkan_init() )
		killme = 1;

	long long last_time = sys_time();

	MSG mesg;

	while(!killme)
	{
		while(PeekMessage(&mesg, NULL, 0, 0, PM_REMOVE))
		{
			if(mesg.message == WM_QUIT)
				killme=TRUE;
			else
			{
				TranslateMessage(&mesg);
				DispatchMessage(&mesg);
			}

		}
		long long time_now = sys_time();

		int ret = vulkan_loop( (time_now - last_time) / (float)sys_ticksecond );
		if(ret)killme = 1;
//		SwapBuffers(hDC);
		RedrawWindow(hWnd, NULL, NULL, RDW_INTERNALPAINT);
	}
	return 0;
}