/*
Copyright (c) 2015 Daniel Burke

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

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#ifdef NO_SLEEP
#import <IOKit/pwr_mgt/IOPMLib.h>	// to disable sleep
#import <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#endif
//#include <ForceFeedback/ForceFeedback.h>


#include <MoltenVK/mvk_vulkan.h>
#define LOG_NO_DEBUG
#include "log.h"
#include "global.h"
///////////////////////////////////////////////////////////////////////////////
//////// Public Interface to the rest of the program
///////////////////////////////////////////////////////////////////////////////

void vulkan_resize(void);

/*
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
*/
///////////////////////////////////////////////////////////////////////////////
//////// Mac OS X OpenGL window setup
///////////////////////////////////////////////////////////////////////////////
void * pView = NULL;
CVDisplayLinkRef _displayLink;


/*
typedef struct osx_joystick
{
	IOHIDDeviceRef device;
	io_service_t iost;
	FFDeviceObjectReference ff;
	FFEFFECT *effect;
	FFCUSTOMFORCE *customforce;
	FFEffectObjectReference effectRef;
} osx_joystick;


osx_joystick osx_joy[4];

void ff_set(void);
*/
static int gargc;
static const char ** gargv;
static NSWindow * window;
static NSApplication * myapp;
static int y_correction = 0;  // to correct mouse position for title bar

extern char *key_names[];


@interface View : NSView
@end

@implementation View

-(id) init
{
	log_debug("View:init");
	CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
	CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, NULL);
	return [super init];
}

static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink,
					const CVTimeStamp *now,
					const CVTimeStamp *outputTime,
					CVOptionFlags flagsIn,
					CVOptionFlags *flagsOut,
					void *target)
{
//	log_debug("View:DisplayLinkCallback");
	main_loop();

	if(killme != 0)
	{
//		[NSApp performSelector:@selector(terminate:) withObject:nil afterDelay:0.0];
		[NSApp terminate:nil];
	}
/*
	if(fullscreen_toggle)
	{
		fullscreen_toggle = 0;
		[window toggleFullScreen:(window)];
	}
*/
	return kCVReturnSuccess;
}

-(BOOL) wantsUpdateLayer {
	log_debug("View:wantsUpdateLayer");

//	vulkan_resize();
	return YES; }

+(Class) layerClass { 
	log_debug("View:layerClass");
	return [CAMetalLayer class]; }

-(CALayer*) makeBackingLayer
{
	log_debug("View:makeBackingLayer");
	CALayer *layer = [self.class.layerClass layer];
	CGSize viewScale = [self convertSizeToBacking: CGSizeMake(1.0, 1.0)];
	layer.contentsScale = MIN(viewScale.width, viewScale.height);
	return layer;
}

/*
-(void)resize
{
	log_info("View:resize");

}

-(void)reshape
{
	log_info("View:reshape");
	vid_width = [self convertRectToBacking:[self bounds]].size.width;
	vid_height = [self convertRectToBacking:[self bounds]].size.height;
	y_correction = sys_dpi * [[[self window] contentView] frame].size.height - vid_height;
	if(vid_height == 0) vid_height = 1;
//	vulkan_resize();
}
*/

-(void) dealloc
{
	log_debug("View:dealloc");
	// main_shutdown();
	CVDisplayLinkRelease(_displayLink);
	[super dealloc];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate


- (void)applicationWillFinishLaunching:(NSNotification *)aNotification
{
	log_debug("AppDelegate:applicationWillFinishLaunching");

	// Create the menu that goes on the Apple Bar
	NSMenu * mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
	NSMenuItem * menuTitle;
	NSMenu * aMenu;

	menuTitle = [mainMenu addItemWithTitle:@"Apple" action:NULL keyEquivalent:@""];
	aMenu = [[NSMenu alloc] initWithTitle:@"Apple"];
	[NSApp performSelector:@selector(setAppleMenu:) withObject:aMenu];

	// generate contents of menu
	NSMenuItem * menuItem;
	NSString * applicationName = [[NSProcessInfo processInfo] processName];
	menuItem = [aMenu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"About", nil), applicationName]
				    action:@selector(orderFrontStandardAboutPanel:)
			     keyEquivalent:@""];
	[menuItem setTarget:NSApp];
	[aMenu addItem:[NSMenuItem separatorItem]];

	menuItem = [aMenu addItemWithTitle:NSLocalizedString(@"Fullscreen", nil)
				    action:@selector(toggleFullScreen:)
			     keyEquivalent:@"f"];
	[menuItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand | NSEventModifierFlagControl];
	menuItem.target = nil;

	[aMenu addItem:[NSMenuItem separatorItem]];

	menuItem = [aMenu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"Quit", nil), applicationName]
				    action:@selector(terminate:)
			     keyEquivalent:@"q"];
	[menuItem setTarget:NSApp];

	// attach generated menu to menuitem
	[mainMenu setSubmenu:aMenu forItem:menuTitle];
	[NSApp setMainMenu:mainMenu];

}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotifcation
{
	log_debug("AppDelegate:applicationDidFinishLaunching");
	// init
	CVDisplayLinkStart(_displayLink);
	log_debug("no really, we did finish launching");
	
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
{
	if( ! killme )
		log_info("Shutdown on : App Close");
	killme = 1;

//	return NSTerminateCancel;
	return NSTerminateNow;
//	return NSTerminateLater;
}

/*
void ff_init(osx_joystick* j)
{
	FFCreateDevice(j->iost, &j->ff);
	if (j->ff == 0) return;

	FFCAPABILITIES capabs;
	FFDeviceGetForceFeedbackCapabilities(j->ff, &capabs);

	if(capabs.numFfAxes != 2) return;

	j->effect = malloc(sizeof(FFEFFECT));
	j->customforce = malloc(sizeof(FFCUSTOMFORCE));
	LONG *c = malloc(2 * sizeof(LONG));
	DWORD *a = malloc(2 * sizeof(DWORD));
	LONG *d = malloc(2 * sizeof(LONG));

	c[0] = 0;
	c[1] = 0;
	a[0] = capabs.ffAxes[0];
	a[1] = capabs.ffAxes[1];
	d[0] = 0;
	d[1] = 0;

	j->customforce->cChannels = 2;
	j->customforce->cSamples = 2;
	j->customforce->rglForceData = c;
	j->customforce->dwSamplePeriod = 100*1000;

	j->effect->cAxes = capabs.numFfAxes;
	j->effect->rglDirection = d;
	j->effect->rgdwAxes = a;
	j->effect->dwSamplePeriod = 0;
	j->effect->dwGain = 10000;
	j->effect->dwFlags = FFEFF_OBJECTOFFSETS | FFEFF_SPHERICAL;
	j->effect->dwSize = sizeof(FFEFFECT);
	j->effect->dwDuration = FF_INFINITE;
	j->effect->dwSamplePeriod = 100*1000;
	j->effect->cbTypeSpecificParams = sizeof(FFCUSTOMFORCE);
	j->effect->lpvTypeSpecificParams = j->customforce;
	j->effect->lpEnvelope = NULL;
	FFDeviceCreateEffect(j->ff, kFFEffectType_CustomForce_ID, j->effect, &j->effectRef);
}

void ff_shutdown(osx_joystick *j)
{
	if (j->effectRef == NULL) return;
	FFDeviceReleaseEffect(j->ff, j->effectRef);
	j->effectRef = NULL;
	free(j->customforce->rglForceData);
	free(j->effect->rgdwAxes);
	free(j->effect->rglDirection);
	free(j->customforce);
	free(j->effect);
	FFReleaseDevice(j->ff);
}
void ff_set(void)
{
	for(int i=0; i<4; i++)
	{
		if(osx_joy[i].effectRef == NULL) continue;
		osx_joy[i].customforce->rglForceData[0] = (joy[i].fflarge * 10000) / 255;
		osx_joy[i].customforce->rglForceData[1] = (joy[i].ffsmall * 10000) / 255;
		FFEffectSetParameters(osx_joy[i].effectRef, osx_joy[i].effect, FFEP_TYPESPECIFICPARAMS);
		FFEffectStart(osx_joy[i].effectRef, 1, 0);
	}
}

void gamepadWasAdded(void* inContext, IOReturn inResult, void* inSender, IOHIDDeviceRef device)
{
	for(int i=0; i<4; i++)
	if(osx_joy[i].device == 0)
	{
		osx_joy[i].device = device;
		osx_joy[i].iost = IOHIDDeviceGetService(device);
		ff_init(&osx_joy[i]);
		joy[i].connected = 1;
		return;
	}
	log_info("More than 4 joysticks plugged in\n");
}

void gamepadWasRemoved(void* inContext, IOReturn inResult, void* inSender, IOHIDDeviceRef device)
{
	for(int i=0; i<4; i++)
	if(osx_joy[i].device == device)
	{
		ff_shutdown(&osx_joy[i]);
		osx_joy[i].device = 0;
		joy[i].connected = 0;
		return;
	}
	log_info("Unexpected Joystick unplugged\n");
}

// https://developer.apple.com/library/mac/documentation/DeviceDrivers/Conceptual/HID/new_api_10_5/tn2187.html#//apple_ref/doc/uid/TP40000970-CH214-SW2
void gamepadAction(void* inContext, IOReturn inResult,
		   void* inSender, IOHIDValueRef valueRef) {

	IOHIDElementRef element = IOHIDValueGetElement(valueRef);
	Boolean isElement = CFGetTypeID(element) == IOHIDElementGetTypeID();
	if (!isElement)
		return;
//	IOHIDElementCookie cookie = IOHIDElementGetCookie(element);
//	IOHIDElementType type = IOHIDElementGetType(element);
//	CFStringRef name = IOHIDElementGetName(element);
	int page = IOHIDElementGetUsagePage(element);
	int usage = IOHIDElementGetUsage(element);

	long value = IOHIDValueGetIntegerValue(valueRef);

	int i = -1;
	for(int j=0; j<4; j++)
	if(osx_joy[j].device == inSender)
	{
		i = j;
		break;
	}
	if(i == -1)
	{
		log_info("Unexpected joystick event = %p\n", inSender);
		return;
	}
	// page == 1 = axis
	// page == 9 = button
	switch(usage) {
	case 1:	// A
	case 2:	// B
	case 3:	// X
	case 4:	// Y
	case 5:	// LB
	case 6: // RB
	case 7: // LJ
	case 8: // RJ
	case 9: // Start
	case 10: // Select
	case 11: // Logo
	case 12: // Dpad Up
	case 13: // Dpad Down
	case 14: // Dpad Left
	case 15: // Dpad Right
		joy[i].button[usage-1] = value;
		break;
	case 48: // L-X
		joy[i].l.x = value;
		break;
	case 49: // L-Y
		joy[i].l.y = value;
		break;
	case 51: // R-X
		joy[i].r.x = value;
		break;
	case 52: // R-Y
		joy[i].r.y = value;
		break;
	case 50: // LT
		joy[i].lt = value;
		break;
	case 53: // RT
		joy[i].rt = value;
		break;
	case -1: // Gets fired occasionally on connect
		break;
	default:
		log_info("usage = %d, page = %d, value = %d\n", usage, page, (int)value);
		break;
	}
}


-(void) setupGamepad {
	hidManager = IOHIDManagerCreate( kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	NSMutableDictionary* criterion = [[NSMutableDictionary alloc] init];
	[criterion setObject: [NSNumber numberWithInt: kHIDPage_GenericDesktop] forKey: (NSString*)CFSTR(kIOHIDDeviceUsagePageKey)];
	[criterion setObject: [NSNumber numberWithInt: kHIDUsage_GD_GamePad] forKey: (NSString*)CFSTR(kIOHIDDeviceUsageKey)];
	IOHIDManagerSetDeviceMatching(hidManager, (__bridge CFDictionaryRef)criterion);
	IOHIDManagerRegisterDeviceMatchingCallback(hidManager, gamepadWasAdded, (void*)self);
	IOHIDManagerRegisterDeviceRemovalCallback(hidManager, gamepadWasRemoved, (void*)self);
	IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
	IOHIDManagerRegisterInputValueCallback(hidManager, gamepadAction, (void*)self);
}
*/
@end

@interface WindowDelegate: NSObject<NSWindowDelegate>
@end

@implementation WindowDelegate

- (BOOL)windowShouldClose:(NSWindow *)sender;
{
	log_info("Shutdown on : Window close");
	killme = 1;
	return true;
}

- (void)windowDidResize:(NSNotification *)notification;
{
	log_debug("WindowDelegate:windowDidResize");
	NSWindow *window = [notification object];
	CGSize box = window.frame.size;
	vid_width = box.width * sys_dpi;
	vid_height = box.height * sys_dpi;
//	log_info("x = %d, y = %d", vid_width, vid_height);
	vulkan_resize();

}


@end


@interface MyApp : NSApplication
{
}

@end

void flags_changed(NSEvent *theEvent)
{
//	log_debug("flags_changed");
	for(int i = 0; i<24; i++)
	{
		int bit = !!(theEvent.modifierFlags & (1 << i));
		switch(i) {
		case   0: keys[KEY_LCONTROL] = bit; break;
		case   1: keys[KEY_LSHIFT] = bit; break;
		case   2: keys[KEY_RSHIFT] = bit; break;
		case   3: keys[KEY_LLOGO] = bit; break;
		case   4: keys[KEY_RLOGO] = bit; break;
		case   5: keys[KEY_LALT] = bit; break;
		case   6: keys[KEY_RALT] = bit; break;
		case   8: break; // Always on?
		case  13: keys[KEY_RCONTROL] = bit; break;
		case  16: keys[KEY_CAPSLOCK] = bit; break;
		case  17: break; // AllShift
		case  18: break; // AllCtrl
		case  19: break; // AllAlt
		case  20: break; // AllLogo
		case  23: keys[KEY_FN] = bit; break;
		default: break;
		}
	}
}



static void mouse_move(NSEvent * theEvent)
{
	mouse_x = theEvent.locationInWindow.x * sys_dpi;
	mouse_y = vid_height-(theEvent.locationInWindow.y + y_correction) * sys_dpi;
	mickey_x -= theEvent.deltaX * sys_dpi;
	mickey_y -= theEvent.deltaY * sys_dpi;
}

@implementation MyApp



-(void)sendEvent:(NSEvent *)theEvent
{
// https://developer.apple.com/library/mac/documentation/Cocoa/Reference/ApplicationKit/Classes/NSEvent_Class/#//apple_ref/c/tdef/NSEventType
	int bit=0;
	switch(theEvent.type) {
	case NSEventTypeLeftMouseDown:
		bit = 1;
	case NSEventTypeLeftMouseUp:
		mouse[0] = bit;
		mouse_move(theEvent);
		break;
	case NSEventTypeRightMouseDown:
		bit = 1;
	case NSEventTypeRightMouseUp:
		mouse[1] = bit;
		mouse_move(theEvent);
		break;
	case NSEventTypeOtherMouseDown:
		bit = 1;
	case NSEventTypeOtherMouseUp:
		switch(theEvent.buttonNumber) {
		case 2: mouse[2] = bit; break;
		case 3: mouse[3] = bit; break;
		case 4: mouse[4] = bit; break;
		case 5: mouse[5] = bit; break;
		case 6: mouse[6] = bit; break;
		case 7: mouse[7] = bit; break;
		default: printf("Unexpected Mouse Button %d\n", (int)theEvent.buttonNumber); break;
		}
		mouse_move(theEvent);
		break;

	case NSEventTypeMouseMoved:
	case NSEventTypeLeftMouseDragged:
	case NSEventTypeRightMouseDragged:
	case NSEventTypeOtherMouseDragged:
		mouse_move(theEvent);
		break;
	case NSEventTypeScrollWheel:
		break;

	case NSEventTypeMouseEntered:
	case NSEventTypeMouseExited:
		mouse[0] = 0;
		break;

	case NSEventTypeKeyDown:
		keys[theEvent.keyCode] = 1;
		break;
	case NSEventTypeKeyUp:
		keys[theEvent.keyCode] = 0;
//		log_info("MyApp:key");
		break;

	case NSEventTypeFlagsChanged:
		flags_changed(theEvent);
		break;

/*
	case NSEventTypeAppKitDefined:
		switch(theEvent.subtype) {
		case NSEventSubtypeWindowExposed:
			log_info("NSEventSubtypeWindowExposed");
			break;
		case NSEventSubtypeApplicationActivated:
			log_info("NSEventSubtypeApplicationActivated");
			break;
		case NSEventSubtypeApplicationDeactivated:
			log_info("NSEventSubtypeApplicationDeactivated");
			break;
		case NSEventSubtypeWindowMoved:
			log_info("NSEventSubtypeWindowMoved");
			break;
		case NSEventSubtypeScreenChanged:
			log_info("NSEventSubtypeScreenChanged");
			break;
		default:
			break;
		}
		log_info("event.appkit.subtype = %d", theEvent.subtype );
		break;
	case NSEventTypeSystemDefined:
		switch(theEvent.subtype) {
		case NSEventSubtypePowerOff:
			log_info("NSEventSubtypePowerOff");
			break;
		default:
			log_info("system.subtype = %d", theEvent.subtype);
			break;
		}
		log_info("event.system.subtype = %d", theEvent.subtype );

		break;
*/
	default:
//		log_info("MyApp:sendEvent = %d", theEvent.type );
		break;
	}
	[super sendEvent:theEvent];
}

@end


int main(int argc, char * argv[])
{
	log_init();
	log_info("Platform    : MacOS");

	NSApplication * myapp = [MyApp sharedApplication];
	AppDelegate * appd = [[AppDelegate alloc] init];
	[myapp setDelegate:appd];

	NSRect contentSize = NSMakeRect(10.0, 500.0, 640.0, 360.0);
	NSUInteger windowStyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
	window = [[NSWindow alloc] initWithContentRect:contentSize
		styleMask:windowStyleMask
		backing:NSBackingStoreBuffered
		defer:NO];
	WindowDelegate * wdg = [[WindowDelegate alloc] init];
	[window setDelegate: wdg];

	window.backgroundColor = [NSColor whiteColor];
	[window setTitle: @"Kittens"];

	[window makeKeyAndOrderFront:window];

//	[window setCollectionBehavior:(NSWindowCollectionBehaviorFullScreenPrimary)];

	NSView * view = [[View alloc] init];
	[window setContentView:view];
	view.wantsLayer = YES;
	pView = [view layer];
//	log_warning("layer = %s", [[[view layer] description] cStringUsingEncoding:typeUTF8Text]);


	if( main_init(argc, argv) )
	{
		killme = 1;
		log_info("Shutdown on : Init Failed");
	}

	[myapp run];
	[myapp setDelegate:nil];

	main_end();
	return 0;
}
