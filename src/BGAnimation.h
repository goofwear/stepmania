#ifndef BGANIMATION_H
#define BGANIMATION_H
/*
-----------------------------------------------------------------------------
 Class: BGAnimation

 Desc: Particles that play in the background of ScreenGameplay

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Ben Nordstrom
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ActorFrame.h"


class BGAnimationLayer;


class BGAnimation : public ActorFrame
{
public:
	BGAnimation();
	virtual ~BGAnimation();

	void Unload();

	void LoadFromStaticGraphic( CString sPath );
	void LoadFromAniDir( CString sAniDir );
	void LoadFromMovie( CString sMoviePath );
	void LoadFromVisualization( CString sMoviePath );

	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();

	virtual void SetDiffuse( const RageColor &c );

	void GainingFocus( float fRate, bool bRewindMovie, bool bLoop );
	void LosingFocus();

	float GetLengthSeconds() { return m_fLengthSeconds; }

	void PlayOffCommand();

protected:
	vector<BGAnimationLayer*> m_Layers;
	float	m_fLengthSeconds;
};


#endif
