#ifndef FADINGBANNER_H
#define FADINGBANNER_H
/*
-----------------------------------------------------------------------------
 Class: FadingBanner

 Desc: Fades between two banners.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Banner.h"
#include "ActorFrame.h"
#include "RageTimer.h"

class FadingBanner : public ActorFrame
{
public:
	FadingBanner();

	virtual bool Load( RageTextureID ID );
	void ScaleToClipped( float fWidth, float fHeight );

	void LoadFromSong( Song* pSong );		// NULL means no song
	void LoadAllMusic();
	void LoadSort();
	void LoadMode();
	void LoadFromGroup( CString sGroupName );
	void LoadFromCourse( Course* pCourse );
	void LoadRoulette();
	void LoadRandom();
	void LoadLeap();
	void LoadFallback();

	void SetMovingFast( bool fast ) { m_bMovingFast=fast; }
	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();

protected:
	void BeforeChange();


	Banner		m_Banner[2];
	int			m_iIndexFront;
	int			GetBackIndex() { return m_iIndexFront==0 ? 1 : 0; }

	void LoadFromCachedBanner( const CString &path );

	CString		m_sPendingBanner;
	RageTimer	m_PendingTimer;
	bool		m_bMovingFast;
};

#endif
