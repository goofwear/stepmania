#include "global.h"
#include "FadingBanner.h"
#include "RageTextureManager.h"
#include "BannerCache.h"
#include "song.h"
#include "RageLog.h"
#include "Course.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "SongManager.h"

/* XXX: metric */
CachedThemeMetricF FADE_SECONDS			("FadingBanner","FadeSeconds");
CachedThemeMetricF MW_SWITCH_SECONDS	("MusicWheel","SwitchSeconds");


FadingBanner::FadingBanner()
{
	FADE_SECONDS.Refresh();
	MW_SWITCH_SECONDS.Refresh();

	m_bMovingFast = false;
	m_bSkipNextBannerUpdate = false;
	m_iIndexFront = 0;
	for( int i=0; i<2; i++ )
		this->AddChild( &m_Banner[i] );
}

void FadingBanner::ScaleToClipped( float fWidth, float fHeight )
{
	for( int i=0; i<2; i++ )
		m_Banner[i].ScaleToClipped( fWidth, fHeight );
}

void FadingBanner::Update( float fDeltaTime )
{
	// update children manually
	// ActorFrame::Update( fDeltaTime );
	Actor::Update( fDeltaTime );

	if( !m_bSkipNextBannerUpdate )
	{
		m_Banner[0].Update( fDeltaTime );
		m_Banner[1].Update( fDeltaTime );
	}

	m_bSkipNextBannerUpdate = false;

	/* Don't fade to the full banner until we finish fading. */
	float HighQualTime = FADE_SECONDS;

	/* Hacky: also don't fade until the music wheel has a chance to settle down. */
	HighQualTime = max( HighQualTime, 1.0f / PREFSMAN->m_iMusicWheelSwitchSpeed );
	HighQualTime = max( HighQualTime, (float)MW_SWITCH_SECONDS );
	
	if( m_sPendingBanner == "" || m_PendingTimer.PeekDeltaTime() < HighQualTime )
		return;

	/* Load the high quality banner. */
	CString banner = m_sPendingBanner;
	BeforeChange();
	m_Banner[GetBackIndex()].Load( banner );
}

void FadingBanner::DrawPrimitives()
{
	// draw manually
//	ActorFrame::DrawPrimitives();
	m_Banner[GetBackIndex()].Draw();
	m_Banner[m_iIndexFront].Draw();
}

bool FadingBanner::Load( RageTextureID ID )
{
	BeforeChange();
	bool bRet = m_Banner[m_iIndexFront].Load(ID);
	return bRet;
}

void FadingBanner::BeforeChange()
{
	m_Banner[m_iIndexFront].SetDiffuse( RageColor(1,1,1,1) );

	m_iIndexFront = GetBackIndex();

	m_Banner[m_iIndexFront].SetDiffuse( RageColor(1,1,1,1) );
	m_Banner[m_iIndexFront].StopTweening();
	m_Banner[m_iIndexFront].BeginTweening( FADE_SECONDS );		// fade out
	m_Banner[m_iIndexFront].SetDiffuse( RageColor(1,1,1,0) );

	m_sPendingBanner = "";

	/* We're about to load a banner.  It'll probably cause a frame skip or
	 * two.  Skip an update, so the fade-in doesn't skip. */
	m_bSkipNextBannerUpdate = true;
}

/* If this returns false, the banner couldn't be loaded. */
void FadingBanner::LoadFromCachedBanner( const CString &path )
{
	/* If we're already on the given banner, don't fade again. */
	if( path != "" && m_Banner[GetBackIndex()].GetTexturePath() == path )
		return;

	if( path == "" )
	{
		BeforeChange();
		m_Banner[GetBackIndex()].LoadFallback();
		return;
	}

	/* If we're currently fading to the given banner, go through this again,
	 * which will cause the fade-in to be further delayed. */

	/* No matter what we load, ensure we don't fade to a stale path. */
	m_sPendingBanner = "";

	RageTextureID ID;
	if( PREFSMAN->m_BannerCache == PrefsManager::BNCACHE_FULL )
		ID = Sprite::SongBannerTexture( path );
	else
		/* Try to load the low quality version. */
		ID = BANNERCACHE->LoadCachedBanner( path );

	if( !TEXTUREMAN->IsTextureRegistered(ID) )
	{
		/* Oops.  We couldn't load a banner quickly.  We can load the actual
		 * banner, but that's slow, so we don't want to do that when we're moving
		 * fast on the music wheel.  In that case, we should just keep the banner
		 * that's there (or load a "moving fast" banner).  Once we settle down,
		 * we'll get called again and load the real banner. */

		if( m_bMovingFast )
			return;

		BeforeChange();
// FIXME 			m_Banner[GetBackIndex()].LoadFromSong( pSong );
		if( IsAFile(path) )
			m_Banner[GetBackIndex()].Load( path );
		else
			m_Banner[GetBackIndex()].LoadFallback();

		return;
	}

	BeforeChange();
	m_Banner[GetBackIndex()].Load( ID );

	if( PREFSMAN->m_BannerCache != PrefsManager::BNCACHE_FULL )
	{
		m_sPendingBanner = path;
		m_PendingTimer.GetDeltaTime(); /* reset */
	}

	return;
}

void FadingBanner::LoadFromSong( const Song* pSong )
{
	/* Don't call HasBanner.  That'll do disk access and cause the music wheel
	 * to skip. */
	LoadFromCachedBanner( pSong->GetBannerPath() );
}

void FadingBanner::LoadAllMusic()
{
	BeforeChange();
	m_Banner[GetBackIndex()].LoadAllMusic();
}

void FadingBanner::LoadSort()
{
	BeforeChange();
	m_Banner[GetBackIndex()].LoadSort();
}

void FadingBanner::LoadMode()
{
	BeforeChange();
	m_Banner[GetBackIndex()].LoadMode();
}

void FadingBanner::LoadFromGroup( CString sGroupName )
{
	const CString sGroupBannerPath = SONGMAN->GetGroupBannerPath( sGroupName );
	LoadFromCachedBanner( sGroupBannerPath );
//	BeforeChange();
//	m_Banner[GetBackIndex()].LoadFromGroup( sGroupName );
}

void FadingBanner::LoadFromCourse( const Course* pCourse )
{
	LoadFromCachedBanner( pCourse->m_sBannerPath );
//	BeforeChange();
//	m_Banner[GetBackIndex()].LoadFromCourse( pCourse );
}

void FadingBanner::LoadRoulette()
{
	BeforeChange();
	m_Banner[GetBackIndex()].LoadRoulette();
}

void FadingBanner::LoadRandom()
{
	BeforeChange();
	m_Banner[GetBackIndex()].LoadRandom();
}

void FadingBanner::LoadFallback()
{
	BeforeChange();
	m_Banner[GetBackIndex()].LoadFallback();
}

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
