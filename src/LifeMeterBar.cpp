#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: LifeMeterBar

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "LifeMeterBar.h"
#include "PrefsManager.h"
#include "RageException.h"
#include "RageTimer.h"
#include "GameState.h"
#include "RageMath.h"
#include "ThemeManager.h"
#include "song.h"
#include "StageStats.h"


//
// Important!!!!  Do not use these macros during gameplay.  They return very slowly.  Cache them in a member.
//
static CachedThemeMetricI METER_WIDTH		("LifeMeterBar","MeterWidth");
static CachedThemeMetricI METER_HEIGHT		("LifeMeterBar","MeterHeight");
static CachedThemeMetricF DANGER_THRESHOLD	("LifeMeterBar","DangerThreshold");
static CachedThemeMetricI NUM_CHAMBERS		("LifeMeterBar","NumChambers");
static CachedThemeMetricI NUM_STRIPS		("LifeMeterBar","NumStrips");


const float FAIL_THRESHOLD = 0;


class LifeMeterStream : public Actor
{
public:
	LifeMeterStream()
	{
		METER_WIDTH.Refresh();
		METER_HEIGHT.Refresh();
		DANGER_THRESHOLD.Refresh();
		NUM_CHAMBERS.Refresh();
		NUM_STRIPS.Refresh();


		bool bExtra = GAMESTATE->IsExtraStage()||GAMESTATE->IsExtraStage2();

		m_quadMask.SetDiffuse( RageColor(0,0,0,1) );
		m_quadMask.SetZ( 1 );
		m_quadMask.SetBlendMode( BLEND_NO_EFFECT );
		m_quadMask.SetUseZBuffer( true );

		CString sGraphicPath;
		RageTextureID ID;
		ID.bStretch = true;

		sGraphicPath = ssprintf("LifeMeterBar %snormal", bExtra?"extra ":"");
		ID.filename = THEME->GetPathToG(sGraphicPath);
		m_sprStreamNormal.Load( ID );
		m_sprStreamNormal.SetUseZBuffer( true );

		sGraphicPath = ssprintf("LifeMeterBar %shot", bExtra?"extra ":"");
		ID.filename = THEME->GetPathToG(sGraphicPath);
		m_sprStreamHot.Load( ID );
		m_sprStreamHot.SetUseZBuffer( true );

		sGraphicPath = ssprintf("LifeMeterBar %sframe", bExtra?"extra ":"");
		ID.filename = THEME->GetPathToG(sGraphicPath);
		m_sprFrame.Load( ID );
	}

	Sprite		m_sprStreamNormal;
	Sprite		m_sprStreamHot;
	Sprite		m_sprFrame;
	Quad		m_quadMask;

	PlayerNumber m_PlayerNumber;
	float m_fPercent;
	float m_fHotAlpha;

	void GetChamberIndexAndOverslow( float fPercent, int& iChamberOut, float& fChamberOverflowPercentOut )
	{
		iChamberOut = (int)(fPercent*NUM_CHAMBERS);
		fChamberOverflowPercentOut = fPercent*NUM_CHAMBERS - iChamberOut;
	}

	float GetChamberLeftPercent( int iChamber )
	{
		return (iChamber+0) / (float)NUM_CHAMBERS;
	}

	float GetChamberRightPercent( int iChamber )
	{
		return (iChamber+1) / (float)NUM_CHAMBERS;
	}

	float GetRightEdgePercent( int iChamber, float fChamberOverflowPercent )
	{
		if( (iChamber%2) == 0 )
			return (iChamber+fChamberOverflowPercent) / (float)NUM_CHAMBERS;
		else
			return (iChamber+1) / (float)NUM_CHAMBERS;
	}

	float GetHeightPercent( int iChamber, float fChamberOverflowPercent )
	{
		if( (iChamber%2) == 1 )
			return 1-fChamberOverflowPercent;
		else
			return 0;
	}

	void DrawPrimitives()
	{
		if( GAMESTATE->IsPlayerEnabled(m_PlayerNumber) )
		{
			DrawMask( m_fPercent );		// this is the "right endcap" to the life
			
			const float fChamberWidthInPercent = 1.0f/NUM_CHAMBERS;
			float fPercentBetweenStrips = 1.0f/NUM_STRIPS;
			// round this so that the chamber overflows align
			if( NUM_CHAMBERS > 10 )
				fPercentBetweenStrips = froundf( fPercentBetweenStrips, fChamberWidthInPercent );

			float fPercentOffset = fmodf( GAMESTATE->m_fSongBeat/4+1000, fPercentBetweenStrips );
			ASSERT( fPercentOffset >= 0  &&  fPercentOffset <= fPercentBetweenStrips );

			for( float f=fPercentOffset+1; f>=0; f-=fPercentBetweenStrips )
			{
				DrawMask( f );
				DrawStrip( f );
			}

		}

		m_sprFrame.Draw();

	}

	void DrawStrip( float fRightEdgePercent )
	{
		RectI rect;

		const float fChamberWidthInPercent = 1.0f/NUM_CHAMBERS;
		const float fStripWidthInPercent = 1.0f/NUM_STRIPS;
		
		const float fCorrectedRightEdgePercent = fRightEdgePercent + fChamberWidthInPercent;
		const float fCorrectedStripWidthInPercent = fStripWidthInPercent + 2*fChamberWidthInPercent;
		const float fCorrectedLeftEdgePercent = fCorrectedRightEdgePercent - fCorrectedStripWidthInPercent;


		// set size of streams
		rect.left	= int(-METER_WIDTH/2 + METER_WIDTH*max(0,fCorrectedLeftEdgePercent));
		rect.top	= int(-METER_HEIGHT/2);
		rect.right	= int(-METER_WIDTH/2 + METER_WIDTH*min(1,fCorrectedRightEdgePercent));
		rect.bottom	= int(+METER_HEIGHT/2);

		ASSERT( rect.left <= METER_WIDTH/2  &&  rect.right <= METER_WIDTH/2 );  

		float fPercentCroppedFromLeft = max( 0, -fCorrectedLeftEdgePercent );
		float fPercentCroppedFromRight = max( 0, fCorrectedRightEdgePercent-1 );


		m_sprStreamNormal.StretchTo( rect );
		m_sprStreamHot.StretchTo( rect );


		// set custom texture coords
//		float fPrecentOffset = fRightEdgePercent;

		RectF frectCustomTexRect(
			fPercentCroppedFromLeft,
			0,
			1-fPercentCroppedFromRight,
			1);

		m_sprStreamNormal.SetCustomTextureRect( frectCustomTexRect );
		m_sprStreamHot.SetCustomTextureRect( frectCustomTexRect );

		m_sprStreamHot.SetDiffuse( RageColor(1,1,1,m_fHotAlpha) );

		m_sprStreamNormal.Draw();
		m_sprStreamHot.Draw();
	}

	void DrawMask( float fPercent )
	{
		RectI rect;

		int iChamber;
		float fChamberOverflowPercent;
		GetChamberIndexAndOverslow( fPercent, iChamber, fChamberOverflowPercent );
		float fRightPercent = GetRightEdgePercent( iChamber, fChamberOverflowPercent );
		float fHeightPercent = GetHeightPercent( iChamber, fChamberOverflowPercent );
		float fChamberLeftPercent = GetChamberLeftPercent( iChamber );
		float fChamberRightPercent = GetChamberRightPercent( iChamber );

		// draw mask for vertical chambers
		rect.left	= int(-METER_WIDTH/2 + fChamberLeftPercent*METER_WIDTH-1);
		rect.top	= int(-METER_HEIGHT/2);
		rect.right	= int(-METER_WIDTH/2 + fChamberRightPercent*METER_WIDTH+1);
		rect.bottom	= int(-METER_HEIGHT/2 + fHeightPercent*METER_HEIGHT);

		rect.left  = MIN( rect.left,  + METER_WIDTH/2 );
		rect.right = MIN( rect.right, + METER_WIDTH/2 );

		m_quadMask.StretchTo( rect );
		m_quadMask.Draw();

		// draw mask for horizontal chambers
		rect.left	= (int)(-METER_WIDTH/2 + fRightPercent*METER_WIDTH); 
		rect.top	= -METER_HEIGHT/2;
		rect.right	= +METER_WIDTH/2;
		rect.bottom	= +METER_HEIGHT/2;

		rect.left  = MIN( rect.left,  + METER_WIDTH/2 );
		rect.right = MIN( rect.right, + METER_WIDTH/2 );

		m_quadMask.StretchTo( rect );
		m_quadMask.Draw();
	}
};


LifeMeterBar::LifeMeterBar()
{
	m_pStream = new LifeMeterStream;

	switch( GAMESTATE->m_SongOptions.m_DrainType )
	{
	case SongOptions::DRAIN_NORMAL:			m_fLifePercentage = 0.5f;	break;
	case SongOptions::DRAIN_NO_RECOVER:		m_fLifePercentage = 1.0f;	break;
	case SongOptions::DRAIN_SUDDEN_DEATH:	m_fLifePercentage = 1.0f;	break;
	default:	ASSERT(0);
	}

	m_fTrailingLifePercentage = 0;
	m_fLifeVelocity = 0;
	m_fHotAlpha = 0;
	m_bFailedEarlier = false;

	// set up lifebar
	m_fBaseLifeDifficulty = PREFSMAN->m_fLifeDifficultyScale;
	m_fLifeDifficulty = m_fBaseLifeDifficulty;

	m_quadBlackBackground.SetDiffuse( RageColor(0,0,0,1) );
	m_quadBlackBackground.SetZoomX( (float)METER_WIDTH );
	m_quadBlackBackground.SetZoomY( (float)METER_HEIGHT );

	this->AddChild( &m_quadBlackBackground );
	this->AddChild( m_pStream );

	// set up progressive lifebar
	m_iProgressiveLifebar = PREFSMAN->m_iProgressiveLifebar;
	m_iMissCombo = 0;

	// set up combotoregainlife
	m_iComboToRegainLife = 0;

	AfterLifeChanged();
}

LifeMeterBar::~LifeMeterBar()
{
	delete m_pStream;
}

void LifeMeterBar::Load( PlayerNumber pn )
{
	LifeMeter::Load( pn );

	m_pStream->m_PlayerNumber = pn;

	if( pn == PLAYER_2 )
		m_pStream->SetZoomX( -1 );
}

void LifeMeterBar::ChangeLife( TapNoteScore score )
{
	float fDeltaLife=0.f;
	switch( GAMESTATE->m_SongOptions.m_DrainType )
	{
	case SongOptions::DRAIN_NORMAL:
		switch( score )
		{
		case TNS_MARVELOUS:	fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeMarvelous;	break;
		case TNS_PERFECT:	fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangePerfect;	break;
		case TNS_GREAT:		fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeGreat;		break;
		case TNS_GOOD:		fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeGood;		break;
		case TNS_BOO:		fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeBoo;		break;
		case TNS_MISS:		fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeMiss;		break;
		default:
			ASSERT(0);
		}
		if( IsHot()  &&  score < TNS_GOOD )
			fDeltaLife = -0.10f;		// make it take a while to get back to "doing great"
		break;
	case SongOptions::DRAIN_NO_RECOVER:
		switch( score )
		{
		case TNS_MARVELOUS:	fDeltaLife = +0.000f;	break;
		case TNS_PERFECT:	fDeltaLife = +0.000f;	break;
		case TNS_GREAT:		fDeltaLife = +0.000f;	break;
		case TNS_GOOD:		fDeltaLife = +0.000f;	break;
		case TNS_BOO:		fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeBoo;	break;
		case TNS_MISS:		fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeMiss;	break;
		default:
			ASSERT(0);
		}
		break;
	case SongOptions::DRAIN_SUDDEN_DEATH:
		switch( score )
		{
		case TNS_MARVELOUS:	fDeltaLife = +0;	break;
		case TNS_PERFECT:	fDeltaLife = +0;	break;
		case TNS_GREAT:		fDeltaLife = +0;	break;
		case TNS_GOOD:		fDeltaLife = -1.0;	break;
		case TNS_BOO:		fDeltaLife = -1.0;	break;
		case TNS_MISS:		fDeltaLife = -1.0;	break;
		default:
			ASSERT(0);
		}
		break;
	default:
		ASSERT(0);
	}

	ChangeLife( fDeltaLife );
}

void LifeMeterBar::ChangeLife( HoldNoteScore score, TapNoteScore tscore )
{
	/* The initial tap note score (which we happen to have in have in
	 * tscore) has already been reported to the above function.  If the
	 * hold end result was an NG, count it as a miss; if the end result
	 * was an OK, count a perfect.  (Remember, this is just life meter
	 * computation, not scoring.) */
	float fDeltaLife=0.f;
	switch( GAMESTATE->m_SongOptions.m_DrainType )
	{
	case SongOptions::DRAIN_NORMAL:
		switch( score )
		{
		case HNS_OK:	fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeOK;	break;
		case HNS_NG:	fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeNG;	break;
		default:
			ASSERT(0);
		}
		if( IsHot()  &&  score == HNS_NG )
			fDeltaLife = -0.10f;		// make it take a while to get back to "doing great"
		break;
	case SongOptions::DRAIN_NO_RECOVER:
		switch( score )
		{
		case HNS_OK:	fDeltaLife = +0.000f;	break;
		case HNS_NG:	fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeNG;	break;
		default:
			ASSERT(0);
		}
		break;
	case SongOptions::DRAIN_SUDDEN_DEATH:
		switch( score )
		{
		case HNS_OK:		fDeltaLife = +0;	break;
		case HNS_NG:		fDeltaLife = -1.0;	break;
		default:
			ASSERT(0);
		}
		break;
	default:
		ASSERT(0);
	}

	ChangeLife( fDeltaLife );
}

void LifeMeterBar::ChangeLife( float fDeltaLife )
{
	if( PREFSMAN->m_bMercifulDrain  &&  fDeltaLife < 0 )
		fDeltaLife *= SCALE( m_fLifePercentage, 0.f, 1.f, 0.5f, 1.f);

	// handle progressiveness and ComboToRegainLife here
	if( fDeltaLife >= 0 )
	{
		m_iMissCombo = 0;
		m_iComboToRegainLife = max( m_iComboToRegainLife-1, 0 );
		if ( m_iComboToRegainLife > 0 )
			fDeltaLife = 0.0f;
	}
	else
	{
		fDeltaLife *= 1 + (float)m_iProgressiveLifebar/8 * m_iMissCombo;
		// do this after; only successive boo/miss will
		// increase the amount of life lost.
		m_iMissCombo++;
		/* Increase by m_iRegenComboAfterMiss; never push it beyond m_iMaxRegenComboAfterMiss
		 * but don't reduce it if it's already past. */
		const int NewComboToRegainLife = min( PREFSMAN->m_iMaxRegenComboAfterMiss,
				m_iComboToRegainLife + PREFSMAN->m_iRegenComboAfterMiss );
		m_iComboToRegainLife = max( m_iComboToRegainLife, NewComboToRegainLife );
	}

	/* If we've already failed, there's no point in letting them fill up the bar again.  */
	if( g_CurStageStats.bFailed[m_PlayerNumber] )
		fDeltaLife = 0;

	switch( GAMESTATE->m_SongOptions.m_DrainType )
	{
	case SongOptions::DRAIN_NORMAL:
	case SongOptions::DRAIN_NO_RECOVER:
		if( fDeltaLife > 0 )
			fDeltaLife *= m_fLifeDifficulty;
		else
			fDeltaLife /= m_fLifeDifficulty;
		break;
	}

	// check if this step would cause a fail
	if( m_fLifePercentage + fDeltaLife <= FAIL_THRESHOLD 
		&& m_fLifePercentage > FAIL_THRESHOLD )
	{
		/* Increase by m_iRegenComboAfterFail; never push it beyond m_iMaxRegenComboAfterFail
		 * but don't reduce it if it's already past. */
		const int NewComboToRegainLife = min( PREFSMAN->m_iMaxRegenComboAfterFail,
				m_iComboToRegainLife + PREFSMAN->m_iRegenComboAfterFail );
		m_iComboToRegainLife = max( m_iComboToRegainLife, NewComboToRegainLife );
	}
	
	m_fLifePercentage += fDeltaLife;
	CLAMP( m_fLifePercentage, 0, 1 );

	if( m_fLifePercentage <= FAIL_THRESHOLD )
		g_CurStageStats.bFailedEarlier[m_PlayerNumber] = true;

	m_fLifeVelocity += fDeltaLife;
}

void LifeMeterBar::ChangeLifeMine()
{
	float fDeltaLife=0.f;
	switch( GAMESTATE->m_SongOptions.m_DrainType )
	{
	case SongOptions::DRAIN_NORMAL:
		fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeHitMine;
		if( IsHot() )
			fDeltaLife = -0.10f;		// make it take a while to get back to "doing great"
		break;
	case SongOptions::DRAIN_NO_RECOVER:
		fDeltaLife = PREFSMAN->m_fLifeDeltaPercentChangeHitMine;
		break;
	case SongOptions::DRAIN_SUDDEN_DEATH:
		fDeltaLife = -1.0;
		break;
	default:
		ASSERT(0);
	}

	ChangeLife( fDeltaLife );
}

void LifeMeterBar::AfterLifeChanged()
{

}

bool LifeMeterBar::IsHot() const
{ 
	return m_fLifePercentage >= 1; 
}

bool LifeMeterBar::IsInDanger() const
{ 
	return m_fLifePercentage < DANGER_THRESHOLD; 
}

bool LifeMeterBar::IsFailing() const
{ 
	return m_fLifePercentage <= 0; 
}


void LifeMeterBar::Update( float fDeltaTime )
{
	LifeMeter::Update( fDeltaTime );


	// HACK:  Tweaking these values is very difficulty.  Update the
	// "physics" many times so that the spring motion appears faster

	for( int i=0; i<10; i++ )
	{

		const float fDelta = m_fLifePercentage - m_fTrailingLifePercentage;

		const float fSpringForce = fDelta * 2.0f;
		m_fLifeVelocity += fSpringForce * fDeltaTime;

		const float fViscousForce = -m_fLifeVelocity * 0.2f;
		m_fLifeVelocity += fViscousForce * fDeltaTime;

		CLAMP( m_fLifeVelocity, -.06f, +.02f );

		m_fTrailingLifePercentage += m_fLifeVelocity * fDeltaTime;
	}

	m_fHotAlpha  += IsHot() ? + fDeltaTime*2 : -fDeltaTime*2;
	CLAMP( m_fHotAlpha, 0, 1 );

	if( IsHot() )
		m_fLifeVelocity = max( 0, m_fLifeVelocity );
}


void LifeMeterBar::DrawPrimitives()
{
	/*
	// set custom texture coords
	static RECT rectSize;
	rectSize.left	= -m_iMeterWidth/2; 
	rectSize.top	= -m_iMeterHeight/2;
	rectSize.right	= -m_iMeterWidth/2 + int(roundf(m_iMeterWidth*m_fTrailingLifePercentage));
	rectSize.bottom	= +m_iMeterHeight/2;

	float fPrecentOffset = RageTimer::GetTimeSinceStart();
	fPrecentOffset -= (int)fPrecentOffset;

	RectF frectCustomTexCoords(
		0 - fPrecentOffset,
		0,
		m_fTrailingLifePercentage - fPrecentOffset,
		1 );

	m_sprStreamNormal.StretchTo( &rectSize );
	m_sprStreamNormal.SetCustomTextureRect( frectCustomTexCoords );
	m_sprStreamHot.StretchTo( &rectSize );
	m_sprStreamHot.SetCustomTextureRect( frectCustomTexCoords );

	m_sprStreamHot.SetDiffuse(    RageColor(1,1,1,m_fHotAlpha) );

*/

	m_pStream->m_fPercent = m_fTrailingLifePercentage;
	m_pStream->m_fHotAlpha = m_fHotAlpha;

	float fPercentRed = (m_fTrailingLifePercentage<DANGER_THRESHOLD) ? sinf( RageTimer::GetTimeSinceStart()*PI*4 )/2+0.5f : 0;
	m_quadBlackBackground.SetDiffuse( RageColor(fPercentRed*0.8f,0,0,1) );

	ActorFrame::DrawPrimitives();
}
#include "RageLog.h"
void LifeMeterBar::UpdateNonstopLifebar(const int cleared, 
		const int total, int ProgressiveLifebarDifficulty)
{
//	if (cleared > total) cleared = total; // clear/total <= 1
//	if (total == 0) total = 1;  // no division by 0

	if (GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2())
	{   // extra stage is its own thing, should not be progressive
	    // and it should be as difficult as life 4
		// (e.g. it should not depend on life settings)

		m_iProgressiveLifebar = 0;
		m_fLifeDifficulty = 1.0f;
		return;
	}

	// should be checked before calling function, but in case
	// it isn't, do so here
	/* No, wait: if we're playing nonstop, event mode just means that we can play another
	 * nonstop course later, so it shouldn't affect life difficulty. */
/*	if (PREFSMAN->m_bEventMode)
	{
		m_fLifeDifficulty = m_fBaseLifeDifficulty;
		return;
	} */

	if (total > 1)
		m_fLifeDifficulty = m_fBaseLifeDifficulty - 0.2f * (int)(ProgressiveLifebarDifficulty * cleared / (total - 1));
	else
		m_fLifeDifficulty = m_fBaseLifeDifficulty - 0.2f * ProgressiveLifebarDifficulty;

	if (m_fLifeDifficulty >= 0.4) return;

    /* Approximate deductions for a miss
	 * Life 1 :    5   %
	 * Life 2 :    5.7 %
	 * Life 3 :    6.6 %
	 * Life 4 :    8   %
	 * Life 5 :   10   %
	 * Life 6 :   13.3 %
	 * Life 7 :   20   %
	 * Life 8 :   26.6 %
	 * Life 9 :   32   %
	 * Life 10:   40   %
	 * Life 11:   50   %
	 * Life 12:   57.1 %
	 * Life 13:   66.6 %
	 * Life 14:   80   %
	 * Life 15:  100   %
	 * Life 16+: 200   %
	 *
	 * Note there is 200%, because boos take off 1/2 as much as
	 * a miss, and a boo would suck up half of your lifebar.
	 *
	 * Everything past 7 is intended mainly for nonstop mode.
     */


	// the lifebar is pretty harsh at 0.4 already (you lose
	// about 20% of your lifebar); at 0.2 it would be 40%, which
	// is too harsh at one difficulty level higher.  Override.

	int m_iLifeDifficulty = int((1.8f - m_fLifeDifficulty)/0.2f);
	
	// first eight values don't matter
	float DifficultyValues[16] = {0,0,0,0,0,0,0,0, 
		0.3f, 0.25f, 0.2f, 0.16f, 0.14f, 0.12f, 0.10f, 0.08f};

	if (m_iLifeDifficulty >= 16)
	{
		// judge 16 or higher
		m_fLifeDifficulty = 0.04f;
		return;
	}

	m_fLifeDifficulty = DifficultyValues[m_iLifeDifficulty];
	return;
}


/*

  Chris:  I'm making the coloring of the lifemeter a property 
  of the theme.  That's where it belongs anyway...

  
const RageColor COLOR_EZ2NORMAL_1	= RageColor(0.7f,0.4f,0,1);
const RageColor COLOR_EZ2NORMAL_2	= RageColor(0.8f,0.4f,0,1);
const RageColor COLOR_EZ2NEARFULL_1	= RageColor(0.7f,0.6f,0,1);
const RageColor COLOR_EZ2NEARFULL_2	= RageColor(0.8f,0.7f,0,1);
const RageColor COLOR_EZ2NEARFAIL_1	= RageColor(0.9f,0.0f,0,1);
const RageColor COLOR_EZ2NEARFAIL_2	= RageColor(0.8f,0.1f,0,1);
const RageColor COLOR_EZ2FULL_1	= RageColor(0.3f,0.9f,0.4f,1);
const RageColor COLOR_EZ2FULL_2	= RageColor(0.2f,0.7f,0.3f,1);
const RageColor COLOR_NORMAL_1	= RageColor(1,1,1,1);
const RageColor COLOR_NORMAL_2	= RageColor(0,1,0,1);
const RageColor COLOR_FULL_1	= RageColor(1,0,0,1);
const RageColor COLOR_FULL_2	= RageColor(1,1,0,1);

RageColor LifeStream::GetColor( float fPercentIntoSection )
{
	float fPercentColor1 = fabsf( fPercentIntoSection*2 - 1 );
	fPercentColor1 *= fPercentColor1 * fPercentColor1 * fPercentColor1;	// make the color bunch around one side
	if (GAMESTATE->m_CurGame != GAME_EZ2)
	{
		if( m_fLifePercentage == 1 )
			return COLOR_FULL_1 * fPercentColor1 + COLOR_FULL_2 * (1-fPercentColor1);
		else
			return COLOR_NORMAL_1 * fPercentColor1 + COLOR_NORMAL_2 * (1-fPercentColor1);
	}
	else
	{
		if( m_fLifePercentage == 1 )
			return COLOR_EZ2FULL_1 * fPercentColor1 + COLOR_EZ2FULL_2 * (1-fPercentColor1);
		else if ( m_fLifePercentage > 0.60 )
			return COLOR_EZ2NEARFULL_1 * fPercentColor1 + COLOR_EZ2NEARFULL_2 * (1-fPercentColor1);
		else if ( m_fLifePercentage < 0.25 )
			return COLOR_EZ2NEARFAIL_1 * fPercentColor1 + COLOR_EZ2NEARFAIL_2 * (1-fPercentColor1);
		else
			return COLOR_EZ2NORMAL_1 * fPercentColor1 + COLOR_EZ2NORMAL_2 * (1-fPercentColor1);
	}
}

void LifeStream::DrawPrimitives()
{
	// make the object in logical units centered at the origin
	LPDIRECT3DVERTEXBUFFER8 pVB = DISPLAY->GetVertexBuffer();
	RageSpriteVertex* v;
	pVB->Lock( 0, 0, (BYTE**)&v, 0 );

	int iNumV = 0;

	float fPercentIntoSection = (RageTimer::GetTimeSinceStart()/0.3f* (m_bIsHot ? 2 : 1) )*LIFE_STREAM_SECTION_WIDTH;
	fPercentIntoSection -= (int)fPercentIntoSection;
	fPercentIntoSection = 1-fPercentIntoSection;
	fPercentIntoSection -= (int)fPercentIntoSection;
	ASSERT( fPercentIntoSection >= 0  &&  fPercentIntoSection < 1 );
	float fX = - 0.5f; 


	while( fX < m_fTrailingLifePercentage-0.5f )
	{
		//
		// draw middle
		//
		v[iNumV++].p = RageVector3( fX, -0.5f, 0 );
		v[iNumV++].p = RageVector3( fX,  0.5f, 0 );
		
		iNumV -= 2;
		const RageColor color = GetColor( fPercentIntoSection ); 
		v[iNumV++].color = color;
		v[iNumV++].color = color;


		if( fPercentIntoSection < 0.5f )
		{
			const float fPercentToAdd = 0.5f-fPercentIntoSection;
			fPercentIntoSection += fPercentToAdd;
			fX += fPercentToAdd*LIFE_STREAM_SECTION_WIDTH;
		}
		else if( fPercentIntoSection < 1.0f )
		{
			const float fPercentToAdd = 1.0f-fPercentIntoSection;
			fPercentIntoSection = 0;
			fX += fPercentToAdd*LIFE_STREAM_SECTION_WIDTH;
		}
		else
			ASSERT( false );
	}

	const float fXToSubtract = fX - (m_fTrailingLifePercentage-0.5f);
	fX -= fXToSubtract;
	fPercentIntoSection -= fXToSubtract/LIFE_STREAM_SECTION_WIDTH;
	if( fPercentIntoSection < 0 )
		fPercentIntoSection += 1;

	//
	// draw right edge
	//
	v[iNumV++].p = RageVector3( fX, -0.5f, 0 );
	v[iNumV++].p = RageVector3( fX,  0.5f, 0 );
	
	iNumV -= 2;
	const RageColor color = GetColor( fPercentIntoSection ); 
	v[iNumV++].color = color;
	v[iNumV++].color = color; 



	pVB->Unlock();


	LPDIRECT3DDEVICE8 pd3dDevice = DISPLAY->GetDevice();
	pd3dDevice->SetTexture( 0, NULL );



	// set texture and alpha properties
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2 );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );

	pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
	pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );


	// finally!  Pump those triangles!	
	pd3dDevice->SetVertexShader( D3DFVF_RageSpriteVertex );
	pd3dDevice->SetStreamSource( 0, pVB, sizeof(RageSpriteVertex) );
	pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, iNumV-2 );

}
*/

void LifeMeterBar::FillForHowToPlay(int NumPerfects, int NumMisses)
{
	m_iProgressiveLifebar = 0;  // disable progressive lifebar

	float AmountForPerfect	= NumPerfects * m_fLifeDifficulty * 0.008f;
	float AmountForMiss		= NumMisses / m_fLifeDifficulty * 0.08f;

	m_fLifePercentage = AmountForMiss - AmountForPerfect;
	CLAMP( m_fLifePercentage, 0.0f, 1.0f );
	AfterLifeChanged();
}
