#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenJukeboxMenu

 Desc: The main title screen and menu.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenJukeboxMenu.h"
#include "SongManager.h"
#include "ScreenManager.h"
#include "GameConstantsAndTypes.h"
#include "RageUtil.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "RageLog.h"
#include "GameState.h"
#include "RageSounds.h"
#include "ThemeManager.h"
#include "ScreenJukebox.h"


//
// Defines specific to ScreenJukeboxMenu
//
#define EXPLANATION_X				THEME->GetMetricF("ScreenJukeboxMenu","ExplanationX")
#define EXPLANATION_Y				THEME->GetMetricF("ScreenJukeboxMenu","ExplanationY")
#define EXPLANATION_TEXT			THEME->GetMetric("ScreenJukeboxMenu","ExplanationText")
#define HELP_TEXT					THEME->GetMetric("ScreenJukeboxMenu","HelpText")


ScreenJukeboxMenu::ScreenJukeboxMenu( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenJukeboxMenu::ScreenJukeboxMenu()" );

	GAMESTATE->m_CurStyle = STYLE_INVALID;

	FOREACH_PlayerNumber( pn )
		GAMESTATE->m_bSideIsJoined[pn] = true;

	m_Selector.SetXY( 0, 0 );
//	m_Selector.AllowNewNotes();
	this->AddChild( &m_Selector );


	m_textExplanation.LoadFromFont( THEME->GetPathToF("Common normal") );
	m_textExplanation.SetXY( EXPLANATION_X, EXPLANATION_Y );
	m_textExplanation.SetText( EXPLANATION_TEXT );
	m_textExplanation.SetZoom( 0.7f );
	this->AddChild( &m_textExplanation );

	this->SortByDrawOrder();

	SOUND->PlayMusic( THEME->GetPathToS("ScreenJukeboxMenu music") );
}


ScreenJukeboxMenu::~ScreenJukeboxMenu()
{
	LOG->Trace( "ScreenJukeboxMenu::~ScreenJukeboxMenu()" );
}

void ScreenJukeboxMenu::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

void ScreenJukeboxMenu::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	LOG->Trace( "ScreenJukeboxMenu::Input()" );

	if( IsTransitioning() )
		return;

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );
}

void ScreenJukeboxMenu::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
		break;
	case SM_GoToNextScreen:
		SCREENMAN->SetNewScreen( "ScreenJukebox" );
		break;
	}
}
	
void ScreenJukeboxMenu::MenuUp( PlayerNumber pn )
{
	m_Selector.Up();
}

void ScreenJukeboxMenu::MenuDown( PlayerNumber pn )
{
	m_Selector.Down();
}

void ScreenJukeboxMenu::MenuLeft( PlayerNumber pn, const InputEventType type )
{
	m_Selector.Left();
}

void ScreenJukeboxMenu::MenuRight( PlayerNumber pn, const InputEventType type )
{
	m_Selector.Right();
}

void ScreenJukeboxMenu::MenuStart( PlayerNumber pn )
{
	if( IsTransitioning() )
		return;

	Style style		= m_Selector.GetSelectedStyle();
	CString sGroup	= m_Selector.GetSelectedGroup();
	Difficulty dc	= m_Selector.GetSelectedDifficulty();
	bool bModifiers	= m_Selector.GetSelectedModifiers();

	GAMESTATE->m_CurStyle = style;
	GAMESTATE->m_sPreferredGroup = (sGroup=="ALL MUSIC") ? GROUP_ALL_MUSIC : sGroup;
	FOREACH_PlayerNumber( p )
		GAMESTATE->m_PreferredDifficulty[p] = dc;
	GAMESTATE->m_bJukeboxUsesModifiers = bModifiers;

	if( !ScreenJukebox::SetSong(false) )
	{
		/* No songs are available for the selected style, group, and difficulty. */

		SCREENMAN->PlayInvalidSound();
		SCREENMAN->SystemMessage( "No songs available with these settings" );
		return;
	}


	SOUND->StopMusic();
	SCREENMAN->PlayStartSound();
	StartTransitioning( SM_GoToNextScreen );
}

void ScreenJukeboxMenu::MenuBack( PlayerNumber pn )
{	
	StartTransitioning( SM_GoToPrevScreen );

	SOUND->StopMusic();
}
