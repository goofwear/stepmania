#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenSelectGroup

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


#include "ScreenSelectGroup.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "RageLog.h"
#include "GameConstantsAndTypes.h"
#include "SongManager.h"
#include "AnnouncerManager.h"
#include "GameState.h"
#include "RageSounds.h"
#include "ThemeManager.h"
#include <map>
#include "ActorUtil.h"
#include "GameState.h"
#include "UnlockSystem.h"
#include "MenuTimer.h"

#define BANNER_WIDTH					THEME->GetMetricF("ScreenSelectGroup","BannerWidth")
#define BANNER_HEIGHT					THEME->GetMetricF("ScreenSelectGroup","BannerHeight")
#define SLEEP_AFTER_TWEEN_OFF_SECONDS	THEME->GetMetricF("ScreenSelectGroup","SleepAfterTweenOffSeconds")
#define NEXT_SCREEN						THEME->GetMetric ("ScreenSelectGroup","NextScreen")


ScreenSelectGroup::ScreenSelectGroup( CString sClassName ) : Screen( sClassName )
{	
	LOG->Trace( "ScreenSelectGroup::ScreenSelectGroup()" );	

	if( !PREFSMAN->m_bShowSelectGroup )
	{
		GAMESTATE->m_sPreferredGroup = GROUP_ALL_MUSIC;
		HandleScreenMessage( SM_GoToNextScreen );
		return;
	}



	m_Menu.Load( "ScreenSelectGroup" );
	this->AddChild( &m_Menu );


	unsigned i;
	int j;
	
	vector<Song*> aAllSongs;
	SONGMAN->GetSongs( aAllSongs );

	// Filter out Songs that can't be played by the current Style
	for( j=aAllSongs.size()-1; j>=0; j-- )		// foreach Song, back to front
	{
		bool DisplaySong = aAllSongs[j]->NormallyDisplayed();
		
		if( UNLOCKSYS->SongIsLocked( aAllSongs[j] ) )
			DisplaySong = false;
		
		if( aAllSongs[j]->SongCompleteForStyle(GAMESTATE->GetCurrentStyleDef()) && 
			DisplaySong )
			continue;

		aAllSongs.erase( aAllSongs.begin()+j, aAllSongs.begin()+j+1 );
	}

	// Add all group names to a map.
	map<CString, CString> mapGroupNames;
	for( i=0; i<aAllSongs.size(); i++ )
	{
		const CString& sGroupName = aAllSongs[i]->m_sGroupName;
		mapGroupNames[ sGroupName ] = "";	// group name maps to nothing
	}

	// copy group names into a vector
	vector<CString> asGroupNames;
	asGroupNames.push_back( "ALL MUSIC" );	// special group
	for( map<CString, CString>::const_iterator iter = mapGroupNames.begin(); iter != mapGroupNames.end(); ++iter )
		asGroupNames.push_back( iter->first );

	// Add songs to the MusicList.
	m_MusicList.Load();
	for( unsigned g=0; g < asGroupNames.size(); g++ ) /* for each group */
	{
		vector<Song*> aSongsInGroup;
		/* find all songs */
		for( i=0; i<aAllSongs.size(); i++ )		// foreach Song
		{
			/* group 0 gets all songs */
			if( g != 0 && aAllSongs[i]->m_sGroupName != asGroupNames[g] )
				continue;

			aSongsInGroup.push_back( aAllSongs[i] );
		}

		SortSongPointerArrayByTitle( aSongsInGroup );

		m_MusicList.AddGroup();
		m_MusicList.AddSongsToGroup(aSongsInGroup);
	}

	m_bChosen = false;

	m_sprExplanation.SetName( "Explanation" );
	m_sprExplanation.Load( THEME->GetPathToG("ScreenSelectGroup explanation") );
	this->AddChild( &m_sprExplanation );

	// these guys get loaded SetSong and TweenToSong
	m_Banner.SetName( "Banner" );
	m_Banner.ScaleToClipped( BANNER_WIDTH, BANNER_HEIGHT );
	this->AddChild( &m_Banner );

	m_sprFrame.SetName( "Frame" );
	m_sprFrame.Load( THEME->GetPathToG("ScreenSelectGroup frame") );
	this->AddChild( &m_sprFrame );

	m_textNumber.SetName( "Number" );
	m_textNumber.LoadFromNumbers( THEME->GetPathToN("ScreenSelectGroup num songs") );
	this->AddChild( &m_textNumber );
	
	m_sprContents.SetName( "Contents" );
	m_sprContents.Load( THEME->GetPathToG("ScreenSelectGroup contents") );
	this->AddChild( &m_sprContents );

	m_MusicList.SetName( "MusicList" );
	this->AddChild( &m_MusicList );
	
	m_GroupList.SetName( "GroupList" );
	m_GroupList.Load( asGroupNames );
	this->AddChild( &m_GroupList );


	m_soundChange.Load( THEME->GetPathToS("ScreenSelectGroup change") );
	m_soundSelect.Load( THEME->GetPathToS("Common start") );

	SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("select group intro") );

	SOUND->PlayMusic( THEME->GetPathToS("ScreenSelectGroup music") );

	AfterChange();
	TweenOnScreen();
	m_GroupList.SetSelection(0);
}


ScreenSelectGroup::~ScreenSelectGroup()
{
	LOG->Trace( "ScreenSelectGroup::~ScreenSelectGroup()" );

}


void ScreenSelectGroup::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	LOG->Trace( "ScreenSelectGroup::Input()" );

	if( m_Menu.IsTransitioning() )
		return;

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default input handler
}

void ScreenSelectGroup::DrawPrimitives()
{
	m_Menu.DrawBottomLayer();
	Screen::DrawPrimitives();
	m_Menu.DrawTopLayer();
}

void ScreenSelectGroup::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_BeginFadingOut:
		m_Menu.StartTransitioning( SM_GoToNextScreen );
		break;
	case SM_MenuTimer:
		MenuStart(PLAYER_1);
		break;
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
		break;
	case SM_GoToNextScreen:
		SCREENMAN->SetNewScreen( NEXT_SCREEN );
		break;
	}
}

void ScreenSelectGroup::AfterChange()
{
	int sel = m_GroupList.GetSelection();
	m_MusicList.SetGroupNo(sel);

	CString sSelectedGroupName = m_GroupList.GetSelectionName();
	if( sSelectedGroupName == "ALL MUSIC" )
		m_Banner.LoadAllMusic();
	else 
		m_Banner.LoadFromGroup( sSelectedGroupName );

	const int iNumSongs = m_MusicList.GetNumSongs();
	m_textNumber.SetText( ssprintf("%d", iNumSongs) );
}


void ScreenSelectGroup::MenuLeft( PlayerNumber pn )
{
	MenuUp( pn );
}

void ScreenSelectGroup::MenuRight( PlayerNumber pn )
{
	MenuDown( pn );
}

void ScreenSelectGroup::MenuUp( PlayerNumber pn )
{
	if( m_bChosen )
		return;

	m_GroupList.Up();

	AfterChange();

	m_soundChange.PlayRandom();
}


void ScreenSelectGroup::MenuDown( PlayerNumber pn )
{
	if( m_bChosen )
		return;

	m_GroupList.Down();
	
	AfterChange();

	m_soundChange.PlayRandom();
}

void ScreenSelectGroup::MenuStart( PlayerNumber pn )
{
	if( m_bChosen )
		return;

	m_soundSelect.PlayRandom();
	m_Menu.m_MenuTimer->Stop();
	m_bChosen = true;

	GAMESTATE->m_pCurSong = NULL;
	GAMESTATE->m_sPreferredGroup = (m_GroupList.GetSelectionName()=="ALL MUSIC" ? GROUP_ALL_MUSIC : m_GroupList.GetSelectionName() );

	if( GAMESTATE->m_sPreferredGroup == GROUP_ALL_MUSIC )
        SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("select group comment all music") );
	else
        SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("select group comment general") );


	TweenOffScreen();
	this->PostScreenMessage( SM_BeginFadingOut, SLEEP_AFTER_TWEEN_OFF_SECONDS );
}

void ScreenSelectGroup::MenuBack( PlayerNumber pn )
{
	m_Menu.Back( SM_GoToPrevScreen );
}

void ScreenSelectGroup::TweenOffScreen()
{
	OFF_COMMAND( m_sprExplanation );
	OFF_COMMAND( m_sprFrame );
	OFF_COMMAND( m_Banner );
	OFF_COMMAND( m_textNumber );
	OFF_COMMAND( m_sprContents );
	OFF_COMMAND( m_MusicList );
	OFF_COMMAND( m_GroupList );
	m_MusicList.TweenOffScreen();
	m_GroupList.TweenOffScreen();
}

void ScreenSelectGroup::TweenOnScreen() 
{
	SET_XY_AND_ON_COMMAND( m_sprExplanation );
	SET_XY_AND_ON_COMMAND( m_sprFrame );
	SET_XY_AND_ON_COMMAND( m_Banner );
	SET_XY_AND_ON_COMMAND( m_textNumber );
	SET_XY_AND_ON_COMMAND( m_sprContents );
	SET_XY_AND_ON_COMMAND( m_MusicList );
	SET_XY_AND_ON_COMMAND( m_GroupList );
	m_MusicList.TweenOnScreen();
	m_GroupList.TweenOnScreen();
}
