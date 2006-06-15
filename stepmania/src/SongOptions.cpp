#include "global.h"
#include "SongOptions.h"
#include "RageUtil.h"
#include "GameState.h"
#include "CommonMetrics.h"

void SongOptions::Init() 
{
	m_LifeType = LIFE_BAR;
	m_DrainType = DRAIN_NORMAL;
	m_iBatteryLives = 4;
	m_FailType = FAIL_IMMEDIATE;
	m_bAssistTick = false;
	m_fMusicRate = 1.0f;
	m_AutosyncType = AUTOSYNC_OFF;
	m_bSaveScore = true;
}

void SongOptions::GetMods( vector<RString> &AddTo ) const
{
	switch( m_LifeType )
	{
	case LIFE_BAR:		
		switch( m_DrainType )
		{
		case DRAIN_NORMAL:						break;
		case DRAIN_NO_RECOVER:		AddTo.push_back("NoRecover");	break;
		case DRAIN_SUDDEN_DEATH:	AddTo.push_back("SuddenDeath");	break;
		}
		break;
	case LIFE_BATTERY:
		AddTo.push_back( ssprintf( "%dLives", m_iBatteryLives ) );
		break;
	case LIFE_TIME:
		AddTo.push_back( "LifeTime" );
		break;
	default:	ASSERT(0);
	}


	switch( m_FailType )
	{
	case FAIL_IMMEDIATE:						break;
	case FAIL_END_OF_SONG:	AddTo.push_back("FailEndOfSong");	break;
	case FAIL_OFF:		AddTo.push_back("FailOff");		break;
	default:		ASSERT(0);
	}

	if( m_fMusicRate != 1 )
	{
		RString s = ssprintf( "%2.2f", m_fMusicRate );
		if( s[s.size()-1] == '0' )
			s.erase( s.size()-1 );
		AddTo.push_back( s + "xMusic" );
	}

	switch( m_AutosyncType )
	{
	case AUTOSYNC_OFF:						break;
	case AUTOSYNC_SONG:	AddTo.push_back("AutosyncSong");	break;
	case AUTOSYNC_MACHINE:	AddTo.push_back("AutosyncMachine");	break;
	default:		ASSERT(0);
	}
}

void SongOptions::GetLocalizedMods( vector<RString> &AddTo ) const
{
	vector<RString> vMods;
	GetMods( vMods );
	FOREACH( RString, vMods, s )
	{
		*s = CommonMetrics::LocalizeOptionItem( *s, true );
	}
}

RString SongOptions::GetString() const
{
	vector<RString> v;
	GetMods( v );
	return join( ", ", v );
}

RString SongOptions::GetLocalizedString() const
{
	vector<RString> v;
	GetLocalizedMods( v );
	return join( ", ", v );
}


/* Options are added to the current settings; call Init() beforehand if
 * you don't want this. */
void SongOptions::FromString( const RString &sOptions )
{
//	Init();
	RString sTemp = sOptions;
	sTemp.MakeLower();
	vector<RString> asBits;
	split( sTemp, ",", asBits, true );

	for( unsigned i=0; i<asBits.size(); i++ )
	{
		RString& sBit = asBits[i];
		TrimLeft(sBit);
		TrimRight(sBit);
		
		Regex mult("^([0-9]+(\\.[0-9]+)?)xmusic$");
		vector<RString> matches;
		if( mult.Compare(sBit, matches) )
			m_fMusicRate = StringToFloat( matches[0] );

		matches.clear();
		Regex lives("^([0-9]+) ?(lives|life)$");
		if( lives.Compare(sBit, matches) )
			m_iBatteryLives = atoi( matches[0] );

		vector<RString> asParts;
		split( sBit, " ", asParts, true );
		bool on = true;
		if( asParts.size() > 1 )
		{
			sBit = asParts[1];

			if( asParts[0] == "no" )
				on = false;
		}

		if(	 sBit == "norecover" )	m_DrainType = DRAIN_NO_RECOVER;
		else if( sBit == "suddendeath" )	m_DrainType = DRAIN_SUDDEN_DEATH;
		else if( sBit == "power-drop" )		m_DrainType = DRAIN_NO_RECOVER;
		else if( sBit == "death" )		m_DrainType = DRAIN_SUDDEN_DEATH;
		else if( sBit == "normal-drain" )	m_DrainType = DRAIN_NORMAL;
		else if( sBit == "failarcade" || 
			 sBit == "failimmediate" )	m_FailType = FAIL_IMMEDIATE;
		else if( sBit == "failendofsong" )	m_FailType = FAIL_END_OF_SONG;
		else if( sBit == "failoff" )		m_FailType = FAIL_OFF;
		
		else if( sBit == "faildefault" )
		{
			SongOptions so;
			GAMESTATE->GetDefaultSongOptions( so );
			m_FailType = so.m_FailType;
		}

		else if( sBit == "assisttick" )		m_bAssistTick = on;
		else if( sBit == "autosync" || sBit == "autosyncsong" )
							m_AutosyncType = on ? AUTOSYNC_SONG : AUTOSYNC_OFF;
		else if( sBit == "autosyncmachine" )	
							m_AutosyncType = on ? AUTOSYNC_MACHINE : AUTOSYNC_OFF;
		else if( sBit == "savescore" )		m_bSaveScore = on;
		else if( sBit == "bar" )		m_LifeType = LIFE_BAR;
		else if( sBit == "battery" )		m_LifeType = LIFE_BATTERY;
		else if( sBit == "lifetime" )		m_LifeType = LIFE_TIME;
	}
}

bool SongOptions::operator==( const SongOptions &other ) const
{
#define COMPARE(x) { if( x != other.x ) return false; }
	COMPARE( m_LifeType );
	COMPARE( m_DrainType );
	COMPARE( m_iBatteryLives );
	COMPARE( m_FailType );
	COMPARE( m_fMusicRate );
	COMPARE( m_bAssistTick );
	COMPARE( m_AutosyncType );
	COMPARE( m_bSaveScore );
#undef COMPARE
	return true;
}

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
