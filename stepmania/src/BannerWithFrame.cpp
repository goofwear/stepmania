#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: BannerWithFrame

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "BannerWithFrame.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "RageLog.h"



BannerWithFrame::BannerWithFrame()
{
	m_sprBannerFrame.Load( THEME->GetPathTo(GRAPHIC_EVALUATION_BANNER_FRAME) );
	m_Banner.SetCroppedSize( m_sprBannerFrame.GetUnzoomedWidth()-6, m_sprBannerFrame.GetUnzoomedHeight()-6 );

	this->AddSubActor( &m_Banner );
	this->AddSubActor( &m_sprBannerFrame );
}

void BannerWithFrame::LoadFromSong( Song* pSong )
{
	m_Banner.LoadFromSong( pSong );
}

void BannerWithFrame::LoadFromGroup( CString sGroupName )
{
	m_Banner.LoadFromGroup( sGroupName );
}

void BannerWithFrame::LoadFromCourse( Course* pCourse )
{
	m_Banner.LoadFromCourse( pCourse );
}
