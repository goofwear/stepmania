#ifndef GAMESTATE_H
#define GAMESTATE_H
/*
-----------------------------------------------------------------------------
 Class: GameState

 Desc: Holds game data that is not saved between sessions.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "GameConstantsAndTypes.h"
#include "PlayerOptions.h"
#include "SongOptions.h"
#include "Game.h"
#include "Style.h"
#include "Grade.h"
#include "StageStats.h"

#include <map>

class Song;
class Steps;
class Course;
class GameDef;
class StyleDef;
class NoteFieldPositioning;
class Character;
class UnlockSystem;

class GameState
{
public:
	GameState();
	~GameState();
	void Reset();

	void Update( float fDelta );


	//
	// Main state info
	//
	UnlockSystem	*m_pUnlockingSys;
	Game			m_CurGame;
	Style			m_CurStyle;
	bool			m_bPlayersCanJoin;	// true if it's not too late for a player to join - this only has an effect on the credits message
	bool			m_bSideIsJoined[NUM_PLAYERS];	// left side, right side
	PlayMode		m_PlayMode;			// many screens display different info depending on this value
	int				m_iCoins;			// not "credits"
	PlayerNumber	m_MasterPlayerNumber;	// used in Styles where one player controls both sides
	bool			m_bIsOnSystemMenu; // system screens will not be effected by the operator key -- Miryokuteki
	bool			m_bDifficultCourses; //used in nonstop

	/* This is set to a random number per-game/round; it can be used for a random seed. */
	int				m_iGameSeed, m_iRoundSeed;

	int				GetNumSidesJoined()
	{ 
		int iNumSidesJoined = 0;
		for( int c=0; c<NUM_PLAYERS; c++ )
			if( m_bSideIsJoined[c] )
				iNumSidesJoined++;	// left side, and right side
		return iNumSidesJoined;
	};

	GameDef*	GetCurrentGameDef();
	const StyleDef*	GetCurrentStyleDef();

	void GetPlayerInfo( PlayerNumber pn, bool& bIsEnabledOut, bool& bIsHumanOut );
	bool IsPlayerEnabled( PlayerNumber pn );
	bool IsPlayerEnabled( int p ) { return IsPlayerEnabled( (PlayerNumber)p ); };
	int	GetNumPlayersEnabled()
	{ 
		int count = 0;
		for( int p=0; p<NUM_PLAYERS; p++ )
			if( IsPlayerEnabled(p) )
				count++;
		return count;
	};
	bool IsHumanPlayer( PlayerNumber pn );
	bool IsHumanPlayer( int p ) { return IsHumanPlayer( (PlayerNumber)p ); };
	PlayerNumber GetFirstHumanPlayer();
	bool IsCpuPlayer( PlayerNumber pn );
	bool IsCpuPlayer( int p ) { return IsCpuPlayer( (PlayerNumber)p ); };
	bool AnyPlayersAreCpu()
	{ 
		for( int p=0; p<NUM_PLAYERS; p++ )
			if( IsCpuPlayer(p) )
				return true;
		return false;
	}
	void GetCharacters( vector<Character*> &apCharactersOut );
	Character* GameState::GetRandomCharacter();
	Character* GameState::GetDefaultCharacter();

	PlayerController	m_PlayerController[NUM_PLAYERS];
	
	// Used in Battle and Rave
	int					m_iCpuSkill[NUM_PLAYERS];	// only used when m_PlayerController is PC_CPU
	// Used in Rave
	float				m_fSuperMeterGrowthScale[NUM_PLAYERS];


	bool IsCourseMode() const;
	bool IsBattleMode() const; /* not Rave */

	bool ShowMarvelous() const;

	CString			m_sLoadingMessage;	// used in loading screen
	CString			m_sPreferredGroup;	// GROUP_ALL_MUSIC denotes no preferred group
	bool			m_bChangedFailType;	// true if FailType was changed in the song options screen
	Difficulty		m_PreferredDifficulty[NUM_PLAYERS];
	SongSortOrder	m_SongSortOrder;			// used by MusicWheel
	bool			m_bEditing;			// NoteField does special stuff when this is true
	bool			m_bDemonstrationOrJukebox;	// ScreenGameplay does special stuff when this is true
	bool			m_bJukeboxUsesModifiers;
	int				m_iCurrentStageIndex;	// incremented on Eval screen.  For a Course, this is always 0

	int				GetStageIndex();
	int				GetNumStagesLeft();
	bool			IsFinalStage();
	bool			IsExtraStage();
	bool			IsExtraStage2();
	CString			GetStageText();
	void			GetAllStageTexts( CStringArray &out );
	int				GetCourseSongIndex();


	//
	// State Info used during gameplay
	//
	Song*		m_pCurSong;
	Steps*		m_pCurNotes[NUM_PLAYERS];
	Course*		m_pCurCourse;


	//
	// Music statistics:  Arcade: the current stage (one song).  Oni/Endles: a single song in a course
	//
	// Let a lot of classes access this info here so the don't have to keep their own copies.
	//
	float		m_fMusicSeconds;	// time into the current song
	float		m_fSongBeat;
	float		m_fCurBPS;
	bool		m_bFreeze;	// in the middle of a freeze
	bool		m_bPastHereWeGo;
	float		m_fLastDrawnBeat[NUM_PLAYERS]; // set by NoteField

	map<float,CString> m_BeatToNoteSkin[NUM_PLAYERS];
	int			m_BeatToNoteSkinRev; /* hack: incremented whenever m_BeatToNoteSkin changes */
	void ResetNoteSkins();

	static const float MUSIC_SECONDS_INVALID;

	void ResetMusicStatistics();	// Call this when it's time to play a new song.  Clears the values above.
	void UpdateSongPosition(float fPositionSeconds);

	//
	// Stage Statistics: 
	// Arcade:	for the current stage (one song).  
	// Nonstop/Oni/Endless:	 for current course (which usually contains multiple songs)
	//
	StageStats	m_CurStageStats;				// current stage (not necessarily passed if Extra Stage)

	// used in PLAY_MODE_BATTLE and PLAY_MODE_RAVE
	struct Attack
	{
		AttackLevel	level;
		float fSecsRemaining;
		CString sModifier;
		bool IsBlank() { return sModifier.empty(); }
		void MakeBlank() { sModifier=""; }
	};
	Attack	m_ActiveAttacks[NUM_PLAYERS][NUM_INVENTORY_SLOTS];
	vector<PlayerOptions::Transform>	m_TransformsToApply[NUM_PLAYERS];

	// used in PLAY_MODE_BATTLE
	Attack	m_Inventory[NUM_PLAYERS][NUM_INVENTORY_SLOTS];
	float	m_fOpponentHealthPercent;

	// used in PLAY_MODE_RAVE
	float	m_fTugLifePercentP1;
	float	m_fSuperMeter[NUM_PLAYERS];	// between 0 and NUM_ATTACK_LEVELS
	
	bool	m_bActiveAttackEndedThisUpdate[NUM_PLAYERS];	// flag for other objects to watch (play sounds)
	void GetUndisplayedBeats( PlayerNumber pn, float TotalSeconds, float &StartBeat, float &EndBeat ); // only meaningful when a NoteField is in use
	void LaunchAttack( PlayerNumber target, Attack aa );
	void RebuildPlayerOptionsFromActiveAttacks( PlayerNumber pn );
	void RemoveAllActiveAttacks()	// called on end of song
	{
		for( int p=0; p<NUM_PLAYERS; p++ )
			RemoveActiveAttacksForPlayer( (PlayerNumber)p );
	}
	void RemoveActiveAttacksForPlayer( PlayerNumber pn );
	void RemoveAllInventory();
	int GetSumOfActiveAttackLevels( PlayerNumber pn );
	PlayerNumber GetBestPlayer();
	StageResult GetStageResult( PlayerNumber pn );

	void ResetStageStatistics();	// Call this when it's time to play a new stage.
	

	vector<StageStats>	m_vPlayedStageStats;	// Only useful in Arcade for final evaluation
												// A song is only inserted here if at least one player passed.
												// StageStats are added by the Evaluation screen

	void	GetFinalEvalStatsAndSongs( StageStats& statsOut, vector<Song*>& vSongsOut );	// shown on arcade final evaluation


	//
	// Options stuff
	//

	PlayerOptions	m_CurrentPlayerOptions[NUM_PLAYERS];    // current approaches destination
	PlayerOptions	m_PlayerOptions[NUM_PLAYERS];			// change this, and current will move gradually toward it
	PlayerOptions   m_StoredPlayerOptions[NUM_PLAYERS];	// user's choices on the PlayerOptions screen
	SongOptions		m_SongOptions;
	SongOptions		m_StoredSongOptions;

	void ApplyModifiers( PlayerNumber pn, CString sModifiers );
	void StoreSelectedOptions();
	void RestoreSelectedOptions();

	void AdjustFailType();

	// character stuff
	private:
		vector<Character*> m_pCharacters;
	public:
	Character* m_pCurCharacters[NUM_PLAYERS];

	void ReloadCharacters();

	


	bool HasEarnedExtraStage();
	bool m_bAllow2ndExtraStage; //only used when "Allow Selection of Extra Stage is on"


	//
	// Ranking Stuff
	//
	void GetRankingFeats( PlayerNumber pn, CStringArray &asFeatsOut, vector<CString*> &vpStringsToFillOut );

	/* Called by name entry screens: */
	void StoreRankingName( PlayerNumber pn, CString name );

	//
	// Arrow positioning
	//
	NoteFieldPositioning *m_pPosition;
};


extern GameState*	GAMESTATE;	// global and accessable from anywhere in our program

#endif
