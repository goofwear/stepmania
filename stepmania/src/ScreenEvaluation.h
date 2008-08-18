/* ScreenEvaluation - Shows the user their score after gameplay has ended. */

#ifndef SCREEN_EVALUATION_H
#define SCREEN_EVALUATION_H

#include "ScreenWithMenuElements.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "GradeDisplay.h"
#include "Banner.h"
#include "PercentageDisplay.h"
#include "ActorUtil.h"
#include "RageSound.h"
#include "ThemeMetric.h"
#include "RollingNumbers.h"

const int MAX_SONGS_TO_SHOW = 5;	// In summary, we show last 3 stages, plus extra stages if passed
enum JudgmentLine
{
	JudgmentLine_W1, 
	JudgmentLine_W2, 
	JudgmentLine_W3, 
	JudgmentLine_W4, 
	JudgmentLine_W5, 
	JudgmentLine_Miss, 
	JudgmentLine_Held, 
	JudgmentLine_MaxCombo, 
	NUM_JudgmentLine,
	JudgmentLine_Invalid
};
enum DetailLine
{
	DetailLine_NumSteps,
	DetailLine_Jumps, 
	DetailLine_Holds, 
	DetailLine_Mines, 
	DetailLine_Hands, 
	DetailLine_Rolls, 
	NUM_DetailLine 
};

class ScreenEvaluation : public ScreenWithMenuElements
{
public:
	virtual void Init();
	virtual void Input( const InputEventPlus &input );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	virtual void MenuBack( const InputEventPlus &input );
	virtual void MenuStart( const InputEventPlus &input );
	virtual void PushSelf( lua_State *L );
	StageStats *GetStageStats() { return m_pStageStats; }

protected:
	virtual bool GenericTweenOn() const { return true; }
	virtual bool GenericTweenOff() const { return true; }

	void HandleMenuStart();

	bool			m_bSummary;
	StageStats		*m_pStageStats;
	StageStats		m_FinalEvalStageStats;

	// banner area
	Banner			m_LargeBanner;
	AutoActor		m_sprLargeBannerFrame;
	BitmapText		m_textPlayerOptions[NUM_PLAYERS];
	AutoActor		m_sprDisqualified[NUM_PLAYERS];
	Banner			m_SmallBanner[MAX_SONGS_TO_SHOW];
	AutoActor		m_sprSmallBannerFrame[MAX_SONGS_TO_SHOW];

	// grade area
	AutoActor		m_sprGradeFrame[NUM_PLAYERS];
	GradeDisplay		m_Grades[NUM_PLAYERS];

	// points area
	bool			m_bNewSongsUnlocked;
	PercentageDisplay	m_Percent[NUM_PLAYERS];
	AutoActor		m_sprPercentFrame[NUM_PLAYERS];

	// bonus area
	AutoActor		m_sprBonusFrame[NUM_PLAYERS];
	Sprite			m_sprPossibleBar[NUM_PLAYERS][NUM_RadarCategory];
	Sprite			m_sprActualBar[NUM_PLAYERS][NUM_RadarCategory];

	// survived area
	AutoActor		m_sprSurvivedFrame[NUM_PLAYERS];
	BitmapText		m_textSurvivedNumber[NUM_PLAYERS];

	// win area
	AutoActor		m_sprWinFrame[NUM_PLAYERS];
	Sprite			m_sprWin[NUM_PLAYERS];

	// judgment area
	AutoActor		m_sprSharedJudgmentLineLabels[NUM_JudgmentLine];
	RollingNumbers		m_textJudgmentLineNumber[NUM_JudgmentLine][NUM_PLAYERS];

	// stats area
	AutoActor		m_sprDetailFrame[NUM_PLAYERS];
	BitmapText		m_textDetailText[NUM_DetailLine][NUM_PLAYERS];

	// score area
	AutoActor		m_sprScoreLabel;
	RollingNumbers		m_textScore[NUM_PLAYERS];

	// time area
	AutoActor		m_sprTimeLabel;
	BitmapText		m_textTime[NUM_PLAYERS];

	// extra area
	AutoActor		m_sprTryExtraStage;
	bool			m_bFailed;

	RageSound		m_soundStart;	// sound played if the player passes or fails

	ThemeMetric<bool>	SUMMARY;

	bool			m_bSavedScreenshot[NUM_PLAYERS];
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
