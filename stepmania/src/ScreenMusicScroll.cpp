#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenMusicScroll

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


#include "ScreenMusicScroll.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "RageLog.h"
#include "GameConstantsAndTypes.h"
#include "SongManager.h"
#include "RageSounds.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "AnnouncerManager.h"


#define SCROLL_DELAY		THEME->GetMetricF("ScreenMusicScroll","ScrollDelay")
#define SCROLL_SPEED		THEME->GetMetricF("ScreenMusicScroll","ScrollSpeed")
#define TEXT_ZOOM			THEME->GetMetricF("ScreenMusicScroll","TextZoom")
#define NEXT_SCREEN			THEME->GetMetric("ScreenMusicScroll","NextScreen")


const CString CREDIT_LINES[] = 
{
""
};
const unsigned NUM_CREDIT_LINES = sizeof(CREDIT_LINES) / sizeof(CString);


ScreenMusicScroll::ScreenMusicScroll( CString sClassName ) : Screen( sClassName )
{
	LOG->Trace( "ScreenMusicScroll::ScreenMusicScroll()" );

	unsigned i;

	GAMESTATE->Reset();		// so that credits message for both players will show

	m_Background.LoadFromAniDir( THEME->GetPathToB("ScreenMusicScroll background") );
	this->AddChild( &m_Background );


	vector<Song*> arraySongs;
	SONGMAN->GetSongs( arraySongs );
	SortSongPointerArrayByTitle( arraySongs );
	
	for( i=0; i < arraySongs.size(); i++ )
	{
		BitmapText *bt = new BitmapText;
		m_textLines.push_back(bt);
		
		Song* pSong = arraySongs[i];
		bt->LoadFromFont( THEME->GetPathToF("ScreenMusicScroll titles") );
		bt->SetText( pSong->GetFullDisplayTitle(), pSong->GetFullTranslitTitle() );
		bt->SetDiffuse( SONGMAN->GetSongColor(pSong) );
		bt->SetZoom( TEXT_ZOOM );
	}

//	for( i=0; i<min(NUM_CREDIT_LINES, MAX_CREDIT_LINES); i++ )
//	{
//		m_textLines[m_iNumLines].LoadFromFont( THEME->GetPathToF("ScreenMusicScroll titles") );
//		m_textLines[m_iNumLines].SetText( CREDIT_LINES[i] );
//		m_textLines[m_iNumLines].SetZoom( TEXT_ZOOM );
//
//		m_iNumLines++;
//	}

	for( i=0; i<m_textLines.size(); i++ )
	{
		m_textLines[i]->SetXY( CENTER_X, SCREEN_BOTTOM + 40 );
		m_textLines[i]->BeginTweening( SCROLL_DELAY * i );
		m_textLines[i]->BeginTweening( 2.0f*SCROLL_SPEED );
		m_textLines[i]->SetXY( CENTER_X, SCREEN_TOP - 40 );	
	}
	
	this->PostScreenMessage( SM_BeginFadingOut, 0.2f * i + 3.0f );

	m_In.Load( THEME->GetPathToB("ScreenMusicScroll in") );
//	this->AddChild( &m_In );	// draw and update manually
	m_In.StartTransitioning();

	m_Out.Load( THEME->GetPathToB("ScreenMusicScroll out") );
//	this->AddChild( &m_Out );	// draw and update manually

	SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("music scroll") );

	SOUND->PlayMusic( THEME->GetPathToS("ScreenMusicScroll music") );
}


void ScreenMusicScroll::Update( float fDeltaTime )
{
	for( unsigned i=0; i<m_textLines.size(); i++ )
		m_textLines[i]->Update( fDeltaTime );

	m_In.Update( fDeltaTime );
	m_Out.Update( fDeltaTime );

	Screen::Update( fDeltaTime );
}	


void ScreenMusicScroll::DrawPrimitives()
{
	Screen::DrawPrimitives();

	for( unsigned i=0; i<m_textLines.size(); i++ )
	{
		if( m_textLines[i]->GetY() > SCREEN_TOP-20  &&
			m_textLines[i]->GetY() < SCREEN_BOTTOM+20 )
			m_textLines[i]->Draw();
	}

	m_In.Draw();	// render it again so it shows over the text
	m_Out.Draw();	// render it again so it shows over the text
}	


void ScreenMusicScroll::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
//	LOG->Trace( "ScreenMusicScroll::Input()" );

	if( m_In.IsTransitioning() || m_Out.IsTransitioning() )
		return;

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default input handler
}

void ScreenMusicScroll::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_BeginFadingOut:
		if( !m_Out.IsTransitioning() )
			m_Out.StartTransitioning( SM_GoToNextScreen );
		break;
	case SM_GoToNextScreen:
		SONGMAN->SaveMachineScoresToDisk();
		SCREENMAN->SetNewScreen( NEXT_SCREEN );
		break;
	}
}

void ScreenMusicScroll::MenuStart( PlayerNumber pn )
{
	this->PostScreenMessage( SM_BeginFadingOut, 0 );
}

void ScreenMusicScroll::MenuBack( PlayerNumber pn )
{
	this->PostScreenMessage( SM_BeginFadingOut, 0 );
}

