#include "global.h"
#include "RageUtil.h"

#include "LoadingWindow_Win32.h"
#include "RageFileManager.h"
#include "archutils/win32/WindowsResources.h"
#include "archutils/win32/WindowIcon.h"
#include <windows.h>
#include "RageSurface_Load.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageLog.h"
#include "ProductInfo.h"
#include "LocalizedString.h"

#include "RageSurfaceUtils_Zoom.h"
static HBITMAP g_hBitmap = NULL;

/* Load a RageSurface into a GDI surface. */
static HBITMAP LoadWin32Surface( RageSurface *&s )
{
	RageSurfaceUtils::ConvertSurface( s, s->w, s->h, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0 );

	HDC hScreen = GetDC(NULL);

	HBITMAP bitmap = CreateCompatibleBitmap( hScreen, s->w, s->h );
	ASSERT( bitmap );

	HDC BitmapDC = CreateCompatibleDC( hScreen );
	SelectObject( BitmapDC, bitmap );

	/* This is silly, but simple.  We only do this once, on a small image. */
	for( int y = 0; y < s->h; ++y )
	{
		unsigned const char *line = ((unsigned char *) s->pixels) + (y * s->pitch);
		for( int x = 0; x < s->w; ++x )
		{
			unsigned const char *data = line + (x*s->format->BytesPerPixel);
			
			SetPixelV( BitmapDC, x, y, RGB( data[3], data[2], data[1] ) );
		}
	}

	SelectObject( BitmapDC, NULL );
	DeleteObject( BitmapDC );

	ReleaseDC( NULL, hScreen );

	return bitmap;
}

static HBITMAP LoadWin32Surface( RString sFile, HWND hWnd )
{
	RString error;
	RageSurface *pSurface = RageSurfaceUtils::LoadFile( sFile, error );
	if( pSurface == NULL )
		return NULL;

	/* Resize the splash image to fit the dialog.  Stretch to fit horizontally,
	 * maintaining aspect ratio. */
	{
		RECT r;
		GetClientRect( hWnd, &r );

		int iWidth = r.right;
		float fRatio = (float) iWidth / pSurface->w;
		int iHeight = lrintf( pSurface->h * fRatio );

		RageSurfaceUtils::Zoom( pSurface, iWidth, iHeight );
	}

	HBITMAP ret = LoadWin32Surface( pSurface );
	delete pSurface;
	return ret;
}

BOOL CALLBACK LoadingWindow_Win32::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_INITDIALOG:
		{
			vector<RString> vs;
			GetDirListing( "Data/splash*.png", vs, false, true );
			if( !vs.empty() )
				g_hBitmap = LoadWin32Surface( vs[0], hWnd );
		}
		if( g_hBitmap == NULL )
			g_hBitmap = LoadWin32Surface( "Data/splash.bmp", hWnd );
		SendMessage( 
			GetDlgItem(hWnd,IDC_SPLASH), 
			STM_SETIMAGE, 
			(WPARAM) IMAGE_BITMAP, 
			(LPARAM) (HANDLE) g_hBitmap );
		SetWindowTextA( hWnd, PRODUCT_ID );
		break;

	case WM_DESTROY:
		DeleteObject( g_hBitmap );
		g_hBitmap = NULL;
		break;
	}

	return FALSE;
}

void LoadingWindow_Win32::SetIcon( const RageSurface *pIcon )
{
	if( m_hIcon != NULL )
		DestroyIcon( m_hIcon );

	m_hIcon = IconFromSurface( pIcon );
	if( m_hIcon != NULL )
		SetClassLong( hwnd, GCL_HICON, (LONG) m_hIcon );
}

LoadingWindow_Win32::LoadingWindow_Win32()
{
	m_hIcon = NULL;
	hwnd = CreateDialog( handle.Get(), MAKEINTRESOURCE(IDD_LOADING_DIALOG), NULL, WndProc );
	for( unsigned i = 0; i < 3; ++i )
		text[i] = "ABC"; /* always set on first call */
	SetText( "" );
	Paint();
}

LoadingWindow_Win32::~LoadingWindow_Win32()
{
	if( hwnd )
		DestroyWindow( hwnd );
	if( m_hIcon != NULL )
		DestroyIcon( m_hIcon );
}

void LoadingWindow_Win32::Paint()
{
	SendMessage( hwnd, WM_PAINT, 0, 0 );

	/* Process all queued messages since the last paint.  This allows the window to
	 * come back if it loses focus during load. */
	MSG msg;
	while( PeekMessage( &msg, hwnd, 0, 0, PM_NOREMOVE ) )
	{
		GetMessage(&msg, hwnd, 0, 0 );
		DispatchMessage( &msg );
	}
}

void LoadingWindow_Win32::SetText( RString sText )
{
	vector<RString> asMessageLines;
	split( sText, "\n", asMessageLines, false );
	while( asMessageLines.size() < 3 )
		asMessageLines.push_back( "" );
	
	const int msgid[] = { IDC_STATIC_MESSAGE1, IDC_STATIC_MESSAGE2, IDC_STATIC_MESSAGE3 };
	for( unsigned i = 0; i < 3; ++i )
	{
		if( text[i] == asMessageLines[i] )
			continue;
		text[i] = asMessageLines[i];

		HWND hwndItem = ::GetDlgItem( hwnd, msgid[i] );
		
		::SetWindowText( hwndItem, ConvertUTF8ToACP(asMessageLines[i]).c_str() );
	}
}

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
