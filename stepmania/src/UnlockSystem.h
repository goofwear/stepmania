#ifndef UNLOCK_SYSTEM_H
#define UNLOCK_SYSTEM_H

#include "grade.h"
/*
-----------------------------------------------------------------------------
 Class: UnlockSystem

 Desc: The unlock system for Stepmania.

 Copyright (c) 2001-2003 by the person(s) listed below.  All rights reserved.
	Kevin Slaughter
	curewater
-----------------------------------------------------------------------------
*/
enum UnlockTypes { UNLOCK_AP, UNLOCK_DP, UNLOCK_SP,
				UNLOCK_EC, UNLOCK_EF, UNLOCK_SC,
				UNLOCK_TT, UNLOCK_RO};

struct SongEntry
{
	CString m_sSongName;	/* Name of the song in the DWI/SM file itself.. This allows
								for a lot easier compatibility since a lot of people's 
								song folders are named differantly, song names tend to
								be the same in the file.*/
	
	float	m_fDancePointsRequired;	// Ways to unlock/lock songs.
	float	m_fArcadePointsRequired;
	float	m_fSongPointsRequired;
	float	m_fExtraStagesCleared;
	float	m_fExtraStagesFailed;
	float	m_fStagesCleared;
	float	m_fToastysSeen;
	int		m_iRouletteSeed;

	bool	isLocked;    // cached locked tag

	SongEntry();

	// if song is selectable vai two means
	bool	SelectableWheel();
	bool	SelectableRoulette();

	bool	updateLocked();  //updates isLocked flag

};

class UnlockSystem
{
public:
	UnlockSystem();
	float NumPointsUntilNextUnlock();
	bool SongIsLocked( CString sSongName );
	bool SongIsRoulette( CString sSongName );
	bool LoadFromDATFile( CString sPath );
	vector<SongEntry>	m_SongEntries;	// All locked songs are stored here

	// functions that add to values, which don't really work.
	float UnlockAddAP(Grade credit);
	float UnlockAddDP(float credit);
	float UnlockAddSP(Grade credit);
	float UnlockClearExtraStage();
	float UnlockFailExtraStage();
	float UnlockClearStage();
	float UnlockToasty();
	bool UnlockRouletteSeed(int seed);

private:
	void SortSongEntriesArray();  // sorts unlocks
	bool ParseRow(CString text, CString &type, float &qty, CString &songname);
};


#endif
