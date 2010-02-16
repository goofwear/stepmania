#include "global.h"
#include "ScreenSelectMusic.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "GameManager.h"
#include "GameSoundManager.h"
#include "GameConstantsAndTypes.h"
#include "RageLog.h"
#include "InputMapper.h"
#include "GameState.h"
#include "CodeDetector.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "ActorUtil.h"
#include "RageTextureManager.h"
#include "Course.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "MenuTimer.h"
#include "StatsManager.h"
#include "StepsUtil.h"
#include "Foreach.h"
#include "Style.h"
#include "PlayerState.h"
#include "CommonMetrics.h"
#include "BannerCache.h"
#include "Song.h"
#include "InputEventPlus.h"
#include "RageInput.h"
#include "OptionsList.h"

static const char *SelectionStateNames[] = {
	"SelectingSong",
	"SelectingSteps",
	"Finalized"
};
XToString( SelectionState );


const int NUM_SCORE_DIGITS = 9;

#define SHOW_OPTIONS_MESSAGE_SECONDS		THEME->GetMetricF( m_sName, "ShowOptionsMessageSeconds" )

AutoScreenMessage( SM_AllowOptionsMenuRepeat )
AutoScreenMessage( SM_SongChanged )
AutoScreenMessage( SM_SortOrderChanging )
AutoScreenMessage( SM_SortOrderChanged )
AutoScreenMessage( SM_BackFromPlayerOptions )

static RString g_sCDTitlePath;
static bool g_bWantFallbackCdTitle;
static bool g_bCDTitleWaiting = false;
static RString g_sBannerPath;
static bool g_bBannerWaiting = false;
static bool g_bSampleMusicWaiting = false;
static RageTimer g_StartedLoadingAt(RageZeroTimer);
static RageTimer g_ScreenStartedLoadingAt(RageZeroTimer);
RageTimer g_CanOpenOptionsList(RageZeroTimer);

REGISTER_SCREEN_CLASS( ScreenSelectMusic );
void ScreenSelectMusic::Init()
{
	g_ScreenStartedLoadingAt.Touch();
	if( PREFSMAN->m_sTestInitialScreen.Get() == m_sName )
	{
		GAMESTATE->m_PlayMode.Set( PLAY_MODE_REGULAR );
		GAMESTATE->SetCurrentStyle( GAMEMAN->GameAndStringToStyle(GAMEMAN->GetDefaultGame(),"versus") );
		GAMESTATE->JoinPlayer( PLAYER_1 );
		GAMESTATE->m_MasterPlayerNumber = PLAYER_1;
	}

	SAMPLE_MUSIC_DELAY_INIT.Load( m_sName, "SampleMusicDelayInit" );
	SAMPLE_MUSIC_DELAY.Load( m_sName, "SampleMusicDelay" );
	SAMPLE_MUSIC_LOOPS.Load( m_sName, "SampleMusicLoops" );
	SAMPLE_MUSIC_FALLBACK_FADE_IN_SECONDS.Load( m_sName, "SampleMusicFallbackFadeInSeconds" );
	DO_ROULETTE_ON_MENU_TIMER.Load( m_sName, "DoRouletteOnMenuTimer" );
	ALIGN_MUSIC_BEATS.Load( m_sName, "AlignMusicBeat" );
	CODES.Load( m_sName, "Codes" );
	MUSIC_WHEEL_TYPE.Load( m_sName, "MusicWheelType" );
	SELECT_MENU_AVAILABLE.Load( m_sName, "SelectMenuAvailable" );
	MODE_MENU_AVAILABLE.Load( m_sName, "ModeMenuAvailable" );
	USE_OPTIONS_LIST.Load( m_sName, "UseOptionsList" );
	USE_PLAYER_SELECT_MENU.Load( m_sName, "UsePlayerSelectMenu" );
	SELECT_MENU_NAME.Load( m_sName, "SelectMenuScreenName" );
	OPTIONS_LIST_TIMEOUT.Load( m_sName, "OptionsListTimeout" );
	SELECT_MENU_CHANGES_DIFFICULTY.Load( m_sName, "SelectMenuChangesDifficulty" );
	TWO_PART_SELECTION.Load( m_sName, "TwoPartSelection" );
	WRAP_CHANGE_STEPS.Load( m_sName, "WrapChangeSteps" );

	m_GameButtonPreviousSong = INPUTMAPPER->GetInputScheme()->ButtonNameToIndex( THEME->GetMetric(m_sName,"PreviousSongButton") );
	m_GameButtonNextSong = INPUTMAPPER->GetInputScheme()->ButtonNameToIndex( THEME->GetMetric(m_sName,"NextSongButton") );

	FOREACH_ENUM( PlayerNumber, p )
	{
		m_bSelectIsDown[p] = false; // used by UpdateSelectButton
		m_bAcceptSelectRelease[p] = false;
	}

	ScreenWithMenuElements::Init();

	this->SubscribeToMessage( Message_PlayerJoined );

	/* Cache: */
	m_sSectionMusicPath =		THEME->GetPathS(m_sName,"section music");
	m_sSortMusicPath =		THEME->GetPathS(m_sName,"sort music");
	m_sRouletteMusicPath =		THEME->GetPathS(m_sName,"roulette music");
	m_sRandomMusicPath =		THEME->GetPathS(m_sName,"random music");
	m_sCourseMusicPath =		THEME->GetPathS(m_sName,"course music");
	m_sLoopMusicPath =		THEME->GetPathS(m_sName,"loop music");
	m_sFallbackCDTitlePath =	THEME->GetPathG(m_sName,"fallback cdtitle");


	m_TexturePreload.Load( m_sFallbackCDTitlePath );

	if( PREFSMAN->m_BannerCache != BNCACHE_OFF )
	{
		m_TexturePreload.Load( Banner::SongBannerTexture(THEME->GetPathG("Banner","all music")) );
		m_TexturePreload.Load( Banner::SongBannerTexture(THEME->GetPathG("Common","fallback banner")) );
		m_TexturePreload.Load( Banner::SongBannerTexture(THEME->GetPathG("Banner","roulette")) );
		m_TexturePreload.Load( Banner::SongBannerTexture(THEME->GetPathG("Banner","random")) );
		m_TexturePreload.Load( Banner::SongBannerTexture(THEME->GetPathG("Banner","mode")) );
	}

	/* Load low-res banners, if needed. */
	BANNERCACHE->Demand();

	m_MusicWheel.SetName( "MusicWheel" );
	m_MusicWheel.Load( MUSIC_WHEEL_TYPE );
	LOAD_ALL_COMMANDS_AND_SET_XY( m_MusicWheel );
	this->AddChild( &m_MusicWheel );

	if( USE_OPTIONS_LIST )
	{
		FOREACH_PlayerNumber(p)
		{
			m_OptionsList[p].SetName( "OptionsList" + PlayerNumberToString(p) );
			m_OptionsList[p].Load( "OptionsList", p );
			m_OptionsList[p].SetDrawOrder( 100 );
			ActorUtil::LoadAllCommands( m_OptionsList[p], m_sName );
			this->AddChild( &m_OptionsList[p] );
		}
		m_OptionsList[PLAYER_1].Link( &m_OptionsList[PLAYER_2] );
		m_OptionsList[PLAYER_2].Link( &m_OptionsList[PLAYER_1] );
	}

	// this is loaded SetSong and TweenToSong
	m_Banner.SetName( "Banner" );
	LOAD_ALL_COMMANDS_AND_SET_XY( m_Banner );
	this->AddChild( &m_Banner );

	m_sprCDTitleFront.SetName( "CDTitle" );
	m_sprCDTitleFront.Load( THEME->GetPathG(m_sName,"fallback cdtitle") );
	LOAD_ALL_COMMANDS_AND_SET_XY( m_sprCDTitleFront );
	COMMAND( m_sprCDTitleFront, "Front" );
	this->AddChild( &m_sprCDTitleFront );

	m_sprCDTitleBack.SetName( "CDTitle" );
	m_sprCDTitleBack.Load( THEME->GetPathG(m_sName,"fallback cdtitle") );
	LOAD_ALL_COMMANDS_AND_SET_XY( m_sprCDTitleBack );
	COMMAND( m_sprCDTitleBack, "Back" );
	this->AddChild( &m_sprCDTitleBack );

	FOREACH_ENUM( PlayerNumber, p )
	{
		m_sprHighScoreFrame[p].SetName( ssprintf("ScoreFrameP%d",p+1) );
		m_sprHighScoreFrame[p].Load( THEME->GetPathG(m_sName,ssprintf("score frame p%d",p+1)) );
		LOAD_ALL_COMMANDS_AND_SET_XY( m_sprHighScoreFrame[p] );
		this->AddChild( &m_sprHighScoreFrame[p] );

		m_textHighScore[p].SetName( ssprintf("ScoreP%d",p+1) );
		m_textHighScore[p].LoadFromFont( THEME->GetPathF(m_sName,"score") );
		m_textHighScore[p].SetShadowLength( 0 );
		m_textHighScore[p].RunCommands( CommonMetrics::PLAYER_COLOR.GetValue(p) );
		LOAD_ALL_COMMANDS_AND_SET_XY( m_textHighScore[p] );
		this->AddChild( &m_textHighScore[p] );
	}	

	RageSoundLoadParams SoundParams;
	SoundParams.m_bSupportPan = true;

	m_soundStart.Load( THEME->GetPathS(m_sName,"start") );
	m_soundDifficultyEasier.Load( THEME->GetPathS(m_sName,"difficulty easier"), false, &SoundParams );
	m_soundDifficultyHarder.Load( THEME->GetPathS(m_sName,"difficulty harder"), false, &SoundParams );
	m_soundOptionsChange.Load( THEME->GetPathS(m_sName,"options") );
	m_soundLocked.Load( THEME->GetPathS(m_sName,"locked") );

	this->SortByDrawOrder();
}

void ScreenSelectMusic::BeginScreen()
{
	g_ScreenStartedLoadingAt.Touch();

	if( CommonMetrics::AUTO_SET_STYLE )
	{
		vector<StepsType> vst;
		GAMEMAN->GetStepsTypesForGame( GAMESTATE->m_pCurGame, vst );
		const Style *pStyle = GAMEMAN->GetFirstCompatibleStyle( GAMESTATE->m_pCurGame, GAMESTATE->GetNumSidesJoined(), vst[0] );
		GAMESTATE->SetCurrentStyle( pStyle );
	}

	if( GAMESTATE->GetCurrentStyle() == NULL )
		RageException::Throw( "The Style has not been set.  A theme must set the Style before loading ScreenSelectMusic." );

	if( GAMESTATE->m_PlayMode == PlayMode_Invalid )
	{
	/* Instead of crashing here, let's just set the PlayMode to regular */
		GAMESTATE->m_PlayMode.Set( PLAY_MODE_REGULAR );
		LOG->Trace( "PlayMode not set, setting as regular." );
	}

	FOREACH_ENUM( PlayerNumber, pn )
	{
		if( GAMESTATE->IsHumanPlayer(pn) )
			continue;
		m_sprHighScoreFrame[pn].SetVisible( false );
		m_textHighScore[pn].SetVisible( false );
	}
	
	OPTIONS_MENU_AVAILABLE.Load( m_sName, "OptionsMenuAvailable" );
	PlayCommand( "Mods" );
	m_MusicWheel.BeginScreen();
	
	m_SelectionState = SelectionState_SelectingSong;
	ZERO( m_bStepsChosen );
	m_bGoToOptions = false;
	m_bAllowOptionsMenu = m_bAllowOptionsMenuRepeat = false;
	ZERO( m_iSelection );

	if( USE_OPTIONS_LIST )
		FOREACH_PlayerNumber( pn )
			m_OptionsList[pn].Reset();

	AfterMusicChange();

	SOUND->PlayOnceFromAnnouncer( "select music intro" );

	ScreenWithMenuElements::BeginScreen();
}

ScreenSelectMusic::~ScreenSelectMusic()
{
	LOG->Trace( "ScreenSelectMusic::~ScreenSelectMusic()" );
	BANNERCACHE->Undemand();

}

/* If bForce is true, the next request will be started even if it might cause a skip. */
void ScreenSelectMusic::CheckBackgroundRequests( bool bForce )
{
	if( g_bCDTitleWaiting )
	{
		/* The CDTitle is normally very small, so we don't bother waiting to display it. */
		RString sPath;
		if( !m_BackgroundLoader.IsCacheFileFinished(g_sCDTitlePath, sPath) )
			return;

		g_bCDTitleWaiting = false;

		RString sCDTitlePath = sPath;

		if( sCDTitlePath.empty() || !IsAFile(sCDTitlePath) )
			sCDTitlePath = g_bWantFallbackCdTitle? m_sFallbackCDTitlePath:RString("");

		if( !sCDTitlePath.empty() )
		{
			TEXTUREMAN->DisableOddDimensionWarning();
			m_sprCDTitleFront.Load( sCDTitlePath );
			m_sprCDTitleBack.Load( sCDTitlePath );
			TEXTUREMAN->EnableOddDimensionWarning();
		}

		m_BackgroundLoader.FinishedWithCachedFile( g_sCDTitlePath );
	}

	/* Loading the rest can cause small skips, so don't do it until the wheel settles. 
	 * Do load if we're transitioning out, though, so we don't miss starting the music
	 * for the options screen if a song is selected quickly.  Also, don't do this
	 * if the wheel is locked, since we're just bouncing around after selecting TYPE_RANDOM,
	 * and it'll take a while before the wheel will settle. */
	if( !m_MusicWheel.IsSettled() && !m_MusicWheel.WheelIsLocked() && !bForce )
		return;

	if( g_bBannerWaiting )
	{
		if( m_Banner.GetTweenTimeLeft() > 0 )
			return;

		RString sPath;
		bool bFreeCache = false;
		if( TEXTUREMAN->IsTextureRegistered( Sprite::SongBannerTexture(g_sBannerPath) ) )
		{
			/* If the file is already loaded into a texture, it's finished,
			 * and we only do this to honor the HighQualTime value. */
			sPath = g_sBannerPath;
		}
		else
		{
			if( !m_BackgroundLoader.IsCacheFileFinished( g_sBannerPath, sPath ) )
				return;
			bFreeCache = true;
		}

		g_bBannerWaiting = false;
		m_Banner.Load( sPath, true );

		if( bFreeCache )
			m_BackgroundLoader.FinishedWithCachedFile( g_sBannerPath );
	}

	/* Nothing else is going.  Start the music, if we haven't yet. */
	if( g_bSampleMusicWaiting )
	{
		if(g_ScreenStartedLoadingAt.Ago() < SAMPLE_MUSIC_DELAY_INIT)
			return;

		/* Don't start the music sample when moving fast. */
		if( g_StartedLoadingAt.Ago() < SAMPLE_MUSIC_DELAY && !bForce )
			return;

		g_bSampleMusicWaiting = false;

		GameSoundManager::PlayMusicParams PlayParams;
		PlayParams.sFile = m_sSampleMusicToPlay;
		PlayParams.pTiming = m_pSampleMusicTimingData;
		PlayParams.bForceLoop = SAMPLE_MUSIC_LOOPS;
		PlayParams.fStartSecond = m_fSampleStartSeconds;
		PlayParams.fLengthSeconds = m_fSampleLengthSeconds;
		PlayParams.fFadeOutLengthSeconds = 1.5f;
		PlayParams.bAlignBeat = ALIGN_MUSIC_BEATS;
		PlayParams.bApplyMusicRate = true;

		GameSoundManager::PlayMusicParams FallbackMusic;
		FallbackMusic.sFile = m_sLoopMusicPath;
		FallbackMusic.fFadeInLengthSeconds = SAMPLE_MUSIC_FALLBACK_FADE_IN_SECONDS;
		FallbackMusic.bAlignBeat = ALIGN_MUSIC_BEATS;

		SOUND->PlayMusic( PlayParams, FallbackMusic );
	}
}

void ScreenSelectMusic::Update( float fDeltaTime )
{
	ScreenWithMenuElements::Update( fDeltaTime );

	CheckBackgroundRequests( false );
}
void ScreenSelectMusic::Input( const InputEventPlus &input )
{
//	LOG->Trace( "ScreenSelectMusic::Input()" );

	bool bHoldingCtrl = 
		INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL)) ||
		INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL));

	wchar_t c = INPUTMAN->DeviceInputToChar(input.DeviceI,false);
	MakeUpper( &c, 1 );

	if ( bHoldingCtrl && ( c >= 'A' ) && ( c <= 'Z' ) )
	{
		// Only allow changing the sort via Control+letter when the wheel isn't locked and we're not in course mode.
		if( !m_MusicWheel.WheelIsLocked() && !GAMESTATE->IsCourseMode() )
		{
			SortOrder so = GAMESTATE->m_SortOrder;
			if ( ( so != SORT_TITLE ) && ( so != SORT_ARTIST ) )
			{
				so = SORT_TITLE;

				GAMESTATE->m_PreferredSortOrder = so;
				GAMESTATE->m_SortOrder.Set( so );
				// Odd, changing the sort order requires us to call SetOpenSection more than once
				m_MusicWheel.ChangeSort( so );
				m_MusicWheel.SetOpenSection( ssprintf("%c", c ) );
			}
			m_MusicWheel.SelectSection( ssprintf("%c", c ) );
			m_MusicWheel.ChangeSort( so );
			m_MusicWheel.SetOpenSection( ssprintf("%c", c ) );
			AfterMusicChange();
			return;
		}
	}

	// debugging?
	// I just like being able to see untransliterated titles occasionally.
	if( input.DeviceI.device == DEVICE_KEYBOARD && input.DeviceI.button == KEY_F9 )
	{
		if( input.type != IET_FIRST_PRESS ) 
			return;
		PREFSMAN->m_bShowNativeLanguage.Set( !PREFSMAN->m_bShowNativeLanguage );
		MESSAGEMAN->Broadcast( "DisplayLanguageChanged" );
		m_MusicWheel.RebuildWheelItems();
		return;
	}

	if( !input.GameI.IsValid() )
		return;		// don't care

	// Handle late joining
	if( m_SelectionState != SelectionState_Finalized  &&  input.MenuI == GAME_BUTTON_START  &&  input.type == IET_FIRST_PRESS  &&  GAMESTATE->JoinInput(input.pn) )
		return;	// don't handle this press again below

	if( !GAMESTATE->IsHumanPlayer(input.pn) )
		return;

	// Check for "Press START again for options" button press
	if( m_SelectionState == SelectionState_Finalized  &&
	    input.MenuI == GAME_BUTTON_START  &&
	    input.type != IET_RELEASE  &&
	    OPTIONS_MENU_AVAILABLE.GetValue() )
	{
		if( m_bGoToOptions )
			return; /* got it already */
		if( !m_bAllowOptionsMenu )
			return; /* not allowed */

		if( !m_bAllowOptionsMenuRepeat && input.type == IET_REPEAT )
		{
			return; /* not allowed yet */
		}
		
		m_bGoToOptions = true;
		m_soundStart.Play();
		this->PlayCommand( "ShowEnteringOptions" );

		// Re-queue SM_BeginFadingOut, since ShowEnteringOptions may have
		// short-circuited animations.
		this->ClearMessageQueue( SM_BeginFadingOut );
		this->PostScreenMessage( SM_BeginFadingOut, this->GetTweenTimeLeft() );

		return;
	}

	if( IsTransitioning() )
		return;		// ignore

	// Handle unselect steps
	if( m_SelectionState == SelectionState_SelectingSteps  &&  m_bStepsChosen[input.pn]  &&  input.MenuI == GAME_BUTTON_SELECT  &&  input.type == IET_FIRST_PRESS )
	{
		Message msg("StepsUnchosen");
		msg.SetParam( "Player", input.pn );
		MESSAGEMAN->Broadcast( msg );
		m_bStepsChosen[input.pn] = false;
		return;
	}

	if( m_SelectionState == SelectionState_Finalized  ||
		m_bStepsChosen[input.pn] )
		return;		// ignore

	if( USE_PLAYER_SELECT_MENU )
	{
		if( input.type == IET_RELEASE  &&  input.MenuI == GAME_BUTTON_SELECT )
		{
			SCREENMAN->AddNewScreenToTop( SELECT_MENU_NAME, SM_BackFromPlayerOptions );
		}
	}

	// handle options list input
	if( USE_OPTIONS_LIST )
	{
		PlayerNumber pn = input.pn;
		if( pn != PLAYER_INVALID )
		{
			if( m_OptionsList[pn].IsOpened() )
			{
				m_OptionsList[pn].Input( input );
				return;
			}
			else
			{
				if( input.type == IET_RELEASE  &&  input.MenuI == GAME_BUTTON_SELECT && m_bAcceptSelectRelease[pn] )
					m_OptionsList[pn].Open();
			}
		}
	}

	if( input.MenuI == GAME_BUTTON_SELECT && input.type != IET_REPEAT )
		m_bAcceptSelectRelease[input.pn] = (input.type == IET_FIRST_PRESS);

	if( SELECT_MENU_AVAILABLE && input.MenuI == GAME_BUTTON_SELECT && input.type != IET_REPEAT )
		UpdateSelectButton( input.pn, input.type == IET_FIRST_PRESS );

	if( SELECT_MENU_AVAILABLE  &&  m_bSelectIsDown[input.pn] )
	{
		if( input.type == IET_FIRST_PRESS && SELECT_MENU_CHANGES_DIFFICULTY )
		{
			switch( input.MenuI )
			{
			case GAME_BUTTON_LEFT:
				ChangeSteps( input.pn, -1 );
				m_bAcceptSelectRelease[input.pn] = false;
				break;
			case GAME_BUTTON_RIGHT:
				ChangeSteps( input.pn, +1 );
				m_bAcceptSelectRelease[input.pn] = false;
				break;
			case GAME_BUTTON_START:
				m_bAcceptSelectRelease[input.pn] = false;
				if( MODE_MENU_AVAILABLE )
					m_MusicWheel.NextSort();
				else
					m_soundLocked.Play();
				break;
			}
		}
		else if( input.type == IET_FIRST_PRESS && input.MenuI != GAME_BUTTON_SELECT )
		{
			Message msg("SelectMenuInput");
			msg.SetParam( "Player", input.pn );
			msg.SetParam( "Button", GameButtonToString(INPUTMAPPER->GetInputScheme(), input.MenuI) );
			MESSAGEMAN->Broadcast( msg );
			m_bAcceptSelectRelease[input.pn] = false;
		}
		if( input.type == IET_FIRST_PRESS )
			g_CanOpenOptionsList.Touch();
		if( g_CanOpenOptionsList.Ago() > OPTIONS_LIST_TIMEOUT )
			m_bAcceptSelectRelease[input.pn] = false;
		return;
	}
	
	if( m_SelectionState == SelectionState_SelectingSong  &&
		(input.MenuI == m_GameButtonNextSong || input.MenuI == m_GameButtonPreviousSong || input.MenuI == GAME_BUTTON_SELECT) )
	{
		{
			/* If we're rouletting, hands off. */
			if( m_MusicWheel.IsRouletting() )
				return;
			
			bool bLeftIsDown = false;
			bool bRightIsDown = false;
			FOREACH_HumanPlayer( p )
			{
				if( m_OptionsList[p].IsOpened() )
					continue;
				if( SELECT_MENU_AVAILABLE && INPUTMAPPER->IsBeingPressed(GAME_BUTTON_SELECT, p) )
					continue;

				bLeftIsDown |= INPUTMAPPER->IsBeingPressed( m_GameButtonPreviousSong, p );
				bRightIsDown |= INPUTMAPPER->IsBeingPressed( m_GameButtonNextSong, p );
			}
			
			bool bBothDown = bLeftIsDown && bRightIsDown;
			bool bNeitherDown = !bLeftIsDown && !bRightIsDown;
			

			if( bNeitherDown )
			{
				/* Both buttons released. */
				m_MusicWheel.Move( 0 );
			}
			else if( bBothDown )
			{
				m_MusicWheel.Move( 0 );
				if( input.type == IET_FIRST_PRESS )
				{
					if( input.MenuI == m_GameButtonPreviousSong )
						m_MusicWheel.ChangeMusicUnlessLocked( -1 );
					else if( input.MenuI == m_GameButtonNextSong )
						m_MusicWheel.ChangeMusicUnlessLocked( +1 );
				}
			}
			else if( bLeftIsDown )
			{
				if( input.type != IET_RELEASE )
				{
					MESSAGEMAN->Broadcast( "PreviousSong" );
					m_MusicWheel.Move( -1 );
				}
			}
			else if( bRightIsDown )
			{
				if( input.type != IET_RELEASE )
				{
					MESSAGEMAN->Broadcast( "NextSong" );
					m_MusicWheel.Move( +1 );
				}
			}
			else
			{
				ASSERT(0);
			}
			
			
			// Reset the repeat timer when the button is released.
			// This fixes jumping when you release Left and Right after entering the sort 
			// code at the same if L & R aren't released at the exact same time.
			if( input.type == IET_RELEASE )
			{
				INPUTMAPPER->ResetKeyRepeat( m_GameButtonPreviousSong, input.pn );
				INPUTMAPPER->ResetKeyRepeat( m_GameButtonNextSong, input.pn );
			}
		}
	}

	if( m_SelectionState == SelectionState_SelectingSteps  &&
		input.type == IET_FIRST_PRESS  &&
		(input.MenuI == m_GameButtonNextSong || input.MenuI == m_GameButtonPreviousSong) &&
		!m_bStepsChosen[input.pn] )
	{
		if( input.MenuI == m_GameButtonPreviousSong )
		{
			if( GAMESTATE->IsAnExtraStageAndSelectionLocked() )
				m_soundLocked.Play();
			else
				ChangeSteps( input.pn, -1 );
		}
		else if( input.MenuI == m_GameButtonNextSong )
		{
			if( GAMESTATE->IsAnExtraStageAndSelectionLocked() )
				m_soundLocked.Play();
			else
				ChangeSteps( input.pn, +1 );
		}		
	}

	if( input.type == IET_FIRST_PRESS && DetectCodes(input) )
		return;

	ScreenWithMenuElements::Input( input );
}

bool ScreenSelectMusic::DetectCodes( const InputEventPlus &input )
{
	if( CodeDetector::EnteredPrevSteps(input.GameI.controller) )
	{
		if( GAMESTATE->IsAnExtraStageAndSelectionLocked() )
			m_soundLocked.Play();
		else
			ChangeSteps( input.pn, -1 );
	}
	else if( CodeDetector::EnteredNextSteps(input.GameI.controller) )
	{
		if( GAMESTATE->IsAnExtraStageAndSelectionLocked() )
			m_soundLocked.Play();
		else
			ChangeSteps( input.pn, +1 );
	}
	else if( CodeDetector::EnteredModeMenu(input.GameI.controller) )
	{
		if( MODE_MENU_AVAILABLE )
			m_MusicWheel.ChangeSort( SORT_MODE_MENU );
		else
			m_soundLocked.Play();
	}
	else if( CodeDetector::EnteredNextSort(input.GameI.controller) )
	{
		if( GAMESTATE->IsAnExtraStageAndSelectionLocked() )
			m_soundLocked.Play();
		else
			m_MusicWheel.NextSort();
	}
	else if( !GAMESTATE->IsAnExtraStageAndSelectionLocked() && CodeDetector::DetectAndAdjustMusicOptions(input.GameI.controller) )
	{
		m_soundOptionsChange.Play();

		Message msg( "PlayerOptionsChanged" );
		msg.SetParam( "PlayerNumber", input.pn );
		MESSAGEMAN->Broadcast( msg );

		MESSAGEMAN->Broadcast( "SongOptionsChanged" );
	}
	else
	{
		return false;
	}
	return true;
}	

void ScreenSelectMusic::UpdateSelectButton( PlayerNumber pn, bool bSelectIsDown )
{
	if( !SELECT_MENU_AVAILABLE  ||  !CanChangeSong() )
		bSelectIsDown = false;

	if( m_bSelectIsDown[pn] != bSelectIsDown )
	{
		m_bSelectIsDown[pn] = bSelectIsDown;
		Message msg( bSelectIsDown ? "SelectMenuOpened" : "SelectMenuClosed" );
		msg.SetParam( "Player", pn );
		MESSAGEMAN->Broadcast( msg );
	}
}

void ScreenSelectMusic::ChangeSteps( PlayerNumber pn, int dir )
{
	LOG->Trace( "ScreenSelectMusic::ChangeSteps( %d, %d )", pn, dir );

	ASSERT( GAMESTATE->IsHumanPlayer(pn) );

	if( GAMESTATE->m_pCurSong )
	{
		m_iSelection[pn] += dir;
		if( WRAP_CHANGE_STEPS )
		{
			wrap( m_iSelection[pn], m_vpSteps.size() );
		}
		else
		{
			if( CLAMP(m_iSelection[pn],0,m_vpSteps.size()-1) )
				return;
		}

		// the user explicity switched difficulties.  Update the preferred Difficulty and StepsType
		Steps *pSteps = m_vpSteps[ m_iSelection[pn] ];
		GAMESTATE->ChangePreferredDifficultyAndStepsType( pn, pSteps->GetDifficulty(), pSteps->m_StepsType );
	}
	else if( GAMESTATE->m_pCurCourse )
	{
		m_iSelection[pn] += dir;
		if( WRAP_CHANGE_STEPS )
		{
			wrap( m_iSelection[pn], m_vpTrails.size() );
		}
		else
		{
			if( CLAMP(m_iSelection[pn],0,m_vpTrails.size()-1) )
				return;
		}

		// the user explicity switched difficulties.  Update the preferred Difficulty and StepsType
		Trail *pTrail = m_vpTrails[ m_iSelection[pn] ];
		GAMESTATE->ChangePreferredCourseDifficultyAndStepsType( pn, pTrail->m_CourseDifficulty, pTrail->m_StepsType );
	}
	else
	{
		// If we're showing multiple StepsTypes in the list, don't allow changing the difficulty/StepsType 
		// when a non-Song, non-Course is selected.  Chaning the preferred Difficulty and StepsType
		// by direction is complicated when multiple StepsTypes are being shown, so we don't support it.
		if( CommonMetrics::AUTO_SET_STYLE )
			return;
		if( !GAMESTATE->ChangePreferredDifficulty( pn, dir ) )
			return;
	}

	vector<PlayerNumber> vpns;
	FOREACH_HumanPlayer( p )
	{
		if( pn == p || GAMESTATE->DifficultiesLocked() )
		{
			m_iSelection[p] = m_iSelection[pn];
			vpns.push_back( p );
		}
	}
	AfterStepsOrTrailChange( vpns );

	float fBalance = GameSoundManager::GetPlayerBalance( pn );
	if( dir < 0 )
	{
		m_soundDifficultyEasier.SetProperty( "Pan", fBalance );
		m_soundDifficultyEasier.PlayCopy();
	}
	else
	{
		m_soundDifficultyHarder.SetProperty( "Pan", fBalance );
		m_soundDifficultyHarder.PlayCopy();
	}

	Message msg( "ChangeSteps" );
	msg.SetParam( "Player", pn );
	MESSAGEMAN->Broadcast( msg );
}


void ScreenSelectMusic::HandleMessage( const Message &msg )
{
	if( m_bRunning && msg == Message_PlayerJoined )
	{
		// The current steps may no longer be playable.  If one player has double steps 
		// selected, they are no longer playable now that P2 has joined.  
		
		// TODO: Invalidate the CurSteps only if they are no longer playable.  That way, 
		// after music change will clamp to the nearest in the StepsDisplayList.
		GAMESTATE->m_pCurSteps[GAMESTATE->m_MasterPlayerNumber].SetWithoutBroadcast( NULL );
		FOREACH_ENUM( PlayerNumber, p )
			GAMESTATE->m_pCurSteps[p].SetWithoutBroadcast( NULL );

		/* If a course is selected, it may no longer be playable.  Let MusicWheel know
		 * about the late join. */
		m_MusicWheel.PlayerJoined();

		AfterMusicChange();

		int iSel = 0;
		PlayerNumber pn;
		bool b = msg.GetParam( "Player", pn );
		ASSERT( b );

		m_iSelection[pn] = iSel;
		if( GAMESTATE->IsCourseMode() )
		{
			Trail* pTrail = m_vpTrails.empty()? NULL: m_vpTrails[m_iSelection[pn]];
			GAMESTATE->m_pCurTrail[pn].Set( pTrail );
		}
		else
		{
			Steps* pSteps = m_vpSteps.empty()? NULL: m_vpSteps[m_iSelection[pn]];
			GAMESTATE->m_pCurSteps[pn].Set( pSteps );
		}
	}
	
	ScreenWithMenuElements::HandleMessage( msg );
}

void ScreenSelectMusic::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_AllowOptionsMenuRepeat )
	{
		m_bAllowOptionsMenuRepeat = true;
	}
	else if( SM == SM_MenuTimer )
	{
		if( m_MusicWheel.IsRouletting() )
		{
			MenuStart( InputEventPlus() );
			m_MenuTimer->SetSeconds( 15 );
			m_MenuTimer->Start();
		}
		else if( DO_ROULETTE_ON_MENU_TIMER  &&  m_MusicWheel.GetSelectedSong() == NULL  &&  m_MusicWheel.GetSelectedCourse() == NULL )
		{
			m_MusicWheel.StartRoulette();
			m_MenuTimer->SetSeconds( 15 );
			m_MenuTimer->Start();
		}
		else
		{
			// Finish sort changing so that the wheel can respond immediately to our
			// request to choose random.
			m_MusicWheel.FinishChangingSorts();
			if( m_MusicWheel.GetSelectedSong() == NULL && m_MusicWheel.GetSelectedCourse() == NULL )
				m_MusicWheel.StartRandom();

			MenuStart( InputEventPlus() );
		}
		return;
	}
	else if( SM == SM_GoToPrevScreen )
	{
		/* We may have stray SM_SongChanged messages from the music wheel.  We can't
		 * handle them anymore, since the title menu (and attract screens) reset
		 * the game state, so just discard them. */
		ClearMessageQueue();
	}
	else if( SM == SM_BeginFadingOut )
	{
		m_bAllowOptionsMenu = false;
		if( OPTIONS_MENU_AVAILABLE && !m_bGoToOptions )
			this->PlayCommand( "HidePressStartForOptions" );

		this->PostScreenMessage( SM_GoToNextScreen, this->GetTweenTimeLeft() );
	}
	else if( SM == SM_GoToNextScreen )
	{
		if( !m_bGoToOptions )
			SOUND->StopMusic();
	}
	else if( SM == SM_SongChanged )
	{
		AfterMusicChange();
	}
	else if( SM == SM_SortOrderChanging ) /* happens immediately */
	{
		this->PlayCommand( "SortChange" );
	}
	else if( SM == SM_GainFocus )
	{
		CodeDetector::RefreshCacheItems( CODES );
	}
	else if( SM == SM_LoseFocus )
	{
		CodeDetector::RefreshCacheItems(); /* reset for other screens */
	}

	ScreenWithMenuElements::HandleScreenMessage( SM );
}

void ScreenSelectMusic::MenuStart( const InputEventPlus &input )
{
	if( input.type != IET_FIRST_PRESS )
		return;

	/* If select is being pressed, this is probably an attempt to change the sort, not
	 * to pick a song or difficulty.  If it gets here, the actual select press was probably
	 * hit during a tween and ignored.  Ignore it. */
	if( input.pn != PLAYER_INVALID && INPUTMAPPER->IsBeingPressed(GAME_BUTTON_SELECT, input.pn) )
		return;

	// Honor locked input for start presses.
	if( m_fLockInputSecs > 0 )
		return;

	switch( m_SelectionState )
	{
	DEFAULT_FAIL( m_SelectionState );
	case SelectionState_SelectingSong:

		/* If false, we don't have a selection just yet. */
		if( !m_MusicWheel.Select() )
			return;

		// a song was selected
		if( m_MusicWheel.GetSelectedSong() != NULL )
		{
			const bool bIsNew = PROFILEMAN->IsSongNew( m_MusicWheel.GetSelectedSong() );
			bool bIsHard = false;
			FOREACH_HumanPlayer( p )
			{
				if( GAMESTATE->m_pCurSteps[p]  &&  GAMESTATE->m_pCurSteps[p]->GetMeter() >= 10 )
					bIsHard = true;
			}

			/* See if this song is a repeat.  If we're in event mode, only check the last five songs. */
			bool bIsRepeat = false;
			int i = 0;
			if( GAMESTATE->IsEventMode() )
				i = max( 0, int(STATSMAN->m_vPlayedStageStats.size())-5 );
			for( ; i < (int)STATSMAN->m_vPlayedStageStats.size(); ++i )
				if( STATSMAN->m_vPlayedStageStats[i].m_vpPlayedSongs.back() == m_MusicWheel.GetSelectedSong() )
					bIsRepeat = true;

			/* Don't complain about repeats if the user didn't get to pick. */
			if( GAMESTATE->IsAnExtraStageAndSelectionLocked() )
				bIsRepeat = false;

			if( bIsRepeat )
				SOUND->PlayOnceFromAnnouncer( "select music comment repeat" );
			else if( bIsNew )
				SOUND->PlayOnceFromAnnouncer( "select music comment new" );
			else if( bIsHard )
				SOUND->PlayOnceFromAnnouncer( "select music comment hard" );
			else
				SOUND->PlayOnceFromAnnouncer( "select music comment general" );

			/* If we're in event mode, we may have just played a course (putting us
			* in course mode).  Make sure we're in a single song mode. */
			if( GAMESTATE->IsCourseMode() )
				GAMESTATE->m_PlayMode.Set( PLAY_MODE_REGULAR );
		}
		else if( m_MusicWheel.GetSelectedCourse() != NULL )
		{
			SOUND->PlayOnceFromAnnouncer( "select course comment general" );

			Course *pCourse = m_MusicWheel.GetSelectedCourse();
			ASSERT( pCourse );
			GAMESTATE->m_PlayMode.Set( pCourse->GetPlayMode() );

			// apply #LIVES
			if( pCourse->m_iLives != -1 )
			{
				SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Stage, m_LifeType, SongOptions::LIFE_BATTERY );
				SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Stage, m_iBatteryLives, pCourse->m_iLives );
			}
			if( pCourse->GetCourseType() == COURSE_TYPE_SURVIVAL)
				SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Stage, m_LifeType, SongOptions::LIFE_TIME );
		}
		else
		{
			/* We haven't made a selection yet. */
			return;
		}

		MESSAGEMAN->Broadcast("SongChosen");

		break;

	case SelectionState_SelectingSteps:
		{
			PlayerNumber pn = input.pn;
			bool bInitiatedByMenuTimer = pn == PLAYER_INVALID;
			bool bAllOtherHumanPlayersDone = true;
			FOREACH_HumanPlayer( p )
			{
				if( p == pn )
					continue;
				bAllOtherHumanPlayersDone &= m_bStepsChosen[p];
			}

			bool bAllPlayersDoneSelectingSteps = bInitiatedByMenuTimer || bAllOtherHumanPlayersDone;

			/* 
			 * TRICKY: if we have a Routine chart selected, we need to ensure the following:
			 *
			 * 1. Both players must select the same Routine steps.
			 * 2. If the other player picks non-Routine steps, this player cannot pick Routine.
			 * 3. If the other player picked Routine steps, and we pick non-Routine steps, the other
			 *    player's steps must be unselected.
			 * 4. If time runs out, and both players don't have the same Routine chart selected,
			 *	  we need to bump the player with a Routine chart selection to a playable chart.
			 *    (Right now, we bump them to Beginner...can we come up with something better?)
			 */

			bool bSelectedRoutineSteps[NUM_PLAYERS], bAnySelectedRoutine = false;
			bool bSelectedSameSteps = GAMESTATE->m_pCurSteps[PLAYER_1] == GAMESTATE->m_pCurSteps[PLAYER_2];

			FOREACH_HumanPlayer( p )
			{
				const Steps *pSteps = GAMESTATE->m_pCurSteps[p];
				const StepsTypeInfo &sti = GAMEMAN->GetStepsTypeInfo( pSteps->m_StepsType );

				bSelectedRoutineSteps[p] = sti.m_StepsTypeCategory == StepsTypeCategory_Routine;
				bAnySelectedRoutine |= bSelectedRoutineSteps[p];
			}

			if( bAnySelectedRoutine )
			{
				/* Timer ran out. If we haven't agreed on steps, move players with Routine steps down
				 * to Beginner. I'll admit that's annoying, but at least they won't lose more stages. */
				if( bInitiatedByMenuTimer && !bSelectedSameSteps )
				{
					/* Since m_vpSteps is sorted by Difficulty, the first entry should be the easiest. */
					ASSERT( m_vpSteps.size() );
					Steps *pSteps = m_vpSteps[0];

					FOREACH_PlayerNumber( p )
					{
						if( bSelectedRoutineSteps[p] )
							GAMESTATE->m_pCurSteps[p].Set( pSteps );
					}

					break;
				}

				/* If the steps don't match up, we need to check some more conditions... */
				if( !bSelectedSameSteps )
				{
					const PlayerNumber other = OPPOSITE_PLAYER[pn];

					if( m_bStepsChosen[other] )
					{
						/* Unready the other player if they selected Routine steps, but we didn't. */
						if( bSelectedRoutineSteps[other] )
						{
							m_bStepsChosen[other] = false;
							bAllPlayersDoneSelectingSteps = false;	// if the timer ran out, we handled it earlier

							// HACK: send an event to Input to tell it to unready.
							InputEventPlus event;
							event.MenuI = GAME_BUTTON_SELECT;
							event.pn = other;

							this->Input( event );
						}
						else if( bSelectedRoutineSteps[pn] )
						{
							/* They selected non-Routine steps, so we can't select Routine steps. */
							return;
						}
					}
				}
			}

			if( !bAllPlayersDoneSelectingSteps )
			{
				m_bStepsChosen[pn] = true;
				m_soundStart.Play();

				Message msg("StepsChosen");
				msg.SetParam( "Player", pn );
				MESSAGEMAN->Broadcast( msg );
				return;
			}
		}
		break;
	}

	FOREACH_ENUM( PlayerNumber, p )
	{
		if( !TWO_PART_SELECTION || m_SelectionState == SelectionState_SelectingSteps )
		{
			if( m_OptionsList[p].IsOpened() )
				m_OptionsList[p].Close();
		}
		UpdateSelectButton( p, false );
	}

	m_SelectionState = GetNextSelectionState();
	Message msg( "Start" + SelectionStateToString(m_SelectionState) );
	MESSAGEMAN->Broadcast( msg );

	m_soundStart.Play();


	if( m_SelectionState == SelectionState_Finalized )
	{
		m_MenuTimer->Stop();


		FOREACH_HumanPlayer( p )
		{
			if( !m_bStepsChosen[p] )
			{
				m_bStepsChosen[p] = true;
				/* Don't play start sound.  We play it again below on finalized */
				//m_soundStart.Play();

				Message msg("StepsChosen");
				msg.SetParam( "Player", p );
				MESSAGEMAN->Broadcast( msg );
			}
		}

		if( CommonMetrics::AUTO_SET_STYLE )
		{
			/* Now that Steps have been chosen, set a Style that can play them. */
			const Style *pStyle = NULL;
			if( GAMESTATE->IsCourseMode() )
				pStyle = GAMESTATE->m_pCurCourse->GetCourseStyle( GAMESTATE->m_pCurGame, GAMESTATE->GetNumSidesJoined() );
			if( pStyle == NULL )
			{
				StepsType stCurrent;
				PlayerNumber pn = GAMESTATE->m_MasterPlayerNumber;
				if( GAMESTATE->IsCourseMode() )
				{
					ASSERT( GAMESTATE->m_pCurTrail[pn] );
					stCurrent = GAMESTATE->m_pCurTrail[pn]->m_StepsType;
				}
				else
				{
					ASSERT( GAMESTATE->m_pCurSteps[pn] );
					stCurrent = GAMESTATE->m_pCurSteps[pn]->m_StepsType;
				}
				vector<StepsType> vst;
				pStyle = GAMEMAN->GetFirstCompatibleStyle( GAMESTATE->m_pCurGame, GAMESTATE->GetNumSidesJoined(), stCurrent );
			}
			GAMESTATE->SetCurrentStyle( pStyle );
		}


		/* If we're currently waiting on song assets, abort all except the music and
		* start the music, so if we make a choice quickly before background requests
		* come through, the music will still start. */
		g_bCDTitleWaiting = g_bBannerWaiting = false;
		m_BackgroundLoader.Abort();
		CheckBackgroundRequests( true );

		if( OPTIONS_MENU_AVAILABLE )
		{
			// show "hold START for options"
			this->PlayCommand( "ShowPressStartForOptions" );

			m_bAllowOptionsMenu = true;

			/* Don't accept a held START for a little while, so it's not
			* hit accidentally.  Accept an initial START right away, though,
			* so we don't ignore deliberate fast presses (which would be
			* annoying). */
			this->PostScreenMessage( SM_AllowOptionsMenuRepeat, 0.5f );

			StartTransitioningScreen( SM_None );
			float fTime = max( SHOW_OPTIONS_MESSAGE_SECONDS, this->GetTweenTimeLeft() );
			this->PostScreenMessage( SM_BeginFadingOut, fTime );
		}
		else
		{
			StartTransitioningScreen( SM_BeginFadingOut );
		}
	}
	else // !finalized.  Set the timer for selecting difficulty and mods.
	{
		float fSeconds = m_MenuTimer->GetSeconds();
		if( fSeconds < 10 )
		{
			m_MenuTimer->SetSeconds( 13 );
			m_MenuTimer->Start();
		}
	}
}


void ScreenSelectMusic::MenuBack( const InputEventPlus &input )
{
	m_BackgroundLoader.Abort();

	Cancel( SM_GoToPrevScreen );
}

void ScreenSelectMusic::AfterStepsOrTrailChange( const vector<PlayerNumber> &vpns )
{
	FOREACH_CONST( PlayerNumber, vpns, p )
	{
		PlayerNumber pn = *p;
		ASSERT( GAMESTATE->IsHumanPlayer(pn) );
		
		if( GAMESTATE->m_pCurSong )
		{
			CLAMP( m_iSelection[pn], 0, m_vpSteps.size()-1 );

			Song* pSong = GAMESTATE->m_pCurSong;
			Steps* pSteps = m_vpSteps.empty()? NULL: m_vpSteps[m_iSelection[pn]];

			GAMESTATE->m_pCurSteps[pn].Set( pSteps );
			GAMESTATE->m_pCurTrail[pn].Set( NULL );

			int iScore = 0;
			if( pSteps )
			{
				const Profile *pProfile = PROFILEMAN->IsPersistentProfile(pn) ? PROFILEMAN->GetProfile(pn) : PROFILEMAN->GetMachineProfile();
				iScore = pProfile->GetStepsHighScoreList(pSong,pSteps).GetTopScore().GetScore();
			}

			m_textHighScore[pn].SetText( ssprintf("%*i", NUM_SCORE_DIGITS, iScore) );
		}
		else if( GAMESTATE->m_pCurCourse )
		{
			CLAMP( m_iSelection[pn], 0, m_vpTrails.size()-1 );

			Course* pCourse = GAMESTATE->m_pCurCourse;
			Trail* pTrail = m_vpTrails.empty()? NULL: m_vpTrails[m_iSelection[pn]];
			
			GAMESTATE->m_pCurSteps[pn].Set( NULL );			
			GAMESTATE->m_pCurTrail[pn].Set( pTrail );

			int iScore = 0;
			if( pTrail )
			{
				const Profile *pProfile = PROFILEMAN->IsPersistentProfile(pn) ? PROFILEMAN->GetProfile(pn) : PROFILEMAN->GetMachineProfile();
				iScore = pProfile->GetCourseHighScoreList(pCourse,pTrail).GetTopScore().GetScore();
			}

			m_textHighScore[pn].SetText( ssprintf("%*i", NUM_SCORE_DIGITS, iScore) );
		}
	}
}

void ScreenSelectMusic::SwitchToPreferredDifficulty()
{
	if( !GAMESTATE->m_pCurCourse )
	{
		FOREACH_HumanPlayer( pn )
		{
			/* Find the closest match to the user's preferred difficulty and StepsType. */
			int iCurDifference = -1;
			int &iSelection = m_iSelection[pn];
			FOREACH_CONST( Steps*, m_vpSteps, s )
			{
				int i = s - m_vpSteps.begin();

				/* If the current steps are listed, use them. */
				if( GAMESTATE->m_pCurSteps[pn] == *s )
				{
					iSelection = i;
					break;
				}

				if( GAMESTATE->m_PreferredDifficulty[pn] != Difficulty_Invalid  )
				{
					int iDifficultyDifference = abs( (*s)->GetDifficulty() - GAMESTATE->m_PreferredDifficulty[pn] );
					int iStepsTypeDifference = 0;
					if( GAMESTATE->m_PreferredStepsType != StepsType_Invalid )
						iStepsTypeDifference = abs( (*s)->m_StepsType - GAMESTATE->m_PreferredStepsType );
					int iTotalDifference = iStepsTypeDifference * NUM_Difficulty + iDifficultyDifference;

					if( iCurDifference == -1 || iTotalDifference < iCurDifference )
					{
						iSelection = i;
						iCurDifference = iTotalDifference;
					}
				}
			}

			CLAMP( iSelection, 0, m_vpSteps.size()-1 );
		}
	}
	else
	{
		FOREACH_HumanPlayer( pn )
		{
			/* Find the closest match to the user's preferred difficulty. */
			int iCurDifference = -1;
			int &iSelection = m_iSelection[pn];
			FOREACH_CONST( Trail*, m_vpTrails, t )
			{
				int i = t - m_vpTrails.begin();

				/* If the current trail is listed, use it. */
				if( GAMESTATE->m_pCurTrail[pn] == m_vpTrails[i] )
				{
					iSelection = i;
					break;
				}

				if( GAMESTATE->m_PreferredCourseDifficulty[pn] != Difficulty_Invalid  &&  GAMESTATE->m_PreferredStepsType != StepsType_Invalid  )
				{
					int iDifficultyDifference = abs( (*t)->m_CourseDifficulty - GAMESTATE->m_PreferredCourseDifficulty[pn] );
					int iStepsTypeDifference = abs( (*t)->m_StepsType - GAMESTATE->m_PreferredStepsType );
					int iTotalDifference = iStepsTypeDifference * NUM_CourseDifficulty + iDifficultyDifference;

					if( iCurDifference == -1 || iTotalDifference < iCurDifference )
					{
						iSelection = i;
						iCurDifference = iTotalDifference;
					}
				}
			}

			CLAMP( iSelection, 0, m_vpTrails.size()-1 );
		}
	}

	if( GAMESTATE->DifficultiesLocked() )
	{
		FOREACH_HumanPlayer( p )
			m_iSelection[p] = m_iSelection[GAMESTATE->m_MasterPlayerNumber];
	}
}

void ScreenSelectMusic::AfterMusicChange()
{
	if( !m_MusicWheel.IsRouletting() )
		m_MenuTimer->Stall();

	Song* pSong = m_MusicWheel.GetSelectedSong();
	GAMESTATE->m_pCurSong.Set( pSong );
	if( pSong )
		GAMESTATE->m_pPreferredSong = pSong;

	Course* pCourse = m_MusicWheel.GetSelectedCourse();
	GAMESTATE->m_pCurCourse.Set( pCourse );
	if( pCourse )
		GAMESTATE->m_pPreferredCourse = pCourse;

	m_vpSteps.clear();
	m_vpTrails.clear();

	m_Banner.SetMovingFast( !!m_MusicWheel.IsMoving() );

	vector<RString> m_Artists, m_AltArtists;

	m_sSampleMusicToPlay = "";
	m_pSampleMusicTimingData = NULL;
	g_sCDTitlePath = "";
	g_sBannerPath = "";
	g_bWantFallbackCdTitle = false;
	bool bWantBanner = true;

	static SortOrder s_lastSortOrder = SortOrder_Invalid;
	if( GAMESTATE->m_SortOrder != s_lastSortOrder )
	{
		// Reload to let Lua metrics have a chance to change the help text.
		s_lastSortOrder = GAMESTATE->m_SortOrder;
	}

	switch( m_MusicWheel.GetSelectedType() )
	{
	case TYPE_SECTION:
	case TYPE_SORT:
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
		FOREACH_PlayerNumber( p )
			m_iSelection[p] = -1;

		g_sCDTitlePath = ""; // none

		m_fSampleStartSeconds = 0;
		m_fSampleLengthSeconds = -1;

		switch( m_MusicWheel.GetSelectedType() )
		{
		case TYPE_SECTION:
			g_sBannerPath = SONGMAN->GetSongGroupBannerPath( m_MusicWheel.GetSelectedSection() );
			m_sSampleMusicToPlay = m_sSectionMusicPath;
			break;
		case TYPE_SORT:
			bWantBanner = false; /* we load it ourself */
			m_Banner.LoadMode();
			m_sSampleMusicToPlay = m_sSortMusicPath;
			break;
		case TYPE_ROULETTE:
			bWantBanner = false; /* we load it ourself */
			m_Banner.LoadRoulette();
			m_sSampleMusicToPlay = m_sRouletteMusicPath;
			break;
		case TYPE_RANDOM:
			bWantBanner = false; /* we load it ourself */
			m_Banner.LoadRandom();
			m_sSampleMusicToPlay = m_sRandomMusicPath;
			break;
		default:
			ASSERT(0);
		}
		break;
	case TYPE_SONG:
	case TYPE_PORTAL:
		m_sSampleMusicToPlay = pSong->GetMusicPath();
		m_pSampleMusicTimingData = &pSong->m_Timing;
		m_fSampleStartSeconds = pSong->m_fMusicSampleStartSeconds;
		m_fSampleLengthSeconds = pSong->m_fMusicSampleLengthSeconds;

		SongUtil::GetPlayableSteps( pSong, m_vpSteps );

		if ( PREFSMAN->m_bShowBanners )
			g_sBannerPath = pSong->GetBannerPath();

		g_sCDTitlePath = pSong->GetCDTitlePath();
		g_bWantFallbackCdTitle = true;

		SwitchToPreferredDifficulty();
		break;

	case TYPE_COURSE:
	{
		const Course *pCourse = m_MusicWheel.GetSelectedCourse();
		const Style *pStyle = NULL;
		if( CommonMetrics::AUTO_SET_STYLE )
			pStyle = pCourse->GetCourseStyle( GAMESTATE->m_pCurGame, GAMESTATE->GetNumSidesJoined() );
		if( pStyle == NULL )
			pStyle = GAMESTATE->GetCurrentStyle();
		pCourse->GetTrails( m_vpTrails, pStyle->m_StepsType );

		m_sSampleMusicToPlay = m_sCourseMusicPath;
		m_fSampleStartSeconds = 0;
		m_fSampleLengthSeconds = -1;

		g_sBannerPath = pCourse->GetBannerPath();
		if( g_sBannerPath.empty() )
			m_Banner.LoadFallback();

		SwitchToPreferredDifficulty();
		break;
	}
	default:
		ASSERT(0);
	}

	m_sprCDTitleFront.UnloadTexture();
	m_sprCDTitleBack.UnloadTexture();

	/* Cancel any previous, incomplete requests for song assets, since we need new ones. */
	m_BackgroundLoader.Abort();

	g_bCDTitleWaiting = false;
	if( !g_sCDTitlePath.empty() || g_bWantFallbackCdTitle )
	{
		LOG->Trace( "cache \"%s\"", g_sCDTitlePath.c_str());
		m_BackgroundLoader.CacheFile( g_sCDTitlePath ); // empty OK
		g_bCDTitleWaiting = true;
	}

	g_bBannerWaiting = false;
	if( bWantBanner )
	{
		LOG->Trace("LoadFromCachedBanner(%s)",g_sBannerPath .c_str());
		if( m_Banner.LoadFromCachedBanner( g_sBannerPath ) )
		{
			/* If the high-res banner is already loaded, just
			 * delay before loading it, so the low-res one has
			 * time to fade in. */
			if( !TEXTUREMAN->IsTextureRegistered( Sprite::SongBannerTexture(g_sBannerPath) ) )
				m_BackgroundLoader.CacheFile( g_sBannerPath );

			g_bBannerWaiting = true;
		}
	}

	// Don't stop music if it's already playing the right file.
	g_bSampleMusicWaiting = false;
	if( !m_MusicWheel.IsRouletting() && SOUND->GetMusicPath() != m_sSampleMusicToPlay )
	{
		SOUND->StopMusic();
		if( !m_sSampleMusicToPlay.empty() )
			g_bSampleMusicWaiting = true;
	}

	g_StartedLoadingAt.Touch();

	vector<PlayerNumber> vpns;
	FOREACH_HumanPlayer( p )
		vpns.push_back( p );

	AfterStepsOrTrailChange( vpns );
}

// lua start
#include "LuaBinding.h"

class LunaScreenSelectMusic: public Luna<ScreenSelectMusic>
{
public:
	static int GetGoToOptions( T* p, lua_State *L ) { lua_pushboolean( L, p->GetGoToOptions() ); return 1; }

	LunaScreenSelectMusic()
	{
  		ADD_METHOD( GetGoToOptions );
	}
};

LUA_REGISTER_DERIVED_CLASS( ScreenSelectMusic, ScreenWithMenuElements )
// lua end

/*
 * (c) 2001-2004 Chris Danford
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
