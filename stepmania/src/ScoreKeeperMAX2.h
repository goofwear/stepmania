#ifndef SCOREKEEPER_MAX2_H
#define SCOREKEEPER_MAX2_H 1
/*
-----------------------------------------------------------------------------
 Class: ScoreKeeperMAX2

 Class to handle scorekeeping, MAX2-style.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
	Glenn Maynard
-----------------------------------------------------------------------------
*/

#include "ScoreKeeper.h"
#include "NoteDataWithScoring.h"
class Steps;

class ScoreKeeperMAX2: public ScoreKeeper
{
	int				m_iScoreRemainder;
	int				m_iMaxPossiblePoints;
	int				m_iTapNotesHit;	// number of notes judged so far, needed by scoring

	int				m_iNumTapsAndHolds;
	int			    m_iMaxScoreSoFar; // for nonstop scoring
	int				m_iPointBonus; // the difference to award at the end
 	int				m_iCurToastyCombo;
	bool			m_bIsLastSongInCourse;

	const vector<Steps*>& apSteps;

	void AddScore( TapNoteScore score );

	/* Configuration: */
	/* Score after each tap will be rounded to the nearest m_iRoundTo; 1 to do nothing. */
	int				m_iRoundTo;
	int				m_ComboBonusFactor[NUM_TAP_NOTE_SCORES];

public:
	ScoreKeeperMAX2( const vector<Song*>& apSongs, const vector<Steps*>& apSteps, const vector<AttackArray> &asModifiers, PlayerNumber pn);

	// before a song plays (called multiple times if course)
	void OnNextSong( int iSongInCourseIndex, const Steps* pSteps, const NoteData* pNoteData );

	void HandleTapScore( TapNoteScore score );
	void HandleTapRowScore( TapNoteScore scoreOfLastTap, int iNumTapsInRow );
	void HandleHoldScore( HoldNoteScore holdScore, TapNoteScore tapScore );

	// This must be calculated using only cached radar values so that we can 
	// do it quickly.
	static int	GetPossibleDancePoints( const RadarValues& fRadars );
	static int	GetPossibleDancePoints( const RadarValues& fOriginalRadars, const RadarValues& fPostRadars );

private:
	static int TapNoteScoreToDancePoints( TapNoteScore tns );
	static int HoldNoteScoreToDancePoints( HoldNoteScore hns );

};

#endif

