#ifndef PLAYER_H
#define PLAYER_H
/*
-----------------------------------------------------------------------------
 Class: Player

 Desc: Object that accepts pad input, knocks down ColorNotes that were stepped on, 
		and keeps score for the player.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "PrefsManager.h"	// for GameplayStatistics
#include "Notes.h"
#include "Sprite.h"
#include "BitmapText.h"

#include "GrayArrow.h"
#include "GhostArrow.h"
#include "GhostArrowBright.h"
#include "HoldGhostArrow.h"
#include "ActorFrame.h"
#include "RandomSample.h"
#include "ScoreDisplay.h"
#include "LifeMeterBar.h"
#include "Judgement.h"
#include "HoldJudgement.h"
#include "Combo.h"
#include "NoteField.h"
#include "GrayArrowRow.h"
#include "GhostArrowRow.h"
#include "NoteDataWithScoring.h"
#include "ScoreKeeper.h"

#define	SAMPLE_COUNT	16

class Player : public NoteDataWithScoring, public ActorFrame
{
public:
	Player();

	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	~Player();

	void Load( PlayerNumber player_no, NoteData* pNoteData, LifeMeter* pLM, ScoreDisplay* pScore );
	void CrossedRow( int iNoteRow );
	void Step( int col );
	int GetPlayersMaxCombo();

	void	FadeToFail();
	
protected:
	int UpdateTapNotesMissedOlderThan( float fMissIfOlderThanThisBeat );
	void OnRowDestroyed( int iStepIndex );
	void HandleNoteScore( TapNoteScore score, int iNumTapsInRow );
	void HandleHoldNoteScore( HoldNoteScore score, TapNoteScore TapNoteScore );

	int GetClosestBeatDirectional( int col, float fBeat, float fMaxBeatsAhead, int iDirection );
	int GetClosestBeat( int col, float fBeat, float fMaxBeatsAhead, float fMaxBeatsBehind );

	static float GetMaxBeatDifference();

	PlayerNumber	m_PlayerNumber;

	float			m_fOffset[SAMPLE_COUNT];//for AutoAdjust
	int				m_iOffsetSample;		//

	GrayArrowRow	m_GrayArrowRow;
	NoteField		m_NoteField;
	GhostArrowRow	m_GhostArrowRow;

	HoldJudgement	m_HoldJudgement[MAX_NOTE_TRACKS];

	ActorFrame		m_frameJudgement;
	Judgement		m_Judgement;
	
	ActorFrame		m_frameCombo;
	Combo			m_Combo;

	LifeMeter*		m_pLifeMeter;
	ScoreDisplay*	m_pScore;
	ScoreKeeper*	m_ScoreKeeper;
};

#endif
