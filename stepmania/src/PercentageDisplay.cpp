#include "global.h"

#include "PercentageDisplay.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "PrefsManager.h"
#include "ActorUtil.h"
#include "RageLog.h"
#include "StageStats.h"


PercentageDisplay::PercentageDisplay()
{
	m_pSource = NULL;
}

void PercentageDisplay::Load( PlayerNumber pn, PlayerStageStats* pSource, const CString &sMetricsGroup, bool bAutoRefresh )
{
	m_PlayerNumber = pn;
	m_pSource = pSource;
	m_bAutoRefresh = bAutoRefresh;
	m_Last = -1;

	DANCE_POINT_DIGITS.Load( sMetricsGroup, "DancePointsDigits" );
	PERCENT_DECIMAL_PLACES.Load( sMetricsGroup, "PercentDecimalPlaces" );
	PERCENT_TOTAL_SIZE.Load( sMetricsGroup, "PercentTotalSize" );
	PERCENT_USE_REMAINDER.Load( sMetricsGroup, "PercentUseRemainder" );


	if( PREFSMAN->m_bDancePointsForOni )
		m_textPercent.SetName( ssprintf("DancePointsP%i", pn+1) );
	else
		m_textPercent.SetName( ssprintf("PercentP%i", pn+1) );

	m_textPercent.LoadFromFont( THEME->GetPathF(sMetricsGroup,"text") );
	ActorUtil::SetXYAndOnCommand( m_textPercent, sMetricsGroup );
	m_textPercent.AddCommand( "Off", THEME->GetMetricA(sMetricsGroup, m_textPercent.GetName()+"OffCommand") );
	this->AddChild( &m_textPercent );

	if( !PREFSMAN->m_bDancePointsForOni && (bool)PERCENT_USE_REMAINDER )
	{
		m_textPercentRemainder.SetName( ssprintf("PercentRemainderP%d",pn+1) );
		m_textPercentRemainder.LoadFromFont( THEME->GetPathF(sMetricsGroup,"remainder") );
		ActorUtil::SetXYAndOnCommand( m_textPercentRemainder, sMetricsGroup );
		m_textPercentRemainder.AddCommand( "Off", THEME->GetMetricA(sMetricsGroup, m_textPercentRemainder.GetName()+"OffCommand") );
		m_textPercentRemainder.SetText( "456" );
		this->AddChild( &m_textPercentRemainder );
	}

	Refresh();
}

void PercentageDisplay::TweenOffScreen()
{
	m_textPercent.PlayCommand( "Off" );
	if( !PREFSMAN->m_bDancePointsForOni && (bool)PERCENT_USE_REMAINDER )
		m_textPercentRemainder.PlayCommand( "Off" );
}

void PercentageDisplay::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );

	if( m_bAutoRefresh )
		Refresh();
}

void PercentageDisplay::Refresh()
{
	const int iActualDancePoints = m_pSource->iActualDancePoints;
	if( iActualDancePoints == m_Last )
		return;

	m_Last = iActualDancePoints;

	CString sNumToDisplay;
	if( PREFSMAN->m_bDancePointsForOni )
	{
		sNumToDisplay = ssprintf( "%*d", (int) DANCE_POINT_DIGITS, max( 0, iActualDancePoints ) );
	}
	else
	{
		float fPercentDancePoints = m_pSource->GetPercentDancePoints();

		// clamp percentage - feedback is that negative numbers look weird here.
		CLAMP( fPercentDancePoints, 0.f, 1.f );

		if( PERCENT_USE_REMAINDER )
		{
			int iPercentWhole = int(fPercentDancePoints*100);
			int iPercentRemainder = int( (fPercentDancePoints*100 - int(fPercentDancePoints*100)) * 10 );
			sNumToDisplay = ssprintf( "%2d", iPercentWhole );
			m_textPercentRemainder.SetText( ssprintf(".%01d%%", iPercentRemainder) );
		}
		else
		{		
			// TRICKY: printf will round, but we want to truncate.  Otherwise, we may display a percent
			// score that's too high and doesn't match up with the calculated grade.
			float fTruncInterval = powf(0.1f, (float) PERCENT_TOTAL_SIZE-1);
			fPercentDancePoints = ftruncf( fPercentDancePoints, fTruncInterval );
			
			sNumToDisplay = ssprintf( "%*.*f%%", (int) PERCENT_TOTAL_SIZE, (int) PERCENT_DECIMAL_PLACES, fPercentDancePoints*100 );
			
			// HACK: Use the last frame in the numbers texture as '-'
			sNumToDisplay.Replace('-','x');
		}
	}

	m_textPercent.SetText( sNumToDisplay );
}

/*
 * (c) 2001-2003 Chris Danford
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
