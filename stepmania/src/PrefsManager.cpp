#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: PrefsManager

 Desc: See Header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "PrefsManager.h"
#include "IniFile.h"
#include "GameManager.h"
#include "GameState.h"


PrefsManager*	PREFSMAN = NULL;	// global and accessable from anywhere in our program


PrefsManager::PrefsManager()
{
	m_bWindowed = false;
	m_iDisplayResolution = 640;
	m_iTextureResolution = 512;
	m_iRefreshRate = 60;
	m_bIgnoreJoyAxes = false;
	m_bShowFPS = false;
	m_BackgroundMode = BGMODE_ANIMATIONS;
	m_fBGBrightness = 0.8f;
	m_bMenuTimer = true;
	m_bEventMode = false;
	m_iNumArcadeStages = 3;
	m_bAutoPlay = false;
	m_fJudgeWindow = 0.18f;

	ReadGlobalPrefsFromDisk( true );
}

PrefsManager::~PrefsManager()
{
	SaveGlobalPrefsToDisk();
	SaveGamePrefsToDisk();
}

void PrefsManager::ReadGlobalPrefsFromDisk( bool bSwitchToLastPlayedGame )
{
	IniFile ini;
	ini.SetPath( "StepMania.ini" );
	if( !ini.ReadFile() )
		return;		// could not read config file, load nothing

	ini.GetValueB( "Options", "Windowed",			m_bWindowed );
	ini.GetValueI( "Options", "DisplayResolution",	m_iDisplayResolution );
	ini.GetValueI( "Options", "TextureResolution",	m_iTextureResolution );
	ini.GetValueI( "Options", "RefreshRate",		m_iRefreshRate );
	ini.GetValueB( "Options", "IgnoreJoyAxes",		m_bIgnoreJoyAxes );
	ini.GetValueB( "Options", "ShowFPS",			m_bShowFPS );
	ini.GetValueI( "Options", "BackgroundMode",		(int&)m_BackgroundMode );
	ini.GetValueF( "Options", "BGBrightness",		m_fBGBrightness );
	ini.GetValueB( "Options", "MenuTimer",			m_bMenuTimer );
	ini.GetValueB( "Options", "EventMode",			m_bEventMode );
	ini.GetValueI( "Options", "NumArcadeStages",	m_iNumArcadeStages );
	ini.GetValueB( "Options", "AutoPlay",			m_bAutoPlay );
	ini.GetValueF( "Options", "JudgeWindow",		m_fJudgeWindow );

	CString sAdditionalSongFolders;
	ini.GetValue( "Options", "SongFolders", sAdditionalSongFolders );
	split( sAdditionalSongFolders, ",", m_asSongFolders, true );

	if( bSwitchToLastPlayedGame )
	{
		Game game;
		if( ini.GetValueI("Options", "Game", (int&)game) )
			GAMESTATE->SwitchGame( game );
	}
}


void PrefsManager::SaveGlobalPrefsToDisk()
{
	IniFile ini;
	ini.SetPath( "StepMania.ini" );

	ini.SetValueB( "Options", "Windowed",			m_bWindowed );
	ini.SetValueI( "Options", "DisplayResolution",	m_iDisplayResolution );
	ini.SetValueI( "Options", "TextureResolution",	m_iTextureResolution );
	ini.SetValueI( "Options", "RefreshRate",		m_iRefreshRate );
	ini.SetValueB( "Options", "IgnoreJoyAxes",		m_bIgnoreJoyAxes );
	ini.SetValueB( "Options", "ShowFPS",			m_bShowFPS );
	ini.SetValueI( "Options", "BackgroundMode",		m_BackgroundMode);
	ini.SetValueF( "Options", "BGBrightness",		m_fBGBrightness );
	ini.SetValueB( "Options", "EventMode",			m_bEventMode );
	ini.SetValueB( "Options", "MenuTimer",			m_bMenuTimer );
	ini.SetValueI( "Options", "NumArcadeStages",	m_iNumArcadeStages );
	ini.SetValueB( "Options", "AutoPlay",			m_bAutoPlay );
	ini.SetValueF( "Options", "JudgeWindow",		m_fJudgeWindow );

	ini.SetValue( "Options", "SongFolders", join(",", m_asSongFolders) );

	ini.SetValueI( "Options", "Game",	GAMESTATE->GetCurGame() );

	ini.WriteFile();
}

void PrefsManager::ReadGamePrefsFromDisk()
{
	if( !GAMESTATE )
		return;

	CString sGameName = GAMESTATE->GetCurrentGameDef()->m_szName;
	IniFile ini;
	ini.SetPath( sGameName + "Prefs.ini" );
	if( !ini.ReadFile() )
		return;		// could not read config file, load nothing

	CString sAnnouncer, sTheme, sNoteSkin;

	ini.GetValue( "Options", "Announcer",			sAnnouncer );
	ini.GetValue( "Options", "Theme",				sTheme );
	ini.GetValue( "Options", "NoteSkin",			sNoteSkin );


	ANNOUNCER->SwitchAnnouncer( sAnnouncer );
	THEME->SwitchTheme( sTheme );
	GAMEMAN->SwitchNoteSkin( sNoteSkin );
}

void PrefsManager::SaveGamePrefsToDisk()
{
	if( !GAMESTATE )
		return;

	CString sGameName = GAMESTATE->GetCurrentGameDef()->m_szName;
	IniFile ini;
	ini.SetPath( sGameName + "Prefs.ini" );

	ini.SetValue( "Options", "Announcer",			ANNOUNCER->GetCurAnnouncerName() );
	ini.SetValue( "Options", "Theme",				THEME->GetCurThemeName() );
	ini.SetValue( "Options", "NoteSkin",			GAMEMAN->GetCurNoteSkin() );

	ini.WriteFile();
}
