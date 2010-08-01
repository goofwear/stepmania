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

static RString GRADE_X_NAME( size_t p ) { return ssprintf("GradeP%dX",int(p+1)); }
static RString GRADE_Y_NAME( size_t p ) { return ssprintf("GradeP%dY",int(p+1)); }

static ThemeMetric<float>		ICON_X			("MusicWheelItem","IconX");
static ThemeMetric<float>		ICON_Y			("MusicWheelItem","IconY");
static ThemeMetric<apActorCommands>	ICON_ON_COMMAND		("MusicWheelItem","IconOnCommand");
static ThemeMetric<float>		SONG_NAME_X		("MusicWheelItem","SongNameX");
static ThemeMetric<float>		SONG_NAME_Y		("MusicWheelItem","SongNameY");
static ThemeMetric<apActorCommands>	SONG_NAME_ON_COMMAND	("MusicWheelItem","SongNameOnCommand");
static ThemeMetric<float>		SECTION_X		("MusicWheelItem","SectionX");
static ThemeMetric<float>		SECTION_Y		("MusicWheelItem","SectionY");
static ThemeMetric<apActorCommands>	SECTION_ON_COMMAND	("MusicWheelItem","SectionOnCommand");
static ThemeMetric<float>		ROULETTE_X		("MusicWheelItem","RouletteX");
static ThemeMetric<float>		ROULETTE_Y		("MusicWheelItem","RouletteY");
static ThemeMetric<apActorCommands>	ROULETTE_ON_COMMAND	("MusicWheelItem","RouletteOnCommand");
static ThemeMetric1D<float>		GRADE_X			("MusicWheelItem",GRADE_X_NAME,NUM_PLAYERS);
static ThemeMetric1D<float>		GRADE_Y			("MusicWheelItem",GRADE_Y_NAME,NUM_PLAYERS);


WheelItemData::WheelItemData( WheelItemType wit, Song* pSong, RString sSectionName, Course* pCourse, RageColor color ):
	WheelItemBaseData(wit, sSectionName, color)
{
	m_pSong = pSong;
	m_pCourse = pCourse;
}


MusicWheelItem::MusicWheelItem( RString sType ):
	WheelItemBase( sType )
{
	data = NULL;

	m_sprSongBar.Load( THEME->GetPathG(sType,"song") );
	this->AddChild( &m_sprSongBar );

	m_sprSectionBar.Load( THEME->GetPathG(sType,"section") );
	this->AddChild( &m_sprSectionBar );

	m_sprExpandedBar.Load( THEME->GetPathG(sType,"expanded") );
	this->AddChild( &m_sprExpandedBar );

	m_sprModeBar.Load( THEME->GetPathG(sType,"mode") );
	this->AddChild( &m_sprModeBar );

	m_sprSortBar.Load( THEME->GetPathG(sType,"sort") );
	this->AddChild( &m_sprSortBar );

	m_WheelNotifyIcon.SetXY( ICON_X, ICON_Y );
	m_WheelNotifyIcon.RunCommands( ICON_ON_COMMAND );
	this->AddChild( &m_WheelNotifyIcon );
	
	m_TextBanner.Load( "TextBanner" );
	m_TextBanner.SetXY( SONG_NAME_X, SONG_NAME_Y );
	m_TextBanner.RunCommands( SONG_NAME_ON_COMMAND );
	this->AddChild( &m_TextBanner );

	m_textSection.LoadFromFont( THEME->GetPathF(sType,"section") );
	m_textSection.SetShadowLength( 0 );
	m_textSection.SetXY( SECTION_X, SECTION_Y );
	m_textSection.RunCommands( SECTION_ON_COMMAND );
	this->AddChild( &m_textSection );

	m_textRoulette.LoadFromFont( THEME->GetPathF(sType,"roulette") );
	m_textRoulette.SetShadowLength( 0 );
	m_textRoulette.TurnRainbowOn();
	m_textRoulette.SetXY( ROULETTE_X, ROULETTE_Y );
	m_textRoulette.RunCommands( ROULETTE_ON_COMMAND );
	this->AddChild( &m_textRoulette );

	m_textCourse.SetName( "CourseName" );
	m_textCourse.LoadFromFont( THEME->GetPathF(sType,"course") );
	SET_XY_AND_ON_COMMAND( &m_textCourse );
	this->AddChild( &m_textCourse );

	m_textSort.SetName( "Sort" );
	m_textSort.LoadFromFont( THEME->GetPathF(sType,"sort") );
	SET_XY_AND_ON_COMMAND( &m_textSort );
	this->AddChild( &m_textSort );

	FOREACH_PlayerNumber( p )
	{
		m_pGradeDisplay[p] = new GradeDisplay;
		m_pGradeDisplay[p]->Load( THEME->GetPathG(sType,"grades") );
		m_pGradeDisplay[p]->SetZoom( 1.0f );
		m_pGradeDisplay[p]->SetXY( GRADE_X.GetValue(p), 0 );
		this->AddChild( m_pGradeDisplay[p] );
	}
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

	this->AddChild( &m_sprSongBar );
	this->AddChild( &m_sprSectionBar );
	this->AddChild( &m_sprExpandedBar );
	this->AddChild( &m_sprModeBar );
	this->AddChild( &m_sprSortBar );
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

void MusicWheelItem::LoadFromWheelItemData( WheelItemData* pWID, bool bExpanded )
{
	ASSERT( pWID != NULL );
	data = pWID;


	// hide all
	m_WheelNotifyIcon.SetHidden( true );
	m_TextBanner.SetHidden( true );
	m_sprSongBar.SetHidden( true );
	m_sprSectionBar.SetHidden( true );
	m_sprExpandedBar.SetHidden( true );
	m_sprModeBar.SetHidden( true );
	m_sprSortBar.SetHidden( true );
	m_textSection.SetHidden( true );
	m_textRoulette.SetHidden( true );
	FOREACH_PlayerNumber( p )
		m_pGradeDisplay[p]->SetHidden( true );
	m_textCourse.SetHidden( true );
	m_textSort.SetHidden( true );


	// init and unhide type specific stuff
	switch( pWID->m_Type )
	{
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
				m_WheelNotifyIcon.SetHidden( false );
				break;
			case TYPE_SORT:
				sDisplayName = data->m_sLabel;
				bt = &m_textSort;
				break;
			default:
				ASSERT(0);
			}

			bt->SetText( sDisplayName, sTranslitName );
			bt->SetDiffuse( data->m_color );
			bt->TurnRainbowOff();
			bt->SetHidden( false );
		}
		break;
	case TYPE_SONG:
		{
			m_TextBanner.LoadFromSong( data->m_pSong );
			m_TextBanner.SetDiffuse( data->m_color );
			m_TextBanner.SetHidden( false );

			m_WheelNotifyIcon.SetFlags( data->m_Flags );
			m_WheelNotifyIcon.SetHidden( false );
			RefreshGrades();
		}
		break;
	case TYPE_ROULETTE:
		m_textRoulette.SetText( THEME->GetString("MusicWheel","Roulette") );
		m_textRoulette.SetHidden( false );
		break;

	case TYPE_RANDOM:
		m_textRoulette.SetText( THEME->GetString("MusicWheel","Random") );
		m_textRoulette.SetHidden( false );
		break;

	case TYPE_PORTAL:
		m_textRoulette.SetText( THEME->GetString("MusicWheel","Portal") );
		m_textRoulette.SetHidden( false );
		break;

	default:
		ASSERT( 0 );	// invalid type
	}

	Actor *pBars[] = { &m_sprBar, &m_sprExpandedBar, &m_sprSectionBar, &m_sprModeBar, &m_sprSortBar, &m_sprSongBar, NULL };
	for( unsigned i = 0; pBars[i] != NULL; ++i )
		pBars[i]->SetVisible( false );

	switch( data->m_Type )
	{
	case TYPE_SECTION: 
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
	case TYPE_PORTAL:
		if( bExpanded )
			m_sprExpandedBar.SetHidden( false );
		else
			m_sprSectionBar.SetHidden( false );
		break;
	case TYPE_SORT:
		if( pWID->m_Action.m_pm != PLAY_MODE_INVALID )
			m_sprModeBar.SetHidden( false );
		else
			m_sprSortBar.SetHidden( false );
		break;
	case TYPE_SONG:		
	case TYPE_COURSE:
		m_sprSongBar.SetHidden( false );
		break;
	default: ASSERT(0);
	}

	for( unsigned i = 0; pBars[i] != NULL; ++i )
	{
		if( !pBars[i]->GetVisible() )
			continue;
		SetGrayBar( pBars[i] );
		break;
	}
}

void MusicWheelItem::RefreshGrades()
{
	FOREACH_HumanPlayer( p )
	{
		if( data->m_pSong == NULL  ||	// this isn't a song display
			!GAMESTATE->IsHumanPlayer(p) )
		{
			m_pGradeDisplay[p]->SetDiffuse( RageColor(1,1,1,0) );
			continue;
		}

		Difficulty dc;
		if( GAMESTATE->m_pCurSteps[p] )
			dc = GAMESTATE->m_pCurSteps[p]->GetDifficulty();
		else
			dc = GAMESTATE->m_PreferredDifficulty[p];

		HighScore hs;
		if( PROFILEMAN->IsPersistentProfile(p) )
			PROFILEMAN->GetHighScoreForDifficulty( data->m_pSong, GAMESTATE->GetCurrentStyle(), (ProfileSlot)p, dc, hs );
		else
			PROFILEMAN->GetHighScoreForDifficulty( data->m_pSong, GAMESTATE->GetCurrentStyle(), ProfileSlot_Machine, dc, hs );

		m_pGradeDisplay[p]->SetGrade( p, hs.GetGrade() );
	}
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
