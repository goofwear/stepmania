#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenCaution

 Desc: Screen that displays while resources are being loaded.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenStyleSplash.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "RageSounds.h"
#include "ScreenManager.h"
#include "ThemeManager.h"
#include "GameState.h"
#include "GameManager.h"
#include "RageLog.h"

#define NEXT_SCREEN				THEME->GetMetric("ScreenStyleSplash","NextScreen")
#define NONSTOP_SCREEN				THEME->GetMetric("ScreenStyleSplash","NonstopScreen")
#define ONI_SCREEN				THEME->GetMetric("ScreenStyleSplash","OniScreen")

const ScreenMessage SM_DoneOpening		= ScreenMessage(SM_User-7);
const ScreenMessage SM_StartClosing		= ScreenMessage(SM_User-8);


ScreenStyleSplash::ScreenStyleSplash( CString sName ) : ScreenWithMenuElements( sName )
{
	SOUND->StopMusic();

	CString sGameName = GAMESTATE->GetCurrentGameDef()->m_szName;	
	CString sStyleName = GAMESTATE->GetCurrentStyleDef()->m_szName;

	LOG->Trace( ssprintf("ScreenStyleSplash: Displaying Splash for style: %s", sStyleName.c_str()));

	int iDifficulty = GAMESTATE->m_PreferredDifficulty[0];
	if( GAMESTATE->m_bSideIsJoined[PLAYER_1] )
	{
		iDifficulty = GAMESTATE->m_PreferredDifficulty[PLAYER_1];
	}
	else
	{
		iDifficulty = GAMESTATE->m_PreferredDifficulty[PLAYER_2];
	}

	m_Background.LoadFromAniDir( THEME->GetPathToB(ssprintf("ScreenStyleSplash-%s-%s-%d",sGameName.c_str(),sStyleName.c_str(),iDifficulty) ) );
	this->AddChild( &m_Background );
	
	this->PostScreenMessage( SM_StartClosing, 2 );
}


void ScreenStyleSplash::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( IsTransitioning() )
		return;

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );
}


void ScreenStyleSplash::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_StartClosing:
		if( !IsTransitioning() )
			StartTransitioning( SM_GoToNextScreen );
		break;
	case SM_DoneOpening:
	
		break;
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
		break;
	case SM_GoToNextScreen:
		SCREENMAN->SetNewScreen( NEXT_SCREEN );
		break;
	}
}

void ScreenStyleSplash::MenuStart( PlayerNumber pn )
{
	StartTransitioning( SM_GoToNextScreen );
}

void ScreenStyleSplash::MenuBack( PlayerNumber pn )
{
	if(IsTransitioning())
		return;
	this->ClearMessageQueue();
	Back( SM_GoToPrevScreen );
	SOUND->PlayOnce( THEME->GetPathToS("menu back") );
}

void ScreenStyleSplash::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

