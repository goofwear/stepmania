#pragma once
/*
-----------------------------------------------------------------------------
 Class: ScoreDisplayRolling

 Desc: A graphic displayed in the ScoreDisplayRolling during Dancing.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Sprite.h"
#include "Song.h"
#include "ActorFrame.h"
#include "BitmapText.h"


const int NUM_SCORE_DIGITS	=	9;


class ScoreDisplayRolling : public BitmapText
{
public:
	ScoreDisplayRolling();

	void SetScore( float fNewScore );
	void SetScore( int iNewScore );
	float GetScore();
	void AddToScore( TapNoteScore score, int iCurCombo );

	virtual void Update( float fDeltaTime );
	virtual void Draw();

protected:
	float m_fScore;

	int m_iCurrentScoreDigits[NUM_SCORE_DIGITS];
	int m_iDestinationScoreDigits[NUM_SCORE_DIGITS];
};
