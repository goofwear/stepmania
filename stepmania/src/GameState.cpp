#include "global.h"
#include "GameState.h"
#include "GameManager.h"
#include "PrefsManager.h"
#include "song.h"
#include "Course.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "SongManager.h"
#include "SongUtil.h"
#include "Steps.h"
#include "HighScore.h"
#include "NoteSkinManager.h"
#include "GameCommand.h"
#include "Character.h"
#include "UnlockManager.h"
#include "AnnouncerManager.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "ThemeManager.h"
#include "LightsManager.h"
#include "RageFile.h"
#include "Bookkeeper.h"
#include "MemoryCardManager.h"
#include "StatsManager.h"
#include "GameConstantsAndTypes.h"
#include "CommonMetrics.h"
#include "Actor.h"
#include "PlayerState.h"
#include "Style.h"
#include "MessageManager.h"
#include "CommonMetrics.h"
#include "Foreach.h"
#include "LuaReference.h"
#include "CommonMetrics.h"
#include "CharacterManager.h"
#include "Game.h"
#include "AdjustSync.h"

#include <ctime>
#include <set>


GameState*	GAMESTATE = NULL;	// global and accessable from anywhere in our program

#define NAME_BLACKLIST_FILE "/Data/NamesBlacklist.txt"

ThemeMetric<bool> USE_NAME_BLACKLIST("GameState","UseNameBlacklist");

ThemeMetric<RString> DEFAULT_SORT	("GameState","DefaultSort");
SortOrder GetDefaultSort()
{
	return StringToSortOrder( DEFAULT_SORT );
}
ThemeMetric<RString> DEFAULT_SONG	("GameState","DefaultSong");
Song* GameState::GetDefaultSong() const
{
	SongID sid;
	sid.LoadFromDir( DEFAULT_SONG );
	return sid.ToSong();
}


GameState::GameState() :
	m_pCurGame(			Message_CurrentGameChanged ),
	m_pCurStyle(			Message_CurrentStyleChanged ),
	m_PlayMode(			Message_PlayModeChanged ),
	m_sPreferredSongGroup(		Message_PreferredSongGroupChanged ),
	m_sPreferredCourseGroup(	Message_PreferredCourseGroupChanged ),
	m_PreferredDifficulty(		Message_PreferredDifficultyP1Changed ),
	m_PreferredCourseDifficulty(	Message_PreferredCourseDifficultyP1Changed ),
	m_SortOrder(			Message_SortOrderChanged ),
	m_pCurSong(			Message_CurrentSongChanged ),
	m_pCurSteps(			Message_CurrentStepsP1Changed ),
	m_pCurCourse(			Message_CurrentCourseChanged ),
	m_pCurTrail(			Message_CurrentTrailP1Changed ),
	m_bGameplayLeadIn(		Message_GameplayLeadInChanged ),
	m_stEdit(			Message_EditStepsTypeChanged ),
	m_cdEdit(			Message_EditCourseDifficultyChanged ),
	m_pEditSourceSteps(		Message_EditSourceStepsChanged ),
	m_stEditSource(			Message_EditSourceStepsTypeChanged ),
	m_iEditCourseEntryIndex(	Message_EditCourseEntryIndexChanged ),
	m_sEditLocalProfileID(		Message_EditLocalProfileIDChanged )
{
	m_pCurStyle.Set( NULL );

	m_pCurGame.Set( NULL );
	m_iCoins = 0;
	m_timeGameStarted.SetZero();
	m_bDemonstrationOrJukebox = false;

	m_iNumTimesThroughAttract = -1;	// initial screen will bump this up to 0
	m_iStageSeed = m_iGameSeed = 0;

	m_PlayMode.Set( PLAY_MODE_INVALID ); // used by IsPlayerEnabled before the first screen
	FOREACH_PlayerNumber( p )
		m_bSideIsJoined[p] = false; // used by GetNumSidesJoined before the first screen

	FOREACH_PlayerNumber( p )
	{
		m_pPlayerState[p] = new PlayerState;
		m_pPlayerState[p]->m_PlayerNumber = p;
	}
	FOREACH_MultiPlayer( p )
	{
		m_pMultiPlayerState[p] = new PlayerState;
		m_pMultiPlayerState[p]->m_PlayerNumber = PLAYER_1;
		m_pMultiPlayerState[p]->m_mp = p;
	}

	m_Environment = new LuaTable;

	/* Don't reset yet; let the first screen do it, so we can
	 * use PREFSMAN and THEME. */
//	Reset();
}

GameState::~GameState()
{
	FOREACH_PlayerNumber( p )
		SAFE_DELETE( m_pPlayerState[p] );
	FOREACH_MultiPlayer( p )
		SAFE_DELETE( m_pMultiPlayerState[p] );

	SAFE_DELETE( m_Environment );
}

void GameState::ApplyGameCommand( const RString &sCommand, PlayerNumber pn )
{
	GameCommand m;
	m.Load( 0, ParseCommands(sCommand) );

	RString sWhy;
	if( !m.IsPlayable(&sWhy) )
		RageException::Throw( "Can't apply mode \"%s\": %s", sCommand.c_str(), sWhy.c_str() );

	if( pn == PLAYER_INVALID )
		m.ApplyToAllPlayers();
	else
		m.Apply( pn );
}

void GameState::ApplyCmdline()
{
	/* We need to join players before we can set the style. */
	RString sPlayer;
	for( int i = 0; GetCommandlineArgument( "player", &sPlayer, i ); ++i )
	{
		int pn = atoi( sPlayer )-1;
		if( !IsAnInt( sPlayer ) || pn < 0 || pn >= NUM_PLAYERS )
			RageException::Throw( "Invalid argument \"--player=%s\"", sPlayer.c_str() );

		this->JoinPlayer( (PlayerNumber) pn );
	}

	RString sMode;
	for( int i = 0; GetCommandlineArgument( "mode", &sMode, i ); ++i )
	{
		ApplyGameCommand( sMode );
	}
}

void GameState::Reset()
{
	EndGame();
	
	ASSERT( THEME );

	m_timeGameStarted.SetZero();
	m_pCurStyle.Set( NULL );
	FOREACH_PlayerNumber( p )
		m_bSideIsJoined[p] = false;
	FOREACH_MultiPlayer( p )
		m_MultiPlayerStatus[p] = MultiPlayerStatus_NotJoined;
	MEMCARDMAN->UnlockCards();
//	m_iCoins = 0;	// don't reset coin count!
	m_MasterPlayerNumber = PLAYER_INVALID;
	m_bMultiplayer = false;
	*m_Environment = LuaTable();
	m_sPreferredSongGroup.Set( GROUP_ALL );
	m_sPreferredCourseGroup.Set( GROUP_ALL );
	m_bChangedFailTypeOnScreenSongOptions = false;
	FOREACH_PlayerNumber( p )
	{
		m_PreferredDifficulty[p].Set( DIFFICULTY_INVALID );
		m_PreferredCourseDifficulty[p].Set( DIFFICULTY_MEDIUM );
	}
	m_SortOrder.Set( SORT_INVALID );
	m_PreferredSortOrder = GetDefaultSort();
	m_PlayMode.Set( PLAY_MODE_INVALID );
	m_EditMode = EditMode_INVALID;
	m_bDemonstrationOrJukebox = false;
	m_bJukeboxUsesModifiers = false;
	m_iCurrentStageIndex = 0;
	m_bGameplayLeadIn.Set( false );
	m_iNumStagesOfThisSong = 0;
	m_bLoadingNextSong = false;

	NOTESKIN->RefreshNoteSkinData( this->m_pCurGame );

	m_iGameSeed = rand();
	m_iStageSeed = rand();

	m_pCurSong.Set( GetDefaultSong() );
	m_pPreferredSong = NULL;
	FOREACH_PlayerNumber( p )
		m_pCurSteps[p].Set( NULL );
	m_pCurCourse.Set( NULL );
	m_pPreferredCourse = NULL;
	FOREACH_PlayerNumber( p )
		m_pCurTrail[p].Set( NULL );

	FOREACH_PlayerNumber( p )
		m_pPlayerState[p]->Reset();
	FOREACH_MultiPlayer( p )
		m_pMultiPlayerState[p]->Reset();

	ResetMusicStatistics();
	ResetStageStatistics();

	FOREACH_PlayerNumber( pn )
		PROFILEMAN->UnloadProfile( pn );

	SONGMAN->UpdatePopular();
	SONGMAN->UpdateShuffled();

	/* We may have cached trails from before everything was loaded (eg. from before
	 * SongManager::UpdatePopular could be called).  Erase the cache. */
	SONGMAN->RegenerateNonFixedCourses();

	STATSMAN->Reset();

	GAMESTATE->GetDefaultSongOptions( m_SongOptions );
	
	FOREACH_PlayerNumber(p)
	{
		// I can't think of a good reason to have both game-specific
		// default mods and theme specific default mods.  We should choose 
		// one or the other. -Chris
		// Having default modifiers in prefs is needed for several things.
		// The theme setting is for eg. BM being reverse by default.  (This
		// could be done in the title menu GameCommand, but then it wouldn't
		// affect demo, and other non-gameplay things ...) -glenn
		
		GAMESTATE->GetDefaultPlayerOptions( GAMESTATE->m_pPlayerState[p]->m_PlayerOptions );
	}

	FOREACH_PlayerNumber(p)
	{
		if( PREFSMAN->m_ShowDancingCharacters == PrefsManager::SDC_Random )
			m_pCurCharacters[p] = CHARMAN->GetRandomCharacter();
		else
			m_pCurCharacters[p] = CHARMAN->GetDefaultCharacter();
		ASSERT( m_pCurCharacters[p] );
	}

	m_bTemporaryEventMode = false;
	m_bStatsCommitted = false;

	LIGHTSMAN->SetLightsMode( LIGHTSMODE_ATTRACT );
	
	m_stEdit.Set( STEPS_TYPE_INVALID );
	m_pEditSourceSteps.Set( NULL );
	m_stEditSource.Set( STEPS_TYPE_INVALID );
	m_iEditCourseEntryIndex.Set( -1 );
	m_sEditLocalProfileID.Set( "" );
	
	m_bBackedOutOfFinalStage = false;

	ApplyCmdline();
}

void GameState::JoinPlayer( PlayerNumber pn )
{
	this->m_bSideIsJoined[pn] = true;
	MESSAGEMAN->Broadcast( (Message)(Message_SideJoinedP1+pn) );

	if( this->m_MasterPlayerNumber == PLAYER_INVALID )
		this->m_MasterPlayerNumber = pn;

	// if first player to join, set start time
	if( this->GetNumSidesJoined() == 1 )
		this->BeginGame();
}

int GameState::GetCoinsNeededToJoin() const
{
	int iCoinsToCharge = 0;

	if( GAMESTATE->GetCoinMode() == COIN_MODE_PAY )
		iCoinsToCharge = PREFSMAN->m_iCoinsPerCredit;

	// If joint premium don't take away a credit for the 2nd join.
	if( GAMESTATE->GetPremium() == PREMIUM_JOINT  &&  
		GAMESTATE->GetNumSidesJoined() == 1 )
		iCoinsToCharge = 0;

	return iCoinsToCharge;
}

/*
 * Game flow:
 *
 * BeginGame() - the first player has joined; the game is starting.
 *
 * PlayersFinalized() - no more players may join (because the style is set)
 *
 * BeginStage() - gameplay is beginning
 *
 * optional: CancelStage() - gameplay aborted (Back pressed), undo BeginStage and back up
 *
 * CommitStageStats() - gameplay is finished
 *   Saves STATSMAN->m_CurStageStats to the profiles, so profile information
 *   is up-to-date for Evaluation.
 *
 * FinishStage() - gameplay and evaluation is finished
 *   Clears data which was stored by CommitStageStats.
 *
 * EndGame() - the game is finished
 * 
 */
void GameState::BeginGame()
{
	m_timeGameStarted.Touch();

	m_vpsNamesThatWereFilled.clear();

	// Play attract on the ending screen, then on the ranking screen
	// even if attract sounds are set to off.
	m_iNumTimesThroughAttract = -1;

	MEMCARDMAN->UnlockCards();
}

void GameState::PlayersFinalized()
{
	// If cards are already locked, this was already called.
	if( MEMCARDMAN->GetCardsLocked() )
		return;

	MESSAGEMAN->Broadcast( Message_PlayersFinalized );

	MEMCARDMAN->LockCards();

	// apply saved default modifiers if any
	FOREACH_HumanPlayer( pn )
	{
		MEMCARDMAN->MountCard( pn );

		PROFILEMAN->LoadFirstAvailableProfile( pn );	// load full profile

		MEMCARDMAN->UnmountCard( pn );

		if( !PROFILEMAN->IsPersistentProfile(pn) )
			continue;	// skip

		Profile* pProfile = PROFILEMAN->GetProfile(pn);

		RString sModifiers;
		if( pProfile->GetDefaultModifiers( this->m_pCurGame, sModifiers ) )
		{
			/* We don't save negative preferences (eg. "no reverse").  If the theme
			 * sets a default of "reverse", and the player turns it off, we should
			 * set it off.  However, don't reset modifiers that aren't saved by the
			 * profile, so we don't ignore unsaved modifiers when a profile is in use. */
			GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions.ResetSavedPrefs();
			GAMESTATE->ApplyModifiers( pn, sModifiers );
		}
		// Only set the sort order if it wasn't already set by a GameCommand (or by an earlier profile)
		if( m_PreferredSortOrder == SORT_INVALID && pProfile->m_SortOrder != SORT_INVALID )
			m_PreferredSortOrder = pProfile->m_SortOrder;
		if( pProfile->m_LastDifficulty != DIFFICULTY_INVALID )
			m_PreferredDifficulty[pn].Set( pProfile->m_LastDifficulty );
		if( pProfile->m_LastCourseDifficulty != DIFFICULTY_INVALID )
			m_PreferredCourseDifficulty[pn].Set( pProfile->m_LastCourseDifficulty );
		if( m_pPreferredSong == NULL )
			m_pPreferredSong = pProfile->m_lastSong.ToSong();
		if( m_pPreferredCourse == NULL )
			m_pPreferredCourse = pProfile->m_lastCourse.ToCourse();
	}

	FOREACH_PotentialCpuPlayer( pn )
		ApplyModifiers( pn, CommonMetrics::DEFAULT_CPU_MODIFIERS );
}

void GameState::EndGame()
{
	LOG->Trace( "GameState::EndGame" );

	if( m_bDemonstrationOrJukebox )
		return;
	if( m_timeGameStarted.IsZero() || !STATSMAN->m_vPlayedStageStats.size() )	// we were in the middle of a game and played at least one song
		return;

	/* Finish the final stage. */
	FinishStage();


	// Update totalPlaySeconds stat
	int iPlaySeconds = max( 0, (int) m_timeGameStarted.PeekDeltaTime() );

	Profile* pMachineProfile = PROFILEMAN->GetMachineProfile();
	pMachineProfile->m_iTotalPlaySeconds += iPlaySeconds;
	pMachineProfile->m_iTotalPlays++;

	FOREACH_HumanPlayer( p )
	{
		Profile* pPlayerProfile = PROFILEMAN->GetProfile( p );
		if( pPlayerProfile )
		{
			pPlayerProfile->m_iTotalPlaySeconds += iPlaySeconds;
			pPlayerProfile->m_iTotalPlays++;
		}
	}

	BOOKKEEPER->WriteToDisk();

	FOREACH_HumanPlayer( pn )
	{
		if( !PROFILEMAN->IsPersistentProfile(pn) )
			continue;

		bool bWasMemoryCard = PROFILEMAN->ProfileWasLoadedFromMemoryCard(pn);
		if( bWasMemoryCard )
			MEMCARDMAN->MountCard( pn );
		PROFILEMAN->SaveProfile( pn );
		if( bWasMemoryCard )
			MEMCARDMAN->UnmountCard( pn );

		PROFILEMAN->UnloadProfile( pn );
	}

	PROFILEMAN->SaveMachineProfile();

	// make sure we don't execute EndGame twice.
	m_timeGameStarted.SetZero();
}

static int GetNumStagesForCurrentSong()
{
	int iNumStagesOfThisSong = 1;
	if( GAMESTATE->m_pCurSong )
		iNumStagesOfThisSong = SongManager::GetNumStagesForSong( GAMESTATE->m_pCurSong );
	else if( GAMESTATE->m_pCurCourse )
		iNumStagesOfThisSong = 1;
	else
		return -1;

	ASSERT( iNumStagesOfThisSong >= 1 && iNumStagesOfThisSong <= 3 );

	/* Never increment more than one past final stage.  That is, if the current
	 * stage is the final stage, and we picked a stage that takes two songs, it
	 * only counts as one stage (so it doesn't bump us all the way to Ex2).
	 * One case where this happens is a long/marathon extra stage.  Another is
	 * if a long/marathon song is selected explicitly in the theme with a GameCommand,
	 * and PREFSMAN->m_iNumArcadeStages is less than the number of stages that
	 * song takes. */
	int iNumStagesLeft = PREFSMAN->m_iSongsPerPlay - GAMESTATE->m_iCurrentStageIndex;
	iNumStagesOfThisSong = min( iNumStagesOfThisSong, iNumStagesLeft );
	iNumStagesOfThisSong = max( iNumStagesOfThisSong, 1 );

	return iNumStagesOfThisSong;
}

/* Called by ScreenGameplay.  Set the length of the current song. */
void GameState::BeginStage()
{
	if( m_bDemonstrationOrJukebox )
		return;

	/* This should only be called once per stage. */
	if( m_iNumStagesOfThisSong != 0 )
		LOG->Warn( "XXX: m_iNumStagesOfThisSong == %i?", m_iNumStagesOfThisSong );

	/* Finish the last stage (if any), if we havn't already.  (For example, we might
	 * have, for some reason, gone from gameplay to evaluation straight back to gameplay.) */
	FinishStage();

	GAMESTATE->ResetStageStatistics();

	m_iNumStagesOfThisSong = GetNumStagesForCurrentSong();
	ASSERT( m_iNumStagesOfThisSong != -1 );
}

void GameState::CancelStage()
{
	m_iNumStagesOfThisSong = 0;
	GAMESTATE->ResetStageStatistics();
}

void GameState::CommitStageStats()
{
	/* Don't commit stats twice. */
	if( m_bStatsCommitted || m_bDemonstrationOrJukebox )
		return;
	m_bStatsCommitted = true;

	STATSMAN->CommitStatsToProfiles();
}

/* Called by ScreenSelectMusic (etc).  Increment the stage counter if we just played a
 * song.  Might be called more than once. */
void GameState::FinishStage()
{
	/* If m_iNumStagesOfThisSong is 0, we've been called more than once before calling
	 * BeginStage.  This can happen when backing out of the player options screen. */
	if( m_iNumStagesOfThisSong == 0 )
		return;

	/* If we havn't committed stats yet, do so. */
	if( !m_bMultiplayer )	// no saved stats in multiplayer
		CommitStageStats();

	RestoreSelectedOptions();

	m_bStatsCommitted = false;

	// Increment the stage counter.
	ASSERT( m_iNumStagesOfThisSong >= 1 && m_iNumStagesOfThisSong <= 3 );
	const int iOldStageIndex = m_iCurrentStageIndex;
	m_iCurrentStageIndex += m_iNumStagesOfThisSong;

	m_iNumStagesOfThisSong = 0;

	// necessary so that bGaveUp is reset
	GAMESTATE->ResetStageStatistics();

	if( m_bDemonstrationOrJukebox )
		return;

	if( GAMESTATE->IsEventMode() )
	{
		const int iSaveProfileEvery = 3;
		if( iOldStageIndex/iSaveProfileEvery < m_iCurrentStageIndex/iSaveProfileEvery )
		{
			LOG->Trace( "Played %i stages; saving profiles ...", iSaveProfileEvery );
			PROFILEMAN->SaveAllProfiles();
		}
	}
}

void GameState::SaveCurrentSettingsToProfile( PlayerNumber pn )
{
	if( !PROFILEMAN->IsPersistentProfile(pn) )
		return;
	if( m_bDemonstrationOrJukebox )
		return;

	Profile* pProfile = PROFILEMAN->GetProfile(pn);

	pProfile->SetDefaultModifiers( this->m_pCurGame, m_pPlayerState[pn]->m_PlayerOptions.GetSavedPrefsString() );
	if( IsSongSort(m_PreferredSortOrder) )
		pProfile->m_SortOrder = m_PreferredSortOrder;
	if( m_PreferredDifficulty[pn] != DIFFICULTY_INVALID )
		pProfile->m_LastDifficulty = m_PreferredDifficulty[pn];
	if( m_PreferredCourseDifficulty[pn] != DIFFICULTY_INVALID )
		pProfile->m_LastCourseDifficulty = m_PreferredCourseDifficulty[pn];
	if( m_pPreferredSong )
		pProfile->m_lastSong.FromSong( m_pPreferredSong );
	if( m_pPreferredCourse )
		pProfile->m_lastCourse.FromCourse( m_pPreferredCourse );
}

void GameState::Update( float fDelta )
{
	FOREACH_PlayerNumber( p )
	{
		m_pPlayerState[p]->Update( fDelta );

		if( !m_bGoalComplete[p] && IsGoalComplete(p) )
		{
			m_bGoalComplete[p] = true;
			MESSAGEMAN->Broadcast( (Message)(Message_GoalCompleteP1+p) );
		}
	}
}

void GameState::SetCurGame( const Game *pGame )
{
	m_pCurGame.Set( pGame );
	RString sGame = pGame ? RString(pGame->m_szName) : RString();
	PREFSMAN->SetCurrentGame( sGame );
}

const float GameState::MUSIC_SECONDS_INVALID = -5000.0f;

void GameState::ResetMusicStatistics()
{	
	m_fMusicSeconds = 0; // MUSIC_SECONDS_INVALID;
	m_fSongBeat = 0;
	m_fSongBeatNoOffset = 0;
	m_fCurBPS = 10;
	m_bFreeze = false;
	Actor::SetBGMTime( 0, 0 );
}

void GameState::ResetStageStatistics()
{
	StageStats OldStats = STATSMAN->m_CurStageStats;
	STATSMAN->m_CurStageStats = StageStats();
	if( PREFSMAN->m_bComboContinuesBetweenSongs )
	{
		if( GetStageIndex() == 0 )
		{
			FOREACH_PlayerNumber( p )
			{
				Profile* pProfile = PROFILEMAN->GetProfile(p);
				STATSMAN->m_CurStageStats.m_player[p].iCurCombo = pProfile->m_iCurrentCombo;
			}
		}
		else	// GetStageIndex() > 0
		{
			FOREACH_PlayerNumber( p )
			{
				STATSMAN->m_CurStageStats.m_player[p].iCurCombo = OldStats.m_player[p].iCurCombo;
			}
		}
	}

	RemoveAllActiveAttacks();
	FOREACH_PlayerNumber( p )
		m_pPlayerState[p]->RemoveAllInventory();
	m_fOpponentHealthPercent = 1;
	m_fTugLifePercentP1 = 0.5f;
	FOREACH_PlayerNumber( p )
	{
		m_pPlayerState[p]->m_fSuperMeter = 0;
		m_pPlayerState[p]->m_HealthState = PlayerState::ALIVE;

		m_pPlayerState[p]->m_iLastPositiveSumOfAttackLevels = 0;
		m_pPlayerState[p]->m_fSecondsUntilAttacksPhasedOut = 0;	// PlayerAI not affected

		m_bGoalComplete[p] = false;
	}


	FOREACH_PlayerNumber( p )
	{
		m_vLastPerDifficultyAwards[p].clear();
		m_vLastPeakComboAwards[p].clear();
	}

	// Reset the round seed.  Do this here and not in FinishStage so that players
	// get new shuffle patterns if they Back out of gameplay and play again.
	GAMESTATE->m_iStageSeed = rand();

	AdjustSync::ResetOriginalSyncData();
}

void GameState::UpdateSongPosition( float fPositionSeconds, const TimingData &timing, const RageTimer &timestamp )
{
	if( !timestamp.IsZero() )
		m_LastBeatUpdate = timestamp;
	else
		m_LastBeatUpdate.Touch();
	timing.GetBeatAndBPSFromElapsedTime( fPositionSeconds, m_fSongBeat, m_fCurBPS, m_bFreeze );
	ASSERT_M( m_fSongBeat > -2000, ssprintf("%f %f", m_fSongBeat, fPositionSeconds) );

	m_fMusicSeconds = fPositionSeconds;
	m_fLightSongBeat = timing.GetBeatFromElapsedTime( fPositionSeconds + g_fLightsAheadSeconds );

	m_fSongBeatNoOffset = timing.GetBeatFromElapsedTimeNoOffset( fPositionSeconds );
	Actor::SetBGMTime( fPositionSeconds, m_fSongBeatNoOffset );
	
//	LOG->Trace( "m_fMusicSeconds = %f, m_fSongBeat = %f, m_fCurBPS = %f, m_bFreeze = %f", m_fMusicSeconds, m_fSongBeat, m_fCurBPS, m_bFreeze );
}

float GameState::GetSongPercent( float beat ) const
{
	/* 0 = first step; 1 = last step */
	return (beat - m_pCurSong->m_fFirstBeat) / m_pCurSong->m_fLastBeat;
}

int GameState::GetStageIndex() const
{
	return m_iCurrentStageIndex;
}

int GameState::GetNumStagesLeft() const
{
	if( IsExtraStage() || IsExtraStage2() )
		return 1;
	if( GAMESTATE->IsEventMode() )
		return 999;
	return PREFSMAN->m_iSongsPerPlay - m_iCurrentStageIndex;
}

bool GameState::IsFinalStage() const
{
	if( GAMESTATE->IsEventMode() )
		return false;

	if( this->IsCourseMode() )
		return true;

	/* This changes dynamically on ScreenSelectMusic as the wheel turns. */
	int iPredictedStageForCurSong = GetNumStagesForCurrentSong();
	if( iPredictedStageForCurSong == -1 )
		iPredictedStageForCurSong = 1;
	return m_iCurrentStageIndex + iPredictedStageForCurSong == PREFSMAN->m_iSongsPerPlay;
}

bool GameState::IsExtraStage() const
{
	if( GAMESTATE->IsEventMode() )
		return false;
	return m_iCurrentStageIndex == PREFSMAN->m_iSongsPerPlay;
}

bool GameState::IsExtraStage2() const
{
	if( GAMESTATE->IsEventMode() )
		return false;
	return m_iCurrentStageIndex == PREFSMAN->m_iSongsPerPlay+1;
}

Stage GameState::GetCurrentStage() const
{
	if( m_bDemonstrationOrJukebox )				return STAGE_DEMO;
	// "event" has precedence
	else if( GAMESTATE->IsEventMode() )			return STAGE_EVENT;
	else if( m_PlayMode == PLAY_MODE_ONI )		return STAGE_ONI;
	else if( m_PlayMode == PLAY_MODE_NONSTOP )	return STAGE_NONSTOP;
	else if( m_PlayMode == PLAY_MODE_ENDLESS )	return STAGE_ENDLESS;
	else if( IsFinalStage() )					return STAGE_FINAL;
	else if( IsExtraStage() )					return STAGE_EXTRA1;
	else if( IsExtraStage2() )					return STAGE_EXTRA2;
	else										return (Stage)(STAGE_1+m_iCurrentStageIndex);
}

// Return true if it's possible for GetCurrentStage() to return the given stage.
bool GameState::IsStagePossible( Stage s ) const
{
	/* HACK: Find out what the stage would be without long or marathon.
	 * This should never change during a screen. */
	Song *pSong = GAMESTATE->m_pCurSong;
	GAMESTATE->m_pCurSong.SetWithoutBroadcast( NULL );
	Stage actual = GetCurrentStage();
	GAMESTATE->m_pCurSong.SetWithoutBroadcast( pSong );

	/* Check long/marathon, which can change the stage to FINAL. */
	if( s == STAGE_FINAL && actual >= STAGE_1 && actual <= STAGE_FINAL )
	{
		Stage max_actual = actual;
		if( PREFSMAN->m_fMarathonVerSongSeconds < 9999 )
			enum_add( max_actual, +2 );
		else if( PREFSMAN->m_fLongVerSongSeconds < 9999 )
			enum_add( max_actual, +1 );

		if( max_actual >= STAGE_FINAL )
			return true;
	}

	return s == actual;
}

int GameState::GetCourseSongIndex() const
{
	int iSongIndex = 0;
	/* iSongsPlayed includes the current song, so it's 1-based; subtract one. */
	FOREACH_EnabledPlayer( pn )
		iSongIndex = max( iSongIndex, STATSMAN->m_CurStageStats.m_player[pn].iSongsPlayed-1 );
	return iSongIndex;
}

/* Hack: when we're loading a new course song, we want to display the new song number, even
 * though we havn't started that song yet. */
int GameState::GetLoadingCourseSongIndex() const
{
	int iIndex = GetCourseSongIndex();
	if( m_bLoadingNextSong )
		++iIndex;
	return iIndex;
}

static LocalizedString PLAYER1	("GameState","Player 1");
static LocalizedString PLAYER2	("GameState","Player 2");
static LocalizedString CPU		("GameState","CPU");
RString GameState::GetPlayerDisplayName( PlayerNumber pn ) const
{
	ASSERT( IsPlayerEnabled(pn) );
	const LocalizedString *pDefaultNames[] = { &PLAYER1, &PLAYER2 };
	if( IsHumanPlayer(pn) )
	{
		if( !PROFILEMAN->GetPlayerName(pn).empty() )
			return PROFILEMAN->GetPlayerName(pn);
		else
			return pDefaultNames[pn]->GetValue();
	}
	else
	{
		return CPU.GetValue();
	}
}

bool GameState::PlayersCanJoin() const
{
	return GetNumSidesJoined() == 0 || this->m_pCurStyle == NULL;	// selecting a style finalizes the players
}

int GameState::GetNumSidesJoined() const
{ 
	int iNumSidesJoined = 0;
	FOREACH_PlayerNumber( p )
		if( m_bSideIsJoined[p] )
			iNumSidesJoined++;	// left side, and right side
	return iNumSidesJoined;
}

const Game* GameState::GetCurrentGame()
{
	ASSERT( m_pCurGame != NULL );	// the game must be set before calling this
	return m_pCurGame;
}

const Style* GameState::GetCurrentStyle() const
{
	return m_pCurStyle;
}


bool GameState::IsPlayerEnabled( PlayerNumber pn ) const
{
	// In rave, all players are present.  Non-human players are CPU controlled.
	switch( m_PlayMode )
	{
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		return true;
	}

	return IsHumanPlayer( pn );
}

bool GameState::IsMultiPlayerEnabled( MultiPlayer mp ) const
{
	return m_MultiPlayerStatus[ mp ] == MultiPlayerStatus_Joined;
}

bool GameState::IsPlayerEnabled( const PlayerState* pPlayerState ) const
{
	if( pPlayerState->m_mp != MultiPlayer_INVALID )
		return IsMultiPlayerEnabled( pPlayerState->m_mp );
	if( pPlayerState->m_PlayerNumber != PLAYER_INVALID )
		return IsPlayerEnabled( pPlayerState->m_PlayerNumber );
	return false;
}

int	GameState::GetNumPlayersEnabled() const
{
	int count = 0;
	FOREACH_EnabledPlayer( pn )
		count++;
	return count;
}

bool GameState::PlayerUsingBothSides() const
{
	ASSERT( this->GetCurrentStyle() != NULL );
	switch( GetCurrentStyle()->m_StyleType )
	{
	case ONE_PLAYER_TWO_SIDES:
	case TWO_PLAYERS_SHARED_SIDES:
		return true;
	default:
		return false;
	}
}

bool GameState::IsHumanPlayer( PlayerNumber pn ) const
{
	if( m_pCurStyle == NULL )	// no style chosen
	{
		if( this->PlayersCanJoin() )	
			return m_bSideIsJoined[pn];	// only allow input from sides that have already joined
		else
			return true;	// if we can't join, then we're on a screen like MusicScroll or GameOver
	}

	switch( GetCurrentStyle()->m_StyleType )
	{
	case TWO_PLAYERS_TWO_SIDES:
	case TWO_PLAYERS_SHARED_SIDES:
		return true;
	case ONE_PLAYER_ONE_SIDE:
	case ONE_PLAYER_TWO_SIDES:
		return pn == m_MasterPlayerNumber;
	default:
		ASSERT(0);		// invalid style type
		return false;
	}
}

int GameState::GetNumHumanPlayers() const
{
	int count = 0;
	FOREACH_HumanPlayer( pn )
		count++;
	return count;
}

PlayerNumber GameState::GetFirstHumanPlayer() const
{
	FOREACH_HumanPlayer( pn )
		return pn;
	ASSERT(0);	// there must be at least 1 human player
	return PLAYER_INVALID;
}

PlayerNumber GameState::GetFirstDisabledPlayer() const
{
	FOREACH_PlayerNumber( pn )
		if( !IsPlayerEnabled(pn) )
			return pn;
	return PLAYER_INVALID;
}

bool GameState::IsCpuPlayer( PlayerNumber pn ) const
{
	return IsPlayerEnabled(pn) && !IsHumanPlayer(pn);
}

bool GameState::AnyPlayersAreCpu() const
{ 
	FOREACH_CpuPlayer( pn )
		return true;
	return false;
}


bool GameState::IsCourseMode() const
{
	switch(m_PlayMode)
	{
	case PLAY_MODE_ONI:
	case PLAY_MODE_NONSTOP:
	case PLAY_MODE_ENDLESS:
		return true;
	default:
		return false;
	}
}

bool GameState::IsBattleMode() const
{
	switch( this->m_PlayMode )
	{
	case PLAY_MODE_BATTLE:
		return true;
	default:
		return false;
	}
}

bool GameState::HasEarnedExtraStage() const
{
	if( GAMESTATE->IsEventMode() )
		return false;

	if( !PREFSMAN->m_bAllowExtraStage )
		return false;

	if( this->m_PlayMode != PLAY_MODE_REGULAR )
		return false;
	
	if( m_bBackedOutOfFinalStage )
		return false;
	
	if( !IsFinalStage() && !IsExtraStage() )
		return false;
	
	bool bOnePassed = false;
	
	FOREACH_EnabledPlayer( pn )
	{
		if( this->m_pCurSteps[pn]->GetDifficulty() != DIFFICULTY_HARD && 
		    this->m_pCurSteps[pn]->GetDifficulty() != DIFFICULTY_CHALLENGE )
			continue; /* not hard enough! */
	
		if( STATSMAN->m_CurStageStats.m_player[pn].GetGrade() <= Grade_Tier03 )
		{
			bOnePassed = true;
			break;
		}
	}
	
	if( !bOnePassed )
		return false;

	/* If PickExtraStage, allow EX2 if the chosen song was correct. */
	if( PREFSMAN->m_bPickExtraStage && this->IsExtraStage() )
	{
		Song* pSong;
		Steps* pSteps;
		SONGMAN->GetExtraStageInfo( false, GAMESTATE->GetCurrentStyle(), pSong, pSteps, NULL, NULL );
		ASSERT(pSong);

		const StageStats &stats = STATSMAN->m_CurStageStats;
		return stats.vpPlayedSongs.size() && stats.vpPlayedSongs.back() == pSong;
	}

	return true;
}

PlayerNumber GameState::GetBestPlayer() const
{
	FOREACH_PlayerNumber( pn )
		if( GetStageResult(pn) == RESULT_WIN )
			return pn;
	return PLAYER_INVALID;	// draw
}

StageResult GameState::GetStageResult( PlayerNumber pn ) const
{
	switch( this->m_PlayMode )
	{
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		if( fabsf(m_fTugLifePercentP1 - 0.5f) < 0.0001f )
			return RESULT_DRAW;
		switch( pn )
		{
		case PLAYER_1:	return (m_fTugLifePercentP1>=0.5f)?RESULT_WIN:RESULT_LOSE;
		case PLAYER_2:	return (m_fTugLifePercentP1<0.5f)?RESULT_WIN:RESULT_LOSE;
		default:	ASSERT(0); return RESULT_LOSE;
		}
	}

	StageResult win = RESULT_WIN;
	FOREACH_PlayerNumber( p )
	{
		if( p == pn )
			continue;

		/* If anyone did just as well, at best it's a draw. */
		if( STATSMAN->m_CurStageStats.m_player[p].iActualDancePoints == STATSMAN->m_CurStageStats.m_player[pn].iActualDancePoints )
			win = RESULT_DRAW;

		/* If anyone did better, we lost. */
		if( STATSMAN->m_CurStageStats.m_player[p].iActualDancePoints > STATSMAN->m_CurStageStats.m_player[pn].iActualDancePoints )
			return RESULT_LOSE;
	}
	return win;
}



void GameState::GetDefaultPlayerOptions( PlayerOptions &po )
{
	po.Init();
	po.FromString( PREFSMAN->m_sDefaultModifiers );
	po.FromString( CommonMetrics::DEFAULT_MODIFIERS );
	if( po.m_sNoteSkin.empty() )
		po.m_sNoteSkin = NOTESKIN->GAME_BASE_NOTESKIN_NAME;
}

void GameState::GetDefaultSongOptions( SongOptions &so )
{
	so.Init();
	so.FromString( PREFSMAN->m_sDefaultModifiers );
	so.FromString( CommonMetrics::DEFAULT_MODIFIERS );
}

void GameState::ApplyModifiers( PlayerNumber pn, RString sModifiers )
{
	m_pPlayerState[pn]->m_PlayerOptions.FromString( sModifiers );
	m_SongOptions.FromString( sModifiers );
}

/* Store the player's preferred options.  This is called at the very beginning
 * of gameplay. */
void GameState::StoreSelectedOptions()
{
	FOREACH_PlayerNumber( pn )
		m_pPlayerState[pn]->m_StoredPlayerOptions = m_pPlayerState[pn]->m_PlayerOptions;
	m_StoredSongOptions = m_SongOptions;
}

void GameState::StoreStageOptions()
{
	FOREACH_PlayerNumber( pn )
		m_pPlayerState[pn]->m_StagePlayerOptions = m_pPlayerState[pn]->m_PlayerOptions;
	m_StoredSongOptions = m_SongOptions;
}

/* Restore the preferred options.  This is called after a song ends, before
 * setting new course options, so options from one song don't carry into the
 * next and we default back to the preferred options.  This is also called
 * at the end of gameplay to restore options. */
void GameState::RestoreSelectedOptions()
{
	FOREACH_PlayerNumber( pn )
		m_pPlayerState[pn]->m_PlayerOptions = m_pPlayerState[pn]->m_StoredPlayerOptions;
	m_SongOptions = m_StoredSongOptions;
}

void GameState::RestoreStageOptions()
{
	FOREACH_PlayerNumber( pn )
		m_pPlayerState[pn]->m_PlayerOptions = m_pPlayerState[pn]->m_StagePlayerOptions;
	m_SongOptions = m_StoredSongOptions;
}

void GameState::ResetCurrentOptions()
{
	FOREACH_PlayerNumber( p )
		GetDefaultPlayerOptions( m_pPlayerState[p]->m_PlayerOptions );
	GetDefaultSongOptions( m_SongOptions );
}

bool GameState::IsDisqualified( PlayerNumber pn )
{
	if( !PREFSMAN->m_bDisqualification )
		return false;

	if( !GAMESTATE->IsHumanPlayer(pn) )
		return false;

	if( STATSMAN->m_CurStageStats.bGaveUp )
		return true;

#ifndef DEBUG
	if( STATSMAN->m_CurStageStats.bUsedAutoplay )
		return true;
#endif //DEBUG

	const PlayerOptions &po = GAMESTATE->m_pPlayerState[pn]->m_StoredPlayerOptions;

	// Check the stored player options for disqualify.  Don't disqualify because
	// of mods that were forced.
	if( GAMESTATE->IsCourseMode() )
	{
		return po.IsEasierForCourseAndTrail( 
			GAMESTATE->m_pCurCourse, 
			GAMESTATE->m_pCurTrail[pn] );
	}
	else
	{
		return po.IsEasierForSongAndSteps( 
			GAMESTATE->m_pCurSong, 
			GAMESTATE->m_pCurSteps[pn] );
	}
}

void GameState::GetAllUsedNoteSkins( vector<RString> &out ) const
{
	FOREACH_EnabledPlayer( pn )
	{
		out.push_back( m_pPlayerState[pn]->m_PlayerOptions.m_sNoteSkin );

		/* Add note skins that are used in courses. */
		if( this->IsCourseMode() )
		{
			const Trail *pTrail = this->m_pCurTrail[pn];
			ASSERT( pTrail );

			FOREACH_CONST( TrailEntry, pTrail->m_vEntries, e )
			{
				PlayerOptions po;
				po.FromString( e->Modifiers );
				if( !po.m_sNoteSkin.empty() )
					out.push_back( po.m_sNoteSkin );
			}
		}
	}

	/* Remove duplicates. */
	sort( out.begin(), out.end() );
	out.erase( unique( out.begin(), out.end() ), out.end() );
}

void GameState::RemoveAllActiveAttacks()	// called on end of song
{
	FOREACH_PlayerNumber( p )
		m_pPlayerState[p]->RemoveActiveAttacks();
}

template<class T>
void setmin( T &a, const T &b )
{
	a = min(a, b);
}

template<class T>
void setmax( T &a, const T &b )
{
	a = max(a, b);
}

SongOptions::FailType GameState::GetPlayerFailType( const PlayerState *pPlayerState ) const
{
	PlayerNumber pn = pPlayerState->m_PlayerNumber;
	SongOptions::FailType ft = m_SongOptions.m_FailType;

	/* If the player changed the fail mode explicitly, leave it alone. */
	if( this->m_bChangedFailTypeOnScreenSongOptions )
		return ft;

	if( GAMESTATE->IsCourseMode() )
	{
		if( PREFSMAN->m_bMinimum1FullSongInCourses && GAMESTATE->GetCourseSongIndex()==0 )
			ft = max( ft, SongOptions::FAIL_END_OF_SONG );	// take the least harsh of the two FailTypes
	}
	else
	{
		Difficulty dc = DIFFICULTY_INVALID;
		if( m_pCurSteps[pn] )
			dc = m_pCurSteps[pn]->GetDifficulty();

		bool bFirstStage = !GAMESTATE->IsEventMode() && m_iCurrentStageIndex == 0;

		/* Easy and beginner are never harder than FAIL_END_OF_SONG. */
		if( dc <= DIFFICULTY_EASY )
			setmax( ft, SongOptions::FAIL_END_OF_SONG );

		if( dc <= DIFFICULTY_EASY && bFirstStage && PREFSMAN->m_bFailOffForFirstStageEasy )
			setmax( ft, SongOptions::FAIL_OFF );

		/* If beginner's steps were chosen, and this is the first stage,
		 * turn off failure completely. */
		if( dc == DIFFICULTY_BEGINNER && bFirstStage )
			setmax( ft, SongOptions::FAIL_OFF );

		if( dc == DIFFICULTY_BEGINNER && PREFSMAN->m_bFailOffInBeginner )
			setmax( ft, SongOptions::FAIL_OFF );
	}

	return ft;
}

bool GameState::ShowW1() const
{
	switch( PREFSMAN->m_AllowW1 )
	{
	case PrefsManager::ALLOW_W1_NEVER:			return false;
	case PrefsManager::ALLOW_W1_COURSES_ONLY:	return IsCourseMode();
	case PrefsManager::ALLOW_W1_EVERYWHERE:	return true;
	default:	ASSERT(0);
	}
}

void GameState::GetRankingFeats( PlayerNumber pn, vector<RankingFeat> &asFeatsOut ) const
{
	if( !IsHumanPlayer(pn) )
		return;

	Profile *pProf = PROFILEMAN->GetProfile(pn);

	// Check for feats even if the PlayMode is rave or battle because the player may have
	// made high scores then switched modes.
	CHECKPOINT_M(ssprintf("PlayMode %i", int(this->m_PlayMode)));
	switch( this->m_PlayMode )
	{
	case PLAY_MODE_REGULAR:
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		{
			CHECKPOINT;

			StepsType st = this->GetCurrentStyle()->m_StepsType;

			//
			// Find unique Song and Steps combinations that were played.
			// We must keep only the unique combination or else we'll double-count
			// high score markers.
			//
			vector<SongAndSteps> vSongAndSteps;

			for( unsigned i=0; i<STATSMAN->m_vPlayedStageStats.size(); i++ )
			{
				CHECKPOINT_M( ssprintf("%u/%i", i, (int)STATSMAN->m_vPlayedStageStats.size() ) );
				SongAndSteps sas;
				sas.pSong = STATSMAN->m_vPlayedStageStats[i].vpPlayedSongs[0];
				ASSERT( sas.pSong );
				sas.pSteps = STATSMAN->m_vPlayedStageStats[i].m_player[pn].vpPlayedSteps[0];
				ASSERT( sas.pSteps );
				vSongAndSteps.push_back( sas );
			}
			CHECKPOINT;

			sort( vSongAndSteps.begin(), vSongAndSteps.end() );

			vector<SongAndSteps>::iterator toDelete = unique( vSongAndSteps.begin(), vSongAndSteps.end() );
			vSongAndSteps.erase(toDelete, vSongAndSteps.end());

			CHECKPOINT;
			for( unsigned i=0; i<vSongAndSteps.size(); i++ )
			{
				Song* pSong = vSongAndSteps[i].pSong;
				Steps* pSteps = vSongAndSteps[i].pSteps;

				// Find Machine Records
				{
					HighScoreList &hsl = PROFILEMAN->GetMachineProfile()->GetStepsHighScoreList(pSong,pSteps);
					for( unsigned j=0; j<hsl.vHighScores.size(); j++ )
					{
						HighScore &hs = hsl.vHighScores[j];

						if( hs.GetName() != RANKING_TO_FILL_IN_MARKER[pn] )
							continue;

						RankingFeat feat;
						feat.Type = RankingFeat::SONG;
						feat.pSong = pSong;
						feat.pSteps = pSteps;
						feat.Feat = ssprintf("MR #%d in %s %s", j+1, pSong->GetTranslitMainTitle().c_str(), DifficultyToString(pSteps->GetDifficulty()).c_str() );
						feat.pStringToFill = hs.GetNameMutable();
						feat.grade = hs.GetGrade();
						feat.fPercentDP = hs.GetPercentDP();
						feat.iScore = hs.GetScore();

						if( pSong->HasBanner() )
							feat.Banner = pSong->GetBannerPath();

						asFeatsOut.push_back( feat );
					}
				}
		
				// Find Personal Records
				if( pProf )
				{
					HighScoreList &hsl = pProf->GetStepsHighScoreList(pSong,pSteps);
					for( unsigned j=0; j<hsl.vHighScores.size(); j++ )
					{
						HighScore &hs = hsl.vHighScores[j];

						if( hs.GetName() != RANKING_TO_FILL_IN_MARKER[pn] )
							continue;

						RankingFeat feat;
						feat.pSong = pSong;
						feat.pSteps = pSteps;
						feat.Type = RankingFeat::SONG;
						feat.Feat = ssprintf("PR #%d in %s %s", j+1, pSong->GetTranslitMainTitle().c_str(), DifficultyToString(pSteps->GetDifficulty()).c_str() );
						feat.pStringToFill = hs.GetNameMutable();
						feat.grade = hs.GetGrade();
						feat.fPercentDP = hs.GetPercentDP();
						feat.iScore = hs.GetScore();

						// XXX: temporary hack
						if( pSong->HasBackground() )
							feat.Banner = pSong->GetBackgroundPath();
		//					if( pSong->HasBanner() )
		//						feat.Banner = pSong->GetBannerPath();

						asFeatsOut.push_back( feat );
					}
				}
			}

			CHECKPOINT;
			StageStats stats;
			STATSMAN->GetFinalEvalStageStats( stats );


			// Find Machine Category Records
			FOREACH_RankingCategory( rc )
			{
				HighScoreList &hsl = PROFILEMAN->GetMachineProfile()->GetCategoryHighScoreList( st, rc );
				for( unsigned j=0; j<hsl.vHighScores.size(); j++ )
				{
					HighScore &hs = hsl.vHighScores[j];
					if( hs.GetName() != RANKING_TO_FILL_IN_MARKER[pn] )
						continue;

					RankingFeat feat;
					feat.Type = RankingFeat::CATEGORY;
					feat.Feat = ssprintf("MR #%d in Type %c (%d)", j+1, 'A'+rc, stats.GetAverageMeter(pn) );
					feat.pStringToFill = hs.GetNameMutable();
					feat.grade = Grade_NoData;
					feat.iScore = hs.GetScore();
					feat.fPercentDP = hs.GetPercentDP();
					asFeatsOut.push_back( feat );
				}
			}

			// Find Personal Category Records
			FOREACH_RankingCategory( rc )
			{
				if( pProf )
				{
					HighScoreList &hsl = pProf->GetCategoryHighScoreList( st, rc );
					for( unsigned j=0; j<hsl.vHighScores.size(); j++ )
					{
						HighScore &hs = hsl.vHighScores[j];
						if( hs.GetName() != RANKING_TO_FILL_IN_MARKER[pn] )
							continue;

						RankingFeat feat;
						feat.Type = RankingFeat::CATEGORY;
						feat.Feat = ssprintf("PR #%d in Type %c (%d)", j+1, 'A'+rc, stats.GetAverageMeter(pn) );
						feat.pStringToFill = hs.GetNameMutable();
						feat.grade = Grade_NoData;
						feat.iScore = hs.GetScore();
						feat.fPercentDP = hs.GetPercentDP();
						asFeatsOut.push_back( feat );
					}
				}
			}
		}
		break;
	case PLAY_MODE_NONSTOP:
	case PLAY_MODE_ONI:
	case PLAY_MODE_ENDLESS:
		{
			CHECKPOINT;
			Course* pCourse = m_pCurCourse;
			ASSERT( pCourse );
			Trail *pTrail = m_pCurTrail[pn];
			ASSERT( pTrail );
			CourseDifficulty cd = pTrail->m_CourseDifficulty;

			// Find Machine Records
			{
				Profile* pProfile = PROFILEMAN->GetMachineProfile();
				HighScoreList &hsl = pProfile->GetCourseHighScoreList( pCourse, pTrail );
				for( unsigned i=0; i<hsl.vHighScores.size(); i++ )
				{
					HighScore &hs = hsl.vHighScores[i];
					if( hs.GetName() != RANKING_TO_FILL_IN_MARKER[pn] )
							continue;

					RankingFeat feat;
					feat.Type = RankingFeat::COURSE;
					feat.pCourse = pCourse;
					feat.Feat = ssprintf("MR #%d in %s", i+1, pCourse->GetDisplayFullTitle().c_str() );
					if( cd != DIFFICULTY_MEDIUM )
						feat.Feat += " " + CourseDifficultyToLocalizedString(cd);
					feat.pStringToFill = hs.GetNameMutable();
					feat.grade = Grade_NoData;
					feat.iScore = hs.GetScore();
					feat.fPercentDP = hs.GetPercentDP();
					if( pCourse->HasBanner() )
						feat.Banner = pCourse->m_sBannerPath;
					asFeatsOut.push_back( feat );
				}
			}

			// Find Personal Records
			if( PROFILEMAN->IsPersistentProfile( pn ) )
			{
				HighScoreList &hsl = pProf->GetCourseHighScoreList( pCourse, pTrail );
				for( unsigned i=0; i<hsl.vHighScores.size(); i++ )
				{
					HighScore& hs = hsl.vHighScores[i];
					if( hs.GetName() != RANKING_TO_FILL_IN_MARKER[pn] )
							continue;

					RankingFeat feat;
					feat.Type = RankingFeat::COURSE;
					feat.pCourse = pCourse;
					feat.Feat = ssprintf("PR #%d in %s", i+1, pCourse->GetDisplayFullTitle().c_str() );
					feat.pStringToFill = hs.GetNameMutable();
					feat.grade = Grade_NoData;
					feat.iScore = hs.GetScore();
					feat.fPercentDP = hs.GetPercentDP();
					if( pCourse->HasBanner() )
						feat.Banner = pCourse->m_sBannerPath;
					asFeatsOut.push_back( feat );
				}
			}
		}
		break;
	default:
		ASSERT(0);
	}
}

bool GameState::AnyPlayerHasRankingFeats() const
{
	vector<RankingFeat> vFeats;
	FOREACH_PlayerNumber( p )
	{
		GetRankingFeats( p, vFeats );
		if( !vFeats.empty() )
			return true;
	}
	return false;
}

void GameState::StoreRankingName( PlayerNumber pn, RString sName )
{
	sName.MakeUpper();

	if( USE_NAME_BLACKLIST )
	{
		RageFile file;
		if( file.Open(NAME_BLACKLIST_FILE) )
		{
			RString sLine;
			
			while( !file.AtEOF() )
			{
				if( file.GetLine(sLine) == -1 )
				{
					LOG->Warn( "Error reading \"%s\": %s", NAME_BLACKLIST_FILE, file.GetError().c_str() );
					break;
				}

				sLine.MakeUpper();
				if( !sLine.empty() && sName.find(sLine) != string::npos )	// name contains a bad word
				{
					LOG->Trace( "entered '%s' matches blacklisted item '%s'", sName.c_str(), sLine.c_str() );
					sName = "";
					break;
				}
			}
		}
	}

	vector<RankingFeat> aFeats;
	GetRankingFeats( pn, aFeats );

	for( unsigned i=0; i<aFeats.size(); i++ )
	{
		*aFeats[i].pStringToFill = sName;

		// save name pointers as we fill them
		m_vpsNamesThatWereFilled.push_back( aFeats[i].pStringToFill );
	}


	Profile *pProfile = PROFILEMAN->GetMachineProfile();

	if( !PREFSMAN->m_bAllowMultipleHighScoreWithSameName )
	{
		//
		// erase all but the highest score for each name
		//
		FOREACHM( SongID, Profile::HighScoresForASong, pProfile->m_SongHighScores, iter )
			FOREACHM( StepsID, Profile::HighScoresForASteps, iter->second.m_StepsHighScores, iter2 )
				iter2->second.hsl.RemoveAllButOneOfEachName();

		FOREACHM( CourseID, Profile::HighScoresForACourse, pProfile->m_CourseHighScores, iter )
			FOREACHM( TrailID, Profile::HighScoresForATrail, iter->second.m_TrailHighScores, iter2 )
				iter2->second.hsl.RemoveAllButOneOfEachName();
	}

	//
	// clamp high score sizes
	//
	FOREACHM( SongID, Profile::HighScoresForASong, pProfile->m_SongHighScores, iter )
		FOREACHM( StepsID, Profile::HighScoresForASteps, iter->second.m_StepsHighScores, iter2 )
			iter2->second.hsl.ClampSize( true );

	FOREACHM( CourseID, Profile::HighScoresForACourse, pProfile->m_CourseHighScores, iter )
		FOREACHM( TrailID, Profile::HighScoresForATrail, iter->second.m_TrailHighScores, iter2 )
			iter2->second.hsl.ClampSize( true );
}

bool GameState::AllAreInDangerOrWorse() const
{
	FOREACH_EnabledPlayer( p )
		if( m_pPlayerState[p]->m_HealthState < PlayerState::DANGER )
			return false;
	return true;
}

bool GameState::AllAreDead() const
{
	FOREACH_EnabledPlayer( p )
		if( m_pPlayerState[p]->m_HealthState < PlayerState::DEAD )
			return false;
	return true;
}

bool GameState::AllHumanHaveComboOf30OrMoreMisses() const
{
	FOREACH_HumanPlayer( p )
		if( STATSMAN->m_CurStageStats.m_player[p].iCurMissCombo < 30 )
			return false;
	return true;
}

bool GameState::OneIsHot() const
{
	FOREACH_EnabledPlayer( p )
		if( m_pPlayerState[p]->m_HealthState == PlayerState::HOT )
			return true;
	return false;
}

bool GameState::IsTimeToPlayAttractSounds() const
{
	// m_iNumTimesThroughAttract will be -1 from the first attract screen after 
	// the end of a game until the next time FIRST_ATTRACT_SCREEN is reached.
	// Play attract sounds for this sort span of time regardless of 
	// m_AttractSoundFrequency because it's awkward to have the machine go 
	// silent immediately after the end of a game.
	if( m_iNumTimesThroughAttract == -1 )
		return true;

	if( PREFSMAN->m_AttractSoundFrequency == PrefsManager::ASF_NEVER )
		return false;

	// play attract sounds once every m_iAttractSoundFrequency times through
	if( (m_iNumTimesThroughAttract % PREFSMAN->m_AttractSoundFrequency)==0 )
		return true;

	return false;
}

void GameState::VisitAttractScreen( const RString sScreenName )
{
	if( sScreenName == CommonMetrics::FIRST_ATTRACT_SCREEN.GetValue() )
		m_iNumTimesThroughAttract++;
}

bool GameState::DifficultiesLocked()
{
 	if( GAMESTATE->m_PlayMode == PLAY_MODE_RAVE )
		return true;
	if( IsCourseMode() )
		return PREFSMAN->m_bLockCourseDifficulties;
 	if( GAMESTATE->GetCurrentStyle()->m_bLockDifficulties )
		return true;
	return false;
}

bool GameState::ChangePreferredDifficulty( PlayerNumber pn, Difficulty dc )
{
	m_PreferredDifficulty[pn].Set( dc );
	if( DifficultiesLocked() )
		FOREACH_PlayerNumber( p )
			if( p != pn )
				m_PreferredDifficulty[p].Set( m_PreferredDifficulty[pn] );

	return true;
}

/* When only displaying difficulties in DIFFICULTIES_TO_SHOW, use GetClosestShownDifficulty
 * to find which difficulty to show, and ChangePreferredDifficulty(pn, dir) to change
 * difficulty. */
bool GameState::ChangePreferredDifficulty( PlayerNumber pn, int dir )
{
	const vector<Difficulty> &v = CommonMetrics::DIFFICULTIES_TO_SHOW.GetValue();

	Difficulty d = GAMESTATE->GetClosestShownDifficulty(pn);
	while( 1 )
	{
		d = (Difficulty)(d+dir);
		if( d < 0 || d >= NUM_Difficulty )
			return false;
		if( find(v.begin(), v.end(), d) != v.end() )
			break; // found
	}

	return ChangePreferredDifficulty( pn, d );
}

/* The user may be set to prefer a difficulty that isn't always shown; typically,
 * DIFFICULTY_EDIT.  Return the closest shown difficulty <= m_PreferredDifficulty. */
Difficulty GameState::GetClosestShownDifficulty( PlayerNumber pn ) const
{
	const vector<Difficulty> &v = CommonMetrics::DIFFICULTIES_TO_SHOW.GetValue();

	Difficulty iClosest = (Difficulty) 0;
	int iClosestDist = -1;
	FOREACH_CONST( Difficulty, v, dc )
	{
		int iDist = m_PreferredDifficulty[pn] - *dc;
		if( iDist < 0 )
			continue;
		if( iClosestDist != -1 && iDist > iClosestDist )
			continue;
		iClosestDist = iDist;
		iClosest = *dc;
	}

	return iClosest;
}

bool GameState::ChangePreferredCourseDifficulty( PlayerNumber pn, CourseDifficulty cd )
{
	m_PreferredCourseDifficulty[pn].Set( cd );

	if( PREFSMAN->m_bLockCourseDifficulties )
		FOREACH_PlayerNumber( p )
			if( p != pn )
				m_PreferredCourseDifficulty[p].Set( m_PreferredCourseDifficulty[pn] );

	return true;
}

bool GameState::ChangePreferredCourseDifficulty( PlayerNumber pn, int dir )
{
	/* If we have a course selected, only choose among difficulties available in the course. */
	const Course *pCourse = this->m_pCurCourse;

	const vector<CourseDifficulty> &v = CommonMetrics::COURSE_DIFFICULTIES_TO_SHOW.GetValue();

	CourseDifficulty cd = m_PreferredCourseDifficulty[pn];
	while( 1 )
	{
		cd = (CourseDifficulty)(cd+dir);
		if( cd < 0 || cd >= NUM_Difficulty )
			return false;
		if( find(v.begin(),v.end(),cd) == v.end() )
			continue; /* not available */
		if( !pCourse || pCourse->GetTrail( GAMESTATE->GetCurrentStyle()->m_StepsType, cd ) )
			break;
	}

	return ChangePreferredCourseDifficulty( pn, cd );
}

bool GameState::IsCourseDifficultyShown( CourseDifficulty cd )
{
	const vector<CourseDifficulty> &v = CommonMetrics::COURSE_DIFFICULTIES_TO_SHOW.GetValue();
	return find(v.begin(), v.end(), cd) != v.end();
}

Difficulty GameState::GetEasiestStepsDifficulty() const
{
	Difficulty dc = DIFFICULTY_INVALID;
	FOREACH_HumanPlayer( p )
	{
		if( this->m_pCurSteps[p] == NULL )
		{
			LOG->Warn( "GetEasiestNotesDifficulty called but p%i hasn't chosen notes", p+1 );
			continue;
		}
		dc = min( dc, this->m_pCurSteps[p]->GetDifficulty() );
	}
	return dc;
}

bool GameState::IsEventMode() const
{
	return m_bTemporaryEventMode || PREFSMAN->m_bEventMode; 
}

CoinMode GameState::GetCoinMode()
{
	if( IsEventMode() && PREFSMAN->m_CoinMode == COIN_MODE_PAY )
		return COIN_MODE_FREE; 
	else 
		return PREFSMAN->m_CoinMode; 
}

Premium	GameState::GetPremium() 
{ 
	if( IsEventMode() ) 
		return PREMIUM_NONE; 
	else 
		return PREFSMAN->m_Premium; 
}

bool GameState::IsPlayerInDanger( const PlayerState *pPlayerState ) const
{
	if( GAMESTATE->GetPlayerFailType(pPlayerState) == SongOptions::FAIL_OFF )
		return false;
	if( !PREFSMAN->m_bShowDanger )
		return false;
	return pPlayerState->m_HealthState == PlayerState::DANGER;
}

bool GameState::IsPlayerDead( const PlayerState *pPlayerState ) const
{
	if( GAMESTATE->GetPlayerFailType(pPlayerState) == SongOptions::FAIL_OFF )
		return false;
	return pPlayerState->m_HealthState == PlayerState::DEAD;
}

float GameState::GetGoalPercentComplete( PlayerNumber pn )
{
	const Profile *pProfile = PROFILEMAN->GetProfile(pn);
	const StageStats &ssAccum = STATSMAN->GetAccumPlayedStageStats();
	const StageStats &ssCurrent = STATSMAN->m_CurStageStats;
	const PlayerStageStats &pssAccum = ssAccum.m_player[pn];
	const PlayerStageStats &pssCurrent = ssCurrent.m_player[pn];

	float fActual = 0;
	float fGoal = 0;
	switch( pProfile->m_GoalType )
	{
	case GoalType_Calories:
		fActual = pssAccum.fCaloriesBurned + pssCurrent.fCaloriesBurned;
		fGoal = (float)pProfile->m_iGoalCalories;
		break;
	case GoalType_Time:
		fActual = ssAccum.fGameplaySeconds + ssCurrent.fGameplaySeconds;
		fGoal = (float)pProfile->m_iGoalSeconds;
		break;
	case GoalType_None:
		return 0;	// never complete
	default:
		ASSERT(0);
	}
	if( fGoal == 0 )
		return 0;
	else
		return fActual / fGoal;
}

bool GameState::PlayerIsUsingModifier( PlayerNumber pn, const RString &sModifier )
{
	PlayerOptions po = GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions;
	SongOptions so = GAMESTATE->m_SongOptions;
	po.FromString( sModifier );
	so.FromString( sModifier );

	return po == GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions && so == GAMESTATE->m_SongOptions;
}

Profile* GameState::GetEditLocalProfile()
{
	if( m_sEditLocalProfileID.Get().empty() )
		return NULL;
	return PROFILEMAN->GetLocalProfile( m_sEditLocalProfileID );
}


// lua start
#include "LuaBinding.h"
#include "Game.h"

class LunaGameState: public Luna<GameState>
{
public:
	LunaGameState() { LUA->Register( Register ); }

	DEFINE_METHOD( IsPlayerEnabled,			IsPlayerEnabled((PlayerNumber)IArg(1)) )
	DEFINE_METHOD( IsHumanPlayer,			IsHumanPlayer((PlayerNumber)IArg(1)) )
	DEFINE_METHOD( GetPlayerDisplayName,		GetPlayerDisplayName((PlayerNumber)IArg(1)) )
	DEFINE_METHOD( GetMasterPlayerNumber,		m_MasterPlayerNumber )
	DEFINE_METHOD( GetMultiplayer,			m_bMultiplayer )
	static int GetPlayerState( T* p, lua_State *L )
	{ 
		PlayerNumber pn = (PlayerNumber)IArg(1);
		p->m_pPlayerState[pn]->PushSelf(L);
		return 1;
	}
	static int GetMultiPlayerState( T* p, lua_State *L )
	{ 
		MultiPlayer mp = (MultiPlayer)IArg(1);
		p->m_pMultiPlayerState[mp]->PushSelf(L);
		return 1;
	}
	static int ApplyGameCommand( T* p, lua_State *L )
	{
		PlayerNumber pn = PLAYER_INVALID;
		if( lua_gettop(L) >= 2 && !lua_isnil(L,2) )
			pn = (PlayerNumber)IArg(2);
		p->ApplyGameCommand(SArg(1),pn);
		return 0;
	}
	static int GetCurrentSong( T* p, lua_State *L )			{ if(p->m_pCurSong) p->m_pCurSong->PushSelf(L); else lua_pushnil(L); return 1; }
	static int SetCurrentSong( T* p, lua_State *L )
	{ 
		if( lua_isnil(L,1) ) { p->m_pCurSong.Set( NULL ); }
		else { Song *pS = Luna<Song>::check(L,1); p->m_pCurSong.Set( pS ); }
		return 0;
	}
	static int GetCurrentSteps( T* p, lua_State *L )
	{
		PlayerNumber pn = (PlayerNumber)IArg(1);
		Steps *pSteps = p->m_pCurSteps[pn];  
		if( pSteps ) { pSteps->PushSelf(L); }
		else		 { lua_pushnil(L); }
		return 1;
	}
	static int SetCurrentSteps( T* p, lua_State *L )
	{ 
		PlayerNumber pn = (PlayerNumber)IArg(1);
		if( lua_isnil(L,2) )	{ p->m_pCurSteps[pn].Set( NULL ); }
		else					{ Steps *pS = Luna<Steps>::check(L,2); p->m_pCurSteps[pn].Set( pS ); }
		MESSAGEMAN->Broadcast( (Message)(Message_CurrentStepsP1Changed+pn) );
		return 0;
	}
	static int GetCurrentCourse( T* p, lua_State *L )		{ if(p->m_pCurCourse) p->m_pCurCourse->PushSelf(L); else lua_pushnil(L); return 1; }
	static int SetCurrentCourse( T* p, lua_State *L )
	{ 
		if( lua_isnil(L,1) ) { p->m_pCurCourse.Set( NULL ); }
		else { Course *pC = Luna<Course>::check(L,1); p->m_pCurCourse.Set( pC ); }
		return 0;
	}
	static int GetCurrentTrail( T* p, lua_State *L )
	{
		PlayerNumber pn = (PlayerNumber)IArg(1);
		Trail *pTrail = p->m_pCurTrail[pn];  
		if( pTrail ) { pTrail->PushSelf(L); }
		else		 { lua_pushnil(L); }
		return 1;
	}
	static int SetCurrentTrail( T* p, lua_State *L )
	{ 
		PlayerNumber pn = (PlayerNumber)IArg(1);
		if( lua_isnil(L,2) )	{ p->m_pCurTrail[pn].Set( NULL ); }
		else					{ Trail *pS = Luna<Trail>::check(L,2); p->m_pCurTrail[pn].Set( pS ); }
		MESSAGEMAN->Broadcast( (Message)(Message_CurrentTrailP1Changed+pn) );
		return 0;
	}
	static int GetPreferredSong( T* p, lua_State *L )		{ if(p->m_pPreferredSong) p->m_pPreferredSong->PushSelf(L); else lua_pushnil(L); return 1; }
	static int SetPreferredSong( T* p, lua_State *L )
	{
		if( lua_isnil(L,1) ) { p->m_pPreferredSong = NULL; }
		else { Song *pS = Luna<Song>::check(L,1); p->m_pPreferredSong = pS; }
		return 0;
	}
	static int SetTemporaryEventMode( T* p, lua_State *L )	{ p->m_bTemporaryEventMode = BArg(1); return 0; }
	static int Env( T* p, lua_State *L )	{ p->m_Environment->PushSelf(L); return 1; }
	static int SetEnv( T* p, lua_State *L )
	{
		int iTop = lua_gettop(L);
		p->m_Environment->PushSelf(L);
		lua_pushvalue( L, iTop-1 );
		lua_pushvalue( L, iTop );
		lua_settable( L, -3 );
		return 0;
	}
	static int GetEnv( T* p, lua_State *L )
	{
		int iTop = lua_gettop(L);
		p->m_Environment->PushSelf(L);
		lua_pushvalue( L, iTop );
		lua_gettable( L, -2 );
		return 1;
	}
	static int GetEditSourceSteps( T* p, lua_State *L )
	{
		Steps *pSteps = p->m_pEditSourceSteps;  
		if( pSteps ) { pSteps->PushSelf(L); }
		else		 { lua_pushnil(L); }
		return 1;
	}
	static int GetPreferredDifficulty( T* p, lua_State *L )		{ lua_pushnumber(L, p->m_PreferredDifficulty[IArg(1)] ); return 1; }
	static int AnyPlayerHasRankingFeats( T* p, lua_State *L )	{ lua_pushboolean(L, p->AnyPlayerHasRankingFeats() ); return 1; }
	static int IsCourseMode( T* p, lua_State *L )			{ lua_pushboolean(L, p->IsCourseMode() ); return 1; }
	static int IsDemonstration( T* p, lua_State *L )		{ lua_pushboolean(L, p->m_bDemonstrationOrJukebox ); return 1; }
	static int GetPlayMode( T* p, lua_State *L )			{ lua_pushnumber(L, p->m_PlayMode ); return 1; }
	static int GetSortOrder( T* p, lua_State *L )			{ lua_pushnumber(L, p->m_SortOrder ); return 1; }
	static int StageIndex( T* p, lua_State *L )			{ lua_pushnumber(L, p->GetStageIndex() ); return 1; }
	static int IsGoalComplete( T* p, lua_State *L )			{ lua_pushboolean(L, p->IsGoalComplete((PlayerNumber)IArg(1)) ); return 1; }
	static int PlayerIsUsingModifier( T* p, lua_State *L )		{ lua_pushboolean(L, p->PlayerIsUsingModifier((PlayerNumber)IArg(1),SArg(2)) ); return 1; }
	static int GetCourseSongIndex( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetCourseSongIndex() ); return 1; }
	static int GetLoadingCourseSongIndex( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetLoadingCourseSongIndex() ); return 1; }
	static int IsFinalStage( T* p, lua_State *L )			{ lua_pushboolean(L, p->IsFinalStage() ); return 1; }
	static int IsExtraStage( T* p, lua_State *L )			{ lua_pushboolean(L, p->IsExtraStage() ); return 1; }
	static int IsExtraStage2( T* p, lua_State *L )			{ lua_pushboolean(L, p->IsExtraStage2() ); return 1; }
	static int GetCurrentStage( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetCurrentStage() ); return 1; }
	static int IsStagePossible( T* p, lua_State *L )		{ lua_pushboolean(L, p->IsStagePossible((Stage)IArg(1)) ); return 1; }
	static int HasEarnedExtraStage( T* p, lua_State *L )		{ lua_pushboolean(L, p->HasEarnedExtraStage() ); return 1; }
	static int GetEasiestStepsDifficulty( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetEasiestStepsDifficulty() ); return 1; }
	static int IsEventMode( T* p, lua_State *L )			{ lua_pushboolean(L, p->IsEventMode() ); return 1; }
	static int GetNumPlayersEnabled( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetNumPlayersEnabled() ); return 1; }
	static int GetSongBeat( T* p, lua_State *L )			{ lua_pushnumber(L, p->m_fSongBeat ); return 1; }
	static int GetGameplayLeadIn( T* p, lua_State *L )		{ lua_pushnumber(L, p->m_bGameplayLeadIn ); return 1; }
	static int PlayerUsingBothSides( T* p, lua_State *L )		{ lua_pushboolean(L, p->PlayerUsingBothSides() ); return 1; }
	static int GetCoins( T* p, lua_State *L )			{ lua_pushnumber(L, p->m_iCoins ); return 1; }
	static int IsSideJoined( T* p, lua_State *L )			{ lua_pushboolean(L, p->m_bSideIsJoined[(PlayerNumber)IArg(1)] ); return 1; }
	static int GetCoinsNeededToJoin( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetCoinsNeededToJoin() ); return 1; }
	static int PlayersCanJoin( T* p, lua_State *L )			{ lua_pushboolean(L, p->PlayersCanJoin() ); return 1; }
	static int GetNumSidesJoined( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetNumSidesJoined() ); return 1; }
	static int GetCoinMode( T* p, lua_State *L )			{ lua_pushnumber(L, p->GetCoinMode() ); return 1; }
	static int GetPremium( T* p, lua_State *L )			{ lua_pushnumber(L, p->GetPremium() ); return 1; }
	static int GetSongOptionsString( T* p, lua_State *L )		{ lua_pushstring(L, p->m_SongOptions.GetString() ); return 1; }
	static int IsWinner( T* p, lua_State *L )
	{
		PlayerNumber pn = (PlayerNumber)IArg(1);
		lua_pushboolean(L, p->GetStageResult(pn)==RESULT_WIN); return 1;
	}
	static int IsDraw( T* p, lua_State *L )
	{
		lua_pushboolean(L, p->GetStageResult(PLAYER_1)==RESULT_DRAW); return 1;
	}
	static int IsDisqualified( T* p, lua_State *L )
	{
		PlayerNumber pn = (PlayerNumber)IArg(1);
		lua_pushboolean(L, p->IsDisqualified(pn)); return 1;
	}
	static int GetCurrentGame( T* p, lua_State *L )			{ const_cast<Game*>(p->GetCurrentGame())->PushSelf( L ); return 1; }
	static int GetEditCourseEntryIndex( T* p, lua_State *L )	{ lua_pushnumber(L, p->m_iEditCourseEntryIndex ); return 1; }
	static int GetEditLocalProfileID( T* p, lua_State *L )		{ lua_pushstring(L, p->m_sEditLocalProfileID.Get() ); return 1; }
	static int GetEditLocalProfile( T* p, lua_State *L )
	{
		Profile *pProfile = p->GetEditLocalProfile();
		if( pProfile )
			pProfile->PushSelf(L);
		else
			lua_pushnil( L );
		return 1;
	}

	static int GetCurrentStepsCredits( T* p, lua_State *L )
	{
		const Song* pSong = p->m_pCurSong;
		if( pSong == NULL )
			return 0;

		// use a vector and not a set so that ordering is maintained
		vector<const Steps*> vpStepsToShow;
		FOREACH_HumanPlayer( p )
		{
			const Steps* pSteps = GAMESTATE->m_pCurSteps[p];
			if( pSteps == NULL )
				return 0;
			bool bAlreadyAdded = find( vpStepsToShow.begin(), vpStepsToShow.end(), pSteps ) != vpStepsToShow.end();
			if( !bAlreadyAdded )
				vpStepsToShow.push_back( pSteps );
		}

		for( unsigned i=0; i<vpStepsToShow.size(); i++ )
		{
			const Steps* pSteps = vpStepsToShow[i];
			RString sDifficulty = DifficultyToLocalizedString( pSteps->GetDifficulty() );
			
			lua_pushstring( L, sDifficulty );
			lua_pushstring( L, pSteps->GetDescription() );
		}

		return vpStepsToShow.size()*2;
	}
	static int GetCurrentStyleType( T* p, lua_State *L )
	{
		lua_pushstring( L, StyleTypeToString(p->GetCurrentStyle()->m_StyleType) );
		return 1;
	}


	static void Register(lua_State *L)
	{
		ADD_METHOD( IsPlayerEnabled );
		ADD_METHOD( IsHumanPlayer );
		ADD_METHOD( GetPlayerDisplayName );
		ADD_METHOD( GetMasterPlayerNumber );
		ADD_METHOD( GetMultiplayer );
		ADD_METHOD( GetPlayerState );
		ADD_METHOD( GetMultiPlayerState );
		ADD_METHOD( ApplyGameCommand );
		ADD_METHOD( GetCurrentSong );
		ADD_METHOD( SetCurrentSong );
		ADD_METHOD( GetCurrentSteps );
		ADD_METHOD( SetCurrentSteps );
		ADD_METHOD( GetCurrentCourse );
		ADD_METHOD( SetCurrentCourse );
		ADD_METHOD( GetCurrentTrail );
		ADD_METHOD( SetCurrentTrail );
		ADD_METHOD( SetPreferredSong );
		ADD_METHOD( GetPreferredSong );
		ADD_METHOD( SetTemporaryEventMode );
		ADD_METHOD( Env );
		ADD_METHOD( SetEnv );
		ADD_METHOD( GetEnv );
		ADD_METHOD( GetEditSourceSteps );
		ADD_METHOD( GetPreferredDifficulty );
		ADD_METHOD( AnyPlayerHasRankingFeats );
		ADD_METHOD( IsCourseMode );
		ADD_METHOD( IsDemonstration );
		ADD_METHOD( GetPlayMode );
		ADD_METHOD( GetSortOrder );
		ADD_METHOD( StageIndex );
		ADD_METHOD( IsGoalComplete );
		ADD_METHOD( PlayerIsUsingModifier );
		ADD_METHOD( GetCourseSongIndex );
		ADD_METHOD( GetLoadingCourseSongIndex );
		ADD_METHOD( IsFinalStage );
		ADD_METHOD( IsExtraStage );
		ADD_METHOD( IsExtraStage2 );
		ADD_METHOD( GetCurrentStage );
		ADD_METHOD( IsStagePossible );
		ADD_METHOD( HasEarnedExtraStage );
		ADD_METHOD( GetEasiestStepsDifficulty );
		ADD_METHOD( IsEventMode );
		ADD_METHOD( GetNumPlayersEnabled );
		ADD_METHOD( GetSongBeat );
		ADD_METHOD( GetGameplayLeadIn );
		ADD_METHOD( PlayerUsingBothSides );
		ADD_METHOD( GetCoins );
		ADD_METHOD( IsSideJoined );
		ADD_METHOD( GetCoinsNeededToJoin );
		ADD_METHOD( PlayersCanJoin );
		ADD_METHOD( GetNumSidesJoined );
		ADD_METHOD( GetCoinMode );
		ADD_METHOD( GetPremium );
		ADD_METHOD( GetSongOptionsString );
		ADD_METHOD( IsWinner );
		ADD_METHOD( IsDraw );
		ADD_METHOD( IsDisqualified );
		ADD_METHOD( GetCurrentGame );
		ADD_METHOD( GetEditCourseEntryIndex );
		ADD_METHOD( GetEditLocalProfileID );
		ADD_METHOD( GetEditLocalProfile );
		ADD_METHOD( GetCurrentStepsCredits );
		ADD_METHOD( GetCurrentStyleType );

		Luna<T>::Register( L );

		// Add global singleton if constructed already.  If it's not constructed yet,
		// then we'll register it later when we reinit Lua just before 
		// initializing the display.
		if( GAMESTATE )
		{
			lua_pushstring(L, "GAMESTATE");
			GAMESTATE->PushSelf( L );
			lua_settable(L, LUA_GLOBALSINDEX);
		}
	}
};

LUA_REGISTER_CLASS( GameState )
// lua end

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
