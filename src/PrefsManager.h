/* PrefsManager - Holds user-chosen preferences that are saved between sessions. */

#ifndef PREFSMANAGER_H
#define PREFSMANAGER_H

#include "PlayerNumber.h"
#include "GameConstantsAndTypes.h"
#include "Preference.h"
class IniFile;

const int MAX_SONGS_PER_PLAY = 7;

class PrefsManager
{
public:
	PrefsManager();
	~PrefsManager();

	void Init();

	void SetCurrentGame( const RString &sGame );
	RString	GetCurrentGame() { return m_sCurrentGame; }
protected:
	Preference<RString>	m_sCurrentGame;

public:
	// Game-specific prefs.  Copy these off and save them every time the game changes
	Preference<RString>	m_sAnnouncer;
	Preference<RString>	m_sTheme;
	Preference<RString>	m_sDefaultModifiers;
protected:
	void StoreGamePrefs();
	void RestoreGamePrefs();
	struct GamePrefs
	{
		RString	m_sAnnouncer;
		RString m_sTheme;
		RString	m_sDefaultModifiers;
	};
	map<RString, GamePrefs> m_mapGameNameToGamePrefs;

public:
	Preference<bool>	m_bWindowed;
	Preference<int>		m_iDisplayWidth;
	Preference<int>		m_iDisplayHeight;
	Preference<float>	m_fDisplayAspectRatio;
	Preference<int>		m_iDisplayColorDepth;
	Preference<int>		m_iTextureColorDepth;
	Preference<int>		m_iMovieColorDepth;
	Preference<int>		m_iMaxTextureResolution;
	Preference<int>		m_iRefreshRate;
	Preference<bool>	m_bAllowMultitexture;
	Preference<bool>	m_bShowStats;
	Preference<bool>	m_bShowBanners;

	Preference<bool>	m_bSongBackgrounds;
	enum RandomBackgroundMode { BGMODE_OFF, BGMODE_ANIMATIONS, BGMODE_RANDOMMOVIES, NUM_RandomBackgroundMode };
	Preference<RandomBackgroundMode,int>		m_RandomBackgroundMode;
	Preference<int>		m_iNumBackgrounds;
	Preference<float>	m_fBGBrightness;
	Preference<bool>	m_bHiddenSongs;
	Preference<bool>	m_bVsync;
	Preference<bool>	m_bInterlaced;
	Preference<bool>	m_bPAL;
	Preference<bool>	m_bDelayedTextureDelete;
	Preference<bool>	m_bDelayedModelDelete;
	enum BannerCache {
		BNCACHE_OFF,
		BNCACHE_LOW_RES_PRELOAD, // preload low-res on start
		BNCACHE_LOW_RES_LOAD_ON_DEMAND, // preload low-res on screen load
		BNCACHE_FULL };
	Preference<BannerCache,int>		m_BannerCache;
	Preference<bool>	m_bPalettedBannerCache;
	Preference<bool>	m_bFastLoad;
	Preference<bool>        m_bFastLoadAdditionalSongs;

	Preference<bool>	m_bOnlyDedicatedMenuButtons;
	Preference<bool>	m_bMenuTimer;
	Preference<bool>	m_bShowDanger;

	Preference<float>	m_fTimingWindowScale;
	Preference<float>	m_fTimingWindowAdd;		// this is useful for compensating for changes in sampling rate between devices
	
	Preference1D<float>	m_fTimingWindowSeconds;

	Preference<float>	m_fLifeDifficultyScale;

	// Whoever added these: Please add a comment saying what they do. -Chris
	Preference<int>		m_iRegenComboAfterFail;
	Preference<int>		m_iRegenComboAfterMiss;
	Preference<int>		m_iMaxRegenComboAfterFail;
	Preference<int>		m_iMaxRegenComboAfterMiss;
	Preference<bool>	m_bMercifulDrain;	// negative life deltas are scaled by the players life percentage
	Preference<bool>	m_bMinimum1FullSongInCourses;	// FEoS for 1st song, FailImmediate thereafter
	Preference<bool>	m_bFailOffInBeginner;
	Preference<bool>	m_bFailOffForFirstStageEasy;
	Preference<bool>	m_bMercifulBeginner;	// don't subtract from percent score or grade DP, larger W5 window

	// percent score (the number that is shown on the screen and saved to memory card)
	Preference1D<int>		m_iPercentScoreWeight;

	// grades are calculated based on a percentage, but might have different weights than the percent score
	Preference1D<int>		m_iGradeWeight;

	// super meter used in rave
	Preference1D<float>	m_fSuperMeterPercentChange;
	Preference<bool>	m_bMercifulSuperMeter;	// negative super deltas are scaled by the players life percentage

	// time meter used in survival
	Preference1D<float>	m_fTimeMeterSecondsChange;

	Preference<PlayerController,int> m_AutoPlay;
	Preference<bool>	m_bDelayedBack;
	Preference<bool>	m_bShowInstructions;
	Preference<bool>	m_bShowSelectGroup;
	Preference<bool>	m_bShowCaution;
	Preference<bool>	m_bShowNativeLanguage;
	Preference<bool>	m_bArcadeOptionsNavigation;
	enum MusicWheelUsesSections { NEVER, ALWAYS, ABC_ONLY };
	Preference<MusicWheelUsesSections,int>		m_MusicWheelUsesSections;
	Preference<int>		m_iMusicWheelSwitchSpeed;
	enum AllowW1 { ALLOW_W1_NEVER, ALLOW_W1_COURSES_ONLY, ALLOW_W1_EVERYWHERE };
	Preference<AllowW1,int>		m_AllowW1;
	Preference<bool>	m_bEventMode;
	Preference<int>		m_iCoinsPerCredit;
	Preference<int>		m_iSongsPerPlay;

	// These options have weird interactions depending on m_bEventMode, 
	// so wrap them in GameState.
	Preference<CoinMode,int>	m_CoinMode;
	Preference<Premium,int>		m_Premium;

	Preference<bool>	m_bDelayedCreditsReconcile;
	Preference<bool>	m_bPickExtraStage;
	Preference<bool>	m_bComboContinuesBetweenSongs;
	Preference<float>	m_fLongVerSongSeconds;
	Preference<float>	m_fMarathonVerSongSeconds;
	enum Maybe { ASK = -1, NO = 0, YES = 1 };
	Preference<Maybe,int>		m_ShowSongOptions;
	Preference<bool>	m_bDancePointsForOni;
	Preference<bool>	m_bPercentageScoring;
	Preference<float>	m_fMinPercentageForMachineSongHighScore;
	Preference<float>	m_fMinPercentageForMachineCourseHighScore;
	Preference<bool>	m_bDisqualification;
	Preference<bool>	m_bAutogenSteps;
	Preference<bool>	m_bAutogenGroupCourses;
	Preference<bool>	m_bBreakComboToGetItem;
	Preference<bool>	m_bLockCourseDifficulties;
	enum ShowDancingCharacters { SDC_Off = 0, SDC_Random = 1, SDC_Select = 2};
	Preference<ShowDancingCharacters,int>		m_ShowDancingCharacters;
	Preference<bool>	m_bUseUnlockSystem;
	Preference<float>	m_fGlobalOffsetSeconds;
	Preference<int>		m_iProgressiveLifebar;
	Preference<int>		m_iProgressiveStageLifebar;
	Preference<int>		m_iProgressiveNonstopLifebar;
	Preference<bool>	m_bShowBeginnerHelper;
	Preference<bool>	m_bDisableScreenSaver;
	Preference<RString>	m_sLanguage;
	Preference<RString>	m_sMemoryCardProfileSubdir;	// the directory on a memory card to look in for a profile
	Preference<int>		m_iProductID;		// Saved in HighScore to track what software version a score came from.
	Preference<int>		m_iCenterImageTranslateX;
	Preference<int>		m_iCenterImageTranslateY;
	Preference<int>		m_fCenterImageAddWidth;
	Preference<int>		m_fCenterImageAddHeight;
	enum AttractSoundFrequency { ASF_NEVER, ASF_EVERY_TIME, ASF_EVERY_2_TIMES, ASF_EVERY_3_TIMES, ASF_EVERY_4_TIMES, ASF_EVERY_5_TIMES };
	Preference<AttractSoundFrequency,int>	m_AttractSoundFrequency;
	Preference<bool>	m_bAllowExtraStage;
	Preference<bool>	m_bHideDefaultNoteSkin;
	Preference<int>		m_iMaxHighScoresPerListForMachine;
	Preference<int>		m_iMaxHighScoresPerListForPlayer;
	Preference<int>		m_iMaxRecentScoresForMachine;
	Preference<int>		m_iMaxRecentScoresForPlayer;
	Preference<bool>	m_bAllowMultipleHighScoreWithSameName;
	Preference<bool>	m_bCelShadeModels;
	Preference<bool>	m_bPreferredSortUsesGroups;

	// Number of seconds it takes for a button on the controller to release
	// after pressed.
	Preference<float>	m_fPadStickSeconds;

	// Useful for non 4:3 displays and resolutions < 640x480 where texels don't
	// map directly to pixels.
	Preference<bool>	m_bForceMipMaps;
	Preference<bool>	m_bTrilinearFiltering;		// has no effect without mipmaps on
	Preference<bool>	m_bAnisotropicFiltering;	// has no effect without mipmaps on.  Not mutually exclusive with trilinear.

	// If true, then signatures created when writing profile data 
	// and verified when reading profile data.  Leave this false if 
	// you want to use a profile on different machines that don't 
	// have the same key, or else the profile's data will be discarded.
	Preference<bool>	m_bSignProfileData;
	
	// course ranking
	enum CourseSortOrders { COURSE_SORT_PREFERRED, COURSE_SORT_SONGS, COURSE_SORT_METER, COURSE_SORT_METER_SUM, COURSE_SORT_RANK, NUM_COURSE_SORT_ORDER };
	Preference<CourseSortOrders,int>	m_CourseSortOrder;
	Preference<bool>	m_bSubSortByNumSteps;	
	enum GetRankingName { RANKING_OFF, RANKING_ON, RANKING_LIST };
	Preference<GetRankingName,int>	m_GetRankingName;

	enum ScoringType { SCORING_NEW, SCORING_OLD };
	Preference<ScoringType,int>	m_ScoringType;

	Preference<RString>	m_sAdditionalSongFolders;
	Preference<RString>	m_sAdditionalFolders;

	Preference<RString>	m_sLastSeenVideoDriver;
	Preference<RString>	m_sVideoRenderers;		// StepMania.cpp sets these on first run based on the card
	Preference<bool>	m_bSmoothLines;
	Preference<float>	m_fSoundVolume;
	Preference<int>		m_iSoundWriteAhead;
	Preference<RString>	m_iSoundDevice;	
	Preference<int>		m_iSoundPreferredSampleRate;
	Preference<RString>	m_sLightsStepsDifficulty;
	Preference<bool>	m_bAllowUnacceleratedRenderer;
	Preference<bool>	m_bThreadedInput;
	Preference<bool>	m_bThreadedMovieDecode;
	Preference<bool>	m_bScreenTestMode;
	Preference<bool>	m_bDebugLights;
	Preference<bool>	m_bMonkeyInput;
	Preference<RString>	m_sMachineName;
	Preference<RString>	m_sCoursesToShowRanking;

	/* Debug: */
	Preference<bool>	m_bLogToDisk;
	Preference<bool>	m_bForceLogFlush;
	Preference<bool>	m_bShowLogOutput;
	Preference<bool>	m_bLogSkips;
	Preference<bool>	m_bLogCheckpoints;
	Preference<bool>	m_bShowLoadingWindow;
	Preference<bool>	m_bPseudoLocalize;

#if !defined(WITHOUT_NETWORKING)
	Preference<bool>	m_bEnableScoreboard;  //Alows disabling of scoreboard in network play
#endif


	// wrappers
	float GetSoundVolume();


	void ReadPrefsFromIni( const IniFile &ini, const RString &sSection, bool bIsStatic );
	void ReadGamePrefsFromIni( const RString &sIni );
	void ReadDefaultsFromIni( const IniFile &ini, const RString &sSection );
	void SavePrefsToIni( IniFile &ini );

	void ReadPrefsFromDisk();
	void SavePrefsToDisk();

	void ResetToFactoryDefaults();

	RString GetPreferencesSection() const;

	// Lua
	void PushSelf( lua_State *L );

protected:
	void ReadPrefsFromFile( const RString &sIni, const RString &sSection, bool bIsStatic );
	void ReadDefaultsFromFile( const RString &sIni, const RString &sSection );
};




/* This is global, because it can be accessed by crash handlers and error handlers
 * that are run after PREFSMAN shuts down (and probably don't want to deref that
 * pointer anyway). */
extern bool			g_bAutoRestart;

extern PrefsManager*	PREFSMAN;	// global and accessable from anywhere in our program

#endif

/*
 * (c) 2001-2004 Chris Danford, Chris Gomez
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
