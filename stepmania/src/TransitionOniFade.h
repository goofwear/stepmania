#pragma once
/*
-----------------------------------------------------------------------------
 Class: TransitionOniFade

 Desc: Fade to white and shows song title and artist info.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


#include "Transition.h"
#include "RageDisplay.h"
#include "RageSound.h"
#include "Sprite.h"
#include "Quad.h"
#include "TransitionFade.h"


class TransitionOniFade : public Transition
{
public:
	TransitionOniFade();
	~TransitionOniFade();

	virtual void DrawPrimitives();

	virtual void OpenWipingRight( ScreenMessage send_when_done = SM_None );
	virtual void OpenWipingLeft(  ScreenMessage send_when_done = SM_None );
	virtual void CloseWipingRight(ScreenMessage send_when_done = SM_None );
	virtual void CloseWipingLeft( ScreenMessage send_when_done = SM_None );

protected:

	void UpdateSongText();

	Quad		m_quadBackground;
	Quad		m_quadStrip;		// background for song text
	BitmapText	m_textSongInfo;
};


