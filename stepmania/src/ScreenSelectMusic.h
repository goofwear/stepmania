/*
-----------------------------------------------------------------------------
 Class: ScreenSelectMusic

 Desc: The screen in PLAY_MODE_ARCADE where you choose a Song and Notes.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Screen.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "GameConstantsAndTypes.h"
#include "MusicWheel.h"
#include "Banner.h"
#include "FadingBanner.h"
#include "BPMDisplay.h"
#include "MenuElements.h"
#include "GrooveRadar.h"
#include "GrooveGraph.h"
#include "DifficultyIcon.h"
#include "DifficultyMeter.h"
#include "OptionIconRow.h"
#include "DifficultyDisplay.h"
#include "CourseContentsList.h"


class ScreenSelectMusic : public Screen
{
public:
	ScreenSelectMusic();
	virtual ~ScreenSelectMusic();

	virtual void DrawPrimitives();

	virtual void Update( float fDeltaTime );
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );

protected:
	void TweenOnScreen();
	void TweenOffScreen();
	void TweenScoreOnAndOffAfterChangeSort();
	void EnterCourseDisplayMode();
	void ExitCourseDisplayMode();
	bool m_bInCourseDisplayMode;
	void TweenSongPartsOnScreen( bool Initial );
	void TweenSongPartsOffScreen();
	void TweenCoursePartsOnScreen( bool Initial );
	void TweenCoursePartsOffScreen();
	void SkipSongPartTweens();
	void SkipCoursePartTweens();

	void EasierDifficulty( PlayerNumber pn );
	void HarderDifficulty( PlayerNumber pn );

	void AfterNotesChange( PlayerNumber pn );
	void SwitchToPreferredDifficulty();
	void AfterMusicChange();
	void SortOrderChanged();

	void UpdateOptionsDisplays();
	void AdjustOptions();

	vector<Notes*> m_arrayNotes[NUM_PLAYERS];
	int					m_iSelection[NUM_PLAYERS];

	MenuElements		m_Menu;

	Sprite				m_sprBannerMask;
	FadingBanner		m_Banner;
	Sprite				m_sprBannerFrame;
	BPMDisplay			m_BPMDisplay;
	Sprite				m_sprStage;
	Sprite				m_sprCDTitleFront, m_sprCDTitleBack;
	Sprite				m_sprDifficultyFrame[NUM_PLAYERS];
	DifficultyIcon		m_DifficultyIcon[NUM_PLAYERS];
	Sprite				m_AutoGenIcon[NUM_PLAYERS];
    GrooveRadar			m_GrooveRadar;
    GrooveGraph			m_GrooveGraph;
	BitmapText			m_textSongOptions;
	OptionIconRow		m_OptionIconRow[NUM_PLAYERS];
	Sprite				m_sprMeterFrame[NUM_PLAYERS];
	DifficultyMeter			m_DifficultyMeter[NUM_PLAYERS];
	MusicSortDisplay	m_MusicSortDisplay;
	Sprite				m_sprHighScoreFrame[NUM_PLAYERS];
	BitmapText			m_textHighScore[NUM_PLAYERS];
	MusicWheel			m_MusicWheel;
	Sprite				m_sprBalloon;
	DifficultyDisplay   m_DifficultyDisplay;
	CourseContentsList	m_CourseContentsFrame;

	bool				m_bMadeChoice;
	bool				m_bGoToOptions;
	Sprite				m_sprOptionsMessage;
	float				m_fPlaySampleCountdown;
	CString				m_sSampleMusicToPlay;
	float				m_fSampleStartSeconds, m_fSampleLengthSeconds;
	bool				m_bAllowOptionsMenu, m_bAllowOptionsMenuRepeat;

	RageSound			m_soundSelect;
	RageSound			m_soundChangeNotes;
	RageSound			m_soundOptionsChange;
	RageSound			m_soundLocked;
};


