#include "global.h"
#include "ActorUtil.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenStage

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenStage.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "GameConstantsAndTypes.h"
#include "BitmapText.h"
#include "SongManager.h"
#include "Sprite.h"
#include "AnnouncerManager.h"
#include "GameState.h"
#include "RageSounds.h"
#include "ThemeManager.h"
#include "LightsManager.h"
#include "song.h"

#define NEXT_SCREEN				THEME->GetMetric (m_sName,"NextScreen")
#define MINIMUM_DELAY			THEME->GetMetricF(m_sName,"MinimumDelay")

const ScreenMessage	SM_PrepScreen		= (ScreenMessage)(SM_User+0);

ScreenStage::ScreenStage( CString sClassName ) : Screen( sClassName )
{
	m_bZeroDeltaOnNextUpdate = false;

	SOUND->StopMusic();

	LIGHTSMAN->SetLightsMode( LIGHTSMODE_STAGE );


	m_Background.LoadFromAniDir( THEME->GetPathToB(m_sName + " "+GAMESTATE->GetStageText()) );
	m_Background.SetDrawOrder( DRAW_ORDER_BEFORE_EVERYTHING );
	this->AddChild( &m_Background );

	m_Overlay.SetName( "Overlay" );
	m_Overlay.LoadFromAniDir( THEME->GetPathToB(m_sName + " overlay"));
	ON_COMMAND( m_Overlay );
	this->AddChild( &m_Overlay );

	m_In.Load( THEME->GetPathToB(m_sName + " in") );
	m_In.StartTransitioning();
	m_In.SetDrawOrder( DRAW_ORDER_TRANSITIONS );
	this->AddChild( &m_In );

	m_Out.Load( THEME->GetPathToB(m_sName + " out") );
	m_Out.SetDrawOrder( DRAW_ORDER_TRANSITIONS );
	this->AddChild( &m_Out );

	m_Back.Load( THEME->GetPathToB("Common back") );
	m_Back.SetDrawOrder( DRAW_ORDER_TRANSITIONS );
	this->AddChild( &m_Back );

	/* Prep the new screen once m_In is complete. */
	this->PostScreenMessage( SM_PrepScreen, m_Background.GetLengthSeconds() );

	FOREACH_PlayerNumber(p)
	{
		Character* pChar = GAMESTATE->m_pCurCharacters[p];
		m_sprCharacterIcon[p].SetName( ssprintf("CharacterIconP%d",p+1) );
		m_sprCharacterIcon[p].Load( pChar->GetStageIconPath() );
		SET_XY_AND_ON_COMMAND( m_sprCharacterIcon[p] );
		this->AddChild( &m_sprCharacterIcon[p] );
	}

	m_SongTitle.SetName( "SongTitle");
	m_Artist.SetName( "Artist" );
	m_SongTitle.LoadFromFont( THEME->GetPathToF("ScreenStage Title") );
	m_Artist.LoadFromFont( THEME->GetPathToF("ScreenStage Artist") );

	this->AddChild( &m_SongTitle );
	this->AddChild( &m_Artist );

	if(GAMESTATE->m_pCurSong != NULL)
	{
		m_SongTitle.SetText( GAMESTATE->m_pCurSong->m_sMainTitle );
		m_Artist.SetText( GAMESTATE->m_pCurSong->m_sArtist );
	}
	else
	{
		m_SongTitle.SetText( "" );
		m_Artist.SetText( "" );
	}

	SET_XY_AND_ON_COMMAND( m_Artist );
	SET_XY_AND_ON_COMMAND( m_SongTitle );

	if ( PREFSMAN->m_bShowBanners )
		if( GAMESTATE->m_pCurSong && GAMESTATE->m_pCurSong->HasBanner() )
		{
			m_Banner.LoadFromSong( GAMESTATE->m_pCurSong );
			this->AddChild( &m_Banner );
		}
	m_Banner.SetName("Banner");
	SET_XY( m_Banner );
	ON_COMMAND(m_Banner);

	SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("stage "+GAMESTATE->GetStageText()) );

	this->SortByDrawOrder();
}

void ScreenStage::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_PrepScreen:
		/* Start fading out in after MINIMUM_DELAY seconds.  Loading the screen
		 * might take longer than that, in which case we'll fade out as early as
		 * we can. */
		this->PostScreenMessage( SM_BeginFadingOut, MINIMUM_DELAY );

		SCREENMAN->PrepNewScreen( NEXT_SCREEN );
		break;

	case SM_BeginFadingOut:
		m_Out.StartTransitioning();
		int p;
		for( p=0; p<NUM_PLAYERS; p++ )
			OFF_COMMAND( m_sprCharacterIcon[p] );
		OFF_COMMAND( m_SongTitle );
		OFF_COMMAND( m_Artist );
		OFF_COMMAND( m_Banner );
		OFF_COMMAND( m_Background );

		this->PostScreenMessage( SM_GoToNextScreen, this->GetTweenTimeLeft() );
		
		break;
	case SM_GoToNextScreen:
		SCREENMAN->LoadPreppedScreen(); /* use prepped */
		break;
	case SM_GoToPrevScreen:
		SCREENMAN->DeletePreppedScreen();
		SCREENMAN->SetNewScreen( "ScreenSelectMusic" );
		break;
	}
}

void ScreenStage::Update( float fDeltaTime )
{
	if( m_bZeroDeltaOnNextUpdate )
	{
		m_bZeroDeltaOnNextUpdate = false;
		fDeltaTime = 0;
	}

	Screen::Update( fDeltaTime );
}

void ScreenStage::MenuBack( PlayerNumber pn )
{
	if( m_In.IsTransitioning() || m_Out.IsTransitioning() || m_Back.IsTransitioning() )
		return;

	this->ClearMessageQueue();
	m_Back.StartTransitioning( SM_GoToPrevScreen );
	SOUND->PlayOnce( THEME->GetPathToS("Common back") );

	/* If a Back is buffered while we're prepping the screen (very common), we'll
	 * get it right after the prep finishes.  However, the next update will contain
	 * the time spent prepping the screen.  Zero the next delta, so the update is
	 * seen. */
	m_bZeroDeltaOnNextUpdate = true;
}
