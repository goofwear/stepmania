#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenOptionsMenu

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenOptionsMenu.h"
#include "RageUtil.h"
#include "RageSounds.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "GameConstantsAndTypes.h"
#include "StepMania.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "AnnouncerManager.h"
#include "GameManager.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "SongManager.h"
#include "UnlockSystem.h"


enum {
	OM_APPEARANCE,
	OM_AUTOGEN,
	OM_BACKGROUND,
	OM_CONFIG_KEY_JOY,
	OM_INPUT,
	OM_GAMEPLAY,
	OM_GRAPHIC,
	OM_MACHINE,
//	OM_SOUND,
	OM_PROFILE,
	OM_RELOAD,
	NUM_OPTIONS_MENU_LINES
};

OptionRow g_OptionsMenuLines[NUM_OPTIONS_MENU_LINES] = {
	OptionRow( "",	true, "Appearance Options" ),
	OptionRow( "",	true, "Autogen Options" ),
	OptionRow( "",	true, "Background Options" ),
	OptionRow( "",	true, "Config Key/Joy Mappings" ),
	OptionRow( "",	true, "Input Options" ),
	OptionRow( "",	true, "Gameplay Options" ),
	OptionRow( "",	true, "Graphic Options" ),
	OptionRow( "",	true, "Machine Options" ),
	OptionRow( "",	true, "Profile Options" ),
//	OptionRow( "",	true, "Sound Options" ),
	OptionRow( "",	true, "Reload Songs/Courses" ),
};

ScreenOptionsMenu::ScreenOptionsMenu() :
	ScreenOptions("ScreenOptionsMenu",false)
{
	LOG->Trace( "ScreenOptionsMenu::ScreenOptionsMenu()" );

	/* We might have been entered from anywhere; make sure all players are enabled. */
	GAMESTATE->m_pCurSong = NULL;
	for( int p=0; p<NUM_PLAYERS; p++ )
		GAMESTATE->m_bSideIsJoined[p] = true;
	GAMESTATE->m_MasterPlayerNumber = PlayerNumber(0);

	Init( 
		INPUTMODE_TOGETHER, 
		g_OptionsMenuLines, 
		NUM_OPTIONS_MENU_LINES,
		true );
	m_Menu.m_MenuTimer.Disable();

	SOUND->PlayMusic( THEME->GetPathToS("ScreenOptionsMenu music") );
}

/* We depend on the SM options navigation for this screen, not arcade. */
void ScreenOptionsMenu::MenuStart( PlayerNumber pn )
{
	StartGoToNextState();
}

void ScreenOptionsMenu::ImportOptions()
{
}

void ScreenOptionsMenu::ExportOptions()
{
}

void ScreenOptionsMenu::GoToPrevState()
{
	SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
}

void ScreenOptionsMenu::GoToNextState()
{
	switch( this->GetCurrentRow() )
	{
		case OM_APPEARANCE:		SCREENMAN->SetNewScreen("ScreenAppearanceOptions");	break;
		case OM_AUTOGEN:		SCREENMAN->SetNewScreen("ScreenAutogenOptions");	break;
		case OM_BACKGROUND:		SCREENMAN->SetNewScreen("ScreenBackgroundOptions");	break;
		case OM_CONFIG_KEY_JOY:	SCREENMAN->SetNewScreen("ScreenMapControllers");	break;
		case OM_GAMEPLAY:		SCREENMAN->SetNewScreen("ScreenGameplayOptions");	break;
		case OM_GRAPHIC:		SCREENMAN->SetNewScreen("ScreenGraphicOptions");	break;
		case OM_INPUT:			SCREENMAN->SetNewScreen("ScreenInputOptions");		break;
		case OM_MACHINE:		SCREENMAN->SetNewScreen("ScreenMachineOptions");	break;
		case OM_PROFILE:		SCREENMAN->SetNewScreen("ScreenProfileOptions");	break;
//		case OM_SOUND:			SCREENMAN->SetNewScreen("ScreenSoundOptions");		break;
		case OM_RELOAD:
			SONGMAN->Reload();
			GAMESTATE->m_pUnlockingSys->UpdateSongs();
			SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
			break;
		default:	// Exit
			SCREENMAN->SetNewScreen("ScreenTitleMenu");
	}
}
