#include "stdafx.h"
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
#include "ScreenSelectMusic.h"
#include "ScreenTitleMenu.h"
#include "GameManager.h"
#include "RageLog.h"
#include "GameConstantsAndTypes.h"
#include "SongManager.h"
#include "AnnouncerManager.h"
#include "GameState.h"


#define FRAME_X				THEME->GetMetricF("SelectGroup","FrameX")
#define FRAME_Y				THEME->GetMetricF("SelectGroup","FrameY")
#define BANNER_X			THEME->GetMetricF("SelectGroup","BannerX")
#define BANNER_Y			THEME->GetMetricF("SelectGroup","BannerY")
#define BANNER_WIDTH		THEME->GetMetricF("SelectGroup","BannerWidth")
#define BANNER_HEIGHT		THEME->GetMetricF("SelectGroup","BannerHeight")
#define NUMBER_X			THEME->GetMetricF("SelectGroup","NumberX")
#define NUMBER_Y			THEME->GetMetricF("SelectGroup","NumberY")
#define EXPLANATION_X		THEME->GetMetricF("SelectGroup","ExplanationX")
#define EXPLANATION_Y		THEME->GetMetricF("SelectGroup","ExplanationY")
#define CONTENTS_X			THEME->GetMetricF("SelectGroup","ContentsX")
#define CONTENTS_Y			THEME->GetMetricF("SelectGroup","ContentsY")

#define HELP_TEXT			THEME->GetMetric("SelectGroup","HelpText")
#define TIMER_SECONDS		THEME->GetMetricI("SelectGroup","TimerSeconds")


const ScreenMessage SM_GoToPrevState		=	ScreenMessage(SM_User + 1);
const ScreenMessage SM_GoToNextState		=	ScreenMessage(SM_User + 2);
const ScreenMessage SM_StartFadingOut		=	ScreenMessage(SM_User + 3);


ScreenSelectGroup::ScreenSelectGroup()
{
	
	LOG->Trace( "ScreenSelectGroup::ScreenSelectGroup()" );	


	int i;


	// The new process by which the group and song lists are formed
	// is bizarre and complex but yields the end result that songs
	// and groups that do not contain any steps for the current
	// style (such as solo) are omitted. Bear with me!
	// -- dro kulix

	// Chris:
	// This is excellent!  I'm going to move the filtering of songs 
	// that can't be played by current style to be the first action.
	// This will simply the code a bit, and fix a weird case that 
	// causes a crash when there are duplicate song names.
	
	CArray<Song*, Song*> aAllSongs;
	aAllSongs.Copy( SONGMAN->m_pSongs );

	// Filter out Songs that can't be played by the current Style
	{
		NotesType nt = GAMESTATE->GetCurrentStyleDef()->m_NotesType;
		for( i=aAllSongs.GetSize()-1; i>=0; i-- )		// foreach Song, back to front
		{
			if( !aAllSongs[i]->SongHasNotesType(nt) )
				aAllSongs.RemoveAt( i );
		}
	}

	// Get group names by thowing them into a hash
	CMapStringToString mapGroupToNothing;
	for( i=0; i<aAllSongs.GetSize(); i++ )		// foreach Song
		mapGroupToNothing[ aAllSongs[i]->m_sGroupName ] = "";			

	CStringArray asGroupNames;
	
	// Read group names back out into asGroupNames
	for( POSITION pos = mapGroupToNothing.GetStartPosition(); pos != NULL; )
	{
		CString sGroupName, sValue;
		mapGroupToNothing.GetNextAssoc( pos, sGroupName, sValue );
		asGroupNames.Add( sGroupName );
	}
	SortCStringArray( asGroupNames, true );
	asGroupNames.InsertAt(0, "ALL MUSIC" );

	// Add songs to the MusicList.
	for( int j=0; j < asGroupNames.GetSize(); j++ ) /* for each group */
	{
		CArray<Song*,Song*> aSongsInGroup;
		/* find all songs */
		for( i=0; i<aAllSongs.GetSize(); i++ )		// foreach Song
		{
			/* group 0 gets all songs */
			if( j != 0 && aAllSongs[i]->m_sGroupName != asGroupNames[j] )
				continue;

			aSongsInGroup.Add( aAllSongs[i] );
		}

		SortSongPointerArrayByTitle( aSongsInGroup );

		m_MusicList.AddGroup();
		m_MusicList.AddSongsToGroup(aSongsInGroup);
	}

	m_bChosen = false;

	m_Menu.Load(
		THEME->GetPathTo("Graphics","select group background") , 
		THEME->GetPathTo("Graphics","select group top edge"),
		HELP_TEXT,
		false, true, TIMER_SECONDS
		);
	this->AddSubActor( &m_Menu );

	m_sprExplanation.Load( THEME->GetPathTo("Graphics","select group explanation") );
	m_sprExplanation.SetXY( EXPLANATION_X, EXPLANATION_Y );
	this->AddSubActor( &m_sprExplanation );

	// these guys get loaded SetSong and TweenToSong
	m_Banner.SetXY( BANNER_X, BANNER_Y );
	m_Banner.SetCroppedSize( BANNER_WIDTH, BANNER_HEIGHT );
	this->AddSubActor( &m_Banner );

	m_sprFrame.Load( THEME->GetPathTo("Graphics","select group info frame") );
	m_sprFrame.SetXY( FRAME_X, FRAME_Y );
	this->AddSubActor( &m_sprFrame );

	m_textNumber.LoadFromFont( THEME->GetPathTo("Fonts","header1") );
	m_textNumber.SetXY( NUMBER_X, NUMBER_Y );
	m_textNumber.SetHorizAlign( Actor::align_right );
	m_textNumber.TurnShadowOff();
	this->AddSubActor( &m_textNumber );
	
	m_sprContents.Load( THEME->GetPathTo("Graphics","select group contents header") );
	m_sprContents.SetXY( CONTENTS_X, CONTENTS_Y );
	this->AddSubActor( &m_sprContents );

	this->AddSubActor( &m_MusicList );
	
	for( i=0; i < asGroupNames.GetSize(); ++i )
		m_GroupList.AddGroup( asGroupNames[i] );
	m_GroupList.DoneAddingGroups();
	this->AddSubActor( &m_GroupList );


	m_soundChange.Load( THEME->GetPathTo("Sounds","select group change") );
	m_soundSelect.Load( THEME->GetPathTo("Sounds","menu start") );

	SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ANNOUNCER_SELECT_GROUP_INTRO) );


	if( !MUSIC->IsPlaying()  ||  MUSIC->GetLoadedFilePath() != THEME->GetPathTo("Sounds","select group music") )
	{
		MUSIC->Load( THEME->GetPathTo("Sounds","select group music") );
		MUSIC->Play( true );
	}

	m_Menu.TweenOnScreenFromMenu( SM_None );
	TweenOnScreen();
	AfterChange();
	m_GroupList.SetSelection(0);
}


ScreenSelectGroup::~ScreenSelectGroup()
{
	LOG->Trace( "ScreenSelectGroup::~ScreenSelectGroup()" );

}


void ScreenSelectGroup::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	LOG->Trace( "ScreenSelectGroup::Input()" );

	if( m_Menu.IsClosing()  ||  m_bChosen )
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
	case SM_MenuTimer:
		MenuStart(PLAYER_1);
		break;
	case SM_GoToPrevState:
		SCREENMAN->SetNewScreen( new ScreenTitleMenu );
		break;
	case SM_GoToNextState:
		SCREENMAN->SetNewScreen( new ScreenSelectMusic );
		break;
	case SM_StartFadingOut:
		m_Menu.TweenOffScreenToMenu( SM_GoToNextState );
		break;
	}
}

void ScreenSelectGroup::AfterChange()
{
	int sel = m_GroupList.GetSelection();
	m_MusicList.SetGroupNo(sel);

	CString sSelectedGroupName = m_GroupList.GetSelectionName();

	CString sGroupBannerPath;
	if( 0 == stricmp(sSelectedGroupName, "ALL MUSIC") )
		sGroupBannerPath = THEME->GetPathTo("Graphics","all music banner");
	else if( SONGMAN->GetGroupBannerPath(sSelectedGroupName) != "" )
		sGroupBannerPath = SONGMAN->GetGroupBannerPath(sSelectedGroupName);
	else
		sGroupBannerPath = THEME->GetPathTo("Graphics","fallback banner");

	const int iNumSongs = m_MusicList.GetNumSongs();
	m_textNumber.SetText( ssprintf("%d", iNumSongs) );

	m_Banner.SetFromGroup( sSelectedGroupName );
}


void ScreenSelectGroup::MenuLeft( const PlayerNumber p )
{
	MenuUp( p );
}

void ScreenSelectGroup::MenuRight( const PlayerNumber p )
{
	MenuDown( p );
}

void ScreenSelectGroup::MenuUp( const PlayerNumber p )
{
	if( m_bChosen )
		return;

	m_GroupList.Up();

	AfterChange();

	m_soundChange.PlayRandom();
}


void ScreenSelectGroup::MenuDown( const PlayerNumber p )
{
	if( m_bChosen )
		return;

	m_GroupList.Down();
	
	AfterChange();

	m_soundChange.PlayRandom();
}

void ScreenSelectGroup::MenuStart( const PlayerNumber p )
{
	m_soundSelect.PlayRandom();
	m_bChosen = true;

	GAMESTATE->m_pCurSong = NULL;
	GAMESTATE->m_sPreferredGroup = m_GroupList.GetSelectionName();

	if( 0 == stricmp(GAMESTATE->m_sPreferredGroup, "All Music") )
        SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ANNOUNCER_SELECT_GROUP_COMMENT_ALL_MUSIC) );
	else
        SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ANNOUNCER_SELECT_GROUP_COMMENT_GENERAL) );


	TweenOffScreen();
	m_Menu.StopTimer();

	this->SendScreenMessage( SM_StartFadingOut, 0.8f );
}

void ScreenSelectGroup::MenuBack( const PlayerNumber p )
{
	m_Menu.TweenOffScreenToBlack( SM_GoToPrevState, true );
}

void ScreenSelectGroup::TweenOffScreen()
{
	m_sprExplanation.BeginTweeningQueued( 0.8f );
	m_sprExplanation.BeginTweeningQueued( 0.5f, TWEEN_BOUNCE_BEGIN );
	m_sprExplanation.SetTweenX( EXPLANATION_X-400 );

	Actor* pActorsInGroupInfoFrame[] = { &m_sprFrame, &m_Banner, &m_textNumber };
	const int iNumActorsInGroupInfoFrame = sizeof(pActorsInGroupInfoFrame) / sizeof(Actor*);
	for( int i=0; i<iNumActorsInGroupInfoFrame; i++ )
	{
		pActorsInGroupInfoFrame[i]->BeginTweeningQueued( 0.9f );
		pActorsInGroupInfoFrame[i]->BeginTweeningQueued( 0.5f, TWEEN_BOUNCE_BEGIN );
		pActorsInGroupInfoFrame[i]->SetTweenX( pActorsInGroupInfoFrame[i]->GetX()-400 );
	}

	m_sprContents.BeginTweeningQueued( 0.7f );
	m_sprContents.BeginTweeningQueued( 0.5f, TWEEN_BIAS_END );
	m_sprContents.SetTweenY( CONTENTS_Y+400 );
	
	m_MusicList.TweenOffScreen();
	m_GroupList.TweenOffScreen();
}

void ScreenSelectGroup::TweenOnScreen() 
{
	m_sprExplanation.SetX( EXPLANATION_X-400 );
	m_sprExplanation.BeginTweening( 0.5f, TWEEN_BOUNCE_END );
	m_sprExplanation.SetTweenX( EXPLANATION_X );

	Actor* pActorsInGroupInfoFrame[] = { &m_sprFrame, &m_Banner, &m_textNumber };
	const int iNumActorsInGroupInfoFrame = sizeof(pActorsInGroupInfoFrame) / sizeof(Actor*);
	for( int i=0; i<iNumActorsInGroupInfoFrame; i++ )
	{
		float fOriginalX = pActorsInGroupInfoFrame[i]->GetX();
		pActorsInGroupInfoFrame[i]->SetX( fOriginalX-400 );
		pActorsInGroupInfoFrame[i]->BeginTweening( 0.5f, TWEEN_BOUNCE_END );
		pActorsInGroupInfoFrame[i]->SetTweenX( fOriginalX );
	}

	m_sprContents.SetY( CONTENTS_Y+400 );
	m_sprContents.BeginTweeningQueued( 0.5f, TWEEN_BIAS_END );	// sleep
	m_sprContents.BeginTweeningQueued( 0.5f, TWEEN_BIAS_END );
	m_sprContents.SetTweenY( CONTENTS_Y );

	m_MusicList.TweenOnScreen();
	m_GroupList.TweenOnScreen();
}
