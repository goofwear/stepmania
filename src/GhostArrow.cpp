#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: GhostArrow

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Ben Nordstrom
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "GhostArrow.h"
#include "NoteSkinManager.h"

GhostArrow::GhostArrow()
{
	for( int i=0; i<NUM_TAP_NOTE_SCORES; i++ )
	{
		m_spr[i].SetHidden( true );
		this->AddChild( &m_spr[i] );
	}
}

void GhostArrow::Load( CString sNoteSkin, CString sButton, CString sElement )
{
	for( int i=0; i<NUM_TAP_NOTE_SCORES; i++ )
	{
		CString sJudge = TapNoteScoreToString( (TapNoteScore)i );
		
		CString sFullElement = sElement  + " " + sJudge;

		// HACK: for backward noteskin compatibility
		CString sPath = NOTESKIN->GetPathToFromNoteSkinAndButton(sNoteSkin, sButton, sFullElement, true);	// optional
		if( sPath.empty() )
			sPath = NOTESKIN->GetPathToFromNoteSkinAndButton(sNoteSkin, sButton, sElement);	// not optional
		m_spr[i].Load( sPath );
	}

	for( int i=0; i<NUM_TAP_NOTE_SCORES; i++ )
	{
		CString sJudge = TapNoteScoreToString( (TapNoteScore)i );
		CString sCommand = Capitalize(sJudge)+"Command";
		m_sScoreCommand[i] = NOTESKIN->GetMetric( sNoteSkin, m_sName, sCommand );
	}
}

void GhostArrow::Init( PlayerNumber pn )
{
	m_PlayerNumber = pn;
}

void GhostArrow::Step( TapNoteScore score )
{
	for( int i=0; i<NUM_TAP_NOTE_SCORES; i++ )
	{
		// HACK: never hide the mine explosion
		if( i == TNS_HIT_MINE )
			continue;
		m_spr[i].StopTweening();
		m_spr[i].SetHidden( true );
	}

	m_spr[score].SetHidden( false );
	m_spr[score].StopTweening();
	m_spr[score].Command( m_sScoreCommand[score] );
}
