#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenCaution

 Desc: Screen that displays while resources are being loaded.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenCaution.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "ScreenManager.h"
#include "AnnouncerManager.h"
#include "GameState.h"
#include "RageSoundManager.h"
#include "ThemeManager.h"


#define NEXT_SCREEN				THEME->GetMetric("ScreenCaution","NextScreen")


const ScreenMessage SM_DoneOpening		= ScreenMessage(SM_User-7);
const ScreenMessage SM_StartClosing		= ScreenMessage(SM_User-8);


ScreenCaution::ScreenCaution()
{
	GAMESTATE->m_bPlayersCanJoin = true;

	if(!PREFSMAN->m_bShowDontDie)
	{
		this->PostScreenMessage( SM_GoToNextScreen, 0.f );
		return;
	}

	m_Background.LoadFromAniDir( THEME->GetPathTo("BGAnimations","ScreenCaution background") );
	this->AddChild( &m_Background );
	
	m_In.Load( THEME->GetPathTo("BGAnimations","ScreenCaution in") );
	m_In.StartTransitioning( SM_DoneOpening );
	this->AddChild( &m_In );

	m_Out.Load( THEME->GetPathTo("BGAnimations","ScreenCaution out") );
	this->AddChild( &m_Out );

	m_Back.Load( THEME->GetPathTo("BGAnimations","Common back") );
	this->AddChild( &m_Back );

	this->PostScreenMessage( SM_StartClosing, 3 );

	SOUNDMAN->PlayMusic( THEME->GetPathTo("Sounds","ScreenCaution music") );
}


void ScreenCaution::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( Screen::JoinInput( DeviceI, type, GameI, MenuI, StyleI ) )
		return;	// handled

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );
}


void ScreenCaution::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_StartClosing:
		if( !m_Out.IsTransitioning() )
			m_Out.StartTransitioning( SM_GoToNextScreen );
		break;
	case SM_DoneOpening:
		SOUNDMAN->PlayOnceFromDir( ANNOUNCER->GetPathTo("caution") );
		break;
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
		break;
	case SM_GoToNextScreen:
		SCREENMAN->SetNewScreen( NEXT_SCREEN );
		break;
	}
}

void ScreenCaution::MenuStart( PlayerNumber pn )
{
	if( !m_Out.IsTransitioning() )
		m_Out.StartTransitioning( SM_GoToNextScreen );
}

void ScreenCaution::MenuBack( PlayerNumber pn )
{
	if( m_In.IsTransitioning() || m_Out.IsTransitioning() || m_Back.IsTransitioning() )
		return;
	this->ClearMessageQueue();
	m_Back.StartTransitioning( SM_GoToPrevScreen );
	SOUNDMAN->PlayOnce( THEME->GetPathTo("Sounds","Common back") );
}

