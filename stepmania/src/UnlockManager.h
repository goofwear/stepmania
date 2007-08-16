/* UnlockManager - Unlocks handling. */

#ifndef UNLOCK_MANAGER_H
#define UNLOCK_MANAGER_H

#include "Grade.h"
#include "Command.h"
#include <set>
#include "Difficulty.h"
#include "SongUtil.h"
#include "CourseUtil.h"

class Song;
class Course;
class Steps;
class Profile;
struct lua_State;

enum UnlockRequirement
{
	UnlockRequirement_ArcadePoints,
	UnlockRequirement_DancePoints,
	UnlockRequirement_SongPoints,
	UnlockRequirement_ExtraCleared,
	UnlockRequirement_ExtraFailed,
	UnlockRequirement_Toasties,
	UnlockRequirement_StagesCleared,
	NUM_UnlockRequirement,
	UnlockRequirement_Invalid,
};
LuaDeclareType( UnlockRequirement );


enum UnlockRewardType {
	UnlockRewardType_Song, 
	UnlockRewardType_Steps, 
	UnlockRewardType_Course, 
	UnlockRewardType_Modifier, 
	NUM_UnlockRewardType, 
	UnlockRewardType_Invalid
};
const RString& UnlockRewardTypeToString( UnlockRewardType i );
const RString& UnlockRewardTypeToLocalizedString( UnlockRewardType i );
LuaDeclareType( UnlockRewardType );

enum UnlockEntryStatus {
	UnlockEntryStatus_RequrementsNotMet, 
	UnlockEntryStatus_RequirementsMet, 
	UnlockEntryStatus_Unlocked, 
};

class UnlockEntry
{
public:
	UnlockEntry()
	{
		m_Type = UnlockRewardType_Invalid;

		m_dc = Difficulty_Invalid;

		ZERO( m_fRequirement );
		m_bRequirePassHardSteps = false;
		m_bRoulette = false;
		m_sEntryID = "";	// "" means not yet filled.  This will be filled in automatically if not specified.
	}

	UnlockRewardType m_Type;
	Command m_cmd;

	/* A cached pointer to the song or course this entry refers to.  Only one of
	 * these will be non-NULL. */
	SongID	m_Song;
	Difficulty m_dc;
	CourseID m_Course;

	float	m_fRequirement[NUM_UnlockRequirement];	// unlocked if any of of these are met
	bool	m_bRequirePassHardSteps;
	bool	m_bRoulette;
	RString	m_sEntryID;

	bool	IsValid() const;
	bool	IsLocked() const	{ return GetUnlockEntryStatus() != UnlockEntryStatus_Unlocked; }
	UnlockEntryStatus GetUnlockEntryStatus() const;
	RString	GetModifier() const { return m_cmd.GetArg(1).s; }
	RString	GetDescription() const;
	RString	GetBannerFile() const;
	RString	GetBackgroundFile() const;

	// Lua
	void PushSelf( lua_State *L );
};

// Option is locked due to an unsatisfied unlock entry.
#define LOCKED_LOCK          0x1

// Option is only available in random selections.
#define LOCKED_ROULETTE      0x2

// Option is locked due to a #SELECTABLE tag.
#define LOCKED_SELECTABLE    0x4

// Option is disabled by the operator.  (For courses, this means that a song in the
// course is disabled.)
#define LOCKED_DISABLED      0x8

class UnlockManager
{
	friend class UnlockEntry;

public:
	UnlockManager();
	~UnlockManager();
	void Reload();

	float PointsUntilNextUnlock( UnlockRequirement t ) const;
	int SongIsLocked( const Song *pSong ) const;
	bool StepsIsLocked( const Song *pSong, const Steps *pSteps ) const;
	int CourseIsLocked( const Course *course ) const;
	bool ModifierIsLocked( const RString &sOneMod ) const;

	// Gets number of unlocks for title screen
	int GetNumUnlocks() const;
	int GetNumUnlocked() const;
	int GetUnlockEntryIndexToCelebrate() const;
	bool AnyUnlocksToCelebrate() const;

	void GetPoints( const Profile *pProfile, float fScores[NUM_UnlockRequirement] ) const;

	// Unlock an entry by code.
	void UnlockEntryID( RString sEntryID );
	void UnlockEntryIndex( int iEntryIndex );

	/*
	 * If a code is associated with at least one song or course, set the preferred song
	 * and/or course in GAMESTATE to them.
	 */
	void PreferUnlockEntryID( RString sEntryID );

	// Unlocks a song.
	void UnlockSong( const Song *pSong );

	// Return the associated EntryID.
	RString FindEntryID( const RString &sName ) const;

	// All locked songs are stored here
	vector<UnlockEntry>	m_UnlockEntries;

	void GetUnlocksByType( UnlockRewardType t, vector<UnlockEntry *> &apEntries );
	void GetSongsUnlockedByEntryID( vector<Song *> &apSongsOut, RString sEntryID );
	void GetStepsUnlockedByEntryID( vector<Song *> &apSongsOut, vector<Difficulty> &apStepsOut, RString sEntryID );

	const UnlockEntry *FindSong( const Song *pSong ) const;
	const UnlockEntry *FindSteps( const Song *pSong, const Steps *pSteps ) const;
	const UnlockEntry *FindCourse( const Course *pCourse ) const;
	const UnlockEntry *FindModifier( const RString &sOneMod ) const;

	// Lua
	void PushSelf( lua_State *L );

private:
	// read unlocks
	void Load();
	
	set<RString> m_RouletteCodes; // "codes" which are available in roulette and which unlock if rouletted
};

extern UnlockManager*	UNLOCKMAN;  // global and accessable from anywhere in program

#endif

/*
 * (c) 2001-2004 Kevin Slaughter, Andrew Wong, Glenn Maynard
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
