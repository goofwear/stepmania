#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenSelectMusic

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


#include "ScreenSelectMusic.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "GameManager.h"
#include "RageSounds.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "InputMapper.h"
#include "InputQueue.h"
#include "AnnouncerManager.h"
#include "InputMapper.h"
#include "GameState.h"
#include "CodeDetector.h"
#include <math.h>
#include "GameConstantsAndTypes.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "ActorUtil.h"
#include "RageDisplay.h"
#include "RageTextureManager.h"
#include "Course.h"
#include "ProfileManager.h"
#include "MenuTimer.h"
#include "LightsManager.h"


const int NUM_SCORE_DIGITS	=	9;

#define FOV									THEME->GetMetricF("ScreenSelectMusic","FOV")
#define FOV_CENTER_X						THEME->GetMetricF("ScreenSelectMusic","FOVCenterX")
#define FOV_CENTER_Y						THEME->GetMetricF("ScreenSelectMusic","FOVCenterY")
#define BANNER_WIDTH						THEME->GetMetricF("ScreenSelectMusic","BannerWidth")
#define BANNER_HEIGHT						THEME->GetMetricF("ScreenSelectMusic","BannerHeight")
#define SONG_OPTIONS_EXTRA_COMMAND			THEME->GetMetric ("ScreenSelectMusic","SongOptionsExtraCommand")
#define SAMPLE_MUSIC_DELAY					THEME->GetMetricF("ScreenSelectMusic","SampleMusicDelay")
#define SHOW_RADAR							THEME->GetMetricB("ScreenSelectMusic","ShowRadar")
#define SHOW_GRAPH							THEME->GetMetricB("ScreenSelectMusic","ShowGraph")
#define SHOW_PANES							THEME->GetMetricB("ScreenSelectMusic","ShowPanes")
#define SHOW_DIFFICULTY_LIST				THEME->GetMetricB("ScreenSelectMusic","ShowDifficultyList")
#define CDTITLE_SPIN_SECONDS				THEME->GetMetricF("ScreenSelectMusic","CDTitleSpinSeconds")
#define PREV_SCREEN( play_mode )			THEME->GetMetric ("ScreenSelectMusic","PrevScreen"+Capitalize(PlayModeToString(play_mode)))
#define NEXT_SCREEN( play_mode )			THEME->GetMetric ("ScreenSelectMusic","NextScreen"+Capitalize(PlayModeToString(play_mode)))
#define NEXT_OPTIONS_SCREEN( play_mode )	THEME->GetMetric ("ScreenSelectMusic","NextOptionsScreen"+Capitalize(PlayModeToString(play_mode)))
#define SCORE_SORT_CHANGE_COMMAND(i) 		THEME->GetMetric ("ScreenSelectMusic",ssprintf("ScoreP%iSortChangeCommand", i+1))
#define SCORE_FRAME_SORT_CHANGE_COMMAND(i)	THEME->GetMetric ("ScreenSelectMusic",ssprintf("ScoreFrameP%iSortChangeCommand", i+1))

static const ScreenMessage	SM_AllowOptionsMenuRepeat	= ScreenMessage(SM_User+1);
CString g_sFallbackCDTitlePath;

/* We make a backface for the CDTitle by rotating it on Y and mirroring it
 * on Y by flipping texture coordinates. */
static void FlipSpriteHorizontally(Sprite &s)
{
	s.StopUsingCustomCoords();

	float Coords[8];
	s.GetActiveTextureCoords(Coords);
	swap(Coords[0], Coords[6]); /* top left X <-> top right X */
	swap(Coords[1], Coords[7]); /* top left Y <-> top right Y */
	swap(Coords[2], Coords[4]); /* bottom left X <-> bottom left X */
	swap(Coords[3], Coords[5]); /* bottom left Y <-> bottom left Y */
	s.SetCustomTextureCoords(Coords);
}

ScreenSelectMusic::ScreenSelectMusic( CString sClassName ) : Screen( sClassName )
{
	LOG->Trace( "ScreenSelectMusic::ScreenSelectMusic()" );

	LIGHTSMAN->SetLightMode( LIGHTMODE_MENU );

	m_bInCourseDisplayMode = GAMESTATE->IsCourseMode();

	/* Cache: */
	g_sFallbackCDTitlePath = THEME->GetPathToG("ScreenSelectMusic fallback cdtitle");

	if( GAMESTATE->m_CurStyle == STYLE_INVALID )
		RageException::Throw( "The Style has not been set.  A theme must set the Style before loading ScreenSelectMusic." );

	if( GAMESTATE->m_PlayMode == PLAY_MODE_INVALID )
		RageException::Throw( "The PlayMode has not been set.  A theme must set the PlayMode before loading ScreenSelectMusic." );

	CodeDetector::RefreshCacheItems();

	int p;

	m_Menu.Load( "ScreenSelectMusic" );
	this->AddChild( &m_Menu );

	m_MusicWheel.SetName( "Wheel" );
	SET_XY( m_MusicWheel );
	this->AddChild( &m_MusicWheel );

	m_sprBannerMask.SetName( "Banner" );	// use the same metrics and animation as Banner
	m_sprBannerMask.Load( THEME->GetPathToG("ScreenSelectMusic banner mask") );
	m_sprBannerMask.SetBlendMode( BLEND_NO_EFFECT );	// don't draw to color buffer
	m_sprBannerMask.SetUseZBuffer( true );	// do draw to the zbuffer
	SET_XY( m_sprBannerMask );
	this->AddChild( &m_sprBannerMask );

	// this is loaded SetSong and TweenToSong
	m_Banner.SetName( "Banner" );
	m_Banner.SetUseZBuffer( true );	// do have to pass the z test
	m_Banner.ScaleToClipped( BANNER_WIDTH, BANNER_HEIGHT );
	SET_XY( m_Banner );
	this->AddChild( &m_Banner );

	m_sprBannerFrame.Load( THEME->GetPathToG("ScreenSelectMusic banner frame") );
	m_sprBannerFrame->SetName( "BannerFrame" );
	SET_XY( m_sprBannerFrame );
	this->AddChild( m_sprBannerFrame );

	m_BPMDisplay.SetName( "BPMDisplay" );
	m_BPMDisplay.Load();
	SET_XY( m_BPMDisplay );
	this->AddChild( &m_BPMDisplay );

	m_DifficultyDisplay.SetName( "DifficultyDisplay" );
	m_DifficultyDisplay.EnableShadow( false );
	SET_XY( m_DifficultyDisplay );
	this->AddChild( &m_DifficultyDisplay );

	{
		CStringArray StageTexts;
		GAMESTATE->GetAllStageTexts( StageTexts );
		for( unsigned i = 0; i < StageTexts.size(); ++i )
		{
			CString path = THEME->GetPathToG( "ScreenSelectMusic stage "+StageTexts[i], true );
			if( path != "" )
				TEXTUREMAN->CacheTexture( path );
		}
	}

	m_sprStage.SetName( "Stage" );
	m_sprStage.Load( THEME->GetPathToG("ScreenSelectMusic stage "+GAMESTATE->GetStageText()) );
	SET_XY( m_sprStage );
	this->AddChild( &m_sprStage );

	m_sprCDTitleFront.SetName( "CDTitle" );
	m_sprCDTitleFront.Load( THEME->GetPathToG("ScreenSelectMusic fallback cdtitle") );
	m_sprCDTitleFront.SetUseBackfaceCull(true);
	m_sprCDTitleFront.SetDiffuse( RageColor(1,1,1,1) );
	m_sprCDTitleFront.SetEffectSpin( RageVector3(0, 360/CDTITLE_SPIN_SECONDS, 0) );
	SET_XY( m_sprCDTitleFront );
	this->AddChild( &m_sprCDTitleFront );

	m_sprCDTitleBack.SetName( "CDTitle" );
	m_sprCDTitleBack.Load( THEME->GetPathToG("ScreenSelectMusic fallback cdtitle") );
	FlipSpriteHorizontally(m_sprCDTitleBack);
	m_sprCDTitleBack.SetUseBackfaceCull(true);
	m_sprCDTitleBack.SetDiffuse( RageColor(0.2f,0.2f,0.2f,1) );
	m_sprCDTitleBack.SetRotationY( 180 );
	m_sprCDTitleBack.SetEffectSpin( RageVector3(0, 360/CDTITLE_SPIN_SECONDS, 0) );
	SET_XY( m_sprCDTitleBack );
	this->AddChild( &m_sprCDTitleBack );

	m_GrooveRadar.SetName( "Radar" );
	SET_XY( m_GrooveRadar );
	if( SHOW_RADAR )
		this->AddChild( &m_GrooveRadar );

	m_GrooveGraph.SetName( "Graph" );
	SET_XY( m_GrooveGraph );
	if( SHOW_GRAPH )
		this->AddChild( &m_GrooveGraph );

	m_textSongOptions.SetName( "SongOptions" );
	m_textSongOptions.LoadFromFont( THEME->GetPathToF("Common normal") );
	SET_XY( m_textSongOptions );
	this->AddChild( &m_textSongOptions );

	m_CourseContentsFrame.SetName( "CourseContents" );
	SET_XY( m_CourseContentsFrame );
	this->AddChild( &m_CourseContentsFrame );

	m_Artist.SetName( "ArtistDisplay" );
	m_Artist.Load();
	SET_XY( m_Artist );
	this->AddChild( &m_Artist );
		
	m_MachineRank.SetName( "MachineRank" );
	m_MachineRank.LoadFromFont( THEME->GetPathToF("ScreenSelectMusic rank") );
	SET_XY( m_MachineRank );
	this->AddChild( &m_MachineRank );

	if( SHOW_DIFFICULTY_LIST )
	{
		m_DifficultyList.SetName( "DifficultyList" );
		m_DifficultyList.Load();
		SET_XY_AND_ON_COMMAND( m_DifficultyList );
		this->AddChild( &m_DifficultyList );
	}

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsHumanPlayer(p) )
			continue;	// skip

		m_sprDifficultyFrame[p].SetName( ssprintf("DifficultyFrameP%d",p+1) );
		m_sprDifficultyFrame[p].Load( THEME->GetPathToG(ssprintf("ScreenSelectMusic difficulty frame p%d",p+1)) );
		m_sprDifficultyFrame[p].StopAnimating();
		SET_XY( m_sprDifficultyFrame[p] );
		this->AddChild( &m_sprDifficultyFrame[p] );

		m_DifficultyIcon[p].SetName( ssprintf("DifficultyIconP%d",p+1) );
		m_DifficultyIcon[p].Load( THEME->GetPathToG("ScreenSelectMusic difficulty icons 1x5") );
		SET_XY( m_DifficultyIcon[p] );
		this->AddChild( &m_DifficultyIcon[p] );

		m_AutoGenIcon[p].SetName( ssprintf("AutogenIconP%d",p+1) );
		m_AutoGenIcon[p].Load( THEME->GetPathToG("ScreenSelectMusic autogen") );
		SET_XY( m_AutoGenIcon[p] );
		this->AddChild( &m_AutoGenIcon[p] );

		m_OptionIconRow[p].SetName( ssprintf("OptionIconsP%d",p+1) );
		m_OptionIconRow[p].Refresh( (PlayerNumber)p );
		SET_XY( m_OptionIconRow[p] );
		this->AddChild( &m_OptionIconRow[p] );

		m_sprMeterFrame[p].SetName( ssprintf("MeterFrameP%d",p+1) );
		m_sprMeterFrame[p].Load( THEME->GetPathToG(ssprintf("ScreenSelectMusic meter frame p%d",p+1)) );
		SET_XY( m_sprMeterFrame[p] );
		this->AddChild( &m_sprMeterFrame[p] );

		if( SHOW_PANES )
		{
			m_PaneDisplay[p].SetName( "PaneDisplay", ssprintf("PaneDisplayP%d",p+1) );
			m_PaneDisplay[p].Load( (PlayerNumber) p );
			this->AddChild( &m_PaneDisplay[p] );
		}

		m_DifficultyMeter[p].SetName( "DifficultyMeter", ssprintf("MeterP%d",p+1) );
		m_DifficultyMeter[p].Load();
		SET_XY_AND_ON_COMMAND( m_DifficultyMeter[p] );
		this->AddChild( &m_DifficultyMeter[p] );

		m_sprHighScoreFrame[p].SetName( ssprintf("ScoreFrameP%d",p+1) );
		m_sprHighScoreFrame[p].Load( THEME->GetPathToG(ssprintf("ScreenSelectMusic score frame p%d",p+1)) );
		SET_XY( m_sprHighScoreFrame[p] );
		this->AddChild( &m_sprHighScoreFrame[p] );

		m_textHighScore[p].SetName( ssprintf("ScoreP%d",p+1) );
		m_textHighScore[p].LoadFromNumbers( THEME->GetPathToN("ScreenSelectMusic score") );
		m_textHighScore[p].EnableShadow( false );
		m_textHighScore[p].SetDiffuse( PlayerToColor(p) );
		SET_XY( m_textHighScore[p] );
		this->AddChild( &m_textHighScore[p] );
	}	

	m_MusicSortDisplay.SetName( "SortIcon" );
	m_MusicSortDisplay.Set( GAMESTATE->m_SongSortOrder );
	SET_XY( m_MusicSortDisplay );
	this->AddChild( &m_MusicSortDisplay );

	m_sprBalloon.SetName( "Balloon" );
	TEXTUREMAN->CacheTexture( THEME->GetPathToG("ScreenSelectMusic balloon long") );
	TEXTUREMAN->CacheTexture( THEME->GetPathToG("ScreenSelectMusic balloon marathon") );
	this->AddChild( &m_sprBalloon );

	m_sprOptionsMessage.SetName( "OptionsMessage" );
	m_sprOptionsMessage.Load( THEME->GetPathToG("ScreenSelectMusic options message 1x2") );
	m_sprOptionsMessage.StopAnimating();
	m_sprOptionsMessage.SetDiffuse( RageColor(1,1,1,0) );	// invisible
	//this->AddChild( &m_sprOptionsMessage );	// we have to draw this manually over the top of transitions


	m_soundSelect.Load( THEME->GetPathToS("Common start") );
	m_soundChangeNotes.Load( THEME->GetPathToS("ScreenSelectMusic difficulty") );
	m_soundOptionsChange.Load( THEME->GetPathToS("ScreenSelectMusic options") );
	m_soundLocked.Load( THEME->GetPathToS("ScreenSelectMusic locked") );

	SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("select music intro") );

	m_bMadeChoice = false;
	m_bGoToOptions = false;
	m_fPlaySampleCountdown = 0;
	m_bAllowOptionsMenu = m_bAllowOptionsMenuRepeat = false;

	UpdateOptionsDisplays();

	AfterMusicChange();
	TweenOnScreen();
}


ScreenSelectMusic::~ScreenSelectMusic()
{
	LOG->Trace( "ScreenSelectMusic::~ScreenSelectMusic()" );

}

void ScreenSelectMusic::DrawPrimitives()
{
	DISPLAY->CameraPushMatrix();
	DISPLAY->LoadMenuPerspective( FOV, FOV_CENTER_X, FOV_CENTER_Y );

	m_Menu.DrawBottomLayer();
	Screen::DrawPrimitives();
	m_Menu.DrawTopLayer();
	m_sprOptionsMessage.Draw();
	
	DISPLAY->CameraPopMatrix();
}

void ScreenSelectMusic::TweenSongPartsOnScreen( bool Initial )
{
	m_GrooveRadar.StopTweening();
	m_GrooveGraph.StopTweening();
	m_GrooveRadar.TweenOnScreen();
	m_GrooveGraph.TweenOnScreen();
	if( SHOW_DIFFICULTY_LIST )
	{
		if( Initial )
		{
			ON_COMMAND( m_DifficultyList );
			m_DifficultyList.TweenOnScreen();
		}
		else
			m_DifficultyList.Show();
	}

	for( int p=0; p<NUM_PLAYERS; p++ )
	{		
		if( !GAMESTATE->IsHumanPlayer(p) )
			continue;	// skip

		ON_COMMAND( m_sprDifficultyFrame[p] );
		ON_COMMAND( m_sprMeterFrame[p] );
		ON_COMMAND( m_DifficultyIcon[p] );
		ON_COMMAND( m_AutoGenIcon[p] );
		ON_COMMAND( m_DifficultyMeter[p] );
	}
}

void ScreenSelectMusic::TweenSongPartsOffScreen( bool Final )
{
	m_GrooveRadar.TweenOffScreen();
	m_GrooveGraph.TweenOffScreen();
	if( SHOW_DIFFICULTY_LIST )
	{
		if( Final )
		{
			OFF_COMMAND( m_DifficultyList );
			m_DifficultyList.TweenOffScreen();
		}
		else
			m_DifficultyList.Hide();
	}

	for( int p=0; p<NUM_PLAYERS; p++ )
	{		
		if( !GAMESTATE->IsHumanPlayer(p) )
			continue;	// skip

		OFF_COMMAND( m_sprDifficultyFrame[p] );
		OFF_COMMAND( m_sprMeterFrame[p] );
		OFF_COMMAND( m_DifficultyIcon[p] );
		OFF_COMMAND( m_AutoGenIcon[p] );
		OFF_COMMAND( m_DifficultyMeter[p] );
	}
}

void ScreenSelectMusic::TweenCoursePartsOnScreen( bool Initial )
{
	m_CourseContentsFrame.SetZoomY( 1 );
	if( Initial )
	{
		m_CourseContentsFrame.FadeOn( 0, "foldy", 0.3f );
		COMMAND( m_CourseContentsFrame, "On" );
	}
	else
	{
		m_CourseContentsFrame.SetFromCourse(NULL);
		COMMAND( m_CourseContentsFrame, "Show" );
	}
	// XXX: if !Initial, m_CourseContentsFrame.Hide?
}

void ScreenSelectMusic::TweenCoursePartsOffScreen( bool Final )
{
	if( Final )
	{
		m_CourseContentsFrame.SetZoomY( 1 );
		m_CourseContentsFrame.FadeOff( 0, "foldy", 0.3f );
		OFF_COMMAND( m_CourseContentsFrame );
	}
	else
	{
		COMMAND( m_CourseContentsFrame, "Hide" );
	}
}

void ScreenSelectMusic::SkipSongPartTweens()
{
	m_GrooveRadar.FinishTweening();
	m_GrooveGraph.FinishTweening();
	if( SHOW_DIFFICULTY_LIST )
		m_DifficultyList.FinishTweening();

	for( int p=0; p<NUM_PLAYERS; p++ )
	{		
		if( !GAMESTATE->IsHumanPlayer(p) )
			continue;	// skip

		m_sprDifficultyFrame[p].FinishTweening();
		m_sprMeterFrame[p].FinishTweening();
		m_DifficultyIcon[p].FinishTweening();
		m_AutoGenIcon[p].FinishTweening();
		m_DifficultyMeter[p].FinishTweening();
	}
}

void ScreenSelectMusic::SkipCoursePartTweens()
{
	m_CourseContentsFrame.FinishTweening();
}

void ScreenSelectMusic::TweenOnScreen()
{
	TweenSongPartsOnScreen( true );
	TweenCoursePartsOnScreen( true );

	switch( GAMESTATE->m_SongSortOrder )
	{
	case SORT_ALL_COURSES:
	case SORT_NONSTOP_COURSES:
	case SORT_ONI_COURSES:
	case SORT_ENDLESS_COURSES:
		TweenSongPartsOffScreen( false );
		SkipSongPartTweens();
		break;
	default:
		TweenCoursePartsOffScreen( false );
		SkipCoursePartTweens();
		break;
	}

	ON_COMMAND( m_sprBannerMask );
	ON_COMMAND( m_Banner );
	ON_COMMAND( m_sprBannerFrame );
	ON_COMMAND( m_BPMDisplay );
	ON_COMMAND( m_DifficultyDisplay );
	ON_COMMAND( m_sprStage );
	ON_COMMAND( m_sprCDTitleFront );
	ON_COMMAND( m_sprCDTitleBack );
	ON_COMMAND( m_GrooveRadar );
	ON_COMMAND( m_GrooveGraph );
	ON_COMMAND( m_textSongOptions );
	ON_COMMAND( m_MusicSortDisplay );
	m_MusicWheel.TweenOnScreen();
	ON_COMMAND( m_MusicWheel );
	ON_COMMAND( m_Artist );
	ON_COMMAND( m_MachineRank );

	for( int p=0; p<NUM_PLAYERS; p++ )
	{		
		if( !GAMESTATE->IsHumanPlayer(p) )
			continue;	// skip

		ON_COMMAND( m_OptionIconRow[p] );
		ON_COMMAND( m_sprHighScoreFrame[p] );
		ON_COMMAND( m_textHighScore[p] );
		if( SHOW_PANES )
			ON_COMMAND( m_PaneDisplay[p] );
	}

	if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
		m_textSongOptions.Command( SONG_OPTIONS_EXTRA_COMMAND );
}

void ScreenSelectMusic::TweenOffScreen()
{
	switch( GAMESTATE->m_SongSortOrder )
	{
	case SORT_ALL_COURSES:
	case SORT_NONSTOP_COURSES:
	case SORT_ONI_COURSES:
	case SORT_ENDLESS_COURSES:
		TweenCoursePartsOffScreen( true );
		break;
	default:
		TweenSongPartsOffScreen( true );
		break;
	}

	OFF_COMMAND( m_sprBannerMask );
	OFF_COMMAND( m_Banner );
	OFF_COMMAND( m_sprBannerFrame );
	OFF_COMMAND( m_BPMDisplay );
	OFF_COMMAND( m_DifficultyDisplay );
	OFF_COMMAND( m_sprStage );
	OFF_COMMAND( m_sprCDTitleFront );
	OFF_COMMAND( m_sprCDTitleBack );
	OFF_COMMAND( m_GrooveRadar );
	OFF_COMMAND( m_GrooveGraph );
	OFF_COMMAND( m_textSongOptions );
	OFF_COMMAND( m_MusicSortDisplay );
	m_MusicWheel.TweenOffScreen();
	OFF_COMMAND( m_MusicWheel );
	OFF_COMMAND( m_sprBalloon );
	OFF_COMMAND( m_Artist );
	OFF_COMMAND( m_MachineRank );

	for( int p=0; p<NUM_PLAYERS; p++ )
	{		
		if( !GAMESTATE->IsHumanPlayer(p) )
			continue;	// skip

		OFF_COMMAND( m_OptionIconRow[p] );
		OFF_COMMAND( m_sprHighScoreFrame[p] );
		OFF_COMMAND( m_textHighScore[p] );
		if( SHOW_PANES )
			OFF_COMMAND( m_PaneDisplay[p] );
	}
}


/* This hides elements that are only relevant when displaying a single song,
 * and shows elements for course display.  XXX: Allow different tween commands. */
void ScreenSelectMusic::EnterCourseDisplayMode()
{
	if( m_bInCourseDisplayMode )
		return;
	m_bInCourseDisplayMode = true;

	TweenSongPartsOffScreen( false );
	TweenCoursePartsOnScreen( false );
}

void ScreenSelectMusic::ExitCourseDisplayMode()
{
	if( !m_bInCourseDisplayMode )
		return;
	m_bInCourseDisplayMode = false;

	TweenSongPartsOnScreen( false );
	TweenCoursePartsOffScreen( false );
}

void ScreenSelectMusic::TweenScoreOnAndOffAfterChangeSort()
{
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsHumanPlayer(p) )
			continue;	// skip
		m_textHighScore[p].Command( SCORE_SORT_CHANGE_COMMAND(p) );
		m_sprHighScoreFrame[p].Command( SCORE_FRAME_SORT_CHANGE_COMMAND(p) );
	}

	switch( GAMESTATE->m_SongSortOrder )
	{
	case SORT_ALL_COURSES:
	case SORT_NONSTOP_COURSES:
	case SORT_ONI_COURSES:
	case SORT_ENDLESS_COURSES:
		EnterCourseDisplayMode();
		break;
	default:
		if( GAMESTATE->m_SongSortOrder != SORT_MENU )
			ExitCourseDisplayMode();
		break;
	}
}

void ScreenSelectMusic::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );
	m_sprOptionsMessage.Update( fDeltaTime );


	if( m_fPlaySampleCountdown > 0 )
	{
		m_fPlaySampleCountdown -= fDeltaTime;
		/* Make sure we don't start the sample when rouletting is
		 * spinning down. */
		if( m_fPlaySampleCountdown <= 0 && !m_MusicWheel.IsRouletting() )
		{
			if( !m_sSampleMusicToPlay.empty() )
			{
				SOUND->PlayMusic(
					m_sSampleMusicToPlay, 
					true,
					m_fSampleStartSeconds,
					m_fSampleLengthSeconds,
					1.5f); /* fade out for 1.5 seconds */
			}
		}
	}
}

void ScreenSelectMusic::Input( const DeviceInput& DeviceI, InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
//	LOG->Trace( "ScreenSelectMusic::Input()" );
	
	if( DeviceI.device == DEVICE_KEYBOARD && DeviceI.button == SDLK_F9 )
	{
		if( type != IET_FIRST_PRESS ) return;
		PREFSMAN->m_bShowNative ^= 1;
		m_MusicWheel.RebuildMusicWheelItems();
		Course* pCourse = m_MusicWheel.GetSelectedCourse();
		if(pCourse)
			m_CourseContentsFrame.SetFromCourse( pCourse );
		return;
	}

	if( MenuI.button == MENU_BUTTON_RIGHT || MenuI.button == MENU_BUTTON_LEFT )
	{
		if( !MenuI.IsValid() ) return;
		if( !GAMESTATE->IsHumanPlayer(MenuI.player) ) return;

		/* If we're rouletting, hands off. */
		if(m_MusicWheel.IsRouletting())
			return;

		int dir = 0;
		if(INPUTMAPPER->IsButtonDown( MenuInput(MenuI.player, MENU_BUTTON_RIGHT) ) )
			dir++;
		if(INPUTMAPPER->IsButtonDown( MenuInput(MenuI.player, MENU_BUTTON_LEFT) ) )
			dir--;
		
		m_MusicWheel.Move(dir);
		return;
	}

	if( type == IET_RELEASE )	return;		// don't care

	if( !GameI.IsValid() )		return;		// don't care

	if( m_bMadeChoice  &&  MenuI.IsValid()  &&  MenuI.button == MENU_BUTTON_START  &&  !GAMESTATE->IsExtraStage()  &&  !GAMESTATE->IsExtraStage2() )
	{
		if(m_bGoToOptions) return; /* got it already */
		if(!m_bAllowOptionsMenu) return; /* not allowed */

		if( !m_bAllowOptionsMenuRepeat &&
			(type == IET_SLOW_REPEAT || type == IET_FAST_REPEAT ))
			return; /* not allowed yet */
		
		m_bGoToOptions = true;
		m_sprOptionsMessage.SetState( 1 );
		SOUND->PlayOnce( THEME->GetPathToS("Common start") );
		return;
	}
	
	if( m_Menu.IsTransitioning() )	return;		// ignore

	if( m_bMadeChoice )
		return;

	PlayerNumber pn = GAMESTATE->GetCurrentStyleDef()->ControllerToPlayerNumber( GameI.controller );

	if( CodeDetector::EnteredEasierDifficulty(GameI.controller) )
	{
		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			m_soundLocked.Play();
		else
			EasierDifficulty( pn );
		return;
	}
	if( CodeDetector::EnteredHarderDifficulty(GameI.controller) )
	{
		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			m_soundLocked.Play();
		else
			HarderDifficulty( pn );
		return;
	}
	if( CodeDetector::EnteredNextSort(GameI.controller) )
	{
		if( ( GAMESTATE->IsExtraStage() && !PREFSMAN->m_bPickExtraStage ) || GAMESTATE->IsExtraStage2() )
			m_soundLocked.Play();
		else
			m_MusicWheel.ChangeSort( SORT_MENU );
		return;
	}
	if( !GAMESTATE->IsExtraStage() && !GAMESTATE->IsExtraStage2() && CodeDetector::DetectAndAdjustMusicOptions(GameI.controller) )
	{
		m_soundOptionsChange.Play();
		UpdateOptionsDisplays();
		return;
	}

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default input handler
}


void ScreenSelectMusic::EasierDifficulty( PlayerNumber pn )
{
	LOG->Trace( "ScreenSelectMusic::EasierDifficulty( %d )", pn );

	if( !GAMESTATE->IsHumanPlayer(pn) )
		return;

	if( m_MusicWheel.GetSelectedType() == TYPE_COURSE && GAMESTATE->m_bDifficultCourses )
	{
		m_soundChangeNotes.Play();
		GAMESTATE->m_bDifficultCourses = false;
		AfterMusicChange();
		return;
	}

	if( m_MusicWheel.GetSelectedType() != TYPE_SONG )
		return;

	if( m_MusicWheel.GetSelectedType() != TYPE_SONG )
		return;
	if( m_iSelection[pn] == 0 )
		return;

	m_iSelection[pn]--;
	// the user explicity switched difficulties.  Update the preferred difficulty
	GAMESTATE->m_PreferredDifficulty[pn] = m_arrayNotes[ m_iSelection[pn] ]->GetDifficulty();

	m_soundChangeNotes.Play();

	AfterNotesChange( pn );
}

void ScreenSelectMusic::HarderDifficulty( PlayerNumber pn )
{
	LOG->Trace( "ScreenSelectMusic::HarderDifficulty( %d )", pn );

	if( !GAMESTATE->IsHumanPlayer(pn) )
		return;

	if( m_MusicWheel.GetSelectedType() == TYPE_COURSE && !GAMESTATE->m_bDifficultCourses )
	{
		m_soundChangeNotes.Play();
		GAMESTATE->m_bDifficultCourses = true;
		AfterMusicChange();
		return;
	}

	if( m_MusicWheel.GetSelectedType() != TYPE_SONG )
		return;

	if( m_iSelection[pn] == int(m_arrayNotes.size()-1) )
		return;

	m_iSelection[pn]++;
	// the user explicity switched difficulties.  Update the preferred difficulty
	GAMESTATE->m_PreferredDifficulty[pn] = m_arrayNotes[ m_iSelection[pn] ]->GetDifficulty();

	m_soundChangeNotes.Play();

	AfterNotesChange( pn );
}


void ScreenSelectMusic::HandleScreenMessage( const ScreenMessage SM )
{
	Screen::HandleScreenMessage( SM );

	switch( SM )
	{
	case SM_AllowOptionsMenuRepeat:
		m_bAllowOptionsMenuRepeat = true;
		break;
	case SM_MenuTimer:
		if( m_MusicWheel.IsRouletting() )
		{
			MenuStart(PLAYER_INVALID);
			m_Menu.m_MenuTimer->SetSeconds( 15 );
		}
		else if( m_MusicWheel.GetSelectedType() != TYPE_SONG )
		{
			m_MusicWheel.StartRoulette();
			m_Menu.m_MenuTimer->SetSeconds( 15 );
		}
		else
		{
			MenuStart(PLAYER_INVALID);
		}
		break;
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( PREV_SCREEN(GAMESTATE->m_PlayMode) );
		/* We may have stray SM_SongChanged messages from the music wheel.  We can't
		 * handle them anymore, since the title menu (and attract screens) reset
		 * the game state, so just discard them. */
		ClearMessageQueue();
		break;
	case SM_GoToNextScreen:
		if( m_bGoToOptions )
		{
			SCREENMAN->SetNewScreen( NEXT_OPTIONS_SCREEN(GAMESTATE->m_PlayMode) );
		}
		else
		{
			GAMESTATE->AdjustFailType();
			SOUND->StopMusic();
			SCREENMAN->SetNewScreen( NEXT_SCREEN(GAMESTATE->m_PlayMode) );
		}
		break;
	case SM_SongChanged:
		AfterMusicChange();
		break;
	case SM_SortOrderChanged:
		SortOrderChanged();
		break;
	case SM_SortOrderChanging:
//				m_MusicSortDisplay.FadeOff( 0, "fade", TWEEN_TIME );
		TweenScoreOnAndOffAfterChangeSort();
		break;
	}
}

void ScreenSelectMusic::MenuStart( PlayerNumber pn )
{
	if( pn != PLAYER_INVALID  &&
		INPUTMAPPER->IsButtonDown( MenuInput(pn, MENU_BUTTON_LEFT) )  &&
		INPUTMAPPER->IsButtonDown( MenuInput(pn, MENU_BUTTON_RIGHT) ) )
	{
//		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
//			m_soundLocked.Play();
//		else
		{
			m_MusicWheel.NextSort();
		}
		return;
	}


	// this needs to check whether valid Steps are selected!
	bool bResult = m_MusicWheel.Select();

	/* If false, we don't have a selection just yet. */
	if( !bResult )
		return;

	// a song was selected
	switch( m_MusicWheel.GetSelectedType() )
	{
	case TYPE_SONG: {
		const bool bIsNew = m_MusicWheel.GetSelectedSong()->IsNew();
		bool bIsHard = false;
		for( int p=0; p<NUM_PLAYERS; p++ )
		{
			if( !GAMESTATE->IsHumanPlayer( (PlayerNumber)p ) )
				continue;	// skip
			if( GAMESTATE->m_pCurNotes[p]  &&  GAMESTATE->m_pCurNotes[p]->GetMeter() >= 10 )
				bIsHard = true;
		}

		/* See if this song is a repeat.  If we're in event mode, only check the last five songs. */
		bool bIsRepeat = false;
		int i = 0;
		if( PREFSMAN->m_bEventMode )
			i = max( 0, int(GAMESTATE->m_vPlayedStageStats.size())-5 );
		for( ; i < (int)GAMESTATE->m_vPlayedStageStats.size(); ++i )
			if( GAMESTATE->m_vPlayedStageStats[i].pSong == m_MusicWheel.GetSelectedSong() )
				bIsRepeat = true;

		/* Don't complain about repeats if the user didn't get to pick. */
		if( GAMESTATE->IsExtraStage() && !PREFSMAN->m_bPickExtraStage )
			bIsRepeat = false;

		if( bIsRepeat )
			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("select music comment repeat") );
		else if( bIsNew )
			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("select music comment new") );
		else if( bIsHard )
			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("select music comment hard") );
		else
			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("select music comment general") );

		m_bMadeChoice = true;

		/* If we're in event mode, we may have just played a course (putting us
		 * in course mode).  Make sure we're in a single song mode. */
		if( GAMESTATE->IsCourseMode() )
			GAMESTATE->m_PlayMode = PLAY_MODE_ARCADE;

		break;
	}
	case TYPE_COURSE:
	{
		SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("select course comment general") );

		Course *pCourse = m_MusicWheel.GetSelectedCourse();
		ASSERT( pCourse );
		GAMESTATE->m_PlayMode = pCourse->GetPlayMode();

		// apply #LIVES
		if( pCourse->m_iLives != -1 )
		{
			GAMESTATE->m_SongOptions.m_LifeType = SongOptions::LIFE_BATTERY;
			GAMESTATE->m_SongOptions.m_iBatteryLives = pCourse->m_iLives;
		}

		m_bMadeChoice = true;

		break;
	}
	case TYPE_SECTION:
	case TYPE_ROULETTE:
	case TYPE_SORT:
		break;
	default:
		ASSERT(0);
	}

	if( m_bMadeChoice )
	{
		TweenOffScreen();
		m_soundSelect.Play();

		if( !GAMESTATE->IsExtraStage()  &&  !GAMESTATE->IsExtraStage2() )
		{
//			float fShowSeconds = m_Menu.m_Out.GetLengthSeconds();

			// show "hold START for options"
			m_sprOptionsMessage.SetDiffuse( RageColor(1,1,1,1) );	// visible
			SET_XY_AND_ON_COMMAND( m_sprOptionsMessage );

			m_bAllowOptionsMenu = true;
			/* Don't accept a held START for a little while, so it's not
			 * hit accidentally.  Accept an initial START right away, though,
			 * so we don't ignore deliberate fast presses (which would be
			 * annoying). */
			this->PostScreenMessage( SM_AllowOptionsMenuRepeat, 0.5f );
		}

		m_Menu.StartTransitioning( SM_GoToNextScreen );
	}

	if( GAMESTATE->IsExtraStage() && PREFSMAN->m_bPickExtraStage )
	{
		/* Check if user selected the real extra stage. */
		Song* pSong;
		Steps* pNotes;
		PlayerOptions po;
		SongOptions so;
		SONGMAN->GetExtraStageInfo( false, GAMESTATE->GetCurrentStyleDef(), pSong, pNotes, po, so );
		ASSERT(pSong);
		
		/* Enable 2nd extra stage if user chose the correct song */
		if( m_MusicWheel.GetSelectedSong() == pSong )
			GAMESTATE->m_bAllow2ndExtraStage = true;
		else
			GAMESTATE->m_bAllow2ndExtraStage = false;
	}
}


void ScreenSelectMusic::MenuBack( PlayerNumber pn )
{
	SOUND->StopMusic();

	m_Menu.Back( SM_GoToPrevScreen );
}

void ScreenSelectMusic::AfterNotesChange( PlayerNumber pn )
{
	if( !GAMESTATE->IsHumanPlayer(pn) )
		return;
	
	m_iSelection[pn] = clamp( m_iSelection[pn], 0, int(m_arrayNotes.size()-1) );	// bounds clamping

	Steps* pNotes = m_arrayNotes.empty()? NULL: m_arrayNotes[m_iSelection[pn]];

	GAMESTATE->m_pCurNotes[pn] = pNotes;

//	m_BPMDisplay.SetZoomY( 0 );
//	m_BPMDisplay.BeginTweening( 0.2f );
//	m_BPMDisplay.SetZoomY( 1.2f );

	if( pNotes )
	{
		int iScore = 0;
		if( PROFILEMAN->IsUsingProfile(pn) )
		{
			if( !pNotes->m_MemCardDatas[pn].vHighScores.empty() )
				iScore = pNotes->m_MemCardDatas[pn].vHighScores[0].iScore;
		}
		else
		{
			if( !pNotes->m_MemCardDatas[MEMORY_CARD_MACHINE].vHighScores.empty() )
				iScore = pNotes->m_MemCardDatas[MEMORY_CARD_MACHINE].vHighScores[0].iScore;
		}
		m_textHighScore[pn].SetText( ssprintf("%*i", NUM_SCORE_DIGITS, iScore) );
	}
	else
	{
		m_textHighScore[pn].SetText( ssprintf("%*i", NUM_SCORE_DIGITS, 0) );
	}

	m_DifficultyIcon[pn].SetFromNotes( pn, pNotes );
	if( pNotes && pNotes->IsAutogen() )
	{
		m_AutoGenIcon[pn].SetEffectDiffuseShift();
	}
	else
	{
		m_AutoGenIcon[pn].SetEffectNone();
		m_AutoGenIcon[pn].SetDiffuse( RageColor(1,1,1,0) );
	}
	m_DifficultyMeter[pn].SetFromGameState( pn );
	if( SHOW_DIFFICULTY_LIST )
		m_DifficultyList.SetFromGameState();
	m_GrooveRadar.SetFromNotes( pn, pNotes );
	m_MusicWheel.NotesChanged( pn );
	if( SHOW_PANES )
		m_PaneDisplay[pn].SetFromGameState();
}

void ScreenSelectMusic::SwitchToPreferredDifficulty()
{
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsHumanPlayer( PlayerNumber(p) ) )
			continue;

		/* Find the closest match to the user's preferred difficulty. */
		int CurDifference = -1;
		for( unsigned i=0; i<m_arrayNotes.size(); i++ )
		{
			int Diff = abs(m_arrayNotes[i]->GetDifficulty() - GAMESTATE->m_PreferredDifficulty[p]);

			if( CurDifference == -1 || Diff < CurDifference )
			{
				m_iSelection[p] = i;
				CurDifference = Diff;
			}
		}

		m_iSelection[p] = clamp( m_iSelection[p], 0, int(m_arrayNotes.size()) ) ;
	}
}

void ScreenSelectMusic::AfterMusicChange()
{
	if( !m_MusicWheel.IsRouletting() )
		m_Menu.m_MenuTimer->Stall();

	Song* pSong = m_MusicWheel.GetSelectedSong();
	if( pSong )
		GAMESTATE->m_pCurSong = pSong;

	m_GrooveGraph.SetFromSong( pSong );

	Course* pCourse = m_MusicWheel.GetSelectedCourse();
	if( pCourse )
		GAMESTATE->m_pCurCourse = pCourse;

	int pn;
	for( pn = 0; pn < NUM_PLAYERS; ++pn)
		m_arrayNotes.clear();

	m_Banner.SetMovingFast( !!m_MusicWheel.IsMoving() );

	CString SampleMusicToPlay; 
	vector<CString> m_Artists, m_AltArtists;

	m_MachineRank.SetText( "" );
	switch( m_MusicWheel.GetSelectedType() )
	{
	case TYPE_SECTION:
	case TYPE_SORT:
		{	
			CString sGroup = m_MusicWheel.GetSelectedSection();
			for( int p=0; p<NUM_PLAYERS; p++ )
				m_iSelection[p] = -1;

			m_Banner.LoadFromGroup( sGroup );	// if this isn't a group, it'll default to the fallback banner
			m_BPMDisplay.NoBPM();
			m_sprCDTitleFront.UnloadTexture();
			m_sprCDTitleBack.UnloadTexture();
			m_DifficultyDisplay.UnsetDifficulties();

			m_fSampleStartSeconds = m_fSampleLengthSeconds = -1;
			switch( m_MusicWheel.GetSelectedType() )
			{
			case TYPE_SECTION:
				SampleMusicToPlay = THEME->GetPathToS("ScreenSelectMusic section music");
				break;
			case TYPE_SORT:
				SampleMusicToPlay = THEME->GetPathToS("ScreenSelectMusic sort music");
				break;
			default:
				ASSERT(0);
			}

			m_sprBalloon.StopTweening();
			OFF_COMMAND( m_sprBalloon );
		}
		break;
	case TYPE_SONG:
		{
			SampleMusicToPlay = pSong->GetMusicPath();
			m_fSampleStartSeconds = pSong->m_fMusicSampleStartSeconds;
			m_fSampleLengthSeconds = pSong->m_fMusicSampleLengthSeconds;

			pSong->GetSteps( m_arrayNotes, GAMESTATE->GetCurrentStyleDef()->m_StepsType );
			SortNotesArrayByDifficulty( m_arrayNotes );

			if ( PREFSMAN->m_bShowBanners )
				m_Banner.LoadFromSong( pSong );
			else
				m_Banner.LoadFallback() ;

			if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			{
				m_BPMDisplay.CycleRandomly();				
			}
			else
			{
				m_BPMDisplay.SetBPM( pSong );
			}

			const CString CDTitlePath = pSong->HasCDTitle()? pSong->GetCDTitlePath():g_sFallbackCDTitlePath;
			TEXTUREMAN->DisableOddDimensionWarning();
			m_sprCDTitleFront.Load( CDTitlePath );
			m_sprCDTitleBack.Load( CDTitlePath );
			TEXTUREMAN->EnableOddDimensionWarning();
			FlipSpriteHorizontally(m_sprCDTitleBack);

			const vector<Song*> best = SONGMAN->GetBestSongs( MEMORY_CARD_MACHINE );
			const int index = FindIndex( best.begin(), best.end(), pSong );
			if( index != -1 )
				m_MachineRank.SetText( ssprintf("%i", index+1) );

			m_DifficultyDisplay.SetDifficulties( pSong, GAMESTATE->GetCurrentStyleDef()->m_StepsType );

			SwitchToPreferredDifficulty();

			/* Short delay before actually showing these, so they don't show
			 * up when scrolling fast.  It'll still show up in "slow" scrolling,
			 * but it doesn't look at weird as it does in "fast", and I don't
			 * like the effect with a lot of delay. */
			if( pSong->m_fMusicLengthSeconds > PREFSMAN->m_fMarathonVerSongSeconds )
			{
				m_sprBalloon.StopTweening();
				m_sprBalloon.Load( THEME->GetPathToG("ScreenSelectMusic balloon marathon") );
				SET_XY_AND_ON_COMMAND( m_sprBalloon );
			}
			else if( pSong->m_fMusicLengthSeconds > PREFSMAN->m_fLongVerSongSeconds )
			{
				m_sprBalloon.StopTweening();
				m_sprBalloon.Load( THEME->GetPathToG("ScreenSelectMusic balloon long") );
				SET_XY_AND_ON_COMMAND( m_sprBalloon );
			}
			else
			{
				m_sprBalloon.StopTweening();
				OFF_COMMAND( m_sprBalloon );
			}

			// update stage counter display (long versions/marathons)
			m_sprStage.Load( THEME->GetPathToG("ScreenSelectMusic stage "+GAMESTATE->GetStageText()) );

			m_Artists.push_back( pSong->GetDisplayArtist() );
			m_AltArtists.push_back( pSong->GetTranslitArtist() );
		}
		break;
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
		switch(m_MusicWheel.GetSelectedType())
		{
		case TYPE_ROULETTE:	m_Banner.LoadRoulette();	break;
		case TYPE_RANDOM: 	m_Banner.LoadRandom();		break;
		default: ASSERT(0);
		}

		m_BPMDisplay.NoBPM();
		m_sprCDTitleFront.UnloadTexture();
		m_sprCDTitleBack.UnloadTexture();
		m_DifficultyDisplay.UnsetDifficulties();

		m_fSampleStartSeconds = m_fSampleLengthSeconds = -1;
		switch( m_MusicWheel.GetSelectedType() )
		{
		case TYPE_ROULETTE:
			SampleMusicToPlay = THEME->GetPathToS("ScreenSelectMusic roulette music");
			break;
		case TYPE_RANDOM:
			SampleMusicToPlay = THEME->GetPathToS("ScreenSelectMusic random music");
			break;
		default:
			ASSERT(0);
		}

		m_sprBalloon.StopTweening();
		OFF_COMMAND( m_sprBalloon );
		break;
	case TYPE_COURSE:
	{
		Course* pCourse = m_MusicWheel.GetSelectedCourse();

		SampleMusicToPlay = THEME->GetPathToS("ScreenSelectMusic course music");
//		m_textNumSongs.SetText( ssprintf("%d", pCourse->GetEstimatedNumStages()) );
//		float fTotalSeconds;
//		if( pCourse->GetTotalSeconds(fTotalSeconds) )
//			m_textTime.SetText( SecondsToTime(fTotalSeconds) );
//		else
//			m_textTime.SetText( "xx:xx:xx" );	// The numbers format doesn't have a '?'.  Is there a better solution?

		m_Banner.LoadFromCourse( pCourse );
		m_BPMDisplay.SetBPM( pCourse );

		m_CourseContentsFrame.SetFromCourse( pCourse );
		m_CourseContentsFrame.TweenInAfterChangedCourse();
		m_DifficultyDisplay.UnsetDifficulties();

		vector<Course::Info> ci;
		pCourse->GetCourseInfo( GAMESTATE->GetCurrentStyleDef()->m_StepsType, ci );

		for( unsigned i = 0; i < ci.size(); ++i )
		{
			if( ci[i].Mystery )
			{
				m_Artists.push_back( "???" );
				m_AltArtists.push_back( "???" );
			} else {
				m_Artists.push_back( ci[i].pSong->GetDisplayArtist() );
				m_AltArtists.push_back( ci[i].pSong->GetTranslitArtist() );
			}
		}

		const vector<Course*> best = SONGMAN->GetBestCourses( MEMORY_CARD_MACHINE );
		const int index = FindIndex( best.begin(), best.end(), pCourse );
		if( index != -1 )
			m_MachineRank.SetText( ssprintf("%i", index+1) );

		break;
	}
	default:
		ASSERT(0);
	}

	// Don't stop music if it's already playing the right file.
	if( SampleMusicToPlay == "" )
		SOUND->StopMusic();
	else if( SOUND->GetMusicPath() != SampleMusicToPlay )
	{
		SOUND->StopMusic();
		m_sSampleMusicToPlay = SampleMusicToPlay;
		m_fPlaySampleCountdown = SAMPLE_MUSIC_DELAY;
	}

	m_Artist.SetTips( m_Artists, m_AltArtists );

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		AfterNotesChange( (PlayerNumber)p );
	}

	/* Make sure we never start the sample when moving fast. */
	if(m_MusicWheel.IsMoving())
		m_fPlaySampleCountdown = 0;
}


void ScreenSelectMusic::UpdateOptionsDisplays()
{
//	m_OptionIcons.Load( GAMESTATE->m_PlayerOptions, &GAMESTATE->m_SongOptions );

//	m_PlayerOptionIcons.Refresh();

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		m_OptionIconRow[p].Refresh( (PlayerNumber)p  );

		if( GAMESTATE->IsHumanPlayer(p) )
		{
			CString s = GAMESTATE->m_PlayerOptions[p].GetString();
			s.Replace( ", ", "\n" );
//			m_textPlayerOptions[p].SetText( s );
		}
	}

	CString s = GAMESTATE->m_SongOptions.GetString();
	s.Replace( ", ", "\n" );
	m_textSongOptions.SetText( s );
}

void ScreenSelectMusic::SortOrderChanged()
{
	m_MusicSortDisplay.Set( GAMESTATE->m_SongSortOrder );

	// tween music sort on screen
//	m_MusicSortDisplay.FadeOn( 0, "fade", TWEEN_TIME );
}

