#include "global.h"
#include "PaneDisplay.h"
#include "ThemeManager.h"
#include "GameState.h"
#include "song.h"
#include "Steps.h"
#include "RageLog.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "Course.h"
#include "Style.h"
#include "ActorUtil.h"
#include "Foreach.h"
#include "LuaManager.h"
#include "XmlFile.h"
#include "PlayerStageStats.h"

#define SHIFT_X(pc)	THEME->GetMetricF(sMetricsGroup, ssprintf("ShiftP%iX", pc+1))
#define SHIFT_Y(pc)	THEME->GetMetricF(sMetricsGroup, ssprintf("ShiftP%iY", pc+1))

static const char *PaneCategoryNames[] = {
	"NumSteps",
	"Jumps",
	"Holds",
	"Rolls",
	"Mines",
	"Hands",
	"MachineHighScore",
	"MachineHighName",
	"ProfileHighScore",
};
XToString( PaneCategory );
LuaXType( PaneCategory );


enum { NEED_NOTES=1, NEED_PROFILE=2 };
struct Content_t
{
	int req;
	RString sFontType;
};

static const Content_t g_Contents[NUM_PaneCategory] =
{
	{ NEED_NOTES, "count" },
	{ NEED_NOTES, "count" },
	{ NEED_NOTES, "count" },
	{ NEED_NOTES, "count" },
	{ NEED_NOTES, "count" },
	{ NEED_NOTES, "count" },
	{ NEED_NOTES, "score" },
	{ NEED_NOTES, "name" },
	{ NEED_NOTES|NEED_PROFILE, "score" },
};

REGISTER_ACTOR_CLASS( PaneDisplay )

void PaneDisplay::Load( const RString &sMetricsGroup, PlayerNumber pn )
{
	m_PlayerNumber = pn;

	EMPTY_MACHINE_HIGH_SCORE_NAME.Load( sMetricsGroup, "EmptyMachineHighScoreName" );
	NOT_AVAILABLE.Load( sMetricsGroup, "NotAvailable" );
	COUNT_FORMAT.Load( sMetricsGroup, "CountFormat" );


	FOREACH_ENUM( PaneCategory, pc )
	{
		LuaThreadVariable var( "PaneCategory", LuaReference::Create(pc) );

		RString sFontType = g_Contents[pc].sFontType;

		m_textContents[pc].LoadFromFont( THEME->GetPathF(sMetricsGroup,sFontType) );
		m_textContents[pc].SetName( PaneCategoryToString(pc) + "Text" );
		ActorUtil::LoadAllCommands( m_textContents[pc], sMetricsGroup );
		ActorUtil::SetXY( m_textContents[pc], sMetricsGroup );
		m_ContentsFrame.AddChild( &m_textContents[pc] );

		m_Labels[pc].Load( THEME->GetPathG(sMetricsGroup,"label " + PaneCategoryToString(pc)) );
		m_Labels[pc]->SetName( PaneCategoryToString(pc) + "Label" );
		ActorUtil::LoadAllCommands( *m_Labels[pc], sMetricsGroup );
		ActorUtil::SetXY( m_Labels[pc], sMetricsGroup );
		m_ContentsFrame.AddChild( m_Labels[pc] );

		ActorUtil::LoadAllCommandsFromName( m_textContents[pc], sMetricsGroup, PaneCategoryToString(pc) );
	}

	m_ContentsFrame.SetXY( SHIFT_X(m_PlayerNumber), SHIFT_Y(m_PlayerNumber) );
	this->AddChild( &m_ContentsFrame );
}

void PaneDisplay::LoadFromNode( const XNode *pNode )
{
	bool b;

	RString sMetricsGroup;
	b = pNode->GetAttrValue( "MetricsGroup", sMetricsGroup );
	ASSERT( b );

	Lua *L = LUA->Get();
	b = pNode->PushAttrValue( L, "PlayerNumber" );
	ASSERT( b );
	PlayerNumber pn;
	LuaHelpers::Pop( L, pn );
	LUA->Release( L );

	Load( sMetricsGroup, pn );

	ActorFrame::LoadFromNode( pNode );
}

void PaneDisplay::SetContent( PaneCategory c )
{
	RString str = "";	// fill this in
	float val = 0;	// fill this in

	const Song *pSong = GAMESTATE->m_pCurSong;
	const Steps *pSteps = GAMESTATE->m_pCurSteps[m_PlayerNumber];
	const Course *pCourse = GAMESTATE->m_pCurCourse;
	const Trail *pTrail = GAMESTATE->m_pCurTrail[m_PlayerNumber];
	const Profile *pProfile = PROFILEMAN->IsPersistentProfile(m_PlayerNumber) ? PROFILEMAN->GetProfile(m_PlayerNumber) : NULL;
	bool bIsPlayerEdit = pSteps && pSteps->IsAPlayerEdit();

	if( (g_Contents[c].req&NEED_NOTES) && !pSteps && !pTrail )
		goto done;
	if( (g_Contents[c].req&NEED_PROFILE) && !pProfile )
	{
		str = NOT_AVAILABLE;
		goto done;
	}

	{
		RadarValues rv;
		HighScoreList *pHSL = NULL;
		ProfileSlot slot = ProfileSlot_Machine;
		switch( c )
		{
		case PaneCategory_ProfileHighScore:
			slot = (ProfileSlot) m_PlayerNumber;
		}

		if( pSteps )
		{
			rv = pSteps->GetRadarValues( m_PlayerNumber );
			pHSL = &PROFILEMAN->GetProfile(slot)->GetStepsHighScoreList(pSong, pSteps);
		}
		else if( pTrail )
		{
			rv = pTrail->GetRadarValues();
			pHSL = &PROFILEMAN->GetProfile(slot)->GetCourseHighScoreList(pCourse, pTrail);
		}

		switch( c )
		{
		case PaneCategory_NumSteps:	val = rv[RadarCategory_TapsAndHolds]; break;
		case PaneCategory_Jumps:	val = rv[RadarCategory_Jumps]; break;
		case PaneCategory_Holds:	val = rv[RadarCategory_Holds]; break;
		case PaneCategory_Rolls:	val = rv[RadarCategory_Rolls]; break;
		case PaneCategory_Mines:	val = rv[RadarCategory_Mines]; break;
		case PaneCategory_Hands:	val = rv[RadarCategory_Hands]; break;
		case PaneCategory_ProfileHighScore:
		case PaneCategory_MachineHighName: /* set val for color */
		case PaneCategory_MachineHighScore:
			CHECKPOINT;
			val = pHSL->GetTopScore().GetPercentDP();
			break;
		};

		if( val == RADAR_VAL_UNKNOWN )
			goto done;

		switch( c )
		{
		case PaneCategory_MachineHighName:
			if( pHSL->vHighScores.empty() )
			{
				str = EMPTY_MACHINE_HIGH_SCORE_NAME;
			}
			else
			{
				str = pHSL->GetTopScore().GetName();
				if( str.empty() )
					str = "????";
			}
			break;
		case PaneCategory_MachineHighScore:
		case PaneCategory_ProfileHighScore:
			// Don't show or save machine high scores for edits loaded from a player profile.
			if( bIsPlayerEdit )
				str = NOT_AVAILABLE;
			else
				str = PlayerStageStats::FormatPercentScore( val );
			break;
		case PaneCategory_NumSteps:
		case PaneCategory_Jumps:
		case PaneCategory_Holds:
		case PaneCategory_Rolls:
		case PaneCategory_Mines:
		case PaneCategory_Hands:
			str = ssprintf( COUNT_FORMAT.GetValue(), val );
		}
	}


done:
	m_textContents[c].SetText( str );

	Lua *L = LUA->Get();

	m_textContents[c].PushSelf( L );
	lua_pushstring( L, "PaneLevel" );
	lua_pushnumber( L, val );
	lua_settable( L, -3 );
	lua_pop( L, 1 );

	m_textContents[c].PlayCommand( "Level" );

	LUA->Release(L);
}

void PaneDisplay::SetFromGameState()
{
	/* Don't update text that doesn't apply to the current mode.  It's still tweening off. */
	FOREACH_ENUM( PaneCategory, i )
		SetContent( i );
}

// lua start
#include "LuaBinding.h"

class LunaPaneDisplay: public Luna<PaneDisplay>
{
public:
	static int SetFromGameState( T* pc, lua_State *L )	{ pc->SetFromGameState(); return 0; }

	LunaPaneDisplay()
	{
		ADD_METHOD( SetFromGameState );
	}
};

LUA_REGISTER_DERIVED_CLASS( PaneDisplay, ActorFrame )
// lua end

/*
 * (c) 2003 Glenn Maynard
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
