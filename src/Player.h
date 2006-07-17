/* Player - Accepts input, knocks down TapNotes that were stepped on, and keeps score for the player. */

#ifndef PLAYER_H
#define PLAYER_H

#include "ActorFrame.h"
#include "Judgment.h"
#include "HoldJudgment.h"
#include "Combo.h"
#include "NoteDataWithScoring.h"
#include "RageSound.h"
#include "AttackDisplay.h"
#include "NoteData.h"
#include "ScreenMessage.h"

class ScoreDisplay;
class LifeMeter;
class CombinedLifeMeter;
class ScoreKeeper;
class Inventory;
class RageTimer;
class NoteField;
class PlayerStageStats;

AutoScreenMessage( SM_100Combo );
AutoScreenMessage( SM_200Combo );
AutoScreenMessage( SM_300Combo );
AutoScreenMessage( SM_400Combo );
AutoScreenMessage( SM_500Combo );
AutoScreenMessage( SM_600Combo );
AutoScreenMessage( SM_700Combo );
AutoScreenMessage( SM_800Combo );
AutoScreenMessage( SM_900Combo );
AutoScreenMessage( SM_1000Combo );
AutoScreenMessage( SM_ComboStopped );
AutoScreenMessage( SM_ComboContinuing );
AutoScreenMessage( SM_MissComboAborted );

class Player: public ActorFrame
{
public:
	// The passed in NoteData isn't touched until Load() is called.
	Player( NoteData &nd, bool bShowNoteField = true, bool bShowJudgment = true );
	~Player();

	virtual void Update( float fDeltaTime );
	virtual void ProcessMessages( float fDeltaTime );
	virtual void DrawPrimitives();

	void Init( 
		const RString &sType,
		PlayerState* pPlayerState, 
		PlayerStageStats* pPlayerStageStats,
		LifeMeter* pLM, 
		CombinedLifeMeter* pCombinedLM, 
		ScoreDisplay* pScoreDisplay, 
		ScoreDisplay* pSecondaryScoreDisplay, 
		Inventory* pInventory, 
		ScoreKeeper* pPrimaryScoreKeeper, 
		ScoreKeeper* pSecondaryScoreKeeper );
	void Load();
	void CrossedRow( int iNoteRow );
	void CrossedMineRow( int iNoteRow );
	void Step( int col, const RageTimer &tm, bool bHeld = false );
	void RandomizeNotes( int iNoteRow );
	void FadeToFail();
	void CacheAllUsedNoteSkins();
	TapNoteScore GetLastTapNoteScore() const { return m_LastTapNoteScore; }
	void ApplyWaitingTransforms();
	void SetPaused( bool bPaused ) { m_bPaused = bPaused; }

	float GetMaxStepDistanceSeconds();
	const NoteData &GetNoteData() const { return m_NoteData; }
	bool HasNoteField() const { return m_pNoteField != NULL; }

protected:
	void HandleStep( int col, const RageTimer &tm, bool bHeld );
	void UpdateTapNotesMissedOlderThan( float fMissIfOlderThanThisBeat );
	void UpdateJudgedRows();
	void DisplayJudgedRow( int iIndexThatWasSteppedOn, TapNoteScore score, int iTrack );
	void OnRowCompletelyJudged( int iStepIndex );
	void HandleTapRowScore( unsigned row );
	void HandleHoldScore( const TapNote &tn );
	void DrawTapJudgments();
	void DrawHoldJudgments();

	void SetJudgment( TapNoteScore tns, bool bEarly );

	int GetClosestNoteDirectional( int col, int iStartRow, int iMaxRowsAhead, bool bAllowGraded, bool bForward ) const;
	int GetClosestNote( int col, int iNoteRow, int iMaxRowsAhead, int iMaxRowsBehind, bool bAllowGraded ) const;

	bool IsPlayingBeginner() const;

	bool			m_bLoaded;

	PlayerState		*m_pPlayerState;
	PlayerStageStats	*m_pPlayerStageStats;
	float			m_fNoteFieldHeight;

	bool			m_bPaused;

	NoteData		&m_NoteData;
	NoteField		*m_pNoteField;

	vector<HoldJudgment*>	m_vHoldJudgment;

	Judgment		*m_pJudgment;
	AutoActor		m_sprJudgmentFrame;
		
	Combo			*m_pCombo;

	AttackDisplay		*m_pAttackDisplay;

	TapNoteScore		m_LastTapNoteScore;
	LifeMeter		*m_pLifeMeter;
	CombinedLifeMeter	*m_pCombinedLifeMeter;
	ScoreDisplay		*m_pScoreDisplay;
	ScoreDisplay		*m_pSecondaryScoreDisplay;
	ScoreKeeper		*m_pPrimaryScoreKeeper;
	ScoreKeeper		*m_pSecondaryScoreKeeper;
	Inventory		*m_pInventory;

	int			m_iRowLastCrossed;
	int			m_iMineRowLastCrossed;
	int			m_iRowLastJudged; // Everything up to and including this row has been judged.

	RageSound		m_soundMine;
	RageSound		m_soundAttackLaunch;
	RageSound		m_soundAttackEnding;

	vector<RageSound>	m_vKeysounds;

	RString			m_sMessageToSendOnStep;

	ThemeMetric<float>	GRAY_ARROWS_Y_STANDARD;
	ThemeMetric<float>	GRAY_ARROWS_Y_REVERSE;
	ThemeMetric2D<float>	COMBO_X;
	ThemeMetric<float>	COMBO_Y;
	ThemeMetric<float>	COMBO_Y_REVERSE;
	ThemeMetric<float>	COMBO_CENTERED_ADDY;
	ThemeMetric<float>	COMBO_CENTERED_ADDY_REVERSE;
	ThemeMetric2D<float>	ATTACK_DISPLAY_X;
	ThemeMetric<float>	ATTACK_DISPLAY_Y;
	ThemeMetric<float>	ATTACK_DISPLAY_Y_REVERSE;
	ThemeMetric<float>	HOLD_JUDGMENT_Y_STANDARD;
	ThemeMetric<float>	HOLD_JUDGMENT_Y_REVERSE;
	ThemeMetric<int>	BRIGHT_GHOST_COMBO_THRESHOLD;
	ThemeMetric<bool>	TAP_JUDGMENTS_UNDER_FIELD;
	ThemeMetric<bool>	HOLD_JUDGMENTS_UNDER_FIELD;
	ThemeMetric<int>	START_DRAWING_AT_PIXELS;
	ThemeMetric<int>	STOP_DRAWING_AT_PIXELS;
	
#define NUM_REVERSE 2
#define NUM_CENTERED 2
	TweenState		m_tsJudgment[NUM_REVERSE][NUM_CENTERED];
};

class PlayerPlus : public Player
{
	NoteData m_NoteData;
public:
	PlayerPlus() : Player(m_NoteData) { }
	void Load( const NoteData &nd ) { m_NoteData = nd; Player::Load(); }
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
