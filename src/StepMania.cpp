#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 File: StepMania.cpp

 Desc: Entry point for program.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "resource.h"

//
// Rage global classes
//
#include "RageLog.h"
#include "RageDisplay.h"
#include "RageTextureManager.h"
#include "RageSound.h"
//#include "RageSoundManager.h"
#include "RageMusic.h"
#include "RageInput.h"
#include "RageTimer.h"
#include "RageException.h"
#include "RageNetwork.h"
#include "RageMath.h"

#include "SDL.h"
#include "SDL_syswm.h"		// for SDL_SysWMinfo

//
// StepMania global classes
//
#include "ThemeManager.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "PrefsManager.h"
#include "GameState.h"
#include "AnnouncerManager.h"
#include "ScreenManager.h"
#include "GameManager.h"
#include "FontManager.h"
#include "InputFilter.h"
#include "InputMapper.h"
#include "InputQueue.h"
#include "SongCacheIndex.h"

//
// StepMania common classes
//
#include "GameConstantsAndTypes.h"

//#include "tls.h"
//#include "crash.h"


#include "SDL.h"
#include "SDL_opengl.h"

#ifdef _DEBUG
#pragma comment(lib, "SDL-1.2.5/lib/SDLmaind.lib")
#else
#pragma comment(lib, "SDL-1.2.5/lib/SDLmain.lib")
#endif
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "d3dx8.lib")



// command line arguments
CString		g_sSongPath = "";
bool		g_bBeClient = false;
bool		g_bBeServer = false;
CString		g_sServerIP = "";
int			g_iNumClients = 0;

const int SM_PORT = 26573;	// Quake port + "Ko" + "na" + "mitsu"

/*------------------------------------------------
	Common stuff
------------------------------------------------*/
int		flags = 0;		/* SDL video flags */
int		bpp = 0;		/* Preferred screen bpp */
int		window_w = SCREEN_WIDTH, window_h = SCREEN_HEIGHT;	/* window width and height */
SDL_Surface *loading_screen = NULL;
CString  g_sErrorString = "";

void ChangeToDirOfExecutable()
{
	//
	// Make sure the current directory is the root program directory
	//
	if( !DoesFileExist("Songs") )
	{
		// change dir to path of the execuctable
		TCHAR szFullAppPath[MAX_PATH];
		GetModuleFileName(NULL, szFullAppPath, MAX_PATH);
		
		// strip off executable name
		LPSTR pLastBackslash = strrchr(szFullAppPath, '\\');
		*pLastBackslash = '\0';	// terminate the string

		SetCurrentDirectory( szFullAppPath );
	}
}

	
void PaintLoadingWindow()
{
	SDL_UpdateRect(loading_screen, 0,0,0,0);
}

void CreateLoadingWindow()
{
	ASSERT( loading_screen == NULL );

    /* Initialize the SDL library */
    if( SDL_InitSubSystem(SDL_INIT_VIDEO) < 0 )
        throw RageException( "Couldn't initialize SDL: %s\n", SDL_GetError() );

    SDL_Surface *image;

    /* Load the BMP file into a surface */
    image = SDL_LoadBMP("loading.bmp");
    if( image == NULL )
        throw RageException("Couldn't load loading.bmp: %s\n",SDL_GetError());

    /* Initialize the display in a 640x480 16-bit mode */
    loading_screen = SDL_SetVideoMode(image->w, image->h, 16, SDL_SWSURFACE|SDL_ANYFORMAT|SDL_NOFRAME);
    if( loading_screen == NULL )
        throw RageException( "Couldn't initialize loading window: %s\n", SDL_GetError() );

    /* Blit onto the screen surface */
    SDL_Rect dest;
    dest.x = 0;
    dest.y = 0;
    dest.w = (Uint16)image->w;
    dest.h = (Uint16)image->h;
    SDL_BlitSurface(image, NULL, loading_screen, NULL);

	SDL_WM_SetCaption( "StepMania", NULL);

	PaintLoadingWindow();

    /* Free the allocated BMP surface */
    SDL_FreeSurface(image);
}

void DestroyLoadingWindow()
{
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	loading_screen = NULL;
}


//-----------------------------------------------------------------------------
// Name: ErrorWndProc()
// Desc: Callback for all Windows messages
//-----------------------------------------------------------------------------
BOOL CALLBACK ErrorWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_INITDIALOG:
		{
			CString sMessage = g_sErrorString;

			sMessage.Replace( "\n", "\r\n" );
			
			SendDlgItemMessage( 
				hWnd, 
				IDC_EDIT_ERROR, 
				WM_SETTEXT, 
				0, 
				(LPARAM)(LPCTSTR)sMessage
				);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_VIEW_LOG:
			{
				PROCESS_INFORMATION pi;
				STARTUPINFO	si;
				ZeroMemory( &si, sizeof(si) );

				CreateProcess(
					NULL,		// pointer to name of executable module
					"notepad.exe log.txt",		// pointer to command line string
					NULL,  // process security attributes
					NULL,   // thread security attributes
					false,  // handle inheritance flag
					0, // creation flags
					NULL,  // pointer to new environment block
					NULL,   // pointer to current directory name
					&si,  // pointer to STARTUPINFO
					&pi  // pointer to PROCESS_INFORMATION
				);
			}
			break;
		case IDC_BUTTON_REPORT:
			GotoURL( "http://sourceforge.net/tracker/?func=add&group_id=37892&atid=421366" );
			break;
		case IDC_BUTTON_RESTART:
			{
				/* Clear the startup mutex, since we're starting a new
				 * instance before ending ourself. */
				TCHAR szFullAppPath[MAX_PATH];
				GetModuleFileName(NULL, szFullAppPath, MAX_PATH);

				// Launch StepMania
				PROCESS_INFORMATION pi;
				STARTUPINFO	si;
				ZeroMemory( &si, sizeof(si) );

				CreateProcess(
					NULL,		// pointer to name of executable module
					szFullAppPath,		// pointer to command line string
					NULL,  // process security attributes
					NULL,   // thread security attributes
					false,  // handle inheritance flag
					0, // creation flags
					NULL,  // pointer to new environment block
					NULL,   // pointer to current directory name
					&si,  // pointer to STARTUPINFO
					&pi  // pointer to PROCESS_INFORMATION
				);
			}
			EndDialog( hWnd, 0 );
			break;
			// fall through
		case IDOK:
			EndDialog( hWnd, 0 );
			break;
		}
	}
	return FALSE;
}


//-----------------------------------------------------------------------------
// Name: ApplyGraphicOptions()
// Desc:
//-----------------------------------------------------------------------------
void ApplyGraphicOptions()
{ 
	DISPLAY->SetVideoMode( 
		PREFSMAN->m_bWindowed, 
		PREFSMAN->m_iDisplayWidth, 
		PREFSMAN->m_iDisplayHeight, 
		PREFSMAN->m_iDisplayColorDepth, 
		(RefreshRateMode)PREFSMAN->m_iRefreshRateMode,
		PREFSMAN->m_bVsync );
	TEXTUREMAN->SetPrefs( PREFSMAN->m_iTextureColorDepth, PREFSMAN->m_iUnloadTextureDelaySeconds );
}


//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Application entry point
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	ChangeToDirOfExecutable();

    atexit(SDL_Quit);   /* Clean up on exit */

	/*
	 * Handle command line args
	 */
	for(int i=0; i<argc; i++)
	{
		if(argv[i] == "--fsck")
			;//crash(); 
		else if(argv[i] == "--song")
			g_sSongPath = argv[++i];
		else if(argv[i] == "--client")
			g_bBeClient = true;
		else if(argv[i] == "--server")
			g_bBeServer = true;
		else if(argv[i] == "--ip")
			g_sServerIP = argv[++i];
		else if(argv[i] == "--numclients")
			g_iNumClients = atoi(argv[++i]);
	}


#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif

	//	SetUnhandledExceptionFilter(CrashHandler);
//	InitThreadData("Main thread");
//	VDCHECKPOINT;

	SDL_Init(0);	/* Fire up the SDL, but don't actually start any subsystems. */

	atexit(SDL_Quit);

#ifndef _DEBUG
	try{
#endif


	CreateLoadingWindow();
	PaintLoadingWindow();

	//
	// Create game objects
	//
	srand( (unsigned)time(NULL) );	// seed number generator
	
	LOG			= new RageLog();
#ifdef _DEBUG
	LOG->ShowConsole();
#endif
	TIMER		= new RageTimer;
	GAMESTATE	= new GameState;
	PREFSMAN	= new PrefsManager;
	GAMEMAN		= new GameManager;
	THEME		= new ThemeManager;
	SOUND		= new RageSound;
	MUSIC		= new RageSoundStream;
	ANNOUNCER	= new AnnouncerManager;
	INPUTFILTER	= new InputFilter;
	INPUTMAPPER	= new InputMapper;
	NETWORK		= new RageNetwork;
	INPUTQUEUE	= new InputQueue;
	SONGINDEX	= new SongCacheIndex;
	/* depends on SONGINDEX: */
	SONGMAN		= new SongManager( PaintLoadingWindow );		// this takes a long time to load

	DestroyLoadingWindow();	// destroy this before init'ing Display

	PREFSMAN->ReadGlobalPrefsFromDisk( true );

	DISPLAY		= new RageDisplay(
		PREFSMAN->m_bWindowed, 
		PREFSMAN->m_iDisplayWidth, 
		PREFSMAN->m_iDisplayHeight, 
		PREFSMAN->m_iDisplayColorDepth, 
		(RefreshRateMode)PREFSMAN->m_iRefreshRateMode,
		PREFSMAN->m_bVsync );
	TEXTUREMAN	= new RageTextureManager( PREFSMAN->m_iTextureColorDepth, PREFSMAN->m_iUnloadTextureDelaySeconds );

	/* Grab the window manager specific information */
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if ( SDL_GetWMInfo(&info) < 0 ) 
		throw RageException( "SDL_GetWMInfo failed" );
	HWND hwnd = info.window;

	INPUTMAN	= new RageInput( hwnd );
//	SOUNDMAN	= new RageSoundManager( hwnd );

	// These things depend on the TextureManager, so do them after!
	FONT		= new FontManager;
	SCREENMAN	= new ScreenManager;

	/*
	 * Load initial screen depending on network mode
	 */
	if( g_bBeClient )
	{
		// immediately try to connect to server
		GAMESTATE->m_pCurSong = SONGMAN->GetSongFromPath( g_sSongPath );
		if( GAMESTATE->m_pCurSong == NULL )
			throw RageException( "The song '%s' is required to play this network game.", g_sSongPath );
		NETWORK->Init( false );
		if( !NETWORK->Connect( (const char*)g_sServerIP, SM_PORT ) )
			throw RageException( "Could not connect to server '%s'", g_sServerIP );
		SCREENMAN->SetNewScreen( "ScreenSandbox" );
	}
	else if( g_bBeServer )
	{
		// wait for clients to connect
		GAMESTATE->m_pCurSong = SONGMAN->GetSongFromPath( g_sSongPath );
		if( GAMESTATE->m_pCurSong == NULL )
			throw RageException( "The song '%s' is required to play this network game.", g_sSongPath );
		NETWORK->Init( true );
		if( !NETWORK->Listen( SM_PORT ) )
			throw RageException( "Could not connect to server '%s'", g_sServerIP );
		SCREENMAN->SetNewScreen( "ScreenSandbox" );
	}
	else
	{
		// normal game
		SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
//		SCREENMAN->SetNewScreen( "ScreenSandbox" );
	}


	// Do game loop
	bool do_exit = false;
	SDL_Event	event;
	while(!do_exit)
	{
		// process all queued events
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_QUIT:
				do_exit = 1;
				break;
			case SDL_VIDEORESIZE:
				PREFSMAN->m_iDisplayWidth = event.resize.w;
				PREFSMAN->m_iDisplayHeight = event.resize.h;
				ApplyGraphicOptions();
				break;
			}
		}

		DISPLAY->Clear();
		DISPLAY->ResetMatrixStack();


		/*
		 * Update
		 */
		float fDeltaTime = TIMER->GetDeltaTime();
		
		// This was a hack to fix timing issues with the old ScreenSelectSong
		// See ScreenManager::Update comments for why we shouldn't do this. -glenn
		//if( fDeltaTime > 0.050f )	// we dropped a bunch of frames
		// 	fDeltaTime = 0.050f;
		if( INPUTMAN->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, DIK_TAB) ) ) {
			if( INPUTMAN->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, DIK_GRAVE) ) )
				fDeltaTime = 0; /* both; stop time */
			else
				fDeltaTime *= 4;
		} else
			if( INPUTMAN->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, DIK_GRAVE) ) )
				fDeltaTime /= 4;

		MUSIC->Update( fDeltaTime );
		SCREENMAN->Update( fDeltaTime );
		NETWORK->Update( fDeltaTime );


		static InputEventArray ieArray;
		ieArray.clear();	// empty the array
		INPUTFILTER->GetInputEvents( ieArray, fDeltaTime );
		for( unsigned i=0; i<ieArray.size(); i++ )
		{
			DeviceInput DeviceI = (DeviceInput)ieArray[i];
			InputEventType type = ieArray[i].type;
			GameInput GameI;
			MenuInput MenuI;
			StyleInput StyleI;

			INPUTMAPPER->DeviceToGame( DeviceI, GameI );
			
			if( GameI.IsValid()  &&  type == IET_FIRST_PRESS )
				INPUTQUEUE->RememberInput( GameI );
			if( GameI.IsValid() )
			{
				INPUTMAPPER->GameToMenu( GameI, MenuI );
				INPUTMAPPER->GameToStyle( GameI, StyleI );
			}

			SCREENMAN->Input( DeviceI, type, GameI, MenuI, StyleI );
		}


		/*
		 * Render
		 */
		DISPLAY->ResetMatrixStack();

		RageMatrix mat;
		
		RageMatrixOrthoOffCenterLH( &mat, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, SCREEN_NEAR, SCREEN_FAR );
		DISPLAY->SetProjectionTransform( &mat );

		RageMatrixIdentity( &mat );
		DISPLAY->SetViewTransform( &mat );

		SCREENMAN->Draw();		// draw the game

		DISPLAY->FlushQueue();
		DISPLAY->Flip();

		if( DISPLAY  &&  DISPLAY->IsWindowed() )
			::Sleep( 0 );	// give some time to other processes
	}

#ifndef _DEBUG
	}
	catch( RageException e )
	{
		g_sErrorString = e.what();

		LOG->Trace( 
			"\n"
			"//////////////////////////////////////////////////////\n"
			"Exception: %s\n"
			"//////////////////////////////////////////////////////\n"
			"\n",
			g_sErrorString.GetString()
			);

		LOG->Flush();
	}
#endif

	SAFE_DELETE( SCREENMAN );
	SAFE_DELETE( NETWORK );
	SAFE_DELETE( INPUTQUEUE );
	SAFE_DELETE( INPUTMAPPER );
	SAFE_DELETE( INPUTFILTER );
	SAFE_DELETE( SONGMAN );
	SAFE_DELETE( SONGINDEX );
	SAFE_DELETE( PREFSMAN );
	SAFE_DELETE( GAMESTATE );
	SAFE_DELETE( GAMEMAN );
	SAFE_DELETE( THEME );
	SAFE_DELETE( ANNOUNCER );
	SAFE_DELETE( INPUTMAN );
	SAFE_DELETE( MUSIC );
	SAFE_DELETE( SOUND );
//	SAFE_DELETE( SOUNDMAN );
	SAFE_DELETE( TIMER );
	SAFE_DELETE( FONT );
	SAFE_DELETE( TEXTUREMAN );
	SAFE_DELETE( DISPLAY );
	SAFE_DELETE( LOG );


	if( g_sErrorString != "" )
	{
		// throw up a pretty error dialog
		DialogBox(
			NULL,
			MAKEINTRESOURCE(IDD_ERROR_DIALOG),
			NULL,
			ErrorWndProc
			);
	}

	return 0;
}
