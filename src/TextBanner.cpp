#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 File: TextBanner.h

 Desc: The song's TextBanner displayed in SelectSong.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
-----------------------------------------------------------------------------
*/

#include "RageUtil.h"

#include "TextBanner.h"
#include "ThemeManager.h"



TextBanner::TextBanner()
{
	m_textTitle.Load( THEME->GetPathTo(FONT_TEXT_BANNER) );
	m_textSubTitle.Load( THEME->GetPathTo(FONT_TEXT_BANNER) );
	m_textArtist.Load( THEME->GetPathTo(FONT_TEXT_BANNER) );

	m_textTitle.SetX( -TEXT_BANNER_WIDTH/2 );
	m_textSubTitle.SetX( -TEXT_BANNER_WIDTH/2 );
	m_textArtist.SetX( -TEXT_BANNER_WIDTH/2 );

	m_textTitle.SetHorizAlign( align_left );
	m_textSubTitle.SetHorizAlign( align_left );
	m_textArtist.SetHorizAlign( align_left );

	m_textTitle.TurnShadowOff();
	m_textSubTitle.TurnShadowOff();
	m_textArtist.TurnShadowOff();

	this->AddActor( &m_textTitle );
	this->AddActor( &m_textSubTitle );
	this->AddActor( &m_textArtist );
}


bool TextBanner::LoadFromSong( Song* pSong )
{
	if( pSong == NULL )
	{
		m_textTitle.SetText( "" );
		m_textSubTitle.SetText( "" );
		m_textArtist.SetText( "" );
		return true;
	}

	CString sMainTitle = pSong->m_sMainTitle;
	CString sSubTitle = pSong->m_sSubTitle;

	m_textTitle.SetText( sMainTitle );
	m_textSubTitle.SetText( sSubTitle );
	m_textArtist.SetText( "/" + pSong->m_sArtist );

	
	float fTitleZoom, fSubTitleZoom, fArtistZoom;

	if( sSubTitle == "" )
	{
		fTitleZoom = 1.0f;
		fSubTitleZoom = 0.0f;
		fArtistZoom = 0.6f;
	}
	else
	{
		fTitleZoom = 1.0f;
		fSubTitleZoom = 0.5f;
		fArtistZoom = 0.6f;
	}

	m_textTitle.SetZoom( fTitleZoom );
	m_textSubTitle.SetZoom( fSubTitleZoom );
	m_textArtist.SetZoom( fArtistZoom );


	float fZoomedTitleWidth		=	m_textTitle.GetWidestLineWidthInSourcePixels() * fTitleZoom;
	float fZoomedSubTitleWidth	=	m_textSubTitle.GetWidestLineWidthInSourcePixels() * fSubTitleZoom;
	float fZoomedArtistWidth	=	m_textArtist.GetWidestLineWidthInSourcePixels() * fArtistZoom;

	// check to see if any of the lines run over the edge of the banner
	if( fZoomedTitleWidth > TEXT_BANNER_WIDTH )
		m_textTitle.SetZoomX( TEXT_BANNER_WIDTH / m_textTitle.GetWidestLineWidthInSourcePixels() );
	if( fZoomedSubTitleWidth > TEXT_BANNER_WIDTH )
		m_textSubTitle.SetZoomX( TEXT_BANNER_WIDTH / m_textSubTitle.GetWidestLineWidthInSourcePixels() );
	if( fZoomedArtistWidth > TEXT_BANNER_WIDTH )
		m_textArtist.SetZoomX( TEXT_BANNER_WIDTH / m_textArtist.GetWidestLineWidthInSourcePixels() );



	if( sSubTitle == "" )
	{
		m_textTitle.SetY( -8 );
		m_textSubTitle.SetY( 0 );
		m_textArtist.SetY( 8 );
	}
	else
	{
		m_textTitle.SetY( -10 );
		m_textSubTitle.SetY( 0 );
		m_textArtist.SetY( 10 );
	}

	return true;
}
