#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H
/*
-----------------------------------------------------------------------------
 Class: GameManager

 Desc: Manages GameDefs (which define different games, like "dance" and "pump")
	and StyleDefs (which define different games, like "single" and "couple")

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/
#include "GameDef.h"
#include "StyleDef.h"
#include "Style.h"
#include "Game.h"
class IniFile;


class GameManager
{
public:
	GameManager();
	~GameManager();

	GameDef*	GetGameDefForGame( Game g );
	static const StyleDef*	GetStyleDefForStyle( Style s );

	void	GetStylesForGame( Game game, vector<Style>& aStylesAddTo, bool editor=false );
	void	GetNotesTypesForGame( Game game, vector<StepsType>& aNotesTypeAddTo );
	Style	GetEditorStyleForNotesType( StepsType st );

	void GetEnabledGames( vector<Game>& aGamesOut );
	bool IsGameEnabled( Game game );

	static int NotesTypeToNumTracks( StepsType st );
	static StepsType StringToNotesType( CString sNotesType );
	static CString NotesTypeToString( StepsType st );
	static CString NotesTypeToThemedString( StepsType st );
	static Game StringToGameType( CString sGameType );
	Style GameAndStringToStyle( Game game, CString sStyle );
	static CString StyleToThemedString( Style s );

protected:
};

extern GameManager*	GAMEMAN;	// global and accessable from anywhere in our program

#endif
