#include "global.h"
#include "HoldJudgment.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "ThemeManager.h"
#include "ThemeMetric.h"
#include "ActorUtil.h"
#include "StatsManager.h"
#include "XmlFile.h"

static ThemeMetric<apActorCommands>	HELD_COMMAND	("HoldJudgment","HeldCommand");
static ThemeMetric<apActorCommands>	LET_GO_COMMAND	("HoldJudgment","LetGoCommand");

REGISTER_ACTOR_CLASS( HoldJudgment )

HoldJudgment::HoldJudgment()
{
	m_mpToTrack = MultiPlayer_INVALID;
}

void HoldJudgment::Load( const RString &sPath )
{
	m_sprJudgment.Load( sPath );
	m_sprJudgment->StopAnimating();
	ResetAnimation();
	this->AddChild( m_sprJudgment );
}

void HoldJudgment::LoadFromNode( const RString& sDir, const XNode* pNode )
{
	RString sFile;
	if( !pNode->GetAttrValue("File", sFile) )
		RageException::Throw( ssprintf("HoldJudgment node in '%s' is missing the attribute \"File\"", sDir.c_str()) );
	LuaHelpers::RunAtExpressionS( sFile );

	if( sFile.Left(1) != "/" )
		sFile = sDir+sFile;

	CollapsePath( sFile );

	Load( sFile );

	ActorFrame::LoadFromNode( sDir, pNode );
}

void HoldJudgment::ResetAnimation()
{
	ASSERT( m_sprJudgment.IsLoaded() );
	m_sprJudgment->SetDiffuse( RageColor(1,1,1,0) );
	m_sprJudgment->SetXY( 0, 0 );
	m_sprJudgment->StopTweening();
	m_sprJudgment->StopEffect();
}

void HoldJudgment::SetHoldJudgment( HoldNoteScore hns )
{
	//LOG->Trace( "Judgment::SetJudgment()" );

	ResetAnimation();

	switch( hns )
	{
	case HNS_None:
		ASSERT(0);
	case HNS_Held:
		m_sprJudgment->SetState( 0 );
		m_sprJudgment->RunCommands( HELD_COMMAND );
		break;
	case HNS_LetGo:
		m_sprJudgment->SetState( 1 );
		m_sprJudgment->RunCommands( LET_GO_COMMAND );
		break;
	default:
		ASSERT(0);
	}
}

void HoldJudgment::LoadFromMultiPlayer( MultiPlayer mp )
{
	ASSERT( m_mpToTrack == MultiPlayer_INVALID );	// assert only load once
	m_mpToTrack = mp;
	this->SubscribeToMessage( enum_add2(Message_ShowHoldJudgmentMuliPlayerP1,m_mpToTrack) );
}

void HoldJudgment::HandleMessage( const RString &sMessage )
{
	ASSERT( m_mpToTrack != MultiPlayer_INVALID );
	if( sMessage == MessageToString( enum_add2(Message_ShowHoldJudgmentMuliPlayerP1,m_mpToTrack) ) )
		SetHoldJudgment( STATSMAN->m_CurStageStats.m_multiPlayer[m_mpToTrack].hnsLast );

	ActorFrame::HandleMessage( sMessage );
}

// lua start
#include "LuaBinding.h"

class LunaHoldJudgment: public Luna<HoldJudgment>
{
public:
	LunaHoldJudgment() { LUA->Register( Register ); }

	static int LoadFromMultiPlayer( T* p, lua_State *L ) { p->LoadFromMultiPlayer( (MultiPlayer)IArg(1) ); return 0; }

	static void Register(lua_State *L) 
	{
		ADD_METHOD( LoadFromMultiPlayer );

		Luna<T>::Register( L );
	}
};

LUA_REGISTER_DERIVED_CLASS( HoldJudgment, ActorFrame )
// lua end

/*
 * (c) 2001-2004 Chris Danford
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
