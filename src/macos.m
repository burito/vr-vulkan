#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>

#include <sys/time.h>
#include <MoltenVK/mvk_vulkan.h>
#include "log.h"
///////////////////////////////////////////////////////////////////////////////
//////// Public Interface to the rest of the program
///////////////////////////////////////////////////////////////////////////////
#include "keyboard.h"

int killme=0;
int sys_width  = 1980;	/* dimensions of default screen */
int sys_height = 1200;
float sys_dpi = 0.5;
int vid_width  = 1280;	/* dimensions of our part of the screen */
int vid_height = 720;
int mouse_x = 0;
int mouse_y = 0;
int mickey_x = 0;
int mickey_y = 0;
char mouse[] = {0,0,0,0,0,0,0,0};
// the Logitech drivers are happy to send up to 8 numbered "mouse" buttons
// http://www.logitech.com/pub/techsupport/mouse/mac/lcc3.9.1.b20.zip

#define KEYMAX 512
char keys[KEYMAX];

int fullscreen = 0;
int fullscreen_toggle = 0;

int main_init(int argc, char *argv[]);
void main_loop(void);
void main_end(void);

const uint64_t sys_ticksecond = 1000000000;
static uint64_t sys_time_start = 0;
uint64_t sys_time(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	return (ts.tv_sec * 1000000000 + ts.tv_nsec) - sys_time_start;
}

void shell_browser(char *url)
{
	NSURL *MyNSURL = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
	[[NSWorkspace sharedWorkspace] openURL:MyNSURL];
}
///////////////////////////////////////////////////////////////////////////////
//////// End Public Interface
///////////////////////////////////////////////////////////////////////////////
void * pView = NULL;


static CVDisplayLinkRef _displayLink;
NSWindow *window;
NSView * window_view = NULL;

static int y_correction = 0;	// to correct mouse position for title bar


//
// NSView
//

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
	return kCVReturnSuccess;
}

-(void) dealloc
{
	log_debug("View:dealloc");
	// main_shutdown();
	CVDisplayLinkRelease(_displayLink);
	[super dealloc];
}

-(BOOL) wantsUpdateLayer { return YES; }

+(Class) layerClass { return [CAMetalLayer class]; }

-(CALayer*) makeBackingLayer
{
//	log_debug("View:makeBackingLayer");
	CALayer *layer = [self.class.layerClass layer];
	CGSize viewScale = [self convertSizeToBacking: CGSizeMake(1.0, 1.0)];
	layer.contentsScale = MIN(viewScale.width, viewScale.height);

	return layer;
}
@end


//
// App Delegate
//
@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate

- (id)init
{
	log_debug("AppDelegate:init");
	NSRect contentSize = NSMakeRect(100.0, 400.0, 640.0, 360.0);
	NSUInteger windowStyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
	window = [[NSWindow alloc] initWithContentRect:contentSize styleMask:windowStyleMask backing:NSBackingStoreBuffered defer:YES];

	[window setCollectionBehavior:(NSWindowCollectionBehaviorFullScreenPrimary)];
	return [super init];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotifcation
{
	log_debug("AppDelegate:applicationDidFinishLaunching");
	// init
	CVDisplayLinkStart(_displayLink);
	log_debug("no really, we did finish launching");
	
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	log_debug("AppDelegate:applicationWillTerminate");
	// shutdown
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)x { return YES; }
@end


@interface WindowDelegate : NSObject<NSWindowDelegate>
-(void)windowWillClose:(NSNotification*)aNotification;
@end
@implementation WindowDelegate
-(void)windowWillClose:(NSNotification*)aNotification
{
	killme = 1;
}
@end

static void mouse_move(NSEvent * theEvent)
{
	mouse_x = theEvent.locationInWindow.x * sys_dpi;
	mouse_y = vid_height-(theEvent.locationInWindow.y + y_correction) * sys_dpi;
	mickey_x -= theEvent.deltaX * sys_dpi;
	mickey_y -= theEvent.deltaY * sys_dpi;
}

static int event_handler(NSEvent *event)
{
	int bit = 0;
	switch(event.type) {
	case NSEventTypeKeyDown:
		bit = 1;
	case NSEventTypeKeyUp:
		if( event.keyCode >= KEYMAX )
		{
			log_warning("unexpected key value = %d", event.keyCode);
			return 1;
		}
		keys[event.keyCode] = bit;
		break;
	
	case NSEventTypeLeftMouseDown:
		bit = 1;
	case NSEventTypeLeftMouseUp:
		mouse[0] = bit;
		break;
	case NSEventTypeRightMouseDown:
		bit = 1;
	case NSEventTypeRightMouseUp:
		mouse[1] = bit;
		break;
	case NSEventTypeOtherMouseDown:
		bit = 1;
	case NSEventTypeOtherMouseUp:
		switch(event.buttonNumber) {
		case 2: mouse[2] = bit; break;
		case 3: mouse[3] = bit; break;
		case 4: mouse[4] = bit; break;
		case 5: mouse[5] = bit; break;
		case 6: mouse[6] = bit; break;
		case 7: mouse[7] = bit; break;
		default:
			log_warning("Unexpected Mouse Button %d", event.buttonNumber);
			break;
		}
		mouse_move(event);
		break;
	case NSEventTypeMouseMoved:
	case NSEventTypeLeftMouseDragged:
	case NSEventTypeRightMouseDragged:
	case NSEventTypeOtherMouseDragged:
		mouse_move(event);
		break;

	case NSEventTypeScrollWheel:
		break;

	case NSEventTypeMouseEntered:
	case NSEventTypeMouseExited:
		mouse[0] = 0;
		break;

	default:
		return 1;
	}
	return 0;
}


int main(int argc, char * argv[])
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	sys_time_start = ts.tv_sec * 1000000000 + ts.tv_nsec;
	log_init();
	log_info("Platform    : MacOS");

	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	NSApplication * myapp = [NSApplication sharedApplication];
	AppDelegate * appd = [[AppDelegate alloc] init];
	[myapp setDelegate:appd];
	[myapp finishLaunching];

	// build menu
	id menubar = [[NSMenu alloc] init];
	id appMenuItem = [[NSMenuItem alloc] init];
	[menubar addItem:appMenuItem];
	[NSApp setMainMenu:menubar];
	id appMenu = [[NSMenu alloc] init];
	id appName = [[NSProcessInfo processInfo] processName];
	id quitTitle = [@"Quit " stringByAppendingString:appName];
	id quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
	id fullscreenTitle = NSLocalizedString(@"Fullscreen", nil);
	[appMenu addItemWithTitle:fullscreenTitle action:@selector(toggleFullScreen:) keyEquivalent:@"f"];
	[appMenu addItem:quitMenuItem];
	[appMenuItem setSubmenu:appMenu];


	id window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0,
			vid_width*sys_dpi, vid_height*sys_dpi)
		styleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
	[window setReleasedWhenClosed:NO];
	WindowDelegate * wdg = [[WindowDelegate alloc] init];
	[window setDelegate:wdg];
//	NSView * contentView = [window contentView];
//	[contentView setWantsBestResolutionOpenGLSurface:YES];	// retina support
	[window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
	[window setTitle:@"sup"];

	// opengl init

	[window makeKeyAndOrderFront:window];
//	[window setAcceptsMouseMovedEvents:YES];
//	[window setBackgroundColor:[NSColor whiteColor]];
//	[NSApp activateIgnoringOtherApps:YES];


	NSView *view = [[View alloc] init];
	[window setContentView:view];
	view.wantsLayer = YES;
	pView = [view layer];

log_warning("layer = %s", [[(id)pView description] cStringUsingEncoding:typeUTF8Text]);


	if( main_init(argc, argv) )
		killme = 1;
//log_warning("layer = %s", [[[view layer] description] cStringUsingEncoding:typeUTF8Text]);

	// main loop
	float time = 0.0;
	while(!killme)
	{
		NSEvent * event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
		if(event) 
		{
			event_handler(event);
			[NSApp sendEvent:event];
			[NSApp updateWindows];
		}

	}

	main_end();
	[pool drain];
	return 0;
}



