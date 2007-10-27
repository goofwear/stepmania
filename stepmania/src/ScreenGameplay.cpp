#include "global.h"
#include "ScreenGameplay.h"
#include "SongManager.h"
#include "ScreenManager.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "GamePreferences.h"
#include "GameManager.h"
#include "RageFileManager.h"
#include "Steps.h"
#include "RageLog.h"
#include "LifeMeter.h"
#include "GameState.h"
#include "ScoreDisplayNormal.h"
#include "ScoreDisplayPercentage.h"
#include "ScoreDisplayLifeTime.h"
#include "ScoreDisplayOni.h"
#include "ScoreDisplayRave.h"
#include "ThemeManager.h"
#include "RageTimer.h"
#include "ScoreKeeperNormal.h"
#include "ScoreKeeperRave.h"
#include "LyricsLoader.h"
#include "ActorUtil.h"
#include "RageSoundManager.h"
#include "RageSoundReader.h"
#include "RageTextureManager.h"
#include "GameSoundManager.h"
#include "CombinedLifeMeterTug.h"
#include "Inventory.h"
#include "Course.h"
#include "NoteDataUtil.h"
#include "UnlockManager.h"
#include "LightsManager.h"
#include "ProfileManager.h"
#include "StatsManager.h"
#include "PlayerAI.h"	// for NUM_SKILL_LEVELS
#include "NetworkSyncManager.h"
#include "Foreach.h"
#include "DancingCharacters.h"
#include "ScreenDimensions.h"
#include "ThemeMetric.h"
#include "PlayerState.h"
#include "Style.h"
#include "LuaManager.h"
#include "MemoryCardManager.h"
#include "CommonMetrics.h"
#include "InputMapper.h"
#include "Game.h"
#include "ActiveAttackList.h"
#include "Player.h"
#include "DifficultyIcon.h"
#include "DifficultyMeter.h"
#include "XmlFile.h"
#include "Background.h"
#include "Foreground.h"
#include "ScreenSaveSync.h"
#include "AdjustSync.h"
#include "SongUtil.h"
#include "song.h"
#include "XmlFileUtil.h"

//
// Defines
//
#define SHOW_LIFE_METER_FOR_DISABLED_PLAYERS	THEME->GetMetricB(m_sName,"ShowLifeMeterForDisabledPlayers")
#define SHOW_SCORE_IN_RAVE			THEME->GetMetricB(m_sName,"ShowScoreInRave")
#define SONG_POSITION_METER_WIDTH		THEME->GetMetricF(m_sName,"SongPositionMeterWidth")
#define PLAYER_X( sName, styleType )		THEME->GetMetricF(m_sName,ssprintf("Player%s%sX",sName.c_str(),StyleTypeToString(styleType).c_str()))
#define STOP_COURSE_EARLY			THEME->GetMetricB(m_sName,"StopCourseEarly")	// evaluate this every time it's used

static ThemeMetric<float> INITIAL_BACKGROUND_BRIGHTNESS	("ScreenGameplay","InitialBackgroundBrightness");
static ThemeMetric<float> SECONDS_BETWEEN_COMMENTS	("ScreenGameplay","SecondsBetweenComments");
static ThemeMetric<RString> SCORE_KEEPER_CLASS		("ScreenGameplay","ScoreKeeperClass");

AutoScreenMessage( SM_PlayGo )

// received while STATE_DANCING
AutoScreenMessage( SM_LoadNextSong )
AutoScreenMessage( SM_StartLoadingNextSong )

// received while STATE_OUTRO
AutoScreenMessage( SM_DoPrevScreen )
AutoScreenMessage( SM_DoNextScreen )

// received while STATE_INTRO
AutoScreenMessage( SM_StartHereWeGo )
AutoScreenMessage( SM_StopHereWeGo )

AutoScreenMessage( SM_BattleTrickLevel1 );
AutoScreenMessage( SM_BattleTrickLevel2 );
AutoScreenMessage( SM_BattleTrickLevel3 );

static Preference<bool> g_bCenter1Player( "Center1Player", false );
static Preference<bool> g_bShowLyrics( "ShowLyrics", true );
static Preference<float> g_fNetStartOffset( "NetworkStartOffset", -3.0 );
static Preference<bool> g_bEasterEggs( "EasterEggs", true );


PlayerInfo::PlayerInfo()
{
	m_pn = PLAYER_INVALID;
	m_mp = MultiPlayer_Invalid;
	m_bIsDummy = false;
	m_iDummyIndex = 0;
	m_iAddToDifficulty = 0;
	m_pLifeMeter = NULL;
	m_ptextCourseSongNumber = NULL;
	m_ptextStepsDescription = NULL;
	m_pPrimaryScoreDisplay = NULL;
	m_pSecondaryScoreDisplay = NULL;
	m_pPrimaryScoreKeeper = NULL;
	m_pSecondaryScoreKeeper = NULL;
	m_ptextPlayerOptions = NULL;
	m_pActiveAttackList = NULL;
	m_pPlayer = NULL;
	m_pInventory = NULL;
	m_pDifficultyIcon = NULL;
}

void PlayerInfo::Load( PlayerNumber pn, MultiPlayer mp, bool bShowNoteField, int iAddToDifficulty )
{
	m_pn = pn;
	m_mp = mp;
	m_bPlayerEnabled = IsEnabled();
	m_bIsDummy = false;
	m_iAddToDifficulty = iAddToDifficulty;
	m_pLifeMeter = NULL;
	m_ptextCourseSongNumber = NULL;
	m_ptextStepsDescription = NULL;

	if( !IsMultiPlayer() )
	{
		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_REGULAR:
		case PLAY_MODE_NONSTOP:
		case PLAY_MODE_BATTLE:
		case PLAY_MODE_RAVE:
			if( PREFSMAN->m_bPercentageScoring )
				m_pPrimaryScoreDisplay = new ScoreDisplayPercentage;
			else
				m_pPrimaryScoreDisplay = new ScoreDisplayNormal;
			break;
		case PLAY_MODE_ONI:
		case PLAY_MODE_ENDLESS:
			if( GAMESTATE->m_SongOptions.GetStage().m_LifeType == SongOptions::LIFE_TIME )
				m_pPrimaryScoreDisplay = new ScoreDisplayLifeTime;
			else
				m_pPrimaryScoreDisplay = new ScoreDisplayOni;
			break;
		default:
			ASSERT(0);
		}
	}
	
	PlayerState *const pPlayerState = GetPlayerState();
	PlayerStageStats *const pPlayerStageStats = GetPlayerStageStats();

	if(  m_pPrimaryScoreDisplay )
		m_pPrimaryScoreDisplay->Init( pPlayerState, pPlayerStageStats );

	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_RAVE:
		m_pSecondaryScoreDisplay = new ScoreDisplayRave;
		m_pSecondaryScoreDisplay->SetName( "ScoreDisplayRave" );
		break;
	}

	if( m_pSecondaryScoreDisplay )
		m_pSecondaryScoreDisplay->Init( pPlayerState, pPlayerStageStats );

	m_pPrimaryScoreKeeper = ScoreKeeper::MakeScoreKeeper( SCORE_KEEPER_CLASS, pPlayerState, pPlayerStageStats );

	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_RAVE:
		m_pSecondaryScoreKeeper = new ScoreKeeperRave( pPlayerState, pPlayerStageStats );
		break;
	}

	m_ptextPlayerOptions = NULL;
	m_pActiveAttackList = NULL;
	m_pPlayer = new Player( m_NoteData, bShowNoteField );
	m_pInventory = NULL;
	m_pDifficultyIcon = NULL;

	if( IsMultiPlayer() )
	{
		pPlayerState->m_PlayerOptions	= GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions;
	}
}

void PlayerInfo::LoadDummyP1( int iDummyIndex, int iAddToDifficulty )
{
	m_pn = PLAYER_1;
	m_bPlayerEnabled = IsEnabled();
	m_bIsDummy = true;
	m_iDummyIndex = iDummyIndex;
	m_iAddToDifficulty = iAddToDifficulty;

	// don't init any of the scoring objects
	m_pPlayer = new Player( m_NoteData, true );

	// PlayerOptions needs to be set now so that we load the correct NoteSkin.
	m_PlayerStateDummy = *GAMESTATE->m_pPlayerState[PLAYER_1];
}

PlayerInfo::~PlayerInfo()
{
	SAFE_DELETE( m_pLifeMeter );
	SAFE_DELETE( m_ptextCourseSongNumber );
	SAFE_DELETE( m_ptextStepsDescription );
	SAFE_DELETE( m_pPrimaryScoreDisplay );
	SAFE_DELETE( m_pSecondaryScoreDisplay );
	SAFE_DELETE( m_pPrimaryScoreKeeper );
	SAFE_DELETE( m_pSecondaryScoreKeeper );
	SAFE_DELETE( m_ptextPlayerOptions );
	SAFE_DELETE( m_pActiveAttackList );
	SAFE_DELETE( m_pPlayer );
	SAFE_DELETE( m_pInventory );
	SAFE_DELETE( m_pDifficultyIcon );
}

void PlayerInfo::ShowOniGameOver()
{
	m_sprOniGameOver->PlayCommand( "Die" );
}

PlayerState *PlayerInfo::GetPlayerState()
{
	if( m_bIsDummy )
		return &m_PlayerStateDummy;
	return IsMultiPlayer() ?
		GAMESTATE->m_pMultiPlayerState[ GetPlayerStateAndStageStatsIndex() ] :
		GAMESTATE->m_pPlayerState[ GetPlayerStateAndStageStatsIndex() ];
}

PlayerStageStats *PlayerInfo::GetPlayerStageStats()
{
	// multiplayer chooses the PlayerStageStats with the highest score on StageFinalized
	if( m_bIsDummy  ||  IsMultiPlayer() )
		return &m_PlayerStageStatsDummy;
	return &STATSMAN->m_CurStageStats.m_player[ GetPlayerStateAndStageStatsIndex() ];
}
				
bool PlayerInfo::IsEnabled()
{
	if( m_pn != PLAYER_INVALID )
		return GAMESTATE->IsPlayerEnabled( m_pn );
	if( m_mp != MultiPlayer_Invalid )
		return GAMESTATE->IsMultiPlayerEnabled( m_mp );
	else if( m_bIsDummy )
		return true;
	ASSERT( 0 );
	return false;
}

vector<PlayerInfo>::iterator 
GetNextEnabledPlayerInfo( vector<PlayerInfo>::iterator iter, vector<PlayerInfo> &v )
{
	for( ; iter != v.end(); ++iter )
	{
		if( !iter->m_bPlayerEnabled )
			continue;
		return iter;
	}
	return iter;
}

vector<PlayerInfo>::iterator 
GetNextEnabledPlayerInfoNotDummy( vector<PlayerInfo>::iterator iter, vector<PlayerInfo> &v )
{
	for( ; iter != v.end(); iter++ )
	{
		if( iter->m_bIsDummy )
			continue;
		if( !iter->m_bPlayerEnabled )
			continue;
		return iter;
	}
	return iter;
}

vector<PlayerInfo>::iterator
GetNextEnabledPlayerNumberInfo( vector<PlayerInfo>::iterator iter, vector<PlayerInfo> &v )
{
	for( ; iter != v.end(); ++iter )
	{
		if( iter->m_bIsDummy )
			continue;
		if( !iter->m_bPlayerEnabled )
			continue;
		if( iter->m_mp != MultiPlayer_Invalid )
			continue;
		return iter;
	}
	return iter;
}

vector<PlayerInfo>::iterator
GetNextPlayerNumberInfo( vector<PlayerInfo>::iterator iter, vector<PlayerInfo> &v )
{
	for( ; iter != v.end(); ++iter )
	{
		if( iter->m_bIsDummy )
			continue;
		if( iter->m_pn == PLAYER_INVALID )
			continue;
		return iter;
	}
	return iter;
}

vector<PlayerInfo>::iterator
GetNextVisiblePlayerInfo( vector<PlayerInfo>::iterator iter, vector<PlayerInfo> &v )
{
	for( ; iter != v.end(); ++iter )
	{
		if( !iter->m_pPlayer->HasVisibleParts() )
			continue;
		return iter;
	}
	return iter;
}


ScreenGameplay::ScreenGameplay()
{
	m_pSongBackground = NULL;
	m_pSongForeground = NULL;
}

void ScreenGameplay::Init()
{
	SubscribeToMessage( "Judgment" );

	PLAYER_TYPE.Load(			m_sName, "PlayerType" );
	PLAYER_INIT_COMMAND.Load(		m_sName, "PlayerInitCommand" );
	GIVE_UP_START_TEXT.Load(		m_sName, "GiveUpStartText" );
	GIVE_UP_BACK_TEXT.Load(			m_sName, "GiveUpBackText" );
	GIVE_UP_ABORTED_TEXT.Load(		m_sName, "GiveUpAbortedText" );
	MUSIC_FADE_OUT_SECONDS.Load(		m_sName, "MusicFadeOutSeconds" );
	OUT_TRANSITION_LENGTH.Load(		m_sName, "OutTransitionLength" );
	COURSE_TRANSITION_LENGTH.Load(		m_sName, "CourseTransitionLength" );
	BEGIN_FAILED_DELAY.Load(		m_sName, "BeginFailedDelay" );
	MIN_SECONDS_TO_STEP.Load(		m_sName, "MinSecondsToStep" );
	MIN_SECONDS_TO_MUSIC.Load(		m_sName, "MinSecondsToMusic" );
	MIN_SECONDS_TO_STEP_NEXT_SONG.Load(	m_sName, "MinSecondsToStepNextSong" );
	START_GIVES_UP.Load(			m_sName, "StartGivesUp" );
	BACK_GIVES_UP.Load(			m_sName, "BackGivesUp" );
	GIVING_UP_GOES_TO_PREV_SCREEN.Load(	m_sName, "GivingUpGoesToPrevScreen" );
	FAIL_ON_MISS_COMBO.Load(		m_sName, "FailOnMissCombo" );
	ALLOW_CENTER_1_PLAYER.Load(		m_sName, "AllowCenter1Player" );
	
	if( UseSongBackgroundAndForeground() )
	{
		m_pSongBackground = new Background;
		m_pSongForeground = new Foreground;
	}

	ScreenWithMenuElements::Init();

	this->FillPlayerInfo( m_vPlayerInfo );
	ASSERT_M( !m_vPlayerInfo.empty(), "m_vPlayerInfo must be filled by FillPlayerInfo" );

	/* Pause MEMCARDMAN.  If a memory card is removed, we don't want to interrupt the
	 * player by making a noise until the game finishes. */
	if( !GAMESTATE->m_bDemonstrationOrJukebox )
		MEMCARDMAN->PauseMountingThread();

	m_pSoundMusic = NULL;
	m_bPaused = false;

	m_pCombinedLifeMeter = NULL;

	if( GAMESTATE->m_pCurSong == NULL && GAMESTATE->m_pCurCourse == NULL )
		return;	// ScreenDemonstration will move us to the next screen.  We just need to survive for one update without crashing.

	/* Save settings to the profile now.  Don't do this on extra stages, since the
	 * user doesn't have full control; saving would force profiles to Difficulty_Hard
	 * and save over their default modifiers every time someone got an extra stage.
	 * Do this before course modifiers are set up. */
	if( !GAMESTATE->IsAnExtraStage() )
	{
		FOREACH_HumanPlayer( pn )
			GAMESTATE->SaveCurrentSettingsToProfile(pn);
	}

	/* Called once per stage (single song or single course). */
	GAMESTATE->BeginStage();




	// fill in difficulty of CPU players with that of the first human player
	FOREACH_PotentialCpuPlayer(p)
		GAMESTATE->m_pCurSteps[p].Set( GAMESTATE->m_pCurSteps[ GAMESTATE->GetFirstHumanPlayer() ] );

	/* Increment the course play count. */
	if( GAMESTATE->IsCourseMode() && !GAMESTATE->m_bDemonstrationOrJukebox )
		FOREACH_EnabledPlayer(p)
			PROFILEMAN->IncrementCoursePlayCount( GAMESTATE->m_pCurCourse, GAMESTATE->m_pCurTrail[p], p );

	STATSMAN->m_CurStageStats.m_Stage = GAMESTATE->GetCurrentStage();
	STATSMAN->m_CurStageStats.m_iStageIndex = GAMESTATE->m_iCurrentStageIndex;
	STATSMAN->m_CurStageStats.m_playMode = GAMESTATE->m_PlayMode;
	STATSMAN->m_CurStageStats.m_pStyle = GAMESTATE->GetCurrentStyle();

	/* Record combo rollover. */
	FOREACH_EnabledPlayerInfoNotDummy( m_vPlayerInfo, pi )
		pi->GetPlayerStageStats()->UpdateComboList( 0, true );

	m_DancingState = STATE_INTRO;

	// Set this in LoadNextSong()
	//m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		
	m_bZeroDeltaOnNextUpdate = false;


	if( m_pSongBackground )
	{
		m_pSongBackground->SetName( "SongBackground" );
		m_pSongBackground->SetDrawOrder( DRAW_ORDER_BEFORE_EVERYTHING );
		ActorUtil::LoadAllCommands( *m_pSongBackground, m_sName );
		this->AddChild( m_pSongBackground );
	}

	if( m_pSongForeground )
	{
		m_pSongForeground->SetName( "SongForeground" );
		m_pSongForeground->SetDrawOrder( DRAW_ORDER_OVERLAY+1 );	// on top of the overlay, but under transitions
		ActorUtil::LoadAllCommands( *m_pSongForeground, m_sName );
		this->AddChild( m_pSongForeground );
	}

	if( PREFSMAN->m_bShowBeginnerHelper )
	{
		m_BeginnerHelper.SetDrawOrder( DRAW_ORDER_BEFORE_EVERYTHING );
		m_BeginnerHelper.SetXY( SCREEN_CENTER_X, SCREEN_CENTER_Y );
		this->AddChild( &m_BeginnerHelper );
	}
	
	if( !GAMESTATE->m_bDemonstrationOrJukebox )	// only load if we're going to use it
	{
		m_Toasty.Load( THEME->GetPathB(m_sName,"toasty") );
		this->AddChild( &m_Toasty );
	}

	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		RString sName = ssprintf("Player%s", pi->GetName().c_str());
		pi->m_pPlayer->SetName( sName );
		// If pi->m_pn is set, then the player will be visible.  If not, then it's not 
		// visible and don't bother setting its position.

		float fPlayerX;
		if( GAMESTATE->m_bMultiplayer && !pi->m_bIsDummy )
		{
			fPlayerX = SCREEN_CENTER_X;
		}
		else
		{
			fPlayerX = PLAYER_X( pi->GetName(), GAMESTATE->GetCurrentStyle()->m_StyleType );
			if( Center1Player() )
				fPlayerX = SCREEN_CENTER_X;
		}

		pi->m_pPlayer->SetX( fPlayerX );
		pi->m_pPlayer->RunCommands( PLAYER_INIT_COMMAND );
		this->AddChild( pi->m_pPlayer );
		pi->m_pPlayer->PlayCommand( "On" );
	}
	
	FOREACH_EnabledPlayerInfoNotDummy( m_vPlayerInfo, pi )
	{
		if( pi->m_pPlayer->HasVisibleParts() )
		{
			pi->m_sprOniGameOver.Load( THEME->GetPathG(m_sName,"oni gameover") );
			pi->m_sprOniGameOver->SetName( ssprintf("OniGameOver%s",pi->GetName().c_str()) );
			LOAD_ALL_COMMANDS_AND_SET_XY( pi->m_sprOniGameOver );
			this->AddChild( pi->m_sprOniGameOver );
		}
	}

	m_NextSong.Load( THEME->GetPathB(m_sName,"next course song") );
	m_NextSong.SetDrawOrder( DRAW_ORDER_TRANSITIONS-1 );
	this->AddChild( &m_NextSong );

	bool bBattery = GAMESTATE->m_SongOptions.GetStage().m_LifeType==SongOptions::LIFE_BATTERY;

	//
	// Add LifeFrame
	//
	m_sprLifeFrame.Load( THEME->GetPathG(m_sName,bBattery?"oni life frame":"life frame") );
	m_sprLifeFrame->SetName( "LifeFrame" );
	LOAD_ALL_COMMANDS_AND_SET_XY( m_sprLifeFrame );
	this->AddChild( m_sprLifeFrame );

	//
	// Add combined life meter
	//
	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		m_pCombinedLifeMeter = new CombinedLifeMeterTug;
		m_pCombinedLifeMeter->SetName( "CombinedLife" );
		LOAD_ALL_COMMANDS_AND_SET_XY( *m_pCombinedLifeMeter );
		this->AddChild( m_pCombinedLifeMeter );		
		break;
	}

	//
	// Before the lifemeter loads, if Networking is required
	// we need to wait, so that there is no Dead On Start issues.
	// if you wait too long at the second checkpoint, you will
	// appear dead when you begin your game.
	//
	NSMAN->StartRequest(0); 



	//
	// Add individual life meter
	//
	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_REGULAR:
	case PLAY_MODE_ONI:
	case PLAY_MODE_NONSTOP:
	case PLAY_MODE_ENDLESS:
		FOREACH_PlayerNumberInfo( m_vPlayerInfo, pi )
		{
			if( !GAMESTATE->IsPlayerEnabled(pi->m_pn) && !SHOW_LIFE_METER_FOR_DISABLED_PLAYERS )
				continue;	// skip

			pi->m_pLifeMeter = LifeMeter::MakeLifeMeter( GAMESTATE->m_SongOptions.GetStage().m_LifeType );
			pi->m_pLifeMeter->Load( pi->GetPlayerState(), pi->GetPlayerStageStats() );
			pi->m_pLifeMeter->SetName( ssprintf("Life%s",pi->GetName().c_str()) );
			LOAD_ALL_COMMANDS_AND_SET_XY( pi->m_pLifeMeter );
			this->AddChild( pi->m_pLifeMeter );		
		}
		break;
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		break;
	}

	m_bShowScoreboard = false;

#if !defined(WITHOUT_NETWORKING)
	// Only used in SMLAN/SMOnline:
	if( NSMAN->useSMserver && GAMESTATE->GetCurrentStyle()->m_StyleType != StyleType_OnePlayerTwoSides )
	{
		m_bShowScoreboard = PREFSMAN->m_bEnableScoreboard.Get();
		PlayerNumber pn = GAMESTATE->GetFirstDisabledPlayer();
		if( pn != PLAYER_INVALID && m_bShowScoreboard )
		{
			FOREACH_NSScoreBoardColumn( col )
			{
				m_Scoreboard[col].LoadFromFont( THEME->GetPathF(m_sName,"scoreboard") );
				m_Scoreboard[col].SetShadowLength( 0 );
				m_Scoreboard[col].SetName( ssprintf("ScoreboardC%iP%i",col+1,pn+1) );
				LOAD_ALL_COMMANDS_AND_SET_XY( m_Scoreboard[col] );
				m_Scoreboard[col].SetText( NSMAN->m_Scoreboard[col] );
				m_Scoreboard[col].SetVertAlign( align_top );
				this->AddChild( &m_Scoreboard[col] );
			}
		}
	}
#endif

	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		// primary score display
		if( pi->m_pPrimaryScoreDisplay )
		{
			pi->m_pPrimaryScoreDisplay->SetName( ssprintf("Score%s",pi->GetName().c_str()) );
			LOAD_ALL_COMMANDS_AND_SET_XY( pi->m_pPrimaryScoreDisplay );
			if( GAMESTATE->m_PlayMode != PLAY_MODE_RAVE || SHOW_SCORE_IN_RAVE ) /* XXX: ugly */
				this->AddChild( pi->m_pPrimaryScoreDisplay );
		}

	
		// secondary score display
		if( pi->m_pSecondaryScoreDisplay )
		{
			pi->m_pSecondaryScoreDisplay->SetName( ssprintf("SecondaryScore%s",pi->GetName().c_str()) );
			LOAD_ALL_COMMANDS_AND_SET_XY( pi->m_pSecondaryScoreDisplay );
			this->AddChild( pi->m_pSecondaryScoreDisplay );
		}
	}
	
	//
	// Add stage / SongNumber
	//
	FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
	{
		if( GAMESTATE->IsCourseMode() )
		{
			ASSERT( pi->m_ptextCourseSongNumber == NULL );
			pi->m_ptextCourseSongNumber = new BitmapText;
			pi->m_ptextCourseSongNumber->LoadFromFont( THEME->GetPathF(m_sName,"song num") );
			pi->m_ptextCourseSongNumber->SetShadowLength( 0 );
			pi->m_ptextCourseSongNumber->SetName( ssprintf("SongNumber%s",pi->GetName().c_str()) );
			LOAD_ALL_COMMANDS_AND_SET_XY( pi->m_ptextCourseSongNumber );
			pi->m_ptextCourseSongNumber->SetText( "" );
			pi->m_ptextCourseSongNumber->SetDiffuse( RageColor(0,0.5f,1,1) );	// light blue
			this->AddChild( pi->m_ptextCourseSongNumber );
		}

		ASSERT( pi->m_ptextStepsDescription == NULL );
		pi->m_ptextStepsDescription = new BitmapText;
		pi->m_ptextStepsDescription->LoadFromFont( THEME->GetPathF(m_sName,"StepsDescription") );
		pi->m_ptextStepsDescription->SetName( ssprintf("StepsDescription%s",pi->GetName().c_str()) );
		LOAD_ALL_COMMANDS_AND_SET_XY( pi->m_ptextStepsDescription );
		this->AddChild( pi->m_ptextStepsDescription );

		//
		// Player/Song options
		//
		ASSERT( pi->m_ptextPlayerOptions == NULL );
		pi->m_ptextPlayerOptions = new BitmapText;
		pi->m_ptextPlayerOptions->LoadFromFont( THEME->GetPathF(m_sName,"player options") );
		pi->m_ptextPlayerOptions->SetShadowLength( 0 );
		pi->m_ptextPlayerOptions->SetName( ssprintf("PlayerOptions%s",pi->GetName().c_str()) );
		LOAD_ALL_COMMANDS_AND_SET_XY( pi->m_ptextPlayerOptions );
		this->AddChild( pi->m_ptextPlayerOptions );

		//
		// Difficulty icon and meter
		//
		ASSERT( pi->m_pDifficultyIcon == NULL );
		pi->m_pDifficultyIcon = new DifficultyIcon;
		pi->m_pDifficultyIcon->Load( THEME->GetPathG(m_sName,ssprintf("difficulty icons %dx%d",NUM_PLAYERS,NUM_Difficulty)) );
		pi->m_pDifficultyIcon->SetName( ssprintf("Difficulty%s",pi->GetName().c_str()) );
		LOAD_ALL_COMMANDS( pi->m_pDifficultyIcon );
		this->AddChild( pi->m_pDifficultyIcon );

//		switch( GAMESTATE->m_PlayMode )
//		{
//		case PLAY_MODE_BATTLE:
//			pi->m_pInventory = new Inventory;
//			pi->m_pInventory->Load( p );
//			this->AddChild( pi->m_pInventory );
//			break;
//		}
	}

	m_textSongOptions.LoadFromFont( THEME->GetPathF(m_sName,"song options") );
	m_textSongOptions.SetShadowLength( 0 );
	m_textSongOptions.SetName( "SongOptions" );
	LOAD_ALL_COMMANDS_AND_SET_XY( m_textSongOptions );
	m_textSongOptions.SetText( GAMESTATE->m_SongOptions.GetStage().GetLocalizedString() );
	this->AddChild( &m_textSongOptions );

	FOREACH_VisiblePlayerInfo( m_vPlayerInfo, pi )
	{
		ASSERT( pi->m_pActiveAttackList == NULL );
		pi->m_pActiveAttackList = new ActiveAttackList;
		pi->m_pActiveAttackList->LoadFromFont( THEME->GetPathF(m_sName,"ActiveAttackList") );
		pi->m_pActiveAttackList->Init( pi->GetPlayerState() );
		pi->m_pActiveAttackList->SetName( ssprintf("ActiveAttackList%s",pi->GetName().c_str()) );
		LOAD_ALL_COMMANDS_AND_SET_XY( pi->m_pActiveAttackList );
		this->AddChild( pi->m_pActiveAttackList );
	}

	if( g_bShowLyrics )
	{
		m_LyricDisplay.SetName( "Lyrics" );
		LOAD_ALL_COMMANDS( m_LyricDisplay );
		this->AddChild( &m_LyricDisplay );
	}

	if( !GAMESTATE->m_bDemonstrationOrJukebox )	// only load if we're going to use it
	{
		m_Ready.Load( THEME->GetPathB(m_sName,"ready") );
		this->AddChild( &m_Ready );

		m_Go.Load( THEME->GetPathB(m_sName,"go") );
		this->AddChild( &m_Go );

		m_Failed.Load( THEME->GetPathB(m_sName,"failed") );
		m_Failed.SetDrawOrder( DRAW_ORDER_TRANSITIONS-1 ); // on top of everything else
		this->AddChild( &m_Failed );

		m_textDebug.LoadFromFont( THEME->GetPathF("Common","normal") );
		m_textDebug.SetName( "Debug" );
		LOAD_ALL_COMMANDS_AND_SET_XY( m_textDebug );
		m_textDebug.SetDrawOrder( DRAW_ORDER_TRANSITIONS-1 );	// just under transitions, over the foreground
		this->AddChild( &m_textDebug );

		m_GameplayAssist.Init();

		if( GAMESTATE->IsAnExtraStage() )	// only load if we're going to use it
		{
			m_textSurviveTime.LoadFromFont( THEME->GetPathF(m_sName,"survive time") );
			m_textSurviveTime.SetShadowLength( 0 );
			m_textSurviveTime.SetName( "SurviveTime" );
			LOAD_ALL_COMMANDS_AND_SET_XY( m_textSurviveTime );
			m_textSurviveTime.SetDrawOrder( DRAW_ORDER_TRANSITIONS-1 );
			m_textSurviveTime.SetDiffuse( RageColor(1,1,1,0) );
			this->AddChild( &m_textSurviveTime );
		}

		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_BATTLE:
			m_soundBattleTrickLevel1.Load(	THEME->GetPathS(m_sName,"battle trick level1"), true );
			m_soundBattleTrickLevel2.Load(	THEME->GetPathS(m_sName,"battle trick level2"), true );
			m_soundBattleTrickLevel3.Load(	THEME->GetPathS(m_sName,"battle trick level3"), true );
			break;
		}
	}

	if( m_pSongBackground )
		m_pSongBackground->Init();

	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		RString sType = PLAYER_TYPE;
		if( pi->m_bIsDummy )
			sType += "Dummy";
		pi->m_pPlayer->Init( 
			sType,
			pi->GetPlayerState(),
			pi->GetPlayerStageStats(),
			pi->m_pLifeMeter, 
			m_pCombinedLifeMeter, 
			pi->m_pPrimaryScoreDisplay, 
			pi->m_pSecondaryScoreDisplay, 
			pi->m_pInventory, 
			pi->m_pPrimaryScoreKeeper, 
			pi->m_pSecondaryScoreKeeper );
	}

	//
	// fill in m_apSongsQueue, m_vpStepsQueue, m_asModifiersQueue
	//
	InitSongQueues();

	// Fill StageStats
	STATSMAN->m_CurStageStats.m_vpPossibleSongs = m_apSongsQueue;
	FOREACH( PlayerInfo, m_vPlayerInfo, pi )
	{
		if( pi->GetPlayerStageStats() )
			pi->GetPlayerStageStats()->m_vpPossibleSteps = pi->m_vpStepsQueue;
	}

	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		ASSERT( !pi->m_vpStepsQueue.empty() );
		if( pi->GetPlayerStageStats() )
			pi->GetPlayerStageStats()->m_bJoined = true;
		if( pi->m_pPrimaryScoreKeeper )
			pi->m_pPrimaryScoreKeeper->Load( m_apSongsQueue, pi->m_vpStepsQueue, pi->m_asModifiersQueue );
		if( pi->m_pSecondaryScoreKeeper )
			pi->m_pSecondaryScoreKeeper->Load( m_apSongsQueue, pi->m_vpStepsQueue, pi->m_asModifiersQueue );
	}

	GAMESTATE->m_bGameplayLeadIn.Set( true );

	/* LoadNextSong first, since that positions some elements which need to be
	 * positioned before we TweenOnScreen. */
	LoadNextSong();

	m_GiveUpTimer.SetZero();
}

bool ScreenGameplay::Center1Player() const
{
	/* Perhaps this should be handled better by defining a new
	 * StyleType for ONE_PLAYER_ONE_CREDIT_AND_ONE_COMPUTER,
	 * but for now just ignore Center1Player when it's Battle or Rave
	 * Mode.  This doesn't begin to address two-player solo (6 arrows) */
	return g_bCenter1Player && 
		(bool)ALLOW_CENTER_1_PLAYER &&
		GAMESTATE->m_PlayMode != PLAY_MODE_BATTLE &&
		GAMESTATE->m_PlayMode != PLAY_MODE_RAVE &&
		GAMESTATE->GetCurrentStyle()->m_StyleType == StyleType_OnePlayerOneSide;
}

//
// fill in m_apSongsQueue, m_vpStepsQueue, m_asModifiersQueue
//
void ScreenGameplay::InitSongQueues()
{
	LOG->Trace("InitSongQueues");

	if( GAMESTATE->IsCourseMode() )
	{
		Course* pCourse = GAMESTATE->m_pCurCourse;
		ASSERT( pCourse );

		m_apSongsQueue.clear();
		PlayerNumber pnMaster = GAMESTATE->m_MasterPlayerNumber;
		Trail *pTrail = GAMESTATE->m_pCurTrail[pnMaster];
		ASSERT( pTrail );
		FOREACH_CONST( TrailEntry, pTrail->m_vEntries, e )
		{
			ASSERT( e->pSong );
			m_apSongsQueue.push_back( e->pSong );
		}

		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
		{
			Trail *pTrail = GAMESTATE->m_pCurTrail[ pi->GetStepsAndTrailIndex() ];
			ASSERT( pTrail );

			pi->m_vpStepsQueue.clear();
			pi->m_asModifiersQueue.clear();
			FOREACH_CONST( TrailEntry, pTrail->m_vEntries, e )
			{
				ASSERT( e->pSteps );
				pi->m_vpStepsQueue.push_back( e->pSteps );
				AttackArray a;
				e->GetAttackArray( a );
				pi->m_asModifiersQueue.push_back( a );
			}

			// In a survival course, override stored mods
			if( pCourse->GetCourseType() == COURSE_TYPE_SURVIVAL )
			{
				pi->GetPlayerState()->m_PlayerOptions.FromString( ModsLevel_Stage, "clearall,"+CommonMetrics::DEFAULT_MODIFIERS.GetValue() );
				pi->GetPlayerState()->RebuildPlayerOptionsFromActiveAttacks();
			}
		}
	}
	else
	{
		m_apSongsQueue.push_back( GAMESTATE->m_pCurSong );
		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
		{
			Steps *pSteps = GAMESTATE->m_pCurSteps[ pi->GetStepsAndTrailIndex() ];
			pi->m_vpStepsQueue.push_back( pSteps );

			AttackArray aa;
			pi->m_asModifiersQueue.push_back( aa );
		}
	}

	if( GAMESTATE->m_bMultiplayer )
	{
		for( int i=0; i<(int)m_apSongsQueue.size(); i++ )
		{
			Song *pSong = m_apSongsQueue[i];

			FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
			{
				Steps *pOldSteps = pi->m_vpStepsQueue[i];

				vector<Steps*> vpSteps;
				SongUtil::GetSteps( pSong, vpSteps, pOldSteps->m_StepsType );
				StepsUtil::SortNotesArrayByDifficulty( vpSteps );
				vector<Steps*>::iterator iter = find( vpSteps.begin(), vpSteps.end(), pOldSteps );
				int iIndexBase = 0;
				if( iter != vpSteps.end() )
				{
					iIndexBase = iter - vpSteps.begin();
					CLAMP( iIndexBase, 0, vpSteps.size() - GAMESTATE->m_iNumMultiplayerNoteFields );
				}

				int iIndexToUse = iIndexBase + pi->m_iAddToDifficulty;
				CLAMP( iIndexToUse, 0, vpSteps.size()-1 );

				Steps *pSteps = vpSteps[iIndexToUse];
				pi->m_vpStepsQueue[i] = pSteps;
			}
		}
	}
}

ScreenGameplay::~ScreenGameplay()
{
	if( this->IsFirstUpdate() )
	{
		/* We never received any updates.  That means we were deleted without being
		 * used, and never actually played.  (This can happen when backing out of
		 * ScreenStage.)  Cancel the stage. */
		GAMESTATE->CancelStage();
	}

	LOG->Trace( "ScreenGameplay::~ScreenGameplay()" );

	SAFE_DELETE( m_pSongBackground );
	SAFE_DELETE( m_pSongForeground );
	
	if( !GAMESTATE->m_bDemonstrationOrJukebox )
		MEMCARDMAN->UnPauseMountingThread();

	SAFE_DELETE( m_pCombinedLifeMeter );
	if( m_pSoundMusic )
		m_pSoundMusic->StopPlaying();

	m_GameplayAssist.StopPlaying();

	NSMAN->ReportSongOver();
}

bool ScreenGameplay::IsLastSong()
{
	if( GAMESTATE->m_pCurCourse  &&  GAMESTATE->m_pCurCourse->m_bRepeat )
		return false;
	return GAMESTATE->GetCourseSongIndex() >= (int)m_apSongsQueue.size()-1; // GetCourseSongIndex() is 0-based
}

void ScreenGameplay::SetupSong( int iSongIndex )
{
	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		/* This is the first beat that can be changed without it being visible.  Until
		 * we draw for the first time, any beat can be changed. */
		pi->GetPlayerState()->m_fLastDrawnBeat = -100;

		Steps *pSteps = pi->m_vpStepsQueue[iSongIndex];
		GAMESTATE->m_pCurSteps[ pi->GetStepsAndTrailIndex() ].Set( pSteps );

		/* Load new NoteData into Player.  Do this before 
		 * RebuildPlayerOptionsFromActiveAttacks or else transform mods will get
		 * propogated to GAMESTATE->m_pPlayerOptions too early and be double-applied
		 * to the NoteData:
		 * once in Player::Load, then again in Player::ApplyActiveAttacks.  This 
		 * is very bad for transforms like AddMines.
		 */
		NoteData originalNoteData;
		pSteps->GetNoteData( originalNoteData );
		
		const Style* pStyle = GAMESTATE->GetCurrentStyle();
		NoteData ndTransformed;
		pStyle->GetTransformedNoteDataForStyle( pi->GetStepsAndTrailIndex(), originalNoteData, ndTransformed );

		// HACK: Apply NoteSkins from global course options.  Do this before Player::Load,
		// since it needs to know which note skin to load.
		pi->GetPlayerState()->m_ModsToApply.clear();
		for( unsigned i=0; i<pi->m_asModifiersQueue[iSongIndex].size(); ++i )
		{
			Attack a = pi->m_asModifiersQueue[iSongIndex][i];
			if( a.fStartSecond != 0 )
				continue;
			a.fStartSecond = -1;	// now

			PlayerOptions po;
			po.FromString( a.sModifiers );
			if( po.m_sNoteSkin.empty() )
				continue;
			a.sModifiers = po.m_sNoteSkin;

			pi->GetPlayerState()->LaunchAttack( a );
		}

		/* Update attack bOn flags, and rebuild Current-level options
		 * from Song-level options.  The current NoteSkin could have changed
		 * because of an attack ending. 
		 */
		pi->GetPlayerState()->Update( 0 );

		// load player
		{
			pi->m_NoteData = ndTransformed;
			NoteDataUtil::RemoveAllTapsOfType( pi->m_NoteData, TapNote::autoKeysound );
			pi->m_pPlayer->Load();
		}

		// load auto keysounds
		{
			NoteData nd = ndTransformed;
			NoteDataUtil::RemoveAllTapsExceptForType( nd, TapNote::autoKeysound );
			m_AutoKeysounds.Load( pi->GetStepsAndTrailIndex(), nd );
		}

		{
			RString sType;
			switch( GAMESTATE->m_SongOptions.GetCurrent().m_SoundEffectType )
			{
			case SongOptions::SOUNDEFFECT_OFF:	sType = "SoundEffectControl_Off";	break;
			case SongOptions::SOUNDEFFECT_SPEED:	sType = "SoundEffectControl_Speed";	break;
			case SongOptions::SOUNDEFFECT_PITCH:	sType = "SoundEffectControl_Pitch";	break;
			}

			pi->m_SoundEffectControl.Load( sType, pi->GetPlayerState(), &pi->m_NoteData );
		}

		// Put course options into effect.  Do this after Player::Load so
		// that mods aren't double-applied.
		pi->GetPlayerState()->m_ModsToApply.clear();
		for( unsigned i=0; i<pi->m_asModifiersQueue[iSongIndex].size(); ++i )
		{
			Attack a = pi->m_asModifiersQueue[iSongIndex][i];
			if( a.fStartSecond == 0 )
				a.fStartSecond = -1;	// now
			
			pi->GetPlayerState()->LaunchAttack( a );
			GAMESTATE->m_SongOptions.FromString( ModsLevel_Song, a.sModifiers );
		}

		/* Update attack bOn flags. */
		pi->GetPlayerState()->Update( 0 );

		/* Hack: Course modifiers that are set to start immediately shouldn't tween on. */
		pi->GetPlayerState()->m_PlayerOptions.SetCurrentToLevel( ModsLevel_Stage );
	}
}

void ScreenGameplay::ReloadCurrentSong()
{
	FOREACH_EnabledPlayerInfoNotDummy( m_vPlayerInfo, pi )
		pi->GetPlayerStageStats()->m_iSongsPlayed--;

	LoadNextSong();
}

void ScreenGameplay::LoadNextSong()
{
	GAMESTATE->ResetMusicStatistics();

	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		pi->GetPlayerStageStats()->m_iSongsPlayed++;
		if( pi->m_ptextCourseSongNumber )
			pi->m_ptextCourseSongNumber->SetText( ssprintf("%d", pi->GetPlayerStageStats()->m_iSongsPassed+1) );
	}

	if( GAMESTATE->m_bMultiplayer )
	{
		FOREACH_ENUM( MultiPlayer, mp )
			this->UpdateStageStats( mp );
	}

	int iPlaySongIndex = GAMESTATE->GetCourseSongIndex();
	iPlaySongIndex %= m_apSongsQueue.size();
	GAMESTATE->m_pCurSong.Set( m_apSongsQueue[iPlaySongIndex] );
	STATSMAN->m_CurStageStats.m_vpPlayedSongs.push_back( GAMESTATE->m_pCurSong );

	// No need to do this here.  We do it in SongFinished().
	//GAMESTATE->RemoveAllActiveAttacks();

	/* If we're in battery mode, force FailImmediate.  We assume in Player::Step that
	 * failed players can't step. */
	if( GAMESTATE->m_SongOptions.GetCurrent().m_LifeType == SongOptions::LIFE_BATTERY )
	{
		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
			PO_GROUP_ASSIGN( pi->GetPlayerState()->m_PlayerOptions, ModsLevel_Song, m_FailType, PlayerOptions::FAIL_IMMEDIATE );
	}

	m_textSongOptions.SetText( GAMESTATE->m_SongOptions.GetCurrent().GetString() );

	SetupSong( iPlaySongIndex );

	Song* pSong = GAMESTATE->m_pCurSong;
	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		Steps* pSteps = GAMESTATE->m_pCurSteps[ pi->GetStepsAndTrailIndex() ];
		++pi->GetPlayerStageStats()->m_iStepsPlayed;

		ASSERT( GAMESTATE->m_pCurSteps[ pi->GetStepsAndTrailIndex() ] );
		if( pi->m_ptextStepsDescription )
			pi->m_ptextStepsDescription->SetText( pSteps->GetDescription() );

		/* Increment the play count even if the player fails.  (It's still popular,
		 * even if the people playing it aren't good at it.) */
		if( !GAMESTATE->m_bDemonstrationOrJukebox )
		{
			if( pi->m_pn != PLAYER_INVALID )
				PROFILEMAN->IncrementStepsPlayCount( pSong, pSteps, pi->m_pn );
		}

		if( pi->m_ptextPlayerOptions )
			pi->m_ptextPlayerOptions->SetText( pi->GetPlayerState()->m_PlayerOptions.GetCurrent().GetString() );
		if( pi->m_pActiveAttackList )
			pi->m_pActiveAttackList->Refresh();

		// reset oni game over graphic
		SET_XY_AND_ON_COMMAND( pi->m_sprOniGameOver );

		if( GAMESTATE->m_SongOptions.GetCurrent().m_LifeType==SongOptions::LIFE_BATTERY && pi->GetPlayerStageStats()->m_bFailed )	// already failed
			pi->ShowOniGameOver();

		if( GAMESTATE->m_SongOptions.GetCurrent().m_LifeType==SongOptions::LIFE_BAR && pi->m_pLifeMeter )
			pi->m_pLifeMeter->UpdateNonstopLifebar();

		if( pi->m_pDifficultyIcon )
			pi->m_pDifficultyIcon->SetFromSteps( pi->GetStepsAndTrailIndex(), pSteps );

		/* The actual note data for scoring is the base class of Player.  This includes
		 * transforms, like Wide.  Otherwise, the scoring will operate on the wrong data. */
		if( pi->m_pPrimaryScoreKeeper )
			pi->m_pPrimaryScoreKeeper->OnNextSong( GAMESTATE->GetCourseSongIndex(), pSteps, &pi->m_pPlayer->GetNoteData() );
		if( pi->m_pSecondaryScoreKeeper )
			pi->m_pSecondaryScoreKeeper->OnNextSong( GAMESTATE->GetCourseSongIndex(), pSteps, &pi->m_pPlayer->GetNoteData() );

		// Don't mess with the PlayerController of the Dummy player
		if( !pi->m_bIsDummy )
		{
			if( GAMESTATE->m_bDemonstrationOrJukebox )
			{
				pi->GetPlayerState()->m_PlayerController = PC_CPU;
				pi->GetPlayerState()->m_iCpuSkill = 5;
			}
			else if( GAMESTATE->IsCpuPlayer(pi->GetStepsAndTrailIndex()) )
			{
				pi->GetPlayerState()->m_PlayerController = PC_CPU;
				int iMeter = pSteps->GetMeter();
				int iNewSkill = SCALE( iMeter, MIN_METER, MAX_METER, 0, NUM_SKILL_LEVELS-1 );
				/* Watch out: songs aren't actually bound by MAX_METER. */
				iNewSkill = clamp( iNewSkill, 0, NUM_SKILL_LEVELS-1 );
				pi->GetPlayerState()->m_iCpuSkill = iNewSkill;
			}
			else
			{
				pi->GetPlayerState()->m_PlayerController = GamePreferences::m_AutoPlay;
			}
		} 
	}
	
	bool bAllReverse = true;
	bool bAtLeastOneReverse = false;
	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		if( pi->GetPlayerState()->m_PlayerOptions.GetCurrent().m_fScrolls[PlayerOptions::SCROLL_REVERSE] == 1 )
			bAtLeastOneReverse = true;
		else
			bAllReverse = false;
	}

	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		bool bReverse = pi->GetPlayerState()->m_PlayerOptions.GetCurrent().m_fScrolls[PlayerOptions::SCROLL_REVERSE] == 1;

		if( pi->m_pDifficultyIcon )
			pi->m_pDifficultyIcon->PlayCommand( bReverse? "SetReverse":"SetNoReverse" );
	}

	m_LyricDisplay.PlayCommand( bAllReverse? "SetReverse": bAtLeastOneReverse? "SetOneReverse": "SetNoReverse" );

	// Load lyrics
	// XXX: don't load this here
	LyricsLoader LL;
	if( GAMESTATE->m_pCurSong->HasLyrics()  )
		LL.LoadFromLRCFile(GAMESTATE->m_pCurSong->GetLyricsPath(), *GAMESTATE->m_pCurSong);

	
	/* Set up song-specific graphics. */
	
	
	// Check to see if any players are in beginner mode.
	// Note: steps can be different if turn modifiers are used.
	if( PREFSMAN->m_bShowBeginnerHelper )
	{
		FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
		{
			PlayerNumber pn = pi->GetStepsAndTrailIndex();
			if( GAMESTATE->IsHumanPlayer(pn) && GAMESTATE->m_pCurSteps[pn]->GetDifficulty() == Difficulty_Beginner )
				m_BeginnerHelper.AddPlayer( pn, pi->m_pPlayer->GetNoteData() );
		}
	}

	if( m_pSongBackground )
		m_pSongBackground->Unload();

	if( !PREFSMAN->m_bShowBeginnerHelper || !m_BeginnerHelper.Initialize(2) )
	{
		m_BeginnerHelper.SetVisible( false );

		/* BeginnerHelper disabled, or failed to load. */
		if( m_pSongBackground )
			m_pSongBackground->LoadFromSong( GAMESTATE->m_pCurSong );

		if( !GAMESTATE->m_bDemonstrationOrJukebox )
		{
			/* This will fade from a preset brightness to the actual brightness (based
			 * on prefs and "cover").  The preset brightness may be 0 (to fade from
			 * black), or it might be 1, if the stage screen has the song BG and we're
			 * coming from it (like Pump).  This used to be done in SM_PlayReady, but
			 * that means it's impossible to snap to the new brightness immediately. */
			if( m_pSongBackground )
			{
				m_pSongBackground->SetBrightness( INITIAL_BACKGROUND_BRIGHTNESS );
				m_pSongBackground->FadeToActualBrightness();
			}
		}
	}
	else
	{
		m_BeginnerHelper.SetVisible( true );
	}

	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		if( !pi->GetPlayerStageStats()->m_bFailed )
		{
			// give a little life back between stages
			if( pi->m_pLifeMeter )
				pi->m_pLifeMeter->OnLoadSong();	
			if( pi->m_pPrimaryScoreDisplay )
				pi->m_pPrimaryScoreDisplay->OnLoadSong();	
			if( pi->m_pSecondaryScoreDisplay )
				pi->m_pSecondaryScoreDisplay->OnLoadSong();	
		}
	}
	if( m_pCombinedLifeMeter )
		m_pCombinedLifeMeter->OnLoadSong();	


	if( m_pSongBackground )
		m_pSongForeground->LoadFromSong( GAMESTATE->m_pCurSong );

	m_fTimeSinceLastDancingComment = 0;


	/* m_soundMusic and m_pSongBackground take a very long time to load,
	 * so cap fDelta at 0 so m_NextSong will show up on screen.
	 * -Chris */
	m_bZeroDeltaOnNextUpdate = true;
	SCREENMAN->ZeroNextUpdate();

	//
	// Load cabinet lights data
	//
	LoadLights();

	/* Load the music last, since it may start streaming and we don't want the music
	 * to compete with other loading. */
	m_AutoKeysounds.FinishLoading();
	m_pSoundMusic = m_AutoKeysounds.GetSound();

	/* Give SoundEffectControls the new RageSoundReaders. */
	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		RageSoundReader *pPlayerSound = m_AutoKeysounds.GetPlayerSound(pi->m_pn);
		if( pPlayerSound == NULL && pi->m_pn == GAMESTATE->m_MasterPlayerNumber )
			pPlayerSound = m_AutoKeysounds.GetSharedSound();
		pi->m_SoundEffectControl.SetSoundReader( pPlayerSound );
	}

	MESSAGEMAN->Broadcast("DoneLoadingNextSong");
}

void ScreenGameplay::LoadLights()
{
	if( !LIGHTSMAN->IsEnabled() )
		return;

	//
	// First, check if the song has explicit lights
	//
	m_CabinetLightsNoteData.Init();
	ASSERT( GAMESTATE->m_pCurSong );

	const Steps *pSteps = SongUtil::GetClosestNotes( GAMESTATE->m_pCurSong, STEPS_TYPE_LIGHTS_CABINET, Difficulty_Medium );
	if( pSteps != NULL )
	{
		pSteps->GetNoteData( m_CabinetLightsNoteData );
		return;
	}

	//
	// No explicit lights.  Create autogen lights.
	//
	RString sDifficulty = PREFSMAN->m_sLightsStepsDifficulty;
	vector<RString> asDifficulties;
	split( sDifficulty, ",", asDifficulties );

	Difficulty d1 = Difficulty_Invalid;
	if( asDifficulties.size() > 0 )
		d1 = StringToDifficulty( asDifficulties[0] );
	pSteps = SongUtil::GetClosestNotes( GAMESTATE->m_pCurSong, GAMESTATE->GetCurrentStyle()->m_StepsType, d1 );

	// If we can't find anything at all, stop.
	if( pSteps == NULL )
		return;

	NoteData TapNoteData1;
	pSteps->GetNoteData( TapNoteData1 );

	if( asDifficulties.size() > 1 )
	{
		Difficulty d2 = StringToDifficulty( asDifficulties[1] );
		const Steps *pSteps2 = SongUtil::GetClosestNotes( GAMESTATE->m_pCurSong, GAMESTATE->GetCurrentStyle()->m_StepsType, d2 );
		if( pSteps != NULL && pSteps2 != NULL && pSteps != pSteps2 )
		{
			NoteData TapNoteData2;
			pSteps2->GetNoteData( TapNoteData2 );
			NoteDataUtil::LoadTransformedLightsFromTwo( TapNoteData1, TapNoteData2, m_CabinetLightsNoteData );
			return;
		}

		/* fall through */
	}

	NoteDataUtil::LoadTransformedLights( TapNoteData1, m_CabinetLightsNoteData, GameManager::StepsTypeToNumTracks(STEPS_TYPE_LIGHTS_CABINET) );
}

void ScreenGameplay::StartPlayingSong( float fMinTimeToNotes, float fMinTimeToMusic )
{
	ASSERT( fMinTimeToNotes >= 0 );
	ASSERT( fMinTimeToMusic >= 0 );

	m_pSoundMusic->SetProperty( "AccurateSync", true );

	RageSoundParams p;
	p.m_fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	p.StopMode = RageSoundParams::M_CONTINUE;

	{
		const float fFirstBeat = GAMESTATE->m_pCurSong->m_fFirstBeat;
		const float fFirstSecond = GAMESTATE->m_pCurSong->GetElapsedTimeFromBeat( fFirstBeat );
		float fStartDelay = fMinTimeToNotes - fFirstSecond;
		fStartDelay = max( fStartDelay, fMinTimeToMusic );
		p.m_StartSecond = -fStartDelay;
	}

	ASSERT( !m_pSoundMusic->IsPlaying() );
	{
		float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
		GetMusicEndTiming( fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut );

		if( fSecondsToStartFadingOutMusic < GAMESTATE->m_pCurSong->m_fMusicLengthSeconds )
		{
			p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
			p.m_LengthSeconds = fSecondsToStartFadingOutMusic + MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
		}
	}
	m_pSoundMusic->Play( &p );
	if( m_bPaused )
		m_pSoundMusic->Pause( true );

	/* Make sure GAMESTATE->m_fMusicSeconds is set up. */
	GAMESTATE->m_fMusicSeconds = -5000;
	UpdateSongPosition(0);

	ASSERT( GAMESTATE->m_fMusicSeconds > -4000 ); /* make sure the "fake timer" code doesn't trigger */
}


void ScreenGameplay::PauseGame( bool bPause, GameController gc )
{
	if( m_bPaused == bPause )
	{
		LOG->Trace( "ScreenGameplay::PauseGame(%i) received, but already in that state; ignored", bPause );
		return;
	}

	/* Don't pause if we're already tweening out. */
	if( bPause && m_DancingState == STATE_OUTRO )
		return;
	
	AbortGiveUp( false );

	m_bPaused = bPause;
	m_PauseController = gc;

	m_pSoundMusic->Pause( bPause );
	if( bPause )
		this->PlayCommand( "Pause" );
	else
		this->PlayCommand( "Unpause" );

	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		pi->m_pPlayer->SetPaused( m_bPaused );
	}
}

// play assist ticks
void ScreenGameplay::PlayTicks()
{
	Player &player = *m_vPlayerInfo[0].m_pPlayer;
	const NoteData &nd = player.GetNoteData();
	m_GameplayAssist.PlayTicks( nd );
}

/* Play announcer "type" if it's been at least fSeconds since the last announcer. */
void ScreenGameplay::PlayAnnouncer( RString type, float fSeconds )
{
	if( GAMESTATE->m_fOpponentHealthPercent == 0 )
		return; // Shut the announcer up

	/* Don't play in demonstration. */
	if( GAMESTATE->m_bDemonstrationOrJukebox )
		return;

	/* Don't play before the first beat, or after we're finished. */
	if( m_DancingState != STATE_DANCING )
		return;
	if( GAMESTATE->m_pCurSong == NULL  ||	// this will be true on ScreenDemonstration sometimes
		GAMESTATE->m_fSongBeat < GAMESTATE->m_pCurSong->m_fFirstBeat )
		return;


	if( m_fTimeSinceLastDancingComment < fSeconds )
		return;
	m_fTimeSinceLastDancingComment = 0;

	SOUND->PlayOnceFromAnnouncer( type );
}

void ScreenGameplay::UpdateSongPosition( float fDeltaTime )
{
	if( !m_pSoundMusic->IsPlaying() )
		return;

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds( NULL, &tm );
	const float fAdjust = SOUND->GetFrameTimingAdjustment( fDeltaTime );
	GAMESTATE->UpdateSongPosition( fSeconds+fAdjust, GAMESTATE->m_pCurSong->m_Timing, tm+fAdjust );
}

void ScreenGameplay::BeginScreen()
{
	if( GAMESTATE->m_pCurSong == NULL  )
		return;

	ScreenWithMenuElements::BeginScreen();

	SOUND->PlayOnceFromAnnouncer( "gameplay intro" );	// crowd cheer

	//
	// Get the transitions rolling
	//
	if( GAMESTATE->m_bDemonstrationOrJukebox )
	{
		StartPlayingSong( MIN_SECONDS_TO_STEP, MIN_SECONDS_TO_MUSIC );
	}
	else if( NSMAN->useSMserver )
	{
		//If we're using networking, we must not have any
		//delay.  If we do this can cause inconsistancy
		//on different computers and differet themes

		StartPlayingSong( 0, 0 );
		m_pSoundMusic->Stop();

		float startOffset = g_fNetStartOffset;

		NSMAN->StartRequest(1); 

		RageSoundParams p;
		p.m_fSpeed = 1.0f;	//Force 1.0 playback speed
		p.StopMode = RageSoundParams::M_CONTINUE;
		p.m_StartSecond = startOffset;
		m_pSoundMusic->SetProperty( "AccurateSync", true );
		m_pSoundMusic->Play( &p );

		UpdateSongPosition(0);
	}
	else
	{
		StartPlayingSong( MIN_SECONDS_TO_STEP, MIN_SECONDS_TO_MUSIC );
	}
}

bool ScreenGameplay::AllAreFailing()
{
	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		if( pi->m_pLifeMeter && !pi->m_pLifeMeter->IsFailing() )
			return false;
	}
	return true;
}

void ScreenGameplay::GetMusicEndTiming( float &fSecondsToStartFadingOutMusic, float &fSecondsToStartTransitioningOut )
{
	float fLastStepSeconds = GAMESTATE->m_pCurSong->GetElapsedTimeFromBeat( GAMESTATE->m_pCurSong->m_fLastBeat );
	fLastStepSeconds += Player::GetMaxStepDistanceSeconds();

	float fTransitionLength;
	if( !GAMESTATE->IsCourseMode() || IsLastSong() )
		fTransitionLength = OUT_TRANSITION_LENGTH;
	else
		fTransitionLength = COURSE_TRANSITION_LENGTH;

	fSecondsToStartTransitioningOut = fLastStepSeconds;

	// Align the end of the music fade to the end of the transition.
	float fSecondsToFinishFadingOutMusic = fSecondsToStartTransitioningOut + fTransitionLength;
	if( fSecondsToFinishFadingOutMusic < GAMESTATE->m_pCurSong->m_fMusicLengthSeconds )
		fSecondsToStartFadingOutMusic = fSecondsToFinishFadingOutMusic - MUSIC_FADE_OUT_SECONDS;
	else
		fSecondsToStartFadingOutMusic = GAMESTATE->m_pCurSong->m_fMusicLengthSeconds; // don't fade

	/* Make sure we keep going long enough to register a miss for the last note, and
	 * never start fading before the last note. */
	fSecondsToStartFadingOutMusic = max( fSecondsToStartFadingOutMusic, fLastStepSeconds );
	fSecondsToStartTransitioningOut = max( fSecondsToStartTransitioningOut, fLastStepSeconds );

	/* Make sure the fade finishes before the transition finishes. */
	fSecondsToStartTransitioningOut = max( fSecondsToStartTransitioningOut, fSecondsToStartFadingOutMusic + MUSIC_FADE_OUT_SECONDS - fTransitionLength );
}

void ScreenGameplay::Update( float fDeltaTime )
{
	if( GAMESTATE->m_pCurSong == NULL  )
	{
		/* ScreenDemonstration will move us to the next screen.  We just need to
		 * survive for one update without crashing.  We need to call Screen::Update
		 * to make sure we receive the next-screen message. */
		Screen::Update( fDeltaTime );
		return;
	}

	UpdateSongPosition( fDeltaTime );

	if( m_bZeroDeltaOnNextUpdate )
	{
		Screen::Update( 0 );
		m_bZeroDeltaOnNextUpdate = false;
	}
	else
	{
		Screen::Update( fDeltaTime );
	}

	/* This happens if ScreenDemonstration::HandleScreenMessage sets a new screen when
	 * PREFSMAN->m_bDelayedScreenLoad. */
	if( GAMESTATE->m_pCurSong == NULL )
		return;
	/* This can happen if ScreenDemonstration::HandleScreenMessage sets a new screen when
	 * !PREFSMAN->m_bDelayedScreenLoad.  (The new screen was loaded when we called Screen::Update,
	 * and the ctor might set a new GAMESTATE->m_pCurSong, so the above check can fail.) */
	if( SCREENMAN->GetTopScreen() != this )
		return;

	/* Update actors when paused, but never move on to another state. */
	if( m_bPaused )
		return;

	//LOG->Trace( "m_fOffsetInBeats = %f, m_fBeatsPerSecond = %f, m_Music.GetPositionSeconds = %f", m_fOffsetInBeats, m_fBeatsPerSecond, m_Music.GetPositionSeconds() );

	m_AutoKeysounds.Update(fDeltaTime);

	//
	// update GameState HealthState
	//
	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		HealthState &hs = pi->GetPlayerState()->m_HealthState;
		HealthState OldHealthState = hs;
		if( GAMESTATE->GetPlayerFailType(pi->GetPlayerState()) != PlayerOptions::FAIL_OFF &&
			pi->m_pLifeMeter && pi->m_pLifeMeter->IsFailing() )
		{
			hs = HealthState_Dead;
		}
		else if( pi->m_pLifeMeter && pi->m_pLifeMeter->IsHot() )
		{
			hs = HealthState_Hot;
		}
		else if( GAMESTATE->GetPlayerFailType(pi->GetPlayerState()) != PlayerOptions::FAIL_OFF &&
			pi->m_pLifeMeter && pi->m_pLifeMeter->IsInDanger() )
		{
			hs = HealthState_Danger;
		}
		else
		{
			hs = HealthState_Alive;
		}

		if( hs != OldHealthState )
		{
			Message msg( "HealthStateChanged" );
			msg.SetParam( "PlayerNumber", pi->m_pn );
			msg.SetParam( "HealthState", hs );
			MESSAGEMAN->Broadcast( msg );
		}
		
		pi->m_SoundEffectControl.Update( fDeltaTime );
	}

	{
		float fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		if( GAMESTATE->m_SongOptions.GetCurrent().m_fHaste != 0.0f )
			fSpeed *= GetHasteRate();

		RageSoundParams p = m_pSoundMusic->GetParams();
		if( fabsf(p.m_fSpeed - fSpeed) > 0.01f )
		{
			p.m_fSpeed = fSpeed;
			m_pSoundMusic->SetParams( p );
		}
	}

	switch( m_DancingState )
	{
	case STATE_DANCING:
		/* Set STATSMAN->m_CurStageStats.bFailed for failed players.  In, FAIL_IMMEDIATE, send
		 * SM_BeginFailed if all players failed, and kill dead Oni players. */
		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
		{
			PlayerNumber pn = pi->GetStepsAndTrailIndex();

			PlayerOptions::FailType ft = GAMESTATE->GetPlayerFailType( pi->GetPlayerState() );
			SongOptions::LifeType lt = GAMESTATE->m_SongOptions.GetCurrent().m_LifeType;

			if( ft == PlayerOptions::FAIL_OFF || ft == PlayerOptions::FAIL_AT_END )
				continue;
			
			// check for individual fail
			if( pi->m_pLifeMeter == NULL || !pi->m_pLifeMeter->IsFailing() )
				continue; /* isn't failing */
			if( pi->GetPlayerStageStats()->m_bFailed )
				continue; /* failed and is already dead */
		
			LOG->Trace("Player %d failed", (int)pn);
			pi->GetPlayerStageStats()->m_bFailed = true;	// fail

			{
				Message msg("PlayerFailed");
				msg.SetParam( "PlayerNumber", pi->m_pn );
				MESSAGEMAN->Broadcast( msg );
			}

			//
			// Check for and do Oni die.
			//
			bool bAllowOniDie = false;
			switch( lt )
			{
			case SongOptions::LIFE_BATTERY:
				bAllowOniDie = true;
				break;
			}
			if( bAllowOniDie && ft == PlayerOptions::FAIL_IMMEDIATE )
			{
				if( !STATSMAN->m_CurStageStats.AllFailed() )	// if not the last one to fail
				{
					// kill them!
					SOUND->PlayOnceFromDir( THEME->GetPathS(m_sName,"oni die") );
					pi->ShowOniGameOver();
					pi->m_NoteData.Init();		// remove all notes and scoring
					pi->m_pPlayer->FadeToFail();	// tell the NoteField to fade to white
				}
			}
		}

		bool bAllFailed = true;
		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
		{
			PlayerOptions::FailType ft = GAMESTATE->GetPlayerFailType( pi->GetPlayerState() );
			switch( ft )
			{
			case PlayerOptions::FAIL_IMMEDIATE:
				if( pi->m_pLifeMeter == NULL  ||  (pi->m_pLifeMeter && !pi->m_pLifeMeter->IsFailing()) )
					bAllFailed = false;
				break;
			case PlayerOptions::FAIL_IMMEDIATE_CONTINUE:
			case PlayerOptions::FAIL_AT_END:
				bAllFailed = false;	// wait until the end of the song to fail.
				break;
			case PlayerOptions::FAIL_OFF:
				bAllFailed = false;	// never fail.
				break;
			default:
				ASSERT(0);
			}
		}
		
		if( bAllFailed )
		{
			m_pSoundMusic->StopPlaying();
			SCREENMAN->PostMessageToTopScreen( SM_NotesEnded, 0 );
		}

		//
		// Update living players' alive time
		//
		// HACK: Don't scale alive time when ussing tab/tilde.  Instead of accumulating time from a timer, 
		// this time should instead be tied to the music position.
		float fUnscaledDeltaTime = m_timerGameplaySeconds.GetDeltaTime();

		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
			if( !pi->GetPlayerStageStats()->m_bFailed )
				pi->GetPlayerStageStats()->m_fAliveSeconds += fUnscaledDeltaTime * GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

		// update fGameplaySeconds
		STATSMAN->m_CurStageStats.m_fGameplaySeconds += fUnscaledDeltaTime;
		if( GAMESTATE->m_fSongBeat >= GAMESTATE->m_pCurSong->m_fFirstBeat && GAMESTATE->m_fSongBeat < GAMESTATE->m_pCurSong->m_fLastBeat )
		{
			STATSMAN->m_CurStageStats.m_fStepsSeconds += fUnscaledDeltaTime;

			if( GAMESTATE->m_SongOptions.GetCurrent().m_fHaste != 0.0f )
			{
				float fHasteRate = GetHasteRate();
				GAMESTATE->m_fAccumulatedHasteSeconds += (fUnscaledDeltaTime * fHasteRate) - fUnscaledDeltaTime;
			}
		}

		//
		// Check for end of song
		//
		{
			float fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut;
			GetMusicEndTiming( fSecondsToStartFadingOutMusic, fSecondsToStartTransitioningOut );

			bool bAllReallyFailed = STATSMAN->m_CurStageStats.AllFailed();
			if( bAllReallyFailed )
				fSecondsToStartTransitioningOut += BEGIN_FAILED_DELAY;

			if( GAMESTATE->m_fMusicSeconds >= fSecondsToStartTransitioningOut && !m_NextSong.IsTransitioning() )
				this->PostScreenMessage( SM_NotesEnded, 0 );
		}

		//
		// update 2d dancing characters
		//
		FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
		{
			DancingCharacters *pCharacter = NULL;
			if( m_pSongBackground )
				pCharacter = m_pSongBackground->GetDancingCharacters();
			if( pCharacter != NULL )
			{
				TapNoteScore tns = pi->m_pPlayer->GetLastTapNoteScore();
				
				ANIM_STATES_2D state = AS2D_MISS;

				switch( tns )
				{
				case TNS_W4:
				case TNS_W3:
					state = AS2D_GOOD;
					break;
				case TNS_W2:
				case TNS_W1:
					state = AS2D_GREAT;
					break;
				default:
					state = AS2D_MISS;
					break;
				}

				if( state == AS2D_GREAT && pi->GetPlayerState()->m_HealthState == HealthState_Hot )
					state = AS2D_FEVER;

				pCharacter->Change2DAnimState( pi->m_pn, state );
			}
		}

		//
		// Check for enemy death in enemy battle
		//
		static float fLastSeenEnemyHealth = 1;
		if( fLastSeenEnemyHealth != GAMESTATE->m_fOpponentHealthPercent )
		{
			fLastSeenEnemyHealth = GAMESTATE->m_fOpponentHealthPercent;

			if( GAMESTATE->m_fOpponentHealthPercent == 0 )
			{
				// HACK:  Load incorrect directory on purpose for now.
				PlayAnnouncer( "gameplay battle damage level3", 0 );

				GAMESTATE->RemoveAllActiveAttacks();

				FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
				{
					if( !GAMESTATE->IsCpuPlayer(pi->m_pn) )
						continue;

					SOUND->PlayOnceFromDir( THEME->GetPathS(m_sName,"oni die") );
					pi->ShowOniGameOver();
					pi->m_NoteData.Init(); // remove all notes and scoring
					pi->m_pPlayer->FadeToFail(); // tell the NoteField to fade to white
				}
			}
		}

		//
		// update give up
		//
		bool bGiveUpTimerFired = !m_GiveUpTimer.IsZero() && m_GiveUpTimer.Ago() > 2.5f;
		bool bAllHumanHaveBigMissCombo = FAIL_ON_MISS_COMBO.GetValue() != -1 && STATSMAN->m_CurStageStats.GetMinimumMissCombo() >= FAIL_ON_MISS_COMBO;
		if( bGiveUpTimerFired || bAllHumanHaveBigMissCombo )
		{
			STATSMAN->m_CurStageStats.m_bGaveUp = true;
			FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
			{
				pi->GetPlayerStageStats()->m_bFailed |= bAllHumanHaveBigMissCombo;
				pi->GetPlayerStageStats()->m_bDisqualified = true;
			}

			AbortGiveUp( false );

			if( GIVING_UP_GOES_TO_PREV_SCREEN )
			{
				BeginBackingOutFromGameplay();
			}
			else
			{
				m_pSoundMusic->StopPlaying();
				this->PostScreenMessage( SM_NotesEnded, 0 );
			}
			return;
		}

		//
		// Check to see if it's time to play a ScreenGameplay comment
		//
		m_fTimeSinceLastDancingComment += fDeltaTime;

		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_REGULAR:
		case PLAY_MODE_BATTLE:
		case PLAY_MODE_RAVE:
			if( GAMESTATE->OneIsHot() )			
				PlayAnnouncer( "gameplay comment hot", SECONDS_BETWEEN_COMMENTS );
			else if( GAMESTATE->AllAreInDangerOrWorse() )	
				PlayAnnouncer( "gameplay comment danger", SECONDS_BETWEEN_COMMENTS );
			else
				PlayAnnouncer( "gameplay comment good", SECONDS_BETWEEN_COMMENTS );
			break;
		case PLAY_MODE_NONSTOP:
		case PLAY_MODE_ONI:
		case PLAY_MODE_ENDLESS:
			PlayAnnouncer( "gameplay comment oni", SECONDS_BETWEEN_COMMENTS );
			break;
		default:
			ASSERT(0);
		}
	}

	PlayTicks();

	UpdateLights();

	SendCrossedMessages();

	if( NSMAN->useSMserver )
	{
		FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
			if( pi->m_pLifeMeter )
				NSMAN->m_playerLife[pi->m_pn] = int(pi->m_pLifeMeter->GetLife()*10000);

		if( m_bShowScoreboard )
			FOREACH_NSScoreBoardColumn(cn)
				if( m_bShowScoreboard && NSMAN->ChangedScoreboard(cn) && GAMESTATE->GetFirstDisabledPlayer() != PLAYER_INVALID )
					m_Scoreboard[cn].SetText( NSMAN->m_Scoreboard[cn] );
	}
}

float ScreenGameplay::GetHasteRate()
{
	if( GAMESTATE->m_fMusicSeconds < GAMESTATE->m_fLastHasteUpdateMusicSeconds || // new song
		GAMESTATE->m_fMusicSeconds > GAMESTATE->m_fLastHasteUpdateMusicSeconds + 4 )
	{
		bool bAnyPlayerHitAllNotes = false;
		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
		{
			if( !GAMESTATE->IsHumanPlayer(pi->m_pn) )
				continue;

			PlayerState *pPS = pi->GetPlayerState();
			if( pPS->m_iTapsHitSinceLastHasteUpdate > 0 &&
				pPS->m_iTapsMissedSinceLastHasteUpdate == 0 )
				bAnyPlayerHitAllNotes = true;

			pPS->m_iTapsHitSinceLastHasteUpdate = 0;
			pPS->m_iTapsMissedSinceLastHasteUpdate = 0;
		}

		if( bAnyPlayerHitAllNotes )
			GAMESTATE->m_fHasteRate += 0.1f;
		CLAMP( GAMESTATE->m_fHasteRate, -1.0f, +1.0f );

		GAMESTATE->m_fLastHasteUpdateMusicSeconds = GAMESTATE->m_fMusicSeconds;
	}

	/* If the life meter is less than half full, push the haste rate down to let
	 * the player use his accumulated haste time. */
	float fMaxLife = 0;
	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		if( !GAMESTATE->IsHumanPlayer(pi->m_pn) )
			continue;
                fMaxLife = max( fMaxLife, pi->m_pLifeMeter->GetLife() );
	}
	if( fMaxLife < 0.5f )
		GAMESTATE->m_fHasteRate = SCALE( fMaxLife, 0.0f, 0.5f, -1.0f, 0.0f );

	float fSpeed = 1.0f;
	if( GAMESTATE->m_fHasteRate < 0 )
		fSpeed = SCALE( GAMESTATE->m_fHasteRate, -1.0f, 0.0f, 0.5f, 1.0f );
	else if( GAMESTATE->m_fHasteRate < 0.3f )
		fSpeed = SCALE( GAMESTATE->m_fHasteRate, 0.0f, 0.3f, 1.0f, 1.2f );
	else
		fSpeed = SCALE( GAMESTATE->m_fHasteRate, 0.3f, 1.0f, 1.2f, 1.5f );
	fSpeed *= GAMESTATE->m_SongOptions.GetCurrent().m_fHaste;

	if( GAMESTATE->m_fAccumulatedHasteSeconds <= 1 )
	{
		/* Only allow slowing down the song while the players have accumulated
		 * haste.  This prevents dragging on the song by keeping the life meter
		 * nearly empty. */
		float fClamped = max( 1.0f, fSpeed );
		fSpeed = lerp( GAMESTATE->m_fAccumulatedHasteSeconds, fClamped, fSpeed );
	}
	return fSpeed;
}

void ScreenGameplay::UpdateLights()
{
	if( !LIGHTSMAN->IsEnabled() )
		return;
	if( m_CabinetLightsNoteData.GetNumTracks() == 0 )	// light data wasn't loaded
		return;

	const Style* pStyle = GAMESTATE->GetCurrentStyle();
	bool bBlinkCabinetLight[NUM_CabinetLight];
	bool bBlinkGameButton[NUM_GameController][NUM_GameButton];
	ZERO( bBlinkCabinetLight );
	ZERO( bBlinkGameButton );
	bool bCrossedABeat = false;
	{
		const float fSongBeat = GAMESTATE->m_fLightSongBeat;
		const int iSongRow = BeatToNoteRowNotRounded( fSongBeat );

		static int iRowLastCrossed = 0;

		float fBeatLast = roundf(NoteRowToBeat(iRowLastCrossed));
		float fBeatNow = roundf(NoteRowToBeat(iSongRow));

		bCrossedABeat = fBeatLast != fBeatNow;

		FOREACH_CabinetLight( cl )
		{	
			// for each index we crossed since the last update:
			FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( m_CabinetLightsNoteData, cl, r, iRowLastCrossed+1, iSongRow+1 )
			{
				if( m_CabinetLightsNoteData.GetTapNote( cl, r ).type != TapNote::empty )
					bBlinkCabinetLight[cl] = true;
			}

			if( m_CabinetLightsNoteData.IsHoldNoteAtRow( cl, iSongRow ) )
				bBlinkCabinetLight[cl] = true;
		}

		FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
		{
			const NoteData &nd = pi->m_pPlayer->GetNoteData();
			for( int t=0; t<nd.GetNumTracks(); t++ )
			{
				bool bBlink = false;

				// for each index we crossed since the last update:
				FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( nd, t, r, iRowLastCrossed+1, iSongRow+1 )
				{
					const TapNote &tn = nd.GetTapNote( t, r );
					if( tn.type != TapNote::mine )
						bBlink = true;
				}

				// check if a hold should be active
				if( nd.IsHoldNoteAtRow( t, iSongRow ) )
					bBlink = true;

				if( bBlink )
				{
					GameInput gi = pStyle->StyleInputToGameInput( t, pi->m_pn );
					bBlinkGameButton[gi.controller][gi.button] = true;
				}
			}
		}

		iRowLastCrossed = iSongRow;
	}

	// Before the first beat of the song, all cabinet lights solid on (except for menu buttons).
	bool bOverrideCabinetBlink = (GAMESTATE->m_fSongBeat < GAMESTATE->m_pCurSong->m_fFirstBeat);
	FOREACH_CabinetLight( cl )
		bBlinkCabinetLight[cl] |= bOverrideCabinetBlink;

	// Send blink data.
	FOREACH_CabinetLight( cl )
	{
		if( bBlinkCabinetLight[cl] )
			LIGHTSMAN->BlinkCabinetLight( cl );
	}

	FOREACH_GameController( gc )
	{
		FOREACH_GameButton( gb )
		{
			if( bBlinkGameButton[gc][gb] )
				LIGHTSMAN->BlinkGameButton( GameInput(gc,gb) );
		}
	}
}

void ScreenGameplay::SendCrossedMessages()
{
	{
		static int iRowLastCrossed = 0;

		float fPositionSeconds = GAMESTATE->m_fMusicSeconds;
		float fSongBeat = GAMESTATE->m_pCurSong->GetBeatFromElapsedTime( fPositionSeconds );

		int iRowNow = BeatToNoteRowNotRounded( fSongBeat );
		iRowNow = max( 0, iRowNow );

		for( int r=iRowLastCrossed+1; r<=iRowNow; r++ )
		{
			if( GetNoteType( r ) == NOTE_TYPE_4TH )
				MESSAGEMAN->Broadcast( Message_BeatCrossed );
		}

		iRowLastCrossed = iRowNow;
	}

	{
		const int NUM_MESSAGES_TO_SEND = 4;
		const float MESSAGE_SPACING_SECONDS = 0.4f;

		PlayerNumber pn = PLAYER_INVALID;
		FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
		{
			if( GAMESTATE->m_pCurSteps[ pi->m_pn ]->GetDifficulty() == Difficulty_Beginner )
			{
				pn = pi->m_pn;
				break;
			}
		}
		if( pn == PLAYER_INVALID )
			return;

		const NoteData &nd = m_vPlayerInfo[pn].m_pPlayer->GetNoteData();

		static int iRowLastCrossedAll[NUM_MESSAGES_TO_SEND] = { 0, 0, 0, 0 };
		for( int i=0; i<NUM_MESSAGES_TO_SEND; i++ )
		{
			float fOffsetFromCurrentSeconds = MESSAGE_SPACING_SECONDS * i;

			float fPositionSeconds = GAMESTATE->m_fMusicSeconds + fOffsetFromCurrentSeconds;
			float fSongBeat = GAMESTATE->m_pCurSong->GetBeatFromElapsedTime( fPositionSeconds );

			int iRowNow = BeatToNoteRowNotRounded( fSongBeat );
			iRowNow = max( 0, iRowNow );
			int &iRowLastCrossed = iRowLastCrossedAll[i];

			FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( nd, r, iRowLastCrossed+1, iRowNow+1 )
			{
				int iNumTracksWithTapOrHoldHead = 0;
				for( int t=0; t<nd.GetNumTracks(); t++ )
				{
					if( nd.GetTapNote(t,r).type == TapNote::empty )
						continue;

					iNumTracksWithTapOrHoldHead++;

					// Send col-specific crossed
					if( i == 0 )
					{
						const Style *pStyle = GAMESTATE->GetCurrentStyle();
						RString sButton = pStyle->ColToButtonName( t );
						RString sMessageName = "NoteCrossed" + sButton;
						MESSAGEMAN->Broadcast( sMessageName );
					}
				}

				if( iNumTracksWithTapOrHoldHead > 0 )
					MESSAGEMAN->Broadcast( (MessageID)(Message_NoteCrossed + i) );
				if( i == 0  &&  iNumTracksWithTapOrHoldHead >= 2 )
				{
					RString sMessageName = "NoteCrossedJump";
					MESSAGEMAN->Broadcast( sMessageName );
				}
			}

			iRowLastCrossed = iRowNow;
		}
	}
}

void ScreenGameplay::BeginBackingOutFromGameplay()
{
	m_DancingState = STATE_OUTRO;
	AbortGiveUp( false );
	
	m_pSoundMusic->StopPlaying();
	m_GameplayAssist.StopPlaying(); /* Stop any queued assist ticks. */

	this->ClearMessageQueue();
	
	// If this is the final stage, don't allow extra stage
	if( GAMESTATE->GetSmallestNumStagesLeftForAnyHumanPlayer() == 0 )
		GAMESTATE->m_bBackedOutOfFinalStage = true;
	// Disallow backing out of extra stage
	if( GAMESTATE->IsAnExtraStage() )
		SCREENMAN->PostMessageToTopScreen( SM_BeginFailed, 0 );
	else
		m_Cancel.StartTransitioning( SM_DoPrevScreen );
}

void ScreenGameplay::AbortGiveUp( bool bShowText )
{
	if( m_GiveUpTimer.IsZero() )
		return;

	m_textDebug.StopTweening();
	if( bShowText )
		m_textDebug.SetText( GIVE_UP_ABORTED_TEXT );
	// otherwise tween out the text that's there

	m_textDebug.BeginTweening( 1/2.f );
	m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
	m_GiveUpTimer.SetZero();
}


void ScreenGameplay::Input( const InputEventPlus &input )
{
	//LOG->Trace( "ScreenGameplay::Input()" );

	if( m_bPaused )
	{
		/* If we're paused, only accept MENU_BUTTON_START to unpause. */
		if( GAMESTATE->IsHumanPlayer(input.pn) && input.MenuI == MENU_BUTTON_START && input.type == IET_FIRST_PRESS )
		{
			if( m_PauseController == GameController_Invalid || m_PauseController == input.GameI.controller )
				this->PauseGame( false );
		}
		return;
	}

	if( m_DancingState != STATE_OUTRO  &&
		GAMESTATE->IsHumanPlayer(input.pn)  &&
		!m_Cancel.IsTransitioning() )
	{
		/*
		 * Allow bailing out by holding any START button.  This gives a way to "give up"
		 * when a back button isn't available.
		 *
		 * If this is also a style button, don't do this; pump center is start.
		 */
		bool bHoldingGiveUp = false;
		if( GAMESTATE->GetCurrentStyle()->GameInputToColumn(input.GameI) == Column_Invalid )
		{
			bHoldingGiveUp |= ( START_GIVES_UP && input.MenuI == MENU_BUTTON_START );
			bHoldingGiveUp |= ( BACK_GIVES_UP && input.MenuI == MENU_BUTTON_BACK );
		}
		
		if( bHoldingGiveUp )
		{
			/* No PREFSMAN->m_bDelayedEscape; always delayed. */
			if( input.type==IET_RELEASE )
			{
				AbortGiveUp( true );
			}
			else if( input.type==IET_FIRST_PRESS && m_GiveUpTimer.IsZero() )
			{
				m_textDebug.SetText( GIVE_UP_START_TEXT );
				m_textDebug.StopTweening();
				m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
				m_textDebug.BeginTweening( 1/8.f );
				m_textDebug.SetDiffuse( RageColor(1,1,1,1) );
				m_GiveUpTimer.Touch(); /* start the timer */
			}

			return;
		}

		/* Only handle MENU_BUTTON_BACK as a regular BACK button if BACK_GIVES_UP is
		 * disabled. */
		bool bHoldingBack = false;
		if( GAMESTATE->GetCurrentStyle()->GameInputToColumn(input.GameI) == Column_Invalid )
		{
			bHoldingBack |= input.MenuI == MENU_BUTTON_BACK && !BACK_GIVES_UP;
		}

		if( bHoldingBack )
		{
			if( ((!PREFSMAN->m_bDelayedBack && input.type==IET_FIRST_PRESS) ||
				(input.DeviceI.device==DEVICE_KEYBOARD && input.type==IET_REPEAT) ||
				(input.DeviceI.device!=DEVICE_KEYBOARD && INPUTFILTER->GetSecsHeld(input.DeviceI) >= 1.0f)) )
			{
				LOG->Trace("Player %i went back", input.pn+1);
				BeginBackingOutFromGameplay();
			}
			else if( PREFSMAN->m_bDelayedBack && input.type==IET_FIRST_PRESS )
			{
				m_textDebug.SetText( GIVE_UP_BACK_TEXT );
				m_textDebug.StopTweening();
				m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
				m_textDebug.BeginTweening( 1/8.f );
				m_textDebug.SetDiffuse( RageColor(1,1,1,1) );
			}
			else if( PREFSMAN->m_bDelayedBack && input.type==IET_RELEASE )
			{
				m_textDebug.StopTweening();
				m_textDebug.BeginTweening( 1/8.f );
				m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
			}

			return;
		}
	}
	
	bool bRelease = input.type == IET_RELEASE;
	if( !input.GameI.IsValid() )
		return;
	
	const int iCol = GAMESTATE->GetCurrentStyle()->GameInputToColumn( input.GameI );

	// Don't pass on any inputs to Player that aren't a press or a release.
	switch( input.type )
	{
	case IET_FIRST_PRESS:
	case IET_RELEASE:
		break;
	default:
		return;
	}

	if( GAMESTATE->m_bMultiplayer )
	{
		if( input.mp != MultiPlayer_Invalid  &&  GAMESTATE->IsMultiPlayerEnabled(input.mp)  &&  iCol != -1 )
		{
			FOREACH( PlayerInfo, m_vPlayerInfo, pi )
			{
				if( input.mp == pi->m_mp )
					pi->m_pPlayer->Step( iCol, -1, input.DeviceI.ts, false, bRelease );
			}
		}
	}
	else
	{	
		//
		// handle a step or battle item activate
		//
		if( GAMESTATE->IsHumanPlayer( input.pn ) )
		{
			AbortGiveUp( true );
			
			if( GamePreferences::m_AutoPlay == PC_HUMAN )
			{
				PlayerInfo& pi = GetPlayerInfoForInput( input );
				
				ASSERT( input.GameI.IsValid() );

				GameButtonType gbt = GAMESTATE->m_pCurGame->GetPerButtonInfo(input.GameI.button)->m_gbt;
				switch( gbt )
				{
				case GameButtonType_INVALID:
					break;
				case GameButtonType_Step:
					if( iCol != -1 )
						pi.m_pPlayer->Step( iCol, -1, input.DeviceI.ts, false, bRelease );
					break;
				case GameButtonType_Fret:
					if( iCol != -1 )
						pi.m_pPlayer->Fret( iCol, -1, input.DeviceI.ts, false, bRelease );
					break;
				case GameButtonType_Strum:
					pi.m_pPlayer->Strum( iCol, -1, input.DeviceI.ts, false, bRelease );
					break;
				}
			}
		}
	}
}



/*
 * Saving StageStats that are affected by the note pattern is a little tricky:
 *
 * Stats are cumulative for course play.
 *
 * For regular songs, it doesn't matter how we do it; the pattern doesn't change
 * during play.
 *
 * The pattern changes during play in battle and course mode.  We want to include these
 * changes, so run stats for a song after the song finishes.
 *
 * If we fail, be sure to include the current song in stats, with the current modifier set.
 *
 * So:
 * 
 * 1. At the end of a song in any mode, pass or fail, add stats for that song (from m_pPlayer).
 * 2. At the end of gameplay in course mode, add stats for any songs that weren't played,
 *    applying the modifiers the song would have been played with.  This doesn't include songs
 *    that were played but failed; that was done in #1.
 */
void ScreenGameplay::SaveStats()
{
	float fMusicLen = GAMESTATE->m_pCurSong->m_fMusicLengthSeconds;

	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		/* Note that adding stats is only meaningful for the counters (eg. RadarCategory_Jumps),
		 * not for the percentages (RadarCategory_Air). */
		RadarValues rv;
		PlayerStageStats &pss = *pi->GetPlayerStageStats();
		const NoteData &nd = pi->m_pPlayer->GetNoteData();
		
		NoteDataUtil::CalculateRadarValues( nd, fMusicLen, rv );
		pss.m_radarPossible += rv;

		NoteDataWithScoring::GetActualRadarValues( nd, pss, fMusicLen, rv );
		pss.m_radarActual += rv;
	}
}

void ScreenGameplay::SongFinished()
{
	AdjustSync::HandleSongEnd();
	SaveStats(); // Let subclasses save the stats.
	/* Extremely important: if we don't remove attacks before moving on to the next
	 * screen, they'll still be turned on eventually. */
	GAMESTATE->RemoveAllActiveAttacks();
	FOREACH_VisiblePlayerInfo( m_vPlayerInfo, pi )
		pi->m_pActiveAttackList->Refresh();
}

void ScreenGameplay::StageFinished( bool bBackedOut )
{
	if( GAMESTATE->IsCourseMode() && GAMESTATE->m_PlayMode != PLAY_MODE_ENDLESS )
	{
		LOG->Trace("Stage finished at index %i/%i", GAMESTATE->GetCourseSongIndex(), (int) m_apSongsQueue.size() );
		/* +1 to skip the current song; that song has already passed. */
		for( unsigned i = GAMESTATE->GetCourseSongIndex()+1; i < m_apSongsQueue.size(); ++i )
		{
			LOG->Trace("Running stats for %i", i );
			SetupSong( i );
			FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
				pi->m_pPlayer->ApplyWaitingTransforms();
			SongFinished();
		}
	}

	if( bBackedOut )
	{
		GAMESTATE->CancelStage();
		return;
	}

	/* If all players failed, kill. */
	if( STATSMAN->m_CurStageStats.AllFailed() )
	{
		FOREACH_HumanPlayer( p )
			GAMESTATE->m_iPlayerStageTokens[p] = 0;
	}

	FOREACH_HumanPlayer( pn )
		STATSMAN->m_CurStageStats.m_player[pn].CalcAwards( pn, STATSMAN->m_CurStageStats.m_bGaveUp, STATSMAN->m_CurStageStats.m_bUsedAutoplay );
	STATSMAN->m_CurStageStats.CommitScores( false );
	if( GAMESTATE->m_bMultiplayer )	// no saved stats in multiplayer
		GAMESTATE->CommitStageStats();

	// save current stage stats
	STATSMAN->m_vPlayedStageStats.push_back( STATSMAN->m_CurStageStats );

	STATSMAN->CalcAccumPlayedStageStats();
	GAMESTATE->FinishStage();
}

void ScreenGameplay::HandleScreenMessage( const ScreenMessage SM )
{
	CHECKPOINT_M( ssprintf("HandleScreenMessage(%i)", SM) );
	if( SM == SM_DoneFadingIn )
	{
		SOUND->PlayOnceFromAnnouncer( "gameplay ready" );
		m_Ready.StartTransitioning( SM_PlayGo );
	}
	else if( SM == SM_PlayGo )
	{
		if( GAMESTATE->IsAnExtraStage() )
			SOUND->PlayOnceFromAnnouncer( "gameplay here we go extra" );
		else if( GAMESTATE->GetSmallestNumStagesLeftForAnyHumanPlayer() == 0 )
			SOUND->PlayOnceFromAnnouncer( "gameplay here we go final" );
		else
			SOUND->PlayOnceFromAnnouncer( "gameplay here we go normal" );

		m_Go.StartTransitioning( SM_None );
		GAMESTATE->m_bGameplayLeadIn.Set( false );
		m_DancingState = STATE_DANCING;		// STATE CHANGE!  Now the user is allowed to press Back
	}
	else if( SM == SM_NotesEnded )	// received while STATE_DANCING
	{
		AbortGiveUp( false );	// don't allow giveup while the next song is loading

		/* Do this in LoadNextSong, so we don't tween off old attacks until
		 * m_NextSong finishes. */
		// GAMESTATE->RemoveAllActiveAttacks();

		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
		{
			/* Mark failure. */
			if( GAMESTATE->GetPlayerFailType(pi->GetPlayerState()) != PlayerOptions::FAIL_OFF &&
				(pi->m_pLifeMeter && pi->m_pLifeMeter->IsFailing()) )
				pi->GetPlayerStageStats()->m_bFailed = true;

			if( !pi->GetPlayerStageStats()->m_bFailed )
				pi->GetPlayerStageStats()->m_iSongsPassed++;

			// set a life record at the point of failure
			if( pi->GetPlayerStageStats()->m_bFailed )
				pi->GetPlayerStageStats()->SetLifeRecordAt( 0, STATSMAN->m_CurStageStats.m_fGameplaySeconds );
		}

		/* If all players have *really* failed (bFailed, not the life meter or
		 * bFailedEarlier): */
		const bool bAllReallyFailed = STATSMAN->m_CurStageStats.AllFailed();
		const bool bStopCourseEarly = STOP_COURSE_EARLY;
		const bool bIsLastSong = IsLastSong();

		LOG->Trace( "bAllReallyFailed = %d, bStopCourseEarly = %d, bIsLastSong = %d", bAllReallyFailed, bStopCourseEarly, bIsLastSong );

		if( bStopCourseEarly || bAllReallyFailed || bIsLastSong )
		{
			// Time to leave from ScreenGameplay
			HandleScreenMessage( SM_LeaveGameplay );
		}
		else
		{
			/* Load the next song in the course. */
			HandleScreenMessage( SM_StartLoadingNextSong );
			return;
		}
	}
	else if( SM == SM_LeaveGameplay )
	{
		// update dancing characters for win / lose
		DancingCharacters *pDancers = NULL;
		if( m_pSongBackground )
			pDancers = m_pSongBackground->GetDancingCharacters();
		if( pDancers )
		{
			FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
			{
				/* XXX: In battle modes, switch( GAMESTATE->GetStageResult(p) ). */
				if( pi->GetPlayerStageStats()->m_bFailed )
					pDancers->Change2DAnimState( pi->m_pn, AS2D_FAIL ); // fail anim
				else if( pi->m_pLifeMeter && pi->GetPlayerState()->m_HealthState == HealthState_Hot )
					pDancers->Change2DAnimState( pi->m_pn, AS2D_WINFEVER ); // full life pass anim
				else
					pDancers->Change2DAnimState( pi->m_pn, AS2D_WIN ); // pass anim
			}
		}

		/* End round. */
		if( m_DancingState == STATE_OUTRO )	// ScreenGameplay already ended
			return;		// ignore
		m_DancingState = STATE_OUTRO;
		AbortGiveUp( false );

		GAMESTATE->RemoveAllActiveAttacks();
		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
		{
			if( pi->m_pActiveAttackList )
				pi->m_pActiveAttackList->Refresh();
		}

		LIGHTSMAN->SetLightsMode( LIGHTSMODE_ALL_CLEARED );

		bool bAllReallyFailed = STATSMAN->m_CurStageStats.AllFailed();

		if( bAllReallyFailed )
		{
			this->PostScreenMessage( SM_BeginFailed, 0 );
			return;
		}

		TweenOffScreen();

		m_Out.StartTransitioning( SM_DoNextScreen );

		// do they deserve an extra stage?
		if( GAMESTATE->HasEarnedExtraStage() )
			SOUND->PlayOnceFromAnnouncer( "gameplay extra" );
		else
			SOUND->PlayOnceFromAnnouncer( "gameplay cleared" );
	}
	else if( SM == SM_StartLoadingNextSong )
	{	
		/* Next song. */

		// give a little life back between stages
		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
		{
			if( pi->m_pLifeMeter )
				pi->m_pLifeMeter->OnSongEnded();	
		}

		GAMESTATE->m_bLoadingNextSong = true;
		MESSAGEMAN->Broadcast( "BeforeLoadingNextCourseSong" );
		m_NextSong.Reset();
		m_NextSong.PlayCommand( "Start" );
		m_NextSong.StartTransitioning( SM_LoadNextSong );
		MESSAGEMAN->Broadcast( "ChangeCourseSongIn" );
	}
	else if( SM == SM_LoadNextSong )
	{
		m_pSoundMusic->Stop();
		SongFinished();

		MESSAGEMAN->Broadcast( "ChangeCourseSongOut" );

		GAMESTATE->m_bLoadingNextSong = false;
		LoadNextSong();

		m_NextSong.Reset();
		m_NextSong.PlayCommand( "Finish" );
		m_NextSong.StartTransitioning( SM_None );

		StartPlayingSong( MIN_SECONDS_TO_STEP_NEXT_SONG, 0 );
	}
	else if( SM == SM_PlayToasty )
	{
		if( g_bEasterEggs )
			if( !m_Toasty.IsTransitioning()  &&  !m_Toasty.IsFinished() )	// don't play if we've already played it once
				m_Toasty.StartTransitioning();
	}
	else if( SM >= SM_100Combo && SM <= SM_1000Combo )
	{
		int iCombo = (SM-SM_100Combo+1)*100;
		PlayAnnouncer( ssprintf("gameplay %d combo",iCombo), 2 );
	}
	else if( SM == SM_ComboStopped )
	{
		PlayAnnouncer( "gameplay combo stopped", 2 );
	}
	else if( SM == SM_ComboContinuing )
	{
		PlayAnnouncer( "gameplay combo overflow", 2 );
	}
	else if( SM >= SM_BattleTrickLevel1 && SM <= SM_BattleTrickLevel3 )
	{
		int iTrickLevel = SM-SM_BattleTrickLevel1+1;
		PlayAnnouncer( ssprintf("gameplay battle trick level%d",iTrickLevel), 3 );
		if( SM == SM_BattleTrickLevel1 ) m_soundBattleTrickLevel1.Play();
		else if( SM == SM_BattleTrickLevel2 ) m_soundBattleTrickLevel2.Play();
		else if( SM == SM_BattleTrickLevel3 ) m_soundBattleTrickLevel3.Play();
	}
	else if( SM >= SM_BattleDamageLevel1 && SM <= SM_BattleDamageLevel3 )
	{
		int iDamageLevel = SM-SM_BattleDamageLevel1+1;
		PlayAnnouncer( ssprintf("gameplay battle damage level%d",iDamageLevel), 3 );
	}
	else if( SM == SM_DoPrevScreen )
	{
		SongFinished();
		this->StageFinished( true );

		m_sNextScreen = GetPrevScreen();

		if( AdjustSync::IsSyncDataChanged() )
			ScreenSaveSync::PromptSaveSync( SM_GoToPrevScreen );
		else
			HandleScreenMessage( SM_GoToPrevScreen );
	}
	else if( SM == SM_DoNextScreen )
	{
		SongFinished();
		this->StageFinished( false );
		//SaveReplay();

		if( AdjustSync::IsSyncDataChanged() )
			ScreenSaveSync::PromptSaveSync( SM_GoToNextScreen );
		else
			HandleScreenMessage( SM_GoToNextScreen );
	}
	else if( SM == SM_GainFocus )
	{
		/* We do this ourself. */
		SOUND->HandleSongTimer( false );
	}
	else if( SM == SM_LoseFocus )
	{
		/* We might have turned the song timer off.  Be sure to turn it back on. */
		SOUND->HandleSongTimer( true );
	}
	else if( SM == SM_BeginFailed )
	{
		m_DancingState = STATE_OUTRO;
		AbortGiveUp( false );
		m_GameplayAssist.StopPlaying(); /* Stop any queued assist ticks. */
		TweenOffScreen();
		m_Failed.StartTransitioning( SM_DoNextScreen );

		// show the survive time if extra stage
		if( GAMESTATE->IsAnExtraStage() )
		{
			float fMaxAliveSeconds = 0;
			FOREACH_EnabledPlayer(p)
				fMaxAliveSeconds = max( fMaxAliveSeconds, STATSMAN->m_CurStageStats.m_player[p].m_fAliveSeconds );
			m_textSurviveTime.SetText( "TIME: " + SecondsToMMSSMsMs(fMaxAliveSeconds) );
			ON_COMMAND( m_textSurviveTime );
		}
		
		if( GAMESTATE->IsCourseMode() )
		{
			if( GAMESTATE->GetCourseSongIndex() >= int(m_apSongsQueue.size() / 2) )
				SOUND->PlayOnceFromAnnouncer( "gameplay oni failed halfway" );
			else
				SOUND->PlayOnceFromAnnouncer( "gameplay oni failed" );
		}
		else
		{
			SOUND->PlayOnceFromAnnouncer( "gameplay failed" );
		}
	}
	else if( SM == SM_Pause )
	{
		/* Ignore SM_Pause when in demonstration. */
		if( GAMESTATE->m_bDemonstrationOrJukebox )
			return;

		if( !m_bPaused )
			PauseGame( true );
	}

	ScreenWithMenuElements::HandleScreenMessage( SM );
}

void ScreenGameplay::HandleMessage( const Message &msg )
{
	if( msg == "Judgment" )
	{
		PlayerNumber pn;
		msg.GetParam( "Player", pn );

		FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
		{
			if( pi->m_pn != pn )
				continue;
			if( !pi->GetPlayerState()->m_PlayerOptions.GetCurrent().m_bMuteOnError )
				continue;

			RageSoundReader *pSoundReader = m_AutoKeysounds.GetPlayerSound( pn );
			if( pSoundReader == NULL )
				pSoundReader = m_AutoKeysounds.GetSharedSound();

			HoldNoteScore hns;
			msg.GetParam( "HoldNoteScore", hns );
			TapNoteScore tns;
			msg.GetParam( "TapNoteScore", tns );

			bool bOn = false;
			if( hns != HoldNoteScore_Invalid )
				bOn = hns != HNS_LetGo;
			else
				bOn = tns != TNS_Miss;

			if( pSoundReader )
				pSoundReader->SetProperty( "Volume", bOn? 1.0f:0.0f );
		}
	}

	ScreenWithMenuElements::HandleMessage( msg );
}
 
void ScreenGameplay::Cancel( ScreenMessage smSendWhenDone )
{
	m_pSoundMusic->Stop();

	ScreenWithMenuElements::Cancel( smSendWhenDone );
}

Song *ScreenGameplay::GetNextCourseSong() const
{
	ASSERT( GAMESTATE->IsCourseMode() );

	int iPlaySongIndex = GAMESTATE->GetCourseSongIndex()+1;
	iPlaySongIndex %= m_apSongsQueue.size();

	return m_apSongsQueue[iPlaySongIndex];
}

PlayerInfo *ScreenGameplay::GetPlayerInfo( PlayerNumber pn )
{
	FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
	{
		if( pi->m_pn == pn )
			return &*pi;
	}
	return NULL;
}

PlayerInfo *ScreenGameplay::GetDummyPlayerInfo( int iDummyIndex )
{
	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		if( pi->m_bIsDummy  &&  pi->m_iDummyIndex == iDummyIndex )
			return &*pi;
	}
	return NULL;
}

void ScreenGameplay::SaveReplay()
{
	FOREACH_HumanPlayer( pn )
	{
		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
		{
			XNode *p = pi->m_pPlayer->GetNoteData().CreateNode();

			//
			// Find a file name for the screenshot
			//
			FILEMAN->FlushDirCache( "Save/" );

			vector<RString> files;
			GetDirListing( "Save/replay*", files, false, false );
			sort( files.begin(), files.end() );

			/* Files should be of the form "replay#####.xxx". */
			int iIndex = 0;

			for( int i = files.size()-1; i >= 0; --i )
			{
				static Regex re( "^replay([0-9]{5})\\....$" );
				vector<RString> matches;
				if( !re.Compare( files[i], matches ) )
					continue;

				ASSERT( matches.size() == 1 );
				iIndex = atoi( matches[0] )+1;
				break;
			}

			RString sFileName = ssprintf( "replay%05d.xml", iIndex );

			XmlFileUtil::SaveToFile( p, "Save/"+sFileName );
			SAFE_DELETE( p );
			return;
		}
	}
}


// lua start
#include "LuaBinding.h"

class LunaScreenGameplay: public Luna<ScreenGameplay>
{
public:
	static int GetNextCourseSong( T* p, lua_State *L ) { p->GetNextCourseSong()->PushSelf(L); return 1; }
	static int Center1Player( T* p, lua_State *L ) { lua_pushboolean( L, p->Center1Player() ); return 1; }
	static int GetLifeMeter( T* p, lua_State *L )
	{
		PlayerNumber pn = Enum::Check<PlayerNumber>( L, 1 );

		PlayerInfo *pi = p->GetPlayerInfo(pn);
		if( pi == NULL )
			return 0;
		LifeMeter *pLM = pi->m_pLifeMeter;
		if( pLM == NULL )
			return 0;

		pLM->PushSelf( L );
		return 1;
	}
	static int GetDummyPlayerInfo( T* p, lua_State *L )
	{
		int iDummyIndex = IArg(1);
		PlayerInfo *pi = p->GetDummyPlayerInfo(iDummyIndex);
		if( pi == NULL )
			return 0;
		pi->PushSelf( L );
		return 1;
	}

	LunaScreenGameplay()
	{
  		ADD_METHOD( GetNextCourseSong );
  		ADD_METHOD( Center1Player );
  		ADD_METHOD( GetLifeMeter );
  		ADD_METHOD( GetDummyPlayerInfo );
	}
};

LUA_REGISTER_DERIVED_CLASS( ScreenGameplay, ScreenWithMenuElements )


class LunaPlayerInfo: public Luna<PlayerInfo>
{
public:
	static int GetLifeMeter( T* p, lua_State *L ) { p->m_pLifeMeter->PushSelf(L); return 1; }
	static int GetStepsQueue( T* p, lua_State *L )
	{
		Steps *pSteps = p->m_vpStepsQueue[ IArg(1) ];
		pSteps->PushSelf(L);
		return 1;
	}

	LunaPlayerInfo()
	{
  		ADD_METHOD( GetLifeMeter );
  		ADD_METHOD( GetStepsQueue );
	}
};

LUA_REGISTER_CLASS( PlayerInfo )
// lua end

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
