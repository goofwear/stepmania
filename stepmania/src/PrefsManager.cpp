#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: PrefsManager

 Desc: See Header.

 Copyright (c) 2001-2003 by the person(s) listed below.  All rights reserved.
	Chris Danford
	Chris Gomez
-----------------------------------------------------------------------------
*/

#include "PrefsManager.h"
#include "IniFile.h"
#include "GameState.h"
#include "RageException.h"
#include "RageDisplay.h"
#include "RageUtil.h"
#include "arch/arch.h" /* for default driver specs */
#include "RageSoundReader_Resample.h" /* for ResampleQuality */

#define STEPMANIA_INI_PATH BASE_PATH "Data" SLASH "StepMania.ini"

PrefsManager*	PREFSMAN = NULL;	// global and accessable from anywhere in our program

const float DEFAULT_SOUND_VOLUME = 0.50;

PrefsManager::PrefsManager()
{
#ifdef DEBUG
	m_bWindowed = true;
#else
	m_bWindowed = false;
#endif
	m_iDisplayWidth = 640;
	m_iDisplayHeight = 480;
	m_iDisplayColorDepth = 16;
	m_iTextureColorDepth = 16;		// default to 16 for better preformance on slower cards
	m_iMovieColorDepth = 16;
	m_iMaxTextureResolution = 2048;
	m_iRefreshRate = REFRESH_DEFAULT;
	m_bOnlyDedicatedMenuButtons = false;
#ifdef DEBUG
	m_bShowStats = true;
#else
	m_bShowStats = false;
#endif
	m_BackgroundMode = BGMODE_ANIMATIONS;
	m_iNumBackgrounds = 8;
	m_bShowDanger = true;
	m_fBGBrightness = 0.8f;
	m_bMenuTimer = true;
	m_iNumArcadeStages = 3;
	m_bEventMode = false;
	m_bAutoPlay = false;
	m_fJudgeWindowScale = 1.0f;
	m_fJudgeWindowMarvelousSeconds = 0.0225f;
	m_fJudgeWindowPerfectSeconds = 0.045f;
	m_fJudgeWindowGreatSeconds = 0.090f;
	m_fJudgeWindowGoodSeconds = 0.135f;
	m_fJudgeWindowBooSeconds = 0.180f;
	m_fJudgeWindowOKSeconds = 0.250f;	// allow enough time to take foot off and put back on
	m_fLifeDifficultyScale = 1.0f;
	m_iMovieDecodeMS = 2;
	m_bDelayedEscape = true;
	m_bInstructions = true;
	m_bShowDontDie = true;
	m_bShowSelectGroup = true;
	m_bShowNative = true;
	m_bArcadeOptionsNavigation = false;
	m_bSoloSingle = false;
	m_bDelayedTextureDelete = true;
	m_bDelayedScreenLoad = false;
	m_bBannerCache = true;
	m_MusicWheelUsesSections = ALWAYS;
	m_iMusicWheelSwitchSpeed = 10;
	m_bEasterEggs = true;
	m_iMarvelousTiming = 2;
	m_iCoinMode = COIN_HOME;
	m_iCoinsPerCredit = 1;
	m_bJointPremium = false;
	m_iBoostAppPriority = -1;
	m_bAntiAliasing = false;
	m_ShowSongOptions = YES;
	m_bDancePointsForOni = false;
	m_bTimestamping = false;
	m_bShowLyrics = true;
	m_bAutogenMissingTypes = true;
	m_bAutogenGroupCourses = true;
	m_bBreakComboToGetItem = false;
	m_ShowDancingCharacters = CO_OFF;
	m_bUseUnlockSystem = false;
	m_bFirstRun = true;
	m_bAutoMapJoysticks = true;
	m_fGlobalOffsetSeconds = 0;
	m_bForceLogFlush = false;
	m_bLogging = true;
	m_bShowBeginnerHelper = false;
	m_bEndlessBreakEnabled = true;
	m_iEndlessNumStagesUntilBreak = 5;
	m_iEndlessBreakLength = 5;
#ifdef DEBUG
	m_bShowLogWindow = true;
#else
	m_bShowLogWindow = false;
#endif
	m_bTenFooterInRed = true;

	// set to 0 so people aren't shocked at first
	m_iProgressiveLifebar = 0;
	m_iProgressiveNonstopLifebar = 0;
	m_iProgressiveStageLifebar = 0;

	/* DDR Extreme-style extra stage support.
	 * Default off so people used to the current behavior (or those with extra
	 * stage CRS files) don't get it changed around on them. */
	m_bPickExtraStage = false;

	m_bComboContinuesBetweenSongs = false;

	// default to old sort order
	m_iCourseSortOrder = COURSE_SORT_SONGS;
	m_bMoveRandomToEnd = false;
	m_iScoringType = SCORING_MAX2;

	m_fLongVerSongSeconds = 60*2.5f;	// Dynamite Rave is 2:55
	m_fMarathonVerSongSeconds = 60*5.f;

	/* I'd rather get occasional people asking for support for this even though it's
	 * already here than lots of people asking why songs aren't being displayed. */
	m_bHiddenSongs = false;
	m_bVsync = true;
	m_sLanguage = "";	// ThemeManager will deal with this invalid language


	/* XXX: Set these defaults for individual consoles using VideoCardDefaults.ini. */
#ifdef _XBOX
	m_bInterlaced = true;
	m_bPAL = false;
#else
	m_bInterlaced = false;
#endif
	m_sSoundDrivers = DEFAULT_SOUND_DRIVER_LIST;
	m_sMovieDrivers = DEFAULT_MOVIE_DRIVER_LIST;

	// StepMania.cpp sets these on first run:
	m_sVideoRenderers = "";
#if defined(WIN32)
	m_iLastSeenMemory = 0;
#endif

	m_fSoundVolume = DEFAULT_SOUND_VOLUME;
	/* This is experimental: let's see if preloading helps people's skipping.
	 * If it doesn't do anything useful, it'll be removed. */
	m_bSoundPreloadAll = false;
	m_iSoundResampleQuality = RageSoundReader_Resample::RESAMP_NORMAL;

	m_bAllowUnacceleratedRenderer = false;
	m_bThreadedInput = true;
	m_sIgnoredMessageWindows = "";

	m_sCoursesToShowRanking = "";

	ReadGlobalPrefsFromDisk( true );
}

PrefsManager::~PrefsManager()
{
}

void PrefsManager::ReadGlobalPrefsFromDisk( bool bSwitchToLastPlayedGame )
{
	IniFile ini;
	ini.SetPath( STEPMANIA_INI_PATH );
	if( !ini.ReadFile() )
		return;		// could not read config file, load nothing

	ini.GetValueB( "Options", "Windowed",						m_bWindowed );
	ini.GetValueB( "Options", "Interlaced",						m_bInterlaced );
#ifdef _XBOX
	ini.GetValueB( "Options", "PAL",							m_bPAL );
#endif
	ini.GetValueI( "Options", "DisplayWidth",					m_iDisplayWidth );
	ini.GetValueI( "Options", "DisplayHeight",					m_iDisplayHeight );
	ini.GetValueI( "Options", "DisplayColorDepth",				m_iDisplayColorDepth );
	ini.GetValueI( "Options", "TextureColorDepth",				m_iTextureColorDepth );
	ini.GetValueI( "Options", "MovieColorDepth",				m_iMovieColorDepth );
	ini.GetValueI( "Options", "MaxTextureResolution",			m_iMaxTextureResolution );
	ini.GetValueI( "Options", "RefreshRate",					m_iRefreshRate );
	ini.GetValueB( "Options", "UseDedicatedMenuButtons",		m_bOnlyDedicatedMenuButtons );
	ini.GetValueB( "Options", "ShowStats",						m_bShowStats );
	ini.GetValueI( "Options", "BackgroundMode",					(int&)m_BackgroundMode );
	ini.GetValueI( "Options", "NumBackgrounds",					m_iNumBackgrounds);
	ini.GetValueB( "Options", "ShowDanger",						m_bShowDanger );
	ini.GetValueF( "Options", "BGBrightness",					m_fBGBrightness );
	ini.GetValueB( "Options", "MenuTimer",						m_bMenuTimer );
	ini.GetValueI( "Options", "NumArcadeStages",				m_iNumArcadeStages );
	ini.GetValueB( "Options", "EventMode",						m_bEventMode );
	ini.GetValueB( "Options", "AutoPlay",						m_bAutoPlay );
	ini.GetValueF( "Options", "JudgeWindowScale",				m_fJudgeWindowScale );
	ini.GetValueF( "Options", "JudgeWindowMarvelousSeconds",	m_fJudgeWindowMarvelousSeconds );
	ini.GetValueF( "Options", "JudgeWindowPerfectSeconds",		m_fJudgeWindowPerfectSeconds );
	ini.GetValueF( "Options", "JudgeWindowGreatSeconds",		m_fJudgeWindowGreatSeconds );
	ini.GetValueF( "Options", "JudgeWindowGoodSeconds",			m_fJudgeWindowGoodSeconds );
	ini.GetValueF( "Options", "JudgeWindowBooSeconds",			m_fJudgeWindowBooSeconds );
	ini.GetValueF( "Options", "JudgeWindowOKSeconds",			m_fJudgeWindowOKSeconds );
	ini.GetValueF( "Options", "LifeDifficultyScale",			m_fLifeDifficultyScale );
	ini.GetValueI( "Options", "MovieDecodeMS",					m_iMovieDecodeMS );
	ini.GetValueB( "Options", "DelayedEscape",					m_bDelayedEscape );
	ini.GetValueB( "Options", "HiddenSongs",					m_bHiddenSongs );
	ini.GetValueB( "Options", "Vsync",							m_bVsync );
	ini.GetValueB( "Options", "HowToPlay",						m_bInstructions );
	ini.GetValueB( "Options", "Caution",						m_bShowDontDie );
	ini.GetValueB( "Options", "ShowSelectGroup",				m_bShowSelectGroup );
	ini.GetValueB( "Options", "ShowNative",						m_bShowNative );
	ini.GetValueB( "Options", "ArcadeOptionsNavigation",		m_bArcadeOptionsNavigation );
	ini.GetValue ( "Options", "DWIPath",						m_DWIPath );
	ini.GetValueB( "Options", "DelayedTextureDelete",			m_bDelayedTextureDelete );
	ini.GetValueB( "Options", "DelayedScreenLoad",				m_bDelayedScreenLoad );
	ini.GetValueB( "Options", "BannerCache",					m_bBannerCache );
	ini.GetValueI( "Options", "MusicWheelUsesSections",			(int&)m_MusicWheelUsesSections );
	ini.GetValueI( "Options", "MusicWheelSwitchSpeed",			m_iMusicWheelSwitchSpeed );
	ini.GetValue ( "Options", "SoundDrivers",					m_sSoundDrivers );
	ini.GetValue ( "Options", "MovieDrivers",					m_sMovieDrivers );
	ini.GetValueB( "Options", "EasterEggs",						m_bEasterEggs );
	ini.GetValueI( "Options", "MarvelousTiming",				(int&)m_iMarvelousTiming );
	ini.GetValueF( "Options", "SoundVolume",					m_fSoundVolume );
	ini.GetValueB( "Options", "SoundPreloadAll",				m_bSoundPreloadAll );
	ini.GetValueI( "Options", "SoundResampleQuality",			m_iSoundResampleQuality );
	ini.GetValueI( "Options", "CoinMode",						m_iCoinMode );
	ini.GetValueI( "Options", "CoinsPerCredit",					m_iCoinsPerCredit );
	ini.GetValueB( "Options", "JointPremium",					m_bJointPremium );
	ini.GetValueI( "Options", "BoostAppPriority",				m_iBoostAppPriority );
	ini.GetValueB( "Options", "PickExtraStage",					m_bPickExtraStage );
	ini.GetValueB( "Options", "ComboContinuesBetweenSongs",		m_bComboContinuesBetweenSongs );
	ini.GetValueF( "Options", "LongVerSeconds",					m_fLongVerSongSeconds );
	ini.GetValueF( "Options", "MarathonVerSeconds",				m_fMarathonVerSongSeconds );
	ini.GetValueI( "Options", "ShowSongOptions",				(int&)m_ShowSongOptions );
	ini.GetValueB( "Options", "AllowUnacceleratedRenderer",		m_bAllowUnacceleratedRenderer );
	ini.GetValueB( "Options", "ThreadedInput",					m_bThreadedInput );
	ini.GetValue ( "Options", "IgnoredMessageWindows",			m_sIgnoredMessageWindows );
	ini.GetValueB( "Options", "SoloSingle",						m_bSoloSingle );
	ini.GetValueB( "Options", "DancePointsForOni",				m_bDancePointsForOni );
	ini.GetValueB( "Options", "ShowLyrics",						m_bShowLyrics );
	ini.GetValueB( "Options", "AutogenMissingTypes",			m_bAutogenMissingTypes );
	ini.GetValueB( "Options", "AutogenGroupCourses",			m_bAutogenGroupCourses );
	ini.GetValueB( "Options", "Timestamping",					m_bTimestamping );
	ini.GetValueB( "Options", "BreakComboToGetItem",			m_bBreakComboToGetItem );
	ini.GetValueI( "Options", "ShowDancingCharacters",			(int&)m_ShowDancingCharacters );
	ini.GetValueB( "Options", "TenFooterInRed",					m_bTenFooterInRed );

	ini.GetValueI( "Options", "CourseSortOrder",				(int&)m_iCourseSortOrder );
	ini.GetValueB( "Options", "MoveRandomToEnd",				m_bMoveRandomToEnd );

	ini.GetValueI( "Options", "ScoringType",					(int&)m_iScoringType );

	ini.GetValueI( "Options", "ProgressiveLifebar",				(int&)m_iProgressiveLifebar );
	ini.GetValueI( "Options", "ProgressiveNonstopLifebar", 		(int&)m_iProgressiveNonstopLifebar );
	ini.GetValueI( "Options", "ProgressiveStageLifebar",		(int&)m_iProgressiveStageLifebar );

	ini.GetValueB( "Options", "UseUnlockSystem",				m_bUseUnlockSystem );

	ini.GetValueB( "Options", "FirstRun",						m_bFirstRun );
	ini.GetValueB( "Options", "AutoMapJoysticks",				m_bAutoMapJoysticks );
	ini.GetValue ( "Options", "VideoRenderers",					m_sVideoRenderers );
	ini.GetValue ( "Options", "LastSeenVideoDriver",			m_sLastSeenVideoDriver );
#if defined(WIN32)
	ini.GetValue ( "Options", "LastSeenMemory",					m_iLastSeenMemory );
#endif
	ini.GetValue ( "Options", "CoursesToShowRanking",			m_sCoursesToShowRanking );
	ini.GetValueB( "Options", "AntiAliasing",					m_bAntiAliasing );
	ini.GetValueF( "Options", "GlobalOffsetSeconds",			m_fGlobalOffsetSeconds );
	ini.GetValueB( "Options", "ForceLogFlush",					m_bForceLogFlush );
	ini.GetValueB( "Options", "Logging",						m_bLogging );
	ini.GetValueB( "Options", "ShowLogWindow",					m_bShowLogWindow );
	ini.GetValueB( "Options", "ShowBeginnerHelper",				m_bShowBeginnerHelper );
	ini.GetValue ( "Options", "Language",						m_sLanguage );
	ini.GetValueB( "Options", "EndlessBreakEnabled",			m_bEndlessBreakEnabled );
	ini.GetValueI( "Options", "EndlessStagesUntilBreak",		m_iEndlessNumStagesUntilBreak );
	ini.GetValueI( "Options", "EndlessBreakLength",				m_iEndlessBreakLength );

	for( int p=0; p<NUM_PLAYERS; p++ )
		ini.GetValue ( "Options", ssprintf("DefaultProfileP%d",p+1),	m_sDefaultProfile[p] );


	m_asAdditionalSongFolders.clear();
	CString sAdditionalSongFolders;
	ini.GetValue( "Options", "AdditionalSongFolders",			sAdditionalSongFolders );
	split( sAdditionalSongFolders, ",", m_asAdditionalSongFolders, true );

	if( bSwitchToLastPlayedGame )
	{
		Game game;
		if( ini.GetValueI("Options", "Game", (int&)game) )
			GAMESTATE->m_CurGame = game;
	}
}


void PrefsManager::SaveGlobalPrefsToDisk()
{
	IniFile ini;
	ini.SetPath( STEPMANIA_INI_PATH );

	ini.SetValueB( "Options", "Windowed",						m_bWindowed );
	ini.SetValueI( "Options", "DisplayWidth",					m_iDisplayWidth );
	ini.SetValueI( "Options", "DisplayHeight",					m_iDisplayHeight );
	ini.SetValueI( "Options", "DisplayColorDepth",				m_iDisplayColorDepth );
	ini.SetValueI( "Options", "TextureColorDepth",				m_iTextureColorDepth );
	ini.SetValueI( "Options", "MovieColorDepth",				m_iMovieColorDepth );
	ini.SetValueI( "Options", "MaxTextureResolution",			m_iMaxTextureResolution );
	ini.SetValueI( "Options", "RefreshRate",					m_iRefreshRate );
	ini.SetValueB( "Options", "UseDedicatedMenuButtons",		m_bOnlyDedicatedMenuButtons );
	ini.SetValueB( "Options", "ShowStats",						m_bShowStats );
	ini.SetValueI( "Options", "BackgroundMode",					m_BackgroundMode);
	ini.SetValueI( "Options", "NumBackgrounds",					m_iNumBackgrounds);
	ini.SetValueB( "Options", "ShowDanger",						m_bShowDanger );
	ini.SetValueF( "Options", "BGBrightness",					m_fBGBrightness );
	ini.SetValueB( "Options", "MenuTimer",						m_bMenuTimer );
	ini.SetValueI( "Options", "NumArcadeStages",				m_iNumArcadeStages );
	ini.SetValueB( "Options", "EventMode",						m_bEventMode );
	ini.SetValueB( "Options", "AutoPlay",						m_bAutoPlay );
	ini.SetValueF( "Options", "JudgeWindowScale",				m_fJudgeWindowScale );
	ini.SetValueF( "Options", "JudgeWindowMarvelousSeconds",	m_fJudgeWindowMarvelousSeconds );
	ini.SetValueF( "Options", "JudgeWindowPerfectSeconds",		m_fJudgeWindowPerfectSeconds );
	ini.SetValueF( "Options", "JudgeWindowGreatSeconds",		m_fJudgeWindowGreatSeconds );
	ini.SetValueF( "Options", "JudgeWindowGoodSeconds",			m_fJudgeWindowGoodSeconds );
	ini.SetValueF( "Options", "JudgeWindowBooSeconds",			m_fJudgeWindowBooSeconds );
	ini.SetValueF( "Options", "JudgeWindowOKSeconds",			m_fJudgeWindowOKSeconds );
	ini.SetValueF( "Options", "LifeDifficultyScale",			m_fLifeDifficultyScale );
	ini.SetValueI( "Options", "MovieDecodeMS",					m_iMovieDecodeMS );
	ini.SetValueB( "Options", "DelayedEscape",					m_bDelayedEscape );
	ini.SetValueB( "Options", "HiddenSongs",					m_bHiddenSongs );
	ini.SetValueB( "Options", "Vsync",							m_bVsync );
	ini.SetValueB( "Options", "Interlaced",						m_bInterlaced );
#ifdef _XBOX
	ini.SetValueB( "Options", "PAL",							m_bPAL );
#endif
	ini.SetValueB( "Options", "HowToPlay",						m_bInstructions );
	ini.SetValueB( "Options", "Caution",						m_bShowDontDie );
	ini.SetValueB( "Options", "ShowSelectGroup",				m_bShowSelectGroup );
	ini.SetValueB( "Options", "ShowNative",						m_bShowNative );
	ini.SetValueB( "Options", "ArcadeOptionsNavigation",		m_bArcadeOptionsNavigation );
	ini.SetValue ( "Options", "DWIPath",						m_DWIPath );
	ini.SetValueB( "Options", "DelayedTextureDelete",			m_bDelayedTextureDelete );
	ini.SetValueB( "Options", "DelayedScreenLoad",				m_bDelayedScreenLoad );
	ini.SetValueB( "Options", "BannerCache",					m_bBannerCache );
	ini.SetValueI( "Options", "MusicWheelUsesSections",			m_MusicWheelUsesSections );
	ini.SetValueI( "Options", "MusicWheelSwitchSpeed",			m_iMusicWheelSwitchSpeed );
	ini.SetValueB( "Options", "EasterEggs",						m_bEasterEggs );
	ini.SetValueI( "Options", "MarvelousTiming",				m_iMarvelousTiming );
	ini.SetValueB( "Options", "SoundPreloadAll",				m_bSoundPreloadAll );
	ini.SetValueI( "Options", "SoundResampleQuality",			m_iSoundResampleQuality );
	ini.SetValueI( "Options", "CoinMode",						m_iCoinMode );
	ini.SetValueI( "Options", "CoinsPerCredit",					m_iCoinsPerCredit );
	ini.SetValueB( "Options", "JointPremium",					m_bJointPremium );
	ini.SetValueI( "Options", "BoostAppPriority",				m_iBoostAppPriority );
	ini.SetValueB( "Options", "PickExtraStage",					m_bPickExtraStage );
	ini.SetValueB( "Options", "ComboContinuesBetweenSongs",		m_bComboContinuesBetweenSongs );
	ini.SetValueF( "Options", "LongVerSeconds",					m_fLongVerSongSeconds );
	ini.SetValueF( "Options", "MarathonVerSeconds",				m_fMarathonVerSongSeconds );
	ini.SetValueI( "Options", "ShowSongOptions",				m_ShowSongOptions );
	ini.SetValueB( "Options", "AllowUnacceleratedRenderer",		m_bAllowUnacceleratedRenderer );
	ini.SetValueB( "Options", "ThreadedInput",					m_bThreadedInput );
	ini.SetValue ( "Options", "IgnoredMessageWindows",			m_sIgnoredMessageWindows );
	ini.SetValueB( "Options", "SoloSingle",						m_bSoloSingle );
	ini.SetValueB( "Options", "DancePointsForOni",				m_bDancePointsForOni );
	ini.SetValueB( "Options", "ShowLyrics",						m_bShowLyrics );
	ini.SetValueB( "Options", "AutogenMissingTypes",			m_bAutogenMissingTypes );
	ini.SetValueB( "Options", "AutogenGroupCourses",			m_bAutogenGroupCourses );
	ini.SetValueB( "Options", "Timestamping",					m_bTimestamping );
	ini.SetValueB( "Options", "BreakComboToGetItem",			m_bBreakComboToGetItem );
	ini.SetValueI( "Options", "ShowDancingCharacters",			m_ShowDancingCharacters );
	ini.SetValueB( "Options", "UseUnlockSystem",				m_bUseUnlockSystem );
	ini.SetValueB( "Options", "FirstRun",						m_bFirstRun );
	ini.SetValueB( "Options", "AutoMapJoysticks",				m_bAutoMapJoysticks );
	ini.SetValue ( "Options", "VideoRenderers",					m_sVideoRenderers );
	ini.SetValue ( "Options", "LastSeenVideoDriver",			m_sLastSeenVideoDriver );
#if defined(WIN32)
	ini.SetValue ( "Options", "LastSeenMemory",					m_iLastSeenMemory );
#endif
	ini.SetValue ( "Options", "CoursesToShowRanking",			m_sCoursesToShowRanking );
	ini.SetValueB( "Options", "AntiAliasing",					m_bAntiAliasing );
	ini.SetValueF( "Options", "GlobalOffsetSeconds",			m_fGlobalOffsetSeconds );
	ini.SetValueB( "Options", "ForceLogFlush",					m_bForceLogFlush );
	ini.SetValueB( "Options", "Logging",						m_bLogging );
	ini.SetValueB( "Options", "ShowLogWindow",					m_bShowLogWindow );

	ini.SetValueB( "Options", "TenFooterInRed",					m_bTenFooterInRed );
	ini.SetValueI( "Options", "CourseSortOrder",				m_iCourseSortOrder );
	ini.SetValueB( "Options", "MoveRandomToEnd",				m_bMoveRandomToEnd );

	ini.SetValueI( "Options", "ScoringType",					m_iScoringType );

	ini.SetValueI( "Options", "ProgressiveLifebar",				m_iProgressiveLifebar );
	ini.SetValueI( "Options", "ProgressiveStageLifebar",		m_iProgressiveStageLifebar );
	ini.SetValueI( "Options", "ProgressiveNonstopLifebar",		m_iProgressiveNonstopLifebar );
	ini.SetValueB( "Options", "ShowBeginnerHelper",				m_bShowBeginnerHelper );
	ini.SetValue ( "Options", "Language",						m_sLanguage );
	ini.SetValueB( "Options", "EndlessBreakEnabled",			m_bEndlessBreakEnabled );
	ini.SetValueI( "Options", "EndlessStagesUntilBreak",		m_iEndlessNumStagesUntilBreak );
	ini.SetValueI( "Options", "EndlessBreakLength",				m_iEndlessBreakLength );

	for( int p=0; p<NUM_PLAYERS; p++ )
		ini.SetValue ( "Options", ssprintf("DefaultProfileP%d",p+1),	m_sDefaultProfile[p] );

	/* Only write these if they aren't the default.  This ensures that we can change
	 * the default and have it take effect for everyone (except people who
	 * tweaked this value). */
	if(m_sSoundDrivers != DEFAULT_SOUND_DRIVER_LIST)
		ini.SetValue ( "Options", "SoundDrivers",				m_sSoundDrivers );
	if(m_fSoundVolume != DEFAULT_SOUND_VOLUME)
		ini.SetValueF( "Options", "SoundVolume",				m_fSoundVolume );
	if(m_sMovieDrivers != DEFAULT_MOVIE_DRIVER_LIST)
		ini.SetValue ( "Options", "MovieDrivers",				m_sMovieDrivers );
	


	ini.SetValue ( "Options", "AdditionalSongFolders", 		join(",", m_asAdditionalSongFolders) );

	ini.SetValueI( "Options", "Game",				GAMESTATE->m_CurGame );

	ini.WriteFile();
}
