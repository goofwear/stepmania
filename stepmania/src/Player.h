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
#include "Sprite.h"
#include "BitmapText.h"

#include "GrayArrow.h"
#include "GhostArrow.h"
#include "GhostArrowBright.h"
#include "HoldGhostArrow.h"
#include "ActorFrame.h"
#include "RandomSample.h"
#include "Judgment.h"
#include "HoldJudgment.h"
#include "Combo.h"
#include "NoteField.h"
#include "GrayArrowRow.h"
#include "GhostArrowRow.h"
#include "NoteDataWithScoring.h"
#include "ArrowBackdrop.h"
#include "RageTimer.h"

class ScoreDisplay;
class LifeMeter;
class CombinedLifeMeter;
class ScoreKeeper;
class Inventory;

#define	SAMPLE_COUNT	16


class Player : public NoteDataWithScoring, public ActorFrame
{
public:
	Player();

	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	~Player();

	void Load( PlayerNumber player_no, NoteData* pNoteData, LifeMeter* pLM, CombinedLifeMeter* pCombinedLM, ScoreDisplay* pScore, Inventory* pInventory, ScoreKeeper* pPrimaryScoreKeeper, ScoreKeeper* pSecondaryScoreKeeper );
	void CrossedRow( int iNoteRow );
	void Step( int col, RageTimer tm );
	void RandomiseNotes( int iNoteRow );
	void FadeToFail();
	void DontShowJudgement() { m_bShowJudgment = false; }	// Used in ScreenHowToPlay
	
protected:
	void UpdateTapNotesMissedOlderThan( float fMissIfOlderThanThisBeat );
	void OnRowCompletelyJudged( int iStepIndex );
	void HandleTapRowScore( unsigned row );
	void HandleHoldScore( HoldNoteScore holdScore, TapNoteScore tapScore );
	void HandleAutosync(float fNoteOffset);

	int GetClosestNoteDirectional( int col, float fBeat, float fMaxBeatsAhead, int iDirection );
	int GetClosestNote( int col, float fBeat, float fMaxBeatsAhead, float fMaxBeatsBehind );

	static float GetMaxStepDistanceSeconds();

	PlayerNumber	m_PlayerNumber;

	float			m_fOffset[SAMPLE_COUNT];//for AutoAdjust
	int				m_iOffsetSample;		//

	ArrowBackdrop	m_ArrowBackdrop;
	GrayArrowRow	m_GrayArrowRow;
	NoteField		m_NoteField;
	GhostArrowRow	m_GhostArrowRow;

	HoldJudgment	m_HoldJudgment[MAX_NOTE_TRACKS];

	Judgment		m_Judgment;
	
	Combo			m_Combo;

	LifeMeter*		m_pLifeMeter;
	CombinedLifeMeter*		m_pCombinedLifeMeter;
	ScoreDisplay*	m_pScore;
	ScoreKeeper*	m_pPrimaryScoreKeeper;
	ScoreKeeper*	m_pSecondaryScoreKeeper;
	Inventory*		m_pInventory;

	CString			m_sLastSeenNoteSkin;
	bool			m_bShowJudgment;
	int				m_iRowLastCrossed;
};

#endif
