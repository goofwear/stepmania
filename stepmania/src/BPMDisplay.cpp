#include "global.h"
#include "BPMDisplay.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "GameState.h"
#include "Course.h"
#include "Style.h"
#include "ActorUtil.h"
#include "CommonMetrics.h"
#include "Song.h"

#include <limits.h>

REGISTER_ACTOR_CLASS( BPMDisplay )

BPMDisplay::BPMDisplay()
{
	m_fBPMFrom = m_fBPMTo = 0;
	m_iCurrentBPM = 0;
	m_BPMS.push_back(0);
	m_fPercentInState = 0;
	m_fCycleTime = 1.0f;
}

void BPMDisplay::Load()
{
	SET_NO_BPM_COMMAND.Load( m_sName, "SetNoBpmCommand");
	SET_NORMAL_COMMAND.Load( m_sName, "SetNormalCommand");
	SET_CHANGING_COMMAND.Load( m_sName, "SetChangeCommand" );
	SET_RANDOM_COMMAND.Load( m_sName, "SetRandomCommand" );
	SET_EXTRA_COMMAND.Load( m_sName, "SetExtraCommand" );
	CYCLE.Load( m_sName, "Cycle" );
	RANDOM_CYCLE_SPEED.Load( m_sName, "RandomCycleSpeed" );
	SEPARATOR.Load( m_sName, "Separator" );
	SHOW_QMARKS.Load( m_sName, "ShowQMarksInRandomCycle" );
	NO_BPM_TEXT.Load( m_sName, "NoBpmText" );

	RunCommands( SET_NORMAL_COMMAND );
}

void BPMDisplay::LoadFromNode( const XNode *pNode )
{
	BitmapText::LoadFromNode( pNode );
	Load();
}

float BPMDisplay::GetActiveBPM() const
{
	return m_fBPMTo + (m_fBPMFrom-m_fBPMTo)*m_fPercentInState;
}

void BPMDisplay::Update( float fDeltaTime ) 
{ 
	BitmapText::Update( fDeltaTime ); 

	if( !(bool)CYCLE )
		return;
	if( m_BPMS.size() == 0 )
		return; /* no bpm */

	m_fPercentInState -= fDeltaTime / m_fCycleTime;
	if( m_fPercentInState < 0 )
	{
		// go to next state
		m_fPercentInState = 1;		// reset timer

		m_iCurrentBPM = (m_iCurrentBPM + 1) % m_BPMS.size();
		m_fBPMFrom = m_fBPMTo;
		m_fBPMTo = m_BPMS[m_iCurrentBPM];

		if(m_fBPMTo == -1)
		{
			m_fBPMFrom = -1;

			if( (bool)SHOW_QMARKS )
				SetText( (RandomFloat(0,1)>0.90f) ? RString("xxx") : ssprintf("%03.0f",RandomFloat(0,999)) );
			else
				SetText( ssprintf("%03.0f",RandomFloat(0,999)) );
		}
		else if(m_fBPMFrom == -1)
		{
			m_fBPMFrom = m_fBPMTo;
		}
	}

	if( m_fBPMTo != -1)
	{
		const float fActualBPM = GetActiveBPM();
		SetText( ssprintf("%03.0f", fActualBPM) );
	}
}


void BPMDisplay::SetBPMRange( const DisplayBpms &bpms )
{
	ASSERT( !bpms.vfBpms.empty() );

	m_BPMS.clear();

	const vector<float> &BPMS = bpms.vfBpms;

	bool AllIdentical = true;
	for( unsigned i = 0; i < BPMS.size(); ++i )
	{
		if( i > 0 && BPMS[i] != BPMS[i-1] )
			AllIdentical = false;
	}

	if( !(bool)CYCLE )
	{
		int MinBPM = INT_MAX;
		int MaxBPM = INT_MIN;
		for( unsigned i = 0; i < BPMS.size(); ++i )
		{
			MinBPM = min( MinBPM, (int)lrintf(BPMS[i]) );
			MaxBPM = max( MaxBPM, (int)lrintf(BPMS[i]) );
		}
		if( MinBPM == MaxBPM )
		{
			if( MinBPM == -1 )
			{
				SetText( "..." ); // random
				IsRandom = true;
			}
			else
				SetText( ssprintf("%i", MinBPM) );
		}
		else
		{
			SetText( ssprintf("%i%s%i", MinBPM, SEPARATOR.GetValue().c_str(), MaxBPM) );
		}
	}
	else
	{
		for( unsigned i = 0; i < BPMS.size(); ++i )
		{
			m_BPMS.push_back(BPMS[i]);
			if( BPMS[i] != -1 )
				m_BPMS.push_back(BPMS[i]); // hold
		}

		m_iCurrentBPM = min(1u, m_BPMS.size()); // start on the first hold
		m_fBPMFrom = BPMS[0];
		m_fBPMTo = BPMS[0];
		m_fPercentInState = 1;
	}

	if( GAMESTATE->IsAnExtraStage() )
		RunCommands( SET_EXTRA_COMMAND );
	else if( !AllIdentical )
		RunCommands( SET_CHANGING_COMMAND );
	else
		RunCommands( SET_NORMAL_COMMAND );
}

void BPMDisplay::CycleRandomly()
{
	DisplayBpms bpms;
	bpms.Add(-1);
	SetBPMRange( bpms );

	RunCommands( SET_RANDOM_COMMAND );

	RString sValue = RANDOM_CYCLE_SPEED;
	sValue.MakeLower();

	if( stricmp(sValue, "vslow") == 0 )
		m_fCycleTime = 1.0f;
	else if( stricmp(sValue, "slow") == 0 )
		m_fCycleTime = 0.5f;
	else if( stricmp(sValue, "fast") == 0 )
		m_fCycleTime = 0.1f;
	else if( stricmp(sValue, "vfast") == 0 )
		m_fCycleTime = 0.05f;
	else if( stricmp(sValue, "hyper") == 0 )
		m_fCycleTime = 0.01f;
	else
		m_fCycleTime = 0.2f;
}

void BPMDisplay::NoBPM()
{
	m_BPMS.clear();
	SetText( NO_BPM_TEXT ); 
	RunCommands( SET_NO_BPM_COMMAND );
}

void BPMDisplay::SetBpmFromSong( const Song* pSong )
{
	ASSERT( pSong );
	switch( pSong->m_DisplayBPMType )
	{
	case Song::DISPLAY_ACTUAL:
	case Song::DISPLAY_SPECIFIED:
		{
			DisplayBpms bpms;
			pSong->GetDisplayBpms( bpms );
			SetBPMRange( bpms );
			m_fCycleTime = 1.0f;
		}
		break;
	case Song::DISPLAY_RANDOM:
		CycleRandomly();
		break;
	default:
		ASSERT(0);
	}
}

void BPMDisplay::SetBpmFromCourse( const Course* pCourse )
{
	ASSERT( pCourse );

	StepsType st = GAMESTATE->GetCurrentStyle()->m_StepsType;
	Trail *pTrail = pCourse->GetTrail( st );
	ASSERT( pTrail );

	m_fCycleTime = 0.2f;

	if( (int)pTrail->m_vEntries.size() > CommonMetrics::MAX_COURSE_ENTRIES_BEFORE_VARIOUS )
	{
		SetVarious();
		return;
	}

	DisplayBpms bpms;
	pTrail->GetDisplayBpms( bpms );
	SetBPMRange( bpms );
}

void BPMDisplay::SetConstantBpm( float fBPM )
{
	DisplayBpms bpms;
	bpms.Add( fBPM );
	SetBPMRange( bpms );
}

void BPMDisplay::SetVarious()
{
	m_BPMS.clear();
	m_BPMS.push_back( -1 );
	SetText( "Various" );
}

void BPMDisplay::SetFromGameState()
{
	if( GAMESTATE->m_pCurSong.Get() )
	{
		if( GAMESTATE->IsAnExtraStage() )
			CycleRandomly();				
		else
			SetBpmFromSong( GAMESTATE->m_pCurSong );
		return;
	}
	if( GAMESTATE->m_pCurCourse.Get() )
	{
		SetBpmFromCourse( GAMESTATE->m_pCurCourse );

		return;
	}

	NoBPM();
}

class SongBPMDisplay: public BPMDisplay
{
public:
	SongBPMDisplay();
	virtual SongBPMDisplay *Copy() const;
	virtual void Update( float fDeltaTime ); 

private:
	float m_fLastGameStateBPM;

};

SongBPMDisplay::SongBPMDisplay()
{
	m_fLastGameStateBPM = 0;
}

void SongBPMDisplay::Update( float fDeltaTime ) 
{
	float fGameStateBPM = GAMESTATE->m_fCurBPS * 60.0f;
	if( m_fLastGameStateBPM != fGameStateBPM )
	{
		m_fLastGameStateBPM = fGameStateBPM;
		SetConstantBpm( fGameStateBPM );
	}

	BPMDisplay::Update( fDeltaTime );
}

REGISTER_ACTOR_CLASS( SongBPMDisplay )

#include "LuaBinding.h"
class LunaBPMDisplay: public Luna<BPMDisplay>
{
public:
	static int SetFromGameState( T* p, lua_State *L ) { p->SetFromGameState(); return 0; }

	LunaBPMDisplay()
	{
		ADD_METHOD( SetFromGameState );
	}
};

LUA_REGISTER_DERIVED_CLASS( BPMDisplay, BitmapText )

/*
 * (c) 2001-2002 Chris Danford
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
