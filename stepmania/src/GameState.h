/* GameState - Holds game data that is not saved between sessions. */

#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Attack.h"
#include "Difficulty.h"
#include "GameConstantsAndTypes.h"
#include "Grade.h"
#include "MessageManager.h"
#include "ModsGroup.h"
#include "RageTimer.h"
#include "PlayerOptions.h"
#include "SongOptions.h"
#include "Preference.h"

#include <map>
#include <deque>
#include <set>

class Character;
class Course;
class Game;
struct lua_State;
class LuaTable;
class PlayerState;
class PlayerOptions;
class Profile;
class Song;
class Steps;
class StageStats;
class Style;
class TimingData;
class Trail;

class GameState
{
public:
	GameState();
	~GameState();
	void Reset();
	void ResetPlayer( PlayerNumber pn );
	void ApplyCmdline(); // called by Reset
	void ApplyGameCommand( const RString &sCommand, PlayerNumber pn=PLAYER_INVALID );
	void BeginGame();	// called when first player joins
	void JoinPlayer( PlayerNumber pn );
	void UnjoinPlayer( PlayerNumber pn );
	bool JoinInput( PlayerNumber pn );
	bool JoinPlayers();
	void LoadProfiles( bool bLoadEdits = true );
	void SavePlayerProfiles();
	void SavePlayerProfile( PlayerNumber pn );
	bool HaveProfileToLoad();
	bool HaveProfileToSave();
	void SaveLocalData();
	void LoadCurrentSettingsFromProfile( PlayerNumber pn );
	void SaveCurrentSettingsToProfile( PlayerNumber pn ); // called at the beginning of each stage
	Song* GetDefaultSong() const;

	void Update( float fDelta );


	//
	// Main state info
	//
	void SetCurGame( const Game *pGame );	// Call this instead of m_pCurGame.Set to make sure PREFSMAN->m_sCurrentGame stays in sync
	BroadcastOnChangePtr<const Game>	m_pCurGame;
	BroadcastOnChangePtr<const Style>	m_pCurStyle;
	bool					m_bSideIsJoined[NUM_PLAYERS];	// left side, right side
	MultiPlayerStatus			m_MultiPlayerStatus[NUM_MultiPlayer];
	BroadcastOnChange<PlayMode>		m_PlayMode;			// many screens display different info depending on this value
	BroadcastOnChange<int>			m_iCoins;			// not "credits"
	PlayerNumber				m_MasterPlayerNumber;		// used in Styles where one player controls both sides
	bool					m_bMultiplayer;
	int					m_iNumMultiplayerNoteFields;
	bool DifficultiesLocked() const;
	bool ChangePreferredDifficultyAndStepsType( PlayerNumber pn, Difficulty dc, StepsType st );
	bool ChangePreferredDifficulty( PlayerNumber pn, int dir );
	bool ChangePreferredCourseDifficultyAndStepsType( PlayerNumber pn, CourseDifficulty cd, StepsType st );
	bool ChangePreferredCourseDifficulty( PlayerNumber pn, int dir );
	bool IsCourseDifficultyShown( CourseDifficulty cd );
	Difficulty GetClosestShownDifficulty( PlayerNumber pn ) const;
	Difficulty GetEasiestStepsDifficulty() const;
	RageTimer			m_timeGameStarted;	// from the moment the first player pressed Start
	LuaTable			*m_Environment;

	/* This is set to a random number per-game/round; it can be used for a random seed. */
	int				m_iGameSeed, m_iStageSeed;
	RString				m_sStageGUID;

	bool		PlayersCanJoin() const;	// true if it's not too late for a player to join
	int 		GetCoinsNeededToJoin() const;
	bool		EnoughCreditsToJoin() const { return m_iCoins >= GetCoinsNeededToJoin(); }
	int		GetNumSidesJoined() const;

	const Game*	GetCurrentGame();
	const Style*	GetCurrentStyle() const;
	void		SetCurrentStyle( const Style *pStyle );

	void GetPlayerInfo( PlayerNumber pn, bool& bIsEnabledOut, bool& bIsHumanOut );
	bool IsPlayerEnabled( PlayerNumber pn ) const;
	bool IsMultiPlayerEnabled( MultiPlayer mp ) const;
	bool IsPlayerEnabled( const PlayerState* pPlayerState ) const;
	int	GetNumPlayersEnabled() const;

	bool IsHumanPlayer( PlayerNumber pn ) const;
	int GetNumHumanPlayers() const;
	PlayerNumber GetFirstHumanPlayer() const;
	PlayerNumber GetFirstDisabledPlayer() const;
	bool IsCpuPlayer( PlayerNumber pn ) const;
	bool AnyPlayersAreCpu() const;


	bool IsCourseMode() const;
	bool IsBattleMode() const; /* not Rave */

	bool ShowW1() const;

	BroadcastOnChange<RString>	m_sPreferredSongGroup;		// GROUP_ALL denotes no preferred group
	BroadcastOnChange<RString>	m_sPreferredCourseGroup;	// GROUP_ALL denotes no preferred group
	bool				m_bChangedFailTypeOnScreenSongOptions;	// true if FailType was changed in the song options screen
	BroadcastOnChange<StepsType>				m_PreferredStepsType;
	BroadcastOnChange1D<Difficulty,NUM_PLAYERS>		m_PreferredDifficulty;
	BroadcastOnChange1D<CourseDifficulty,NUM_PLAYERS>	m_PreferredCourseDifficulty;// used in nonstop
	BroadcastOnChange<SortOrder>	m_SortOrder;			// set by MusicWheel
	SortOrder			m_PreferredSortOrder;		// used by MusicWheel
	EditMode			m_EditMode;
	bool				IsEditing() const { return m_EditMode != EditMode_Invalid; }
	bool				m_bDemonstrationOrJukebox;	// ScreenGameplay does special stuff when this is true
	bool				m_bJukeboxUsesModifiers;
	int				m_iNumStagesOfThisSong;
	// Increases every stage, doesn't reset when player continues.  It's cosmetic and not used in Stage or Screen branching logic.
	int				m_iCurrentStageIndex;
	// Num stages available for player.  Resets when player joins/continues.
	int				m_iPlayerStageTokens[NUM_PLAYERS];

	static int GetNumStagesMultiplierForSong( const Song* pSong );
	static int GetNumStagesForSongAndStyleType( const Song* pSong, StyleType st );
	int GetNumStagesForCurrentSongAndStepsOrCourse() const;

	void		BeginStage();
	void		CancelStage();
	void		CommitStageStats();
	void		FinishStage();
	int		GetNumStagesLeft( PlayerNumber pn ) const;
	int		GetSmallestNumStagesLeftForAnyHumanPlayer() const;
	bool		IsFinalStageForAnyHumanPlayer() const;
	bool		IsAnExtraStage() const;
	bool		IsAnExtraStageAndSelectionLocked() const;
	bool		IsExtraStage() const;
	bool		IsExtraStage2() const;
	Stage		GetCurrentStage() const;
	int		GetCourseSongIndex() const;
	RString		GetPlayerDisplayName( PlayerNumber pn ) const;

	bool		m_bLoadingNextSong;
	int		GetLoadingCourseSongIndex() const;

	//
	// State Info used during gameplay
	//

	// NULL on ScreenSelectMusic if the currently selected wheel item isn't a Song.
	BroadcastOnChangePtr<Song>	m_pCurSong;
	// The last Song that the user manually changed to.
	Song*		m_pPreferredSong;
	BroadcastOnChangePtr1D<Steps,NUM_PLAYERS> m_pCurSteps;
	
	// NULL on ScreenSelectMusic if the currently selected wheel item isn't a Course.
	BroadcastOnChangePtr<Course>	m_pCurCourse;
	// The last Course that the user manually changed to.
	Course*		m_pPreferredCourse;
	BroadcastOnChangePtr1D<Trail,NUM_PLAYERS>	m_pCurTrail;

	bool        m_bBackedOutOfFinalStage;
	
	//
	// Music statistics:  Arcade: the current stage (one song).  Oni/Endles: a single song in a course
	//
	// Let a lot of classes access this info here so the don't have to keep their own copies.
	//
	float		m_fMusicSeconds;	// time into the current song, not scaled by music rate
	float		m_fSongBeat;
	float		m_fSongBeatNoOffset;
	float		m_fCurBPS;
	float		m_fLightSongBeat; // g_fLightsFalloffSeconds ahead
	bool		m_bFreeze;	// in the middle of a freeze
	RageTimer	m_LastBeatUpdate; // time of last m_fSongBeat, etc. update
	BroadcastOnChange<bool> m_bGameplayLeadIn;

	float		m_fMusicSecondsVisible;
	float		m_fSongBeatVisible;

	void GetAllUsedNoteSkins( vector<RString> &out ) const;

	static const float MUSIC_SECONDS_INVALID;

	void ResetMusicStatistics();	// Call this when it's time to play a new song.  Clears the values above.
	void UpdateSongPosition( float fPositionSeconds, const TimingData &timing, const RageTimer &timestamp = RageZeroTimer );
	float GetSongPercent( float beat ) const;

	bool AllAreInDangerOrWorse() const;
	bool OneIsHot() const;

	//
	// Haste
	//
	float			m_fHasteRate; // [-1,+1]; 0 = normal speed
	float			m_fLastHasteUpdateMusicSeconds;
	float			m_fAccumulatedHasteSeconds;

	//
	// Random Attacks & Attack Mines
	//
	vector<RString>		m_RandomAttacks;

	// used in PLAY_MODE_BATTLE
	float	m_fOpponentHealthPercent;

	// used in PLAY_MODE_RAVE
	float	m_fTugLifePercentP1;

	// used in workout
	bool	m_bGoalComplete[NUM_PLAYERS];
	bool	m_bWorkoutGoalComplete;
	
	void RemoveAllActiveAttacks();	// called on end of song
	PlayerNumber GetBestPlayer() const;
	StageResult GetStageResult( PlayerNumber pn ) const;

	void ResetStageStatistics();	// Call this when it's time to play a new stage.

	//
	// Options stuff
	//

	ModsGroup<SongOptions>	m_SongOptions;

	// True if the current mode has changed the default NoteSkin, such as Edit/Sync Songs does.
	// Note: any mode that wants to use it must set it
	bool m_bDidModeChangeNoteSkin;

	void GetDefaultPlayerOptions( PlayerOptions &po );
	void GetDefaultSongOptions( SongOptions &so );
	void ResetToDefaultSongOptions( ModsLevel l );
	void ApplyPreferredModifiers( PlayerNumber pn, RString sModifiers );
	void ApplyStageModifiers( PlayerNumber pn, RString sModifiers );
	void ClearStageModifiersIllegalForCourse();
	void ResetOptions();

	bool CurrentOptionsDisqualifyPlayer( PlayerNumber pn );
	bool PlayerIsUsingModifier( PlayerNumber pn, const RString &sModifier );

	PlayerOptions::FailType GetPlayerFailType( const PlayerState *pPlayerState ) const;

	// character stuff
	Character* m_pCurCharacters[NUM_PLAYERS];


	bool HasEarnedExtraStage() const { return m_bEarnedExtraStage; }


	//
	// Ranking Stuff
	//
	struct RankingFeat
	{
		enum { SONG, COURSE, CATEGORY } Type;
		Song* pSong;		// valid if Type == SONG
		Steps* pSteps;		// valid if Type == SONG
		Course* pCourse;	// valid if Type == COURSE
		Grade grade;
		int iScore;
		float fPercentDP;
		RString Banner;
		RString Feat;
		RString *pStringToFill;
	};

	void GetRankingFeats( PlayerNumber pn, vector<RankingFeat> &vFeatsOut ) const;
	bool AnyPlayerHasRankingFeats() const;
	void StoreRankingName( PlayerNumber pn, RString name );	// Called by name entry screens
	vector<RString*> m_vpsNamesThatWereFilled;	// filled on StoreRankingName, 


	//
	// Award stuff
	//
	// lowest priority in front, highest priority at the back.
	deque<StageAward> m_vLastStageAwards[NUM_PLAYERS];
	deque<PeakComboAward> m_vLastPeakComboAwards[NUM_PLAYERS];


	//
	// Attract stuff
	//
	int m_iNumTimesThroughAttract;	// negative means play regardless of m_iAttractSoundFrequency setting
	bool IsTimeToPlayAttractSounds() const;
	void VisitAttractScreen( const RString sScreenName );

	//
	// PlayerState
	//
	PlayerState* m_pPlayerState[NUM_PLAYERS];
	PlayerState* m_pMultiPlayerState[NUM_MultiPlayer];

	//
	// Preferences
	//
	static Preference<bool> m_bAutoJoin;

	//
	// These options have weird interactions depending on m_bEventMode, 
	// so wrap them
	//
	bool		m_bTemporaryEventMode;
	bool		IsEventMode() const;
	CoinMode	GetCoinMode() const;
	Premium		GetPremium() const;
	
	//
	// Edit stuff
	//
	BroadcastOnChange<StepsType> m_stEdit;
	BroadcastOnChange<CourseDifficulty> m_cdEdit;
	BroadcastOnChangePtr<Steps> m_pEditSourceSteps;
	BroadcastOnChange<StepsType> m_stEditSource;
	BroadcastOnChange<int> m_iEditCourseEntryIndex;
	BroadcastOnChange<RString> m_sEditLocalProfileID;
	Profile* GetEditLocalProfile();
	int8_t		m_iEditMidiNote;

	// Workout stuff
	float GetGoalPercentComplete( PlayerNumber pn );
	bool IsGoalComplete( PlayerNumber pn )	{ return GetGoalPercentComplete( pn ) >= 1; }

	// Lua
	void PushSelf( lua_State *L );
	
	// Keep extra stage logic internal to GameState.
private:
	EarnedExtraStage	CalculateEarnedExtraStage() const;
	int	m_iAwardedExtraStages[NUM_PLAYERS];
	bool	m_bEarnedExtraStage;

};

PlayerNumber GetNextHumanPlayer( PlayerNumber pn );
PlayerNumber GetNextEnabledPlayer( PlayerNumber pn );
PlayerNumber GetNextCpuPlayer( PlayerNumber pn );
PlayerNumber GetNextPotentialCpuPlayer( PlayerNumber pn );

#define FOREACH_HumanPlayer( pn ) for( PlayerNumber pn=GetNextHumanPlayer((PlayerNumber)-1); pn!=PLAYER_INVALID; pn=GetNextHumanPlayer(pn) )
#define FOREACH_EnabledPlayer( pn ) for( PlayerNumber pn=GetNextEnabledPlayer((PlayerNumber)-1); pn!=PLAYER_INVALID; pn=GetNextEnabledPlayer(pn) )
#define FOREACH_CpuPlayer( pn ) for( PlayerNumber pn=GetNextCpuPlayer((PlayerNumber)-1); pn!=PLAYER_INVALID; pn=GetNextCpuPlayer(pn) )
#define FOREACH_PotentialCpuPlayer( pn ) for( PlayerNumber pn=GetNextPotentialCpuPlayer((PlayerNumber)-1); pn!=PLAYER_INVALID; pn=GetNextPotentialCpuPlayer(pn) )


MultiPlayer GetNextEnabledMultiPlayer( MultiPlayer mp );
#define FOREACH_EnabledMultiPlayer( mp ) for( MultiPlayer mp=GetNextEnabledMultiPlayer((MultiPlayer)-1); mp!=MultiPlayer_Invalid; mp=GetNextEnabledMultiPlayer(mp) )

extern GameState*	GAMESTATE;	// global and accessable from anywhere in our program

#endif

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard, Chris Gomez
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
