#include "stdafx.h"	// testing updates
/*
-----------------------------------------------------------------------------
 Class: Actor

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Actor.h"
#include <math.h>
#include "RageDisplay.h"
#include "PrefsManager.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"


Actor::Actor()
{
	m_iNumTweenStates = 0;

	m_size = RageVector2( 1, 1 );

	m_start.Init();
	m_current.Init();

	m_HorizAlign = align_center;
	m_VertAlign = align_middle;

	m_Effect =  no_effect ;
	m_fPercentBetweenColors = 0.0f;
	m_bTweeningTowardEndColor = true;
	m_fDeltaPercentPerSecond = 1.0f;
	m_fWagRadians =  0.2f;
	m_fWagPeriod =  2.0f;
	m_fWagTimer =  0.0f;
	m_vSpinVelocity =  RageVector3(0,0,0);
	m_fVibrationDistance =  5.0f;
	m_bVisibleThisFrame =  FALSE;

	m_bShadow = false;
	m_fShadowLength = 4;

	m_bBlendAdd = false;

}


void Actor::Draw()
{
	// call the most-derived versions
	this->BeginDraw();	
	this->DrawPrimitives();	// call the most-derived version of DrawPrimitives();
	this->EndDraw();	
}

void Actor::BeginDraw()		// set the world matrix and calculate actor properties
{
	DISPLAY->PushMatrix();	// we're actually going to do some drawing in this function	

	int i;

	m_temp = m_current;


	//
	// set temporary drawing properties based on Effects 
	//
	switch( m_Effect )
	{
	case no_effect:
		break;
	case blinking:
		for(i=0; i<4; i++)
			m_temp.diffuse[i] = m_bTweeningTowardEndColor ? m_effect_colorDiffuse1 : m_effect_colorDiffuse2;
		break;
	case camelion:
		for(i=0; i<4; i++)
			m_temp.diffuse[i] = m_effect_colorDiffuse1*m_fPercentBetweenColors + m_effect_colorDiffuse2*(1.0f-m_fPercentBetweenColors);
		break;
	case glowing:
		float fCurvedPercent;
		fCurvedPercent = sinf( m_fPercentBetweenColors * PI );
		m_temp.glow = m_effect_colorGlow1*fCurvedPercent + m_effect_colorGlow2*(1.0f-fCurvedPercent);
		break;
	case wagging:
		m_temp.rotation.z = m_fWagRadians * sinf( 
			(m_fWagTimer / m_fWagPeriod)	// percent through wag
			* 2.0f * PI );
		break;
	case spinning:
		// nothing needs to be here
		break;
	case vibrating:
		m_temp.pos.x += m_fVibrationDistance * randomf(-1.0f, 1.0f) * GetZoom();
		m_temp.pos.y += m_fVibrationDistance * randomf(-1.0f, 1.0f) * GetZoom();
		break;
	case flickering:
		m_bVisibleThisFrame = !m_bVisibleThisFrame;
		if( !m_bVisibleThisFrame )
			for(int i=0; i<4; i++)
				m_temp.diffuse[i] = RageColor(0,0,0,0);		// don't draw the frame
		break;
	case bouncing:
		{
		float fPercentThroughBounce = m_fTimeIntoBounce / m_fBouncePeriod;
		float fPercentOffset = sinf( fPercentThroughBounce*PI ); 
		m_temp.pos += m_vectBounce * fPercentOffset;
		}
		break;
	case bobbing:
		{
		float fPercentThroughBounce = m_fTimeIntoBounce / m_fBouncePeriod;
		float fPercentOffset = sinf( fPercentThroughBounce*PI*2 ); 
		m_temp.pos += m_vectBounce * fPercentOffset;
		}
		break;
	default:
		ASSERT(0);	// invalid Effect
	}



	
	DISPLAY->Translate( m_temp.pos.x, m_temp.pos.y, m_temp.pos.z );
	DISPLAY->Scale( m_temp.scale.x, m_temp.scale.y, 1 );

	// super slow, and most Actors don't have any rotation	
	//	RageMatrixRotationYawPitchRoll( &matTemp, rotation.y, rotation.x, rotation.z );	// add in the rotation
	//	matNewWorld = matTemp * matNewWorld;

	// replace with...
	if( m_temp.rotation.x != 0 )	DISPLAY->RotateX( m_temp.rotation.x );
	if( m_temp.rotation.y != 0 )	DISPLAY->RotateY( m_temp.rotation.y );
	if( m_temp.rotation.z != 0 )	DISPLAY->RotateZ( m_temp.rotation.z );
}

void Actor::EndDraw()
{
	DISPLAY->PopMatrix();
}

void Actor::UpdateTweening( float fDeltaTime )
{
	// update tweening
	if( m_iNumTweenStates == 0 )
		return;

	// we are performing a tween
	TweenState &TS = m_TweenStates[0];		// earliest tween
	TweenInfo  &TI = m_TweenInfo[0];		// earliest tween

	if( TI.m_fTimeLeftInTween == TI.m_fTweenTime )	// we are just beginning this tween
	{
		// set the start position
		m_start = m_current;
	}
	
	TI.m_fTimeLeftInTween -= fDeltaTime;

	if( TI.m_fTimeLeftInTween <= 0 )	// The tweening is over.  Stop the tweening
	{
		m_current = TS;
		
		// delete the head tween
		for( int i=0; i<m_iNumTweenStates-1; i++ )
		{
			m_TweenStates[i] = m_TweenStates[i+1];
            m_TweenInfo[i] = m_TweenInfo[i+1];
		}
		m_iNumTweenStates--;
	}
	else		// in the middle of tweening.  Recalcute the current position.
	{
		const float fPercentThroughTween = 1-(TI.m_fTimeLeftInTween / TI.m_fTweenTime);

		// distort the percentage if appropriate
		float fPercentAlongPath = 0.f;
		switch( TI.m_TweenType )
		{
		case TWEEN_LINEAR:		fPercentAlongPath = fPercentThroughTween;													break;
		case TWEEN_BIAS_BEGIN:	fPercentAlongPath = 1 - (1-fPercentThroughTween) * (1-fPercentThroughTween);				break;
		case TWEEN_BIAS_END:	fPercentAlongPath = fPercentThroughTween * fPercentThroughTween;							break;
		case TWEEN_BOUNCE_BEGIN:fPercentAlongPath = 1 - sinf( 1.1f + fPercentThroughTween*(PI-1.1f) ) / 0.89f;				break;
		case TWEEN_BOUNCE_END:	fPercentAlongPath = sinf( 1.1f + (1-fPercentThroughTween)*(PI-1.1f) ) / 0.89f;				break;
		case TWEEN_SPRING:		fPercentAlongPath = 1 - cosf( fPercentThroughTween*PI*2.5f )/(1+fPercentThroughTween*3);	break;
		default:	ASSERT(0);
		}

		m_current.pos			= m_start.pos	  + (TS.pos		 - m_start.pos	   )*fPercentAlongPath;
		m_current.scale			= m_start.scale	  + (TS.scale	 - m_start.scale   )*fPercentAlongPath;
		m_current.rotation		= m_start.rotation+ (TS.rotation - m_start.rotation)*fPercentAlongPath;
		for(int i=0; i<4; i++) 
			m_current.diffuse[i]= m_start.diffuse[i]*(1.0f-fPercentAlongPath) + TS.diffuse[i]*(fPercentAlongPath);
		m_current.glow			= m_start.glow      *(1.0f-fPercentAlongPath) + TS.glow      *(fPercentAlongPath);
	}
}

void Actor::Update( float fDeltaTime )
{
//	LOG->Trace( "Actor::Update( %f )", fDeltaTime );

	// update effect
	switch( m_Effect )
	{
	case no_effect:
		break;
	case blinking:
	case camelion:
	case glowing:
		if( m_bTweeningTowardEndColor ) {
			m_fPercentBetweenColors += m_fDeltaPercentPerSecond * fDeltaTime;
			if( m_fPercentBetweenColors > 1.0f ) {
				m_fPercentBetweenColors = 1.0f;

				m_bTweeningTowardEndColor = FALSE;
			}
		}
		else {		// !m_bTweeningTowardEndColor
			m_fPercentBetweenColors -= m_fDeltaPercentPerSecond * fDeltaTime;
			if( m_fPercentBetweenColors < 0.0f ) {
				m_fPercentBetweenColors = 0.0f;
				m_bTweeningTowardEndColor = TRUE;
			}
		}
		//LOG->Trace( "Actor::m_fPercentBetweenColors %f", m_fPercentBetweenColors );
		break;
	case wagging:
		m_fWagTimer += fDeltaTime;
		if( m_fWagTimer > m_fWagPeriod )
			m_fWagTimer -= m_fWagPeriod;
		break;
	case spinning:
		m_current.rotation += m_vSpinVelocity;
		if( m_current.rotation.x > 1000*PI*2 )	m_current.rotation.x -= 1000*PI*2;
		if( m_current.rotation.y > 1000*PI*2 )	m_current.rotation.y -= 1000*PI*2;
		if( m_current.rotation.z > 1000*PI*2 )	m_current.rotation.z -= 1000*PI*2;
		break;
	case vibrating:
		break;
	case flickering:
		break;
	case bouncing:
	case bobbing:
		m_fTimeIntoBounce += fDeltaTime;
		if( m_fTimeIntoBounce >= m_fBouncePeriod )
			m_fTimeIntoBounce -= m_fBouncePeriod;
		break;
	}

	UpdateTweening( fDeltaTime );
}

void Actor::BeginTweening( float time, TweenType tt )
{
	ASSERT( time >= 0 );

	// HACK to keep from out of bounds access..
	if( m_iNumTweenStates == MAX_TWEEN_STATES )
	{
		for( int i=0; i<m_iNumTweenStates-1; i++ )
		{
			m_TweenStates[i] = m_TweenStates[i+1];
			m_TweenInfo[i] = m_TweenInfo[i+1];
		}
	}

	// add a new TweenState to the tail, and initialize it
	m_iNumTweenStates++;
	TweenState &TS = m_TweenStates[m_iNumTweenStates-1];	// latest
	TweenInfo  &TI = m_TweenInfo[m_iNumTweenStates-1];		// latest

	if( m_iNumTweenStates >= 2 )		// if there was already a TS on the stack
	{
		// initialize the new TS from the last TS in the list
		TS = m_TweenStates[m_iNumTweenStates-2];
	}
	else
	{
		// This new TS is the only TS.
		// Set our tween starting and ending values to the current position.
		TS = m_current;
	}

	TI.m_TweenType = tt;
	TI.m_fTweenTime = time;
	TI.m_fTimeLeftInTween = time;
}

void Actor::StopTweening()
{
	m_iNumTweenStates = 0;
}


void Actor::SetTweenX( float x )				{ LatestTween().pos.x = x; } 
void Actor::SetTweenY( float y )				{ LatestTween().pos.y = y; }
void Actor::SetTweenZ( float z )				{ LatestTween().pos.z = z; }
void Actor::SetTweenXY( float x, float y )		{ LatestTween().pos.x = x; LatestTween().pos.y = y; }
void Actor::SetTweenZoom( float zoom )			{ LatestTween().scale.x = zoom;  LatestTween().scale.y = zoom; }
void Actor::SetTweenZoomX( float zoom )			{ LatestTween().scale.x = zoom; }
void Actor::SetTweenZoomY( float zoom )			{ LatestTween().scale.y = zoom; }
void Actor::SetTweenZoomToWidth( float zoom )	{ SetTweenZoomX( zoom/GetUnzoomedWidth() ); }
void Actor::SetTweenZoomToHeight( float zoom )	{ SetTweenZoomX( zoom/GetUnzoomedHeight() ); }
void Actor::SetTweenRotationX( float r )		{ LatestTween().rotation.x = r; }
void Actor::SetTweenRotationY( float r )		{ LatestTween().rotation.y = r; }
void Actor::SetTweenRotationZ( float r )		{ LatestTween().rotation.z = r; }
void Actor::SetTweenDiffuse( RageColor c )				{ for(int i=0; i<4; i++) LatestTween().diffuse[i] = c; };
void Actor::SetTweenDiffuseUpperLeft( RageColor c )		{ LatestTween().diffuse[0] = c; };
void Actor::SetTweenDiffuseUpperRight( RageColor c )	{ LatestTween().diffuse[1] = c; };
void Actor::SetTweenDiffuseLowerLeft( RageColor c )		{ LatestTween().diffuse[2] = c; };
void Actor::SetTweenDiffuseLowerRight( RageColor c )	{ LatestTween().diffuse[3] = c; };
void Actor::SetTweenDiffuseTopEdge( RageColor c )		{ LatestTween().diffuse[0] = LatestTween().diffuse[1] = c; };
void Actor::SetTweenDiffuseRightEdge( RageColor c )		{ LatestTween().diffuse[1] = LatestTween().diffuse[3] = c; };
void Actor::SetTweenDiffuseBottomEdge( RageColor c )	{ LatestTween().diffuse[2] = LatestTween().diffuse[3] = c; };
void Actor::SetTweenDiffuseLeftEdge( RageColor c )		{ LatestTween().diffuse[0] = LatestTween().diffuse[2] = c; };
void Actor::SetTweenGlow( RageColor c )					{ LatestTween().glow = c; };


void Actor::ScaleTo( const RectI &rect, StretchType st )
{
	// width and height of rectangle
	float rect_width = (float)RECTWIDTH(rect);
	float rect_height = (float)RECTHEIGHT(rect);

	if( rect_width < 0 )	SetRotationY( PI );
	if( rect_height < 0 )	SetRotationX( PI );

	// center of the rectangle
	float rect_cx = rect.left + rect_width/2;
	float rect_cy = rect.top  + rect_height/2;

	// zoom fActor needed to scale the Actor to fill the rectangle
	float fNewZoomX = fabsf(rect_width  / m_size.x);
	float fNewZoomY = fabsf(rect_height / m_size.y);

	float fNewZoom = 0.f;
	switch( st )
	{
	case cover:
		fNewZoom = fNewZoomX>fNewZoomY ? fNewZoomX : fNewZoomY;	// use larger zoom
		break;
	case fit_inside:
		fNewZoom = fNewZoomX>fNewZoomY ? fNewZoomY : fNewZoomX; // use smaller zoom
		break;
	}

	SetXY( rect_cx, rect_cy );
	SetZoom( fNewZoom );
}


void Actor::StretchTo( const RectI &rect )
{
	// width and height of rectangle
	int rect_width = RECTWIDTH(rect);
	int rect_height = RECTHEIGHT(rect);

	// center of the rectangle
	float rect_cx = rect.left + rect_width/2.0f;
	float rect_cy = rect.top  + rect_height/2.0f;

	// zoom fActor needed to scale the Actor to fill the rectangle
	float fNewZoomX = rect_width  / m_size.x;
	float fNewZoomY = rect_height / m_size.y;

	SetXY( (float)rect_cx, (float)rect_cy );
	SetZoomX( fNewZoomX );
	SetZoomY( fNewZoomY );
}




// effects

void Actor::SetEffectNone()
{
	m_Effect = no_effect;
	//m_color = RageColor( 1.0,1.0,1.0,1.0 );
}

void Actor::SetEffectBlinking( float fDeltaPercentPerSecond, RageColor Color, RageColor Color2 )
{
	m_Effect = blinking;
	m_effect_colorDiffuse1 = Color;
	m_effect_colorDiffuse2 = Color2;

	m_fDeltaPercentPerSecond = fDeltaPercentPerSecond;
}

void Actor::SetEffectCamelion( float fDeltaPercentPerSecond, RageColor Color, RageColor Color2 )
{
	m_Effect = camelion;
	m_effect_colorDiffuse1 = Color;
	m_effect_colorDiffuse2 = Color2;

	m_fDeltaPercentPerSecond = fDeltaPercentPerSecond;
}

void Actor::SetEffectGlowing( float fDeltaPercentPerSecond, RageColor Color, RageColor Color2 )
{
	m_Effect = glowing;
	m_effect_colorGlow1 = Color;
	m_effect_colorGlow2 = Color2;

	m_fDeltaPercentPerSecond = fDeltaPercentPerSecond;
}

void Actor::SetEffectWagging(	float fWagRadians, float fWagPeriod )
{
	m_Effect = wagging;
	m_fWagRadians = fWagRadians;
	m_fWagPeriod = fWagPeriod;
}

void Actor::SetEffectSpinning( RageVector3 vectRotationVelocity )
{
	m_Effect = spinning;
	m_vSpinVelocity = vectRotationVelocity;
}

void Actor::SetEffectVibrating( float fVibrationDistance )
{
	m_Effect = vibrating;
	m_fVibrationDistance = fVibrationDistance;
}

void Actor::SetEffectFlickering()
{
	m_Effect = flickering;
}

void Actor::SetEffectBouncing( RageVector3 vectBounce, float fPeriod )
{
	m_Effect = bouncing;
	
	m_vectBounce = vectBounce;
	m_fBouncePeriod = fPeriod;
	m_fTimeIntoBounce = 0;

}

void Actor::SetEffectBobbing( RageVector3 vectBob, float fPeriod )
{
	m_Effect = bobbing;
	
	m_vectBounce = vectBob;
	m_fBouncePeriod = fPeriod;
	m_fTimeIntoBounce = 0;

}


void GetFadeFlagsFromString( CString sFadeString, 
	bool& linear, bool& accelerate, bool& bounce, bool& spring, 
	bool& foldx, bool& foldy, bool& fade, bool& zoombig, bool& zoomsmall, bool& spinx, bool& spiny, bool& spinz, bool& ghost, 
	bool& left, bool& right, bool& top, bool& bottom )
{
	sFadeString.MakeLower();
	foldx = -1!=sFadeString.Find("foldx");
	foldy = -1!=sFadeString.Find("foldy");
	fade = -1!=sFadeString.Find("fade");
	zoombig = -1!=sFadeString.Find("zoombig");
	zoomsmall = -1!=sFadeString.Find("zoomsmall");
	spinx = -1!=sFadeString.Find("spinx");
	spiny = -1!=sFadeString.Find("spiny");
	spinz = -1!=sFadeString.Find("spinz");
	ghost = -1!=sFadeString.Find("ghost");
	linear = -1!=sFadeString.Find("linear");
	accelerate = -1!=sFadeString.Find("accelerate");
	bounce = -1!=sFadeString.Find("bounce");
	spring = -1!=sFadeString.Find("spring");
	left = -1!=sFadeString.Find("left");
	right = -1!=sFadeString.Find("right");
	top = -1!=sFadeString.Find("top");
	bottom = -1!=sFadeString.Find("bottom");
}



void Actor::Fade( float fSleepSeconds, CString sFadeString, float fFadeSeconds, bool bOnToScreenOrOffOfScreen )
{
	sFadeString.MakeLower();

	TweenState original = m_current;
	TweenState mod = m_current;

#define CONTAINS(needle)	(-1!=sFadeString.Find(needle))

	TweenType tt;
	if( CONTAINS("linear") )			tt = TWEEN_LINEAR;
	else if( CONTAINS("accelerate") )	tt = TWEEN_BIAS_BEGIN;
	else if( CONTAINS("bounce") )		tt = TWEEN_BOUNCE_END;
	else if( CONTAINS("spring") )		tt = TWEEN_SPRING;
	else								tt = TWEEN_LINEAR;

	mod.pos.x		+= (CONTAINS("left")?-SCREEN_WIDTH:0) + (CONTAINS("right")?+SCREEN_HEIGHT:0);
	mod.pos.y		+= (CONTAINS("top")?-SCREEN_WIDTH:0)  + (CONTAINS("bottom")?+SCREEN_HEIGHT:0);
	mod.pos.z		+= 0;
	mod.rotation.x	+= (CONTAINS("spinx")?-PI*2:0);
	mod.rotation.y	+= (CONTAINS("spiny")?-PI*2:0);
	mod.rotation.z	+= (CONTAINS("spinz")?-PI*2:0);
	mod.scale.x		*= (CONTAINS("foldx")?0:1) * (CONTAINS("zoomx")?3:1);
	mod.scale.y		*= (CONTAINS("foldy")?0:1) * (CONTAINS("zoomy")?3:1);
	for( int i=0; i<4; i++ )
	{
		mod.diffuse[i] = GetDiffuse();
		mod.diffuse[i].a *= CONTAINS("fade")?0:1;
	}
	mod.glow = GetGlow();
	mod.glow.a *= CONTAINS("glow")?1:0;


	StopTweening();
	m_current = bOnToScreenOrOffOfScreen ? original : mod;
	BeginTweening( fSleepSeconds );
	BeginTweening( fFadeSeconds, tt );
	LatestTween() = bOnToScreenOrOffOfScreen ? mod : original;
}

