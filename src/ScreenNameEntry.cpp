#include "global.h"
#include "ScreenNameEntry.h"
#include "SongManager.h"
#include "GameConstantsAndTypes.h"
#include "RageUtil.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "RageLog.h"
#include "GameState.h"
#include "GameSoundManager.h"
#include "ThemeManager.h"
#include "Course.h"
#include "AnnouncerManager.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "StageStats.h"
#include "Game.h"
#include "ScreenDimensions.h"
#include "PlayerState.h"
#include "Style.h"
#include "NoteSkinManager.h"
#include "InputEventPlus.h"
#include "RageSoundManager.h"
#include "RageDisplay.h"


//
// Defines specific to ScreenNameEntry
//
#define TIMER_X				THEME->GetMetricF(m_sName,"TimerX")
#define TIMER_Y				THEME->GetMetricF(m_sName,"TimerY")
#define CATEGORY_Y			THEME->GetMetricF(m_sName,"CategoryY")
#define CATEGORY_ZOOM			THEME->GetMetricF(m_sName,"CategoryZoom")
#define CHARS_ZOOM_SMALL		THEME->GetMetricF(m_sName,"CharsZoomSmall")
#define CHARS_ZOOM_LARGE		THEME->GetMetricF(m_sName,"CharsZoomLarge")
#define CHARS_SPACING_Y			THEME->GetMetricF(m_sName,"CharsSpacingY")
#define SCROLLING_CHARS_COMMAND		THEME->GetMetricA(m_sName,"ScrollingCharsCommand")
#define SELECTED_CHARS_COMMAND		THEME->GetMetricA(m_sName,"SelectedCharsCommand")
#define GRAY_ARROWS_Y			THEME->GetMetricF(m_sName,"ReceptorArrowsY")
#define NUM_CHARS_TO_DRAW_BEHIND	THEME->GetMetricI(m_sName,"NumCharsToDrawBehind")
#define NUM_CHARS_TO_DRAW_TOTAL		THEME->GetMetricI(m_sName,"NumCharsToDrawTotal")
#define FAKE_BEATS_PER_SEC		THEME->GetMetricF(m_sName,"FakeBeatsPerSec")
#define TIMER_SECONDS			THEME->GetMetricF(m_sName,"TimerSeconds")
#define MAX_RANKING_NAME_LENGTH		THEME->GetMetricI(m_sName,"MaxRankingNameLength")
#define PLAYER_X( p, styleType )	THEME->GetMetricF(m_sName,ssprintf("PlayerP%d%sX",p+1,StyleTypeToString(styleType).c_str()))


// cache for frequently used metrics
static float	g_fCharsZoomSmall;
static float	g_fCharsZoomLarge; 
static float	g_fCharsSpacingY;
static float	g_fReceptorArrowsY;
static int	g_iNumCharsToDrawBehind;
static int	g_iNumCharsToDrawTotal;
static float	g_fFakeBeatsPerSec;

RString ScreenNameEntry::ScrollingText::g_sNameChars = "    ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void ScreenNameEntry::ScrollingText::Init( const RString &sName, const vector<float> &xs )
{
	SetName( sName );
	m_Xs = xs;
	m_bDone = false;
	m_Stamp.LoadFromFont( THEME->GetPathF(sName, "letters") );
	m_Stamp.RunCommands( THEME->GetMetricA(sName, "ScrollingCharsCommand") );
}

void ScreenNameEntry::ScrollingText::DrawPrimitives()
{
	const float fFakeBeat = GAMESTATE->m_fSongBeat;
	const size_t iClosestIndex = (int)roundf( fFakeBeat ) % g_sNameChars.size();
	const float fClosestYOffset = GetClosestCharYOffset( fFakeBeat );

	size_t iCharIndex = ( iClosestIndex - NUM_CHARS_TO_DRAW_BEHIND + g_sNameChars.size() ) % g_sNameChars.size();
	float fY = GRAY_ARROWS_Y + ( fClosestYOffset - g_iNumCharsToDrawBehind ) * g_fCharsSpacingY;

	for( int i = 0; i < NUM_CHARS_TO_DRAW_TOTAL; ++i )
	{
		const RString c = g_sNameChars.substr( iCharIndex, 1 );
		float fZoom = g_fCharsZoomSmall;
		float fAlpha = 1.f;

		if( iCharIndex == iClosestIndex )
			fZoom = SCALE( fabs(fClosestYOffset), 0, 0.5f, g_fCharsZoomLarge, g_fCharsZoomSmall );
		if( i == 0 )
			fAlpha *= SCALE( fClosestYOffset, -0.5f, 0.f, 0.f, 1.f );
		if( i == g_iNumCharsToDrawTotal-1 )
			fAlpha *= SCALE( fClosestYOffset, 0.f, 0.5f, 1.f, 0.f );

		m_Stamp.SetZoom( fZoom );
		m_Stamp.SetDiffuseAlpha( fAlpha );
		m_Stamp.SetText( c );
		m_Stamp.SetY( fY );
		FOREACH_CONST( float, m_Xs, x )
		{
			m_Stamp.SetX( *x );
			m_Stamp.Draw();
		}
		fY += g_fCharsSpacingY;
		iCharIndex = (iCharIndex+1) % g_sNameChars.size();
	}
}

char ScreenNameEntry::ScrollingText::GetClosestChar( float fFakeBeat ) const
{
	ASSERT( fFakeBeat >= 0.f );
	return g_sNameChars[(size_t)roundf(fFakeBeat) % g_sNameChars.size()];
}

// return value is relative to gray arrows
float ScreenNameEntry::ScrollingText::GetClosestCharYOffset( float fFakeBeat ) const
{
	float f = fmodf(fFakeBeat, 1.0f);
	if( f > 0.5f )
		f -= 1;
	ASSERT( f>-0.5f && f<=0.5f );
	return -f;	
}

REGISTER_SCREEN_CLASS( ScreenNameEntry );
ScreenNameEntry::ScreenNameEntry()
{
#if 0
		// DEBUGGING STUFF
	GAMESTATE->m_pCurGame.Set( GAMEMAN->GetDefaultGame() );
	GAMESTATE->m_pCurStyle.Set( GAMEMAN->GetHowToPlayStyleForGame(GAMESTATE->m_pCurGame) );
	GAMESTATE->m_PlayMode.Set( PLAY_MODE_REGULAR );
	GAMESTATE->m_bSideIsJoined[PLAYER_1] = true;
	GAMESTATE->m_MasterPlayerNumber = PLAYER_1;
#endif
}

void ScreenNameEntry::Init()
{
	Screen::Init();

	// update cache
	g_fCharsZoomSmall = CHARS_ZOOM_SMALL;
	g_fCharsZoomLarge = CHARS_ZOOM_LARGE;
	g_fCharsSpacingY = CHARS_SPACING_Y;
	g_fReceptorArrowsY = GRAY_ARROWS_Y;
	g_iNumCharsToDrawBehind = NUM_CHARS_TO_DRAW_BEHIND;
	g_iNumCharsToDrawTotal = NUM_CHARS_TO_DRAW_TOTAL;
	g_fFakeBeatsPerSec = FAKE_BEATS_PER_SEC;

	// reset Player and Song Options
	{
		FOREACH_PlayerNumber( p )
			PO_GROUP_CALL( GAMESTATE->m_pPlayerState[p]->m_PlayerOptions, ModsLevel_Stage, Init );
		SO_GROUP_CALL( GAMESTATE->m_SongOptions, ModsLevel_Stage, Init );
	}

	vector<GameState::RankingFeat> aFeats[NUM_PLAYERS];

	// Find out if players deserve to enter their name
	FOREACH_PlayerNumber( p )
	{
		GAMESTATE->GetRankingFeats( p, aFeats[p] );
		GAMESTATE->JoinPlayer( p );
		m_bStillEnteringName[p] = aFeats[p].size()>0;
#if 0 // Debugging.
		m_bStillEnteringName[p] = p == PLAYER_1;
#endif
	}

	if( !AnyStillEntering() )
	{
		/* Nobody made a high score. */
		PostScreenMessage( SM_GoToNextScreen, 0 );
		return;
	}

	bool IsOnRanking = ( (GAMESTATE->m_PlayMode == PLAY_MODE_NONSTOP || GAMESTATE->m_PlayMode == PLAY_MODE_ONI)
		&& !(GAMESTATE->m_pCurCourse->IsRanking()) );

	if( PREFSMAN->m_GetRankingName == RANKING_OFF || 
		(PREFSMAN->m_GetRankingName == RANKING_LIST && !IsOnRanking) )
	{
		// don't collect score due to ranking setting
		PostScreenMessage( SM_GoToNextScreen, 0 );
		return;
	}


	GAMESTATE->m_bGameplayLeadIn.Set( false );	// enable the gray arrows

	FOREACH_PlayerNumber( p )
	{
		// load last used ranking name if any
		Profile* pProfile = PROFILEMAN->GetProfile(p);
		if( pProfile && !pProfile->m_sLastUsedHighScoreName.empty() )
			 m_sSelectedName[p] = pProfile->m_sLastUsedHighScoreName;

		// resize string to MAX_RANKING_NAME_LENGTH
		m_sSelectedName[p] = ssprintf( "%*.*s", MAX_RANKING_NAME_LENGTH, MAX_RANKING_NAME_LENGTH, m_sSelectedName[p].c_str() );
		ASSERT( (int) m_sSelectedName[p].length() == MAX_RANKING_NAME_LENGTH );

		// don't load player if they aren't going to enter their name
		if( !m_bStillEnteringName[p] )
			continue;	// skip

		// remove modifiers that may have been on the last song
		PlayerOptions po;
		GAMESTATE->GetDefaultPlayerOptions( po );
		GAMESTATE->m_pPlayerState[p]->m_PlayerOptions.Assign( ModsLevel_Stage, po );

		ASSERT( GAMESTATE->IsHumanPlayer(p) );	// they better be enabled if they made a high score!

		const float fPlayerX = PLAYER_X( p, GAMESTATE->GetCurrentStyle()->m_StyleType );

		{
			LockNoteSkin l( GAMESTATE->m_pPlayerState[p]->m_PlayerOptions.GetCurrent().m_sNoteSkin );

			m_ReceptorArrowRow[p].Load( GAMESTATE->m_pPlayerState[p], 0 );
			m_ReceptorArrowRow[p].SetX( fPlayerX );
			m_ReceptorArrowRow[p].SetY( GRAY_ARROWS_Y );
			this->AddChild( &m_ReceptorArrowRow[p] );
		}


		const Style* pStyle = GAMESTATE->GetCurrentStyle();

		m_ColToStringIndex[p].insert(m_ColToStringIndex[p].begin(), pStyle->m_iColsPerPlayer, -1);
		int CurrentStringIndex = 0;
		vector<float> xs;

		for( int iCol=0; iCol<pStyle->m_iColsPerPlayer; iCol++ )
		{
			if( CurrentStringIndex == MAX_RANKING_NAME_LENGTH )
				break; /* We have enough columns. */

			/* Find out if this column is associated with the START menu button. */
			GameInput gi = GAMESTATE->GetCurrentStyle()->StyleInputToGameInput( iCol, p );
			MenuButton mb = INPUTMAPPER->GameToMenu( gi );
			if( mb == MENU_BUTTON_START )
				continue;
			m_ColToStringIndex[p][iCol] = CurrentStringIndex++;

			float ColX = fPlayerX + pStyle->m_ColumnInfo[p][iCol].fXOffset;

			m_textSelectedChars[p][iCol].LoadFromFont( THEME->GetPathF(m_sName,"letters") );
			m_textSelectedChars[p][iCol].SetX( ColX );
			m_textSelectedChars[p][iCol].SetY( GRAY_ARROWS_Y );
			m_textSelectedChars[p][iCol].RunCommands( SELECTED_CHARS_COMMAND );
			m_textSelectedChars[p][iCol].SetZoom( CHARS_ZOOM_LARGE );
			if( iCol < (int)m_sSelectedName[p].length() )
				m_textSelectedChars[p][iCol].SetText( m_sSelectedName[p].substr(iCol,1) );
			this->AddChild( &m_textSelectedChars[p][iCol] );
			xs.push_back( ColX );
		}
		m_Text[p].Init( m_sName, xs );
		this->AddChild( &m_Text[p] );

		m_textCategory[p].LoadFromFont( THEME->GetPathF(m_sName,"category") );
		m_textCategory[p].SetX( fPlayerX );
		m_textCategory[p].SetY( CATEGORY_Y );
		m_textCategory[p].SetZoom( CATEGORY_ZOOM );
		RString joined;
		for( unsigned j = 0; j < aFeats[p].size(); ++j )
		{
			if( j )
				joined += "\n";
			joined += aFeats[p][j].Feat;
		}

		m_textCategory[p].SetText( joined );
		this->AddChild( &m_textCategory[p] );
	}


	m_Timer.Load();
	if( !PREFSMAN->m_bMenuTimer )
		m_Timer.Disable();
	else
		m_Timer.SetSeconds(TIMER_SECONDS);
	m_Timer.SetXY( TIMER_X, TIMER_Y );
	this->AddChild( &m_Timer );

	m_In.Load( THEME->GetPathB(m_sName,"in") );
	m_In.StartTransitioning();
	m_In.SetDrawOrder( DRAW_ORDER_TRANSITIONS );
	this->AddChild( &m_In );

	m_Out.Load( THEME->GetPathB(m_sName,"out") );
	m_Out.SetDrawOrder( DRAW_ORDER_TRANSITIONS );
	this->AddChild( &m_Out );

	this->SortByDrawOrder();

	m_soundStep.Load( THEME->GetPathS(m_sName,"step") );
	m_sPathToMusic = THEME->GetPathS( m_sName, "music" );
	m_fFakeBeat = 0;
}

void ScreenNameEntry::BeginScreen()
{
	Screen::BeginScreen();
	SOUND->PlayMusic( m_sPathToMusic );
}

void ScreenNameEntry::EndScreen()
{
	SOUND->StopMusic();
	Screen::EndScreen();
}

bool ScreenNameEntry::AnyStillEntering() const
{
	FOREACH_PlayerNumber( p )
		if( m_bStillEnteringName[p] )
			return true;
	return false;
}

ScreenNameEntry::~ScreenNameEntry()
{
	LOG->Trace( "ScreenNameEntry::~ScreenNameEntry()" );
}

void ScreenNameEntry::Update( float fDelta )
{
	if( m_bFirstUpdate )
		SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo("name entry") );

	m_fFakeBeat += fDelta * FAKE_BEATS_PER_SEC;
	GAMESTATE->m_fSongBeat = m_fFakeBeat;

	Screen::Update(fDelta);
}

void ScreenNameEntry::Input( const InputEventPlus &input )
{
	LOG->Trace( "ScreenNameEntry::Input()" );

	if( m_In.IsTransitioning() || m_Out.IsTransitioning() )
		return;	

	if( input.type != IET_FIRST_PRESS || !input.GameI.IsValid() )
		return;		// ignore

	const int iCol = GAMESTATE->GetCurrentStyle()->GameInputToColumn( input.GameI );
	if( iCol != Column_Invalid && m_bStillEnteringName[input.pn] )
	{
		int iStringIndex = m_ColToStringIndex[input.pn][iCol];
		if( iStringIndex != -1 )
		{
			m_ReceptorArrowRow[input.pn].Step( iCol, TNS_W1 );
			m_soundStep.Play();
			char c = m_Text[input.pn].GetClosestChar( m_fFakeBeat );
			m_textSelectedChars[input.pn][iCol].SetText( RString(1, c) );
			m_sSelectedName[input.pn][iStringIndex] = c;
		}
	}

	Screen::Input( input );
}

void ScreenNameEntry::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_MenuTimer )
	{
		if( !m_Out.IsTransitioning() )
		{
			InputEventPlus iep;
			FOREACH_PlayerNumber( p )
			{
				iep.pn = p;
				this->MenuStart( iep );
			}
		}
	}

	Screen::HandleScreenMessage( SM );
}


void ScreenNameEntry::MenuStart( const InputEventPlus &input )
{
	PlayerNumber pn = input.pn;
	
	if( !m_bStillEnteringName[pn] )
		return;
	m_bStillEnteringName[pn] = false;
	m_Text[pn].SetDone();

	m_soundStep.Play();

	// save last used ranking name
	Profile* pProfile = PROFILEMAN->GetProfile(pn);
	pProfile->m_sLastUsedHighScoreName = m_sSelectedName[pn];

	TrimRight( m_sSelectedName[pn], " " );
	TrimLeft( m_sSelectedName[pn], " " );

	GAMESTATE->StoreRankingName( pn, m_sSelectedName[pn] );

	if( !AnyStillEntering() && !m_Out.IsTransitioning() )
		m_Out.StartTransitioning( SM_GoToNextScreen );
}

/*
 * (c) 2001-2006 Chris Danford, Steve Checkoway
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
