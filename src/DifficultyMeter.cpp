#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: DifficultyMeter

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "DifficultyMeter.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "GameState.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "Course.h"
#include "SongManager.h"
#include "ActorUtil.h"


#define NUM_FEET_IN_METER						THEME->GetMetricI(m_sName,"NumFeetInMeter")
#define MAX_FEET_IN_METER						THEME->GetMetricI(m_sName,"MaxFeetInMeter")
#define GLOW_IF_METER_GREATER_THAN				THEME->GetMetricI(m_sName,"GlowIfMeterGreaterThan")
#define SHOW_FEET								THEME->GetMetricB(m_sName,"ShowFeet")
/* "easy", "hard" */
#define SHOW_DIFFICULTY							THEME->GetMetricB(m_sName,"ShowDifficulty")
/* 3, 9 */
#define SHOW_METER								THEME->GetMetricB(m_sName,"ShowMeter")


DifficultyMeter::DifficultyMeter()
{
}

/* sID experiment:
 *
 * Names of an actor, "Foo":
 * [Foo]
 * Metric=abc
 *
 * [ScreenSomething]
 * FooP1X=20
 * FooP2Y=30
 *
 * Graphics\Foo under p1
 *
 * We want to call it different things in different contexts: we may only want one
 * set of internal metrics for a given use, but separate metrics for each player at
 * the screen level, and we may or may not want separate names at the asset level.
 *
 * As is, we tend to end up having to either duplicate [Foo] to [FooP1] and [FooP2]
 * or not use m_sName for [Foo], which limits its use.  Let's try using a separate
 * name for internal metrics.  I'm not sure if this will cause more confusion than good,
 * so I'm trying it first in only this object.
 */

void DifficultyMeter::Load()
{
	if( SHOW_FEET )
	{
		m_textFeet.SetName( "Feet" );
		m_textFeet.LoadFromTextureAndChars( THEME->GetPathToG( ssprintf("%s bar 2x1", m_sName.c_str())), "10" );
		SET_XY_AND_ON_COMMAND( &m_textFeet );
		this->AddChild( &m_textFeet );
	}

	if( SHOW_DIFFICULTY )
	{
		m_Difficulty.Load( THEME->GetPathToG( ssprintf("%s difficulty", m_sName.c_str())) );
		m_Difficulty->SetName( "Difficulty" );
		SET_XY_AND_ON_COMMAND( m_Difficulty );
		this->AddChild( m_Difficulty );
	}

	if( SHOW_METER )
	{
		m_textMeter.SetName( "Meter" );
		m_textMeter.LoadFromNumbers( THEME->GetPathToN( ssprintf("%s meter", m_sName.c_str())) );
		SET_XY_AND_ON_COMMAND( m_textMeter );
		this->AddChild( &m_textMeter );
	}

	Unset();
}

void DifficultyMeter::SetFromNotes( const Steps* pNotes )
{
	if( pNotes == NULL )
	{
		Unset();
		return;
	}

	SetMeter( pNotes->GetMeter() );
	m_textFeet.SetDiffuse( SONGMAN->GetDifficultyColor(pNotes->GetDifficulty()) );

	SetDifficulty( DifficultyToString( pNotes->GetDifficulty() ) );
}

void DifficultyMeter::SetFromCourse( const Course* pCourse )
{
	if( pCourse == NULL )
	{
		Unset();
		return;
	}

	const int meter = (int) roundf(pCourse->GetMeter());
	SetMeter( meter );
	
	// XXX
	Difficulty FakeDifficulty;
	if( meter <= 1 )
		FakeDifficulty = DIFFICULTY_BEGINNER;
	else if( meter <= 2 )
		FakeDifficulty = DIFFICULTY_EASY;
	else if( meter <= 5 )
		FakeDifficulty = DIFFICULTY_MEDIUM;
	else if( meter <= 7 )
		FakeDifficulty = DIFFICULTY_HARD;
	else
		FakeDifficulty = DIFFICULTY_CHALLENGE;

	RageColor c = SONGMAN->GetDifficultyColor( FakeDifficulty );
	m_textFeet.SetDiffuse( c );

	SetDifficulty( DifficultyToString(FakeDifficulty) + "Course" );
}

void DifficultyMeter::Unset()
{
	m_textFeet.SetEffectNone();
	m_textFeet.SetDiffuse( RageColor(0.8f,0.8f,0.8f,1) );
	SetMeter( 0 );
	SetDifficulty( "None" );
}

void DifficultyMeter::SetFromGameState( PlayerNumber pn )
{
	if( GAMESTATE->m_pCurNotes[pn] )
		SetFromNotes( GAMESTATE->m_pCurNotes[pn] );
	else if( GAMESTATE->m_pCurCourse )
		SetFromCourse( GAMESTATE->m_pCurCourse );
	else
		Unset();
}

void DifficultyMeter::SetMeter( int iMeter )
{
	if( SHOW_FEET )
	{
		CString sNewText;
		int f;
		for( f=0; f<NUM_FEET_IN_METER; f++ )
			sNewText += (f<iMeter) ? "1" : "0";
		for( f=NUM_FEET_IN_METER; f<MAX_FEET_IN_METER; f++ )
			if( f<iMeter )
				sNewText += "1";

		m_textFeet.SetText( sNewText );

		if( iMeter > GLOW_IF_METER_GREATER_THAN )
			m_textFeet.SetEffectGlowShift();
		else
			m_textFeet.SetEffectNone();
	}

	if( SHOW_METER )
	{
		const CString sMeter = ssprintf( "%i", iMeter );
		if( sMeter != m_textMeter.GetText() )
			m_textMeter.SetText( sMeter );
	}
}

void DifficultyMeter::SetDifficulty( CString diff )
{
	if( m_CurDifficulty == diff )
		return;
	m_CurDifficulty = diff;

	if( SHOW_DIFFICULTY )
		COMMAND( m_Difficulty, "Set" + Capitalize(diff) );
	if( SHOW_METER )
		COMMAND( m_textMeter, "Set" + Capitalize(diff) );
}
