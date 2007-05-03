#include "global.h"
#include "MusicWheelItem.h"
#include "RageUtil.h"
#include "SongManager.h"
#include "GameManager.h"
#include "RageLog.h"
#include "GameConstantsAndTypes.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "song.h"
#include "Course.h"
#include "ProfileManager.h"
#include "ActorUtil.h"
#include "ThemeMetric.h"
#include "HighScore.h"

WheelItemData::WheelItemData( WheelItemType wit, Song* pSong, RString sSectionName, Course* pCourse, RageColor color ):
	WheelItemBaseData(wit, sSectionName, color)
{
	m_pSong = pSong;
	m_pCourse = pCourse;
	m_Flags = WheelNotifyIcon::Flags();
}

MusicWheelItem::MusicWheelItem( RString sType ):
	WheelItemBase( sType )
{
	data = NULL;

	m_sprSongBar.Load( THEME->GetPathG(sType,"song") );
	this->AddChild( m_sprSongBar );

	m_sprSectionBar.Load( THEME->GetPathG(sType,"section") );
	this->AddChild( m_sprSectionBar );

	m_sprExpandedBar.Load( THEME->GetPathG(sType,"expanded") );
	this->AddChild( m_sprExpandedBar );

	m_sprModeBar.Load( THEME->GetPathG(sType,"mode") );
	this->AddChild( m_sprModeBar );

	m_sprSortBar.Load( THEME->GetPathG(sType,"sort") );
	this->AddChild( m_sprSortBar );

	m_WheelNotifyIcon.SetName( "Icon" );
	ActorUtil::LoadAllCommands( m_WheelNotifyIcon, "MusicWheelItem" );
	ActorUtil::SetXY( m_WheelNotifyIcon, "MusicWheelItem" );
	m_WheelNotifyIcon.PlayCommand( "On" );
	this->AddChild( &m_WheelNotifyIcon );
	
	m_TextBanner.SetName( "SongName" );
	ActorUtil::LoadAllCommands( m_TextBanner, "MusicWheelItem" );
	m_TextBanner.Load( "TextBanner" );
	ActorUtil::SetXY( m_WheelNotifyIcon, "MusicWheelItem" );
	m_TextBanner.PlayCommand( "On" );
	this->AddChild( &m_TextBanner );

	m_textSection.SetName( "Section" );
	ActorUtil::LoadAllCommands( m_textSection, "MusicWheelItem" );
	m_textSection.LoadFromFont( THEME->GetPathF(sType,"section") );
	ActorUtil::SetXY( m_textSection, "MusicWheelItem" );
	m_textSection.SetShadowLength( 0 );
	m_textSection.PlayCommand( "On" );
	this->AddChild( &m_textSection );

	m_textRoulette.SetName( "Roulette" );
	ActorUtil::LoadAllCommands( m_textRoulette, "MusicWheelItem" );
	m_textRoulette.LoadFromFont( THEME->GetPathF(sType,"roulette") );
	ActorUtil::SetXY( m_textRoulette, "MusicWheelItem" );
	m_textRoulette.SetShadowLength( 0 );
	m_textRoulette.PlayCommand( "On" );
	this->AddChild( &m_textRoulette );

	m_textCourse.SetName( "CourseName" );
	m_textCourse.LoadFromFont( THEME->GetPathF(sType,"course") );
	LOAD_ALL_COMMANDS_AND_SET_XY( &m_textCourse );
	this->AddChild( &m_textCourse );

	m_textSort.SetName( "Sort" );
	m_textSort.LoadFromFont( THEME->GetPathF(sType,"sort") );
	LOAD_ALL_COMMANDS_AND_SET_XY( &m_textSort );
	this->AddChild( &m_textSort );

	FOREACH_PlayerNumber( p )
	{
		m_pGradeDisplay[p] = new GradeDisplay;
		m_pGradeDisplay[p]->SetName( ssprintf("GradeP%d",int(p+1)) );
		m_pGradeDisplay[p]->Load( THEME->GetPathG(sType,"grades") );
		m_pGradeDisplay[p]->SetZoom( 1.0f );
		this->AddChild( m_pGradeDisplay[p] );
		LOAD_ALL_COMMANDS_AND_SET_XY( m_pGradeDisplay[p] );
	}

	this->SubscribeToMessage( Message_CurrentStepsP1Changed );
	this->SubscribeToMessage( Message_CurrentStepsP2Changed );
	this->SubscribeToMessage( Message_CurrentTrailP1Changed );
	this->SubscribeToMessage( Message_CurrentTrailP2Changed );
	this->SubscribeToMessage( Message_PreferredDifficultyP1Changed );
	this->SubscribeToMessage( Message_PreferredDifficultyP2Changed );
}

MusicWheelItem::MusicWheelItem( const MusicWheelItem &cpy ):
	WheelItemBase( cpy ),
	m_sprSongBar( cpy.m_sprSongBar ),
	m_sprSectionBar( cpy.m_sprSectionBar ),
	m_sprExpandedBar( cpy.m_sprExpandedBar ),
	m_sprModeBar( cpy.m_sprModeBar ),
	m_sprSortBar( cpy.m_sprSortBar ),
	m_WheelNotifyIcon( cpy.m_WheelNotifyIcon ),
	m_TextBanner( cpy.m_TextBanner ),
	m_textSection( cpy.m_textSection ),
	m_textRoulette( cpy.m_textRoulette ),
	m_textCourse( cpy.m_textCourse ),
	m_textSort( cpy.m_textSort )
{
	data = NULL;

	this->AddChild( m_sprSongBar );
	this->AddChild( m_sprSectionBar );
	this->AddChild( m_sprExpandedBar );
	this->AddChild( m_sprModeBar );
	this->AddChild( m_sprSortBar );
	this->AddChild( &m_WheelNotifyIcon );
	this->AddChild( &m_TextBanner );
	this->AddChild( &m_textSection );
	this->AddChild( &m_textRoulette );
	this->AddChild( &m_textCourse );
	this->AddChild( &m_textSort );

	FOREACH_PlayerNumber( p )
	{
		m_pGradeDisplay[p] = new GradeDisplay( *cpy.m_pGradeDisplay[p] );
		this->AddChild( m_pGradeDisplay[p] );
	}
}

MusicWheelItem::~MusicWheelItem()
{
	FOREACH_PlayerNumber( p )
		delete m_pGradeDisplay[p];
}

void MusicWheelItem::LoadFromWheelItemData( const WheelItemBaseData *pWIBD, int iIndex, bool bHasFocus )
{
	const WheelItemData *pWID = dynamic_cast<const WheelItemData*>( pWIBD );
	
	ASSERT( pWID != NULL );
	data = pWID;


	// hide all
	m_WheelNotifyIcon.SetVisible( false );
	m_TextBanner.SetVisible( false );
	m_sprSongBar->SetVisible( false );
	m_sprSectionBar->SetVisible( false );
	m_sprExpandedBar->SetVisible( false );
	m_sprModeBar->SetVisible( false );
	m_sprSortBar->SetVisible( false );
	m_textSection.SetVisible( false );
	m_textRoulette.SetVisible( false );
	FOREACH_PlayerNumber( p )
		m_pGradeDisplay[p]->SetVisible( false );
	m_textCourse.SetVisible( false );
	m_textSort.SetVisible( false );


	// init and unhide type specific stuff
	switch( pWID->m_Type )
	{
	DEFAULT_FAIL( pWID->m_Type );
	case TYPE_SECTION:
	case TYPE_COURSE:
	case TYPE_SORT:
	{
		RString sDisplayName, sTranslitName;
		BitmapText *bt = NULL;
		switch( pWID->m_Type )
		{
		case TYPE_SECTION:
			sDisplayName = SONGMAN->ShortenGroupName(data->m_sText);
			bt = &m_textSection;
			break;
		case TYPE_COURSE:
			sDisplayName = data->m_pCourse->GetDisplayFullTitle();
			sTranslitName = data->m_pCourse->GetTranslitFullTitle();
			bt = &m_textCourse;
			m_WheelNotifyIcon.SetFlags( data->m_Flags );
			m_WheelNotifyIcon.SetVisible( true );
			break;
		case TYPE_SORT:
			sDisplayName = data->m_sLabel;
			bt = &m_textSort;
			break;
		DEFAULT_FAIL( pWID->m_Type );
		}

		bt->SetText( sDisplayName, sTranslitName );
		bt->SetDiffuse( data->m_color );
		bt->SetRainbowScroll( false );
		bt->SetVisible( true );
		break;
	}
	case TYPE_SONG:
		m_TextBanner.LoadFromSong( data->m_pSong );
		m_TextBanner.SetDiffuse( data->m_color );
		m_TextBanner.SetVisible( true );

		m_WheelNotifyIcon.SetFlags( data->m_Flags );
		m_WheelNotifyIcon.SetVisible( true );
		RefreshGrades();
		break;
	case TYPE_ROULETTE:
		m_textRoulette.SetText( THEME->GetString("MusicWheel","Roulette") );
		m_textRoulette.SetVisible( true );
		break;

	case TYPE_RANDOM:
		m_textRoulette.SetText( THEME->GetString("MusicWheel","Random") );
		m_textRoulette.SetVisible( true );
		break;

	case TYPE_PORTAL:
		m_textRoulette.SetText( THEME->GetString("MusicWheel","Portal") );
		m_textRoulette.SetVisible( true );
		break;
	}

	Actor *pBars[] = { m_sprBar, m_sprExpandedBar, m_sprSectionBar, m_sprModeBar, m_sprSortBar, m_sprSongBar, NULL };
	for( unsigned i = 0; pBars[i] != NULL; ++i )
		pBars[i]->SetVisible( false );

	switch( data->m_Type )
	{
	case TYPE_SECTION: 
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
	case TYPE_PORTAL:
		if( m_bExpanded )
			m_sprExpandedBar->SetVisible( true );
		else
			m_sprSectionBar->SetVisible( true );
		break;
	case TYPE_SORT:
		if( pWID->m_pAction->m_pm != PlayMode_Invalid )
			m_sprModeBar->SetVisible( true );
		else
			m_sprSortBar->SetVisible( true );
		break;
	case TYPE_SONG:		
	case TYPE_COURSE:
		m_sprSongBar->SetVisible( true );
		break;
	DEFAULT_FAIL( data->m_Type );
	}

	for( unsigned i = 0; pBars[i] != NULL; ++i )
	{
		if( !pBars[i]->GetVisible() )
			continue;
		SetGrayBar( pBars[i] );
		break;
	}

	GameCommand gc;
	gc.m_pSong = pWID->m_pSong;
	gc.m_sSongGroup = pWID->m_sText;
	gc.m_bHasFocus = true;
	gc.m_iIndex = iIndex;
	gc.m_bHasFocus = bHasFocus;

	Lua *L = LUA->Get();
	gc.PushSelf( L );
	lua_setglobal( L, "ThisGameCommand" );
	LUA->Release( L );

	// Call "Set" so that elements can react to the change in song.
	{
		Message msg( "Set" );
		msg.SetParam( "Song", data->m_pSong );
		this->HandleMessage( msg );
	}

	LUA->UnsetGlobal( "ThisGameCommand" );
}

void MusicWheelItem::RefreshGrades()
{
	if( data == NULL )
		return; // LoadFromWheelItemData() hasn't been called yet.
	FOREACH_HumanPlayer( p )
	{
		if( data->m_pSong == NULL )
		{
			m_pGradeDisplay[p]->SetDiffuse( RageColor(1,1,1,0) );
			continue;
		}

		Difficulty dc;
		if( GAMESTATE->m_pCurSteps[p] )
			dc = GAMESTATE->m_pCurSteps[p]->GetDifficulty();
		else if( GAMESTATE->m_pCurTrail[p] )
			dc = GAMESTATE->m_pCurTrail[p]->m_CourseDifficulty;
		else
			dc = GAMESTATE->m_PreferredDifficulty[p];

		ProfileSlot ps = ProfileSlot_Machine;
		if( PROFILEMAN->IsPersistentProfile(p) )
			ps = (ProfileSlot)p;

		Grade g = PROFILEMAN->GetGradeForSteps( data->m_pSong, GAMESTATE->GetCurrentStyle(), ps, dc );
		m_pGradeDisplay[p]->SetGrade( p, g );
	}
}

void MusicWheelItem::HandleMessage( const Message &msg )
{
	if( msg == Message_CurrentStepsP1Changed ||
	    msg == Message_CurrentStepsP2Changed ||
	    msg == Message_CurrentTrailP1Changed ||
	    msg == Message_CurrentTrailP2Changed ||
	    msg == Message_PreferredDifficultyP1Changed ||
	    msg == Message_PreferredDifficultyP2Changed )
	{
		RefreshGrades();
	}

	WheelItemBase::HandleMessage( msg );
}

/*
 * (c) 2001-2004 Chris Danford, Chris Gomez, Glenn Maynard
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
