/* ScreenSelectMusic - Choose a Song and Steps. */

#ifndef SCREEN_SELECT_MUSIC_H
#define SCREEN_SELECT_MUSIC_H

#include "ScreenWithMenuElements.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "GameConstantsAndTypes.h"
#include "MusicWheel.h"
#include "Banner.h"
#include "FadingBanner.h"
#include "BPMDisplay.h"
#include "GrooveRadar.h"
#include "DifficultyIcon.h"
#include "DifficultyMeter.h"
#include "DifficultyDisplay.h"
#include "CourseContentsList.h"
#include "RageUtil_BackgroundLoader.h"
#include "ThemeMetric.h"
#include "ActorCommands.h"
#include "RageTexturePreloader.h"
#include "TimingData.h"

class ScreenSelectMusic : public ScreenWithMenuElements
{
public:
	ScreenSelectMusic();
	virtual ~ScreenSelectMusic();
	virtual void Init();
	virtual void BeginScreen();

	virtual void Update( float fDeltaTime );
	virtual void Input( const InputEventPlus &input );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );

	/* ScreenWithMenuElements override: never play music here; we do it ourself. */
	virtual void StartPlayingMusic() { }
		
	bool GetGoToOptions() const { return m_bGoToOptions; }

	//
	// Lua
	//
	virtual void PushSelf( lua_State *L );

protected:
	void TweenOffScreen();
	void TweenScoreOnAndOffAfterChangeSort();
	enum DisplayMode { DISPLAY_SONGS, DISPLAY_COURSES, DISPLAY_MODES } m_DisplayMode;
	void SwitchDisplayMode( DisplayMode dm );
	void TweenSongPartsOnScreen( bool Initial );
	void TweenSongPartsOffScreen( bool Final );
	void TweenCoursePartsOnScreen( bool Initial );
	void TweenCoursePartsOffScreen( bool Final );
	void SkipSongPartTweens();
	void SkipCoursePartTweens();
	void UpdateSelectButton();

	void ChangeDifficulty( PlayerNumber pn, int dir );

	void AfterStepsChange( const vector<PlayerNumber> &vpns );
	void AfterTrailChange( const vector<PlayerNumber> &vpns );
	void SwitchToPreferredDifficulty();
	void AfterMusicChange();

	void CheckBackgroundRequests( bool bForce );

	vector<Steps*>			m_vpSteps;
	vector<Trail*>			m_vpTrails;
	int				m_iSelection[NUM_PLAYERS];

	ThemeMetric<float> SAMPLE_MUSIC_DELAY;
	ThemeMetric<bool> SHOW_RADAR;
	ThemeMetric<bool> SHOW_COURSE_CONTENTS;
	ThemeMetric<bool> DO_ROULETTE_ON_MENU_TIMER;
	ThemeMetric<bool> ALIGN_MUSIC_BEATS;
	ThemeMetric<RString> CODES;
	ThemeMetric<RString> MUSIC_WHEEL_TYPE;
	ThemeMetric<bool> OPTIONS_MENU_AVAILABLE;
	DynamicThemeMetric<bool> SELECT_MENU_AVAILABLE;
	DynamicThemeMetric<bool> MODE_MENU_AVAILABLE;

	RString m_sSectionMusicPath;
	RString m_sSortMusicPath;
	RString m_sRouletteMusicPath;
	RString m_sRandomMusicPath;
	RString m_sCourseMusicPath;
	RString m_sFallbackCDTitlePath;

	FadingBanner			m_Banner;
	BPMDisplay			m_BPMDisplay;
	Sprite				m_sprCDTitleFront, m_sprCDTitleBack;
	Sprite				m_sprDifficultyFrame[NUM_PLAYERS];
	DifficultyIcon			m_DifficultyIcon[NUM_PLAYERS];
	Sprite				m_AutoGenIcon[NUM_PLAYERS];
	GrooveRadar			m_GrooveRadar;
	BitmapText			m_textNumSongs;
	BitmapText			m_textTotalTime;
	Sprite				m_sprMeterFrame[NUM_PLAYERS];
	DifficultyMeter			m_DifficultyMeter[NUM_PLAYERS];
	Sprite				m_sprHighScoreFrame[NUM_PLAYERS];
	BitmapText			m_textHighScore[NUM_PLAYERS];
	MusicWheel			m_MusicWheel;
	AutoActor			m_sprLongBalloon;
	AutoActor			m_sprMarathonBalloon;
	DifficultyDisplay		m_DifficultyDisplay;
	CourseContentsList		m_CourseContents;
	BitmapText			m_MachineRank;

	bool				m_bMadeChoice;
	bool				m_bGoToOptions;
	RString				m_sSampleMusicToPlay;
	TimingData			*m_pSampleMusicTimingData;
	float				m_fSampleStartSeconds, m_fSampleLengthSeconds;
	bool				m_bAllowOptionsMenu, m_bAllowOptionsMenuRepeat;
	bool				m_bSelectIsDown;

	RageSound			m_soundDifficultyEasier;
	RageSound			m_soundDifficultyHarder;
	RageSound			m_soundOptionsChange;
	RageSound			m_soundLocked;
	RageSound			m_soundSelectPressed;

	BackgroundLoader		m_BackgroundLoader;
	RageTexturePreloader		m_TexturePreload;
};

#endif

/*
 * (c) 2001-2004 Chris Danford
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
