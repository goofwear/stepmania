#include "global.h"
/*
-----------------------------------------------------------------------------
 File: ArrowEffects.cpp

 Desc: Functions that return properties of arrows based on StyleDef and PlayerOptions

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ArrowEffects.h"
#include "Steps.h"
#include "GameConstantsAndTypes.h"
#include "GameManager.h"
#include "GameState.h"
#include "RageException.h"
#include "RageTimer.h"
#include "NoteDisplay.h"
#include "song.h"
#include <math.h>
#include "RageMath.h"

const float ARROW_SPACING	= ARROW_SIZE;// + 2;

float		g_fExpandSeconds = 0;

float ArrowGetYOffset( PlayerNumber pn, int iCol, float fNoteBeat )
{
	float fYOffset;
	if( !GAMESTATE->m_PlayerOptions[pn].m_bTimeSpacing )
	{
		float fSongBeat = GAMESTATE->m_fSongBeat;
		float fBeatsUntilStep = fNoteBeat - fSongBeat;
		fYOffset = fBeatsUntilStep * ARROW_SPACING;
	}
	else
	{
		float fSongSeconds = GAMESTATE->m_fMusicSeconds;
		float fNoteSeconds = GAMESTATE->m_pCurSong->GetElapsedTimeFromBeat(fNoteBeat);
		float fSecondsUntilStep = fNoteSeconds - fSongSeconds;
		float fBPM = GAMESTATE->m_PlayerOptions[pn].m_fScrollBPM;
		float fBPS = fBPM/60.f;
		fYOffset = fSecondsUntilStep * fBPS * ARROW_SPACING;
	}

	// don't mess with the arrows after they've crossed 0
	if( fYOffset < 0 )
		return fYOffset;

	const float* fAccels = GAMESTATE->m_CurrentPlayerOptions[pn].m_fAccels;
	//const float* fEffects = GAMESTATE->m_CurrentPlayerOptions[pn].m_fEffects;


	float fYAdjust = 0;	// fill this in depending on PlayerOptions

	if( fAccels[PlayerOptions::ACCEL_BOOST] > 0 )
	{
		float fNewYOffset = fYOffset * 1.5f / ((fYOffset+SCREEN_HEIGHT/1.2f)/SCREEN_HEIGHT); 
		fYAdjust +=	fAccels[PlayerOptions::ACCEL_BOOST] * (fNewYOffset - fYOffset);
	}
	if( fAccels[PlayerOptions::ACCEL_BRAKE] > 0 )
	{
		float fScale = SCALE( fYOffset, 0.f, SCREEN_HEIGHT, 0, 1.f );
		float fNewYOffset = fYOffset * fScale; 
		fYAdjust += (fNewYOffset - fYOffset);
	}
	if( fAccels[PlayerOptions::ACCEL_WAVE] > 0 )
		fYAdjust +=	fAccels[PlayerOptions::ACCEL_WAVE] * 20.0f*sinf( fYOffset/38.0f );
	if( fAccels[PlayerOptions::ACCEL_EXPAND] > 0 )
	{
		/* Timers can't be global, since they'll be initialized before SDL. */
		static RageTimer timerExpand;
		if( !GAMESTATE->m_bFreeze )
			g_fExpandSeconds += timerExpand.GetDeltaTime();
		else
			timerExpand.GetDeltaTime();	// throw away
		fYAdjust +=	fAccels[PlayerOptions::ACCEL_EXPAND] * (fYOffset * SCALE( cosf(g_fExpandSeconds*3), -1, 1, 0.5f, 1.5f ) - fYOffset); 
	}
	if( fAccels[PlayerOptions::ACCEL_BOOMERANG] > 0 )
		fYAdjust +=	fAccels[PlayerOptions::ACCEL_BOOMERANG] * (fYOffset * SCALE( fYOffset, 0.f, SCREEN_HEIGHT, 1.5f, 0.5f )- fYOffset);

	return fYOffset + fYAdjust;
}

float ArrowGetYPosWithoutReverse( PlayerNumber pn, int iCol, float fYOffset )
{
	float fYPos = fYOffset * GAMESTATE->m_CurrentPlayerOptions[pn].m_fScrollSpeed;
	return fYPos;
}

float ArrowGetYPos( PlayerNumber pn, int iCol, float fYOffset, float fYReverseOffsetPixels )
{
	float f = ArrowGetYPosWithoutReverse(pn,iCol,fYOffset);
	f *= SCALE( GAMESTATE->m_CurrentPlayerOptions[pn].GetReversePercentForColumn(iCol), 0.f, 1.f, 1.f, -1.f );

	/* XXX: Hack: we need to scale the reverse shift by the zoom. */
	float fMiniPercent = GAMESTATE->m_CurrentPlayerOptions[pn].m_fEffects[PlayerOptions::EFFECT_MINI];
	float fZoom = 1 - fMiniPercent*0.5f;

	f += SCALE( GAMESTATE->m_CurrentPlayerOptions[pn].GetReversePercentForColumn(iCol), 0.f, 1.f, -fYReverseOffsetPixels/fZoom/2, fYReverseOffsetPixels/fZoom/2 );

	const float* fEffects = GAMESTATE->m_CurrentPlayerOptions[pn].m_fEffects;

	if( fEffects[PlayerOptions::EFFECT_TIPSY] > 0 )
		f += fEffects[PlayerOptions::EFFECT_TIPSY] * ( cosf( RageTimer::GetTimeSinceStart()*1.2f + iCol*2.f) * ARROW_SIZE*0.4f );
	return f;
}


float ArrowGetXPos( PlayerNumber pn, int iColNum, float fYPos ) 
{
	float fPixelOffsetFromCenter = 0;
	
	const float* fEffects = GAMESTATE->m_CurrentPlayerOptions[pn].m_fEffects;

	if( fEffects[PlayerOptions::EFFECT_TORNADO] > 0 )
	{
		float fMinX, fMaxX;
		GAMESTATE->GetCurrentStyleDef()->GetMinAndMaxColX( pn, fMinX, fMaxX );

		const float fRealPixelOffset = GAMESTATE->GetCurrentStyleDef()->m_ColumnInfo[pn][iColNum].fXOffset;
		const float fPositionBetween = SCALE( fRealPixelOffset, fMinX, fMaxX, -1, 1 );
		float fRads = acosf( fPositionBetween );
		fRads += fYPos * 6 / SCREEN_HEIGHT;
		
		const float fAdjustedPixelOffset = SCALE( cosf(fRads), -1, 1, fMinX, fMaxX );

		fPixelOffsetFromCenter = fAdjustedPixelOffset - fRealPixelOffset;
	}

	if( fEffects[PlayerOptions::EFFECT_DRUNK] > 0 )
		fPixelOffsetFromCenter += fEffects[PlayerOptions::EFFECT_DRUNK] * ( cosf( RageTimer::GetTimeSinceStart() + iColNum*0.2f + fYPos*10/SCREEN_HEIGHT) * ARROW_SIZE*0.5f );
	if( fEffects[PlayerOptions::EFFECT_FLIP] > 0 )
	{
//		fPixelOffsetFromCenter *= SCALE(fEffects[PlayerOptions::EFFECT_FLIP], 0.f, 1.f, 1.f, -1.f);
		const float fRealPixelOffset = GAMESTATE->GetCurrentStyleDef()->m_ColumnInfo[pn][iColNum].fXOffset;
		const float fDistance = -fRealPixelOffset * 2;
		fPixelOffsetFromCenter += fDistance * fEffects[PlayerOptions::EFFECT_FLIP];
	}

	if( fEffects[PlayerOptions::EFFECT_BEAT] > 0 )
	do {
		float fAccelTime = 0.2f, fTotalTime = 0.5f;
		
		/* If the song is really fast, slow down the rate, but speed up the
		 * acceleration to compensate or it'll look weird. */
		const float fBPM = GAMESTATE->m_fCurBPS * 60;
		const float fDiv = max(1.0f, truncf( fBPM / 150.0f ));
		fAccelTime /= fDiv;
		fTotalTime /= fDiv;

		float fBeat = GAMESTATE->m_fSongBeat + fAccelTime;
		fBeat /= fDiv;

		const bool bEvenBeat = ( int(fBeat) % 2 ) != 0;

		/* -100.2 -> -0.2 -> 0.2 */
		if( fBeat < 0 )
			break;

		fBeat -= truncf( fBeat );
		fBeat += 1;
		fBeat -= truncf( fBeat );

		if( fBeat >= fTotalTime )
			break;

		float fAmount;
		if( fBeat < fAccelTime )
		{
			fAmount = SCALE( fBeat, 0.0f, fAccelTime, 0.0f, 1.0f);
			fAmount *= fAmount;
		} else /* fBeat < fTotalTime */ {
			fAmount = SCALE( fBeat, fAccelTime, fTotalTime, 1.0f, 0.0f);
			fAmount = 1 - (1-fAmount) * (1-fAmount);
		}

		if( bEvenBeat )
			fAmount *= -1;

		const float fShift = 20.0f*fAmount*sinf( fYPos / 15.0f + PI/2.0f );
		fPixelOffsetFromCenter += fEffects[PlayerOptions::EFFECT_BEAT] * fShift;
	} while(0);

	return fPixelOffsetFromCenter;
}

float ArrowGetRotation( PlayerNumber pn, float fNoteBeat ) 
{
	if( GAMESTATE->m_CurrentPlayerOptions[pn].m_fEffects[PlayerOptions::EFFECT_DIZZY] > 0 )
	{
		float fSongBeat = GAMESTATE->m_fSongBeat;
		float fDizzyRotation = fNoteBeat - fSongBeat;
		fDizzyRotation = fmodf( fDizzyRotation, 2*PI );
		fDizzyRotation *= 180/PI;
		return fDizzyRotation * GAMESTATE->m_CurrentPlayerOptions[pn].m_fEffects[PlayerOptions::EFFECT_DIZZY];
	}
	else
		return 0;
}


const float fCenterLine = 160;	// from fYPos == 0
const float fFadeDist = 100;

// used by ArrowGetAlpha and ArrowGetGlow below
static float ArrowGetPercentVisible( PlayerNumber pn, int iCol, float fYPos, float fYReverseOffsetPixels )
{
	/* Ack.  What we're really doing here is unapplying reverse--what we really want is the Y position
	 * *with* EFFECT_TIPSY, but without reverse; this is tricky since EFFECT_TIPSY is applied after
	 * reverse.  This would be more straightforward if we were passed fYPosWORev; we could just apply
	 * EFFECT_TIPSY.  That would make the interface more complicated, however. */
	const float fMiniPercent = GAMESTATE->m_CurrentPlayerOptions[pn].m_fEffects[PlayerOptions::EFFECT_MINI];
	const float fZoom = 1 - fMiniPercent*0.5f;
	if( GAMESTATE->m_CurrentPlayerOptions[pn].GetReversePercentForColumn(iCol) > 0.5f )
	{
		fYPos -= SCALE( GAMESTATE->m_CurrentPlayerOptions[pn].GetReversePercentForColumn(iCol), 0.f, 1.f, 0.f, fYReverseOffsetPixels/fZoom );
		fYPos /= SCALE( GAMESTATE->m_CurrentPlayerOptions[pn].GetReversePercentForColumn(iCol), 0.f, 1.f, 1.f, -1.f );

	}

	/* Another mini hack: if EFFECT_MINI is on, then our fYPos range is eg. 0..640, when we really
	 * want 0..320: */
	fYPos *= fZoom;

	const float fDistFromCenterLine = fYPos - fCenterLine;

	if( fYPos < 0 )	// past Gray Arrows
		return 1;	// totally visible


	const float* fAppearances = GAMESTATE->m_CurrentPlayerOptions[pn].m_fAppearances;

	float fVisibleAdjust = 0;

	if( fAppearances[PlayerOptions::APPEARANCE_HIDDEN] > 0 )
		fVisibleAdjust += fAppearances[PlayerOptions::APPEARANCE_HIDDEN] * SCALE( fDistFromCenterLine, 0, fFadeDist, -1, 0 );
	if( fAppearances[PlayerOptions::APPEARANCE_SUDDEN] > 0 )
		fVisibleAdjust += fAppearances[PlayerOptions::APPEARANCE_SUDDEN] * SCALE( fDistFromCenterLine, 0, -fFadeDist, -1, 0 );
	if( fAppearances[PlayerOptions::APPEARANCE_STEALTH] > 0 )
		fVisibleAdjust += fAppearances[PlayerOptions::APPEARANCE_STEALTH] * -1;
	if( fAppearances[PlayerOptions::APPEARANCE_BLINK] > 0 )
	{
		float f = sinf(RageTimer::GetTimeSinceStart()*10);
        f = froundf( f, 0.3333f );
		fVisibleAdjust += SCALE( f, 0, 1, -1, 0 );
	}
	if( fAppearances[PlayerOptions::APPEARANCE_RANDOMVANISH] > 0)
	{
		
		float adjustment = GAMESTATE->m_CurrentPlayerOptions[pn].m_fScrollSpeed;
		if(adjustment == 0)
			adjustment = 1;

		float fRealCenterLine = fCenterLine / adjustment;
		const float fDistFromRealCenterLine = fYPos - fRealCenterLine;
		float fRealFadeDist = (fFadeDist - 20) / adjustment;
		
		if(fYPos >= fRealCenterLine + (fRealFadeDist))
		{
			fVisibleAdjust += fAppearances[PlayerOptions::APPEARANCE_RANDOMVANISH] * (SCALE( fDistFromRealCenterLine-fRealFadeDist, 0, (fRealFadeDist), -1, 0 )); // go hidden
		}
		else if(fYPos <= fRealCenterLine - (fRealFadeDist))
		{
			fVisibleAdjust += fAppearances[PlayerOptions::APPEARANCE_RANDOMVANISH] * (SCALE( fDistFromRealCenterLine+fRealFadeDist, 0, -((fRealFadeDist)), -1, 0 )); // suddenly appear
		}
		else
		{
			fVisibleAdjust += fAppearances[PlayerOptions::APPEARANCE_RANDOMVANISH] * -1; // be invisible
		}
	}

	return clamp( 1+fVisibleAdjust, 0, 1 );
}

float ArrowGetAlpha( PlayerNumber pn, int iCol, float fYPos, float fPercentFadeToFail, float fYReverseOffsetPixels )
{
//	fYPos /= GAMESTATE->m_CurrentPlayerOptions[pn].m_fScrollSpeed;
	float fPercentVisible = ArrowGetPercentVisible(pn,iCol,fYPos,fYReverseOffsetPixels);

	if( fPercentFadeToFail != -1 )
		fPercentVisible = 1 - fPercentFadeToFail;

	return (fPercentVisible>0.5f) ? 1.0f : 0.0f;
}

float ArrowGetGlow( PlayerNumber pn, int iCol, float fYPos, float fPercentFadeToFail, float fYReverseOffsetPixels )
{
//	fYPos /= GAMESTATE->m_CurrentPlayerOptions[pn].m_fScrollSpeed;
	float fPercentVisible = ArrowGetPercentVisible(pn,iCol,fYPos,fYReverseOffsetPixels);

	if( fPercentFadeToFail != -1 )
		fPercentVisible = 1 - fPercentFadeToFail;

	const float fDistFromHalf = fabsf( fPercentVisible - 0.5f );
	return SCALE( fDistFromHalf, 0, 0.5f, 1.3f, 0 );
}

float ArrowGetBrightness( PlayerNumber pn, float fNoteBeat )
{
	if( GAMESTATE->m_bEditing )
		return 1;

	float fSongBeat = GAMESTATE->m_fSongBeat;
	float fBeatsUntilStep = fNoteBeat - fSongBeat;

	float fBrightness = SCALE( fBeatsUntilStep, 0, -1, 1.f, 0.f );
	CLAMP( fBrightness, 0, 1 );
	return fBrightness;
}


float ArrowGetZPos( PlayerNumber pn, int iCol, float fYPos )
{
	float fZPos=0;
	const float* fEffects = GAMESTATE->m_CurrentPlayerOptions[pn].m_fEffects;

	if( fEffects[PlayerOptions::EFFECT_BUMPY] > 0 )
		fZPos += fEffects[PlayerOptions::EFFECT_BUMPY] * 40*sinf( fYPos/16.0f );

	return fZPos;
}

bool ArrowsNeedZBuffer( PlayerNumber pn )
{
	const float* fEffects = GAMESTATE->m_CurrentPlayerOptions[pn].m_fEffects;
	if( fEffects[PlayerOptions::EFFECT_BUMPY] > 0 )
		return true;

	return false;
}

