#include "global.h"

#include "StepMania.h"

//
// Rage global classes
//
#include "RageLog.h"
#include "RageTextureManager.h"
#include "RageSoundManager.h"
#include "GameSoundManager.h"
#include "RageInput.h"
#include "RageTimer.h"
#include "RageMath.h"
#include "RageDisplay.h"
#include "RageThreads.h"
#include "LocalizedString.h"

#include "arch/ArchHooks/ArchHooks.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "arch/Dialog/Dialog.h"
#include <ctime>

#include "ProductInfo.h"

#include "Screen.h"
#include "InputEventPlus.h"
#include "ScreenDimensions.h"
#include "CodeDetector.h"
#include "CommonMetrics.h"
#include "Game.h"
#include "RageSurface.h"
#include "RageSurface_Load.h"
#include "CommandLineActions.h"

#if !defined(SUPPORT_OPENGL) && !defined(SUPPORT_D3D)
#define SUPPORT_OPENGL
#endif

//
// StepMania global classes
//
#include "ThemeManager.h"
#include "NoteSkinManager.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "CharacterManager.h"
#include "GameState.h"
#include "AnnouncerManager.h"
#include "ProfileManager.h"
#include "MemoryCardManager.h"
#include "ScreenManager.h"
#include "LuaManager.h"
#include "GameManager.h"
#include "FontManager.h"
#include "InputFilter.h"
#include "InputMapper.h"
#include "InputQueue.h"
#include "SongCacheIndex.h"
#include "BannerCache.h"
#include "UnlockManager.h"
#include "RageFileManager.h"
#include "Bookkeeper.h"
#include "LightsManager.h"
#include "ModelManager.h"
#include "CryptManager.h"
#include "NetworkSyncManager.h"
#include "MessageManager.h"
#include "StatsManager.h"
#include "GameLoop.h"
#include "SpecialFiles.h"
#include "Profile.h"

#if defined(XBOX)
#include "Archutils/Xbox/VirtualMemory.h"
#endif

#if defined(WIN32) && !defined(XBOX)
#include <windows.h>
#endif

static Preference<bool> g_bAllowMultipleInstances( "AllowMultipleInstances", false );


void StepMania::GetPreferredVideoModeParams( VideoModeParams &paramsOut )
{
	/*
	 * We can't rely on there being full-screen video modes that give us square pixels 
	 * at non-4:3 aspects.  The lowest non-4:3 resolution my new laptop with Radeon supports is 1280x720.  
	 * In most cases, we'll using a 4:3 resolution when full-screen let the monitor stretch to the correct 
	 * aspect.  When windowed (no monitor stretching), we will tweak the width so that we get square pixels.
	 * -Chris
	 */
	int iWidth = PREFSMAN->m_iDisplayWidth;
	if( PREFSMAN->m_bWindowed )
	{
		iWidth = PREFSMAN->m_iDisplayHeight * PREFSMAN->m_fDisplayAspectRatio;
	}

	paramsOut = VideoModeParams(
		PREFSMAN->m_bWindowed,
		iWidth,
		PREFSMAN->m_iDisplayHeight,
		PREFSMAN->m_iDisplayColorDepth,
		PREFSMAN->m_iRefreshRate,
		PREFSMAN->m_bVsync,
		PREFSMAN->m_bInterlaced,
		PREFSMAN->m_bSmoothLines,
		PREFSMAN->m_bTrilinearFiltering,
		PREFSMAN->m_bAnisotropicFiltering,
		CommonMetrics::WINDOW_TITLE,
		THEME->GetPathG("Common","window icon"),
		PREFSMAN->m_bPAL,
		PREFSMAN->m_fDisplayAspectRatio
	);
}

static LocalizedString COLOR		("StepMania","color");
static LocalizedString TEXTURE		("StepMania","texture");
static LocalizedString WINDOWED		("StepMania","Windowed");
static LocalizedString FULLSCREEN	("StepMania","Fullscreen");
static LocalizedString ANNOUNCER_	("StepMania","Announcer");
static LocalizedString VSYNC		("StepMania","Vsync");
static LocalizedString NO_VSYNC		("StepMania","NoVsync");
static LocalizedString SMOOTH_LINES	("StepMania","SmoothLines");
static LocalizedString NO_SMOOTH_LINES	("StepMania","NoSmoothLines");

static RString GetActualGraphicOptionsString()
{
	const VideoModeParams &params = DISPLAY->GetActualVideoModeParams();
	RString sFormat = "%s %s %dx%d %d "+COLOR.GetValue()+" %d "+TEXTURE.GetValue()+" %dHz %s %s";
	RString sLog = ssprintf( sFormat,
		DISPLAY->GetApiDescription().c_str(),
		(params.windowed? WINDOWED : FULLSCREEN).GetValue().c_str(),
		(int)params.width, 
		(int)params.height, 
		(int)params.bpp, 
		(int)PREFSMAN->m_iTextureColorDepth, 
		(int)params.rate,
		(params.vsync? VSYNC : NO_VSYNC).GetValue().c_str(),
		(PREFSMAN->m_bSmoothLines? SMOOTH_LINES : NO_SMOOTH_LINES).GetValue().c_str() );
	return sLog;
}

static void StoreActualGraphicOptions()
{
	/* Store the settings that RageDisplay was actually able to use so that
	 * we don't go through the process of auto-detecting a usable video mode
	 * every time.
	 */
	const VideoModeParams &params = DISPLAY->GetActualVideoModeParams();
	PREFSMAN->m_bWindowed		.Set( params.windowed );

	/* If we're windowed, we may have tweaked the width based on the aspect ratio.  
	 * Don't save this new value over the preferred value.
	 */
	if( !PREFSMAN->m_bWindowed )
	{
		PREFSMAN->m_iDisplayWidth	.Set( params.width );
		PREFSMAN->m_iDisplayHeight	.Set( params.height );
	}
	PREFSMAN->m_iDisplayColorDepth	.Set( params.bpp );
	if( PREFSMAN->m_iRefreshRate != REFRESH_DEFAULT )
		PREFSMAN->m_iRefreshRate.Set( params.rate );
	PREFSMAN->m_bVsync		.Set( params.vsync );

	Dialog::SetWindowed( params.windowed );
}

static RageDisplay *CreateDisplay();

bool StepMania::GetHighResolutionTextures()
{
	switch( PREFSMAN->m_HighResolutionTextures )
	{
	default:
	case HighResolutionTextures_Auto:
		{
			int height = PREFSMAN->m_iDisplayHeight;
			return height > 480;
		}
	case HighResolutionTextures_ForceOn:
		return true;
	case HighResolutionTextures_ForceOff:
		return false;
	}
}

static void StartDisplay()
{
	if( DISPLAY != NULL )
		return; // already started

	DISPLAY = CreateDisplay();


	DISPLAY->ChangeCentering(
		PREFSMAN->m_iCenterImageTranslateX, 
		PREFSMAN->m_iCenterImageTranslateY,
		PREFSMAN->m_fCenterImageAddWidth,
		PREFSMAN->m_fCenterImageAddHeight );

	TEXTUREMAN	= new RageTextureManager;
	TEXTUREMAN->SetPrefs( 
		RageTextureManagerPrefs( 
			PREFSMAN->m_iTextureColorDepth, 
			PREFSMAN->m_iMovieColorDepth,
			PREFSMAN->m_bDelayedTextureDelete, 
			PREFSMAN->m_iMaxTextureResolution,
			StepMania::GetHighResolutionTextures(),
			PREFSMAN->m_bForceMipMaps
			)
		);

	MODELMAN	= new ModelManager;
	MODELMAN->SetPrefs( 
		ModelManagerPrefs(
			PREFSMAN->m_bDelayedModelDelete 
			)
		);
}

void StepMania::ApplyGraphicOptions()
{ 
	bool bNeedReload = false;

	VideoModeParams params;
	GetPreferredVideoModeParams( params );
	RString sError = DISPLAY->SetVideoMode( params, bNeedReload );
	if( sError != "" )
		RageException::Throw( "%s", sError.c_str() );

	DISPLAY->ChangeCentering(
		PREFSMAN->m_iCenterImageTranslateX, 
		PREFSMAN->m_iCenterImageTranslateY,
		PREFSMAN->m_fCenterImageAddWidth,
		PREFSMAN->m_fCenterImageAddHeight );

	bNeedReload |= TEXTUREMAN->SetPrefs( 
		RageTextureManagerPrefs( 
			PREFSMAN->m_iTextureColorDepth, 
			PREFSMAN->m_iMovieColorDepth,
			PREFSMAN->m_bDelayedTextureDelete, 
			PREFSMAN->m_iMaxTextureResolution,
			StepMania::GetHighResolutionTextures(),
			PREFSMAN->m_bForceMipMaps
			)
		);

	bNeedReload |= MODELMAN->SetPrefs( 
		ModelManagerPrefs(
			PREFSMAN->m_bDelayedModelDelete 
			)
		);

	if( bNeedReload )
		TEXTUREMAN->ReloadAll();

	StoreActualGraphicOptions();
	if( SCREENMAN )
		SCREENMAN->SystemMessage( GetActualGraphicOptionsString() );

	/* Give the input handlers a chance to re-open devices as necessary. */
	INPUTMAN->WindowReset();
}

static bool CheckVideoDefaultSettings();

void StepMania::ResetPreferences()
{
	PREFSMAN->ResetToFactoryDefaults();
	SOUNDMAN->SetMixVolume();
	CheckVideoDefaultSettings();
	ApplyGraphicOptions();
}

/* Shutdown all global singletons.  Note that this may be called partway through
 * initialization, due to an object failing to initialize, in which case some of
 * these may still be NULL. */
void ShutdownGame()
{
	/* First, tell SOUNDMAN that we're shutting down.  This signals sound drivers to
	 * stop sounds, which we want to do before any threads that may have started sounds
	 * are closed; this prevents annoying DirectSound glitches and delays. */
	if( SOUNDMAN )
		SOUNDMAN->Shutdown();

	SAFE_DELETE( SCREENMAN );
	SAFE_DELETE( STATSMAN );
	SAFE_DELETE( MESSAGEMAN );
	SAFE_DELETE( NSMAN );
	/* Delete INPUTMAN before the other INPUTFILTER handlers, or an input
	 * driver may try to send a message to INPUTFILTER after we delete it. */
	SAFE_DELETE( INPUTMAN );
	SAFE_DELETE( INPUTQUEUE );
	SAFE_DELETE( INPUTMAPPER );
	SAFE_DELETE( INPUTFILTER );
	SAFE_DELETE( MODELMAN );
	SAFE_DELETE( PROFILEMAN );	// PROFILEMAN needs the songs still loaded
	SAFE_DELETE( CHARMAN );
	SAFE_DELETE( UNLOCKMAN );
	SAFE_DELETE( CRYPTMAN );
	SAFE_DELETE( MEMCARDMAN );
	SAFE_DELETE( SONGMAN );
	SAFE_DELETE( BANNERCACHE );
	SAFE_DELETE( SONGINDEX );
	SAFE_DELETE( SOUND ); /* uses GAMESTATE, PREFSMAN */
	SAFE_DELETE( PREFSMAN );
	SAFE_DELETE( GAMESTATE );
	SAFE_DELETE( GAMEMAN );
	SAFE_DELETE( NOTESKIN );
	SAFE_DELETE( THEME );
	SAFE_DELETE( ANNOUNCER );
	SAFE_DELETE( BOOKKEEPER );
	SAFE_DELETE( LIGHTSMAN );
	SAFE_DELETE( SOUNDMAN );
	SAFE_DELETE( FONT );
	SAFE_DELETE( TEXTUREMAN );
	SAFE_DELETE( DISPLAY );
	Dialog::Shutdown();
	SAFE_DELETE( LOG );
	SAFE_DELETE( FILEMAN );
	SAFE_DELETE( LUA );
	SAFE_DELETE( HOOKS );
}

static void HandleException( const RString &sError )
{
	if( g_bAutoRestart )
		HOOKS->RestartProgram();

	/* Shut down first, so we exit graphics mode before trying to open a dialog. */
	ShutdownGame();

	/* Throw up a pretty error dialog. */
	Dialog::Error( sError );
	Dialog::Shutdown(); // Shut it back down.
}
	
void StepMania::ResetGame()
{
	GAMESTATE->Reset();
	
	if( !THEME->DoesThemeExist( THEME->GetCurThemeName() ) )
	{
		RString sGameName = GAMESTATE->GetCurrentGame()->m_szName;
		if( !THEME->DoesThemeExist(sGameName) )
			sGameName = "default";
		THEME->SwitchThemeAndLanguage( sGameName, THEME->GetCurLanguage(), PREFSMAN->m_bPseudoLocalize );
		TEXTUREMAN->DoDelayedDelete();
	}

	PREFSMAN->SavePrefsToDisk();
}

ThemeMetric<RString>	INITIAL_SCREEN	("Common","InitialScreen");
RString StepMania::GetInitialScreen()
{
	if( PREFSMAN->m_sTestInitialScreen.Get() != "" )
		return PREFSMAN->m_sTestInitialScreen;
	return INITIAL_SCREEN.GetValue();
}

#if defined(WIN32)
static Preference<int> g_iLastSeenMemory( "LastSeenMemory", 0 );
#endif

static void AdjustForChangedSystemCapabilities()
{
#if defined(WIN32)
	/* Has the amount of memory changed? */
	MEMORYSTATUS mem;
	GlobalMemoryStatus(&mem);

	const int Memory = mem.dwTotalPhys / (1024*1024);

	if( g_iLastSeenMemory == Memory )
		return;
	
	LOG->Trace( "Memory changed from %i to %i; settings changed", g_iLastSeenMemory.Get(), Memory );
	g_iLastSeenMemory.Set( Memory );

	/* Let's consider 128-meg systems low-memory, and 256-meg systems high-memory.
	 * Cut off at 192.  This is somewhat conservative; many 128-meg systems can
	 * deal with higher memory profile settings, but some can't. 
	 *
	 * Actually, Windows lops off a meg or two; cut off a little lower to treat
	 * 192-meg systems as high-memory. */
	const bool HighMemory = (Memory >= 190);
	const bool LowMemory = (Memory < 100); /* 64 and 96-meg systems */

	/* Two memory-consuming features that we can disable are texture caching and
	 * preloaded banners.  Texture caching can use a lot of memory; disable it for
	 * low-memory systems. */
	PREFSMAN->m_bDelayedTextureDelete.Set( HighMemory );

	/* Preloaded banners takes about 9k per song. Although it's smaller than the
	 * actual song data, it still adds up with a lot of songs. Disable it for 64-meg
	 * systems. */
	PREFSMAN->m_BannerCache.Set( LowMemory ? BNCACHE_OFF:BNCACHE_LOW_RES_PRELOAD );

	PREFSMAN->SavePrefsToDisk();
#endif
}

#if defined(WIN32)
#include "RageDisplay_D3D.h"
#include "archutils/Win32/VideoDriverInfo.h"
#endif

#if defined(SUPPORT_OPENGL)
#include "RageDisplay_OGL.h"
#endif

#include "RageDisplay_Null.h"


struct VideoCardDefaults
{
	RString sDriverRegex;
	RString sVideoRenderers;
	int iWidth;
	int iHeight;
	int iDisplayColor;
	int iTextureColor;
	int iMovieColor;
	int iTextureSize;
	bool bSmoothLines;

	VideoCardDefaults() {}
	VideoCardDefaults(
		RString sDriverRegex_,
		RString sVideoRenderers_,
		int iWidth_,
		int iHeight_,
		int iDisplayColor_,
		int iTextureColor_,
		int iMovieColor_,
		int iTextureSize_,
		bool bSmoothLines_
		)
	{
		sDriverRegex = sDriverRegex_;
		sVideoRenderers = sVideoRenderers_;
		iWidth = iWidth_;
		iHeight = iHeight_;
		iDisplayColor = iDisplayColor_;
		iTextureColor = iTextureColor_;
		iMovieColor = iMovieColor_;
		iTextureSize = iTextureSize_;
		bSmoothLines = bSmoothLines_;
	}
} const g_VideoCardDefaults[] = 
{
	VideoCardDefaults(
		"Xbox",
		"d3d",
		600,400,
		32,32,32,
		2048,
		true
	),
	VideoCardDefaults(
		"Voodoo *5",
		"d3d,opengl",	// received 3 reports of opengl crashing. -Chris
		640,480,
		32,32,32,
		2048,
		true	// accelerated
	),
	VideoCardDefaults(
		"Voodoo|3dfx", /* all other Voodoos: some drivers don't identify which one */
		"d3d,opengl",
		640,480,
		16,16,16,
		256,
		false	// broken, causes black screen
	),
	VideoCardDefaults(
		"Radeon.* 7|Wonder 7500|ArcadeVGA",	// Radeon 7xxx, RADEON Mobility 7500
		"d3d,opengl",	// movie texture performance is terrible in OpenGL, but fine in D3D.
		640,480,
		16,16,16,
		2048,
		true	// accelerated
	),
	VideoCardDefaults(
		"GeForce|Radeon|Wonder 9|Quadro",
		"opengl,d3d",
		640,480,
		32,32,32,	// 32 bit textures are faster to load
		2048,
		true	// hardware accelerated
	),
	VideoCardDefaults(
		"TNT|Vanta|M64",
		"opengl,d3d",
		640,480,
		16,16,16,	// Athlon 1.2+TNT demonstration w/ movies: 70fps w/ 32bit textures, 86fps w/ 16bit textures
		2048,
		true	// hardware accelerated
	),
	VideoCardDefaults(
		"G200|G250|G400",
		"d3d,opengl",
		640,480,
		16,16,16,
		2048,
		false	// broken, causes black screen
	),
	VideoCardDefaults(
		"Savage",
		"d3d",
			// OpenGL is unusable on my Savage IV with even the latest drivers.  
			// It draws 30 frames of gibberish then crashes.  This happens even with
			// simple NeHe demos.  -Chris
		640,480,
		16,16,16,
		2048,
		false
	),
	VideoCardDefaults(
		"XPERT@PLAY|IIC|RAGE PRO|RAGE LT PRO",	// Rage Pro chip, Rage IIC chip
		"d3d",
			// OpenGL is not hardware accelerated, despite the fact that the 
			// drivers come with an ICD.  Also, the WinXP driver performance 
			// is terrible and supports only 640.  The ATI driver is usable.
			// -Chris
		320,240,	// lower resolution for 60fps.  In-box WinXP driver doesn't support 400x300.
		16,16,16,
		256,
		false
	),
	VideoCardDefaults(
		"RAGE MOBILITY-M1",
		"d3d,opengl",	// Vertex alpha is broken in OpenGL, but not D3D. -Chris
		400,300,	// lower resolution for 60fps
		16,16,16,
		256,
		false
	),
	VideoCardDefaults(
		"Mobility M3",	// ATI Rage Mobility 128 (AKA "M3")
		"d3d,opengl",	// bad movie texture performance in opengl
		640,480,
		16,16,16,
		1024,
		false
	),
	VideoCardDefaults(
		"Intel.*82810|Intel.*82815",
		"opengl,d3d",// OpenGL is 50%+ faster than D3D w/ latest Intel drivers.  -Chris
		512,384,	// lower resolution for 60fps
		16,16,16,
		512,
		false
	),
	VideoCardDefaults(
		"Intel*Extreme Graphics",
		"d3d",	// OpenGL blue screens w/ XP drivers from 6-21-2002
		640,480,
		16,16,16,	// slow at 32bpp
		1024,
		false
	),
	VideoCardDefaults(
		"Intel.*", /* fallback: all unknown Intel cards to D3D, since Intel is notoriously bad at OpenGL */
		"d3d,opengl",
		640,480,
		16,16,16,
		2048,
		false
	),
	VideoCardDefaults(
		// Cards that have problems with OpenGL:
		// ASSERT fail somewhere in RageDisplay_OpenGL "Trident Video Accelerator CyberBlade"
		// bug 764499: ASSERT fail after glDeleteTextures for "SiS 650_651_740"
		// bug 764830: ASSERT fail after glDeleteTextures for "VIA Tech VT8361/VT8601 Graphics Controller"
		// bug 791950: AV in glsis630!DrvSwapBuffers for "SiS 630/730"
		"Trident Video Accelerator CyberBlade|VIA.*VT|SiS 6*",
		"d3d,opengl",
		640,480,
		16,16,16,
		2048,
		false
	),
	VideoCardDefaults(
		/* Unconfirmed texture problems on this; let's try D3D, since it's a VIA/S3
		 * chipset. */
		"VIA/S3G KM400/KN400",
		"d3d,opengl",
		640,480,
		16,16,16,
		2048,
		false
	),
	VideoCardDefaults(
		"OpenGL",	// This matches all drivers in Mac and Linux. -Chris
		"opengl",
		640,480,
		16,16,16,
		2048,
		true		// Right now, they've got to have NVidia or ATi Cards anyway..
	),
	VideoCardDefaults(
		// Default graphics settings used for all cards that don't match above.
		// This must be the very last entry!
		"",
		"opengl,d3d",
		640,480,
		32,32,32,
		2048,
		false  // AA is slow on some cards, so let's selectively enable HW accelerated cards.
	),
};


static RString GetVideoDriverName()
{
#if defined(_WINDOWS)
	return GetPrimaryVideoDriverName();
#elif defined(_XBOX)
	return "Xbox";
#else
	return "OpenGL";
#endif
}

bool CheckVideoDefaultSettings()
{
	// Video card changed since last run
	RString sVideoDriver = GetVideoDriverName();
	
	LOG->Trace( "Last seen video driver: %s", PREFSMAN->m_sLastSeenVideoDriver.Get().c_str() );

	VideoCardDefaults defaults;
	
	for( unsigned i=0; i<ARRAYLEN(g_VideoCardDefaults); i++ )
	{
		defaults = g_VideoCardDefaults[i];

		RString sDriverRegex = defaults.sDriverRegex;
		Regex regex( sDriverRegex );
		if( regex.Compare(sVideoDriver) )
		{
			LOG->Trace( "Card matches '%s'.", sDriverRegex.size()? sDriverRegex.c_str():"(unknown card)" );
			goto found_defaults;
		}
	}
	ASSERT( 0 );	// we must have matched at least one above

found_defaults:

	bool bSetDefaultVideoParams = false;
	if( PREFSMAN->m_sVideoRenderers.Get() == "" )
	{
		bSetDefaultVideoParams = true;
		LOG->Trace( "Applying defaults for %s.", sVideoDriver.c_str() );
	}
	else if( PREFSMAN->m_sLastSeenVideoDriver.Get() != sVideoDriver ) 
	{
		bSetDefaultVideoParams = true;
		LOG->Trace( "Video card has changed from %s to %s.  Applying new defaults.", PREFSMAN->m_sLastSeenVideoDriver.Get().c_str(), sVideoDriver.c_str() );
	}
		
	if( bSetDefaultVideoParams )
	{
		PREFSMAN->m_sVideoRenderers.Set( defaults.sVideoRenderers );
		PREFSMAN->m_iDisplayWidth.Set( defaults.iWidth );
		PREFSMAN->m_iDisplayHeight.Set( defaults.iHeight );
		PREFSMAN->m_iDisplayColorDepth.Set( defaults.iDisplayColor );
		PREFSMAN->m_iTextureColorDepth.Set( defaults.iTextureColor );
		PREFSMAN->m_iMovieColorDepth.Set( defaults.iMovieColor );
		PREFSMAN->m_iMaxTextureResolution.Set( defaults.iTextureSize );
		PREFSMAN->m_bSmoothLines.Set( defaults.bSmoothLines );
		PREFSMAN->m_fDisplayAspectRatio.Set( HOOKS->GetDisplayAspectRatio() );

		// Update last seen video card
		PREFSMAN->m_sLastSeenVideoDriver.Set( GetVideoDriverName() );
	}
	else if( PREFSMAN->m_sVideoRenderers.Get().CompareNoCase(defaults.sVideoRenderers) )
	{
		LOG->Warn("Video renderer list has been changed from '%s' to '%s'",
				defaults.sVideoRenderers.c_str(), PREFSMAN->m_sVideoRenderers.Get().c_str() );
	}

	LOG->Info( "Video renderers: '%s'", PREFSMAN->m_sVideoRenderers.Get().c_str() );
	return bSetDefaultVideoParams;
}

static LocalizedString ERROR_INITIALIZING_CARD		( "StepMania", "There was an error while initializing your video card." );
static LocalizedString ERROR_DONT_FILE_BUG		( "StepMania", "Please do not file this error as a bug!  Use the web page below to troubleshoot this problem." );
static LocalizedString ERROR_VIDEO_DRIVER		( "StepMania", "Video Driver: %s" );
static LocalizedString ERROR_NO_VIDEO_RENDERERS		( "StepMania", "No video renderers attempted." );
static LocalizedString ERROR_INITIALIZING		( "StepMania", "Initializing %s..." );
static LocalizedString ERROR_UNKNOWN_VIDEO_RENDERER	( "StepMania", "Unknown video renderer value: %s" );

RageDisplay *CreateDisplay()
{
	/* We never want to bother users with having to decide which API to use.
	 *
	 * Some cards simply are too troublesome with OpenGL to ever use it, eg. Voodoos.
	 * If D3D8 isn't installed on those, complain and refuse to run (by default).
	 * For others, always use OpenGL.  Allow forcing to D3D as an advanced option.
	 *
	 * If we're missing acceleration when we load D3D8 due to a card being in the
	 * D3D list, it means we need drivers and that they do exist.
	 *
	 * If we try to load OpenGL and we're missing acceleration, it may mean:
	 *  1. We're missing drivers, and they just need upgrading.
	 *  2. The card doesn't have drivers, and it should be using D3D8.  In other words,
	 *     it needs an entry in this table.
	 *  3. The card doesn't have drivers for either.  (Sorry, no S3 868s.)  Can't play.
	 * 
	 * In this case, fail to load; don't silently fall back on D3D.  We don't want
	 * people unknowingly using D3D8 with old drivers (and reporting obscure bugs
	 * due to driver problems).  We'll probably get bug reports for all three types.
	 * #2 is the only case that's actually a bug.
	 *
	 * Actually, right now we're falling back.  I'm not sure which behavior is better.
	 */

	//bool bAppliedDefaults = CheckVideoDefaultSettings();
	CheckVideoDefaultSettings();

	VideoModeParams params;
	StepMania::GetPreferredVideoModeParams( params );

	RString error = ERROR_INITIALIZING_CARD.GetValue()+"\n\n"+ 
		ERROR_DONT_FILE_BUG.GetValue()+"\n\n"
		VIDEO_TROUBLESHOOTING_URL "\n\n"+
		ssprintf(ERROR_VIDEO_DRIVER.GetValue(), GetVideoDriverName().c_str())+"\n\n";

	vector<RString> asRenderers;
	split( PREFSMAN->m_sVideoRenderers, ",", asRenderers, true );

	if( asRenderers.empty() )
		RageException::Throw( "%s", ERROR_NO_VIDEO_RENDERERS.GetValue().c_str() );

	RageDisplay *pRet = NULL;
	for( unsigned i=0; i<asRenderers.size(); i++ )
	{
		RString sRenderer = asRenderers[i];

		if( sRenderer.CompareNoCase("opengl")==0 )
		{
#if defined(SUPPORT_OPENGL)
			pRet = new RageDisplay_OGL;
#endif
		}
		else if( sRenderer.CompareNoCase("d3d")==0 )
		{
#if defined(SUPPORT_D3D)
			pRet = new RageDisplay_D3D;
#endif
		}
		else if( sRenderer.CompareNoCase("null")==0 )
		{
			return new RageDisplay_Null;
		}
		else
		{
			RageException::Throw( ERROR_UNKNOWN_VIDEO_RENDERER.GetValue(), sRenderer.c_str() );
		}

		if( pRet == NULL )
			continue;

		RString sError = pRet->Init( params, PREFSMAN->m_bAllowUnacceleratedRenderer );
		if( !sError.empty() )
		{
			error += ssprintf(ERROR_INITIALIZING.GetValue(), sRenderer.c_str())+"\n" + sError;
			SAFE_DELETE( pRet );
			error += "\n\n\n";
			continue;
		}

		break;	// the display is ready to go
	}

	if( pRet == NULL)
		RageException::Throw( "%s", error.c_str() );

	return pRet;
}

static void SwitchToLastPlayedGame()
{
	const Game *pGame = GAMEMAN->StringToGame( PREFSMAN->GetCurrentGame() );

	/* If the active game type isn't actually available, revert to the default. */
	if( pGame == NULL )
		pGame = GAMEMAN->GetDefaultGame();
	
	if( !GAMEMAN->IsGameEnabled( pGame ) && pGame != GAMEMAN->GetDefaultGame() )
	{
		pGame = GAMEMAN->GetDefaultGame();
		LOG->Warn( "Default NoteSkin for \"%s\" missing, reverting to \"%s\"",
			pGame->m_szName, GAMEMAN->GetDefaultGame()->m_szName );
	}

	ASSERT( GAMEMAN->IsGameEnabled(pGame) );

	StepMania::ChangeCurrentGame( pGame );
}

void StepMania::ChangeCurrentGame( const Game* g )
{
	ASSERT( g );
	ASSERT( GAMESTATE );
	ASSERT( ANNOUNCER );
	ASSERT( THEME );

	GAMESTATE->SetCurGame( g );

	RString sAnnouncer = PREFSMAN->m_sAnnouncer;
	RString sTheme = PREFSMAN->m_sTheme;
	RString sLanguage = PREFSMAN->m_sLanguage;

	if( sAnnouncer.empty() )
		sAnnouncer = GAMESTATE->GetCurrentGame()->m_szName;
	if( sTheme.empty() )
		sTheme = GAMESTATE->GetCurrentGame()->m_szName;

	// process theme and language command line arguments;
	// these change the preferences as well, for consistency in the UI.
	RString argTheme;
	if( GetCommandlineArgument(	"theme",&argTheme) && argTheme != sTheme )
	{
		sTheme = argTheme;
		PREFSMAN->m_sTheme.Set(sTheme);
	}

	RString argLanguage;
	if( GetCommandlineArgument(	"language",&argLanguage) )
	{
		sLanguage = argLanguage;
		PREFSMAN->m_sLanguage.Set(sLanguage);
	}

	// it's OK to call these functions with names that don't exist.
	ANNOUNCER->SwitchAnnouncer( sAnnouncer );
	THEME->SwitchThemeAndLanguage( sTheme, PREFSMAN->m_sLanguage, PREFSMAN->m_bPseudoLocalize );


	/* Set the input scheme for the new game, and load keymaps. */
	if( INPUTMAPPER )
	{
		INPUTMAPPER->SetInputScheme( &g->m_InputScheme );
		INPUTMAPPER->ReadMappingsFromDisk();
	}
}

static void MountTreeOfZips( const RString &dir )
{
	vector<RString> dirs;
	dirs.push_back( dir );

	while( dirs.size() )
	{
		RString path = dirs.back();
		dirs.pop_back();

#if !defined(XBOX)
		// Xbox doesn't detect directories properly, so we'll ignore this
		if( !IsADirectory(path) )
			continue;
#endif

		vector<RString> zips;
		GetDirListing( path + "/*.zip", zips, false, true );
		GetDirListing( path + "/*.smzip", zips, false, true );

		for( unsigned i = 0; i < zips.size(); ++i )
		{
			if( !IsAFile(zips[i]) )
				continue;

			LOG->Trace( "VFS: found %s", zips[i].c_str() );
			FILEMAN->Mount( "zip", zips[i], "/" );
		}

		GetDirListing( path + "/*", dirs, true, true );
	}
}

#if defined(HAVE_VERSION_INFO)
extern unsigned long version_num;
extern const char *const version_time;
#endif

static void WriteLogHeader()
{
	LOG->Info( PRODUCT_ID_VER );

#if defined(HAVE_VERSION_INFO)
	LOG->Info( "Compiled %s (build %lu)", version_time, version_num );
#endif

	time_t cur_time;
	time(&cur_time);
	struct tm now;
	localtime_r( &cur_time, &now );

	LOG->Info( "Log starting %.4d-%.2d-%.2d %.2d:%.2d:%.2d", 
		1900+now.tm_year, now.tm_mon+1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec );
	LOG->Trace( " " );

	if( g_argc > 1 )
	{
		RString args;
		for( int i = 1; i < g_argc; ++i )
		{
			if( i>1 )
				args += " ";

			// surround all params with some marker, as they might have whitespace.
			// using [[ and ]], as they are not likely to be in the params.
			args += ssprintf( "[[%s]]", g_argv[i] );
		}
		LOG->Info( "Command line args (count=%d): %s", (g_argc - 1), args.c_str());
	}
}

static void ApplyLogPreferences()
{
	LOG->SetShowLogOutput( PREFSMAN->m_bShowLogOutput );
	LOG->SetLogToDisk( PREFSMAN->m_bLogToDisk );
	LOG->SetInfoToDisk( true );
	LOG->SetUserLogToDisk( true );
	LOG->SetFlushing( PREFSMAN->m_bForceLogFlush );
	Checkpoints::LogCheckpoints( PREFSMAN->m_bLogCheckpoints );
}

static LocalizedString COULDNT_OPEN_LOADING_WINDOW( "StepMania", "Couldn't open any loading windows." );

int main(int argc, char* argv[])
{
	RageThreadRegister thread( "Main thread" );
	RageException::SetCleanupHandler( HandleException );
	
	SetCommandlineArguments( argc, argv );


	enum RunMode
	{ 
		RunMode_Normal, 
		RunMode_Install, 
		RunMode_ExportNsisStrings, 
		RunMode_ExportLuaInformation,
	};
	RunMode runmode = RunMode_Normal;
	if( CommandLineActions::AnyPackageFilesInCommandLine() )
		runmode = RunMode_Install;
	else if( GetCommandlineArgument("ExportNsisStrings") )
		runmode = RunMode_ExportNsisStrings;
	else if( GetCommandlineArgument("ExportLuaInformation") )
		runmode = RunMode_ExportLuaInformation;


	/* Set up arch hooks first.  This may set up crash handling. */
	HOOKS = ArchHooks::Create();
	HOOKS->Init();

	LUA		= new LuaManager;

	/* Almost everything uses this to read and write files.  Load this early. */
	FILEMAN = new RageFileManager( argv[0] );
	FILEMAN->MountInitialFilesystems();
	bool bPortable = DoesFileExist("Portable.ini");
	if( !bPortable )
		FILEMAN->MountUserFilesystems();


	/* Set this up next.  Do this early, since it's needed for RageException::Throw. */
	LOG		= new RageLog;

	/* Whew--we should be able to crash safely now! */

	//
	// load preferences and mount any alternative trees.
	//
	PREFSMAN	= new PrefsManager;

	/* Allow HOOKS to check for multiple instances.  We need to do this after PREFS is initialized,
	 * so ArchHooks can use a preference to turn this off.  We want to do this before ApplyLogPreferences,
	 * so if we exit because of another instance, we don't try to clobber its log.  We also want to
	 * do this before opening the loading window, so if we give focus away, we don't flash the window. */
	if( !g_bAllowMultipleInstances.Get() && HOOKS->CheckForMultipleInstances() )
	{
		ShutdownGame();
		return 0;
	}

	ApplyLogPreferences();

#if defined(XBOX)
	vmem_Manager.Init();
#endif

	WriteLogHeader();

	/* Set up alternative filesystem trees. */
	if( PREFSMAN->m_sAdditionalFolders.Get() != "" )
	{
		vector<RString> dirs;
		split( PREFSMAN->m_sAdditionalFolders, ",", dirs, true );
		for( unsigned i=0; i < dirs.size(); i++)
			FILEMAN->Mount( "dir", dirs[i], "/" );
	}
	if( PREFSMAN->m_sAdditionalSongFolders.Get() != "" )
	{
		vector<RString> dirs;
		split( PREFSMAN->m_sAdditionalSongFolders, ",", dirs, true );
		for( unsigned i=0; i < dirs.size(); i++)
			FILEMAN->Mount( "dir", dirs[i], "/AdditionalSongs" );
	}
	if( PREFSMAN->m_sAdditionalCourseFolders.Get() != "" )
	{
		vector<RString> dirs;
		split( PREFSMAN->m_sAdditionalCourseFolders, ",", dirs, true );
		for( unsigned i=0; i < dirs.size(); i++)
			FILEMAN->Mount( "dir", dirs[i], "/AdditionalCourses" );
	}

	MountTreeOfZips( SpecialFiles::PACKAGES_DIR );
	MountTreeOfZips( SpecialFiles::USER_PACKAGES_DIR );

	/* One of the above filesystems might contain files that affect preferences, eg Data/Static.ini.
	 * Re-read preferences. */
	PREFSMAN->ReadPrefsFromDisk();
	ApplyLogPreferences();
	
	/* This needs PREFSMAN. */
	Dialog::Init();

	//
	// Create game objects
	//

	GAMESTATE	= new GameState;

	/* This requires PREFSMAN, for PREFSMAN->m_bShowLoadingWindow. */
	LoadingWindow *pLoadingWindow = NULL;
	switch( runmode )
	{
	case RunMode_Normal:
		pLoadingWindow = LoadingWindow::Create();
		if( pLoadingWindow == NULL )
			RageException::Throw( "%s", COULDNT_OPEN_LOADING_WINDOW.GetValue().c_str() );
		break;
	default:
		;	// no loading window for other RunModes
	}

	srand( time(NULL) );	// seed number generator	
	
	/* Do this early, so we have debugging output if anything else fails.  LOG and
	 * Dialog must be set up first.  It shouldn't take long, but it might take a
	 * little time; do this after the LoadingWindow is shown, since we don't want
	 * that to appear delayed. */
	HOOKS->DumpDebugInfo();

#if defined(HAVE_TLS)
	LOG->Info( "TLS is %savailable", RageThread::GetSupportsTLS()? "":"not " );
#endif

	AdjustForChangedSystemCapabilities();

	GAMEMAN		= new GameManager;
	THEME		= new ThemeManager;
	ANNOUNCER	= new AnnouncerManager;
	NOTESKIN	= new NoteSkinManager;

	/* Switch to the last used game type, and set up the theme and announcer. */
	SwitchToLastPlayedGame();


	// Handle special RunModes.  Some of these depend on ThemeManager being loaded above in SwitchToLastPlayedGame.
	switch( runmode )
	{
	DEFAULT_FAIL( runmode );
	case RunMode_Normal:
		break;
	case RunMode_Install:
	case RunMode_ExportNsisStrings:
	case RunMode_ExportLuaInformation:
		THEME->SwitchThemeAndLanguage( "default", PREFSMAN->m_sLanguage, PREFSMAN->m_bPseudoLocalize );
		switch( runmode )
		{
		DEFAULT_FAIL( runmode );
		case RunMode_Install:
			CommandLineActions::Install();
			break;
		case RunMode_ExportNsisStrings:
			CommandLineActions::Nsis();
			break;
		case RunMode_ExportLuaInformation:
			CommandLineActions::LuaInformation();
			break;
		};
		exit(0);
	}



	{
		/* Now that THEME is loaded, load the icon for the current theme into the
		 * loading window. */
		RString sError;
		RageSurface *pIcon = RageSurfaceUtils::LoadFile( THEME->GetPathG( "Common", "window icon" ), sError );
		if( pIcon )
			pLoadingWindow->SetIcon( pIcon );
		delete pIcon;
	}

	if( PREFSMAN->m_iSoundWriteAhead )
		LOG->Info( "Sound writeahead has been overridden to %i", PREFSMAN->m_iSoundWriteAhead.Get() );
	SOUNDMAN	= new RageSoundManager;
	SOUNDMAN->Init();
	SOUNDMAN->SetMixVolume();
	SOUND		= new GameSoundManager;
	BOOKKEEPER	= new Bookkeeper;
	LIGHTSMAN	= new LightsManager;
	INPUTFILTER	= new InputFilter;
	INPUTMAPPER	= new InputMapper;

	StepMania::ChangeCurrentGame( GAMESTATE->GetCurrentGame() );

	INPUTQUEUE	= new InputQueue;
	SONGINDEX	= new SongCacheIndex;
	BANNERCACHE	= new BannerCache;
	
	/* depends on SONGINDEX: */
	SONGMAN		= new SongManager;
	SONGMAN->InitAll( pLoadingWindow );	// this takes a long time
	CRYPTMAN	= new CryptManager;		// need to do this before ProfileMan
	if( PREFSMAN->m_bSignProfileData )
		CRYPTMAN->GenerateGlobalKeys();
	MEMCARDMAN	= new MemoryCardManager;
	CHARMAN		= new CharacterManager;
	PROFILEMAN	= new ProfileManager;
	PROFILEMAN->Init();				// must load after SONGMAN
	UNLOCKMAN	= new UnlockManager;
	SONGMAN->UpdatePopular();
	SONGMAN->UpdatePreferredSort();

	NSMAN 		= new NetworkSyncManager( pLoadingWindow ); 
	MESSAGEMAN	= new MessageManager;
	STATSMAN	= new StatsManager;

	SAFE_DELETE( pLoadingWindow );		// destroy this before init'ing Display

	/* If the user has tried to quit during the loading, do it before creating the
	 * main window. This prevents going to full screen just to quit. */
	if( ArchHooks::UserQuit() )
	{
		ShutdownGame();
		return 0;
	}
	
	StartDisplay();

	StoreActualGraphicOptions();
	LOG->Info( "%s", GetActualGraphicOptionsString().c_str() );

	SONGMAN->PreloadSongImages();

	/* Input handlers can have dependences on the video system so
	 * INPUTMAN must be initialized after DISPLAY. */
	INPUTMAN	= new RageInput;

	// These things depend on the TextureManager, so do them after!
	FONT		= new FontManager;
	SCREENMAN	= new ScreenManager;

	StepMania::ResetGame();

	/* Now that GAMESTATE is reset, tell SCREENMAN to update the theme (load
	 * overlay screens and global sounds), and load the initial screen. */
	SCREENMAN->ThemeChanged();
	SCREENMAN->SetNewScreen( StepMania::GetInitialScreen() );

	// Do this after ThemeChanged so that we can show a system message
	RString sMessage;
	if( INPUTMAPPER->CheckForChangedInputDevicesAndRemap(sMessage) )
		SCREENMAN->SystemMessage( sMessage );

	CodeDetector::RefreshCacheItems();

	/* Initialize which courses are ranking courses here. */
	SONGMAN->UpdateRankingCourses();

	if( GetCommandlineArgument("netip") )
		NSMAN->DisplayStartupStatus();	// If we're using networking show what happened

	/* Run the main loop. */
	GameLoop::RunGameLoop();
	
	PREFSMAN->SavePrefsToDisk();

	ShutdownGame();
	
	return 0;
}

RString StepMania::SaveScreenshot( RString sDir, bool bSaveCompressed, bool bMakeSignature, int iIndex )
{
	/* Files should be of the form "screen#####.xxx".  Ignore the extension; find
	 * the last file of this form, and use the next number.  This way, we don't
	 * write the same screenshot number for different formats (screen00011.bmp,
	 * screen00011.jpg), and we always increase from the end, so if screen00003.jpg
	 * is deleted, we won't fill in the hole (which makes screenshots hard to find). */
	RString sFileNameNoExtension;
	if( iIndex == -1 ) 
		sFileNameNoExtension = Profile::MakeUniqueFileNameNoExtension( sDir, "screen" );
	else
		sFileNameNoExtension = Profile::MakeFileNameNoExtension( "screen", iIndex );


	//
	// Save the screenshot.  If writing lossy to a memcard, use SAVE_LOSSY_LOW_QUAL, so we
	// don't eat up lots of space.
	//
	RageDisplay::GraphicsFileFormat fmt;
	if( bSaveCompressed && MEMCARDMAN->PathIsMemCard(sDir) )
		fmt = RageDisplay::SAVE_LOSSY_LOW_QUAL;
	else if( bSaveCompressed )
		fmt = RageDisplay::SAVE_LOSSY_HIGH_QUAL;
	else
		fmt = RageDisplay::SAVE_LOSSLESS;

	RString sFileName = sFileNameNoExtension + "." + (bSaveCompressed ? "jpg" : "bmp");
	RString sPath = sDir+sFileName;
	bool bResult = DISPLAY->SaveScreenshot( sPath, fmt );
	if( !bResult )
	{
		SCREENMAN->PlayInvalidSound();
		return RString();
	}

	SCREENMAN->PlayScreenshotSound();

	if( PREFSMAN->m_bSignProfileData && bMakeSignature )
		CryptManager::SignFileToFile( sPath );

	return sFileName;
}

void StepMania::InsertCoin( int iNum, bool bCountInBookkeeping )
{
	if( bCountInBookkeeping )
	{
		LIGHTSMAN->PulseCoinCounter();
		BOOKKEEPER->CoinInserted();
	}
	int iNumCoinsOld = GAMESTATE->m_iCoins;
	GAMESTATE->m_iCoins.Set( GAMESTATE->m_iCoins + iNum );

	int iCredits = GAMESTATE->m_iCoins / PREFSMAN->m_iCoinsPerCredit;
	bool bMaxCredits = iCredits >= MAX_NUM_CREDITS;
	if( bMaxCredits )
		GAMESTATE->m_iCoins.Set( MAX_NUM_CREDITS * PREFSMAN->m_iCoinsPerCredit );

	LOG->Trace("%i coins inserted, %i needed to play", GAMESTATE->m_iCoins.Get(), PREFSMAN->m_iCoinsPerCredit.Get() );
	if( iNumCoinsOld != GAMESTATE->m_iCoins )
		SCREENMAN->PlayCoinSound();
	else
		SCREENMAN->PlayInvalidSound();

	/* If AutoJoin and a player is already joined, then try to join a player.  (If no players
	 * are joined, they'll join on the first JoinInput.) */
	if( GAMESTATE->m_bAutoJoin.Get() && GAMESTATE->GetNumSidesJoined() > 0 )
	{
		if( GAMESTATE->GetNumSidesJoined() > 0 && GAMESTATE->JoinPlayers() )
			SCREENMAN->PlayStartSound();
	}

	/* TODO: remove this redundant message and things that depend on it */
	Message msg( "CoinInserted" );
	// below params are unused
	//msg.SetParam( "Coins", GAMESTATE->m_iCoins );
	//msg.SetParam( "Inserted", iNum );
	//msg.SetParam( "MaxCredits", bMaxCredits );
	MESSAGEMAN->Broadcast( msg );
}

void StepMania::InsertCredit()
{
	InsertCoin( PREFSMAN->m_iCoinsPerCredit, false );
}

void StepMania::ClearCredits()
{
	LOG->Trace("%i coins cleared", GAMESTATE->m_iCoins.Get() );
	GAMESTATE->m_iCoins.Set( 0 );
	SCREENMAN->PlayInvalidSound();
	
	/* TODO: remove this redundant message and things that depend on it */
	Message msg( "CoinInserted" );
	// below params are unused
	//msg.SetParam( "Coins", GAMESTATE->m_iCoins );
	//msg.SetParam( "Clear", true );
	MESSAGEMAN->Broadcast( msg );
}

/* Returns true if the key has been handled and should be discarded, false if
 * the key should be sent on to screens. */
static LocalizedString SERVICE_SWITCH_PRESSED ( "StepMania", "Service switch pressed" );
bool HandleGlobalInputs( const InputEventPlus &input )
{
	/* None of the globals keys act on types other than FIRST_PRESS */
	if( input.type != IET_FIRST_PRESS ) 
		return false;

	switch( input.MenuI )
	{
	case GAME_BUTTON_OPERATOR:

		/* Global operator key, to get quick access to the options menu. Don't
		 * do this if we're on a "system menu", which includes the editor
		 * (to prevent quitting without storing changes). */
		if( SCREENMAN->AllowOperatorMenuButton() )
		{
			SCREENMAN->SystemMessage( SERVICE_SWITCH_PRESSED );
			SCREENMAN->PopAllScreens();
			GAMESTATE->Reset();
			SCREENMAN->SetNewScreen( "ScreenOptionsService" );
		}
		return true;

	case GAME_BUTTON_COIN:
		/* Handle a coin insertion. */
		if( GAMESTATE->IsEditing() )	// no coins while editing
		{
			LOG->Trace( "Ignored coin insertion (editing)" );
			break;
		}
		StepMania::InsertCoin();
		return false;	// Attract need to know because they go to TitleMenu on > 1 credit
	}

#if !defined(MACOSX)
	if( input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F4) )
	{
		if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RALT), &input.InputList) ||
			INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LALT), &input.InputList) )
		{
			// pressed Alt+F4
			ArchHooks::SetUserQuit();
			return true;
		}
	}
#else
	if( input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_Cq) &&
	    (INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LMETA), &input.InputList ) ||
	     INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RMETA), &input.InputList )) )
	{
		/* The user quit is handled by the menu item so we don't need to set it here;
		 * however, we do want to return that it has been handled since this will happen
		 * first. */
		return true;
	}
#endif

	bool bDoScreenshot = 
#if defined(MACOSX)
	/* Notebooks don't have F13. Use cmd-F12 as well. */
		input.DeviceI == DeviceInput( DEVICE_KEYBOARD, KEY_PRTSC ) ||
		input.DeviceI == DeviceInput( DEVICE_KEYBOARD, KEY_F13 ) ||
		( input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F12) && 
		  (INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LMETA), &input.InputList) ||
		   INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RMETA), &input.InputList)) );

#else
	/* The default Windows message handler will capture the desktop window upon
	 * pressing PrntScrn, or will capture the foregroud with focus upon pressing
	 * Alt+PrntScrn.  Windows will do this whether or not we save a screenshot 
	 * ourself by dumping the frame buffer.  */
	// "if pressing PrintScreen and not pressing Alt"
		input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_PRTSC) && 
		!INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LALT), &input.InputList) &&
		!INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RALT), &input.InputList);
#endif
	if( bDoScreenshot )
	{
		// If holding LShift save uncompressed, else save compressed
		bool bSaveCompressed = !INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT) );
		RageTimer timer;
		StepMania::SaveScreenshot( "Screenshots/", bSaveCompressed, false );
		LOG->Trace( "Screenshot took %f seconds.", timer.GetDeltaTime() );
		return true;	// handled
	}
	
	if( input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_ENTER) &&
		(INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RALT), &input.InputList) ||
		 INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LALT), &input.InputList)) )
	{
		/* alt-enter */
		/* In OS X, this is a menu item and will be handled as such. This will happen
		 * first and then the lower priority GUI thread will happen second causing the
		 * window to toggle twice. Another solution would be to put a timer in
		 * ArchHooks::SetToggleWindowed() and just not set the bool it if it's been less
		 * than, say, half a second. */
#if !defined(MACOSX)
		ArchHooks::SetToggleWindowed();
#endif
		return true;
	}

	return false;
}

void HandleInputEvents(float fDeltaTime)
{
	INPUTFILTER->Update( fDeltaTime );
	
	/* Hack: If the topmost screen hasn't been updated yet, don't process input, since
	 * we must not send inputs to a screen that hasn't at least had one update yet. (The
	 * first Update should be the very first thing a screen gets.)  We'll process it next
	 * time.  Do call Update above, so the inputs are read and timestamped. */
	if( SCREENMAN->GetTopScreen()->IsFirstUpdate() )
		return;

	vector<InputEvent> ieArray;
	INPUTFILTER->GetInputEvents( ieArray );

	/* If we don't have focus, discard input unless there's AcceptInputOnNoFocus option set. */
	if( !HOOKS->AppHasFocus() )
		if ( !PREFSMAN->m_bAcceptInputOnNoFocus )
			return;

	for( unsigned i=0; i<ieArray.size(); i++ )
	{
		InputEventPlus input;
		input.DeviceI = ieArray[i].di;
		input.type = ieArray[i].type;
		swap( input.InputList, ieArray[i].m_ButtonState );

		// hack for testing with only one joytick
		if( input.DeviceI.IsJoystick() )
		{
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD,KEY_LSHIFT) ) )
				input.DeviceI.device = (InputDevice)(input.DeviceI.device + 1);
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD,KEY_LCTRL) ) )
				input.DeviceI.device = (InputDevice)(input.DeviceI.device + 2);
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD,KEY_LALT) ) )
				input.DeviceI.device = (InputDevice)(input.DeviceI.device + 4);
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD,KEY_RALT) ) )
				input.DeviceI.device = (InputDevice)(input.DeviceI.device + 8);
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD,KEY_RCTRL) ) )
				input.DeviceI.device = (InputDevice)(input.DeviceI.device + 16);
		}

		INPUTMAPPER->DeviceToGame( input.DeviceI, input.GameI );
		
		input.mp = MultiPlayer_Invalid;
		
		{
			// Translate input to the appropriate MultiPlayer.  Assume that all
			// joystick devices are mapped the same as the master player.
			if( input.DeviceI.IsJoystick() )
			{
				DeviceInput diTemp = input.DeviceI;
				diTemp.device = DEVICE_JOY1;
				GameInput gi;

				//LOG->Trace( "device %d, %d", diTemp.device, diTemp.button );
				if( INPUTMAPPER->DeviceToGame(diTemp, gi) )
				{
					if( GAMESTATE->m_bMultiplayer )
					{
						input.GameI = gi;
						//LOG->Trace( "game %d %d", input.GameI.controller, input.GameI.button );
					}

					input.mp = InputMapper::InputDeviceToMultiPlayer( input.DeviceI.device );
					//LOG->Trace( "multiplayer %d", input.mp );
					ASSERT( input.mp >= 0 && input.mp < NUM_MultiPlayer );					
				}
			}
		}

		if( input.GameI.IsValid() )
		{
			input.MenuI = INPUTMAPPER->GameButtonToMenuButton( input.GameI.button );
			input.pn = INPUTMAPPER->ControllerToPlayerNumber( input.GameI.controller );
		}

		INPUTQUEUE->RememberInput( input );

		/* When a GameButton is pressed, stop repeating other keys on the same controller. */
		if( input.type == IET_FIRST_PRESS && input.MenuI != GameButton_Invalid )
		{
			FOREACH_ENUM( GameButton,  m )
			{
				if( input.MenuI != m )
					INPUTMAPPER->RepeatStopKey( m, input.pn );
			}
		}

		if( HandleGlobalInputs(input) )
			continue;	// skip
		
		// check back in event mode
		if( GAMESTATE->IsEventMode() &&
			CodeDetector::EnteredCode(input.GameI.controller,CODE_BACK_IN_EVENT_MODE) )
		{
			input.pn = PLAYER_1;
			input.MenuI = GAME_BUTTON_BACK;
		}

		SCREENMAN->Input( input );
	}
	
	if( ArchHooks::GetAndClearToggleWindowed() )
	{
		PREFSMAN->m_bWindowed.Set( !PREFSMAN->m_bWindowed );
		StepMania::ApplyGraphicOptions();
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

