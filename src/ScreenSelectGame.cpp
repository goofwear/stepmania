#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 File: ScreenSelectGame.cpp

 Desc: Select a song.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
-----------------------------------------------------------------------------
*/

#include "ScreenSelectGame.h"
#include <assert.h>
#include "RageTextureManager.h"
#include "RageUtil.h"
#include "RageMusic.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "ScreenOptions.h"
#include "ScreenTitleMenu.h"
#include "GameConstantsAndTypes.h"
#include "StepMania.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "GameManager.h"
#include <string.h>
#include "GameState.h"
#include "InputMapper.h"


enum {
	SG_GAME = 0,
	NUM_SELECT_GAME_LINES
};


OptionRowData g_SelectGameLines[NUM_SELECT_GAME_LINES] = 
{
	"Game",	0, { "" }
};


ScreenSelectGame::ScreenSelectGame() :
	ScreenOptions(
		THEME->GetPathTo("BGAnimations","select game"),
		THEME->GetPathTo("Graphics","select game page"),
		THEME->GetPathTo("Graphics","select game top edge")
		)
{
	LOG->Trace( "ScreenSelectGame::ScreenSelectGame()" );


	//
	// populate g_SelectGameLines
	//
//	CStringArray arrayGameNames;
//	GAMEMAN->GetGameNames( arrayGameNames );
	CArray<Game,Game> aGames;
	GAMEMAN->GetEnabledGames( aGames );
	for( int i=0; i<aGames.GetSize(); i++ )
	{
		Game game = aGames[i];
		CString sGameName = GAMEMAN->GetGameDefForGame(game)->m_szName;
		strcpy( g_SelectGameLines[0].szOptionsText[i], sGameName );
	}
	g_SelectGameLines[0].iNumOptions = i;


	Init( 
		INPUTMODE_BOTH, 
		g_SelectGameLines, 
		NUM_SELECT_GAME_LINES,
		false );

	MUSIC->LoadAndPlayIfNotAlready( THEME->GetPathTo("Sounds","select game music") );
}

void ScreenSelectGame::ImportOptions()
{
	m_iSelectedOption[0][SG_GAME] = GAMESTATE->m_CurGame;
}

void ScreenSelectGame::ExportOptions()
{
	LOG->Trace("ScreenSelectGame::ExportOptions()");

	INPUTMAPPER->SaveMappingsToDisk();	// save mappings before switching the game
	PREFSMAN->SaveGamePrefsToDisk();

	// Switch the current style to the frist style of the selected game
	int iSelection = m_iSelectedOption[0][SG_GAME];

	CArray<Game,Game> aGames;
	GAMEMAN->GetEnabledGames( aGames );
	Game game = aGames[iSelection];

	GAMESTATE->m_CurGame = game;
	PREFSMAN->ReadGamePrefsFromDisk();
	INPUTMAPPER->ReadMappingsFromDisk();
}

void ScreenSelectGame::GoToPrevState()
{
	SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
}

void ScreenSelectGame::GoToNextState()
{
	SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
}


