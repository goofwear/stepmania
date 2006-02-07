#include "global.h"
#include "LowLevelWindow_Win32.h"
#include "archutils/Win32/GraphicsWindow.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageDisplay.h"

#include <GL/gl.h>

static PIXELFORMATDESCRIPTOR g_CurrentPixelFormat;
static HGLRC g_HGLRC = NULL;
static HGLRC g_HGLRC_Background = NULL;

static void DestroyGraphicsWindowAndOpenGLContext()
{
	if( g_HGLRC != NULL )
	{
		wglMakeCurrent( NULL, NULL );
		wglDeleteContext( g_HGLRC );
		g_HGLRC = NULL;
	}

	if( g_HGLRC_Background != NULL )
	{
		wglDeleteContext( g_HGLRC_Background );
		g_HGLRC_Background = NULL;
	}

	ZERO( g_CurrentPixelFormat );

	GraphicsWindow::DestroyGraphicsWindow();
}

void *LowLevelWindow_Win32::GetProcAddress( RString s )
{
	void *pRet = wglGetProcAddress( s );
	if( pRet != NULL )
		return pRet;

	return GetProcAddress( s );
}

LowLevelWindow_Win32::LowLevelWindow_Win32()
{
	ASSERT( g_HGLRC == NULL );
	ASSERT( g_HGLRC_Background == NULL );

	GraphicsWindow::Initialize( false );
}

LowLevelWindow_Win32::~LowLevelWindow_Win32()
{
	DestroyGraphicsWindowAndOpenGLContext();
	GraphicsWindow::Shutdown();
}

void LowLevelWindow_Win32::GetDisplayResolutions( DisplayResolutions &out ) const
{
	GraphicsWindow::GetDisplayResolutions( out );
}

float LowLevelWindow_Win32::GetMonitorAspectRatio() const
{
	return GraphicsWindow::GetMonitorAspectRatio();
}

int ChooseWindowPixelFormat( const VideoModeParams &p, PIXELFORMATDESCRIPTOR *PixelFormat )
{
	ASSERT( GraphicsWindow::GetHwnd() != NULL );
	ASSERT( GraphicsWindow::GetHDC() != NULL );

	ZERO( *PixelFormat );
	PixelFormat->nSize		= sizeof(PIXELFORMATDESCRIPTOR);
	PixelFormat->nVersion		= 1;
	PixelFormat->dwFlags		= PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL;
	PixelFormat->iPixelType		= PFD_TYPE_RGBA;
	PixelFormat->cColorBits		= p.bpp == 16? 16:24;
	PixelFormat->cDepthBits		= 16;

	return ChoosePixelFormat( GraphicsWindow::GetHDC(), PixelFormat );
}

void DumpPixelFormat( const PIXELFORMATDESCRIPTOR &pfd )
{
	RString str = ssprintf( "Mode: " );
	bool bInvalidFormat = false;

	if( pfd.dwFlags & PFD_GENERIC_FORMAT )
	{
		if( pfd.dwFlags & PFD_GENERIC_ACCELERATED ) str += "MCD ";
		else { str += "software "; bInvalidFormat = true; }
	}
	else
	{
		str += "ICD ";
	}

	if( pfd.iPixelType != PFD_TYPE_RGBA ) { str += "indexed "; bInvalidFormat = true; }
	if( !(pfd.dwFlags & PFD_SUPPORT_OPENGL) ) { str += "!OPENGL "; bInvalidFormat = true; }
	if( !(pfd.dwFlags & PFD_DRAW_TO_WINDOW) ) { str += "!window "; bInvalidFormat = true; }
	if( !(pfd.dwFlags & PFD_DOUBLEBUFFER) ) { str += "!dbuff "; bInvalidFormat = true; }

	str += ssprintf( "%i (%i%i%i) ", pfd.cColorBits, pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits );
	if( pfd.cAlphaBits ) str += ssprintf( "%i alpha ", pfd.cAlphaBits );
	if( pfd.cDepthBits ) str += ssprintf( "%i depth ", pfd.cDepthBits );
	if( pfd.cStencilBits ) str += ssprintf( "%i stencil ", pfd.cStencilBits );
	if( pfd.cAccumBits ) str += ssprintf( "%i accum ", pfd.cAccumBits );

	if( bInvalidFormat )
		LOG->Warn( "Invalid format: %s", str.c_str() );
	else
		LOG->Info( "%s", str.c_str() );
}

/* This function does not reset the video mode if it fails, because we might be trying
 * yet another video mode, so we'd just thrash the display.  On fatal error,
 * LowLevelWindow_Win32::~LowLevelWindow_Win32 will call GraphicsWindow::Shutdown(). */
RString LowLevelWindow_Win32::TryVideoMode( const VideoModeParams &p, bool &bNewDeviceOut )
{
	//LOG->Warn( "LowLevelWindow_Win32::TryVideoMode" );
	
	ASSERT_M( p.bpp == 16 || p.bpp == 32, ssprintf("%i", p.bpp) );

	bNewDeviceOut = false;

	/* We're only allowed to change the pixel format of a window exactly once. */
	bool bCanSetPixelFormat = true;

	/* Do we have an old window? */
	if( GraphicsWindow::GetHwnd() == NULL )
	{
		/* No.  Always create and show the window before changing the video mode.
		 * Otherwise, some other window may have focus, and changing the video mode will
		 * cause that window to be resized. */
		GraphicsWindow::CreateGraphicsWindow( p );
	} else {
		/* We already have a window.  Assume that it's pixel format has already been
		 * set. */
		bCanSetPixelFormat = false;
	}
	
	ASSERT( GraphicsWindow::GetHwnd() );

	/* Set the display mode: switch to a fullscreen mode or revert to windowed mode. */
	LOG->Trace("SetScreenMode ...");
	RString sErr = GraphicsWindow::SetScreenMode( p );
	if( !sErr.empty() )
		return sErr;

	PIXELFORMATDESCRIPTOR PixelFormat;
	int iPixelFormat = ChooseWindowPixelFormat( p, &PixelFormat );
	if( iPixelFormat == 0 )
	{
		/* Destroy the window. */
		DestroyGraphicsWindowAndOpenGLContext();
		return "Pixel format not found";
	}

	bool bNeedToSetPixelFormat = false;
	{
		/* We'll need to recreate it if the pixel format is going to change.  We
		 * aren't allowed to change the pixel format twice. */
		PIXELFORMATDESCRIPTOR DestPixelFormat;
		ZERO( DestPixelFormat );
		DescribePixelFormat( GraphicsWindow::GetHDC(), iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &DestPixelFormat );
		if( memcmp( &DestPixelFormat, &g_CurrentPixelFormat, sizeof(PIXELFORMATDESCRIPTOR) ) )
		{
			LOG->Trace("Reset: pixel format changing" );
			bNeedToSetPixelFormat = true;
		}
	}

	if( bNeedToSetPixelFormat && !bCanSetPixelFormat )
	{
		/*
		 * The screen mode has changed, so we need to set the pixel format.  If we're
		 * not allowed to do so, destroy the window and make a new one.
		 *
		 * For some reason, if we destroy the old window before creating the new one,
		 * the "maximized apps go under the taskbar" glitch will happen when we quit.
		 * We have to create the new window first.
		 */
		LOG->Trace( "Mode requires new pixel format, and we've already set one; resetting OpenGL context" );
		if( g_HGLRC != NULL )
		{
			wglMakeCurrent( NULL, NULL );
			wglDeleteContext( g_HGLRC );
			g_HGLRC = NULL;
			wglDeleteContext( g_HGLRC_Background );
			g_HGLRC_Background = NULL;
		}

		bNewDeviceOut = true;
	}

	/* If we deleted the OpenGL context above, also recreate the window.  Otherwise, just
	 * reconfigure it. */
	GraphicsWindow::CreateGraphicsWindow( p, bNewDeviceOut );

	if( bNeedToSetPixelFormat )
	{
		/* Set the pixel format. */
		if( !SetPixelFormat(GraphicsWindow::GetHDC(), iPixelFormat, &PixelFormat) )
		{
			/* Destroy the window. */
			DestroyGraphicsWindowAndOpenGLContext();

			return werr_ssprintf( GetLastError(), "Pixel format failed" );
		}

		DescribePixelFormat( GraphicsWindow::GetHDC(), iPixelFormat, sizeof(g_CurrentPixelFormat), &g_CurrentPixelFormat );

		DumpPixelFormat( g_CurrentPixelFormat );
	}

	if( g_HGLRC == NULL )
	{
		g_HGLRC = wglCreateContext( GraphicsWindow::GetHDC() );
		if ( g_HGLRC == NULL )
		{
			DestroyGraphicsWindowAndOpenGLContext();
			return hr_ssprintf( GetLastError(), "wglCreateContext" );
		}

		g_HGLRC_Background = wglCreateContext( GraphicsWindow::GetHDC() );
		if( g_HGLRC_Background == NULL )
		{
			DestroyGraphicsWindowAndOpenGLContext();
			return hr_ssprintf( GetLastError(), "wglCreateContext" );
		}

		if( !wglShareLists(g_HGLRC, g_HGLRC_Background) )
		{
			LOG->Warn( werr_ssprintf(GetLastError(), "wglShareLists failed") );
			wglDeleteContext( g_HGLRC_Background );
			g_HGLRC_Background = NULL;
		}

		if( !wglMakeCurrent( GraphicsWindow::GetHDC(), g_HGLRC ) )
		{
			DestroyGraphicsWindowAndOpenGLContext();
			return hr_ssprintf( GetLastError(), "wglCreateContext" );
		}
	}
	return RString();	// we set the video mode successfully
}

bool LowLevelWindow_Win32::SupportsThreadedRendering()
{
	return g_HGLRC_Background != NULL;
}
	
void LowLevelWindow_Win32::BeginConcurrentRendering()
{
	if( !wglMakeCurrent( GraphicsWindow::GetHDC(), g_HGLRC_Background ) )
	{
		LOG->Warn( hr_ssprintf(GetLastError(), "wglMakeCurrent") );
		FAIL_M( hr_ssprintf(GetLastError(), "wglMakeCurrent") );
	}
}

void LowLevelWindow_Win32::EndConcurrentRendering()
{
	wglMakeCurrent( NULL, NULL );
}

bool LowLevelWindow_Win32::IsSoftwareRenderer( RString &sError )
{
	if( strcmp((const char*)glGetString(GL_VENDOR),"Microsoft Corporation") &&
		strcmp((const char*)glGetString(GL_RENDERER),"GDI Generic") )
		return false;

	sError = "OpenGL hardware acceleration is not available.";
	return true;
}

void LowLevelWindow_Win32::SwapBuffers()
{
	::SwapBuffers( GraphicsWindow::GetHDC() );
}

void LowLevelWindow_Win32::Update()
{
	GraphicsWindow::Update();
}

const VideoModeParams &LowLevelWindow_Win32::GetActualVideoModeParams() const
{
	return GraphicsWindow::GetParams();
}

/*
 * (c) 2004 Glenn Maynard
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
