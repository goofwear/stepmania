#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: MusicWheelItem

 Desc: See header.

 Copyright (c) 2001-2003 by the person(s) listed below.  All rights reserved.
	Chris Danford
	Chris Gomez
	Glenn Maynard
-----------------------------------------------------------------------------
*/

#include "MusicWheelItem.h"
#include "RageUtil.h"
#include "SongManager.h"
#include "GameManager.h"
#include "PrefsManager.h"
#include "ScreenManager.h"	// for sending SM_PlayMusicSample
#include "RageLog.h"
#include "GameConstantsAndTypes.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "song.h"
#include "Course.h"
#include "ProfileManager.h"
#include "ActorUtil.h"


// WheelItem stuff
#define ICON_X			THEME->GetMetricF("MusicWheelItem","IconX")
#define SONG_NAME_X		THEME->GetMetricF("MusicWheelItem","SongNameX")
#define SECTION_NAME_X	THEME->GetMetricF("MusicWheelItem","SectionNameX")
#define SECTION_ZOOM	THEME->GetMetricF("MusicWheelItem","SectionZoom")
#define ROULETTE_X		THEME->GetMetricF("MusicWheelItem","RouletteX")
#define ROULETTE_ZOOM	THEME->GetMetricF("MusicWheelItem","RouletteZoom")
#define GRADE_X( p )	THEME->GetMetricF("MusicWheelItem",ssprintf("GradeP%dX",p+1))




WheelItemData::WheelItemData( WheelItemType wit, Song* pSong, CString sSectionName, Course* pCourse, RageColor color, SortOrder so )
{
	m_Type = wit;
	m_pSong = pSong;
	m_sSectionName = sSectionName;
	m_pCourse = pCourse;
	m_color = color;
	m_Flags = WheelNotifyIcon::Flags();
	m_SortOrder = so;
}


MusicWheelItem::MusicWheelItem()
{
	data = NULL;

	SetName( "MusicWheelItem" );

	m_fPercentGray = 0;
	m_WheelNotifyIcon.SetXY( ICON_X, 0 );
	
	m_TextBanner.SetName( "TextBanner" );
	m_TextBanner.SetHorizAlign( align_left );
	m_TextBanner.SetXY( SONG_NAME_X, 0 );
	m_All.AddChild( &m_TextBanner );

	m_sprSongBar.Load( THEME->GetPathToG("MusicWheelItem song") );
	m_sprSongBar.SetXY( 0, 0 );
	m_All.AddChild( &m_sprSongBar );

	m_sprSectionBar.Load( THEME->GetPathToG("MusicWheelItem section") );
	m_sprSectionBar.SetXY( 0, 0 );
	m_All.AddChild( &m_sprSectionBar );

	m_textSectionName.LoadFromFont( THEME->GetPathToF("MusicWheelItem section") );
	m_textSectionName.SetShadowLength( 0 );
	m_textSectionName.SetVertAlign( align_middle );
	m_textSectionName.SetXY( SECTION_NAME_X, 0 );
	m_textSectionName.SetZoom( SECTION_ZOOM );
	m_All.AddChild( &m_textSectionName );


	m_textRoulette.LoadFromFont( THEME->GetPathToF("MusicWheelItem roulette") );
	m_textRoulette.SetShadowLength( 0 );
	m_textRoulette.TurnRainbowOn();
	m_textRoulette.SetZoom( ROULETTE_ZOOM );
	m_textRoulette.SetXY( ROULETTE_X, 0 );
	m_All.AddChild( &m_textRoulette );

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		m_GradeDisplay[p].Load( THEME->GetPathToG("MusicWheelItem grades") );
		m_GradeDisplay[p].SetZoom( 1.0f );
		m_GradeDisplay[p].SetXY( GRADE_X(p), 0 );
		m_All.AddChild( &m_GradeDisplay[p] );
	}

	m_textCourse.SetName( "CourseName" );
	m_textCourse.LoadFromFont( THEME->GetPathToF("MusicWheelItem course") );
	SET_XY_AND_ON_COMMAND( &m_textCourse );
	m_All.AddChild( &m_textCourse );

	m_textSort.SetName( "Sort" );
	m_textSort.LoadFromFont( THEME->GetPathToF("MusicWheelItem sort") );
	SET_XY_AND_ON_COMMAND( &m_textSort );
	m_All.AddChild( &m_textSort );
}


void MusicWheelItem::LoadFromWheelItemData( WheelItemData* pWID )
{
	ASSERT( pWID != NULL );
	
	
	
	data = pWID;
	/*
	// copy all data items
	this->m_Type	= pWID->m_Type;
	this->m_sSectionName	= pWID->m_sSectionName;
	this->m_pCourse			= pWID->m_pCourse;
	this->m_pSong			= pWID->m_pSong;
	this->m_color			= pWID->m_color;
	this->m_Type		= pWID->m_Type; */


	// init type specific stuff
	switch( pWID->m_Type )
	{
	case TYPE_SECTION:
	case TYPE_COURSE:
	case TYPE_SORT:
		{
			CString sDisplayName;
			BitmapText *bt = NULL;
			switch( pWID->m_Type )
			{
				case TYPE_SECTION:
					sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
					bt = &m_textSectionName;
					break;
				case TYPE_COURSE:
					sDisplayName = data->m_pCourse->m_sName;
					bt = &m_textCourse;
					break;
				case TYPE_SORT:
					sDisplayName = data->m_sLabel;
					bt = &m_textSort;
					break;
				default:
					ASSERT(0);
			}

			bt->SetZoom( 1 );
			bt->SetText( sDisplayName );
			bt->SetDiffuse( data->m_color );
			bt->TurnRainbowOff();

			const float fSourcePixelWidth = (float)bt->GetUnzoomedWidth();
			const float fMaxTextWidth = 200;
			if( fSourcePixelWidth > fMaxTextWidth  )
				bt->SetZoomX( fMaxTextWidth / fSourcePixelWidth );
		}
		break;
	case TYPE_SONG:
		{
			m_TextBanner.LoadFromSong( data->m_pSong );
			m_TextBanner.SetDiffuse( data->m_color );
			m_WheelNotifyIcon.SetFlags( data->m_Flags );
			RefreshGrades();
		}
		break;
	case TYPE_ROULETTE:
		m_textRoulette.SetText( "ROULETTE" );
		break;

	case TYPE_RANDOM:
		m_textRoulette.SetText( "RANDOM" );
		break;

	default:
		ASSERT( 0 );	// invalid type
	}
}

void MusicWheelItem::RefreshGrades()
{
	// Refresh Grades
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( !data->m_pSong  ||	// this isn't a song display
			!GAMESTATE->IsHumanPlayer(p) )
		{
			m_GradeDisplay[p].SetDiffuse( RageColor(1,1,1,0) );
			continue;
		}

		Difficulty dc;
		if( GAMESTATE->m_pCurNotes[p] )
			dc = GAMESTATE->m_pCurNotes[p]->GetDifficulty();
		else
			dc = GAMESTATE->m_PreferredDifficulty[p];
		Grade grade;
		if( PROFILEMAN->IsUsingProfile((PlayerNumber)p) )
			grade = PROFILEMAN->GetHighScoreForDifficulty( data->m_pSong, GAMESTATE->GetCurrentStyleDef(), (ProfileSlot)p, dc ).grade;
		else
			grade = PROFILEMAN->GetHighScoreForDifficulty( data->m_pSong, GAMESTATE->GetCurrentStyleDef(), PROFILE_SLOT_MACHINE, dc ).grade;

		m_GradeDisplay[p].SetGrade( (PlayerNumber)p, grade );
	}

}


void MusicWheelItem::Update( float fDeltaTime )
{
	Actor::Update( fDeltaTime );

	switch( data->m_Type )
	{
	case TYPE_SECTION:
		m_sprSectionBar.Update( fDeltaTime );
		m_textSectionName.Update( fDeltaTime );
		break;
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
		m_sprSectionBar.Update( fDeltaTime );
		m_textRoulette.Update( fDeltaTime );
		break;
	case TYPE_SONG:
		{
			m_sprSongBar.Update( fDeltaTime );
			m_WheelNotifyIcon.Update( fDeltaTime );
			m_TextBanner.Update( fDeltaTime );
			for( int p=0; p<NUM_PLAYERS; p++ )
				m_GradeDisplay[p].Update( fDeltaTime );
		}
		break;
	case TYPE_COURSE:
		m_sprSongBar.Update( fDeltaTime );
		m_textCourse.Update( fDeltaTime );
		break;
	case TYPE_SORT:
		m_sprSectionBar.Update( fDeltaTime );
		m_textSort.Update( fDeltaTime );
		break;
	default:
		ASSERT(0);
	}
}

void MusicWheelItem::DrawPrimitives()
{
	Sprite *bar = NULL;
	switch( data->m_Type )
	{
	case TYPE_SECTION: 
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
	case TYPE_SORT:
		bar = &m_sprSectionBar; 
		break;
	case TYPE_SONG:		
	case TYPE_COURSE:
		bar = &m_sprSongBar; 
		break;
	default: ASSERT(0);
	}
	
	bar->Draw();

	switch( data->m_Type )
	{
	case TYPE_SECTION:
		m_textSectionName.Draw();
		break;
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
		m_textRoulette.Draw();
		break;
	case TYPE_SONG:		
		m_TextBanner.Draw();
		m_WheelNotifyIcon.Draw();
		int p;
		for( p=0; p<NUM_PLAYERS; p++ )
			m_GradeDisplay[p].Draw();
		break;
	case TYPE_COURSE:
		m_textCourse.Draw();
		break;
	case TYPE_SORT:
		m_textSort.Draw();
		break;
	default:
		ASSERT(0);
	}

	if( m_fPercentGray > 0 )
	{
		bar->SetGlow( RageColor(0,0,0,m_fPercentGray) );
		bar->SetDiffuse( RageColor(0,0,0,0) );
		bar->Draw();
		bar->SetDiffuse( RageColor(0,0,0,1) );
		bar->SetGlow( RageColor(0,0,0,0) );
	}
}


void MusicWheelItem::SetZTest( bool b )
{
	ActorFrame::SetZTest( b );

	// set all sub-Actors
	m_All.SetZTest( b );
}

void MusicWheelItem::SetZWrite( bool b )
{
	ActorFrame::SetZWrite( b );

	// set all sub-Actors
	m_All.SetZWrite( b );
}

bool WheelItemData::HasBanner() const
{
	switch( m_Type )
	{
	case TYPE_SONG:
		return m_pSong->HasBanner();
	case TYPE_COURSE:
		return m_pCourse->HasBanner();
	case TYPE_SECTION:
		return SONGMAN->GetGroupBannerPath( m_sSectionName ).size() != 0;
	/* XXX: These are special cases. */
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
	case TYPE_SORT:
		return false;
		
	default: ASSERT(0); return false; // "";
	}
}

CString WheelItemData::GetBanner() const
{
	switch( m_Type )
	{
	case TYPE_SONG:
		return m_pSong->GetBannerPath();
	case TYPE_COURSE:
		return m_pCourse->m_sBannerPath;
	case TYPE_SECTION:
		return SONGMAN->GetGroupBannerPath( m_sSectionName );

	case TYPE_ROULETTE:
		return THEME->GetPathToG("Banner random");

	case TYPE_RANDOM:
		return THEME->GetPathToG("Banner roulette");
		
	default: ASSERT(0); return "";
	}
}
