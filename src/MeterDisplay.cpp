#include "global.h"
/*
-----------------------------------------------------------------------------
 File: MeterDisplay.h

 Desc: The song's MeterDisplay displayed in SelectSong.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
-----------------------------------------------------------------------------
*/

#include "MeterDisplay.h"


MeterDisplay::MeterDisplay()
{
	this->AddChild( &m_sprStream );
}

void MeterDisplay::Load( CString sStreamPath, float fStreamWidth )
{
	m_fStreamWidth = fStreamWidth;

	m_sprStream.Load( sStreamPath );
	m_sprStream.SetZoomX( fStreamWidth / m_sprStream.GetUnzoomedWidth() );

	SetPercent( 0.5f );
}

void MeterDisplay::SetPercent( float fPercent )
{
	ASSERT( fPercent >= 0 && fPercent <= 1 );

	m_sprStream.SetCropRight( 1-fPercent );
}
