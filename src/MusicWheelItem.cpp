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
#include "Song.h"
#include "Course.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "Style.h"
#include "ActorUtil.h"
#include "ThemeMetric.h"
#include "HighScore.h"
#include "ScreenSelectMusic.h"
#include "ScreenManager.h"

static const char *MusicWheelItemTypeNames[] = {
	"Song",
	"SectionExpanded",
	"SectionCollapsed",
	"Roulette",
	"Course",
	"Sort",
	"Mode",
};
XToString( MusicWheelItemType );

MusicWheelItemData::MusicWheelItemData( WheelItemDataType type, Song* pSong, RString sSectionName, Course* pCourse, RageColor color, int iSectionCount ):
	WheelItemBaseData(type, sSectionName, color)
{
	m_pSong = pSong;
	m_pCourse = pCourse;
	m_Flags = WheelNotifyIcon::Flags();
	m_iSectionCount = iSectionCount;
}

MusicWheelItem::MusicWheelItem( RString sType ):
	WheelItemBase( sType )
{
	GRADES_SHOW_MACHINE.Load( sType, "GradesShowMachine" );

	FOREACH_ENUM( MusicWheelItemType, i )
	{
		m_sprColorPart[i].Load( THEME->GetPathG(sType,MusicWheelItemTypeToString(i)+" ColorPart") );
		this->AddChild( m_sprColorPart[i] );

		m_sprNormalPart[i].Load( THEME->GetPathG(sType,MusicWheelItemTypeToString(i)+" NormalPart") );
		this->AddChild( m_sprNormalPart[i] );
	}

	m_TextBanner.SetName( "SongName" );
	ActorUtil::LoadAllCommands( m_TextBanner, "MusicWheelItem" );
	m_TextBanner.Load( "TextBanner" );
	ActorUtil::SetXY( m_TextBanner, "MusicWheelItem" );
	m_TextBanner.PlayCommand( "On" );
	this->AddChild( &m_TextBanner );

	FOREACH_ENUM( MusicWheelItemType, i )
	{
		m_pText[i] = NULL;

		// Don't init text for Type_Song.  It uses a TextBanner.
		if( i == MusicWheelItemType_Song )
			continue;

		m_pText[i] = new BitmapText;
		m_pText[i]->SetName( MusicWheelItemTypeToString(i) );
		ActorUtil::LoadAllCommands( m_pText[i], "MusicWheelItem" );
		m_pText[i]->LoadFromFont( THEME->GetPathF(sType,MusicWheelItemTypeToString(i)) );
		ActorUtil::SetXY( m_pText[i], "MusicWheelItem" );
		m_pText[i]->PlayCommand( "On" );
		this->AddChild( m_pText[i] );
	}

	m_pTextSectionCount = new BitmapText;
	m_pTextSectionCount->SetName( "SectionCount" );
	ActorUtil::LoadAllCommands( m_pTextSectionCount, "MusicWheelItem" );
	m_pTextSectionCount->LoadFromFont( THEME->GetPathF(sType,"SectionCount") );
	ActorUtil::SetXY( m_pTextSectionCount, "MusicWheelItem" );
	m_pTextSectionCount->PlayCommand( "On" );
	this->AddChild( m_pTextSectionCount );

	m_WheelNotifyIcon.SetName( "Icon" );
	ActorUtil::LoadAllCommands( m_WheelNotifyIcon, "MusicWheelItem" );
	ActorUtil::SetXY( m_WheelNotifyIcon, "MusicWheelItem" );
	m_WheelNotifyIcon.PlayCommand( "On" );
	this->AddChild( &m_WheelNotifyIcon );
	
	FOREACH_PlayerNumber( p )
	{
		m_pGradeDisplay[p].Load( THEME->GetPathG(sType,"grades") );
		m_pGradeDisplay[p]->SetName( ssprintf("GradeP%d",int(p+1)) );
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
	GRADES_SHOW_MACHINE( cpy.GRADES_SHOW_MACHINE ),
	m_TextBanner( cpy.m_TextBanner ),
	m_WheelNotifyIcon( cpy.m_WheelNotifyIcon )
{
	FOREACH_ENUM( MusicWheelItemType, i )
	{
		m_sprColorPart[i] = cpy.m_sprColorPart[i];
		this->AddChild( m_sprColorPart[i] );

		m_sprNormalPart[i] = cpy.m_sprNormalPart[i];
		this->AddChild( m_sprNormalPart[i] );
	}

	this->AddChild( &m_TextBanner );

	FOREACH_ENUM( MusicWheelItemType, i )
	{
		if( cpy.m_pText[i] == NULL )
		{
			m_pText[i] = NULL;
		}
		else
		{
			m_pText[i] = new BitmapText( *cpy.m_pText[i] );
			this->AddChild( m_pText[i] );
		}
	}

	m_pTextSectionCount = new BitmapText( *cpy.m_pTextSectionCount );
	this->AddChild( m_pTextSectionCount );

	this->AddChild( &m_WheelNotifyIcon );

	FOREACH_PlayerNumber( p )
	{
		m_pGradeDisplay[p] = cpy.m_pGradeDisplay[p];
		this->AddChild( m_pGradeDisplay[p] );
	}
}

MusicWheelItem::~MusicWheelItem()
{
}

void MusicWheelItem::LoadFromWheelItemData( const WheelItemBaseData *pData, int iIndex, bool bHasFocus )
{
	WheelItemBase::LoadFromWheelItemData( pData, iIndex, bHasFocus );

	const MusicWheelItemData *pWID = dynamic_cast<const MusicWheelItemData*>( pData );
	
	// hide all
	FOREACH_ENUM( MusicWheelItemType, i )
	{
		m_sprColorPart[i]->SetVisible( false );
		m_sprNormalPart[i]->SetVisible( false );
	}
	m_TextBanner.SetVisible( false );
	FOREACH_ENUM( MusicWheelItemType, i )
		if( m_pText[i] )
			m_pText[i]->SetVisible( false );
	m_pTextSectionCount->SetVisible( false );
	m_WheelNotifyIcon.SetVisible( false );
	FOREACH_PlayerNumber( p )
		m_pGradeDisplay[p]->SetVisible( false );


	// Fill these in below
	RString sDisplayName, sTranslitName;
	MusicWheelItemType type = MusicWheelItemType_Invalid;

	switch( pWID->m_Type )
	{
	DEFAULT_FAIL( pWID->m_Type );
	case TYPE_SONG:
		type = MusicWheelItemType_Song;

		m_TextBanner.SetFromSong( pWID->m_pSong );
		m_TextBanner.SetDiffuse( pWID->m_color );
		m_TextBanner.SetVisible( true );

		m_WheelNotifyIcon.SetFlags( pWID->m_Flags );
		m_WheelNotifyIcon.SetVisible( true );
		RefreshGrades();
		break;
	case TYPE_SECTION:
		{
			sDisplayName = SONGMAN->ShortenGroupName(pWID->m_sText);
			RString sExpandedSectionName;

			// TODO: Move the expaneded section name into GameState?
			ScreenSelectMusic *pScreen = dynamic_cast<ScreenSelectMusic*>(SCREENMAN->GetTopScreen() );
			if( pScreen )
				sExpandedSectionName = pScreen->GetMusicWheel()->GetExpandedSectionName();
			if( sExpandedSectionName == pWID->m_sText )
				type = MusicWheelItemType_SectionExpanded;
			else
				type = MusicWheelItemType_SectionCollapsed;

			m_pTextSectionCount->SetText( ssprintf("%d",pWID->m_iSectionCount) );
			m_pTextSectionCount->SetVisible( true );
		}
		break;
	case TYPE_COURSE:
		sDisplayName = pWID->m_pCourse->GetDisplayFullTitle();
		sTranslitName = pWID->m_pCourse->GetTranslitFullTitle();
		type = MusicWheelItemType_Course;
		m_WheelNotifyIcon.SetFlags( pWID->m_Flags );
		m_WheelNotifyIcon.SetVisible( true );
		break;
	case TYPE_SORT:
		sDisplayName = pWID->m_sLabel;
		type = MusicWheelItemType_Sort;
		break;
	case TYPE_ROULETTE:
		sDisplayName = THEME->GetString("MusicWheel","Roulette");
		type = MusicWheelItemType_Roulette;
		break;
	case TYPE_RANDOM:
		sDisplayName = THEME->GetString("MusicWheel","Random");
		type = MusicWheelItemType_Roulette;
		break;
	case TYPE_PORTAL:
		sDisplayName = THEME->GetString("MusicWheel","Portal");
		type = MusicWheelItemType_Roulette;
		break;
	}

	m_sprColorPart[type]->SetVisible( true );
	m_sprColorPart[type]->SetDiffuse( pWID->m_color );
	m_sprNormalPart[type]->SetVisible( true );
	BitmapText *bt = m_pText[type];
	if( bt )
	{
		bt->SetText( sDisplayName, sTranslitName );
		bt->SetDiffuse( pWID->m_color );
		bt->SetVisible( true );
	}

	FOREACH_ENUM( MusicWheelItemType, i )
	{
		if( m_sprColorPart[i]->GetVisible() )
		{
			SetGrayBar( m_sprColorPart[i] );
			break;
		}
	}

	// Call "Set" so that elements like TextBanner react to the change in song.
	{
		Message msg( "Set" );
		msg.SetParam( "Song", pWID->m_pSong );
		msg.SetParam( "Course", pWID->m_pCourse );
		msg.SetParam( "Index", iIndex );
		msg.SetParam( "HasFocus", bHasFocus );
		msg.SetParam( "SongGroup", pWID->m_sText );
		this->HandleMessage( msg );
	}
}

void MusicWheelItem::RefreshGrades()
{
	const MusicWheelItemData *pWID = dynamic_cast<const MusicWheelItemData*>( m_pData );

	if( pWID == NULL )
		return; // LoadFromWheelItemData() hasn't been called yet.
	FOREACH_HumanPlayer( p )
	{
		m_pGradeDisplay[p]->SetVisible( false );

		if( pWID->m_pSong == NULL && pWID->m_pCourse == NULL )
			continue;

		Difficulty dc;
		if( GAMESTATE->m_pCurSteps[p] )
			dc = GAMESTATE->m_pCurSteps[p]->GetDifficulty();
		else if( GAMESTATE->m_pCurTrail[p] )
			dc = GAMESTATE->m_pCurTrail[p]->m_CourseDifficulty;
		else
			dc = GAMESTATE->m_PreferredDifficulty[p];

		ProfileSlot ps;
		if( PROFILEMAN->IsPersistentProfile(p) )
			ps = (ProfileSlot)p;
		else if( GRADES_SHOW_MACHINE )
			ps = ProfileSlot_Machine;
		else
			continue;

		StepsType st;
		if( GAMESTATE->m_pCurSteps[p] )
			st = GAMESTATE->m_pCurSteps[p]->m_StepsType;
		else if( GAMESTATE->m_pCurTrail[p] )
			st = GAMESTATE->m_pCurTrail[p]->m_StepsType;
		else
			st = GAMESTATE->m_pCurStyle->m_StepsType;

		m_pGradeDisplay[p]->SetVisible( true );


		Profile *pProfile = PROFILEMAN->GetProfile(ps);

		HighScoreList *pHSL = NULL;
		if( PROFILEMAN->IsPersistentProfile(ps) && dc != Difficulty_Invalid )
		{
			if( pWID->m_pSong )
			{
				const Steps* pSteps = SongUtil::GetStepsByDifficulty( pWID->m_pSong, st, dc );
				if( pSteps != NULL )
					pHSL = &pProfile->GetStepsHighScoreList(pWID->m_pSong, pSteps);
			}
			else if( pWID->m_pCourse )
			{
				const Trail *pTrail = pWID->m_pCourse->GetTrail( st, dc );
				if( pTrail != NULL )
					pHSL = &pProfile->GetCourseHighScoreList( pWID->m_pCourse, pTrail );
			}
		}

		Message msg( "SetGrade" );
		msg.SetParam( "PlayerNumber", p );
		if( pHSL )
		{
			msg.SetParam( "Grade", pHSL->HighGrade );
			msg.SetParam( "NumTimesPlayed", pHSL->GetNumTimesPlayed() );
		}
		m_pGradeDisplay[p]->HandleMessage( msg );
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
