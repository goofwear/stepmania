#ifndef BITMAPTEXT_H
#define BITMAPTEXT_H
/*
-----------------------------------------------------------------------------
 File: BitmapText

 Desc: An actor that holds a Font and draws text to the screen.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


#include "Sprite.h"

class Font;

class BitmapText : public Actor
{
public:
	BitmapText();
	virtual ~BitmapText();


	bool LoadFromFont( CString sFontName );
	bool LoadFromNumbers( CString sTexturePath )	{ return LoadFromTextureAndChars(sTexturePath,"0123456789%. :x"); };
	bool LoadFromTextureAndChars( CString sTexturePath, CString sChars );
	void SetText( CString sText, CString sAlternateText = "", int iWrapWidthPixels = -1 );
	void SetTextMaxWidth( float MaxWidth, const CString &text, const CString &alttext = "" );
	void SetWrapWidthPixels( int iWrapWidthPixels );

	int GetWidestLineWidthInSourcePixels() { return m_iWidestLineWidth; };
	void CropToWidth( int iWidthInSourcePixels );

	virtual void DrawPrimitives();

	void TurnRainbowOn()	{ m_bRainbow = true; };
	void TurnRainbowOff()	{ m_bRainbow = false; };

	void SetHorizAlign( HorizAlign ha );
	void SetVertAlign( VertAlign va );

	CString GetText() const { return m_sText; }
	/* Return true if the string 's' will use an alternate string, if available. */
	bool StringWillUseAlternate(CString sText, CString sAlternateText) const;

	virtual void HandleCommand( const CStringArray &asTokens );

public:
	Font* m_pFont;

protected:
	
	// recalculate the items below on SetText()
	CString			m_sText;
	vector<wstring>	m_wTextLines;
	vector<int>		m_iLineWidths;			// in source pixels
	int				m_iWidestLineWidth;		// in source pixels
	int				m_iWrapWidthPixels;	// -1 = no wrap

	bool m_bRainbow;

	vector<RageSpriteVertex> verts;
	vector<RageTexture *> tex;
	
	void BuildChars();
	void DrawChars();
};


#endif
