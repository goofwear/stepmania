/*
-----------------------------------------------------------------------------
 File: BitmapText.h

 Desc: A font class that draws characters from a bitmap.

 Copyright (c) 2001 Chris Danford.  All rights reserved.
-----------------------------------------------------------------------------
*/

#ifndef _BITMAPTEXT_H_
#define _BITMAPTEXT_H_


#include "Sprite.h"
#include "BitmapText.h"


const int NUM_CHARS = 256;


class BitmapText : public Sprite
{
protected:

public:
	BitmapText();

	virtual void RenderPrimitives();

	bool Load( CString sFontName );
	void SetText( CString sText );
	CString GetText();

	bool LoadFontWidths( CString sFilePath );

	float GetWidestLineWidthInSourcePixels();	// in logical, pre-zoomed units
	float GetLineWidthInSourcePixels( int iLineNo );

protected:
	bool LoadCharWidths( CString sWidthFilePath );

	CString m_sFontFilePath;
	int  m_iCharToFrameNo[NUM_CHARS];
	float m_fFrameNoToWidth[NUM_CHARS];	// in soure coordinate space
	
	CStringArray	m_sTextLines;
};


#endif