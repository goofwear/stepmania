/* WheelItemBase - An item on the wheel. */

#ifndef WHEELITEMBASE_H
#define WHEELITEMBASE_H

#include "ActorFrame.h"
#include "GradeDisplay.h"
#include "BitmapText.h"
#include "WheelNotifyIcon.h"
#include "TextBanner.h"
#include "GameConstantsAndTypes.h"
#include "GameCommand.h"

struct WheelItemBaseData;

enum WheelItemType 
{
	TYPE_GENERIC,
	TYPE_GROUP
};

class WheelItemBase : public ActorFrame
{
public:
	WheelItemBase();
	WheelItemBaseData* data;
	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	virtual void SetZTestMode( ZTestMode mode );
	virtual void SetZWrite( bool b );
	
	virtual void LoadFromWheelItemBaseData( WheelItemBaseData* pWID );

	float				m_fPercentGray;

	// for TYPE_GENERIC
	Sprite	m_sprBar;
	BitmapText m_text;

	WheelItemType m_Type;
	RageColor m_color;

	// for TYPE_SORT
	ActorFrame m_All;
};

struct WheelItemBaseData
{
	WheelItemBaseData() {}
	WheelItemBaseData( WheelItemType wit, CString sText, RageColor color );

	WheelItemType	m_Type;
	CString			m_sText;
	RageColor		m_color;	// either text color or section background color
	WheelNotifyIcon::Flags  m_Flags;

	// for TYPE_SORT
//	CString			m_sLabel;
//	GameCommand		m_Action;
};

#endif

/*
 * (c) 2001-2004 Chris Danford, Chris Gomez, Glenn Maynard, Josh Allen
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
