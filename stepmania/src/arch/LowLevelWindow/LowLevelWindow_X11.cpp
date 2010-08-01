#include "global.h"
#include "LowLevelWindow_X11.h"
#include "RageLog.h"
#include "RageException.h"
#include "archutils/Unix/X11Helper.h"
#include "PrefsManager.h" // XXX
#include "RageDisplay.h" // VideoModeParams
#include "DisplayResolutions.h"
#include "LocalizedString.h"

#include <stack>
#include <math.h>	// ceil()
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>	// All sorts of stuff...
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#if defined(HAVE_LIBXTST)
#include <X11/extensions/XTest.h>
#endif

static GLXContext g_pContext = NULL;
static GLXContext g_pBackgroundContext = NULL;
static Window g_AltWindow = 0;

static LocalizedString FAILED_CONNECTION_XSERVER( "LowLevelWindow_X11", "Failed to establish a connection with the X server" );
LowLevelWindow_X11::LowLevelWindow_X11()
{
	if( !X11Helper::Go() )
		RageException::Throw( FAILED_CONNECTION_XSERVER.GetValue() );

	const int iScreen = DefaultScreen( X11Helper::Dpy );

	LOG->Info( "Display: %s (screen %i)", DisplayString(X11Helper::Dpy), iScreen );
	LOG->Info( "Direct rendering: %s", glXIsDirect( X11Helper::Dpy, glXGetCurrentContext() )? "yes":"no" );

	int iXServerVersion = XVendorRelease( X11Helper::Dpy ); /* eg. 40201001 */
	int iMajor = iXServerVersion / 10000000; iXServerVersion %= 10000000;
	int iMinor = iXServerVersion / 100000;   iXServerVersion %= 100000;
	int iRevision = iXServerVersion / 1000;  iXServerVersion %= 1000;
	int iPatch = iXServerVersion;

	LOG->Info( "X server vendor: %s [%i.%i.%i.%i]", XServerVendor( X11Helper::Dpy ), iMajor, iMinor, iRevision, iPatch );
	LOG->Info( "Server GLX vendor: %s [%s]", glXQueryServerString( X11Helper::Dpy, iScreen, GLX_VENDOR ), glXQueryServerString( X11Helper::Dpy, iScreen, GLX_VERSION ) );
	LOG->Info( "Client GLX vendor: %s [%s]", glXGetClientString( X11Helper::Dpy, GLX_VENDOR ), glXGetClientString( X11Helper::Dpy, GLX_VERSION ) );
	
	m_bWasWindowed = true;
}

LowLevelWindow_X11::~LowLevelWindow_X11()
{
	// Reset the display
	if( !m_bWasWindowed )
	{
		XRRScreenConfiguration *pScreenConfig = XRRGetScreenInfo( X11Helper::Dpy, RootWindow( X11Helper::Dpy, DefaultScreen( X11Helper::Dpy ) ) );
		XRRSetScreenConfig( X11Helper::Dpy, pScreenConfig, RootWindow( X11Helper::Dpy, DefaultScreen( X11Helper::Dpy ) ), 0, 1, CurrentTime );
		XRRFreeScreenConfigInfo( pScreenConfig );
		
		XUngrabKeyboard( X11Helper::Dpy, CurrentTime );
	}
	X11Helper::Stop();	// Xlib cleans up the window for us
}

void *LowLevelWindow_X11::GetProcAddress( RString s )
{
	// XXX: We should check whether glXGetProcAddress or
	// glXGetProcAddressARB is available, and go by that, instead of
	// assuming like this.
	return (void*) glXGetProcAddressARB( (const GLubyte*) s.c_str() );
}

RString LowLevelWindow_X11::TryVideoMode( const VideoModeParams &p, bool &bNewDeviceOut )
{
#if defined(LINUX)
	/*
	 * nVidia cards:
	 *
	 * This only works the first time we set up a window; after that, the
	 * drivers appear to cache the value, so you have to actually restart
	 * the program to change it again.
	 */
	static char buf[128];
	strcpy( buf, "__GL_SYNC_TO_VBLANK=" );
	strcat( buf, p.vsync?"1":"0" );
	putenv( buf );
#endif

	XSizeHints hints;
	XEvent ev;
	stack<XEvent> otherEvs;

	hints.flags = PBaseSize;
	hints.base_width = p.width;
	hints.base_height = p.height;

	if( g_pContext == NULL || p.bpp != CurrentParams.bpp || m_bWasWindowed != p.windowed )
	{
		// Different depth, or we didn't make a window before. New context.
		bNewDeviceOut = true;

		int visAttribs[32];
		int i = 0;
		ASSERT( p.bpp == 16 || p.bpp == 32 );
		if( p.bpp == 32 )
		{
			visAttribs[i++] = GLX_RED_SIZE;		visAttribs[i++] = 8;
			visAttribs[i++] = GLX_GREEN_SIZE;	visAttribs[i++] = 8;
			visAttribs[i++] = GLX_BLUE_SIZE;	visAttribs[i++] = 8;
		}
		else
		{
			visAttribs[i++] = GLX_RED_SIZE;		visAttribs[i++] = 5;
			visAttribs[i++] = GLX_GREEN_SIZE;	visAttribs[i++] = 6;
			visAttribs[i++] = GLX_BLUE_SIZE;	visAttribs[i++] = 5;
		}

		visAttribs[i++] = GLX_DEPTH_SIZE;	visAttribs[i++] = 16;
		visAttribs[i++] = GLX_RGBA;
		visAttribs[i++] = GLX_DOUBLEBUFFER;

		visAttribs[i++] = None;

		XVisualInfo *xvi = glXChooseVisual( X11Helper::Dpy, DefaultScreen(X11Helper::Dpy), visAttribs );

		if( xvi == NULL )
			return "No visual available for that depth.";

		/* Enable StructureNotifyMask, so we receive a MapNotify for the following XMapWindow. */
		X11Helper::OpenMask( StructureNotifyMask );

		// I get strange behavior if I add override redirect after creating the window.
		// So, let's recreate the window when changing that state.
		if( !X11Helper::MakeWindow(xvi->screen, xvi->depth, xvi->visual, p.width, p.height, !p.windowed) )
			return "Failed to create the window.";
		
		g_AltWindow = X11Helper::CreateWindow(xvi->screen, xvi->depth, xvi->visual, p.width, p.height, !p.windowed);
		ASSERT( g_AltWindow );

		char *szWindowTitle = const_cast<char *>( p.sWindowTitle.c_str() );
		XChangeProperty( X11Helper::Dpy, X11Helper::Win, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
				reinterpret_cast<unsigned char*>(szWindowTitle), strlen(szWindowTitle) );

		g_pContext = glXCreateContext( X11Helper::Dpy, xvi, NULL, True );
		g_pBackgroundContext = glXCreateContext( X11Helper::Dpy, xvi, g_pContext, True );

		glXMakeCurrent( X11Helper::Dpy, X11Helper::Win, g_pContext );

		XMapWindow( X11Helper::Dpy, X11Helper::Win );

		// HACK: Wait for the MapNotify event, without spinning and
		// eating CPU unnecessarily, and without smothering other
		// events. Do this by grabbing all events, remembering
		// uninteresting events, and putting them back on the queue
		// after MapNotify arrives.
		while(1)
		{
			XNextEvent( X11Helper::Dpy, &ev );
			if( ev.type == MapNotify )
				break;

			otherEvs.push( ev );
		}

		while( !otherEvs.empty() )
		{
			XPutBackEvent( X11Helper::Dpy, &otherEvs.top() );
			otherEvs.pop();
		}

		X11Helper::CloseMask( StructureNotifyMask );

	}
	else
	{
		// We're remodeling the existing window, and not touching the
		// context.
		bNewDeviceOut = false;
	}
	
	XRRScreenConfiguration *pScreenConfig = XRRGetScreenInfo( X11Helper::Dpy, RootWindow( X11Helper::Dpy, DefaultScreen(X11Helper::Dpy) ) );
	
	if( !p.windowed )
	{
		// Find a matching mode.
		int iSizesXct;
		XRRScreenSize *pSizesX = XRRSizes( X11Helper::Dpy, DefaultScreen( X11Helper::Dpy ), &iSizesXct );
		ASSERT_M( iSizesXct != 0, "Couldn't get resolution list from X server" );
	
		int iSizeMatch = -1;
		
		for( int i = 0; i < iSizesXct; ++i )
		{
			if( pSizesX[i].width == p.width && pSizesX[i].height == p.height )
			{
				iSizeMatch = i;
				break;
			}
		}

		// Set this mode.
		// XXX: This doesn't handle if the config has changed since we queried it (see man Xrandr)
		XRRSetScreenConfig( X11Helper::Dpy, pScreenConfig, RootWindow( X11Helper::Dpy, DefaultScreen(X11Helper::Dpy) ), iSizeMatch, 1, CurrentTime );
		
		// Move the window to the corner that the screen focuses in on.
		XMoveWindow( X11Helper::Dpy, X11Helper::Win, 0, 0 );
		
		XRaiseWindow( X11Helper::Dpy, X11Helper::Win );
		
		if( m_bWasWindowed )
		{
			// We want to prevent the WM from catching anything that comes from the keyboard.
			XGrabKeyboard( X11Helper::Dpy, X11Helper::Win, True,
				GrabModeAsync, GrabModeAsync, CurrentTime );
			m_bWasWindowed = false;
		}
	}
	else
	{
		if( !m_bWasWindowed )
		{
			XRRSetScreenConfig( X11Helper::Dpy, pScreenConfig, RootWindow( X11Helper::Dpy, DefaultScreen( X11Helper::Dpy ) ), 0, 1, CurrentTime );
			// In windowed mode, we actually want the WM to function normally.
			// Release any previous grab.
			XUngrabKeyboard( X11Helper::Dpy, CurrentTime );
			m_bWasWindowed = true;
		}
	}
	int rate = XRRConfigCurrentRate( pScreenConfig );

	XRRFreeScreenConfigInfo( pScreenConfig );

	// Do this before resizing the window so that pane-style WMs (Ion,
	// ratpoison) don't resize us back inappropriately.
	XSetWMNormalHints( X11Helper::Dpy, X11Helper::Win, &hints );

	// Do this even if we just created the window -- works around Ion2 not
	// catching WM normal hints changes in mapped windows.
	XResizeWindow( X11Helper::Dpy, X11Helper::Win, p.width, p.height );

	CurrentParams = p;
	CurrentParams.rate = rate;
	return ""; // Success
}

bool LowLevelWindow_X11::IsSoftwareRenderer( RString &sError )
{
	if( glXIsDirect( X11Helper::Dpy, glXGetCurrentContext() ) )
		return false;

	sError = "Direct rendering is not available.";
	return true;
}

void LowLevelWindow_X11::SwapBuffers()
{
	glXSwapBuffers( X11Helper::Dpy, X11Helper::Win );

	if( PREFSMAN->m_bDisableScreenSaver )
	{
		/* Disable the screensaver. */
#if defined(HAVE_LIBXTST)
		/* This causes flicker. */
		// XForceScreenSaver( X11Helper::Dpy, ScreenSaverReset );
		
		/*
		 * Instead, send a null relative mouse motion, to trick X into thinking there has been
		 * user activity. 
		 *
		 * This also handles XScreenSaver; XForceScreenSaver only handles the internal X11
		 * screen blanker.
		 *
		 * This will delay the X blanker, DPMS and XScreenSaver from activating, and will
		 * disable the blanker and XScreenSaver if they're already active (unless XSS is
		 * locked).  For some reason, it doesn't un-blank DPMS if it's already active.
		 */

		XLockDisplay( X11Helper::Dpy );

		int event_base, error_base, major, minor;
		if( XTestQueryExtension( X11Helper::Dpy, &event_base, &error_base, &major, &minor ) )
		{
			XTestFakeRelativeMotionEvent( X11Helper::Dpy, 0, 0, 0 );
			XSync( X11Helper::Dpy, False );
		}

		XUnlockDisplay( X11Helper::Dpy );
#endif
	}
}

void LowLevelWindow_X11::GetDisplayResolutions( DisplayResolutions &out ) const
{
	int iSizesXct;
	XRRScreenSize *pSizesX = XRRSizes( X11Helper::Dpy, DefaultScreen( X11Helper::Dpy ), &iSizesXct );
	ASSERT_M( iSizesXct != 0, "Couldn't get resolution list from X server" );
	
	for( int i = 0; i < iSizesXct; ++i )
	{
		DisplayResolution res = { pSizesX[i].width, pSizesX[i].height, true };
		out.insert( res );
	}
}

float LowLevelWindow_X11::GetMonitorAspectRatio() const
{
	return 4.0f/3;
}

bool LowLevelWindow_X11::SupportsThreadedRendering()
{
	return g_pBackgroundContext != NULL;
}

void LowLevelWindow_X11::BeginConcurrentRenderingMainThread()
{
	/* Move the main thread, which is going to be loading textures, etc. but
	 * not rendering, to an undisplayed window.  This results in smoother
	 * rendering. */
	bool b = glXMakeCurrent( X11Helper::Dpy, g_AltWindow, g_pContext );
	ASSERT(b);
}

void LowLevelWindow_X11::EndConcurrentRenderingMainThread()
{
	bool b = glXMakeCurrent( X11Helper::Dpy, X11Helper::Win, g_pContext );
	ASSERT(b);
}

void LowLevelWindow_X11::BeginConcurrentRendering()
{
	bool b = glXMakeCurrent( X11Helper::Dpy, X11Helper::Win, g_pBackgroundContext );
	ASSERT(b);
}

void LowLevelWindow_X11::EndConcurrentRendering()
{
	bool b = glXMakeCurrent( X11Helper::Dpy, None, NULL );
	ASSERT(b);
}

/*
 * (c) 2005 Ben Anderson
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
