#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 File: TransitionStarWipe.cpp

 Desc: Black bands (horizontal window blinds) gradually close.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
-----------------------------------------------------------------------------
*/

#include "RageUtil.h"

#include "TransitionFadeWipe.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"

const float FADE_RECT_WIDTH		=	SCREEN_WIDTH/2;


TransitionFadeWipe::TransitionFadeWipe()
{
}

TransitionFadeWipe::~TransitionFadeWipe()
{

}

void TransitionFadeWipe::DrawPrimitives()
{
	if( m_TransitionState == opened ) 
		return;

	/////////////////////////
	// draw rectangles that get progressively smaller at the edge of the wipe
	/////////////////////////
	bool bLeftEdgeIsDarker = m_TransitionState == opening_left || m_TransitionState == closing_right;
	bool bFadeIsMovingLeft = m_TransitionState == opening_left || m_TransitionState == closing_left;

	float fPercentComplete = m_fPercentThroughTransition;
	float fPercentAlongScreen = bFadeIsMovingLeft ? 1-fPercentComplete*1.3f : fPercentComplete*1.3f;

	float fFadeLeftEdgeX  = fPercentAlongScreen * SCREEN_WIDTH - FADE_RECT_WIDTH/2;
	float fFadeRightEdgeX = fPercentAlongScreen * SCREEN_WIDTH + FADE_RECT_WIDTH/2;

	float fDarkEdgeX = bLeftEdgeIsDarker ? fFadeLeftEdgeX : fFadeRightEdgeX;
	float fLightEdgeX = bLeftEdgeIsDarker ? fFadeRightEdgeX : fFadeLeftEdgeX;

	float fDarkOutsideX = bLeftEdgeIsDarker ? fDarkEdgeX - SCREEN_WIDTH*2 : fDarkEdgeX + SCREEN_WIDTH*2;


	m_rectBlack.SetDiffuse( RageColor(0,0,0,1) );
	m_rectBlack.StretchTo( CRect((int)fDarkOutsideX, 0, (int)fDarkEdgeX, (int)SCREEN_HEIGHT) );
	m_rectBlack.Draw();
	
	m_rectGradient.SetDiffuseLeftEdge( RageColor(0,0,0,1) );
	m_rectGradient.SetDiffuseRightEdge( RageColor(0,0,0,0) );
	m_rectGradient.StretchTo( CRect((int)fDarkEdgeX, 0, (int)fLightEdgeX, (int)SCREEN_HEIGHT) );
	m_rectGradient.Draw();


}


