#ifndef COURSE_H
#define COURSE_H
/*
-----------------------------------------------------------------------------
 Class: Course

 Desc: A queue of songs and notes.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "GameConstantsAndTypes.h"
#include "Style.h"	// for NUM_STYLES

struct PlayerOptions;
struct SongOptions;
class Song;
struct Notes;

class Course
{
	struct course_entry {
		CString description;	// the Notes description
		CString modifiers; // set player and song options from these
		Song *song;
	};
	vector<course_entry> entries;
	vector<int> order;

public:
	Course();

	CString		m_sPath;
	CString		m_sName;
	CString		m_sBannerPath;
	CString		m_sCDTitlePath;

	bool		m_bRepeat;	// repeat after last song?
	bool		m_bRandomize;	// play the songs in a random order
	int			m_iLives;	// -1 means use bar life meter
	int			m_iExtra;	// extra stage number...

	Notes *GetNotesForStage( int iStage );
	Song *GetSong( int iStage ) const;
	CString GetDescription( int iStage ) const;
//	CString GetModifiers( int iStage ) const;	// redundant.  -Chris
	void GetPlayerOptions( int iStage, PlayerOptions* pPO_out ) const;
	void GetSongOptions( SongOptions* pSO_out) const;
	int GetNumStages() const;

	void LoadFromCRSFile( CString sPath, vector<Song*> &apSongs );
	void CreateEndlessCourseFromGroupAndDifficulty( CString sGroupName, Difficulty dc, vector<Song*> &apSongsInGroup );
	void AddStage( Song* pSong, CString sDescription, CString sModifiers );

	void GetSongAndNotesForCurrentStyle( vector<Song*>& apSongsOut, vector<Notes*>& apNotesOut, CStringArray& asModifiersOut, bool bShuffled );
	RageColor	GetColor();

	// Statistics
	int m_iNumTimesPlayed;

	struct MachineScore
	{
		int iDancePoints;
		float fSurviveTime;
		CString	sName;
	} m_MachineScores[NUM_STYLES][NUM_DIFFICULTIES][NUM_HIGH_SCORE_LINES];	// sorted highest to lowest by iDancePoints

	bool IsNewMachineRecord( PlayerNumber pn, int iDancePoints, float fSurviveTime ) const;	// return true if this is would be a new machine record
	void AddMachineRecord( int iDancePoints[NUM_PLAYERS], float fSurviveTime[NUM_PLAYERS], int iNewRecordIndexOut[NUM_PLAYERS] );	// set iNewRecordIndex = -1 if not a new record

	struct MemCardScore
	{
		int iDancePoints;
		float fSurviveTime;
	} m_MemCardScores[NUM_STYLES][NUM_DIFFICULTIES][NUM_PLAYERS];

	bool AddMemCardRecord( PlayerNumber pn, int iDancePoints, float fSurviveTime );	// return true if this is a new record


private:
	void Shuffle();
};


void SortCoursePointerArrayByDifficulty( vector<Course*> &apCourses );


#endif
