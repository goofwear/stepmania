/*
-----------------------------------------------------------------------------
 Class: ScreenNameEntry

 Desc: Enter you name for a new high score.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Screen.h"
#include "BitmapText.h"
#include "Transition.h"
#include "RandomSample.h"
#include "GrayArrowRow.h"
#include "BGAnimation.h"
#include "MenuTimer.h"


class ScreenNameEntry : public Screen
{
public:
	ScreenNameEntry( CString sName );
	virtual ~ScreenNameEntry();

	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	virtual void MenuStart( PlayerNumber pn );

	enum { ABS_MAX_RANKING_NAME_LENGTH = 10 };
private:
	bool AnyStillEntering() const;

	BGAnimation		m_Background;

	GrayArrowRow	m_GrayArrowRow[NUM_PLAYERS];
	BitmapText		m_textSelectedChars[NUM_PLAYERS][ABS_MAX_RANKING_NAME_LENGTH];
	BitmapText		m_textScrollingChars[NUM_PLAYERS][ABS_MAX_RANKING_NAME_LENGTH];
	BitmapText		m_textCategory[NUM_PLAYERS];
	MenuTimer		m_Timer;

	Transition	m_In;
	Transition	m_Out;

	RageSound		m_soundStep;

	float			m_fFakeBeat;
	CString			m_sSelectedName[NUM_PLAYERS];
	bool			m_bStillEnteringName[NUM_PLAYERS];

	vector<int>		m_ColToStringIndex[NUM_PLAYERS];
};



