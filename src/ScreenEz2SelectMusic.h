#ifndef SCREENEZ2SELECTMUSIC_H
#define SCREENEZ2SELECTMUSIC_H
/*
-----------------------------------------------------------------------------
 Class: ScreenEz2SelectMusic

 Desc: A Scrolling List Of Song Banners used to select the song the player wants.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Frieza
-----------------------------------------------------------------------------
*/

#include "ScreenWithMenuElements.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "MusicBannerWheel.h"
#include "DifficultyRating.h"
#include "ModeSwitcher.h"

class ScreenEz2SelectMusic : public ScreenWithMenuElements
{
public:
	ScreenEz2SelectMusic( CString sName );
	virtual void DrawPrimitives();

	virtual void Update( float fDeltaTime );
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );
	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuLeft( PlayerNumber pn, const InputEventType type );
	virtual void MenuRight( PlayerNumber pn, const InputEventType type );
	virtual void MenuBack( PlayerNumber pn );

protected:
	void AfterNotesChange( PlayerNumber pn );
	void MusicChanged();

	void EasierDifficulty( PlayerNumber pn );
	void HarderDifficulty( PlayerNumber pn );

	void UpdateOptions( PlayerNumber pn, int nosound );

	CString sOptions;
	CStringArray asOptions;

	void TweenOffScreen();

	ModeSwitcher m_ModeSwitcher;
	Sprite  m_ChoiceListFrame;
	Sprite  m_ChoiceListHighlight;
	Sprite  m_Guide;
	Sprite	m_sprOptionsMessage;
	Sprite	m_InfoFrame;
	Sprite  m_PumpDifficultyCircle;
	Sprite	m_SpeedIcon[NUM_PLAYERS];
	Sprite	m_MirrorIcon[NUM_PLAYERS];
	Sprite	m_ShuffleIcon[NUM_PLAYERS];
	Sprite	m_HiddenIcon[NUM_PLAYERS];
	Sprite	m_VanishIcon[NUM_PLAYERS];
	Sprite				m_sprBalloon;
	BitmapText	m_PumpDifficultyRating;
	BitmapText  m_CurrentGroup;
	BitmapText  m_CurrentTitle;
	BitmapText  m_CurrentSubTitle;
	BitmapText  m_CurrentArtist;


	RageSound			m_soundOptionsChange;
	RageSound			m_soundMusicChange;
	RageSound			m_soundMusicCycle;
	RageSound			m_soundBackMusic;
	RageSound			m_soundButtonPress;

	float m_fRemainingWaitTime;
	MusicBannerWheel			m_MusicBannerWheel;
	DifficultyRating	m_DifficultyRating;
	vector<Steps*>		m_arrayNotes[NUM_PLAYERS];

	int					m_iSelection[NUM_PLAYERS];
	bool m_bGoToOptions;
	bool m_bMadeChoice;
	bool m_bTransitioning;
	bool m_bScanning;

	int i_SkipAheadOffset;
	float ScrollStartTime;
	float LastInputTime;

	int i_ErrorDetected;

	#ifdef DEBUG
		BitmapText	m_debugtext;
	#endif

	int iConfirmSelection;


	RageSound			m_soundSelect;
};


#endif
