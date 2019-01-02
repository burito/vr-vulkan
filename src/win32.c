/*
Copyright (c) 2012 Daniel Burke

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
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

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "global.h"
#include "log.h"

int fullscreen = 0;
int fullscreen_toggle = 0;


struct fvec2
{
	float x, y;
};

typedef struct joystick
{
	int connected;
	struct fvec2 l, r;
	float lt, rt;
	int button[15];
	int fflarge, ffsmall;
} joystick;

joystick joy[4];

///////////////////////////////////////////////////////////////////////////////
//////// End Public Interface
///////////////////////////////////////////////////////////////////////////////
HINSTANCE hInst;
HWND hWnd;

void vulkan_resize(void);

int win_resizing = 0;
int CmdShow;

int win_width  = 0;	/* used for switching from fullscreen back to window */
int win_height = 0;

int window_maximized = 0;
int focus = 1;
int menu = 1;
int sys_bpp = 32;

///////////////////////////////////////////////////////////////////////////////
//////// borrowed from XInput.h 
///////////////////////////////////////////////////////////////////////////////
//#include <Xinput.h>
#define XINPUT_GAMEPAD_DPAD_UP          0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define XINPUT_GAMEPAD_START            0x0010
#define XINPUT_GAMEPAD_BACK             0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define XINPUT_GAMEPAD_A                0x1000
#define XINPUT_GAMEPAD_B                0x2000
#define XINPUT_GAMEPAD_X                0x4000
#define XINPUT_GAMEPAD_Y                0x8000

typedef struct _XINPUT_GAMEPAD {
    WORD wButtons;
    BYTE bLeftTrigger;
    BYTE bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE {
    DWORD dwPacketNumber;
    XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef struct _XINPUT_VIBRATION {
    WORD wLeftMotorSpeed;
    WORD wRightMotorSpeed;
} XINPUT_VIBRATION, *PXINPUT_VIBRATION;

DWORD WINAPI (*XInputGetState)(DWORD, XINPUT_STATE*) = NULL;
DWORD WINAPI (*XInputSetState)(DWORD, XINPUT_VIBRATION*) = NULL;

HMODULE xinput_dll = NULL;
///////////////////////////////////////////////////////////////////////////////
//////// XInput.h - end
///////////////////////////////////////////////////////////////////////////////


static const char* win_errormsg(void)
{
	int err;
	static char errStr[1000];
	err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, err, 0,
		errStr, 1000, 0);
	return errStr;
}

static void sys_input(void)
{
 	DWORD result;
 	XINPUT_STATE state;
	if(xinput_dll == NULL)return;

	for(int i=0; i<4; i++)
	{
		ZeroMemory( &state, sizeof(XINPUT_STATE) );
		result = XInputGetState( i, &state );
		if( result != ERROR_SUCCESS )
		{
			joy[i].connected = 0;
			continue;
		}
		joy[i].connected = 1;

		joy[i].l.x = state.Gamepad.sThumbLX;
		joy[i].l.y = state.Gamepad.sThumbLY;
		joy[i].r.x = state.Gamepad.sThumbRX;
		joy[i].r.y = state.Gamepad.sThumbRY;
		joy[i].lt = state.Gamepad.bLeftTrigger;
		joy[i].rt = state.Gamepad.bRightTrigger;

		joy[i].button[0] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_A);
		joy[i].button[1] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_B);
		joy[i].button[2] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_X);
		joy[i].button[3] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_Y);
		joy[i].button[4] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
		joy[i].button[5] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
		joy[i].button[6] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
		joy[i].button[7] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
		joy[i].button[8] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_START);
		joy[i].button[9] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK);
		joy[i].button[10] = 0;
		joy[i].button[11] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP);
		joy[i].button[12] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
		joy[i].button[13] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
		joy[i].button[14] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

		XINPUT_VIBRATION vib;
		vib.wLeftMotorSpeed = joy[i].fflarge * 255;
		vib.wRightMotorSpeed = joy[i].ffsmall * 255;
		XInputSetState(i, &vib);
	}
}

int w32_moving = 0;
int w32_delta_x = 0;
int w32_delta_y = 0;

static LONG WINAPI wProc(HWND hWndProc, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int code;
	int bit=0;
	switch(uMsg) {
	case WM_SYSCOMMAND:
//		log_debug("WM_SYSCOMMAND\n");
		switch(wParam & 0xFFF0) {
		case SC_SIZE:
//			log_debug("SC_SIZE\n");
			break;
			return 0;
		case SC_MOVE:	// Moving the window locks the app, so implement it manually
			w32_moving = 1;
			POINT p;
			GetCursorPos(&p);
			RECT r;
			GetWindowRect(hWnd,&r);
			w32_delta_x = p.x - r.left;
			w32_delta_y = p.y - r.top;
			SetCapture(hWnd);
			return 0;
		default:
			break;
		}
		break;
		return 0;
//	case WM_EXITSIZEMOVE:
//		printf("WM_EXITSIZEMOVE\n");
//		return 0;
/*
	case WM_SIZING:
		{
			RECT *rect = lParam;
			vid_width = rect->right - rect->left;
			vid_height = rect->bottom - rect->top;
//			log_warning("resizing=%d: wParam=%d, x=%d, y=%d", win_resizing, wParam, vid_width, vid_height );
			vulkan_resize();
		}
		PostMessage(hWndProc, WM_PAINT, 0, 0);
		return 0;
*/
	case WM_SIZE:
		vid_width = LOWORD(lParam);
		vid_height = HIWORD(lParam);
//		if( win_resizing )
		{
			if(wParam == 8)return 0;
//			log_warning("resize=%d: wParam=%d, loword=%d, hiword=%d", win_resizing, wParam, LOWORD(lParam), HIWORD(lParam) );
			vulkan_resize();
		}
		PostMessage(hWndProc, WM_PAINT, 0, 0);
		return 0;
	case WM_ENTERSIZEMOVE:
		win_resizing = 1;
		break;
	case WM_EXITSIZEMOVE:
		win_resizing = 0;
		break;


	case WM_KEYDOWN:
		bit = 1;
	case WM_KEYUP:
		code = (HIWORD(lParam)) & 511;
		if(code < KEYMAX)keys[code]=bit;
		if(code == KEY_F11 && bit == 0)
			fullscreen_toggle = 1;
		return 0;

	case WM_LBUTTONDOWN:
		bit = 1;
	case WM_LBUTTONUP:
		mouse[0]=bit;
		if(w32_moving)
		{
			w32_moving = 0;
			ReleaseCapture();
		}
		return 0;

	case WM_MBUTTONDOWN:
		bit = 1;
	case WM_MBUTTONUP:
		mouse[1]=bit;
		return 0;

	case WM_RBUTTONDOWN:
		bit = 1;
	case WM_RBUTTONUP:
		mouse[2]=bit;
		return 0;

	case WM_XBUTTONDOWN:
		bit = 1;
	case WM_XBUTTONUP:
		switch(GET_XBUTTON_WPARAM(wParam)) {
		case XBUTTON1:
			mouse[3]=bit;
			break;
		case XBUTTON2:
			mouse[4]=bit;
			break;
		default:
			break;
		}
		return 0;

	case WM_MOUSEMOVE:
		mickey_x += mouse_x - LOWORD(lParam);
		mickey_y += mouse_y - HIWORD(lParam);
		mouse_x = LOWORD(lParam);
		mouse_y = HIWORD(lParam);
		if(w32_moving)
		{
			RECT r;
			int win_width, win_height;
			GetWindowRect(hWnd,&r);
			win_height = r.bottom - r.top;
			win_width = r.right - r.left;
			POINT p;
			GetCursorPos(&p);
			MoveWindow(hWnd, p.x - w32_delta_x,
					 p.y - w32_delta_y,
					 win_width, win_height, TRUE);
		}
		else
		{

		}
		break;

	case WM_SETCURSOR:
		switch(LOWORD(lParam))
		{
		case HTCLIENT:
			if(!menu)
				while(ShowCursor(FALSE) >= 0);
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return 0;
		default:
			while(ShowCursor(TRUE) < 0);
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			break;
		}
		break;

	case 0x02E0:	// WM_DPICHANGED
		sys_dpi =(float)LOWORD(wParam) / 96.0; // dpi-X
//		sys_dpi = HIWORD(wParam);	// dpi-Y
		RECT *r = (RECT*)lParam;
		SetWindowPos(hWnd, HWND_TOP, 0, 0,
			r->left - r->right,
			r->top - r->bottom,
			SWP_NOMOVE);
		return 0;

	case WM_CLOSE:
		log_info("Shutdown on : Window close");
		killme=1;
		return 0;
	}
	return DefWindowProc(hWndProc, uMsg, wParam, lParam);
}


// this is Google Chrome style window fullscreen
// https://stackoverflow.com/questions/2382464/win32-full-screen-and-hiding-taskbar
int win_style;
int win_exstyle;
BOOL win_maximized;
RECT win_rect;
static void win_toggle(void)
{
	if(!fullscreen)
	{
		win_maximized = IsZoomed(hWnd);
		if(win_maximized)
		{
			SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		}
		GetWindowRect(hWnd, &win_rect);
		win_width = win_rect.right - win_rect.left;
		win_height = win_rect.bottom - win_rect.top;
		win_style = GetWindowLongPtr(hWnd, GWL_STYLE);
		win_exstyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
		SetWindowLongPtr(hWnd, GWL_STYLE, win_style & ~(WS_CAPTION | WS_THICKFRAME) );
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, win_exstyle &
			~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE) );
		
		MONITORINFO monitor_info;
		monitor_info.cbSize = sizeof(monitor_info);
		GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &monitor_info);
		SetWindowPos(hWnd, NULL,
			monitor_info.rcMonitor.left,
			monitor_info.rcMonitor.top,
			monitor_info.rcMonitor.right,
			monitor_info.rcMonitor.bottom,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
		);
	}
	else
	{
		SetWindowLongPtr(hWnd, GWL_STYLE, win_style);
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, win_exstyle);
		SetWindowPos(hWnd, NULL,
			win_rect.left,
			win_rect.top,
			win_rect.right - win_rect.left,
			win_rect.bottom - win_rect.top,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
		);
		if(win_maximized)
		{
			SendMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		}
	}

	fullscreen_toggle = 0;
	fullscreen = !fullscreen;

	ShowWindow(hWnd, CmdShow);
	UpdateWindow(hWnd);
	vulkan_resize();
}


char *winClassName = "Kittens";
static void win_init(void)
{
	memset(keys, 0, KEYMAX);
	memset(joy, 0, sizeof(joystick)*4);

	HMODULE l = LoadLibrary("shcore.dll");
	if(NULL != l)
	{
		void (*SetProcessDpiAwareness)() =
			(void(*)())GetProcAddress(l, "SetProcessDpiAwareness");

		void (*GetDpiForMonitor)() =
			(void(*)())GetProcAddress(l, "GetDpiForMonitor");

		SetProcessDpiAwareness(2);// 2 = Process_Per_Monitor_DPI_Aware
		int dpi_x=0, dpi_y=0;
		POINT p = {1, 1};
		HMONITOR hmon = MonitorFromPoint(p, MONITOR_DEFAULTTOPRIMARY);
		GetDpiForMonitor(hmon, 0, &dpi_x, &dpi_y);
		FreeLibrary(l);
		sys_dpi = (float)dpi_x / 96.0;
	}

	xinput_dll = LoadLibrary("XINPUT1_3.dll");
	if(xinput_dll == NULL)
	{
		log_warning("XInput not found (XINPUT1_3.dll)");
		log_warning("XInput, a part of DirectX, is needed for Xbox360 Controller support");
		log_warning("See: https://www.microsoft.com/en-us/download/details.aspx?id=35");
	}
	else
	{
		XInputGetState = (DWORD WINAPI (*)(DWORD, XINPUT_STATE*))GetProcAddress(xinput_dll, "XInputGetState");
		XInputSetState = (DWORD WINAPI (*)(DWORD, XINPUT_VIBRATION*))GetProcAddress(xinput_dll, "XInputSetState");
	}

	WNDCLASSEX wc;
	wc.cbSize	= sizeof(WNDCLASSEX);
	wc.style	= CS_OWNDC;
	wc.lpfnWndProc	= (WNDPROC)wProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= hInst;
	wc.hIcon	= LoadIcon(hInst, MAKEINTRESOURCE(0));
	wc.hIconSm	= NULL; //LoadIcon(hInst, IDI_APPLICAITON);
	wc.hCursor	= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground= NULL;
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= winClassName;

	if(!RegisterClassEx(&wc))
	{
		log_fatal("RegisterClassEx() = %s",  win_errormsg());
		return;
	}

	hWnd = CreateWindowEx(0, "Kittens", "Kittens",
		WS_TILEDWINDOW, //|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
		50,50,vid_width,vid_height,NULL,NULL,hInst,NULL);

	if(!hWnd)
	{
		log_fatal("CreateWindowEx() = %s",  win_errormsg());
		return;
	}
	ShowWindow(hWnd, CmdShow);
	UpdateWindow(hWnd);

}

static void win_end(void)
{
	if(fullscreen)ShowCursor(TRUE);
	DestroyWindow(hWnd);
	UnregisterClass(winClassName, hInst);
	if(xinput_dll != NULL)
	{
		XInputGetState = NULL;
		XInputSetState = NULL;
		FreeLibrary(xinput_dll);
		xinput_dll = NULL;
	}

}

static void handle_events(void)
{
	MSG mesg;
	mickey_x = mickey_y = 0;
	sys_input();

	while(PeekMessage(&mesg, NULL, 0, 0, PM_REMOVE))
	{
		if(mesg.message == WM_QUIT)
			killme=TRUE;
		if(mesg.hwnd == hWnd)
		{
			wProc(mesg.hwnd,mesg.message,mesg.wParam,mesg.lParam);
		}
		else
		{
			TranslateMessage(&mesg);
			DispatchMessage(&mesg);
		}
	}

	if(fullscreen_toggle)
	{
		win_toggle();
	}
}


int APIENTRY WinMain(HINSTANCE hCurrentInst, HINSTANCE hPrev,
	    LPSTR lpszCmdLine, int nCmdShow)
{
	hInst = hCurrentInst;
	CmdShow = nCmdShow;
	log_init();
	log_info("Platform    : win32");
	/* Convert win32 style arguments to standard format */
#define ARGC_MAX 10
	int last=0;
	int escape=0;
	int argc=1;
	char *argv[ARGC_MAX];
	argv[0] = NULL;
	for(int i=0; i<1000; i++)
	{
		if(ARGC_MAX <= argc)break;
		if(lpszCmdLine[i] == '"')escape = !escape;
		if(lpszCmdLine[i]==0 || (!escape && lpszCmdLine[i] == ' '))
		{
			int size = i - last;
			char *arg = malloc(size+1);
			memcpy(arg, lpszCmdLine+last, size);
			arg[size]=0;
			argv[argc]=arg;
			argc++;
			while(lpszCmdLine[i]==' ')i++;
			last = i;
			if(lpszCmdLine[i]==0)break;
		}
	}
#undef ARGC_MAX

	win_init();
	int ret = main_init(argc, argv);
	for(int i=0; i<argc; i++)free(argv[i]); /* delete args */
	if(ret)
	{
		win_end();
		return ret;
	}

	while(!killme)
	{
		handle_events();
		main_loop();
	}

	main_end();
	win_end();
	return 0;
}


