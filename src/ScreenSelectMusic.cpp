#include "stdafx.h"
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
#include "RageMusic.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "InputMapper.h"
#include "InputQueue.h"
#include "AnnouncerManager.h"
#include "InputMapper.h"
#include "GameState.h"


#define BANNER_FRAME_X		THEME->GetMetricF("ScreenSelectMusic","BannerFrameX")
#define BANNER_FRAME_Y		THEME->GetMetricF("ScreenSelectMusic","BannerFrameY")
#define BANNER_X			THEME->GetMetricF("ScreenSelectMusic","BannerX")
#define BANNER_Y			THEME->GetMetricF("ScreenSelectMusic","BannerY")
#define BANNER_WIDTH		THEME->GetMetricF("ScreenSelectMusic","BannerWidth")
#define BANNER_HEIGHT		THEME->GetMetricF("ScreenSelectMusic","BannerHeight")
#define BPM_X				THEME->GetMetricF("ScreenSelectMusic","BPMX")
#define BPM_Y				THEME->GetMetricF("ScreenSelectMusic","BPMY")
#define STAGE_X				THEME->GetMetricF("ScreenSelectMusic","StageX")
#define STAGE_Y				THEME->GetMetricF("ScreenSelectMusic","StageY")
#define CD_TITLE_X			THEME->GetMetricF("ScreenSelectMusic","CDTitleX")
#define CD_TITLE_Y			THEME->GetMetricF("ScreenSelectMusic","CDTitleY")
#define DIFFICULTY_X		THEME->GetMetricF("ScreenSelectMusic","DifficultyX")
#define DIFFICULTY_Y		THEME->GetMetricF("ScreenSelectMusic","DifficultyY")
#define ICON_X( p )			THEME->GetMetricF("ScreenSelectMusic",ssprintf("IconP%dX",p+1))
#define ICON_Y( i )			THEME->GetMetricF("ScreenSelectMusic",ssprintf("IconP%dY",i+1))
#define RADAR_X				THEME->GetMetricF("ScreenSelectMusic","RadarX")
#define RADAR_Y				THEME->GetMetricF("ScreenSelectMusic","RadarY")
#define SORT_ICON_X			THEME->GetMetricF("ScreenSelectMusic","SortIconX")
#define SORT_ICON_Y			THEME->GetMetricF("ScreenSelectMusic","SortIconY")
#define SCORE_X( p )		THEME->GetMetricF("ScreenSelectMusic",ssprintf("ScoreP%dX",p+1))
#define SCORE_Y( i )		THEME->GetMetricF("ScreenSelectMusic",ssprintf("ScoreP%dY",i+1))
#define METER_FRAME_X		THEME->GetMetricF("ScreenSelectMusic","MeterFrameX")
#define METER_FRAME_Y		THEME->GetMetricF("ScreenSelectMusic","MeterFrameY")
#define METER_X( p )		THEME->GetMetricF("ScreenSelectMusic",ssprintf("MeterP%dX",p+1))
#define METER_Y( i )		THEME->GetMetricF("ScreenSelectMusic",ssprintf("MeterP%dY",i+1))
#define WHEEL_X				THEME->GetMetricF("ScreenSelectMusic","WheelX")
#define WHEEL_Y				THEME->GetMetricF("ScreenSelectMusic","WheelY")
#define PLAYER_OPTIONS_X( p )THEME->GetMetricF("ScreenSelectMusic",ssprintf("PlayerOptionsP%dX",p+1))
#define PLAYER_OPTIONS_Y( i )THEME->GetMetricF("ScreenSelectMusic",ssprintf("PlayerOptionsP%dY",i+1))
#define SONG_OPTIONS_X		THEME->GetMetricF("ScreenSelectMusic","SongOptionsX")
#define SONG_OPTIONS_Y		THEME->GetMetricF("ScreenSelectMusic","SongOptionsY")
#define HELP_TEXT			THEME->GetMetric("ScreenSelectMusic","HelpText")
#define TIMER_SECONDS		THEME->GetMetricI("ScreenSelectMusic","TimerSeconds")
#define SCORE_CONNECTED_TO_MUSIC_WHEEL	THEME->GetMetricB("ScreenSelectMusic","ScoreConnectedToMusicWheel")

const float TWEEN_TIME		= 0.5f;


const ScreenMessage SM_GoToPrevScreen		=	ScreenMessage(SM_User+1);
const ScreenMessage SM_GoToNextScreen		=	ScreenMessage(SM_User+2);



ScreenSelectMusic::ScreenSelectMusic()
{
	LOG->Trace( "ScreenSelectMusic::ScreenSelectMusic()" );

	// for debugging
	if( GAMESTATE->m_CurStyle == STYLE_NONE )
		GAMESTATE->m_CurStyle = STYLE_DANCE_SINGLE;

	int p;

	m_Menu.Load(
		THEME->GetPathTo("Graphics","select music background"), 
		THEME->GetPathTo("Graphics","select music top edge"),
		HELP_TEXT, true, TIMER_SECONDS 
		);
	this->AddSubActor( &m_Menu );

	// these guys get loaded SetSong and TweenToSong
	m_Banner.SetXY( BANNER_X, BANNER_Y );
	m_Banner.SetCroppedSize( BANNER_WIDTH, BANNER_HEIGHT );
	this->AddSubActor( &m_Banner );

	m_sprBannerFrame.Load( THEME->GetPathTo("Graphics","select music info frame") );
	m_sprBannerFrame.SetXY( BANNER_FRAME_X, BANNER_FRAME_Y );
	this->AddSubActor( &m_sprBannerFrame );

	m_BPMDisplay.SetXY( BPM_X, BPM_Y );
	m_BPMDisplay.SetZoomX( 1.0f );
	this->AddSubActor( &m_BPMDisplay );

	m_textStage.LoadFromFont( THEME->GetPathTo("Fonts","Header2") );
	m_textStage.TurnShadowOff();
	m_textStage.SetZoomX( 1.0f );
	m_textStage.SetXY( STAGE_X, STAGE_Y );
	m_textStage.SetText( GAMESTATE->GetStageText() );
	m_textStage.SetDiffuseColor( GAMESTATE->GetStageColor() );
	this->AddSubActor( &m_textStage );

	m_sprCDTitle.Load( THEME->GetPathTo("Graphics","fallback cd title") );
	m_sprCDTitle.TurnShadowOff();
	m_sprCDTitle.SetXY( CD_TITLE_X, CD_TITLE_Y );
	this->AddSubActor( &m_sprCDTitle );

	m_sprDifficultyFrame.Load( THEME->GetPathTo("Graphics","select music difficulty frame") );
	m_sprDifficultyFrame.SetXY( DIFFICULTY_X, DIFFICULTY_Y );
	this->AddSubActor( &m_sprDifficultyFrame );

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		m_DifficultyIcon[p].SetXY( ICON_X(p), ICON_Y(p) );
		this->AddSubActor( &m_DifficultyIcon[p] );
	}

	m_GrooveRadar.SetXY( RADAR_X, RADAR_Y );
	this->AddSubActor( &m_GrooveRadar );

	m_textSongOptions.LoadFromFont( THEME->GetPathTo("Fonts","normal") );
	m_textSongOptions.SetXY( SONG_OPTIONS_X, SONG_OPTIONS_Y );
	m_textSongOptions.SetZoom( 0.5f );
	if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
		m_textSongOptions.SetEffectCamelion( 2.5f, D3DXCOLOR(1,0,0,1), D3DXCOLOR(1,1,1,1) );	// blink red
	m_textSongOptions.SetDiffuseColor( D3DXCOLOR(1,1,1,1) );	// white
	this->AddSubActor( &m_textSongOptions );

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled(p) )
			continue;

		m_textPlayerOptions[p].LoadFromFont( THEME->GetPathTo("Fonts","normal") );
		m_textPlayerOptions[p].SetXY( PLAYER_OPTIONS_X(p), PLAYER_OPTIONS_Y(p) );
		m_textPlayerOptions[p].SetZoom( 0.5f );
		m_textPlayerOptions[p].SetHorizAlign( p==PLAYER_1 ? Actor::align_left : Actor::align_right );
		m_textPlayerOptions[p].SetVertAlign( Actor::align_middle );
		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			m_textPlayerOptions[p].SetEffectCamelion( 2.5f, D3DXCOLOR(1,0,0,1), D3DXCOLOR(1,1,1,1) );	// blink red
		m_textPlayerOptions[p].SetDiffuseColor( D3DXCOLOR(1,1,1,1) );	// white
		this->AddSubActor( &m_textPlayerOptions[p] );
	}

	m_sprMeterFrame.Load( THEME->GetPathTo("Graphics","select music meter frame") );
	m_sprMeterFrame.SetXY( METER_FRAME_X, METER_FRAME_Y );
	this->AddSubActor( &m_sprMeterFrame );

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		m_FootMeter[p].LoadFromFont( THEME->GetPathTo("Fonts","meter") );
		m_FootMeter[p].SetXY( METER_X(p), METER_Y(p) );
		m_FootMeter[p].SetShadowLength( 2 );
		this->AddSubActor( &m_FootMeter[p] );
	}

	m_MusicWheel.SetXY( WHEEL_X, WHEEL_Y );
	this->AddSubActor( &m_MusicWheel );

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled((PlayerNumber)p) )
			continue;	// skip

		m_sprHighScoreFrame[p].Load( THEME->GetPathTo("Graphics","select music score frame") );
		m_sprHighScoreFrame[p].StopAnimating();
		m_sprHighScoreFrame[p].SetState( p );
		m_sprHighScoreFrame[p].SetXY( SCORE_X(p), SCORE_Y(p) );
		this->AddSubActor( &m_sprHighScoreFrame[p] );

		m_HighScore[p].SetXY( SCORE_X(p), SCORE_Y(p) );
		m_HighScore[p].SetZoom( 0.6f );
		m_HighScore[p].SetDiffuseColor( PlayerToColor(p) );
		this->AddSubActor( &m_HighScore[p] );
	}	

	m_MusicSortDisplay.SetXY( SORT_ICON_X, SORT_ICON_Y );
	//m_MusicSortDisplay.SetEffectGlowing( 1.0f );
	m_MusicSortDisplay.Set( GAMESTATE->m_SongSortOrder );
	this->AddSubActor( &m_MusicSortDisplay );


	m_textHoldForOptions.LoadFromFont( THEME->GetPathTo("Fonts","stage") );
	m_textHoldForOptions.SetXY( CENTER_X, CENTER_Y );
	m_textHoldForOptions.SetText( "press START again for options" );
	m_textHoldForOptions.SetZoom( 1 );
	m_textHoldForOptions.SetZoomY( 0 );
	m_textHoldForOptions.SetDiffuseColor( D3DXCOLOR(1,1,1,0) );
	m_textHoldForOptions.SetZ( -2 );
	this->AddSubActor( &m_textHoldForOptions );


	m_soundSelect.Load( THEME->GetPathTo("Sounds","menu start") );
	m_soundChangeNotes.Load( THEME->GetPathTo("Sounds","select music change notes") );
	m_soundLocked.Load( THEME->GetPathTo("Sounds","select music wheel locked") );

	SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo("select music intro") );

	m_bMadeChoice = false;
	m_bGoToOptions = false;

	UpdateOptionsDisplays();

	AfterMusicChange();
	TweenOnScreen();
	m_Menu.TweenOnScreenFromMenu( SM_None );
}


ScreenSelectMusic::~ScreenSelectMusic()
{
	LOG->Trace( "ScreenSelectMusic::~ScreenSelectMusic()" );

}

void ScreenSelectMusic::DrawPrimitives()
{
	m_Menu.DrawBottomLayer();
	Screen::DrawPrimitives();
	m_Menu.DrawTopLayer();
}

void ScreenSelectMusic::TweenOnScreen()
{
	float fOriginalZoomY;
	int i;

	CArray<Actor*,Actor*> apActorsInGroupInfoFrame;
	apActorsInGroupInfoFrame.Add( &m_sprBannerFrame );
	apActorsInGroupInfoFrame.Add( &m_Banner );
	apActorsInGroupInfoFrame.Add( &m_BPMDisplay );
	apActorsInGroupInfoFrame.Add( &m_textStage );
	apActorsInGroupInfoFrame.Add( &m_sprCDTitle );
	for( i=0; i<apActorsInGroupInfoFrame.GetSize(); i++ )
	{
		float fOriginalX = apActorsInGroupInfoFrame[i]->GetX();
		apActorsInGroupInfoFrame[i]->SetX( fOriginalX-400 );
		apActorsInGroupInfoFrame[i]->BeginTweening( TWEEN_TIME, TWEEN_BOUNCE_END );
		apActorsInGroupInfoFrame[i]->SetTweenX( fOriginalX );
	}

	fOriginalZoomY = m_sprDifficultyFrame.GetZoomY();
	m_sprDifficultyFrame.BeginTweening( TWEEN_TIME );
	m_sprDifficultyFrame.SetTweenZoomY( fOriginalZoomY );

	fOriginalZoomY = m_sprMeterFrame.GetZoomY();
	m_sprMeterFrame.BeginTweening( TWEEN_TIME );
	m_sprMeterFrame.SetTweenZoomY( fOriginalZoomY );

	m_GrooveRadar.TweenOnScreen();

	fOriginalZoomY = m_textSongOptions.GetZoomY();
	m_textSongOptions.BeginTweening( TWEEN_TIME );
	m_textSongOptions.SetTweenZoomY( fOriginalZoomY );

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		fOriginalZoomY = m_textPlayerOptions[p].GetZoomY();
		m_textPlayerOptions[p].BeginTweening( TWEEN_TIME );
		m_textPlayerOptions[p].SetTweenZoomY( fOriginalZoomY );

		fOriginalZoomY = m_DifficultyIcon[p].GetZoomY();
		m_DifficultyIcon[p].BeginTweening( TWEEN_TIME );
		m_DifficultyIcon[p].SetTweenZoomY( fOriginalZoomY );

		fOriginalZoomY = m_FootMeter[p].GetZoomY();
		m_FootMeter[p].BeginTweening( TWEEN_TIME );
		m_FootMeter[p].SetTweenZoomY( fOriginalZoomY );
	}

	m_MusicSortDisplay.SetDiffuseColor( D3DXCOLOR(1,1,1,0) );
	m_MusicSortDisplay.BeginTweening( TWEEN_TIME );
	m_MusicSortDisplay.SetTweenDiffuseColor( D3DXCOLOR(1,1,1,1) );

	CArray<Actor*,Actor*> apActorsInScore;
	for( p=0; p<NUM_PLAYERS; p++ )
	{
		apActorsInScore.Add( &m_sprHighScoreFrame[p] );
		apActorsInScore.Add( &m_HighScore[p] );
	}
	for( i=0; i<apActorsInScore.GetSize(); i++ )
	{
		float fOriginalX = apActorsInScore[i]->GetX();
		apActorsInScore[i]->SetX( SCORE_CONNECTED_TO_MUSIC_WHEEL ? fOriginalX+400 : fOriginalX-400 );
		apActorsInScore[i]->BeginTweening( TWEEN_TIME, TWEEN_BIAS_END );
		apActorsInScore[i]->SetTweenX( fOriginalX );
	}

	m_MusicWheel.TweenOnScreen();
}

void ScreenSelectMusic::TweenOffScreen()
{
	int i;

	CArray<Actor*,Actor*> apActorsInGroupInfoFrame;
	apActorsInGroupInfoFrame.Add( &m_sprBannerFrame );
	apActorsInGroupInfoFrame.Add( &m_Banner );
	apActorsInGroupInfoFrame.Add( &m_BPMDisplay );
	apActorsInGroupInfoFrame.Add( &m_textStage );
	apActorsInGroupInfoFrame.Add( &m_sprCDTitle );
	for( i=0; i<apActorsInGroupInfoFrame.GetSize(); i++ )
	{
		apActorsInGroupInfoFrame[i]->BeginTweeningQueued( TWEEN_TIME, TWEEN_BOUNCE_BEGIN );
		apActorsInGroupInfoFrame[i]->SetTweenX( apActorsInGroupInfoFrame[i]->GetX()-400 );
	}

	m_sprDifficultyFrame.BeginTweening( TWEEN_TIME );
	m_sprDifficultyFrame.SetTweenZoomY( 0 );

	m_sprMeterFrame.BeginTweening( TWEEN_TIME );
	m_sprMeterFrame.SetTweenZoomY( 0 );

	m_GrooveRadar.TweenOffScreen();

	m_textSongOptions.BeginTweening( TWEEN_TIME );
	m_textSongOptions.SetTweenZoomY( 0 );

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		m_textPlayerOptions[p].BeginTweening( TWEEN_TIME );
		m_textPlayerOptions[p].SetTweenZoomY( 0 );

		m_DifficultyIcon[p].BeginTweening( TWEEN_TIME );
		m_DifficultyIcon[p].SetTweenZoomY( 0 );

		m_FootMeter[p].BeginTweening( TWEEN_TIME );
		m_FootMeter[p].SetTweenZoomY( 0 );
	}

	m_MusicSortDisplay.SetEffectNone();
	m_MusicSortDisplay.BeginTweening( TWEEN_TIME );
	m_MusicSortDisplay.SetTweenDiffuseColor( D3DXCOLOR(1,1,1,0) );

	CArray<Actor*,Actor*> apActorsInScore;
	for( p=0; p<NUM_PLAYERS; p++ )
	{
		apActorsInScore.Add( &m_sprHighScoreFrame[p] );
		apActorsInScore.Add( &m_HighScore[p] );
	}
	for( i=0; i<apActorsInScore.GetSize(); i++ )
	{
		apActorsInScore[i]->BeginTweening( TWEEN_TIME, TWEEN_BIAS_END );
		apActorsInScore[i]->SetTweenX( SCORE_CONNECTED_TO_MUSIC_WHEEL ? apActorsInScore[i]->GetX()+400 : apActorsInScore[i]->GetX()-400 );
	}

	m_MusicWheel.TweenOffScreen();
}

void ScreenSelectMusic::TweenScoreOnAndOffAfterChangeSort()
{
	if( !SCORE_CONNECTED_TO_MUSIC_WHEEL )
		return;	// do nothing

	CArray<Actor*,Actor*> apActorsInScore;
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		apActorsInScore.Add( &m_sprHighScoreFrame[p] );
		apActorsInScore.Add( &m_HighScore[p] );
	}
	for( int i=0; i<apActorsInScore.GetSize(); i++ )
	{
		float fOriginalX = apActorsInScore[i]->GetX();
		apActorsInScore[i]->BeginTweening( TWEEN_TIME, TWEEN_BIAS_END );		// tween off screen
		apActorsInScore[i]->SetTweenX( fOriginalX+400 );
		
		apActorsInScore[i]->BeginTweeningQueued( 0.5f );		// sleep

		apActorsInScore[i]->BeginTweeningQueued( 1, TWEEN_BIAS_BEGIN );		// tween back on screen
		apActorsInScore[i]->SetTweenX( fOriginalX );
	}
}

void ScreenSelectMusic::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );

	float fNewRotation = m_sprCDTitle.GetRotationY()+D3DX_PI*fDeltaTime/2;
	fNewRotation = fmodf( fNewRotation, D3DX_PI*2 );
	m_sprCDTitle.SetRotationY( fNewRotation );
	if( fNewRotation > D3DX_PI/2  &&  fNewRotation <= D3DX_PI*3.0f/2 )
		m_sprCDTitle.SetDiffuseColor( D3DXCOLOR(0.2f,0.2f,0.2f,1) );
	else
		m_sprCDTitle.SetDiffuseColor( D3DXCOLOR(1,1,1,1) );
}

const GameButton DANCE_EASIER_DIFFICULTY_PATTERN[] = { DANCE_BUTTON_UP, DANCE_BUTTON_UP };
const int DANCE_EASIER_DIFFICULTY_PATTERN_SIZE = sizeof(DANCE_EASIER_DIFFICULTY_PATTERN) / sizeof(GameButton);

const GameButton DANCE_HARDER_DIFFICULTY_PATTERN[] = { DANCE_BUTTON_DOWN, DANCE_BUTTON_DOWN };
const int DANCE_HARDER_DIFFICULTY_PATTERN_SIZE = sizeof(DANCE_HARDER_DIFFICULTY_PATTERN) / sizeof(GameButton);

const MenuButton MENU_EASIER_DIFFICULTY_PATTERN[] = { MENU_BUTTON_UP, MENU_BUTTON_UP };
const int MENU_EASIER_DIFFICULTY_PATTERN_SIZE = sizeof(MENU_EASIER_DIFFICULTY_PATTERN) / sizeof(MenuButton);

const MenuButton MENU_HARDER_DIFFICULTY_PATTERN[] = { MENU_BUTTON_DOWN, MENU_BUTTON_DOWN };
const int MENU_HARDER_DIFFICULTY_PATTERN_SIZE = sizeof(MENU_HARDER_DIFFICULTY_PATTERN) / sizeof(MenuButton);

const MenuButton MENU_NEXT_SORT_PATTERN[] = { MENU_BUTTON_UP, MENU_BUTTON_DOWN, MENU_BUTTON_UP, MENU_BUTTON_DOWN };
const int MENU_NEXT_SORT_PATTERN_SIZE = sizeof(MENU_NEXT_SORT_PATTERN) / sizeof(MenuButton);

void ScreenSelectMusic::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	LOG->Trace( "ScreenSelectMusic::Input()" );
	if(type == IET_RELEASE) return; // don't care

	if( MenuI.player == PLAYER_INVALID )
		return;

	if( m_Menu.IsClosing() )
		return;		// ignore

	if( m_bMadeChoice  &&  !m_bGoToOptions  &&  MenuI.button == MENU_BUTTON_START  &&  !GAMESTATE->IsExtraStage()  &&  !GAMESTATE->IsExtraStage2() )
	{
		m_bGoToOptions = true;
		m_textHoldForOptions.SetText( "Entering Options..." );
		SOUND->PlayOnceStreamed( THEME->GetPathTo("Sounds","menu start") );
		return;
	}
	
	if( m_bMadeChoice )
		return;

	switch( GAMESTATE->m_CurGame )
	{
	case GAME_DANCE:
		if( INPUTQUEUE->MatchesPattern(GameI.controller, DANCE_EASIER_DIFFICULTY_PATTERN, DANCE_EASIER_DIFFICULTY_PATTERN_SIZE) )
		{
			if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
				m_soundLocked.Play();
			else
				EasierDifficulty( MenuI.player );
			return;
		}
		if( INPUTQUEUE->MatchesPattern(GameI.controller, DANCE_HARDER_DIFFICULTY_PATTERN, DANCE_HARDER_DIFFICULTY_PATTERN_SIZE) )
		{
			if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
				m_soundLocked.Play();
			else
				HarderDifficulty( MenuI.player );
			return;
		}
		break;
	}

	if( INPUTQUEUE->MatchesPattern(GameI.controller, MENU_EASIER_DIFFICULTY_PATTERN, MENU_EASIER_DIFFICULTY_PATTERN_SIZE) )
	{
		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			m_soundLocked.Play();
		else
			EasierDifficulty( MenuI.player );
		return;
	}
	if( INPUTQUEUE->MatchesPattern(GameI.controller, MENU_HARDER_DIFFICULTY_PATTERN, MENU_HARDER_DIFFICULTY_PATTERN_SIZE) )
	{
		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			m_soundLocked.Play();
		else
			HarderDifficulty( MenuI.player );
		return;
	}
	if( INPUTQUEUE->MatchesPattern(GameI.controller, MENU_NEXT_SORT_PATTERN, MENU_NEXT_SORT_PATTERN_SIZE) )
	{
		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			m_soundLocked.Play();
		else
			if( m_MusicWheel.NextSort() )
			{
				MUSIC->Stop();

				m_MusicSortDisplay.BeginTweening( 0.3f );
				m_MusicSortDisplay.SetTweenDiffuseColor( D3DXCOLOR(1,1,1,0) );

				TweenScoreOnAndOffAfterChangeSort();
			}
		return;
	}

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default input handler
}


void ScreenSelectMusic::EasierDifficulty( PlayerNumber p )
{
	LOG->Trace( "ScreenSelectMusic::EasierDifficulty( %d )", p );

	if( !GAMESTATE->IsPlayerEnabled(p) )
		return;
	if( m_arrayNotes.GetSize() == 0 )
		return;
	if( m_iSelection[p] == 0 )
		return;

	m_iSelection[p]--;
	// the user explicity switched difficulties.  Update the preferred difficulty
	GAMESTATE->m_PreferredDifficultyClass[p] = m_arrayNotes[ m_iSelection[p] ]->m_DifficultyClass;

	m_soundChangeNotes.Play();

	AfterNotesChange( p );
}

void ScreenSelectMusic::HarderDifficulty( PlayerNumber p )
{
	LOG->Trace( "ScreenSelectMusic::HarderDifficulty( %d )", p );

	if( !GAMESTATE->IsPlayerEnabled(p) )
		return;
	if( m_arrayNotes.GetSize() == 0 )
		return;
	if( m_iSelection[p] == m_arrayNotes.GetSize()-1 )
		return;

	m_iSelection[p]++;
	// the user explicity switched difficulties.  Update the preferred difficulty
	GAMESTATE->m_PreferredDifficultyClass[p] = m_arrayNotes[ m_iSelection[p] ]->m_DifficultyClass;

	m_soundChangeNotes.Play();

	AfterNotesChange( p );
}


void ScreenSelectMusic::HandleScreenMessage( const ScreenMessage SM )
{
	Screen::HandleScreenMessage( SM );

	switch( SM )
	{
	case SM_MenuTimer:
		if( m_MusicWheel.IsRouletting() )
		{
			MenuStart(PLAYER_INVALID);
			m_Menu.SetTimer( 15 );
		}
		else if( m_MusicWheel.GetSelectedType() != TYPE_SONG )
		{
			m_MusicWheel.StartRoulette();
			m_Menu.SetTimer( 15 );
		}
		else
		{
			MenuStart(PLAYER_INVALID);
		}
		break;
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
		break;
	case SM_GoToNextScreen:
		if( m_bGoToOptions )
		{
			SCREENMAN->SetNewScreen( "ScreenPlayerOptions" );
		}
		else
		{
			MUSIC->Stop();
			SCREENMAN->SetNewScreen( "ScreenStage" );
		}
		break;
	case SM_PlaySongSample:
		PlayMusicSample();
		break;
	case SM_SongChanged:
		AfterMusicChange();
		break;
	case SM_SortOrderChanged:
		SortOrderChanged();
		break;
	}
}

void ScreenSelectMusic::MenuLeft( PlayerNumber p, const InputEventType type )
{
	if( type >= IET_SLOW_REPEAT  &&  INPUTMAPPER->IsButtonDown( MenuInput(p, MENU_BUTTON_RIGHT) ) )
			return;		// ignore
	
	if( ! m_MusicWheel.WheelIsLocked() )
		MUSIC->Stop();

	m_MusicWheel.PrevMusic();
}


void ScreenSelectMusic::MenuRight( PlayerNumber p, const InputEventType type )
{
	if( type >= IET_SLOW_REPEAT  &&  INPUTMAPPER->IsButtonDown( MenuInput(p, MENU_BUTTON_LEFT) ) )
		return;		// ignore

	if( ! m_MusicWheel.WheelIsLocked() )
		MUSIC->Stop();

	m_MusicWheel.NextMusic();
}

void ScreenSelectMusic::MenuStart( PlayerNumber p )
{
	if( p != PLAYER_INVALID  &&
		INPUTMAPPER->IsButtonDown( MenuInput(p, MENU_BUTTON_LEFT) )  &&
		INPUTMAPPER->IsButtonDown( MenuInput(p, MENU_BUTTON_RIGHT) ) )
	{
		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			m_soundLocked.Play();
		else
		{
			if( m_MusicWheel.NextSort() )
			{
				MUSIC->Stop();

				m_MusicSortDisplay.BeginTweening( 0.3f );
				m_MusicSortDisplay.SetTweenDiffuseColor( D3DXCOLOR(1,1,1,0) );

				TweenScoreOnAndOffAfterChangeSort();
			}
		}
		return;
	}


	// this needs to check whether valid Notes are selected!
	bool bResult = m_MusicWheel.Select();

	if( !bResult )
	{
	/* why do this? breaks tabs and roulette -glenn */
//		if( p != PLAYER_INVALID )
//			this->SendScreenMessage( SM_MenuTimer, 1 );	// re-throw a timer message
	}
	else	// if !bResult
	{
		// a song was selected
		switch( m_MusicWheel.GetSelectedType() )
		{
		case TYPE_SONG:
			{
				if( !m_MusicWheel.GetSelectedSong()->HasMusic() )
				{
					/* TODO: gray these out. 
					 *
					 * XXX: also, make sure they're not selected by roulette */
					SCREENMAN->Prompt( SM_None, "ERROR:\n \nThis song does not have a music file\n and cannot be played." );
					return;
				}

				bool bIsNew = m_MusicWheel.GetSelectedSong()->IsNew();
				bool bIsHard = false;
				for( int p=0; p<NUM_PLAYERS; p++ )
				{
					if( !GAMESTATE->IsPlayerEnabled( (PlayerNumber)p ) )
						continue;	// skip
					if( GAMESTATE->m_pCurNotes[p]  &&  GAMESTATE->m_pCurNotes[p]->m_iMeter >= 9 )
						bIsHard = true;
				}

				if( bIsNew )
					SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo("select music comment new") );
				else if( bIsHard )
					SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo("select music comment hard") );
				else
					SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo("select music comment general") );


				TweenOffScreen();

				m_bMadeChoice = true;

				m_soundSelect.Play();

				if( !GAMESTATE->IsExtraStage()  &&  !GAMESTATE->IsExtraStage2() )
				{
					// show "hold START for options"
					m_textHoldForOptions.SetDiffuseColor( D3DXCOLOR(1,1,1,0) );
					m_textHoldForOptions.BeginTweeningQueued( 0.25f );	// fade in
					m_textHoldForOptions.SetTweenZoomY( 1 );
					m_textHoldForOptions.SetTweenDiffuseColor( D3DXCOLOR(1,1,1,1) );
					m_textHoldForOptions.BeginTweeningQueued( 2.0f );	// sleep
					m_textHoldForOptions.BeginTweeningQueued( 0.25f );	// fade out
					m_textHoldForOptions.SetTweenDiffuseColor( D3DXCOLOR(1,1,1,0) );
					m_textHoldForOptions.SetTweenZoomY( 0 );
				}

				m_Menu.TweenOffScreenToBlack( SM_None, false );

				m_Menu.StopTimer();

				this->SendScreenMessage( SM_GoToNextScreen, 2.5f );
			}
			break;
		case TYPE_SECTION:
			
			break;
		case TYPE_ROULETTE:

			break;
		}
	}

}


void ScreenSelectMusic::MenuBack( PlayerNumber p )
{
	MUSIC->Stop();

	m_Menu.TweenOffScreenToBlack( SM_GoToPrevScreen, true );
}

void ScreenSelectMusic::AfterNotesChange( PlayerNumber p )
{
	if( !GAMESTATE->IsPlayerEnabled(p) )
		return;
	
	m_iSelection[p] = clamp( m_iSelection[p], 0, m_arrayNotes.GetSize()-1 );	// bounds clamping

	Notes* pNotes = m_arrayNotes.GetSize()>0 ? m_arrayNotes[m_iSelection[p]] : NULL;

	GAMESTATE->m_pCurNotes[p] = pNotes;

//	m_BPMDisplay.SetZoomY( 0 );
//	m_BPMDisplay.BeginTweening( 0.2f );
//	m_BPMDisplay.SetTweenZoomY( 1.2f );

	DifficultyClass dc = GAMESTATE->m_PreferredDifficultyClass[p];
	Song* pSong = GAMESTATE->m_pCurSong;
	Notes* m_pNotes = GAMESTATE->m_pCurNotes[p];
	
	if( m_pNotes )
		m_HighScore[p].SetScore( (float)m_pNotes->m_iTopScore );

	m_DifficultyIcon[p].SetFromNotes( pNotes );
	m_FootMeter[p].SetFromNotes( pNotes );
	m_GrooveRadar.SetFromNotes( p, pNotes );
	m_MusicWheel.NotesChanged( p );
}

void ScreenSelectMusic::AfterMusicChange()
{
	m_Menu.StallTimer();

	Song* pSong = m_MusicWheel.GetSelectedSong();
	GAMESTATE->m_pCurSong = pSong;

	m_arrayNotes.RemoveAll();

	switch( m_MusicWheel.GetSelectedType() )
	{
	case TYPE_SECTION:
		{	
			CString sGroup = m_MusicWheel.GetSelectedSection();
			for( int p=0; p<NUM_PLAYERS; p++ )
			{
				m_iSelection[p] = -1;
			}

			m_Banner.SetFromGroup( sGroup );	// if this isn't a group, it'll default to the fallback banner
			m_BPMDisplay.SetBPMRange( 0, 0 );
			m_sprCDTitle.UnloadTexture();
		}
		break;
	case TYPE_SONG:
		{
			pSong->GetNotesThatMatch( GAMESTATE->GetCurrentStyleDef()->m_NotesType, m_arrayNotes );
			SortNotesArrayByDifficulty( m_arrayNotes );

			m_Banner.SetFromSong( pSong );

			float fMinBPM, fMaxBPM;
			pSong->GetMinMaxBPM( fMinBPM, fMaxBPM );
			m_BPMDisplay.SetBPMRange( fMinBPM, fMaxBPM );

			if( pSong->HasCDTitle() )
				m_sprCDTitle.Load( pSong->GetCDTitlePath() );
			else
				m_sprCDTitle.Load( THEME->GetPathTo("Graphics","fallback cd title") );
			for( int p=0; p<NUM_PLAYERS; p++ )
			{
				if( !GAMESTATE->IsPlayerEnabled( PlayerNumber(p) ) )
					continue;
				for( int i=0; i<m_arrayNotes.GetSize(); i++ )
					if( m_arrayNotes[i]->m_DifficultyClass == GAMESTATE->m_PreferredDifficultyClass[p] )
						m_iSelection[p] = i;

				m_iSelection[p] = clamp( m_iSelection[p], 0, m_arrayNotes.GetSize() ) ;
			}
		}
		break;
	case TYPE_ROULETTE:
		m_Banner.SetRoulette();
		m_BPMDisplay.SetBPMRange( 0, 0 );
		m_sprCDTitle.UnloadTexture();
		break;
	}

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		AfterNotesChange( (PlayerNumber)p );
	}
}


void ScreenSelectMusic::PlayMusicSample()
{
	//LOG->Trace( "ScreenSelectSong::PlaySONGample()" );

	Song* pSong = m_MusicWheel.GetSelectedSong();
	if( pSong )
	{
		MUSIC->Stop();
		MUSIC->Load( pSong->GetMusicPath() );
		MUSIC->Play( true, pSong->m_fMusicSampleStartSeconds, pSong->m_fMusicSampleLengthSeconds );
	}
	else
		MUSIC->LoadAndPlayIfNotAlready( THEME->GetPathTo("Sounds","select music music") );
}

void ScreenSelectMusic::UpdateOptionsDisplays()
{
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( GAMESTATE->IsPlayerEnabled(p) )
		{
			CString s = GAMESTATE->m_PlayerOptions[p].GetString();
			s.Replace( ", ", "\n" );
			m_textPlayerOptions[p].SetText( s );
		}
	}

	CString s = GAMESTATE->m_SongOptions.GetString();
	s.Replace( ", ", "\n" );
	m_textSongOptions.SetText( s );
}

void ScreenSelectMusic::SortOrderChanged()
{
	m_MusicSortDisplay.SetState( GAMESTATE->m_SongSortOrder );

	// tween music sort on screen
//	m_MusicSortDisplay.SetEffectGlowing();
	m_MusicSortDisplay.BeginTweening( 0.3f );
	m_MusicSortDisplay.SetTweenDiffuseColor( D3DXCOLOR(1,1,1,1) );		
}

