#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: RageDisplay

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
    Glenn Maynard
-----------------------------------------------------------------------------
*/

#include "RageDisplay.h"
#include "RageDisplay_D3D.h"
#include "D3D8.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageTimer.h"
#include "RageException.h"
#include "RageTexture.h"
#include "RageTextureManager.h"
#include "RageMath.h"
#include "RageTypes.h"
#include "GameConstantsAndTypes.h"
#include "StepMania.h"
#include "RageUtil.h"
#include "D3dx8math.h"
#include "SDL_video.h"	// for SDL_Surface
#include "SDL_utils.h"
#include "D3DX8Core.h"

#include "arch/arch.h"

// Static libraries
#ifdef _XBOX
#pragma comment(lib, "D3d8.lib")
#endif
// load Windows D3D8 dynamically
#pragma comment(lib, "D3dx8.lib")
//#pragma comment(lib, "Dxerr8.lib")

#include <math.h>
#include <list>


CString GetErrorString( HRESULT hr )
{
	char szError[1024] = "";
	D3DXGetErrorString( hr, szError, sizeof(szError) );
	return szError;
}


//
// Globals
//
#if !defined(_XBOX)
HMODULE					g_D3D8_Module = NULL;
#endif
LPDIRECT3D8				g_pd3d = NULL;
LPDIRECT3DDEVICE8		g_pd3dDevice = NULL;
D3DCAPS8				g_DeviceCaps;
D3DDISPLAYMODE			g_DesktopMode;
D3DPRESENT_PARAMETERS	g_d3dpp;
int						g_ModelMatrixCnt=0;
RageDisplay::VideoModeParams	g_CurrentParams;

/* Direct3D doesn't associate a palette with textures.
 * Instead, we load a palette into a slot.  We need to keep track
 * of which texture's palette is stored in what slot. */
map<unsigned,int>	g_TexResourceToPaletteIndex;
list<int>			g_PaletteIndex;
struct TexturePalette { PALETTEENTRY p[256]; };
map<unsigned,TexturePalette>	g_TexResourceToTexturePalette;

/* Load the palette, if any, for the given texture into a palette slot, and make
 * it current. */
static void SetPalette( unsigned TexResource )
{
	/* If the texture isn't paletted, we have nothing to do. */
	if( g_TexResourceToTexturePalette.find(TexResource) == g_TexResourceToTexturePalette.end() )
		return;

	/* Is the palette already loaded? */
	if( g_TexResourceToPaletteIndex.find(TexResource) == g_TexResourceToPaletteIndex.end() )
	{
		/* It's not.  Grab the least recently used slot. */
		int iPalIndex = g_PaletteIndex.front();

		/* If any other texture is currently using this slot, mark that palette unloaded. */
		for( map<unsigned,int>::iterator i = g_TexResourceToPaletteIndex.begin(); i != g_TexResourceToPaletteIndex.end(); ++i )
		{
			if( i->second != iPalIndex )
				continue;
			g_TexResourceToPaletteIndex.erase(i);
			break;
		}

		/* Load it. */
#ifndef _XBOX
		TexturePalette& pal = g_TexResourceToTexturePalette[TexResource];
		g_pd3dDevice->SetPaletteEntries( iPalIndex, pal.p );
#else
		ASSERT(0);
#endif

		g_TexResourceToPaletteIndex[TexResource] = iPalIndex;
	}
	
	const int iPalIndex = g_TexResourceToPaletteIndex[TexResource];

	/* Find this palette index in the least-recently-used queue and move it to the end. */
	for(list<int>::iterator i = g_PaletteIndex.begin(); i != g_PaletteIndex.end(); ++i)
	{
		if( *i != iPalIndex )
			continue;
		g_PaletteIndex.erase(i);
		g_PaletteIndex.push_back(iPalIndex);
		break;
	}

#ifndef _XBOX
	g_pd3dDevice->SetCurrentTexturePalette( iPalIndex );
#else
	ASSERT(0);
#endif
}

#define D3DFVF_RageSpriteVertex (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#define D3DFVF_RageModelVertex (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)


static const PixelFormatDesc PIXEL_FORMAT_DESC[NUM_PIX_FORMATS] = {
	{
		/* A8B8G8R8 */
		32,
		{ 0x00FF0000,
		  0x0000FF00,
		  0x000000FF,
		  0xFF000000 }
	}, {
		/* A4R4G4B4 */
		16,
		{ 0x0F00,
		  0x00F0,
		  0x000F,
		  0xF000 },
	}, {
		/* A1B5G5R5 */
		16,
		{ 0x7C00,
		  0x03E0,
		  0x001F,
		  0x8000 },
	}, {
		/* X1R5G5R5 */
		16,
		{ 0x7C00,
		  0x03E0,
		  0x001F,
		  0x0000 },
	}, {
#if defined _XBOX
		/* A8B8G8R8 */
		32,
		{ 0x00FF0000,
		  0x0000FF00,
		  0x000000FF,
		  0xFF000000 }
#else
		/* B8G8R8 */
		24,
		{ 0xFF0000,
		  0x00FF00,
		  0x0000FF,
		  0x000000 }
#endif
	}, {
		/* Paletted */
		8,
		{ 0,0,0,0 } /* N/A */
	}, {
		/* BGR (N/A; OpenGL only) */
		0, { 0,0,0,0 }
	}, {
		/* ABGR (N/A; OpenGL only) */
		0, { 0,0,0,0 }
	}
};

static D3DFORMAT D3DFORMATS[NUM_PIX_FORMATS] = 
{
	D3DFMT_A8R8G8B8,
	D3DFMT_A4R4G4B4,
	D3DFMT_A1R5G5B5,
	D3DFMT_X1R5G5B5,
#if defined _XBOX
	D3DFMT_A8R8G8B8,
#else
	D3DFMT_R8G8B8,
#endif
	D3DFMT_P8,
	D3DFMT_UNKNOWN, /* no BGR */
	D3DFMT_UNKNOWN /* no ABGR */
};

const PixelFormatDesc *RageDisplay_D3D::GetPixelFormatDesc(PixelFormat pf) const
{
	ASSERT( pf < NUM_PIX_FORMATS );
	return &PIXEL_FORMAT_DESC[pf];
}



RageDisplay_D3D::RageDisplay_D3D( VideoModeParams p )
{
	try
	{
		LOG->Trace( "RageDisplay_D3D::RageDisplay_D3D()" );
		LOG->MapLog("renderer", "Current renderer: Direct3D");

		typedef IDirect3D8 * (WINAPI * Direct3DCreate8_t) (UINT SDKVersion);
		Direct3DCreate8_t pDirect3DCreate8;
#if defined(_XBOX)
		pDirect3DCreate8 = Direct3DCreate8;
#else
		g_D3D8_Module = LoadLibrary("D3D8.dll");
		if(!g_D3D8_Module)
			throw RageException_D3DNotInstalled();

		pDirect3DCreate8 = (Direct3DCreate8_t) GetProcAddress(g_D3D8_Module, "Direct3DCreate8");
#endif

		g_pd3d = pDirect3DCreate8( D3D_SDK_VERSION );

		if( FAILED( g_pd3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &g_DeviceCaps) ) )
			throw RageException_D3DNoAcceleration();

		D3DADAPTER_IDENTIFIER8	identifier;
		g_pd3d->GetAdapterIdentifier( D3DADAPTER_DEFAULT, 0, &identifier );

		LOG->Trace( 
			"Driver: %s\n"
			"Description: %s\n"
			"Max texture size: %d\n",
			identifier.Driver, 
			identifier.Description,
			g_DeviceCaps.MaxTextureWidth
			);

		LOG->Trace( "This display adaptor supports the following modes:" );
		D3DDISPLAYMODE mode;
		for( UINT u=0; u<g_pd3d->GetAdapterModeCount(D3DADAPTER_DEFAULT); u++ )
			if( SUCCEEDED( g_pd3d->EnumAdapterModes( D3DADAPTER_DEFAULT, u, &mode ) ) )
				LOG->Trace( "  %ux%u %uHz, format %d", mode.Width, mode.Height, mode.RefreshRate, mode.Format );

		/* Up until now, all we've done is set up g_pd3d and do some queries.  Now,
		 * actually initialize the window.  Do this after as many error conditions
		 * as possible, because if we have to shut it down again we'll flash a window
		 * briefly. */
//#if defined _WINDOWS
		if(!SDL_WasInit(SDL_INIT_VIDEO))
		{
			if( SDL_InitSubSystem(SDL_INIT_VIDEO) == -1 )
				RageException::Throw( "SDL_INIT_VIDEO failed: %s", SDL_GetError() );
		}

		/* By default, ignore all SDL events.  We'll enable them as we need them.
		 * We must not enable any events we don't actually want, since we won't
		 * query for them and they'll fill up the event queue. 
		 *
		 * This needs to be done after we initialize video, since it's really part
		 * of the SDL video system--it'll be reinitialized on us if we do this first. */
		SDL_EventState(0xFF /*SDL_ALLEVENTS*/, SDL_IGNORE);
		SDL_EventState(SDL_VIDEORESIZE, SDL_ENABLE);
//#endif

		g_PaletteIndex.clear();
		for( int i = 0; i < 256; ++i )
			g_PaletteIndex.push_back(i);

		// Save the original desktop format.
		g_pd3d->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &g_DesktopMode );

#if defined _WINDOWS
		// Create the SDL window
		int flags = SDL_RESIZABLE | SDL_SWSURFACE;
		SDL_Surface *screen = SDL_SetVideoMode(p.width, p.height, p.bpp, flags);
		if(!screen)
			RageException::Throw("SDL_SetVideoMode failed: %s", SDL_GetError());
#endif

		SetVideoMode( p );
	} catch(...) {
		// Clean up; ~RageDisplay will not be called.
#if defined _WINDOWS
		if( SDL_WasInit(SDL_INIT_VIDEO) )
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif

		if( g_pd3d )
		{
			g_pd3d->Release();
			g_pd3d = NULL;
		}

#if !defined(_XBOX)
		if( g_D3D8_Module )
		{
			FreeLibrary( g_D3D8_Module );
			g_D3D8_Module = NULL;
		}
#endif
		throw;
	}
}

void RageDisplay_D3D::Update(float fDeltaTime)
{
#if defined _WINDOWS
	SDL_Event event;
	while(SDL_GetEvent(event, SDL_VIDEORESIZEMASK))
	{
		switch(event.type)
		{
		case SDL_VIDEORESIZE:
			g_CurrentParams.width = event.resize.w;
			g_CurrentParams.height = event.resize.h;

			/* Let DISPLAY know that our resolution has changed. */
			ResolutionChanged();
			break;
		}
	}
#endif
}

bool RageDisplay_D3D::IsSoftwareRenderer()
{
	return false;
}

RageDisplay_D3D::~RageDisplay_D3D()
{
	LOG->Trace( "RageDisplay_D3D::~RageDisplay()" );

#if defined _WINDOWS
	SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif

	g_pd3dDevice->Release();
    g_pd3d->Release();
}

D3DFORMAT FindBackBufferType(bool bWindowed, int iBPP)
{
	HRESULT hr;

	// If windowed, then bpp is ignored.  Use whatever works.
    vector<D3DFORMAT> vBackBufferFormats;		// throw all possibilities in here
	
	/* When windowed, add all formats; otherwise add only formats that match dwBPP. */
	if( iBPP == 16 || bWindowed )
	{
		vBackBufferFormats.push_back( D3DFMT_R5G6B5 );
		vBackBufferFormats.push_back( D3DFMT_X1R5G5B5 );
		vBackBufferFormats.push_back( D3DFMT_A1R5G5B5 );
	}
	if( iBPP == 32 || bWindowed )
	{
#ifndef _XBOX
		vBackBufferFormats.push_back( D3DFMT_R8G8B8 );
#endif
		vBackBufferFormats.push_back( D3DFMT_X8R8G8B8 );
		vBackBufferFormats.push_back( D3DFMT_A8R8G8B8 );
	}
	if( !bWindowed && iBPP != 16 && iBPP != 32 )
	{
#if defined _WINDOWS
		SDL_QuitSubSystem(SDL_INIT_VIDEO);	// exit out of full screen.  The ~RageDisplay will not be called!
#endif
		throw RageException( ssprintf("Invalid BPP '%u' specified", iBPP) );
	}

	// Test each back buffer format until we find something that works.
	for( unsigned i=0; i < vBackBufferFormats.size(); i++ )
	{
		D3DFORMAT fmtBackBuffer = vBackBufferFormats[i];

		D3DFORMAT fmtDisplay;
		if( bWindowed )
			fmtDisplay = g_DesktopMode.Format;
		else	// Fullscreen
			fmtDisplay = vBackBufferFormats[i];

		LOG->Trace( "Testing format: display %d, back buffer %d, windowed %d...",
					fmtDisplay, fmtBackBuffer, bWindowed );

		hr = g_pd3d->CheckDeviceType( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
			fmtDisplay, fmtBackBuffer, bWindowed );

		if( FAILED(hr) )
			continue;	// skip

		// done searching
		LOG->Trace( "This will work." );
		return fmtBackBuffer;
	}

	LOG->Trace( "Couldn't find an appropriate back buffer format." );
	return (D3DFORMAT)-1;
}

#if defined _WINDOWS
HWND GetHwnd()
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if( SDL_GetWMInfo(&info) < 0 ) 
	{
		SDL_QuitSubSystem(SDL_INIT_VIDEO);	// exit out of full screen.  The ~RageDisplay will not be called!
		RageException::Throw( "SDL_GetWMInfo failed" );
	}
	return info.window;
}
#endif


/* Set the video mode. */
CString RageDisplay_D3D::TryVideoMode( VideoModeParams p, bool &bNewDeviceOut )
{
	g_CurrentParams = p;

	HRESULT hr;

	if( FindBackBufferType( p.windowed, p.bpp ) == -1 )	// no possible back buffer formats
		return ssprintf( "FindBackBufferType(%i,%i) failed", p.windowed, p.bpp );	// failed to set mode

#if defined _WINDOWS
	/* Set SDL window title and icon -before- creating the window */
	SDL_WM_SetCaption( p.sWindowTitle, "" );
	mySDL_WM_SetIcon( p.sIconFile );

	
	// HACK: On Windows 98, we can't call SDL_SetVideoMode while D3D is full screen.
	// It will result in "SDL_SetVideoMode  failed: DirectDraw2::CreateSurface(PRIMARY): 
	// Not in exclusive access mode".  So, we'll Reset the D3D device, then resize the 
	// SDL window only if we're not fullscreen.

	SDL_ShowCursor( p.windowed );
#endif
	
#ifdef _XBOX
	p.windowed = false;
#endif

	ZeroMemory( &g_d3dpp, sizeof(g_d3dpp) );

	g_d3dpp.BackBufferWidth			=	p.width;
	g_d3dpp.BackBufferHeight		=	p.height;
	g_d3dpp.BackBufferFormat		=	FindBackBufferType( p.windowed, p.bpp );
	g_d3dpp.BackBufferCount			=	1;
	g_d3dpp.MultiSampleType			=	D3DMULTISAMPLE_NONE;
	g_d3dpp.SwapEffect				=	D3DSWAPEFFECT_DISCARD;
	g_d3dpp.hDeviceWindow			=	NULL;
	g_d3dpp.Windowed				=	p.windowed;
	g_d3dpp.EnableAutoDepthStencil	=	TRUE;
	g_d3dpp.AutoDepthStencilFormat	=	D3DFMT_D16;

	if(p.windowed)
		g_d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	else
		g_d3dpp.FullScreen_PresentationInterval = p.vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

#ifndef _XBOX
	g_d3dpp.Flags					=	0;
	g_d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
#else
	g_d3dpp.Flags					=	(p.interlaced? D3DPRESENTFLAG_INTERLACED: D3DPRESENTFLAG_PROGRESSIVE ) | D3DPRESENTFLAG_10X11PIXELASPECTRATIO;
	g_d3dpp.FullScreen_RefreshRateInHz = p.PAL ? 50 : 60;
#endif

	LOG->Trace( "Present Parameters: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", 
		g_d3dpp.BackBufferWidth, g_d3dpp.BackBufferHeight, g_d3dpp.BackBufferFormat,
		g_d3dpp.BackBufferCount,
		g_d3dpp.MultiSampleType, g_d3dpp.SwapEffect, g_d3dpp.hDeviceWindow,
		g_d3dpp.Windowed, g_d3dpp.EnableAutoDepthStencil, g_d3dpp.AutoDepthStencilFormat,
		g_d3dpp.Flags, g_d3dpp.FullScreen_RefreshRateInHz,
		g_d3dpp.FullScreen_PresentationInterval
	);

	if( g_pd3dDevice == NULL )		// device is not yet created.  We need to create it
	{
		bNewDeviceOut = true;
		hr = g_pd3d->CreateDevice(
			D3DADAPTER_DEFAULT, 
			D3DDEVTYPE_HAL, 
#ifndef _XBOX
			GetHwnd(),
			D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
#else
			NULL,
			D3DCREATE_HARDWARE_VERTEXPROCESSING,
#endif
			&g_d3dpp, 
			&g_pd3dDevice );
		if( FAILED(hr) )
		{
#if defined _WINDOWS
			SDL_QuitSubSystem(SDL_INIT_VIDEO);	// exit out of full screen.  The ~RageDisplay will not be called!
#endif
			RageException::Throw( "CreateDevice failed: '%s'", GetErrorString(hr).c_str() );
		}
	}
	else
	{
		bNewDeviceOut = false;
		hr = g_pd3dDevice->Reset( &g_d3dpp );
		if( FAILED(hr) )
		{
#if defined _WINDOWS
			SDL_QuitSubSystem(SDL_INIT_VIDEO);	// exit out of full screen.  The ~RageDisplay will not be called!
#endif
			RageException::Throw( "g_pd3dDevice->Reset failed: '%s'", GetErrorString(hr).c_str() );
		}
	}
	
	if( p.windowed )
	{
#if defined _WINDOWS
		int flags = SDL_RESIZABLE | SDL_SWSURFACE;
		
		// Don't use SDL to change the video mode.  This will cause a 
		// D3DERR_DRIVERINTERNALERROR on TNTs, V3s, and probably more.
	//	if( !windowed )
	//		flags |= SDL_FULLSCREEN;

		SDL_ShowCursor( p.windowed );

		SDL_Surface *screen = SDL_SetVideoMode(p.width, p.height, p.bpp, flags);
		if(!screen)
		{
			SDL_QuitSubSystem(SDL_INIT_VIDEO);	// exit out of full screen.  The ~RageDisplay will not be called!
			RageException::Throw("SDL_SetVideoMode failed: %s", SDL_GetError());
		}
#endif
	}

	ResolutionChanged();

	this->SetDefaultRenderStates();
	
	/* Palettes were lost by Reset(), so mark them unloaded. */
	g_TexResourceToPaletteIndex.clear();

	return "";	// mode change successful
}

void RageDisplay_D3D::ResolutionChanged()
{
	// no need to clear because D3D uses an overlay
//	SetViewport(0,0);
//
//	/* Clear any junk that's in the framebuffer. */
//	Clear();
//	Flip();
}

void RageDisplay_D3D::SetViewport(int shift_left, int shift_down)
{
	/* left and down are on a 0..SCREEN_WIDTH, 0..SCREEN_HEIGHT scale.
	 * Scale them to the actual viewport range. */
	shift_left = int( shift_left * float(g_CurrentParams.width) / SCREEN_WIDTH );
	shift_down = int( shift_down * float(g_CurrentParams.height) / SCREEN_HEIGHT );

	D3DVIEWPORT8 viewData = { shift_left, -shift_down, g_CurrentParams.width, g_CurrentParams.height, 0.f, 1.f };
	g_pd3dDevice->SetViewport( &viewData );
}

int RageDisplay_D3D::GetMaxTextureSize() const
{
	return g_DeviceCaps.MaxTextureWidth;
}

void RageDisplay_D3D::BeginFrame()
{
#ifndef _XBOX
	if( g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET )
		SetVideoMode( g_CurrentParams );
#endif

	g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
						 D3DCOLOR_XRGB(0,0,0), 1.0f, 0x00000000 );
	g_pd3dDevice->BeginScene();
}

void RageDisplay_D3D::EndFrame()
{
	g_pd3dDevice->EndScene();
	g_pd3dDevice->Present( 0, 0, 0, 0 );
	ProcessStatsOnFlip();
}

bool RageDisplay_D3D::SupportsTextureFormat( PixelFormat pixfmt )
{
#if defined _XBOX
	// Lazy...  Xbox handles paletted textures completely differently
	// than D3D and I don't want to add a bunch of code for it.  Also, 
	// paletted textures result in worse cache efficiency (see "Xbox 
	// Palettized Texture Performance" in XDK).  So, we'll force 32bit
	// ARGB textures.  -Chris
	return pixfmt == FMT_RGBA8;
#endif

	// Some cards (Savage) don't support alpha in palettes.
	// Don't allow paletted textures if this is the case.
	if( pixfmt == FMT_PAL  &&  !(g_DeviceCaps.TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE) )
		return false;

	if(	D3DFORMATS[pixfmt] == D3DFMT_UNKNOWN )
		return false;

	D3DFORMAT d3dfmt = D3DFORMATS[pixfmt];
	HRESULT hr = g_pd3d->CheckDeviceFormat( 
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		g_d3dpp.BackBufferFormat,
		0,
		D3DRTYPE_TEXTURE,
		d3dfmt);

    return SUCCEEDED( hr );
}

void RageDisplay_D3D::SaveScreenshot( CString sPath )
{
#ifndef _XBOX
	IDirect3DSurface8* pSurface;
	g_pd3dDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pSurface );
	D3DXSaveSurfaceToFile( sPath, D3DXIFF_BMP, pSurface, 0, NULL );
	pSurface->Release();
#endif
}

RageDisplay::VideoModeParams RageDisplay_D3D::GetVideoModeParams() const { return g_CurrentParams; }

#define SEND_CURRENT_MATRICES \
	g_pd3dDevice->SetTransform( D3DTS_PROJECTION, (D3DMATRIX*)GetProjectionTop() ); \
	g_pd3dDevice->SetTransform( D3DTS_VIEW, (D3DMATRIX*)GetViewTop() ); \
	RageMatrix m; \
	/* Convert to OpenGL-style "pixel-centered" coords */ \
	RageMatrixTranslation( &m, -0.5f, -0.5f, 0 ); \
	RageMatrixMultiply( &m, &m, GetWorldTop() ); \
	g_pd3dDevice->SetTransform( D3DTS_WORLD, (D3DMATRIX*)&m );


void RageDisplay_D3D::DrawQuads( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( (iNumVerts%4) == 0 );

	if(iNumVerts == 0)
		return;

	// there isn't a quad primitive in D3D, so we have to fake it with indexed triangles
	int iNumQuads = iNumVerts/4;
	int iNumTriangles = iNumQuads*2;
	int iNumIndices = iNumTriangles*3;

	// make a temporary index buffer
	static vector<Uint16> vIndices;
	unsigned uOldSize = vIndices.size();
	unsigned uNewSize = max(uOldSize,(unsigned)iNumIndices);
	vIndices.resize( uNewSize );
	for( Uint16 i=(Uint16)uOldSize/6; i<(Uint16)iNumQuads; i++ )
	{
		vIndices[i*6+0] = i*4+0;
		vIndices[i*6+1] = i*4+1;
		vIndices[i*6+2] = i*4+2;
		vIndices[i*6+3] = i*4+2;
		vIndices[i*6+4] = i*4+3;
		vIndices[i*6+5] = i*4+0;
	}

	g_pd3dDevice->SetVertexShader( D3DFVF_RageSpriteVertex );
	SEND_CURRENT_MATRICES;
	g_pd3dDevice->DrawIndexedPrimitiveUP(
		D3DPT_TRIANGLELIST, // PrimitiveType
		0, // MinIndex
		iNumVerts, // NumVertices
		iNumTriangles, // PrimitiveCount,
		&vIndices[0], // pIndexData,
		D3DFMT_INDEX16, // IndexDataFormat,
		v, // pVertexStreamZeroData,
		sizeof(RageSpriteVertex) // VertexStreamZeroStride
	);

	StatsAddVerts( iNumVerts );
}
void RageDisplay_D3D::DrawFan( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( iNumVerts >= 3 );
	g_pd3dDevice->SetVertexShader( D3DFVF_RageSpriteVertex );
	SEND_CURRENT_MATRICES;
	g_pd3dDevice->DrawPrimitiveUP(
		D3DPT_TRIANGLEFAN, // PrimitiveType
		iNumVerts-2, // PrimitiveCount,
		v, // pVertexStreamZeroData,
		sizeof(RageSpriteVertex)
	);
	StatsAddVerts( iNumVerts );
}

void RageDisplay_D3D::DrawStrip( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( iNumVerts >= 3 );
	g_pd3dDevice->SetVertexShader( D3DFVF_RageSpriteVertex );
	SEND_CURRENT_MATRICES;
	g_pd3dDevice->DrawPrimitiveUP(
		D3DPT_TRIANGLESTRIP, // PrimitiveType
		iNumVerts-2, // PrimitiveCount,
		v, // pVertexStreamZeroData,
		sizeof(RageSpriteVertex)
	);
	StatsAddVerts( iNumVerts );
}

void RageDisplay_D3D::DrawTriangles( const RageSpriteVertex v[], int iNumVerts )
{
	if( iNumVerts == 0 )
		return;
	ASSERT( iNumVerts >= 3 );
	g_pd3dDevice->SetVertexShader( D3DFVF_RageSpriteVertex );
	SEND_CURRENT_MATRICES;
	g_pd3dDevice->DrawPrimitiveUP(
		D3DPT_TRIANGLELIST, // PrimitiveType
		iNumVerts/3, // PrimitiveCount,
		v, // pVertexStreamZeroData,
		sizeof(RageSpriteVertex)
	);
	StatsAddVerts( iNumVerts );
}

void RageDisplay_D3D::DrawIndexedTriangles( const RageModelVertex v[], int iNumVerts, const Uint16 pIndices[], int iNumIndices )
{
	if( iNumIndices == 0 )
		return;
	g_pd3dDevice->SetVertexShader( D3DFVF_RageModelVertex );
	SEND_CURRENT_MATRICES;
	g_pd3dDevice->DrawIndexedPrimitiveUP(
		D3DPT_TRIANGLELIST, // PrimitiveType
		0, // MinIndex
		iNumVerts, // NumVertices
		iNumIndices/3, // PrimitiveCount,
		pIndices, // pIndexData,
		D3DFMT_INDEX16, // IndexDataFormat,
		v, // pVertexStreamZeroData,
		sizeof(RageModelVertex) // VertexStreamZeroStride
	);
	StatsAddVerts( iNumIndices );
}

/* Use the default poly-based implementation.  D3D lines apparently don't support
 * AA with greater-than-one widths. */
/*
void RageDisplay_D3D::DrawLineStrip( const RageSpriteVertex v[], int iNumVerts, float LineWidth )
{
	ASSERT( iNumVerts >= 2 );
	g_pd3dDevice->SetRenderState( D3DRS_POINTSIZE, *((DWORD*)&LineWidth) );	// funky cast.  See D3DRENDERSTATETYPE doc
	g_pd3dDevice->SetVertexShader( D3DFVF_RageSpriteVertex );
	SEND_CURRENT_MATRICES;
	g_pd3dDevice->DrawPrimitiveUP(
		D3DPT_LINESTRIP, // PrimitiveType
		iNumVerts-1, // PrimitiveCount,
		v, // pVertexStreamZeroData,
		sizeof(RageSpriteVertex)
	);
	StatsAddVerts( iNumVerts );
}
*/
void RageDisplay_D3D::SetTexture( RageTexture* pTexture )
{
	if( pTexture == NULL )
	{
		g_pd3dDevice->SetTexture( 0, NULL );
		return;
	}

	unsigned uTexHandle = pTexture->GetTexHandle();
	IDirect3DTexture8* pTex = (IDirect3DTexture8*)uTexHandle;
	g_pd3dDevice->SetTexture( 0, pTex );
	
	// Set palette (if any)
	SetPalette(uTexHandle);
}
void RageDisplay_D3D::SetTextureModeModulate()
{
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
}

void RageDisplay_D3D::SetTextureModeGlow(GlowMode m)
{
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2 );
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
}

void RageDisplay_D3D::SetTextureFiltering( bool b )
{
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, b ? D3DTEXF_LINEAR : D3DTEXF_POINT );
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, b ? D3DTEXF_LINEAR : D3DTEXF_POINT );
}

void RageDisplay_D3D::SetBlendMode( BlendMode mode )
{
	g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	switch( mode )
	{
	case BLEND_NORMAL:
		g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
		g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		break;
	case BLEND_ADD:
		g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
		g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		break;
	case BLEND_NO_EFFECT:
		g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ZERO );
		g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		break;
	default:
		ASSERT(0);
	}
}

bool RageDisplay_D3D::IsZBufferEnabled() const
{
	DWORD b;
	g_pd3dDevice->GetRenderState( D3DRS_ZENABLE, &b );
	return b!=0;
}

void RageDisplay_D3D::SetZBuffer( bool b )
{
	g_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      b ? D3DZB_TRUE : D3DZB_FALSE );
	g_pd3dDevice->SetRenderState( D3DRS_ZFUNC,		  D3DCMP_LESSEQUAL );
	g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, b );
}
void RageDisplay_D3D::ClearZBuffer()
{
	g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0x00000000 );
}

void RageDisplay_D3D::SetTextureWrapping( bool b )
{
	int mode = b ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, mode );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, mode );
}

void RageDisplay_D3D::SetMaterial( 
	float emissive[4],
	float ambient[4],
	float diffuse[4],
	float specular[4],
	float shininess
	)
{
	D3DMATERIAL8 mat;
	memcpy( &mat.Diffuse, diffuse, sizeof(float)*4 );
	memcpy( &mat.Ambient, ambient, sizeof(float)*4 );
	memcpy( &mat.Specular, specular, sizeof(float)*4 );
	memcpy( &mat.Emissive, emissive, sizeof(float)*4 );
	mat.Power = shininess;
	g_pd3dDevice->SetMaterial( &mat );
}

void RageDisplay_D3D::SetLighting( bool b )
{
	g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, b );
}

void RageDisplay_D3D::SetLightOff( int index )
{
	g_pd3dDevice->LightEnable( index, false );
}
void RageDisplay_D3D::SetLightDirectional( 
	int index, 
	RageColor ambient, 
	RageColor diffuse, 
	RageColor specular, 
	RageVector3 dir )
{
	g_pd3dDevice->LightEnable( index, true );

	D3DLIGHT8 light;
	ZERO( light );
	light.Type = D3DLIGHT_DIRECTIONAL;

	/* Without this, characters are lit from behind, so the directional light can
	 * barely be seen.  I'm not sure that this is generally correct, since I don't
	 * know how the coordinate systems are different, but it works for
	 * DancingCharacters::DrawPrimitives.  XXX: figure out exactly why this is needed */
	float position[] = { -dir.x, -dir.y, -dir.z };
	memcpy( &light.Direction, position, sizeof(position) );
	memcpy( &light.Diffuse, diffuse, sizeof(diffuse) );
	memcpy( &light.Ambient, ambient, sizeof(ambient) );
	memcpy( &light.Specular, specular, sizeof(specular) );
	
	// Same as OpenGL defaults.  Not used in directional lights.
//	light.Attenuation0 = 1;
//	light.Attenuation1 = 0;
//	light.Attenuation2 = 0;

	g_pd3dDevice->SetLight( index, &light );
}

void RageDisplay_D3D::SetBackfaceCull( bool b )
{
	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, b ? D3DCULL_CW : D3DCULL_NONE );
}

void RageDisplay_D3D::DeleteTexture( unsigned uTexHandle )
{
	IDirect3DTexture8* pTex = (IDirect3DTexture8*) uTexHandle;
	pTex->Release();

	// Delete palette (if any)
	if( g_TexResourceToPaletteIndex.find(uTexHandle) != g_TexResourceToPaletteIndex.end() )
		g_TexResourceToPaletteIndex.erase( g_TexResourceToPaletteIndex.find(uTexHandle) );
	if( g_TexResourceToTexturePalette.find(uTexHandle) != g_TexResourceToTexturePalette.end() )
		g_TexResourceToTexturePalette.erase( g_TexResourceToTexturePalette.find(uTexHandle) );
}


unsigned RageDisplay_D3D::CreateTexture( 
	PixelFormat pixfmt,
	SDL_Surface* img )
{
	// texture must be power of two
	ASSERT( img->w == power_of_two(img->w) );
	ASSERT( img->h == power_of_two(img->h) );


	HRESULT hr;
	IDirect3DTexture8* pTex;
	hr = g_pd3dDevice->CreateTexture( img->w, img->h, 1, 0, D3DFORMATS[pixfmt], D3DPOOL_MANAGED, &pTex );
	if( FAILED(hr) )
		RageException::Throw( "CreateTexture(%i,%i,pixfmt=%i) failed: %s", 
		img->w, img->h, pixfmt, GetErrorString(hr).c_str() );

	unsigned uTexHandle = (unsigned)pTex;

	if( pixfmt == FMT_PAL )
	{
		// Save palette
		TexturePalette pal;
		memset( pal.p, 0, sizeof(pal.p) );
		for( int i=0; i<img->format->palette->ncolors; i++ )
		{
			SDL_Color c = img->format->palette->colors[i];
			pal.p[i].peRed = c.r;
			pal.p[i].peGreen = c.g;
			pal.p[i].peBlue = c.b;
			bool bIsColorKey = img->flags & SDL_SRCCOLORKEY && (unsigned)i == img->format->colorkey;
			pal.p[i].peFlags = bIsColorKey ? 0x00 : 0xFF;
		}

		ASSERT( g_TexResourceToTexturePalette.find(uTexHandle) == g_TexResourceToTexturePalette.end() );
		g_TexResourceToTexturePalette[uTexHandle] = pal;
	}

	UpdateTexture( uTexHandle, img, 0, 0, img->w, img->h );

	return uTexHandle;
}

void RageDisplay_D3D::UpdateTexture( 
	unsigned uTexHandle, 
	SDL_Surface* img,
	int xoffset, int yoffset, int width, int height )
{
	IDirect3DTexture8* pTex = (IDirect3DTexture8*)uTexHandle;
	ASSERT( pTex != NULL );
	
	/* Make sure that the pixel format of the image is legit.  We don't actually
	 * care, but the OpenGL renderer does, so make sure people coding in the D3D
	 * renderer don't accidentally break the OpenGL one. */
	FindPixelFormat( img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask );

	RECT rect; 
	rect.left = xoffset;
	rect.top = yoffset;
	rect.right = width - xoffset;
	rect.bottom = height - yoffset;

	D3DLOCKED_RECT lr;
	pTex->LockRect( 0, &lr, &rect, 0 );
	
	D3DSURFACE_DESC desc;
	pTex->GetLevelDesc(0, &desc);
	ASSERT( xoffset+width <= int(desc.Width) );
	ASSERT( yoffset+height <= int(desc.Height) );

	//
	// Copy bits
	//
#if _XBOX
	// Xbox textures need to be swizzled
	XGSwizzleRect(
		img->pixels,	// pSource, 
		img->pitch,		// Pitch,
		NULL,	// pRect,
		lr.pBits,	// pDest,
		img->w,	// Width,
		img->h,	// Height,
		NULL,	// pPoint,
		img->format->BytesPerPixel ); //BytesPerPixel
#else
	int texpixfmt;
	for(texpixfmt = 0; texpixfmt < NUM_PIX_FORMATS; ++texpixfmt)
		if(D3DFORMATS[texpixfmt] == desc.Format) break;
	ASSERT( texpixfmt != NUM_PIX_FORMATS );

	SDL_Surface *Texture = CreateSurfaceFromPixfmt(PixelFormat(texpixfmt), lr.pBits, width, height, lr.Pitch);
	ASSERT( Texture );
	SDL_Rect area;
	area.x = area.y = 0;
	area.w = (Uint16) width;
	area.h = (Uint16) height;
	SDL_SetAlpha( img, 0, SDL_ALPHA_OPAQUE );
//	SDL_SetColorKey( img, 0, 0 );
//	SDL_BlitSurface( img, &area, Texture, &area );
	mySDL_BlitSurface( img, Texture, width, height, false );

	SDL_FreeSurface( Texture );
#endif

	pTex->UnlockRect( 0 );
}

void RageDisplay_D3D::SetAlphaTest( bool b )
{
	g_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE,  b );
	g_pd3dDevice->SetRenderState( D3DRS_ALPHAREF,         0 );
	g_pd3dDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATER );
}

RageMatrix RageDisplay_D3D::GetOrthoMatrix( float l, float r, float b, float t, float zn, float zf )
{
	// D3DXMatrixOrthoOffCenterRH
	RageMatrix m(
		2/(r-l),      0,            0,           0,
		0,            2/(t-b),      0,           0,
		0,            0,            -1/(zf-zn),  0,
		-(r+l)/(r-l), -(t+b)/(t-b), -zn/(zf-zn),  1 );
	return m;
}
