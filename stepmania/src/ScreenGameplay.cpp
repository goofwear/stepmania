#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenGameplay

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenGameplay.h"
#include "SongManager.h"
#include "ScreenManager.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "SongManager.h"
#include "RageLog.h"
#include "AnnouncerManager.h"
#include "LifeMeterBar.h"
#include "LifeMeterBattery.h"
#include "GameState.h"
#include "ScoreDisplayNormal.h"
#include "ScoreDisplayPercentage.h"
#include "ScoreDisplayOni.h"
#include "ScoreDisplayBattle.h"
#include "ScoreDisplayRave.h"
#include "ScreenPrompt.h"
#include "GrooveRadar.h"
#include "NotesLoaderSM.h"
#include "ThemeManager.h"
#include "RageTimer.h"
#include "ScoreKeeperMAX2.h"
#include "ScoreKeeperRave.h"
#include "NoteFieldPositioning.h"
#include "LyricsLoader.h"
#include "ActorUtil.h"
#include "NoteSkinManager.h"
#include "RageTextureManager.h"
#include "RageSounds.h"
#include "CombinedLifeMeterTug.h"
#include "Inventory.h"
#include "Course.h"
#include "NoteDataUtil.h"
#include "UnlockSystem.h"
#include "LightsManager.h"

//
// Defines
//
#define PREV_SCREEN( play_mode )				THEME->GetMetric ("ScreenGameplay","PrevScreen"+Capitalize(PlayModeToString(play_mode)))
#define NEXT_SCREEN( play_mode )				THEME->GetMetric ("ScreenGameplay","NextScreen"+Capitalize(PlayModeToString(play_mode)))
#define SHOW_LIFE_METER_FOR_DISABLED_PLAYERS	THEME->GetMetricB("ScreenGameplay","ShowLifeMeterForDisabledPlayers")
#define EVAL_ON_FAIL							THEME->GetMetricB("ScreenGameplay","ShowEvaluationOnFail")

CachedThemeMetricF SECONDS_BETWEEN_COMMENTS	("ScreenGameplay","SecondsBetweenComments");
CachedThemeMetricF G_TICK_EARLY_SECONDS		("ScreenGameplay","TickEarlySeconds");

/* Global, so it's accessible from ShowSavePrompt: */
static float g_fOldOffset;  // used on offset screen to calculate difference

const ScreenMessage	SM_PlayReady			= ScreenMessage(SM_User+0);
const ScreenMessage	SM_PlayGo				= ScreenMessage(SM_User+1);


// received while STATE_DANCING
const ScreenMessage	SM_NotesEnded			= ScreenMessage(SM_User+10);
const ScreenMessage	SM_LoadNextSong			= ScreenMessage(SM_User+11);

// received while STATE_OUTRO
const ScreenMessage	SM_SaveChangedBeforeGoingBack	= ScreenMessage(SM_User+20);
const ScreenMessage	SM_GoToScreenAfterBack	= ScreenMessage(SM_User+21);
const ScreenMessage	SM_GoToStateAfterCleared= ScreenMessage(SM_User+22);

const ScreenMessage	SM_BeginFailed			= ScreenMessage(SM_User+30);
const ScreenMessage	SM_GoToScreenAfterFail	= ScreenMessage(SM_User+31);

// received while STATE_INTRO
const ScreenMessage	SM_StartHereWeGo		= ScreenMessage(SM_User+40);
const ScreenMessage	SM_StopHereWeGo			= ScreenMessage(SM_User+41);


/* XXX: Not using sName yet here until I work out jukebox/demo. */
ScreenGameplay::ScreenGameplay( CString sName, bool bDemonstration ) : Screen("ScreenGameplay")
{
	LOG->Trace( "ScreenGameplay::ScreenGameplay()" );

	int p;

	m_bDemonstration = bDemonstration;

	if( m_bDemonstration )
		LIGHTSMAN->SetLightMode( LIGHTMODE_DEMONSTRATION );
	else
		LIGHTSMAN->SetLightMode( LIGHTMODE_GAMEPLAY );

	SECONDS_BETWEEN_COMMENTS.Refresh();
	G_TICK_EARLY_SECONDS.Refresh();

	//need to initialize these before checking for demonstration mode
	//otherwise destructor will try to delete possibly invalid pointers

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		m_pLifeMeter[p] = NULL;
		m_pPrimaryScoreDisplay[p] = NULL;
		m_pSecondaryScoreDisplay[p] = NULL;
		m_pPrimaryScoreKeeper[p] = NULL;
		m_pSecondaryScoreKeeper[p] = NULL;
		m_pInventory[p] = NULL ;
	}
	m_pCombinedLifeMeter = NULL;

	if( GAMESTATE->m_pCurSong == NULL && GAMESTATE->m_pCurCourse == NULL )
		return;	// ScreenDemonstration will move us to the next scren.  We just need to survive for one update without crashing.

	/* Save selected options before we change them. */
	GAMESTATE->StoreSelectedOptions();

	GAMESTATE->ResetStageStatistics();




	// fill in difficulty of CPU players with that of the first human player
	for( p=0; p<NUM_PLAYERS; p++ )
		if( GAMESTATE->IsCpuPlayer(p) )
			GAMESTATE->m_pCurNotes[p] = GAMESTATE->m_pCurNotes[ GAMESTATE->GetFirstHumanPlayer() ];

	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		{
			// cache NoteSkin graphics
			CStringArray asNames;
			NOTESKIN->GetNoteSkinNames( asNames );
			for( unsigned i=0; i<asNames.size(); i++ )
			{
				CString sDir = NOTESKIN->GetNoteSkinDir( asNames[i] );
				CStringArray asGraphicPaths;
				GetDirListing( sDir+"*.png", asGraphicPaths, false, true ); 
				GetDirListing( sDir+"*.jpg", asGraphicPaths, false, true ); 
				GetDirListing( sDir+"*.gif", asGraphicPaths, false, true ); 
				GetDirListing( sDir+"*.bmp", asGraphicPaths, false, true ); 
				for( unsigned j=0; j<asGraphicPaths.size(); j++ )
					TEXTUREMAN->CacheTexture( asGraphicPaths[j] );
			}
		}
		break;
	}



	//
	// fill in m_apSongsQueue, m_apNotesQueue, m_asModifiersQueue
	//
	if( GAMESTATE->IsCourseMode() )
	{
		const StepsType st = GAMESTATE->GetCurrentStyleDef()->m_StepsType;
		/* Increment the play count. */
		for( int mc = 0; mc < NUM_MEMORY_CARDS; ++mc )
			++GAMESTATE->m_pCurCourse->m_MemCardDatas[st][mc].iNumTimesPlayed;

		vector<Course::Info> ci;
		GAMESTATE->m_pCurCourse->GetCourseInfo( GAMESTATE->GetCurrentStyleDef()->m_StepsType, ci );
		for( int p=0; p<NUM_PLAYERS; p++ )
		{
			m_apNotesQueue[p].clear();
			m_asModifiersQueue[p].clear();
			for( unsigned c=0; c<ci.size(); ++c )
			{
				m_apNotesQueue[p].push_back( ci[c].pNotes );
				AttackArray a;
				ci[c].GetAttackArray( a );
				m_asModifiersQueue[p].push_back( a );
			}
		}
		m_apSongsQueue.clear();
		for( unsigned c=0; c<ci.size(); ++c )
			m_apSongsQueue.push_back( ci[c].pSong );
	}
	else
	{
		m_apSongsQueue.push_back( GAMESTATE->m_pCurSong );
		for( int p=0; p<NUM_PLAYERS; p++ )
		{
			m_apNotesQueue[p].push_back( GAMESTATE->m_pCurNotes[p] );
			m_asModifiersQueue[p].push_back( AttackArray() );
		}
	}


	if( !GAMESTATE->IsCourseMode() )
		GAMESTATE->m_CurStageStats.pSong = GAMESTATE->m_pCurSong;
	else
		GAMESTATE->m_CurStageStats.pSong = NULL;

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled(p) )
			continue;	// skip

		ASSERT( !m_apNotesQueue[p].empty() );
		GAMESTATE->m_CurStageStats.pSteps[p] = m_apNotesQueue[p][0];
		GAMESTATE->m_CurStageStats.iMeter[p] = m_apNotesQueue[p][0]->GetMeter();
	}

	if( GAMESTATE->IsExtraStage() )
		GAMESTATE->m_CurStageStats.StageType = StageStats::STAGE_EXTRA;
	else if( GAMESTATE->IsExtraStage2() )
		GAMESTATE->m_CurStageStats.StageType = StageStats::STAGE_EXTRA2;
	else
		GAMESTATE->m_CurStageStats.StageType = StageStats::STAGE_NORMAL;
	
	//
	// Init ScoreKeepers
	//
	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled(p) )
			continue;	// skip
		
		switch( PREFSMAN->m_iScoringType )
		{
		case PrefsManager::SCORING_MAX2:
		case PrefsManager::SCORING_5TH:
			m_pPrimaryScoreKeeper[p] = new ScoreKeeperMAX2( m_apSongsQueue, m_apNotesQueue[p], m_asModifiersQueue[p], (PlayerNumber)p );
			break;
		default: ASSERT(0);
		}

		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_RAVE:
			m_pSecondaryScoreKeeper[p] = new ScoreKeeperRave( (PlayerNumber)p );
			break;
		}
	}

	m_bChangedOffsetOrBPM = GAMESTATE->m_SongOptions.m_bAutoSync;

	m_DancingState = STATE_INTRO;

	// Set this in LoadNextSong()
	//m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
	
	m_bZeroDeltaOnNextUpdate = false;
	
	 const bool bExtra = GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2();
	// init old offset in case offset changes in song
	if( GAMESTATE->IsCourseMode() )
		g_fOldOffset = -1000;
	else
		g_fOldOffset = GAMESTATE->m_pCurSong->m_fBeat0OffsetInSeconds;



	m_Background.SetDiffuse( RageColor(0.4f,0.4f,0.4f,1) );
	this->AddChild( &m_Background );
	
	m_sprStaticBackground.SetName( "StaticBG" );
	m_sprStaticBackground.Load( THEME->GetPathToG("ScreenGameplay Static Background"));
	SET_XY( m_sprStaticBackground );
	this->AddChild(&m_sprStaticBackground);

	if( !bDemonstration )	// only load if we're going to use it
	{
		m_Toasty.Load( THEME->GetPathToB("ScreenGameplay toasty") );
		this->AddChild( &m_Toasty );
	}

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled(PlayerNumber(p)) )
			continue;

		float fPlayerX = (float) GAMESTATE->GetCurrentStyleDef()->m_iCenterX[p];

		/* Perhaps this should be handled better by defining a new
		 * StyleType for ONE_PLAYER_ONE_CREDIT_AND_ONE_COMPUTER,
		 * but for now just ignore SoloSingles when it's Battle or Rave
		 * Mode.  This doesn't begin to address two-player solo (6 arrows) */
		if( PREFSMAN->m_bSoloSingle && 
			GAMESTATE->m_PlayMode != PLAY_MODE_BATTLE &&
			GAMESTATE->m_PlayMode != PLAY_MODE_RAVE &&
			GAMESTATE->GetCurrentStyleDef()->m_StyleType == StyleDef::ONE_PLAYER_ONE_CREDIT )
			fPlayerX = SCREEN_WIDTH/2;

		m_Player[p].SetX( fPlayerX );
		this->AddChild( &m_Player[p] );
	
		m_TimingAssist.Load((PlayerNumber)p, &m_Player[p]);

		m_sprOniGameOver[p].Load( THEME->GetPathToG("ScreenGameplay oni gameover") );
		m_sprOniGameOver[p].SetX( fPlayerX );
		m_sprOniGameOver[p].SetY( SCREEN_TOP - m_sprOniGameOver[p].GetZoomedHeight()/2 );
		m_sprOniGameOver[p].SetDiffuse( RageColor(1,1,1,0) );	// 0 alpha so we don't waste time drawing while not visible
		this->AddChild( &m_sprOniGameOver[p] );
	}

	this->AddChild(&m_TimingAssist);

	this->AddChild( &m_NextSongIn );

	this->AddChild( &m_NextSongOut );



	//
	// Add LifeFrame
	//
	CString sLifeFrameName;
	if( bExtra )
		sLifeFrameName = "ScreenGameplay extra life frame";
	else if( GAMESTATE->m_SongOptions.m_LifeType == SongOptions::LIFE_BATTERY )
		sLifeFrameName = "ScreenGameplay oni life frame";
	else 
		sLifeFrameName = "ScreenGameplay life frame";
	m_sprLifeFrame.Load( THEME->GetPathToG(sLifeFrameName) );
	m_sprLifeFrame.SetName( bExtra?"LifeFrameExtra":"LifeFrame" );
	SET_XY( m_sprLifeFrame );
	this->AddChild( &m_sprLifeFrame );

	//
	// Add score frame
	//
	CString sScoreFrameName;
	if( bExtra )
		sScoreFrameName = "ScreenGameplay extra score frame";
	else if( GAMESTATE->m_SongOptions.m_LifeType == SongOptions::LIFE_BATTERY )
		sScoreFrameName = "ScreenGameplay oni score frame";
	else 
		sScoreFrameName = "ScreenGameplay score frame";
	m_sprScoreFrame.Load( THEME->GetPathToG(sScoreFrameName) );
	m_sprScoreFrame.SetName( ssprintf("ScoreFrame%s",bExtra?"Extra":"") );
	SET_XY( m_sprScoreFrame );
	this->AddChild( &m_sprScoreFrame );


	//
	// Add combined life meter
	//
	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		m_pCombinedLifeMeter = new CombinedLifeMeterTug;
		m_pCombinedLifeMeter->SetName( ssprintf("CombinedLife%s",bExtra?"Extra":"") );
		SET_XY( *m_pCombinedLifeMeter );
		this->AddChild( m_pCombinedLifeMeter );		
		break;
	}

	//
	// Add individual life meter
	//
	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_ARCADE:
	case PLAY_MODE_ONI:
	case PLAY_MODE_NONSTOP:
	case PLAY_MODE_ENDLESS:
		for( p=0; p<NUM_PLAYERS; p++ )
		{
			if( !GAMESTATE->IsPlayerEnabled(p) && !SHOW_LIFE_METER_FOR_DISABLED_PLAYERS )
				continue;	// skip

			switch( GAMESTATE->m_SongOptions.m_LifeType )
			{
			case SongOptions::LIFE_BAR:
				m_pLifeMeter[p] = new LifeMeterBar;
				break;
			case SongOptions::LIFE_BATTERY:
				m_pLifeMeter[p] = new LifeMeterBattery;
				break;
			default:
				ASSERT(0);
			}

			m_pLifeMeter[p]->Load( (PlayerNumber)p );
			m_pLifeMeter[p]->SetName( ssprintf("LifeP%d%s",p+1,bExtra?"Extra":"") );
			SET_XY( *m_pLifeMeter[p] );
			this->AddChild( m_pLifeMeter[p] );		
		}
		break;
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		break;
	}




	m_textSongTitle.LoadFromFont( THEME->GetPathToF("ScreenGameplay song title") );
	m_textSongTitle.EnableShadow( false );
	m_textSongTitle.SetName( "SongTitle" );
	SET_XY( m_textSongTitle );
	this->AddChild( &m_textSongTitle );

	m_MaxCombo.LoadFromNumbers( THEME->GetPathToN("ScreenGameplay max combo") );
	m_MaxCombo.SetName( "MaxCombo" );
	SET_XY( m_MaxCombo );
	m_MaxCombo.SetText( ssprintf("%d", GAMESTATE->m_CurStageStats.iMaxCombo[GAMESTATE->m_MasterPlayerNumber]) ); // TODO: Make this work for both players
	this->AddChild( &m_MaxCombo );

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled(p) )
			continue;

		//
		// primary score display
		//
		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_ARCADE:
		case PLAY_MODE_NONSTOP:
		case PLAY_MODE_BATTLE:
		case PLAY_MODE_RAVE:
			if( PREFSMAN->m_bPercentageScoring )
				m_pPrimaryScoreDisplay[p] = new ScoreDisplayPercentage;
			else
				m_pPrimaryScoreDisplay[p] = new ScoreDisplayNormal;
			break;
		case PLAY_MODE_ONI:
		case PLAY_MODE_ENDLESS:
			m_pPrimaryScoreDisplay[p] = new ScoreDisplayOni;
			break;
		default:
			ASSERT(0);
		}

		m_pPrimaryScoreDisplay[p]->Init( (PlayerNumber)p );
		m_pPrimaryScoreDisplay[p]->SetName( ssprintf("ScoreP%d%s",p+1,bExtra?"Extra":"") );
		SET_XY( *m_pPrimaryScoreDisplay[p] );
		this->AddChild( m_pPrimaryScoreDisplay[p] );

	
		//
		// secondary score display
		//
		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_RAVE:
			m_pSecondaryScoreDisplay[p] = new ScoreDisplayRave;
			break;
		}

		if( m_pSecondaryScoreDisplay[p] )
		{
			m_pSecondaryScoreDisplay[p]->Init( (PlayerNumber)p );
			m_pSecondaryScoreDisplay[p]->SetName( ssprintf("SecondaryScoreP%d%s",p+1,bExtra?"Extra":"") );
			SET_XY( *m_pSecondaryScoreDisplay[p] );
			this->AddChild( m_pSecondaryScoreDisplay[p] );
		}
	}
	
	//
	// Add stage / SongNumber
	//
	m_sprStage.SetName( ssprintf("Stage%s",bExtra?"Extra":"") );
	SET_XY( m_sprStage );

	
	for( p=0; p<NUM_PLAYERS; p++ )
	{
		m_textCourseSongNumber[p].LoadFromNumbers( THEME->GetPathToN("ScreenGameplay song num") );
		m_textCourseSongNumber[p].EnableShadow( false );
		m_textCourseSongNumber[p].SetName( ssprintf("SongNumberP%d%s",p+1,bExtra?"Extra":"") );
		SET_XY( m_textCourseSongNumber[p] );
		m_textCourseSongNumber[p].SetText( "" );
		m_textCourseSongNumber[p].SetDiffuse( RageColor(0,0.5f,1,1) );	// light blue
	}

	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_ARCADE:
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		m_sprStage.Load( THEME->GetPathToG("ScreenGameplay stage "+GAMESTATE->GetStageText()) );
		this->AddChild( &m_sprStage );
		break;
	case PLAY_MODE_NONSTOP:
	case PLAY_MODE_ONI:
	case PLAY_MODE_ENDLESS:
		for( p=0; p<NUM_PLAYERS; p++ )
			if( GAMESTATE->IsPlayerEnabled(p) )
			{
				this->AddChild( &m_textCourseSongNumber[p] );
			}
		break;
	default:
		ASSERT(0);	// invalid GameMode
	}



	//
	// Player/Song options
	//
	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled(p) )
			continue;

		m_textPlayerOptions[p].LoadFromFont( THEME->GetPathToF("Common normal") );
		m_textPlayerOptions[p].EnableShadow( false );
		m_textPlayerOptions[p].SetName( ssprintf("PlayerOptionsP%d%s",p+1,bExtra?"Extra":"") );
		SET_XY( m_textPlayerOptions[p] );
		this->AddChild( &m_textPlayerOptions[p] );
	}

	m_textSongOptions.LoadFromFont( THEME->GetPathToF("Common normal") );
	m_textSongOptions.EnableShadow( false );
	m_textSongOptions.SetName( ssprintf("SongOptions%s",bExtra?"Extra":"") );
	SET_XY( m_textSongOptions );
	m_textSongOptions.SetText( GAMESTATE->m_SongOptions.GetString() );
	this->AddChild( &m_textSongOptions );





	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled(p) )
			continue;

		m_DifficultyIcon[p].Load( THEME->GetPathToG("ScreenGameplay difficulty icons 2x5") );
		
		/* Position it in LoadNextSong. */
		this->AddChild( &m_DifficultyIcon[p] );
	}


	if(PREFSMAN->m_bShowLyrics)
		this->AddChild(&m_LyricDisplay);
	

	m_textAutoPlay.LoadFromFont( THEME->GetPathToF("ScreenGameplay autoplay") );
	m_textAutoPlay.SetName( "AutoPlay" );
	SET_XY( m_textAutoPlay );
	if( !bDemonstration )	// only load if we're not in demonstration of jukebox
		this->AddChild( &m_textAutoPlay );
	UpdateAutoPlayText();
	

	m_BPMDisplay.SetName( "BPMDisplay" );
	m_BPMDisplay.Load();
	SET_XY( m_BPMDisplay );
	this->AddChild( &m_BPMDisplay );

	ZERO( m_pInventory );
	for( p=0; p<NUM_PLAYERS; p++ )
	{
//		switch( GAMESTATE->m_PlayMode )
//		{
//		case PLAY_MODE_BATTLE:
//			m_pInventory[p] = new Inventory;
//			m_pInventory[p]->Load( (PlayerNumber)p );
//			this->AddChild( m_pInventory[p] );
//			break;
//		}
	}

	
	if( !bDemonstration )	// only load if we're going to use it
	{
		m_Ready.Load( THEME->GetPathToB("ScreenGameplay ready") );
		this->AddChild( &m_Ready );

		m_Go.Load( THEME->GetPathToB("ScreenGameplay go") );
		this->AddChild( &m_Go );

		m_Cleared.Load( THEME->GetPathToB("ScreenGameplay cleared") );
		this->AddChild( &m_Cleared );

		m_Failed.Load( THEME->GetPathToB("ScreenGameplay failed") );
		this->AddChild( &m_Failed );

		if( PREFSMAN->m_bAllowExtraStage && GAMESTATE->IsFinalStage() )	// only load if we're going to use it
			m_Extra.Load( THEME->GetPathToB("ScreenGameplay extra1") );
		if( PREFSMAN->m_bAllowExtraStage && GAMESTATE->IsExtraStage() )	// only load if we're going to use it
			m_Extra.Load( THEME->GetPathToB("ScreenGameplay extra2") );
		this->AddChild( &m_Extra );

		// only load if we're going to use it
		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_BATTLE:
		case PLAY_MODE_RAVE:
			for( p=0; p<NUM_PLAYERS; p++ )
			{
				m_Win[p].Load( THEME->GetPathToB(ssprintf("ScreenGameplay win p%d",p+1)) );
				this->AddChild( &m_Win[p] );
			}
			m_Draw.Load( THEME->GetPathToB("ScreenGameplay draw") );
			this->AddChild( &m_Draw );
			break;
		}

		m_In.Load( THEME->GetPathToB("ScreenGameplay in") );
		this->AddChild( &m_In );


		m_textDebug.LoadFromFont( THEME->GetPathToF("Common normal") );
		m_textDebug.SetName( "Debug" );
		SET_XY( m_textDebug );
		this->AddChild( &m_textDebug );


		m_Back.Load( THEME->GetPathToB("Common back") );
		this->AddChild( &m_Back );


		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )	// only load if we're going to use it
		{
			m_textSurviveTime.LoadFromFont( THEME->GetPathToF("ScreenGameplay survive time") );
			m_textSurviveTime.EnableShadow( false );
			m_textSurviveTime.SetName( "SurviveTime" );
			SET_XY( m_textSurviveTime );
			m_textSurviveTime.SetDiffuse( RageColor(1,1,1,0) );
			this->AddChild( &m_textSurviveTime );
		}
	}


	/* LoadNextSong first, since that positions some elements which need to be
	 * positioned before we TweenOnScreen. */
	LoadNextSong();

	TweenOnScreen();


	if( !bDemonstration )	// only load if we're going to use it
	{
		m_soundOniDie.Load(				THEME->GetPathToS("ScreenGameplay oni die") );
		m_announcerReady.Load(			ANNOUNCER->GetPathTo("gameplay ready"), 1 );
		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			m_announcerHereWeGo.Load(	ANNOUNCER->GetPathTo("gameplay here we go extra"), 1 );
		else if( GAMESTATE->IsFinalStage() )
			m_announcerHereWeGo.Load(	ANNOUNCER->GetPathTo("gameplay here we go final"), 1 );
		else
			m_announcerHereWeGo.Load(	ANNOUNCER->GetPathTo("gameplay here we go normal"), 1 );
		m_announcerDanger.Load(			ANNOUNCER->GetPathTo("gameplay comment danger") );
		m_announcerGood.Load(			ANNOUNCER->GetPathTo("gameplay comment good") );
		m_announcerHot.Load(			ANNOUNCER->GetPathTo("gameplay comment hot") );
		m_announcerOni.Load(			ANNOUNCER->GetPathTo("gameplay comment oni") );

		m_announcer100Combo.Load(		ANNOUNCER->GetPathTo("gameplay 100 combo") );
		m_announcer200Combo.Load(		ANNOUNCER->GetPathTo("gameplay 200 combo") );
		m_announcer300Combo.Load(		ANNOUNCER->GetPathTo("gameplay 300 combo") );
		m_announcer400Combo.Load(		ANNOUNCER->GetPathTo("gameplay 400 combo") );
		m_announcer500Combo.Load(		ANNOUNCER->GetPathTo("gameplay 500 combo") );
		m_announcer600Combo.Load(		ANNOUNCER->GetPathTo("gameplay 600 combo") );
		m_announcer700Combo.Load(		ANNOUNCER->GetPathTo("gameplay 700 combo") );
		m_announcer800Combo.Load(		ANNOUNCER->GetPathTo("gameplay 800 combo") );
		m_announcer900Combo.Load(		ANNOUNCER->GetPathTo("gameplay 900 combo") );
		m_announcer1000Combo.Load(		ANNOUNCER->GetPathTo("gameplay 1000 combo") );
		m_announcerComboStopped.Load(	ANNOUNCER->GetPathTo("gameplay combo stopped") );
		m_announcerComboContinuing.Load(ANNOUNCER->GetPathTo("gameplay combo overflow") );
		m_soundAssistTick.Load(			THEME->GetPathToS("ScreenGameplay assist tick") );

		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_BATTLE:
			m_announcerBattleTrickLevel1.Load(	ANNOUNCER->GetPathTo("gameplay battle trick level1") );
			m_announcerBattleTrickLevel2.Load(	ANNOUNCER->GetPathTo("gameplay battle trick level2") );
			m_announcerBattleTrickLevel3.Load(	ANNOUNCER->GetPathTo("gameplay battle trick level3") );
			m_soundBattleTrickLevel1.Load(	THEME->GetPathToS("ScreenGameplay battle trick level1"), true );
			m_soundBattleTrickLevel2.Load(	THEME->GetPathToS("ScreenGameplay battle trick level2"), true );
			m_soundBattleTrickLevel3.Load(	THEME->GetPathToS("ScreenGameplay battle trick level3"), true );
			m_announcerBattleDamageLevel1.Load(	ANNOUNCER->GetPathTo("gameplay battle damage level1") );
			m_announcerBattleDamageLevel2.Load(	ANNOUNCER->GetPathTo("gameplay battle damage level2") );
			m_announcerBattleDamageLevel3.Load(	ANNOUNCER->GetPathTo("gameplay battle damage level3") );
			// HACK:  Load incorrect directory on purpose for now.
			m_announcerBattleDie.Load(	ANNOUNCER->GetPathTo("gameplay battle damage level3") );
			break;
		}
	}

	m_GiveUpTimer.SetZero();

	// Get the transitions rolling on the first update.
	// We can't do this in the constructor because ScreenGameplay is constructed 
	// in the middle of ScreenStage.
}

ScreenGameplay::~ScreenGameplay()
{
	LOG->Trace( "ScreenGameplay::~ScreenGameplay()" );
	
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		SAFE_DELETE( m_pLifeMeter[p] );
		SAFE_DELETE( m_pPrimaryScoreDisplay[p] );
		SAFE_DELETE( m_pSecondaryScoreDisplay[p] );
		SAFE_DELETE( m_pSecondaryScoreDisplay[p] );
		SAFE_DELETE( m_pPrimaryScoreKeeper[p] );
		SAFE_DELETE( m_pSecondaryScoreKeeper[p] );
		SAFE_DELETE( m_pInventory[p] );
	}
	SAFE_DELETE( m_pCombinedLifeMeter );

	m_soundMusic.StopPlaying();
}

bool ScreenGameplay::IsLastSong()
{
	if( GAMESTATE->m_pCurCourse  &&  GAMESTATE->m_pCurCourse->m_bRepeat )
		return false;
	return GAMESTATE->GetCourseSongIndex()+1 == (int)m_apSongsQueue.size(); // GetCourseSongIndex() is 0-based but size() is not
}

void ScreenGameplay::LoadNextSong()
{
	GAMESTATE->ResetMusicStatistics();
	int p;
	for( p=0; p<NUM_PLAYERS; p++ )
		if( GAMESTATE->IsPlayerEnabled(p) )
		{
			GAMESTATE->m_CurStageStats.iSongsPlayed[p]++;
			m_textCourseSongNumber[p].SetText( ssprintf("%d", GAMESTATE->m_CurStageStats.iSongsPlayed[p]) );
		}

	int iPlaySongIndex = GAMESTATE->GetCourseSongIndex();
	iPlaySongIndex %= m_apSongsQueue.size();
	GAMESTATE->m_pCurSong = m_apSongsQueue[iPlaySongIndex];

	// Restore the player's originally selected options.
	GAMESTATE->RemoveAllActiveAttacks();
	GAMESTATE->RestoreSelectedOptions();

	m_textSongOptions.SetText( GAMESTATE->m_SongOptions.GetString() );

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled(p) )
			continue;

		/* This is the first beat that can be changed without it being visible.  Until
		 * we draw for the first time, any beat can be changed. */
		GAMESTATE->m_fLastDrawnBeat[p] = -100;

		GAMESTATE->m_pCurNotes[p] = m_apNotesQueue[p][iPlaySongIndex];

		/* Increment the play count even if the player fails.  (It's still popular,
		 * even if the people playing it aren't good at it.) */
		for( int mc = 0; mc < NUM_MEMORY_CARDS; ++mc )
			++GAMESTATE->m_pCurNotes[p]->m_MemCardDatas[mc].iNumTimesPlayed;

		// Put course options into effect.
		for( unsigned i=0; i<m_asModifiersQueue[p][iPlaySongIndex].size(); ++i )
		{
			GAMESTATE->LaunchAttack( (PlayerNumber)p, m_asModifiersQueue[p][iPlaySongIndex][i] );
			GAMESTATE->m_SongOptions.FromString( m_asModifiersQueue[p][iPlaySongIndex][i].sModifier );
		}

		/* Hack: Course modifiers that are set to start immediately shouldn't tween on. */
		for( unsigned s=0; s<GAMESTATE->m_ActiveAttacks[p].size(); s++ )
		{
			if( GAMESTATE->m_ActiveAttacks[p][s].fStartSecond == 0 )
				GAMESTATE->m_ActiveAttacks[p][s].fStartSecond = -1;
		}
		GAMESTATE->RebuildPlayerOptionsFromActiveAttacks( (PlayerNumber)p );
		GAMESTATE->m_CurrentPlayerOptions[p] = GAMESTATE->m_PlayerOptions[p];

		m_textPlayerOptions[p].SetText( GAMESTATE->m_PlayerOptions[p].GetString() );

		// reset oni game over graphic
		m_sprOniGameOver[p].SetY( SCREEN_TOP - m_sprOniGameOver[p].GetZoomedHeight()/2 );
		m_sprOniGameOver[p].SetDiffuse( RageColor(1,1,1,0) );	// 0 alpha so we don't waste time drawing while not visible

		if( GAMESTATE->m_SongOptions.m_LifeType==SongOptions::LIFE_BATTERY && GAMESTATE->m_CurStageStats.bFailed[p] )	// already failed
			ShowOniGameOver((PlayerNumber)p);

		if( GAMESTATE->m_SongOptions.m_LifeType==SongOptions::LIFE_BAR && GAMESTATE->m_PlayMode == PLAY_MODE_ARCADE && !PREFSMAN->m_bEventMode && !m_bDemonstration)
		{
			m_pLifeMeter[p]->UpdateNonstopLifebar(
				GAMESTATE->GetStageIndex(), 
				PREFSMAN->m_iNumArcadeStages, 
				PREFSMAN->m_iProgressiveStageLifebar);
		}
		if( GAMESTATE->m_SongOptions.m_LifeType==SongOptions::LIFE_BAR && GAMESTATE->m_PlayMode == PLAY_MODE_NONSTOP )
		{
			m_pLifeMeter[p]->UpdateNonstopLifebar(
				GAMESTATE->GetCourseSongIndex(), 
				GAMESTATE->m_pCurCourse->GetEstimatedNumStages(),
				PREFSMAN->m_iProgressiveNonstopLifebar);
		}

		m_DifficultyIcon[p].SetFromNotes( PlayerNumber(p), GAMESTATE->m_pCurNotes[p] );


		NoteData pOriginalNoteData;
		GAMESTATE->m_pCurNotes[p]->GetNoteData( &pOriginalNoteData );
		
		const StyleDef* pStyleDef = GAMESTATE->GetCurrentStyleDef();
		NoteData pNewNoteData;
		pStyleDef->GetTransformedNoteDataForStyle( (PlayerNumber)p, &pOriginalNoteData, &pNewNoteData );
		m_Player[p].Load( (PlayerNumber)p, &pNewNoteData, m_pLifeMeter[p], m_pCombinedLifeMeter, m_pPrimaryScoreDisplay[p], m_pSecondaryScoreDisplay[p], m_pInventory[p], m_pPrimaryScoreKeeper[p], m_pSecondaryScoreKeeper[p] );

		/* The actual note data for scoring is the base class of Player.  This includes
		 * transforms, like Wide.  Otherwise, the scoring will operate on the wrong data. */
		m_pPrimaryScoreKeeper[p]->OnNextSong( GAMESTATE->GetCourseSongIndex(), GAMESTATE->m_pCurNotes[p], &m_Player[p] );
		if( m_pSecondaryScoreKeeper[p] )
			m_pSecondaryScoreKeeper[p]->OnNextSong( GAMESTATE->GetCourseSongIndex(), GAMESTATE->m_pCurNotes[p], &m_Player[p] );

		if( m_bDemonstration )
		{
			GAMESTATE->m_PlayerController[p] = PC_CPU;
			GAMESTATE->m_iCpuSkill[p] = 5;
		}
		else if( GAMESTATE->IsCpuPlayer(p) )
		{
			GAMESTATE->m_PlayerController[p] = PC_CPU;
			if( GAMESTATE->m_iCpuSkill[p] == -1 )
			{
				switch( GAMESTATE->m_pCurNotes[p]->GetDifficulty() )
				{
				case DIFFICULTY_BEGINNER:	GAMESTATE->m_iCpuSkill[p] = 1;	break;
				case DIFFICULTY_EASY:		GAMESTATE->m_iCpuSkill[p] = 3;	break;
				case DIFFICULTY_MEDIUM:		GAMESTATE->m_iCpuSkill[p] = 5;	break;
				case DIFFICULTY_HARD:		GAMESTATE->m_iCpuSkill[p] = 7;	break;
				case DIFFICULTY_CHALLENGE:	GAMESTATE->m_iCpuSkill[p] = 9;	break;
				default:	ASSERT(0);
				}
			}
		}
		else if( PREFSMAN->m_bAutoPlay )
			GAMESTATE->m_PlayerController[p] = PC_AUTOPLAY;
		else
			GAMESTATE->m_PlayerController[p] = PC_HUMAN;

		m_TimingAssist.Reset();
	}

	m_textSongTitle.SetText( GAMESTATE->m_pCurSong->m_sMainTitle );

	/* XXX: set it to the current BPM, not the range */
	/* What does this comment mean?  -Chris 
	 *
	 * We're in gameplay.  A BPM display should display the current BPM, updating
	 * it as it changes, instead of the "BPM preview" of ScreenSelectMusic.  That'd
	 * be used in IIDX, anyway.  (Havn't done this since I don't know what this is
	 * currently actually used for and don't feel like investigating it until it's
	 * needed.)
	 * -glenn
	 */
	m_BPMDisplay.SetBPM( GAMESTATE->m_pCurSong );

	const bool bExtra = GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2();
	const bool bReverse[NUM_PLAYERS] = { 
		GAMESTATE->m_PlayerOptions[0].m_fScrolls[PlayerOptions::SCROLL_REVERSE] == 1,
		GAMESTATE->m_PlayerOptions[1].m_fScrolls[PlayerOptions::SCROLL_REVERSE] == 1
	};

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled(PlayerNumber(p)) )
			continue;

		m_DifficultyIcon[p].SetName( ssprintf("DifficultyP%d%s%s",p+1,bExtra?"Extra":"",bReverse[p]?"Reverse":"") );
		SET_XY( m_DifficultyIcon[p] );
	}

	const bool bBothReverse = bReverse[PLAYER_1] && bReverse[PLAYER_2];
	const bool bOneReverse = !bBothReverse && (bReverse[PLAYER_1] || bReverse[PLAYER_2]);

	/* XXX: We want to put the lyrics out of the way, but it's likely that one
	 * player is in reverse and the other isn't.  What to do? */
	m_LyricDisplay.SetName( ssprintf( "Lyrics%s", bBothReverse? "Reverse": bOneReverse? "OneReverse": "") );
	SET_XY( m_LyricDisplay );

	/* Load the Oni transitions */
	m_NextSongIn.Load( THEME->GetPathToB("ScreenGameplay next song in") );
	// Instead, load this right before it's used
//	m_NextSongOut.Load( THEME->GetPathToB("ScreenGameplay next song out") );

	// Load lyrics
	// XXX: don't load this here
	LyricsLoader LL;
	if( GAMESTATE->m_pCurSong->HasLyrics()  )
		LL.LoadFromLRCFile(GAMESTATE->m_pCurSong->GetLyricsPath(), *GAMESTATE->m_pCurSong);

	
	m_soundMusic.SetAccurateSync();
	m_soundMusic.Load( GAMESTATE->m_pCurSong->GetMusicPath() );
	
	/* Set up song-specific graphics. */
	
	
	// Check to see if any players are in beginner mode.
	// Note: steps can be different if turn modifiers are used.
	if( PREFSMAN->m_bShowBeginnerHelper && BeginnerHelper::CanUse())
	{
		bool anybeginners = false;

		for( int pb=0; pb<NUM_PLAYERS; pb++ )
			if( GAMESTATE->IsHumanPlayer(pb) && GAMESTATE->m_pCurNotes[pb]->GetDifficulty() == DIFFICULTY_BEGINNER )
			{
				anybeginners = true;
				m_BeginnerHelper.AddPlayer( pb, &m_Player[pb] );
			}

		if(anybeginners && m_BeginnerHelper.Initialize( 2 ))	// Init for doubles
		{
				m_Background.Unload();	// BeginnerHelper has its own BG control.
				m_Background.StopAnimating();
				m_BeginnerHelper.SetX( CENTER_X );
				m_BeginnerHelper.SetY( CENTER_Y );
		}
		else
		{
			m_Background.LoadFromSong( GAMESTATE->m_pCurSong );
		}
	}
	else
	{
		m_Background.LoadFromSong( GAMESTATE->m_pCurSong );
	}

	m_Background.SetDiffuse( RageColor(0.5f,0.5f,0.5f,1) );
	m_Background.BeginTweening( 2 );
	m_Background.SetDiffuse( RageColor(1,1,1,1) );


	m_fTimeLeftBeforeDancingComment = GAMESTATE->m_pCurSong->m_fFirstBeat + SECONDS_BETWEEN_COMMENTS;


	/* m_soundMusic and m_Background take a very long time to load,
	 * so cap fDelta at 0 so m_NextSongIn will show up on screen.
	 * -Chris */
	m_bZeroDeltaOnNextUpdate = true;
}

float ScreenGameplay::StartPlayingSong(float MinTimeToNotes, float MinTimeToMusic)
{
	ASSERT(MinTimeToNotes >= 0);
	ASSERT(MinTimeToMusic >= 0);

	/* XXX: We want the first beat *in use*, so we don't delay needlessly. */
	const float fFirstBeat = GAMESTATE->m_pCurSong->m_fFirstBeat;
	const float fFirstSecond = GAMESTATE->m_pCurSong->GetElapsedTimeFromBeat( fFirstBeat );
	float fStartSecond = fFirstSecond - MinTimeToNotes;

	fStartSecond = min(fStartSecond, -MinTimeToMusic);
	
	m_soundMusic.SetPositionSeconds( fStartSecond );
	m_soundMusic.SetPlaybackRate( GAMESTATE->m_SongOptions.m_fMusicRate );

	/* Keep the music playing after it's finished; we'll stop it. */
	m_soundMusic.SetStopMode(RageSound::M_CONTINUE);
	m_soundMusic.StartPlaying();

	/* Return the amount of time until the first beat. */
	return fFirstSecond - fStartSecond;
}

bool ScreenGameplay::OneIsHot() const
{
	for( int p=0; p<NUM_PLAYERS; p++ )
		if( GAMESTATE->IsPlayerEnabled(PlayerNumber(p)) )
			if( (m_pLifeMeter[p] && m_pLifeMeter[p]->IsHot()) || 
				(m_pCombinedLifeMeter && m_pCombinedLifeMeter->IsHot((PlayerNumber)p)) )
				return true;
	return false;
}

bool ScreenGameplay::AllAreInDanger() const
{
	for( int p=0; p<NUM_PLAYERS; p++ )
		if( GAMESTATE->IsPlayerEnabled(PlayerNumber(p)) )
			if( (m_pLifeMeter[p] && !m_pLifeMeter[p]->IsInDanger()) || 
				(m_pCombinedLifeMeter && !m_pCombinedLifeMeter->IsInDanger((PlayerNumber)p)) )
				return false;
	return true;
}

bool ScreenGameplay::AllAreFailing() const
{
	for( int p=0; p<NUM_PLAYERS; p++ )
		if( GAMESTATE->IsPlayerEnabled(PlayerNumber(p)) )
			if( (m_pLifeMeter[p] && !m_pLifeMeter[p]->IsFailing()) || 
				(m_pCombinedLifeMeter && !m_pCombinedLifeMeter->IsFailing((PlayerNumber)p)) )
				return false;
	
	return true;
}

bool ScreenGameplay::AllFailedEarlier() const
{
	for( int p=0; p<NUM_PLAYERS; p++ )
		if( GAMESTATE->IsPlayerEnabled(p) && !GAMESTATE->m_CurStageStats.bFailedEarlier[p] )
			return false;
	return true;
}

// play assist ticks
bool ScreenGameplay::IsTimeToPlayTicks() const
{
	// Sound cards have a latency between when a sample is Play()ed and when the sound
	// will start coming out the speaker.  Compensate for this by boosting
	// fPositionSeconds ahead
	float fPositionSeconds = GAMESTATE->m_fMusicSeconds;
	fPositionSeconds += (SOUND->GetPlayLatency()+(float)G_TICK_EARLY_SECONDS) * m_soundMusic.GetPlaybackRate();
	float fSongBeat = GAMESTATE->m_pCurSong->GetBeatFromElapsedTime( fPositionSeconds );

	int iRowNow = BeatToNoteRowNotRounded( fSongBeat );
	iRowNow = max( 0, iRowNow );
	static int iRowLastCrossed = 0;

	bool bAnyoneHasANote = false;	// set this to true if any player has a note at one of the indicies we crossed

	for( int r=iRowLastCrossed+1; r<=iRowNow; r++ )  // for each index we crossed since the last update
	{
		for( int p=0; p<NUM_PLAYERS; p++ )
		{
			if( !GAMESTATE->IsPlayerEnabled( (PlayerNumber)p ) )
				continue;		// skip

			bAnyoneHasANote |= m_Player[p].IsThereATapOrHoldHeadAtRow( r );
			break;	// this will only play the tick for the first player that is joined
		}
	}

	iRowLastCrossed = iRowNow;

	return bAnyoneHasANote;
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

	if( m_bFirstUpdate )
	{
		SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("gameplay intro") );	// crowd cheer

		//
		// Get the transitions rolling
		//
		if( m_bDemonstration )
		{
			StartPlayingSong( 0, 0 );	// *kick* (no transitions)
		}
		else
		{
			float fMinTimeToMusic = m_In.GetLengthSeconds();	// start of m_Ready
			float fMinTimeToNotes = fMinTimeToMusic + m_Ready.GetLengthSeconds() + m_Go.GetLengthSeconds()+2;	// end of Go

			/*
			 * Tell the music to start, but don't actually make any noise for
			 * at least 2.5 (or 1.5) seconds.  (This is so we scroll on screen smoothly.)
			 *
			 * This is only a minimum: the music might be started later, to meet
			 * the minimum-time-to-notes value.  If you're writing song data,
			 * and you want to make sure we get ideal timing here, make sure there's
			 * a bit of space at the beginning of the music with no steps. 
			 */

			/*float delay =*/ StartPlayingSong( fMinTimeToNotes, fMinTimeToMusic );

			m_In.StartTransitioning( SM_PlayReady );
		}
	}



	/* Very important:  Update GAMESTATE's song beat information
	 * -before- calling update on all the classes that depend on it.
	 * If you don't do this first, the classes are all acting on old 
	 * information and will lag.  -Chris */

	/* If ScreenJukebox is changing screens, it'll stop m_soundMusic to tell
	 * us not to update the time here.  (In that case, we've already created
	 * a new ScreenJukebox and reset music statistics, and if we do this then
	 * we'll un-reset them.) */
	if(m_soundMusic.IsPlaying())
		GAMESTATE->UpdateSongPosition(m_soundMusic.GetPositionSeconds());

	if( m_bZeroDeltaOnNextUpdate )
	{
		Screen::Update( 0 );
		m_bZeroDeltaOnNextUpdate = false;
	}
	else
		Screen::Update( fDeltaTime );

	if( GAMESTATE->m_pCurSong == NULL )
		return;

	if( GAMESTATE->m_MasterPlayerNumber != PLAYER_INVALID )
		m_MaxCombo.SetText( ssprintf("%d", GAMESTATE->m_CurStageStats.iMaxCombo[GAMESTATE->m_MasterPlayerNumber]) ); /* MAKE THIS WORK FOR BOTH PLAYERS! */
	
	//LOG->Trace( "m_fOffsetInBeats = %f, m_fBeatsPerSecond = %f, m_Music.GetPositionSeconds = %f", m_fOffsetInBeats, m_fBeatsPerSecond, m_Music.GetPositionSeconds() );

	int pn;
	m_BeginnerHelper.Update(fDeltaTime);
	
	switch( m_DancingState )
	{
	case STATE_DANCING:
		//
		// Update living players' alive time
		//
		for( pn=0; pn<NUM_PLAYERS; pn++ )
			if( GAMESTATE->IsPlayerEnabled(pn) && !GAMESTATE->m_CurStageStats.bFailed[pn])
				GAMESTATE->m_CurStageStats.fAliveSeconds [pn] += fDeltaTime * GAMESTATE->m_SongOptions.m_fMusicRate;

		//
		// Check for end of song
		//
		float fSecondsToStop = GAMESTATE->m_pCurSong->GetElapsedTimeFromBeat( GAMESTATE->m_pCurSong->m_fLastBeat ) + 1;
		if( GAMESTATE->m_fMusicSeconds > fSecondsToStop  &&  !m_NextSongOut.IsTransitioning() )
			this->PostScreenMessage( SM_NotesEnded, 0 );

		//
		// Handle the background danger graphic.  Never show it in FAIL_OFF.  Don't
		// show it if everyone is already failing: it's already too late and it's
		// annoying for it to show for the entire duration of a song.
		//
		if( GAMESTATE->m_SongOptions.m_FailType != SongOptions::FAIL_OFF )
		{
			if( AllAreInDanger() && !AllAreFailing() )
				m_Background.TurnDangerOn();
			else
				m_Background.TurnDangerOff();
		}

		//
		// check for fail
		//
		UpdateCheckFail();
	
		//
		// update 2d dancing characters
		//
		for( int p=0; p<NUM_PLAYERS; p++ )
		{
			if( !GAMESTATE->IsPlayerEnabled(p) )
				continue;
			if(m_Background.GetDancingCharacters() != NULL)
			{
				if(m_Player[p].GetDancingCharacterState() != AS2D_IGNORE) // grab the state of play from player and update the character
					m_Background.GetDancingCharacters()->Change2DAnimState(p,m_Player[p].GetDancingCharacterState());
				m_Player[p].SetCharacterState(AS2D_IGNORE); // set to ignore as we've already grabbed the latest change
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

				// HACK:  Shut the announcer up
				m_announcerReady.UnloadAll();
				m_announcerHereWeGo.UnloadAll();
				m_announcerDanger.UnloadAll();
				m_announcerGood.UnloadAll();
				m_announcerHot.UnloadAll();
				m_announcerOni.UnloadAll();
				m_announcer100Combo.UnloadAll();
				m_announcer200Combo.UnloadAll();
				m_announcer300Combo.UnloadAll();
				m_announcer400Combo.UnloadAll();
				m_announcer500Combo.UnloadAll();
				m_announcer600Combo.UnloadAll();
				m_announcer700Combo.UnloadAll();
				m_announcer800Combo.UnloadAll();
				m_announcer900Combo.UnloadAll();
				m_announcer1000Combo.UnloadAll();
				m_announcerComboStopped.UnloadAll();
				m_announcerBattleTrickLevel1.UnloadAll();
				m_announcerBattleTrickLevel2.UnloadAll();
				m_announcerBattleTrickLevel3.UnloadAll();
				m_announcerBattleDamageLevel1.UnloadAll();
				m_announcerBattleDamageLevel2.UnloadAll();
				m_announcerBattleDamageLevel3.UnloadAll();

				m_announcerBattleDie.PlayRandom();

				GAMESTATE->RemoveAllActiveAttacks();

				for( int p=0; p<NUM_PLAYERS; p++ )
				{
					if( GAMESTATE->IsCpuPlayer(p) )
					{
						m_soundOniDie.PlayRandom();
						ShowOniGameOver((PlayerNumber)p);
						m_Player[p].Init();		// remove all notes and scoring
						m_Player[p].FadeToFail();	// tell the NoteField to fade to white
					}
				}
			}
		}

		//
		// Check to see if it's time to play a ScreenGameplay comment
		//
		m_fTimeLeftBeforeDancingComment -= fDeltaTime;
		if( m_fTimeLeftBeforeDancingComment <= 0 )
		{
			switch( GAMESTATE->m_PlayMode )
			{
			case PLAY_MODE_ARCADE:
			case PLAY_MODE_BATTLE:
			case PLAY_MODE_RAVE:
				if( OneIsHot() )			m_announcerHot.PlayRandom();
				else if( AllAreInDanger() )	m_announcerDanger.PlayRandom();
				else						m_announcerGood.PlayRandom();
				if( m_pCombinedLifeMeter )
					m_pCombinedLifeMeter->OnTaunt();
				break;
			case PLAY_MODE_NONSTOP:
			case PLAY_MODE_ONI:
			case PLAY_MODE_ENDLESS:
				m_announcerOni.PlayRandom();
				break;
			default:
				ASSERT(0);
			}
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;	// reset for the next comment
		}
	}

	if( !m_GiveUpTimer.IsZero() && m_GiveUpTimer.Ago() > 4.0f )
	{
		m_GiveUpTimer.SetZero();
		/* Unless we're in FailOff, giving up means failing the song. */
		switch( GAMESTATE->m_SongOptions.m_FailType )
		{
		case SongOptions::FAIL_ARCADE:
		case SongOptions::FAIL_END_OF_SONG:
			for ( pn=0; pn<NUM_PLAYERS; pn++ )
				GAMESTATE->m_CurStageStats.bFailed[pn] = true;	// fail
		}

		this->PostScreenMessage( SM_NotesEnded, 0 );
	}

	bool bPlayTicks = IsTimeToPlayTicks();
	if( bPlayTicks )
	{
		if( GAMESTATE->m_SongOptions.m_bAssistTick )
			m_soundAssistTick.Play();
	}

	static float s_fSecsLeftOnUpperLights = 0;
	if( bPlayTicks )
	{
		float fSecsPerBeat = 1.f/GAMESTATE->m_fCurBPS;
		float fSecsToLight = fSecsPerBeat*.2f;
		s_fSecsLeftOnUpperLights = fSecsToLight;
	}
	else
	{
		s_fSecsLeftOnUpperLights -= fDeltaTime;
		if( s_fSecsLeftOnUpperLights < 0 )
			s_fSecsLeftOnUpperLights = 0;
	}
	if( s_fSecsLeftOnUpperLights>0 )
		LIGHTSMAN->SetAllUpperLights( true );
	else
		LIGHTSMAN->SetAllUpperLights( false );
}

/* Set m_CurStageStats.bFailed for failed players.  In, FAIL_ARCADE, send
 * SM_BeginFailed if all players failed, and kill dead Oni players. */
void ScreenGameplay::UpdateCheckFail()
{
	if( GAMESTATE->m_SongOptions.m_FailType == SongOptions::FAIL_OFF )
		return;

	// check for individual fail
	for ( int pn=0; pn<NUM_PLAYERS; pn++ )
	{
		if( !GAMESTATE->IsPlayerEnabled(pn) )
			continue;
		if( (m_pLifeMeter[pn] && !m_pLifeMeter[pn]->IsFailing()) || 
			(m_pCombinedLifeMeter && !m_pCombinedLifeMeter->IsFailing((PlayerNumber)pn)) )
			continue; /* isn't failing */
		if( GAMESTATE->m_CurStageStats.bFailed[pn] )
			continue; /* failed and is already dead */
		
		/* If recovery is enabled, only set fail if both are failing.
		 * There's no way to recover mid-song in battery mode. */
		if( GAMESTATE->m_SongOptions.m_LifeType != SongOptions::LIFE_BATTERY &&
			PREFSMAN->m_bTwoPlayerRecovery && !AllAreFailing() )
			continue;

		LOG->Trace("Player %d failed", (int)pn);
		GAMESTATE->m_CurStageStats.bFailed[pn] = true;	// fail

		if( GAMESTATE->m_SongOptions.m_LifeType == SongOptions::LIFE_BATTERY &&
			GAMESTATE->m_SongOptions.m_FailType == SongOptions::FAIL_ARCADE )
		{
			if( !AllFailedEarlier() )	// if not the last one to fail
			{
				// kill them!
				m_soundOniDie.PlayRandom();
				ShowOniGameOver((PlayerNumber)pn);
				m_Player[pn].Init();		// remove all notes and scoring
				m_Player[pn].FadeToFail();	// tell the NoteField to fade to white
			}
		}
	}

	/* If FAIL_ARCADE and everyone is failing, start SM_BeginFailed. */
	if( GAMESTATE->m_SongOptions.m_FailType == SongOptions::FAIL_ARCADE &&
		AllAreFailing() )
		SCREENMAN->PostMessageToTopScreen( SM_BeginFailed, 0 );
}

void ScreenGameplay::AbortGiveUp()
{
	if( m_GiveUpTimer.IsZero() )
		return;

	m_textDebug.StopTweening();
	m_textDebug.SetText("Don't give up!");
	m_textDebug.BeginTweening( 1/2.f );
	m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
	m_GiveUpTimer.SetZero();
}

void ScreenGameplay::DrawPrimitives()
{
	m_BeginnerHelper.DrawPrimitives();
	Screen::DrawPrimitives();
}


void ScreenGameplay::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	//LOG->Trace( "ScreenGameplay::Input()" );

	if( MenuI.IsValid()  &&  
		m_DancingState != STATE_OUTRO  &&
		!m_Back.IsTransitioning() )
	{
		/* Allow bailing out by holding the START button of all active players.  This
		 * gives a way to "give up" when a back button isn't available.  Doing this is
		 * treated as failing the song, unlike BACK, since it's always available.
		 *
		 * However, if this is also a style button, don't do this. (pump center = start) */
		if( MenuI.button == MENU_BUTTON_START && !StyleI.IsValid() )
		{
			/* No PREFSMAN->m_bDelayedEscape; always delayed. */
			if( type==IET_RELEASE )
				AbortGiveUp();
			else if( type==IET_FIRST_PRESS && m_GiveUpTimer.IsZero() )
			{
				m_textDebug.SetText( "Continue holding START to give up" );
				m_textDebug.StopTweening();
				m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
				m_textDebug.BeginTweening( 1/8.f );
				m_textDebug.SetDiffuse( RageColor(1,1,1,1) );
				m_GiveUpTimer.Touch(); /* start the timer */
			}

			return;
		}

		if( MenuI.button == MENU_BUTTON_BACK && PREFSMAN->m_bDelayedEscape && type==IET_FIRST_PRESS)
		{
			m_textDebug.SetText( "Continue holding BACK to quit" );
			m_textDebug.StopTweening();
			m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
			m_textDebug.BeginTweening( 1/8.f );
			m_textDebug.SetDiffuse( RageColor(1,1,1,1) );
			return;
		}
		
		if( MenuI.button == MENU_BUTTON_BACK && PREFSMAN->m_bDelayedEscape && type==IET_RELEASE )
		{
			m_textDebug.StopTweening();
			m_textDebug.BeginTweening( 1/8.f );
			m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
			return;
		}
		
		if( MenuI.button == MENU_BUTTON_BACK && 
			((!PREFSMAN->m_bDelayedEscape && type==IET_FIRST_PRESS) ||
			(DeviceI.device==DEVICE_KEYBOARD && (type==IET_SLOW_REPEAT||type==IET_FAST_REPEAT))  ||
			(DeviceI.device!=DEVICE_KEYBOARD && type==IET_FAST_REPEAT)) )
		{
			m_DancingState = STATE_OUTRO;
			SOUND->PlayOnce( THEME->GetPathToS("Common back") );
			/* Hmm.  There are a bunch of subtly different ways we can
			 * tween out: 
			 *   1. Keep rendering the song, and keep it moving.  This might
			 *      cause problems if the cancel and the end of the song overlap.
			 *   2. Stop the song completely, so all song motion under the tween
			 *      ceases.
			 *   3. Stop the song, but keep effects (eg. Drunk) running.
			 *   4. Don't display the song at all.
			 *
			 * We're doing #3.  I'm not sure which is best.
			 */
			m_soundMusic.StopPlaying();

			this->ClearMessageQueue();
			m_Back.StartTransitioning( SM_SaveChangedBeforeGoingBack );
			return;
		}
	}

	/* Nothing else cares about releases. */
	if(type == IET_RELEASE) return;

	// Handle special keys to adjust the offset
	if( DeviceI.device == DEVICE_KEYBOARD )
	{
		switch( DeviceI.button )
		{
		case SDLK_F6:
			m_bChangedOffsetOrBPM = true;
			GAMESTATE->m_SongOptions.m_bAutoSync = !GAMESTATE->m_SongOptions.m_bAutoSync;	// toggle
			UpdateAutoPlayText();
			break;
		case SDLK_F7:
			GAMESTATE->m_SongOptions.m_bAssistTick ^= 1;

			/* Store this change, so it sticks if we change songs: */
			GAMESTATE->m_StoredSongOptions.m_bAssistTick = GAMESTATE->m_SongOptions.m_bAssistTick;
			
			m_textDebug.SetText( ssprintf("Assist Tick is %s", GAMESTATE->m_SongOptions.m_bAssistTick?"ON":"OFF") );
			m_textDebug.StopTweening();
			m_textDebug.SetDiffuse( RageColor(1,1,1,1) );
			m_textDebug.BeginTweening( 3 );		// sleep
			m_textDebug.BeginTweening( 0.5f );	// fade out
			m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
			break;
		case SDLK_F8:
			{
				PREFSMAN->m_bAutoPlay = !PREFSMAN->m_bAutoPlay;
				UpdateAutoPlayText();
				for( int p=0; p<NUM_PLAYERS; p++ )
					if( GAMESTATE->IsHumanPlayer(p) )
						GAMESTATE->m_PlayerController[p] = PREFSMAN->m_bAutoPlay?PC_AUTOPLAY:PC_HUMAN;
			}
			break;
		case SDLK_F9:
		case SDLK_F10:
			{
				m_bChangedOffsetOrBPM = true;

				float fOffsetDelta;
				switch( DeviceI.button )
				{
				case SDLK_F9:	fOffsetDelta = -0.020f;		break;
				case SDLK_F10:	fOffsetDelta = +0.020f;		break;
				default:	ASSERT(0);						return;
				}
				if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, SDLK_RALT)) ||
					INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, SDLK_LALT)) )
					fOffsetDelta /= 2; /* .010 */
				else if( type == IET_FAST_REPEAT )
					fOffsetDelta *= 10;
				BPMSegment& seg = GAMESTATE->m_pCurSong->GetBPMSegmentAtBeat( GAMESTATE->m_fSongBeat );

				seg.m_fBPM += fOffsetDelta;

				m_textDebug.SetText( ssprintf("Cur BPM = %.2f", seg.m_fBPM) );
				m_textDebug.StopTweening();
				m_textDebug.SetDiffuse( RageColor(1,1,1,1) );
				m_textDebug.BeginTweening( 3 );		// sleep
				m_textDebug.BeginTweening( 0.5f );	// fade out
				m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
			}
			break;
		case SDLK_F11:
		case SDLK_F12:
			{
				m_bChangedOffsetOrBPM = true;

				float fOffsetDelta;
				switch( DeviceI.button )
				{
				case SDLK_F11:	fOffsetDelta = -0.02f;		break;
				case SDLK_F12:	fOffsetDelta = +0.02f;		break;
				default:	ASSERT(0);						return;
				}
				if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, SDLK_RALT)) ||
					INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, SDLK_LALT)) )
					fOffsetDelta /= 20; /* 1ms */
				else switch( type )
				{
				case IET_SLOW_REPEAT:	fOffsetDelta *= 10;	break;
				case IET_FAST_REPEAT:	fOffsetDelta *= 40;	break;
				}

				GAMESTATE->m_pCurSong->m_fBeat0OffsetInSeconds += fOffsetDelta;

				m_textDebug.SetText( ssprintf("Offset = %.3f", GAMESTATE->m_pCurSong->m_fBeat0OffsetInSeconds) );
				m_textDebug.StopTweening();
				m_textDebug.SetDiffuse( RageColor(1,1,1,1) );
				m_textDebug.BeginTweening( 3 );		// sleep
				m_textDebug.BeginTweening( 0.5f );	// fade out
				m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
			}
			break;
		// testing:
		case SDLK_PAUSE:
			{
				if( GAMESTATE->m_PlayerOptions[PLAYER_1].m_fPerspectiveTilt == -1 )	// incoming
				{
					for( int p=0; p<NUM_PLAYERS; p++ )
						GAMESTATE->m_PlayerOptions[PLAYER_1].m_fPerspectiveTilt = 0;
				}
				else
				{
					for( int p=0; p<NUM_PLAYERS; p++ )
						GAMESTATE->m_PlayerOptions[PLAYER_1].m_fPerspectiveTilt = -1;
				}
			}
			break;
		}
	}

	//
	// handle a step or battle item activate
	//
	if( type==IET_FIRST_PRESS && 
		StyleI.IsValid() &&
		GAMESTATE->IsPlayerEnabled( StyleI.player ) )
	{
		AbortGiveUp();
		
		if( !PREFSMAN->m_bAutoPlay )
			m_Player[StyleI.player].Step( StyleI.col, DeviceI.ts ); 
	}
//	else if( type==IET_FIRST_PRESS && 
//		!PREFSMAN->m_bAutoPlay && 
//		MenuI.IsValifd() &&
//		GAMESTATE->IsPlayerEnabled( MenuI.player ) &&
//		GAMESTATE->IsBattleMode() )
//	{
//		int iItemSlot;
//		switch( MenuI.button )
//		{
//		case MENU_BUTTON_LEFT:	iItemSlot = 0;	break;
//		case MENU_BUTTON_START:	iItemSlot = 1;	break;
//		case MENU_BUTTON_RIGHT:	iItemSlot = 2;	break;
//		default:				iItemSlot = -1;	break;
//		}
//		
//		if( iItemSlot != -1 )
//			m_pInventory[MenuI.player]->UseItem( iItemSlot );
//	}
}

void ScreenGameplay::UpdateAutoPlayText()
{
	CString sText;

	if( PREFSMAN->m_bAutoPlay )
		sText += "AutoPlay     ";
	if( GAMESTATE->m_SongOptions.m_bAutoSync )
		sText += "AutoSync     ";

	if( sText.length() > 0 )
		sText.resize( sText.length()-5 );

	m_textAutoPlay.SetText( sText );
}

void SaveChanges( void* papSongsQueue )
{
	vector<Song*>& apSongsQueue = *(vector<Song*>*)papSongsQueue;
	for( unsigned i=0; i<apSongsQueue.size(); i++ )
		apSongsQueue[i]->Save();
}

void RevertChanges( void* papSongsQueue )
{
	vector<Song*>& apSongsQueue = *(vector<Song*>*)papSongsQueue;
	for( unsigned i=0; i<apSongsQueue.size(); i++ )
		apSongsQueue[i]->RevertFromDisk();
}

void ScreenGameplay::ShowSavePrompt( ScreenMessage SM_SendWhenDone )
{
	CString sMessage;
	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_ARCADE:
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		sMessage = ssprintf( 
			"You have changed the offset or BPM of\n"
			"%s\n", 
			GAMESTATE->m_pCurSong->GetFullDisplayTitle().c_str() );

		if( fabs(GAMESTATE->m_pCurSong->m_fBeat0OffsetInSeconds - g_fOldOffset) > 0.001 )
		{
			sMessage += ssprintf( 
				"\n"
				"Offset was changed from %.3f to %.3f (%.3f).\n",
				g_fOldOffset, 
				GAMESTATE->m_pCurSong->m_fBeat0OffsetInSeconds,
				GAMESTATE->m_pCurSong->m_fBeat0OffsetInSeconds - g_fOldOffset );
		}

		sMessage +=
			"\n"
			"Would you like to save these changes back\n"
			"to the song file?\n"
			"Choosing NO will discard your changes.";
			
		break;
	case PLAY_MODE_NONSTOP:
	case PLAY_MODE_ONI:
	case PLAY_MODE_ENDLESS:
		sMessage = ssprintf( 
			"You have changed the offset or BPM of\n"
			"one or more songs in this course.\n"
			"Would you like to save these changes back\n"
			"to the song file(s)?\n"
			"Choosing NO will discard your changes." );
		break;
	default:
		ASSERT(0);
	}

	SCREENMAN->Prompt( SM_SendWhenDone, sMessage, true, false, SaveChanges, RevertChanges, &m_apSongsQueue );
}

void ScreenGameplay::HandleScreenMessage( const ScreenMessage SM )
{
	CHECKPOINT_M( ssprintf("HandleScreenMessage(%i)", SM) );
	switch( SM )
	{
	case SM_PlayReady:
		m_Background.FadeIn();
		m_announcerReady.PlayRandom();
		m_Ready.StartTransitioning( SM_PlayGo );
		break;

	case SM_PlayGo:
		m_announcerHereWeGo.PlayRandom();
		m_Go.StartTransitioning( SM_None );
		GAMESTATE->m_bPastHereWeGo = true;
		m_DancingState = STATE_DANCING;		// STATE CHANGE!  Now the user is allowed to press Back
		break;

	// received while STATE_DANCING
	case SM_NotesEnded:
		{
			// save any statistics
			int p;
			for( p=0; p<NUM_PLAYERS; p++ )
			{
				if( GAMESTATE->IsPlayerEnabled(p) )
				{
					for( int r=0; r<NUM_RADAR_CATEGORIES; r++ )
					{
						RadarCategory rc = (RadarCategory)r;
						GAMESTATE->m_CurStageStats.fRadarPossible[p][r] = NoteDataUtil::GetRadarValue( m_Player[p], rc, GAMESTATE->m_pCurSong->m_fMusicLengthSeconds );
						GAMESTATE->m_CurStageStats.fRadarActual[p][r] = m_Player[p].GetActualRadarValue( rc, (PlayerNumber)p, GAMESTATE->m_pCurSong->m_fMusicLengthSeconds );
					}
				}
			}

			/* Do this in LoadNextSong, so we don't tween off old attacks until
			 * m_NextSongOut finishes. */
			// GAMESTATE->RemoveAllActiveAttacks();

			for( p=0; p<NUM_PLAYERS; p++ )
			{
				if( !GAMESTATE->IsPlayerEnabled(p) )
					continue;

				/* If either player's passmark is enabled, check it. */
				if( GAMESTATE->m_PlayerOptions[p].m_fPassmark > 0 &&
					m_pLifeMeter[p] &&
					m_pLifeMeter[p]->GetLife() < GAMESTATE->m_PlayerOptions[p].m_fPassmark )
				{
					LOG->Trace("Player %i failed: life %f is under %f",
						p+1, m_pLifeMeter[p]->GetLife(), GAMESTATE->m_PlayerOptions[p].m_fPassmark );
					GAMESTATE->m_CurStageStats.bFailed[p] = true;
				}

				if( !GAMESTATE->m_CurStageStats.bFailed[p] )
					GAMESTATE->m_CurStageStats.iSongsPassed[p]++;
			}

			/* Mark failure.  This was possibly already done by UpdateCheckFail, but
			 * not always, if m_bTwoPlayerRecovery is set. */
			if( GAMESTATE->m_SongOptions.m_FailType != SongOptions::FAIL_OFF )
			{
				if( (m_pLifeMeter[p] && !m_pLifeMeter[p]->IsFailing()) || 
					(m_pCombinedLifeMeter && !m_pCombinedLifeMeter->IsFailing((PlayerNumber)p)) )
					GAMESTATE->m_CurStageStats.bFailed[p] = true;
			}

			/* If all players have *really* failed (bFailed, not the life meter or
			 * bFailedEarlier): */
			const bool bAllReallyFailed = GAMESTATE->m_CurStageStats.AllFailed();

			if( !bAllReallyFailed && !IsLastSong() )
			{
				/* Next song. */
				for( p=0; p<NUM_PLAYERS; p++ )
				if( GAMESTATE->IsPlayerEnabled(p) && !GAMESTATE->m_CurStageStats.bFailed[p] )
				{
					// give a little life back between stages
					if( m_pLifeMeter[p] )
						m_pLifeMeter[p]->OnSongEnded();	
					if( m_pCombinedLifeMeter )
						m_pCombinedLifeMeter->OnSongEnded();	
				}

				// HACK:  Temporarily set the song pointer to the next song so that 
				// this m_NextSongOut will show the next song banner
				Song* pCurSong = GAMESTATE->m_pCurSong;

				int iPlaySongIndex = GAMESTATE->GetCourseSongIndex()+1;
				iPlaySongIndex %= m_apSongsQueue.size();
				GAMESTATE->m_pCurSong = m_apSongsQueue[iPlaySongIndex];

				m_NextSongOut.Load( THEME->GetPathToB("ScreenGameplay next song out") );
				GAMESTATE->m_pCurSong = pCurSong;

				m_NextSongOut.StartTransitioning( SM_LoadNextSong );
				return;
			}
			
			// update dancing characters for win / lose
			DancingCharacters *Dancers = m_Background.GetDancingCharacters();
			if( Dancers )
				for( p=0; p<NUM_PLAYERS; p++ )
				{
					if( !GAMESTATE->IsPlayerEnabled(p) )
						continue;

					/* XXX: In battle modes, switch( GAMESTATE->GetStageResult(p) ). */
					if( GAMESTATE->m_CurStageStats.bFailed[p] )
						Dancers->Change2DAnimState( p, AS2D_FAIL ); // fail anim
					else if( m_pLifeMeter[p] && m_pLifeMeter[p]->GetLife() == 1.0f ) // full life
						Dancers->Change2DAnimState( p, AS2D_WINFEVER ); // full life pass anim
					else
						Dancers->Change2DAnimState( p, AS2D_WIN ); // pass anim
				}

			/* End round. */
			if( m_DancingState == STATE_OUTRO )	// ScreenGameplay already ended
				return;		// ignore
			m_DancingState = STATE_OUTRO;

			GAMESTATE->RemoveAllActiveAttacks();

			LIGHTSMAN->SetLightMode( LIGHTMODE_ALL_CLEARED );


			if( bAllReallyFailed )
			{
				this->PostScreenMessage( SM_BeginFailed, 0 );
				return;
			}

			// do they deserve an extra stage?
			if( GAMESTATE->HasEarnedExtraStage() )
			{
				TweenOffScreen();
				m_Extra.StartTransitioning( SM_GoToStateAfterCleared );
				SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("gameplay extra") );
			}
			else
			{
				TweenOffScreen();
				
				switch( GAMESTATE->m_PlayMode )
				{
				case PLAY_MODE_BATTLE:
				case PLAY_MODE_RAVE:
					{
						PlayerNumber winner = GAMESTATE->GetBestPlayer();
						switch( winner )
						{
						case PLAYER_INVALID:
							m_Draw.StartTransitioning( SM_GoToStateAfterCleared );
							break;
						default:
							m_Win[winner].StartTransitioning( SM_GoToStateAfterCleared );
							break;
						}
					}
					break;
				default:
					m_Cleared.StartTransitioning( SM_GoToStateAfterCleared );
					break;
				}
				
				SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("gameplay cleared") );
			}
		}

		break;

	case SM_LoadNextSong:
		LoadNextSong();
		GAMESTATE->m_bPastHereWeGo = true;
		/* We're fading in, so don't hit any notes for a few seconds; they'll be
		 * obscured by the fade. */
		StartPlayingSong( m_NextSongIn.GetLengthSeconds()+2, 0 );
		m_NextSongIn.StartTransitioning( SM_None );
		break;

	case SM_PlayToasty:
		if( PREFSMAN->m_bEasterEggs )
			if( !m_Toasty.IsTransitioning()  &&  !m_Toasty.IsFinished() )	// don't play if we've already played it once
				m_Toasty.StartTransitioning();
		break;

#define SECS_SINCE_LAST_COMMENT (SECONDS_BETWEEN_COMMENTS-m_fTimeLeftBeforeDancingComment)
	case SM_100Combo:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcer100Combo.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_200Combo:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcer200Combo.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_300Combo:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcer300Combo.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_400Combo:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcer400Combo.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_500Combo:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcer500Combo.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_600Combo:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcer600Combo.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_700Combo:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcer700Combo.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_800Combo:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcer800Combo.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_900Combo:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcer900Combo.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_1000Combo:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcer1000Combo.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_ComboStopped:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcerComboStopped.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_ComboContinuing:
		if( SECS_SINCE_LAST_COMMENT > 2 )
		{
			m_announcerComboContinuing.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;	
	case SM_BattleTrickLevel1:
		if( SECS_SINCE_LAST_COMMENT > 3 )
		{
			m_announcerBattleTrickLevel1.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		m_soundBattleTrickLevel1.Play();
		break;
	case SM_BattleTrickLevel2:
		if( SECS_SINCE_LAST_COMMENT > 3 )
		{
			m_announcerBattleTrickLevel2.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		m_soundBattleTrickLevel2.Play();
		break;
	case SM_BattleTrickLevel3:
		if( SECS_SINCE_LAST_COMMENT > 3 )
		{
			m_announcerBattleTrickLevel3.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		m_soundBattleTrickLevel3.Play();
		break;
	
	case SM_BattleDamageLevel1:
		if( SECS_SINCE_LAST_COMMENT > 3 )
		{
			m_announcerBattleDamageLevel1.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_BattleDamageLevel2:
		if( SECS_SINCE_LAST_COMMENT > 3 )
		{
			m_announcerBattleDamageLevel2.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	case SM_BattleDamageLevel3:
		if( SECS_SINCE_LAST_COMMENT > 3 )
		{
			m_announcerBattleDamageLevel3.PlayRandom();
			m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		}
		break;
	
	case SM_SaveChangedBeforeGoingBack:
		if( m_bChangedOffsetOrBPM )
		{
			m_bChangedOffsetOrBPM = false;
			ShowSavePrompt( SM_GoToScreenAfterBack );
		}
		else
			HandleScreenMessage( SM_GoToScreenAfterBack );
		break;

	case SM_GoToScreenAfterBack:
		/* Reset options.  (Should this be done in ScreenSelect*?) */
		GAMESTATE->RestoreSelectedOptions();
		SCREENMAN->SetNewScreen( PREV_SCREEN(GAMESTATE->m_PlayMode) );
		break;

	case SM_GoToStateAfterCleared:
		/* Reset options.  (Should this be done in ScreenSelect*?) */
		GAMESTATE->RestoreSelectedOptions();

		if( m_bChangedOffsetOrBPM )
		{
			m_bChangedOffsetOrBPM = false;
			ShowSavePrompt( SM_GoToStateAfterCleared );
			break;
		}
		
		SCREENMAN->SetNewScreen( NEXT_SCREEN(GAMESTATE->m_PlayMode) );
		break;

	case SM_BeginFailed:
		m_DancingState = STATE_OUTRO;
		m_soundMusic.StopPlaying();
		TweenOffScreen();
		m_Failed.StartTransitioning( SM_GoToScreenAfterFail );

		// make the background invisible so we don't waste power drawing it
		m_Background.BeginTweening( 1 );
		m_Background.SetDiffuse( RageColor(1,1,1,0) );

		// show the survive time if extra stage
		if( GAMESTATE->IsExtraStage()  ||  GAMESTATE->IsExtraStage2() )
		{
			float fMaxSurviveSeconds = 0;
			for( int p=0; p<NUM_PLAYERS; p++ )
				if( GAMESTATE->IsPlayerEnabled(p) )
					fMaxSurviveSeconds = max( fMaxSurviveSeconds, GAMESTATE->m_CurStageStats.fAliveSeconds[p] );
			ASSERT( fMaxSurviveSeconds > 0 );
			m_textSurviveTime.SetText( "TIME: " + SecondsToTime(fMaxSurviveSeconds) );
			SET_XY_AND_ON_COMMAND( m_textSurviveTime );
			
			// if unlocks are on, update fail extra stage count
			if (PREFSMAN->m_bUseUnlockSystem)
				UNLOCKSYS->UnlockFailExtraStage();
		}
		
		// Feels hackish. Feel free to make cleaner.
                // MD 10/27/03 - I felt free. :-)  And now it's arcade-accurate as well.
		if(GAMESTATE->IsCourseMode())
                        if(GAMESTATE->GetCourseSongIndex() > (int(m_apSongsQueue.size() / 2) - 1 ))
				SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay oni failed halfway" ) );
			else
				SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay oni failed" ) );
		else
			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("gameplay failed") );
		break;

	case SM_GoToScreenAfterFail:
		/* Reset options.  (Should this be done in ScreenSelect*?) */
		GAMESTATE->RestoreSelectedOptions();

		if( m_bChangedOffsetOrBPM )
		{
			m_bChangedOffsetOrBPM = false;
			ShowSavePrompt( SM_GoToScreenAfterFail );
			break;
		}

		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_ARCADE:
		case PLAY_MODE_BATTLE:
		case PLAY_MODE_RAVE:
			if( PREFSMAN->m_bEventMode )
			{
				if(EVAL_ON_FAIL) // go to the eval screen if we fail
				{
					SCREENMAN->SetNewScreen( "ScreenEvaluationStage" );
				}
				else // the theme says just fail and go back to the song select for event mode
				{
					HandleScreenMessage( SM_GoToScreenAfterBack );
				}
			}
			else if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
				SCREENMAN->SetNewScreen( "ScreenEvaluationStage" );
			else
			{
				if(EVAL_ON_FAIL) // go to the eval screen if we fail
				{
					SCREENMAN->SetNewScreen( "ScreenEvaluationStage" );
				}
				else // if not just game over now
				{
					SCREENMAN->SetNewScreen( "ScreenGameOver" );
				}
			}
			break;
		case PLAY_MODE_NONSTOP:
			SCREENMAN->SetNewScreen( "ScreenEvaluationNonstop" );
			break;
		case PLAY_MODE_ONI:
			SCREENMAN->SetNewScreen( "ScreenEvaluationOni" );
			break;
		case PLAY_MODE_ENDLESS:
			SCREENMAN->SetNewScreen( "ScreenEvaluationEndless" );
			break;
		default:
			ASSERT(0);
		}
	}
}


void ScreenGameplay::TweenOnScreen()
{
	ON_COMMAND( m_sprLifeFrame );
	ON_COMMAND( m_sprStage );
	ON_COMMAND( m_textSongOptions );
	ON_COMMAND( m_sprScoreFrame );
	ON_COMMAND( m_textSongTitle );
	if( m_pCombinedLifeMeter )
		ON_COMMAND( *m_pCombinedLifeMeter );
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( m_pLifeMeter[p] )
			ON_COMMAND( *m_pLifeMeter[p] );
		if( !GAMESTATE->IsPlayerEnabled(p) )
			continue;
		ON_COMMAND( m_textCourseSongNumber[p] );
		if( m_pPrimaryScoreDisplay[p] )
			ON_COMMAND( *m_pPrimaryScoreDisplay[p] );
		if( m_pSecondaryScoreDisplay[p] )
			ON_COMMAND( *m_pSecondaryScoreDisplay[p] );
		ON_COMMAND( m_textPlayerOptions[p] );
		ON_COMMAND( m_DifficultyIcon[p] );
	}
}

void ScreenGameplay::TweenOffScreen()
{
	OFF_COMMAND( m_sprLifeFrame );
	OFF_COMMAND( m_sprStage );
	OFF_COMMAND( m_textSongOptions );
	OFF_COMMAND( m_sprScoreFrame );
	OFF_COMMAND( m_textSongTitle );
	if( m_pCombinedLifeMeter )
		OFF_COMMAND( *m_pCombinedLifeMeter );
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( m_pLifeMeter[p] )
			OFF_COMMAND( *m_pLifeMeter[p] );
		if( !GAMESTATE->IsPlayerEnabled(p) )
			continue;
		OFF_COMMAND( m_textCourseSongNumber[p] );
		if( m_pPrimaryScoreDisplay[p] )
			OFF_COMMAND( *m_pPrimaryScoreDisplay[p] );
		if( m_pSecondaryScoreDisplay[p] )
			OFF_COMMAND( *m_pSecondaryScoreDisplay[p] );
		OFF_COMMAND( m_textPlayerOptions[p] );
		OFF_COMMAND( m_DifficultyIcon[p] );
	}

	m_textDebug.StopTweening();
	m_textDebug.BeginTweening( 1/8.f );
	m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
}

void ScreenGameplay::ShowOniGameOver( PlayerNumber pn )
{
	m_sprOniGameOver[pn].SetDiffuse( RageColor(1,1,1,1) );
	m_sprOniGameOver[pn].BeginTweening( 0.5f, Actor::TWEEN_BOUNCE_END );
	m_sprOniGameOver[pn].SetY( CENTER_Y );
	m_sprOniGameOver[pn].SetEffectBob( 4, RageVector3(0,6,0) );
}
