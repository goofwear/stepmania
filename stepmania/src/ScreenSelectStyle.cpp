#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 File: ScreenSelectStyle.cpp

 Desc: Testing the Screen class.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
-----------------------------------------------------------------------------
*/


#include "ScreenSelectStyle.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "RageMusic.h"
#include "ScreenTitleMenu.h"
#include "ScreenCaution.h"
#include "GameConstantsAndTypes.h"
#include "ThemeManager.h"
#include "ScreenSelectDifficulty.h"
#include "ScreenSandbox.h"
#include "GameManager.h"
#include "RageLog.h"
#include "AnnouncerManager.h"


const float ICONS_START_X	= SCREEN_LEFT + 60;
const float ICONS_SPACING_X	= 76;
const float ICON_Y			= SCREEN_TOP + 100;

const float EXPLANATION_X	= SCREEN_RIGHT - 160;
const float EXPLANATION_Y	= CENTER_Y;

const float INFO_X			= SCREEN_RIGHT - 160;
const float INFO_Y			= CENTER_Y+200;

const float PREVIEW_X		= SCREEN_LEFT + 160;
const float PREVIEW_Y		= CENTER_Y;


const ScreenMessage SM_GoToPrevState		=	ScreenMessage(SM_User + 1);
const ScreenMessage SM_GoToNextState		=	ScreenMessage(SM_User + 2);


ScreenSelectStyle::ScreenSelectStyle()
{
	LOG->WriteLine( "ScreenSelectStyle::ScreenSelectStyle()" );

	for( int s=0; s<NUM_STYLES; s++ )
	{
		Style style = (Style)s;
		if( StyleToGame(style) == GAMEMAN->m_CurGame )	// games match
			m_aPossibleStyles.Add( style );		
	}

	m_iSelection = 0;

	for( int i=0; i<m_aPossibleStyles.GetSize(); i++ )
	{
		Style style = m_aPossibleStyles[i];
		m_sprIcon[i].Load( THEME->GetPathTo(GRAPHIC_SELECT_STYLE_ICONS) );
		m_sprIcon[i].StopAnimating();
		m_sprIcon[i].SetState( style );
		m_sprIcon[i].SetXY( ICONS_START_X + ICONS_SPACING_X*i, ICON_Y );
		this->AddActor( &m_sprIcon[i] );
	}

	m_sprExplanation.Load( THEME->GetPathTo(GRAPHIC_SELECT_STYLE_EXPLANATION) );
	m_sprExplanation.SetXY( EXPLANATION_X, EXPLANATION_Y );
	this->AddActor( &m_sprExplanation );
	
	m_sprPreview.SetXY( PREVIEW_X, PREVIEW_Y );
	this->AddActor( &m_sprPreview );
	
	m_sprInfo.SetXY( INFO_X, INFO_Y );
	this->AddActor( &m_sprInfo );
	
	m_Menu.Load( 	
		THEME->GetPathTo(GRAPHIC_SELECT_STYLE_BACKGROUND), 
		THEME->GetPathTo(GRAPHIC_SELECT_STYLE_TOP_EDGE),
		ssprintf("Use %c %c to select, then press NEXT", char(1), char(2) )
		);
	this->AddActor( &m_Menu );

	m_Fade.SetZ( -2 );
	this->AddActor( &m_Fade );

	m_soundChange.Load( THEME->GetPathTo(SOUND_SELECT_STYLE_CHANGE) );
	m_soundSelect.Load( THEME->GetPathTo(SOUND_MENU_START) );


	SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ANNOUNCER_SELECT_STYLE_INTRO) );


	if( !MUSIC->IsPlaying() )
	{
		MUSIC->Load( THEME->GetPathTo(SOUND_MENU_MUSIC) );
        MUSIC->Play( true );
	}

	m_soundChange.PlayRandom();
	AfterChange();
	TweenOnScreen();
	m_Menu.TweenAllOnScreen();
}


ScreenSelectStyle::~ScreenSelectStyle()
{
	LOG->WriteLine( "ScreenSelectStyle::~ScreenSelectStyle()" );
}

void ScreenSelectStyle::DrawPrimitives()
{
	m_Menu.DrawBottomLayer();
	Screen::DrawPrimitives();
	m_Menu.DrawTopLayer();
}

void ScreenSelectStyle::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	LOG->WriteLine( "ScreenSelectStyle::Input()" );

	if( m_Fade.IsClosing() )
		return;

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default input handler
}

void ScreenSelectStyle::HandleScreenMessage( const ScreenMessage SM )
{
	Screen::HandleScreenMessage( SM );

	switch( SM )
	{
	case SM_MenuTimer:
		MenuStart(PLAYER_1);
		break;
	case SM_GoToPrevState:
		MUSIC->Stop();
		SCREENMAN->SetNewScreen( new ScreenTitleMenu );
		break;
	case SM_GoToNextState:
		SCREENMAN->SetNewScreen( new ScreenSelectDifficulty );
		break;
	}
}

void ScreenSelectStyle::BeforeChange()
{
	m_sprIcon[m_iSelection].SetEffectNone();
}

void ScreenSelectStyle::AfterChange()
{
	m_sprIcon[m_iSelection].SetEffectGlowing();

	ThemeElement te;

	te = (ThemeElement)(GRAPHIC_SELECT_STYLE_PREVIEW_0+GetSelectedStyle());
	m_sprPreview.Load( THEME->GetPathTo(te) );
	m_sprPreview.SetZoomX( 0 );
	m_sprPreview.BeginTweening( 0.3f, Actor::TWEEN_BOUNCE_END );
	m_sprPreview.SetTweenZoomX( 1 );

	te = (ThemeElement)(GRAPHIC_SELECT_STYLE_INFO_0+GetSelectedStyle());
	m_sprInfo.Load( THEME->GetPathTo(te) );
	m_sprInfo.SetZoomY( 0 );
	m_sprInfo.BeginTweening( 0.3f, Actor::TWEEN_BOUNCE_END );
	m_sprInfo.SetTweenZoomY( 1 );
}

void ScreenSelectStyle::MenuLeft( const PlayerNumber p )
{
	if( m_iSelection == 0 )		// can't go left any further
		return;

	BeforeChange();
	m_iSelection--;
	m_soundChange.PlayRandom();
	AfterChange();
}


void ScreenSelectStyle::MenuRight( const PlayerNumber p )
{
	if( m_iSelection == m_aPossibleStyles.GetSize()-1 )		// can't go right any further
		return;

	BeforeChange();
	m_iSelection++;
	m_soundChange.PlayRandom();
	AfterChange();
}

void ScreenSelectStyle::MenuStart( const PlayerNumber p )
{
	GAMEMAN->m_sMasterPlayerNumber = p;
	GAMEMAN->m_CurStyle = GetSelectedStyle();

	AnnouncerElement ae;
	switch( GAMEMAN->m_CurStyle )
	{
		case STYLE_DANCE_SINGLE:		ae = ANNOUNCER_SELECT_STYLE_COMMENT_SINGLE;		break;
		case STYLE_DANCE_VERSUS:		ae = ANNOUNCER_SELECT_STYLE_COMMENT_VERSUS;		break;
		case STYLE_DANCE_DOUBLE:		ae = ANNOUNCER_SELECT_STYLE_COMMENT_DOUBLE;		break;
		case STYLE_DANCE_COUPLE:		ae = ANNOUNCER_SELECT_STYLE_COMMENT_COUPLE;		break;
		case STYLE_DANCE_SOLO:			ae = ANNOUNCER_SELECT_STYLE_COMMENT_SOLO;		break;
		case STYLE_DANCE_SOLO_VERSUS:	ae = ANNOUNCER_SELECT_STYLE_COMMENT_VERSUS;		break;
		case STYLE_PUMP_SINGLE:			ae = ANNOUNCER_SELECT_STYLE_COMMENT_SINGLE;		break;
		case STYLE_PUMP_VERSUS:			ae = ANNOUNCER_SELECT_STYLE_COMMENT_VERSUS;		break;
		default:	ASSERT(0);	break;	// invalid Style
	}
	SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ae) );

	m_Menu.TweenTopEdgeOffScreen();

	m_soundSelect.PlayRandom();

	m_Fade.CloseWipingRight( SM_GoToNextState );

	TweenOffScreen();
}

void ScreenSelectStyle::MenuBack( const PlayerNumber p )
{
	MUSIC->Stop();

	m_Menu.TweenAllOffScreen();

	m_Fade.CloseWipingLeft( SM_GoToPrevState );

	TweenOffScreen();
}


void ScreenSelectStyle::TweenOffScreen()
{
	for( int i=0; i<NUM_STYLES; i++ )
	{
		m_sprIcon[i].BeginTweening( MENU_ELEMENTS_TWEEN_TIME );
		m_sprIcon[i].SetTweenZoomY( 0 );
	}

	m_sprExplanation.BeginTweening( MENU_ELEMENTS_TWEEN_TIME );
	m_sprExplanation.SetTweenZoomY( 0 );

	m_sprPreview.BeginTweening( MENU_ELEMENTS_TWEEN_TIME );
	m_sprPreview.SetTweenZoomY( 0 );

	m_sprInfo.BeginTweening( MENU_ELEMENTS_TWEEN_TIME );
	m_sprInfo.SetTweenZoomX( 0 );
}
  

void ScreenSelectStyle::TweenOnScreen() 
{
	float fOriginalZoom;

	for( int i=0; i<NUM_STYLES; i++ )
	{
		fOriginalZoom = m_sprIcon[i].GetZoomY();
		m_sprIcon[i].SetZoomY( 0 );
		m_sprIcon[i].BeginTweening( MENU_ELEMENTS_TWEEN_TIME );
		m_sprIcon[i].SetTweenZoomY( fOriginalZoom );
	}

	fOriginalZoom = m_sprExplanation.GetZoomY();
	m_sprExplanation.SetZoomY( 0 );
	m_sprExplanation.BeginTweening( MENU_ELEMENTS_TWEEN_TIME );
	m_sprExplanation.SetTweenZoomY( fOriginalZoom );

	fOriginalZoom = m_sprPreview.GetZoomY();
	m_sprPreview.SetZoomY( 0 );
	m_sprPreview.BeginTweening( MENU_ELEMENTS_TWEEN_TIME );
	m_sprPreview.SetTweenZoomY( fOriginalZoom );

	fOriginalZoom = m_sprInfo.GetZoomY();
	m_sprInfo.SetZoomY( 0 );
	m_sprInfo.BeginTweening( MENU_ELEMENTS_TWEEN_TIME );
	m_sprInfo.SetTweenZoomY( fOriginalZoom );
}
