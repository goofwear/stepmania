/* ScoreKeeperNormal -  */

#ifndef ScoreKeeperNormal_H
#define ScoreKeeperNormal_H

#include "ScoreKeeper.h"
#include "Attack.h"
#include "ScreenMessage.h"
class Steps;
class Song;
struct RadarValues;

AutoScreenMessage( SM_PlayToasty );

class ScoreKeeperNormal: public ScoreKeeper
{
	int		m_iScoreRemainder;
	int		m_iMaxPossiblePoints;
	int		m_iTapNotesHit;	// number of notes judged so far, needed by scoring

	int		m_iNumTapsAndHolds;
	int		m_iMaxScoreSoFar; // for nonstop scoring
	int		m_iPointBonus; // the difference to award at the end
 	int		m_iCurToastyCombo;
	bool		m_bIsLastSongInCourse;
	bool		m_bIsBeginner;

	bool		m_bComboIsPerRow;
	TapNoteScore	m_MinScoreToContinueCombo;
	TapNoteScore	m_MinScoreToMaintainCombo;

	vector<Steps*>	m_apSteps;

	void AddScore( TapNoteScore score );

	/* Configuration: */
	/* Score after each tap will be rounded to the nearest m_iRoundTo; 1 to do nothing. */
	int		m_iRoundTo;
	int		m_ComboBonusFactor[NUM_TapNoteScore];

public:
	ScoreKeeperNormal( 
		PlayerState* pPlayerState,
		PlayerStageStats* pPlayerStageStats );
	virtual ~ScoreKeeperNormal() { }

	void Load(
		const vector<Song*>& apSongs,
		const vector<Steps*>& apSteps,
		const vector<AttackArray> &asModifiers );

	// before a song plays (called multiple times if course)
	void OnNextSong( int iSongInCourseIndex, const Steps* pSteps, const NoteData* pNoteData );

	void HandleTapScore( const TapNote &tn );
	void HandleTapRowScore( const NoteData &nd, int iRow );
	void HandleHoldScore( const TapNote &tn );

	// This must be calculated using only cached radar values so that we can 
	// do it quickly.
	static int GetPossibleDancePoints( const RadarValues& fRadars );
	static int GetPossibleDancePoints( const RadarValues& fOriginalRadars, const RadarValues& fPostRadars );
	static int GetPossibleGradePoints( const RadarValues& fRadars );
	static int GetPossibleGradePoints( const RadarValues& fOriginalRadars, const RadarValues& fPostRadars );

	int TapNoteScoreToDancePoints( TapNoteScore tns ) const;
	int HoldNoteScoreToDancePoints( HoldNoteScore hns ) const;
	int TapNoteScoreToGradePoints( TapNoteScore tns ) const;
	int HoldNoteScoreToGradePoints( HoldNoteScore hns ) const;
	static int TapNoteScoreToDancePoints( TapNoteScore tns, bool bBeginner );
	static int HoldNoteScoreToDancePoints( HoldNoteScore hns, bool bBeginner );
	static int TapNoteScoreToGradePoints( TapNoteScore tns, bool bBeginner );
	static int HoldNoteScoreToGradePoints( HoldNoteScore hns, bool bBeginner );
};

#endif

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
