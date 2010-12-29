#include "global.h"
#include "GameConstantsAndTypes.h"
#include "GameState.h"
#include "RageUtil.h"
#include "ThemeMetric.h"
#include "EnumHelper.h"
#include "Foreach.h"
#include "LuaManager.h"
#include "GameManager.h"
#include <float.h>

#include "LuaFunctions.h"
#include "LocalizedString.h"

const RString RANKING_TO_FILL_IN_MARKER[NUM_PLAYERS] = {"#P1#","#P2#"};

extern const RString GROUP_ALL = "---Group All---";

static const char *RadarCategoryNames[] = {
	"Stream",
	"Voltage",
	"Air",
	"Freeze",
	"Chaos",
	"TapsAndHolds",
	"Jumps",
	"Holds",
	"Mines",
	"Hands",
	"Rolls"
};
XToString( RadarCategory, NUM_RadarCategory );
XToLocalizedString( RadarCategory );
LuaFunction( RadarCategoryToLocalizedString, RadarCategoryToLocalizedString((RadarCategory) IArg(1)) );

static void LuaRadarCategory(lua_State* L)
{
	FOREACH_RadarCategory( rc )
	{
		RString s = RadarCategoryToString( rc );
		LUA->SetGlobal( "RadarCategory_"+s, rc );
	}
}
REGISTER_WITH_LUA_FUNCTION( LuaRadarCategory );

static const char *StepsTypeNames[] = {
	"dance-single",
	"dance-double",
	"dance-couple",
	"dance-solo",
	"dance-routine",
	"pump-single",
	"pump-halfdouble",
	"pump-double",
	"pump-couple",
	"ez2-single",	// Single: TL,LHH,D,RHH,TR
	"ez2-double",	// Double: Single x2
	"ez2-real",	// Real: TL,LHH,LHL,D,RHL,RHH,TR
	"para-single",
	"para-versus",
	"ds3ddx-single",
	"bm-single5",	// called "bm" for backward compat
	"bm-double5",	// called "bm" for backward compat
	"bm-single7",	// called "bm" for backward compat
	"bm-double7",	// called "bm" for backward compat
	"maniax-single",
	"maniax-double",
	"techno-single4",
	"techno-single5",
	"techno-single8",
	"techno-double4",
	"techno-double5",
	"pnm-five",
	"pnm-nine",
	"lights-cabinet",
};
XToString( StepsType, NUM_StepsType );
StringToX( StepsType );

static void LuaStepsType(lua_State* L)
{
	FOREACH_StepsType( st )
	{
		RString s = GAMEMAN->StepsTypeToString( st );
		s.MakeUpper();
		s.Replace('-','_');
		LUA->SetGlobal( "STEPS_TYPE_"+s, st );
	}
	LUA->SetGlobal( "STEPS_TYPE_INVALID", STEPS_TYPE_INVALID );
}
REGISTER_WITH_LUA_FUNCTION( LuaStepsType );


static const char *PlayModeNames[] = {
	"Regular",
	"Nonstop",
	"Oni",
	"Endless",
	"Battle",
	"Rave",
};
XToString( PlayMode, NUM_PlayMode );
XToLocalizedString( PlayMode );
StringToX( PlayMode );
LuaXToString( PlayMode );
LuaFunction( PlayModeToLocalizedString, PlayModeToLocalizedString((PlayMode) IArg(1)) );
LuaXType( PlayMode, NUM_PlayMode, "PLAY_MODE_", true )

RankingCategory AverageMeterToRankingCategory( int iAverageMeter )
{
	if(      iAverageMeter <= 3 )	return RANKING_A;
	else if( iAverageMeter <= 6 )	return RANKING_B;
	else if( iAverageMeter <= 9 )	return RANKING_C;
	else							return RANKING_D;
}


static const char *RankingCategoryNames[] = {
	"a",
	"b",
	"c",
	"d",
};
XToString( RankingCategory, NUM_RANKING_CATEGORIES );
StringToX( RankingCategory );


static const char *PlayerControllerNames[] = {
	"Human",
	"Autoplay",
	"Cpu",
};
XToString( PlayerController, NUM_PlayerController );
XToLocalizedString( PlayerController );


static const char *CoinModeNames[] = {
	"home",
	"pay",
	"free",
};
XToString( CoinMode, NUM_COIN_MODES );
static void LuaCoinMode(lua_State* L)
{
	FOREACH_CoinMode( i )
	{
		RString s = CoinModeNames[i];
		s.MakeUpper();
		LUA->SetGlobal( "COIN_MODE_"+s, i );
	}
}
REGISTER_WITH_LUA_FUNCTION( LuaCoinMode );


static const char *PremiumNames[] = {
	"none",
	"double",
	"joint",
};
XToString( Premium, NUM_PREMIUMS );
static void LuaPremium(lua_State* L)
{
	FOREACH_Premium( i )
	{
		RString s = PremiumNames[i];
		s.MakeUpper();
		LUA->SetGlobal( "PREMIUM_"+s, i );
	}
}
REGISTER_WITH_LUA_FUNCTION( LuaPremium );


static const char *SortOrderNames[] = {
	"Preferred",
	"Group",
	"Title",
	"BPM",
	"Popularity",
	"TopGrades",
	"Artist",
	"Genre",
	"EasyMeter",
	"MediumMeter",
	"HardMeter",
	"ChallengeMeter",
	"ModeMenu",
	"AllCourses",
	"Nonstop",
	"Oni",
	"Endless",
	"Roulette",
};
XToString( SortOrder, NUM_SORT_ORDERS );
StringToX( SortOrder );

static void LuaSortOrder(lua_State* L)
{
	FOREACH_SortOrder( so )
	{
		RString s = SortOrderToString( so );
		
		// [uppercase] -> _[uppercase]
		for( unsigned i=0; i<s.size(); i++ )
		{
			if( isupper(s[i]) )
			{
				s.insert( s.begin()+i, '_' );
				i++;
			}
		}

		s.MakeUpper();
		LUA->SetGlobal( "SORT"+s, so );
	}
}
REGISTER_WITH_LUA_FUNCTION( LuaSortOrder );


static const char *TapNoteScoreNames[] = {
	"None",
	"HitMine",
	"AvoidMine",
	"Miss",
	"W5",
	"W4",
	"W3",
	"W2",
	"W1",
};
XToString( TapNoteScore, NUM_TapNoteScore );
TapNoteScore StringToTapNoteScore( const RString &s )
{
	// new style
	if	   ( s == "None" )		return TNS_None;
	else if( s == "HitMine" )	return TNS_HitMine;
	else if( s == "AvoidMine" )	return TNS_AvoidMine;
	else if( s == "Miss" )		return TNS_Miss;
	else if( s == "W5" )		return TNS_W5;
	else if( s == "W4" )		return TNS_W4;
	else if( s == "W3" )		return TNS_W3;
	else if( s == "W2" )		return TNS_W2;
	else if( s == "W1" )		return TNS_W1;

	// for backward compatibility
	else if( s == "Boo" )		return TNS_W5;
	else if( s == "Good" )		return TNS_W4;
	else if( s == "Great" )		return TNS_W3;
	else if( s == "Perfect" )	return TNS_W2;
	else if( s == "Marvelous" )	return TNS_W1;

	return TNS_INVALID;
}
template<> void StringTo<TapNoteScore>( const RString &s, TapNoteScore &out )
{
	out = StringToTapNoteScore( s );
}
XToLocalizedString( TapNoteScore );
LuaFunction( TapNoteScoreToLocalizedString, TapNoteScoreToLocalizedString((TapNoteScore) IArg(1)) );
static void LuaTapNoteScores( lua_State* L )
{
	FOREACH_TapNoteScore( i )
	{
		RString s = TapNoteScoreNames[i];
		LUA->SetGlobal( "TNS_"+s, i );
	}
}
REGISTER_WITH_LUA_FUNCTION( LuaTapNoteScores );


static const char *HoldNoteScoreNames[] = {
	"None",
	"LetGo",
	"Held",
};
XToString( HoldNoteScore, NUM_HoldNoteScore );
HoldNoteScore StringToHoldNoteScore( const RString &s )
{
	// for backward compatibility
	if     ( s == "NG" )	return HNS_LetGo;
	else if( s == "OK" )	return HNS_Held;

	// new style
	else if( s == "None" )	return HNS_None;
	else if( s == "LetGo" )	return HNS_LetGo;
	else if( s == "Held" )	return HNS_Held;

	return HNS_INVALID;
}
XToLocalizedString( HoldNoteScore );
static void LuaHoldNoteScores( lua_State* L )
{
	FOREACH_HoldNoteScore( i )
	{
		RString s = HoldNoteScoreNames[i];
		LUA->SetGlobal( "HNS_"+s, i );
	}
}
REGISTER_WITH_LUA_FUNCTION( LuaHoldNoteScores );


static const char *MemoryCardStateNames[] = {
	"ready",
	"checking",
	"late",
	"error",
	"removed",
	"none",
};
XToString( MemoryCardState, NUM_MemoryCardState );


static const char *PerDifficultyAwardNames[] = {
	"FullComboW3",
	"SingleDigitW3",
	"OneW3",
	"FullComboW2",
	"SingleDigitW2",
	"OneW2",
	"FullComboW1",
	"Percent80W3",
	"Percent90W3",
	"Percent100W3",
};
XToString( PerDifficultyAward, NUM_PerDifficultyAward );
XToLocalizedString( PerDifficultyAward );
StringToX( PerDifficultyAward );
LuaFunction( PerDifficultyAwardToLocalizedString, PerDifficultyAwardToLocalizedString((PerDifficultyAward) IArg(1)) );
static void LuaPerDifficultyAward( lua_State* L )
{
	FOREACH_PerDifficultyAward( i )
		LUA->SetGlobal( PerDifficultyAwardNames[i], i );
}
REGISTER_WITH_LUA_FUNCTION( LuaPerDifficultyAward );

// Numbers are intentially not at the front of these strings so that the 
// strings can be used as XML entity names.
// Numbers are intentially not at the back so that "1000" and "10000" don't 
// conflict when searching for theme elements.
static const char *PeakComboAwardNames[] = {
	"Peak1000Combo",
	"Peak2000Combo",
	"Peak3000Combo",
	"Peak4000Combo",
	"Peak5000Combo",
	"Peak6000Combo",
	"Peak7000Combo",
	"Peak8000Combo",
	"Peak9000Combo",
	"Peak10000Combo",
};
XToString( PeakComboAward, NUM_PeakComboAward );
XToLocalizedString( PeakComboAward );
StringToX( PeakComboAward );
LuaFunction( PeakComboAwardToLocalizedString, PeakComboAwardToLocalizedString((PeakComboAward) IArg(1)) );
static void LuaPeakComboAward( lua_State* L )
{
	FOREACH_PeakComboAward( i )
		LUA->SetGlobal( PeakComboAwardNames[i], i );
}
REGISTER_WITH_LUA_FUNCTION( LuaPeakComboAward );

LuaXToString( Difficulty );
LuaStringToX( Difficulty );


void DisplayBpms::Add( float f )
{
	vfBpms.push_back( f );
}

float DisplayBpms::GetMin() const
{
	float fMin = FLT_MAX;
	FOREACH_CONST( float, vfBpms, f )
	{
		if( *f != -1 )
			fMin = min( fMin, *f );
	}
	if( fMin == FLT_MAX )
		return 0;
	else
		return fMin;
}

float DisplayBpms::GetMax() const
{
	float fMax = 0;
	FOREACH_CONST( float, vfBpms, f )
	{
		if( *f != -1 )
			fMax = max( fMax, *f );
	}
	return fMax;
}

bool DisplayBpms::BpmIsConstant() const
{
	return fabsf( GetMin() - GetMax() ) < 0.001f;
}

bool DisplayBpms::IsSecret() const
{
	FOREACH_CONST( float, vfBpms, f )
	{
		if( *f == -1 )
			return true;
	}
	return false;
}

static const char *StyleTypeNames[] = {
	"OnePlayerOneSide",
	"TwoPlayersTwoSides",
	"OnePlayerTwoSides",
	"TwoPlayersSharedSides",
};
XToString( StyleType, NUM_STYLE_TYPES );
StringToX( StyleType );


static const char *GoalTypeNames[] = {
	"Calories",
	"Time",
	"None",
};
XToString( GoalType, NUM_GoalType );
StringToX( GoalType );
static void LuaGoalType(lua_State* L)
{
	FOREACH_GoalType( gt )
	{
		RString s = GoalTypeNames[gt];
		s.MakeUpper();
		LUA->SetGlobal( "GOAL_"+s, gt );
	}
}
REGISTER_WITH_LUA_FUNCTION( LuaGoalType );

static const char *EditModeNames[] = {
	"Practice",
	"CourseMods",
	"Home",
	"Full"
};
XToString( EditMode, EditMode_INVALID );
StringToX( EditMode );

static const char *StageNames[] = {
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"Final",
	"Extra1",
	"Extra2",
	"Nonstop",
	"Oni",
	"Endless",
	"Event",
	"Demo",
};
XToString( Stage, NUM_STAGES );
static void LuaStage(lua_State* L)
{
	FOREACH_Stage( st )
	{
		RString s = StageNames[st];
		s.MakeUpper();
		LUA->SetGlobal( "STAGE_"+s, st );
	}
}
REGISTER_WITH_LUA_FUNCTION( LuaStage );
LuaXToString( Stage );


static const char *MultiPlayerStatusNames[] = {
	"Joined",
	"NotJoined",
	"Unplugged",
	"MissingMultitap",
};
XToString( MultiPlayerStatus, NUM_MultiPlayerStatus );


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
