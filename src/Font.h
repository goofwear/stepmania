#ifndef FONT_H
#define FONT_H
/*
-----------------------------------------------------------------------------
 File: Font

 Desc: A holder for information that is used by BitmapText objects.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "RageTexture.h"
#include "IniFile.h"

struct glyph {
	int width;
	/* X coordinates to be rendered for this character are X-left and X+right. */
	int left, right;

	/* Texture coordinate rect. */
	RectF rect;
};

class Font
{
public:
	Font( const CString &sASCIITexturePath );
	Font( const CString &sTexturePath, const CString& sChars );
	~Font();
	void Init();

	int GetLineWidthInSourcePixels( const CString &szLine );

	int m_iRefCount;

	CString m_sTexturePath;

	vector<glyph> glyphs;

	RageTexture* m_pTexture;
	bool m_bCapitalsOnly;
	int m_iLineSpacing;

	map<int,int> m_iCharToFrameNo;

	const glyph &GetGlyph( int frameNo ) const { return glyphs[frameNo]; }

private:
	void SetExtraPixels(int DrawExtraPixelsLeft, int DrawExtraPixelsRight);
	void SetTextureCoords(const vector<int> &widths);
	void Load( const CString &sASCIITexturePath, IniFile &ini );
};

#endif
